#
# As part of the RECALBOX Project
# http://www.recalbox.com
#
# Loads remap-profiles.json and resolves an input profile for a given game and
# hardware. Pure data logic: no sysfs, no side effect beyond reading the json
#
import json
from pathlib import Path
from typing import Any, Dict, List, Optional

from .SnesHeader import SnesHeader


class RemapProfileStore:
    # Bundled default data, shipped next to this module.
    DEFAULT_PATH = Path(__file__).with_name("remap-profiles.json")

    def __init__(self, path: Optional[str] = None):
        p = Path(path) if path is not None else self.DEFAULT_PATH
        self._data: Dict[str, Any] = json.loads(p.read_text()) if p.exists() else {}

    def resolve(self, system: str, rom: str, controller_type: str,
                cab_type: Optional[str] = None, rom_path: Optional[str] = None) -> dict:
        """Return the rules dict ({} = nothing to apply) for this game on this
        hardware. Two orthogonal dimensions:
          - byController: keyed on the stick type (arcade4/6/8), applies anywhere
          - jvs.byCabType: keyed on the JVS cab_type, JVS-only
        A matching jvs.byCabType entry wins over byController. A matched game
        replaces the system 'default' (no merge). rom_path (when given) lets an
        entry match on the rom's internal header title instead of its filename."""
        sys_block = self._data.get(system, {})
        game = self._match_game(sys_block, rom, rom_path)
        if game is None:
            game = sys_block.get("default", {})

        # 1) On JVS, a cab-specific entry wins (an empty entry = native, no rule).
        if cab_type is not None:
            for entry in game.get("jvs", {}).get("byCabType", []):
                if cab_type in entry.get("cabTypes", []):
                    return entry

        # 2) Otherwise, the controller-type base layer.
        for entry in game.get("byController", []):
            if controller_type in entry.get("types", []):
                return entry

        return {}

    def rotate_remaps(self) -> Dict[str, str]:
        """Face-button rotation (physical->target names) applied for TATE /
        vertical-screen games, from the top-level `_rotate` rule. Empty if absent."""
        return self._data.get("_rotate", {}).get("remaps", {})

    def rotate_axes(self) -> Dict[str, str]:
        """TATE analog-stick remap (joystick name -> retroarch axis) from `_rotate.axes`."""
        return self._data.get("_rotate", {}).get("axes", {})

    def rotate_invert_axes(self) -> List[str]:
        """Joystick names whose analog sign flips when rotated (`_rotate.invertAxes`)."""
        return self._data.get("_rotate", {}).get("invertAxes", [])

    @staticmethod
    def _match_game(sys_block: Dict[str, Any], rom: str,
                    rom_path: Optional[str] = None) -> Optional[dict]:
        # The rom's internal header title (read at most once, lazily) is the
        # robust identifier for console games on a cab; filename `match` is the
        # fallback (and the only key for arcade shortnames).
        title = None
        title_read = False
        for key, entry in sys_block.items():
            if key == "default":
                continue
            headers = entry.get("matchHeader")
            if headers and rom_path:
                if not title_read:
                    title = SnesHeader.title_from_file(rom_path)
                    title_read = True
                if SnesHeader.matches(title, headers):
                    return entry
            patterns = entry.get("match", [key])
            if rom in patterns or any(rom.startswith(p) for p in patterns):
                return entry
        return None
