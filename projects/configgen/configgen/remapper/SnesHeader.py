#
# As part of the RECALBOX Project
# http://www.recalbox.com
#
# Reads a SNES rom's internal registration title (the 21-byte name the publisher
# burns into the cart header). Unlike a file hash it is identical across every
# dump/region/revision of a game, so it identifies a game robustly, independent
# of the rom filename. Pure byte logic lives in title_from_bytes(); the file
# wrapper handles .zip/.7z/raw and never raises (returns None so callers fall
# back to filename matching).
#
import os
import re
import shutil
import struct
import subprocess
import zipfile
from typing import List, Optional


class SnesHeader:
    _SMC_HEADER = 512               # copier header, present iff size % 1024 == 512
    _SLOTS = (0x7FC0, 0xFFC0, 0x40FFC0)  # registration area: LoROM / HiROM / ExHiROM
    _TITLE_LEN = 21
    _CKSUM_OFF = 0x1C               # complement (2B LE) then checksum (2B LE) at +0x1E
    _READ_CAP = 8 * 1024 * 1024     # enough to reach the ExHiROM header
    _ROM_EXTS = (".sfc", ".smc", ".fig", ".swc", ".bin")

    # -- pure logic -----------------------------------------------------------

    @classmethod
    def title_from_bytes(cls, data: Optional[bytes]) -> Optional[str]:
        """Extract the internal title from raw (decompressed) rom bytes, or None.
        Tries every memory-map slot and keeps the best: a valid header checksum
        outweighs a merely-printable one, so LoROM vs HiROM resolves itself."""
        if not data:
            return None
        if len(data) % 1024 == cls._SMC_HEADER:
            data = data[cls._SMC_HEADER:]

        best = None  # (score, title)
        for slot in cls._SLOTS:
            if slot + cls._TITLE_LEN > len(data):
                continue
            raw = data[slot:slot + cls._TITLE_LEN]
            title = cls._normalize(cls._decode(raw))
            if not title:
                continue
            printable = sum(1 for b in raw if 0x20 <= b < 0x7F)
            score = (2 if cls._checksum_ok(data, slot) else 0) + printable / cls._TITLE_LEN
            if best is None or score > best[0]:
                best = (score, title)
        return best[1] if best else None

    @classmethod
    def matches(cls, title: Optional[str], patterns: List[str]) -> bool:
        """True if a read title equals one of the profile's matchHeader entries
        (both normalized: collapsed whitespace, upper-cased)."""
        if not title:
            return False
        t = cls._normalize(title)
        return any(t == cls._normalize(p) for p in patterns)

    @classmethod
    def _checksum_ok(cls, data: bytes, slot: int) -> bool:
        off = slot + cls._CKSUM_OFF
        if off + 4 > len(data):
            return False
        comp, cksum = struct.unpack_from("<HH", data, off)
        return (comp ^ cksum) == 0xFFFF

    @staticmethod
    def _decode(raw: bytes) -> str:
        return "".join(chr(b) if 0x20 <= b < 0x7F else " " for b in raw)

    @staticmethod
    def _normalize(s: str) -> str:
        return re.sub(r"\s+", " ", s).strip().upper()

    # -- file/archive wrapper -------------------------------------------------

    @classmethod
    def title_from_file(cls, path: str) -> Optional[str]:
        """Best-effort title from a rom path (.zip/.7z/raw). Never raises."""
        try:
            return cls.title_from_bytes(cls._read_rom_bytes(path))
        except Exception:
            return None

    @classmethod
    def _read_rom_bytes(cls, path: str) -> Optional[bytes]:
        ext = os.path.splitext(path)[1].lower()
        if ext == ".zip":
            with zipfile.ZipFile(path) as z:
                name = cls._pick_member([i.filename for i in z.infolist() if not i.is_dir()])
                if name is None:
                    return None
                with z.open(name) as f:
                    return f.read(cls._READ_CAP)
        if ext == ".7z":
            return cls._read_7z(path)
        with open(path, "rb") as f:
            return f.read(cls._READ_CAP)

    @classmethod
    def _pick_member(cls, names: List[str]) -> Optional[str]:
        roms = [n for n in names if os.path.splitext(n)[1].lower() in cls._ROM_EXTS]
        if roms:
            return roms[0]
        return names[0] if names else None

    @classmethod
    def _read_7z(cls, path: str) -> Optional[bytes]:
        # No stdlib .7z reader; stream the archive's content via the 7z CLI.
        for tool in ("7zr", "7za", "7z"):
            exe = shutil.which(tool)
            if exe is None:
                continue
            p = subprocess.run([exe, "e", "-so", path], stdout=subprocess.PIPE,
                               stderr=subprocess.DEVNULL, timeout=30)
            if p.returncode == 0 and p.stdout:
                return p.stdout[:cls._READ_CAP]
        return None
