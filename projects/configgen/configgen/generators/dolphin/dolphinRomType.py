#!/usr/bin/env python3

import os
import re
import struct


class DolphinRomType:
    @staticmethod
    def extract_gameid_from_rvz(game_path) -> str | None:
        candidates = {}
        with open(game_path, 'rb') as f:
            # Search in first file sectors
            search_zones = [
                (0x0, 1024), (0x40, 1024), (0x100, 2048),
                (0x400, 2048), (0x1000, 4096), (0x8000, 8192), (0x20000, 16384)
            ]

            for start_offset, size in search_zones:
                f.seek(start_offset)
                data = f.read(size)

                for i in range(len(data) - 5):
                    try:
                        candidate = data[i:i + 6].decode('ascii')
                        if len(candidate) == 6 and bool(re.match(r'^[GRSD][A-Z0-9]{2}[EJPKFIXD][A-Z0-9]{2}$', candidate)):
                            candidates[candidate] = candidates.get(candidate, 0) + 1
                    except UnicodeDecodeError:
                        continue

        if not candidates:
            return None

        # Use the more frequent Game ID
        game_id = max(candidates.items(), key=lambda x: x[1])[0]

        return game_id

    @staticmethod
    def extract_gameid_from_iso(game_path) -> str | None:
        with open(game_path, 'rb') as f:
            # Game ID is ALWAYS at the first 6 bytes of GameCube/Wii ISOs
            f.seek(0x00)
            game_id = f.read(6).decode('ascii', errors='ignore')

            if len(game_id) == 6 and game_id.isalnum():
                return game_id
            else:
                return None

    @staticmethod
    def extract_gameid_from_wbfs(game_path) -> str | None:
        with open(game_path, 'rb') as f:
            # Read WBFS header (512 bytes)
            f.seek(0)
            wbfs_header = f.read(512)

            # Verify WBFS signature
            if wbfs_header[:4] != b'WBFS':
                return None

            # Game ID is generally after WBFS header
            offsets_to_try = [
                0x200,  # Standard offset
                0x218,  # Alternative offset
                0x400,
                0x800,
            ]

            for offset in offsets_to_try:
                f.seek(offset)
                potential_gameid = f.read(6)

                try:
                    game_id = potential_gameid.decode('ascii')

                    if (len(game_id) == 6 and
                            game_id.isalnum() and
                            game_id[0] in 'GRSD' and
                            game_id[3] in 'EJPKFIXD'):
                        return game_id
                except UnicodeDecodeError:
                    continue

            # If GameID is not found at standard offsets, scan wider
            f.seek(0x200)
            scan_data = f.read(4096)  # Scan 4KB after header

            for i in range(len(scan_data) - 5):
                try:
                    candidate = scan_data[i:i + 6].decode('ascii')
                    if (len(candidate) == 6 and
                            candidate.isalnum() and
                            candidate[0] in 'GRSD' and
                            candidate[3] in 'EJPKFIXD'):
                        return candidate
                except UnicodeDecodeError:
                    continue

            return None

    @staticmethod
    def extract_gameid_from_ciso(game_path) -> str | None:
        with open(game_path, 'rb') as f:
            # Read CISO header
            f.seek(0)
            header = f.read(8)

            # Verify CISO signature
            if header[:4] != b'CISO':
                return None

            # For non-standard CISO, search directly the Game ID
            # into multiples zones of the file
            search_zones = [
                (0x20, 1024),  # After signature
                (0x100, 2048),  # Classical zone
                (0x800, 4096),
                (0x2000, 8192),
            ]

            for start_offset, size in search_zones:
                f.seek(start_offset)
                data = f.read(size)

                for i in range(len(data) - 5):
                    try:
                        candidate = data[i:i + 6].decode('ascii')

                        if (len(candidate) == 6 and
                                candidate.isalnum() and
                                candidate[0] in 'GRSD' and
                                candidate[3] in 'EJPKFIXD'):
                            return candidate
                    except UnicodeDecodeError:
                        continue

            # If nothing has been found, scan larger
            f.seek(0x20)  # Skip header
            chunk_size = 32768

            for chunk_start in range(0x20, min(1024 * 1024, f.seek(0, 2)), chunk_size):
                f.seek(chunk_start)
                data = f.read(chunk_size)

                for i in range(len(data) - 5):
                    try:
                        candidate = data[i:i + 6].decode('ascii')
                        if (len(candidate) == 6 and
                                candidate.isalnum() and
                                candidate[0] in 'GRSD' and
                                candidate[3] in 'EJPKFIXD'):
                            return candidate
                    except UnicodeDecodeError:
                        continue

            return None

    @staticmethod
    def extract_gameid_from_wad(game_path) -> str | None:
        with open(game_path, 'rb') as f:
            # Read WAD header (64 bytes)
            f.seek(0)
            wad_header = f.read(64)

            # Verify WAD signature
            if wad_header[:4] != b'\x00\x00\x00\x20':
                return None

            # Parse WAD header
            cert_size = struct.unpack('>I', wad_header[8:12])[0]
            ticket_size = struct.unpack('>I', wad_header[16:20])[0]

            # Calculate TMD offset
            cert_offset = 64
            ticket_offset = cert_offset + ((cert_size + 63) // 64) * 64
            tmd_offset = ticket_offset + ((ticket_size + 63) // 64) * 64

            # Try multiple offsets for Title ID in TMD
            title_id_offsets = [0x4C, 0x1C, 0x64, 0x18C]

            for tid_offset in title_id_offsets:
                try:
                    f.seek(tmd_offset + tid_offset)
                    title_id_bytes = f.read(8)

                    if len(title_id_bytes) == 8:
                        title_id = struct.unpack('>Q', title_id_bytes)[0]

                        if 0 < title_id < 0xFFFFFFFFFFFFFFFF:
                            title_id_hex = f"{title_id:016x}".upper()

                            # Check only Title IDs starting with 00010001
                            if not title_id_hex.startswith('00010001'):
                                continue

                            # Read more context to capture separated parts
                            f.seek(tmd_offset + tid_offset - 32)
                            context_data = f.read(128)

                            ascii_context = context_data.decode('ascii', errors='replace')

                            # Extract base from Title ID (4 characters)
                            try:
                                ascii_part = bytes.fromhex(title_id_hex[8:])[:4].decode('ascii', errors='ignore')

                                # Search base in context
                                base_pos = ascii_context.find(ascii_part)

                                if base_pos != -1:
                                    # Search suffix after base
                                    # Suffix can be separated by null bytes
                                    after_base = ascii_context[base_pos + 4:]

                                    # Search a pattern with 2 alphanum characters
                                    suffix_match = re.search(r'([A-Z0-9]{2})', after_base)

                                    if suffix_match:
                                        suffix = suffix_match.group(1)
                                        game_id = ascii_part + suffix

                                        # We have finally the GameID
                                        return game_id
                            except Exception as e:
                                print(f"Extraction error: {e}")
                                continue

                except Exception as e:
                    print(f"Error at offset {tid_offset}: {e}")
                    continue

        return None

    @staticmethod
    def extract_gameid_from_m3u(game_path) -> str | None:
        try:
            with open(game_path, 'r') as f:
                first_line = f.readline().strip()
                game_fullpath = os.path.dirname(game_path)
                game_id = DolphinRomType.extract_gameid(str(game_fullpath) + '/' + first_line)

                return game_id
        except Exception as e:
            print("Error while reading file: {}".format(e))
            return None

    @staticmethod
    def extract_gameid(game_path) -> str | None:
        # Extract extension from game path
        extension = os.path.splitext(os.path.basename(game_path))[1][1:]

        # Check if it corresponds to a known extension
        match extension:
            case 'ciso':
                return DolphinRomType.extract_gameid_from_ciso(game_path)
            case 'gc' | 'gcm' | 'iso':
                return DolphinRomType.extract_gameid_from_iso(game_path)
            case 'm3u':
                return DolphinRomType.extract_gameid_from_m3u(game_path)
            case 'rvz':
                return DolphinRomType.extract_gameid_from_rvz(game_path)
            case 'wad':
                return DolphinRomType.extract_gameid_from_wad(game_path)
            case 'wbfs':
                return DolphinRomType.extract_gameid_from_wbfs(game_path)

        return None
