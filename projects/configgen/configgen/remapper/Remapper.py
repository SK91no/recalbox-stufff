#
# As part of the RECALBOX Project
# http://www.recalbox.com
#
# Remapper — the single entry point for JVS/arcade input adaptation.
# Replaces the old getJvsMapping() whack-a-mole: it applies per-game button
# remaps (pure) and pushes per-game combos to the R.G.P. `combos` sysfs (I/O).
# All arcade-profile knowledge lives here and in the sibling modules so the
# generators/launcher stay thin.
#
from typing import Dict, Optional, Tuple

from configgen.controllers.controller import Controller
from configgen.controllers.inputItem import InputItem

from .RemapProfileStore import RemapProfileStore
from .RgpComboResolver import RgpComboResolver, EV_KEY, EV_ABS
from .RgpSysfs import RgpSysfs

# evdev ABS_HAT0X / ABS_HAT0Y (dpads exposed as a hat).
_ABS_HAT0X = 16
_ABS_HAT0Y = 17

# Proxied JVS pad name the R.G.P. combos target (substring match).
_JVS_TARGET = "RecalboxJVS Gamepad #1"

# Logical names a combo may reference; translated to (ev_type, ev_code, dir).
_COMBO_INPUTS = ("start", "select", "up", "down", "left", "right",
                 "east", "south", "north", "west", "eastprime", "northprime",
                 "l1", "r1", "l2", "r2", "l3", "r3")

# The RAM-watch runs as a background thread inside the live configgen process
# (blocked in runCommand for the whole game). Module-level so clear(), called
# from a fresh Remapper() in emulatorlauncher's finally, can stop it.
_watch_thread = None
_watch_stop = None


def jvs_cab_type() -> str:
    """Cabinet model reported by the JVS driver (e.g. 'crazy_taxi'), '' if none."""
    try:
        with open("/sys/devices/platform/recalboxjvs/cab_type") as f:
            return f.read().strip()
    except OSError:
        return ""


