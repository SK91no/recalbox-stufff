#
# As part of the RECALBOX Project
# http://www.recalbox.com
#
# Pure translator: logical combo rules + a name->(ev_type, ev_code, trig_val)
# table for the proxied pad -> the RGP `combos` sysfs string. No I/O.
#
# combos sysfs grammar (';'-separated):
#   <target>:<mod-code>:<trig-type>:<trig-code>:<trig-val>:<emit-code>:<mode>
#
# The direction of a directional trigger (up/down/...) is NOT repeated in the
# combo: it is carried by the name table's trig_val (built from the pad's
# InputItem sign), so "up" already means the right axis + direction.
#
from typing import Dict, List, Tuple

EV_KEY = 1
EV_ABS = 3


class RgpComboResolver:
    def __init__(self, name_to_ev: Dict[str, Tuple[int, int, int]], target: str):
        # Logical input name (case-insensitive) -> (ev_type, ev_code, trig_val).
        # trig_val is the axis direction (+/-1) for EV_ABS, else 1 for EV_KEY.
        self._ev = {k.lower(): v for k, v in name_to_ev.items()}
        self._target = target

    def combos_to_sysfs(self, combos: List[dict]) -> str:
        entries = []
        for c in combos:
            # An input the pad does not expose, or a name a profile got wrong: drop that
            # one combo with a word in the log instead of aborting the whole launch.
            unknown = [c[f] for f in ("modifier", "trigger", "emit")
                       if c.get(f) and c[f].lower() not in self._ev]
            if unknown:
                print("warning: combo {} skipped, unknown input(s) {}".format(c, unknown))
                continue
            # No "modifier" = a plain remap (mod-code 0): the R.G.P. fires it on
            # the trigger alone, still suppressing the source. Used by the
            # state-switched console profiles (e.g. Super Mario Kart).
            mod_code = self._ev[c["modifier"].lower()][1] if c.get("modifier") else 0
            trig_type, trig_code, trig_val = self._ev[c["trigger"].lower()]
            _, emit_code, _ = self._ev[c["emit"].lower()]
            entries.append(
                "{t}:{mod}:{tt}:{tc}:{tv}:{emit}:{mode}".format(
                    t=self._target, mod=mod_code, tt=trig_type, tc=trig_code,
                    tv=trig_val, emit=emit_code, mode="hold"))
        return ";".join(entries)
