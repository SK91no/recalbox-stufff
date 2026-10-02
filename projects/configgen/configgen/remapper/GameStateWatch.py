#
# As part of the RECALBOX Project — http://www.recalbox.com
#
# RAM-watch helpers: read a game-state byte from the running libretro core via
# RetroArch's UDP network-command interface, and drive a menu<->race callback.
# Runs as a background thread inside configgen (which stays alive for the whole
# game, blocked in runCommand) — no separate process, no core patch. Used by
# Remapper to swap the R.G.P. `combos` rule set live for console racers on a
# JVS cabinet (e.g. Super Mario Kart: SYSTEM_RAM 0x148, 0x00 = menu, else race).
#
import socket
import time
from typing import Callable, Optional

MENU = "menu"
RACE = "race"


class NetCmd:
    """RetroArch UDP network-command client. Tries READ_CORE_MEMORY (absolute
    bus address) then falls back to READ_CORE_RAM (SYSTEM_RAM offset)."""

    def __init__(self, host: str = "127.0.0.1", port: int = 55355, timeout: float = 0.2):
        self._addr = (host, port)
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.settimeout(timeout)
        self._cmd: Optional[str] = None

    def _query(self, cmd: str, bus_addr: str, offset: str) -> Optional[int]:
        addr = bus_addr if cmd == "READ_CORE_MEMORY" else offset
        try:
            self._sock.sendto("{} {} 1".format(cmd, addr).encode(), self._addr)
            data, _ = self._sock.recvfrom(256)
        except OSError:
            return None
        parts = data.decode(errors="ignore").split()
        if len(parts) < 3 or parts[-1] == "-1":
            return None
        try:
            return int(parts[-1], 16)
        except ValueError:
            return None

    def read_byte(self, bus_addr: str, offset: str) -> Optional[int]:
        if self._cmd is None:
            for cmd in ("READ_CORE_MEMORY", "READ_CORE_RAM"):
                if self._query(cmd, bus_addr, offset) is not None:
                    self._cmd = cmd
                    break
        if self._cmd is None:
            return None
        return self._query(self._cmd, bus_addr, offset)

    def show_osd(self, text: str) -> None:
        """Flash an on-screen message via RetroArch's SHOW_MSG command (native,
        no reply). Best-effort: a dead socket must not disturb the watch loop."""
        if not text:
            return
        try:
            self._sock.sendto("SHOW_MSG {}".format(text).encode(), self._addr)
        except OSError:
            pass


def classify(value: Optional[int], menu_value: int) -> Optional[str]:
    if value is None:
        return None
    return MENU if value == menu_value else RACE


def watch(read_byte: Callable[[], Optional[int]],
          apply_state: Callable[[str, bool], None],
          menu_value: int,
          poll_hz: float = 25.0,
          stable_frames: int = 2,
          resync_seconds: float = 2.0,
          should_stop: Callable[[], bool] = lambda: False) -> None:
    """Poll the state byte; apply a state once it has been stable for
    `stable_frames` reads (debounces the 1-frame loading value).
    apply_state(state, changed) is called with changed=True on every genuine
    transition, and periodically re-called with changed=False (every
    `resync_seconds`) so the combos SELF-HEAL to the current state even if a
    transition edge was missed or the combos were overwritten. Runs until
    should_stop()."""
    period = 1.0 / poll_hz
    resync = max(1, int(resync_seconds * poll_hz))
    applied: Optional[str] = None
    candidate: Optional[str] = None
    count = 0
    since = 0
    while not should_stop():
        state = classify(read_byte(), menu_value)
        if state is not None:
            if state == candidate:
                count += 1
            else:
                candidate, count = state, 1
            if count >= stable_frames:
                since += 1
                if candidate != applied:
                    applied, since = candidate, 0
                    apply_state(applied, True)
                elif since >= resync:
                    since = 0
                    apply_state(applied, False)
        time.sleep(period)