class Remapper:
    # Base 1:1 JVS button map (logical InputItem -> RetroArch name).
    BASE: Dict[int, str] = {
        InputItem.ItemEast: 'a', InputItem.ItemSouth: 'b',
        InputItem.ItemNorth: 'x', InputItem.ItemWest: 'y',
        InputItem.ItemL1: 'l', InputItem.ItemR1: 'r',
        InputItem.ItemL2: 'l2', InputItem.ItemR2: 'r2',
        InputItem.ItemL3: 'l3', InputItem.ItemR3: 'r3',
        InputItem.ItemStart: 'start', InputItem.ItemSelect: 'select',
    }

    def __init__(self, store: Optional[RemapProfileStore] = None, rgp: Optional[RgpSysfs] = None):
        self._store = store if store is not None else RemapProfileStore()
        self._rgp = rgp if rgp is not None else RgpSysfs()

    def button_map(self, system: str, rom: str, controller: Controller,
                   cab_type: Optional[str], rom_path: str = "",
                   base: Optional[Dict[int, str]] = None, rotate: bool = False) -> Dict[int, str]:
        """Overlay onto `base` (default: the 1:1 BASE map), in order: the TATE
        face-button rotation (when `rotate`), then this game's remap-profile remaps;
        and push the game's combos to the R.G.P. (side effect, no-op if absent).
        Used for JVS cabs AND plain arcade sticks; byCabType rules only fire when
        cab_type is set (JVS), byController applies for any arcade stick.
        rom_path (full path) enables header-based game identification."""
        buttons = dict(self.BASE if base is None else base)
        names = InputItem.ConverterNameToItem

        if rotate:
            for phys, target in self._store.rotate_remaps().items():
                if phys in names:
                    buttons[names[phys]] = target

        profile = self._store.resolve(system, rom, controller.GamepadType.Type.value,
                                      cab_type, rom_path or None)
        for phys, target in profile.get("remaps", {}).items():
            if phys in names:
                buttons[names[phys]] = target

        # A game with per-state profiles (console racer on a JVS cab, e.g. Super
        # Mario Kart) drives its combos live from a RAM-watch daemon instead of a
        # single static push.
        if "states" in profile and "ramwatch" in profile:
            self._setup_state_watch(profile, controller)
        else:
            self._push_combos(profile.get("combos", []), controller)
        return buttons

    def rotate_axes(self) -> Dict[int, str]:
        """TATE analog-stick remap {InputItem: retroarch axis name} for a rotated
        screen, resolved from the `_rotate.axes` data. Empty if absent."""
        names = InputItem.ConverterNameToItem
        return {names[n]: ax for n, ax in self._store.rotate_axes().items() if n in names}

    def rotate_invert_axes(self) -> set:
        """InputItems whose analog sign flips when the screen is rotated
        (`_rotate.invertAxes`)."""
        names = InputItem.ConverterNameToItem
        return {names[n] for n in self._store.rotate_invert_axes() if n in names}

    def clear(self) -> None:
        """Stop any RAM-watch daemon and wipe the combos we pushed, so nothing
        leaks to the next game. Safe no-op when the R.G.P. is absent."""
        self._kill_watch()
        self._rgp.push_combos("")

    def _push_combos(self, combos: list, controller: Controller) -> None:
        if not self._rgp.available():
            return
        # Push only when this controller has combos; never clear on empty. Combos
        # are a single global R.G.P. state written once per controller, so a pad
        # with no rules (e.g. a plain gamepad on a JVS cab) must NOT wipe the combos
        # another controller pushed. Stale combos from a prior game are cleared once
        # up front (see clear_combos()) and at game exit.
        if not combos:
            return
        table = self._ev_table(controller)
        self._rgp.push_combos(RgpComboResolver(table, _JVS_TARGET).combos_to_sysfs(combos))

    def clear_combos(self) -> None:
        """Wipe the R.G.P. combos once (stale from a prior game), without touching
        any running watch. Call before per-controller processing repopulates them."""
        self._rgp.push_combos("")

    def _setup_state_watch(self, profile: dict, controller: Controller) -> None:
        """Render the per-state combo sets and start the RAM-watch thread that
        swaps them live on the game's menu<->race transitions. No-op if absent."""
        global _watch_thread, _watch_stop
        import threading
        from .GameStateWatch import NetCmd, watch, MENU, RACE
        self._kill_watch()
        if not self._rgp.available():
            return
        resolver = RgpComboResolver(self._ev_table(controller), _JVS_TARGET)
        states = profile.get("states", {})
        rendered = {
            MENU: resolver.combos_to_sysfs(states.get("menu", {}).get("combos", [])),
            RACE: resolver.combos_to_sysfs(states.get("race", {}).get("combos", [])),
        }
        rw = profile.get("ramwatch", {})
        bus_addr, offset = str(rw.get("busAddr", "7e0148")), str(rw.get("sysramOffset", "148"))
        menu_value = int(rw.get("menuValue", 0))
        # OSD notice flashed in RetroArch on each switch; per-state `osd` overrides.
        labels = {MENU: "Menu", RACE: "Race"}
        osd = {st: states.get(st, {}).get(
                   "osd", "Recalbox JVS: switched controls to {} Mode".format(labels[st]))
               for st in (MENU, RACE)}
        rgp, stop = self._rgp, threading.Event()

        # Start from the safe (menu/native) state so stale combos from a previous
        # game never linger before the daemon's first successful RAM read.
        rgp.push_combos(rendered.get(MENU, ""))

        def run():
            nc = NetCmd()
            fails = 0

            def read():
                nonlocal nc, fails
                v = nc.read_byte(bus_addr, offset)
                if v is None:
                    fails += 1
                    if fails >= 25:  # ~1s of dead reads: socket may be stale, rebuild it
                        nc, fails = NetCmd(), 0
                else:
                    fails = 0
                return v

            def apply_state(state, changed):
                rgp.push_combos(rendered.get(state, ""))
                if changed:
                    nc.show_osd(osd.get(state, ""))

            watch(read, apply_state, menu_value=menu_value, should_stop=stop.is_set)

        _watch_stop = stop
        _watch_thread = threading.Thread(target=run, name="rgp-ramwatch", daemon=True)
        _watch_thread.start()

    def _kill_watch(self) -> None:
        global _watch_thread, _watch_stop
        if _watch_stop is not None:
            _watch_stop.set()
        if _watch_thread is not None:
            _watch_thread.join(timeout=1.0)
        _watch_stop = _watch_thread = None

    @staticmethod
    def _ev_table(controller: Controller) -> Dict[str, Tuple[int, int, int]]:
        names = InputItem.ConverterNameToItem
        table: Dict[str, Tuple[int, int, int]] = {}
        for name in _COMBO_INPUTS:
            if name not in names:
                continue
            item = names[name]
            if not controller.HasInput(item):
                continue
            ev = Remapper._input_to_ev(name, controller.Input(item))
            if ev is not None:
                table[name] = ev
        return table

    @staticmethod
    def _input_to_ev(name: str, inp: InputItem) -> Optional[Tuple[int, int, int]]:
        # trig_val carries the axis direction for EV_ABS, else 1 for EV_KEY.
        if inp.IsButton or inp.IsKey:
            return (EV_KEY, inp.Code, 1)
        if inp.IsAxis:
            return (EV_ABS, inp.Code, -1 if inp.Value < 0 else 1)
        if inp.IsHat:
            # A hat's Value is an SDL bitmask, not a signed axis: derive the
            # ABS_HAT axis+direction from the logical name instead.
            axis = _ABS_HAT0Y if name in ("up", "down") else _ABS_HAT0X
            sign = -1 if name in ("up", "left") else 1
            return (EV_ABS, axis, sign)
        return None
