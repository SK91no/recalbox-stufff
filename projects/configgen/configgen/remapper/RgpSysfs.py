#
# As part of the RECALBOX Project
# http://www.recalbox.com
#
# Thin I/O layer over the R.G.P. (recalbox-gamepad-proxy) sysfs attributes.
# Isolated so the resolver logic stays pure and testable. The base path is
# injectable so tests can point at a temp directory.
#
from pathlib import Path


class RgpSysfs:
    def __init__(self, base: str = "/sys/class/recalbox_gamepad_proxy"):
        self._base = Path(base)

    @property
    def _combos(self) -> Path:
        return self._base / "combos"

    @property
    def _external_maps(self) -> Path:
        return self._base / "external_maps"

    def available(self) -> bool:
        # `combos` is the new attribute; its presence gates the whole feature.
        return self._combos.exists()

    # A zero-length write never reaches the kernel store (sysfs skips a 0-byte
    # write), so an empty set — the menu state and clear() — must be sent as a
    # newline, which the module strips back to "" and applies as "wipe".
    @staticmethod
    def _nonempty(value: str) -> str:
        return value if value else "\n"

    def push_combos(self, value: str) -> bool:
        return self._write(self._combos, self._nonempty(value))

    def push_external_maps(self, value: str) -> bool:
        return self._write(self._external_maps, self._nonempty(value))

    def clear(self) -> None:
        # Wipe every rule we may have pushed, so nothing leaks to the next game.
        self._write(self._combos, "\n")
        self._write(self._external_maps, "\n")

    @staticmethod
    def _write(path: Path, value: str) -> bool:
        try:
            path.write_text(value)
            return True
        except OSError:
            return False
