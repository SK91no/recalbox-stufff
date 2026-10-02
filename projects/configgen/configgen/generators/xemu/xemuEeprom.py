"""
XBOX EEPROM editing utility

From https://github.com/abaire/pyxbeeprom
Based on https://github.com/mborgerson/xbeeprom
"""
#!/usr/bin/env python3
import logging
import os
import configgen.recalboxFiles as recalboxFiles
from configgen.settings.iniSettings import IniSettings
from configgen.generators.xemu.xk.eeprom import EEPROM, XboxEepromData, XboxVersion, XboxRegion, XboxVideo, XboxTimeZone, XboxDvdZone, XboxLanguage


class XemuEeprom:
    eepromFile = recalboxFiles.SAVES + "/xbox/xemu_eeprom.bin"
    eepromConfigFile = recalboxFiles.CONF + '/xemu/eeprom_config.ini'

    @staticmethod
    def create_default_eeprom(region: int, video_standard: int, timezone: int, dvd_zone: int, language: int) -> EEPROM:
        """
        Create a default EEPROM with basic configuration.

        :param region: Region number
        :param video_standard: Video standard
        :param timezone: Timezone number
        :param dvd_zone: DVD zone number
        :param language: Language number

        :return: EEPROM object
        """
        eeprom = EEPROM()

        # Initialize with default Xbox EEPROM structure
        eeprom._data = XboxEepromData()
        eeprom._encrypted = False
        eeprom.data._encrypted = False  # Important: also set at data level
        eeprom._version = XboxVersion.V1_0

        logging.basicConfig(level=logging.DEBUG)
        logger = logging.getLogger(__name__)
        logger.info(f"Creating default EEPROM for region {region}")

        # Set basic configuration
        eeprom.data.XBERegion = region
        eeprom.data.VideoStandard = video_standard
        eeprom.data.TimeZoneBias = timezone
        eeprom.data.DVDPlaybackKitZone = dvd_zone
        eeprom.data.LanguageID = language

        # Set default video flags (480p enabled)
        eeprom.data.VideoFlags = (1 << 19)  # 480p enabled

        # Set default audio flags (stereo)
        eeprom.data.AudioFlags = 0  # Stereo mode (no mono/surround bits set)

        # Set default timezone DST settings (disabled)
        eeprom.data.TimeZoneStdDate = 0
        eeprom.data.TimeZoneDltDate = 0
        eeprom.data.TimeZoneStdBias = 0
        eeprom.data.TimeZoneDltBias = 0

        # Generate random MAC address using Xbox-specific prefixes
        import random
        xbox_mac_prefixes = [
            bytes.fromhex("0050F2"),  # 1.0+
            bytes.fromhex("000D3A"),  # 1.1+
            bytes.fromhex("00125A"),  # 1.6
        ]

        selected_prefix = random.choice(xbox_mac_prefixes)
        random_suffix = bytes([random.randint(0, 255) for _ in range(3)])
        default_mac = selected_prefix + random_suffix
        for i, byte in enumerate(default_mac):
            eeprom.data.MACAddress[i] = byte

        # Generate random serial number (11 digits + '9' at end)
        serial_digits = [str(random.randint(0, 9)) for _ in range(11)]
        serial_digits.append('9')  # Last digit always 9
        default_serial = ''.join(serial_digits).encode('ascii')
        for i, byte in enumerate(default_serial):
            eeprom.data.SerialNumber[i] = byte

        # Generate random confounder (8 bytes)
        default_confounder = bytes([random.randint(0, 255) for _ in range(8)])
        for i, byte in enumerate(default_confounder):
            eeprom.data.Confounder[i] = byte

        # Generate random HDD key (16 bytes)
        default_hddkey = bytes([random.randint(0, 255) for _ in range(16)])
        for i, byte in enumerate(default_hddkey):
            eeprom.data.HDDKey[i] = byte

        # Generate random online key (16 bytes)
        default_onlinekey = bytes([random.randint(0, 255) for _ in range(16)])
        for i, byte in enumerate(default_onlinekey):
            eeprom.data.OnlineKey[i] = byte

        # Initialize UNKNOWN fields that are part of checksums
        for i in range(2):
            eeprom.data.UNKNOWN2[i] = 0
        for i in range(4):
            eeprom.data.UNKNOWN3[i] = 0
        for i in range(8):
            eeprom.data.UNKNOWN4[i] = 0
        for i in range(8):
            eeprom.data.UNKNOWN5[i] = 0
        for i in range(64):
            eeprom.data.UNKNOWN6[i] = 0

        # Initialize timezone name fields
        eeprom.data.TimeZoneStdName = 0
        eeprom.data.TimeZoneDltName = 0

        # Set parental control defaults
        # Those must stay to 0 or Xemu won't be able to read any disc
        eeprom.data.ParentalControlGames = 0
        eeprom.data.ParentalControlMovies = 0
        eeprom.data.ParentalControlPwd = 0

        # Set Xbox Live settings to defaults
        eeprom.data.XBOXLiveIPAddress = 0
        eeprom.data.XBOXLiveDNS = 0
        eeprom.data.XBOXLiveGateWay = 0
        eeprom.data.XBOXLiveSubNetMask = 0
        eeprom.data.OtherSettings = 0

        # Initialize checksums to 0 (they'll be calculated during encryption)
        eeprom.data.Checksum2 = 0
        eeprom.data.Checksum3 = 0

        # Initialize HMAC SHA1 Hash to zeros (will be calculated during encryption)
        for i in range(20):
            eeprom.data.HMAC_SHA1_Hash[i] = 0

        return eeprom

    def eepromConfiguration(self) -> None:
        """
        Configuration of the Xbox EEPROM.
        """
        # Load EEPROM settings
        eepromSettings = IniSettings(self.eepromConfigFile, True)
        eepromSettings.loadFile(True)

        # Get all values to write from the .ini file
        region = eepromSettings.getInt('general', 'region', 1)
        video_standard = eepromSettings.getInt('general', 'video_standard', 4194560)
        timezone = eepromSettings.getInt('general', 'timezone', 4294967176)
        disable_dst = eepromSettings.getInt('general', 'disable_dst', 0)
        dvd_zone = eepromSettings.getInt('general', 'dvd_zone', 0)
        language = eepromSettings.getInt('general', 'language', 1)

        resolution_480p = eepromSettings.getInt('video', 'resolution_480p', 1)
        resolution_720p = eepromSettings.getInt('video', 'resolution_720p', 0)
        resolution_1080i = eepromSettings.getInt('video', 'resolution_1080i', 0)
        video_mode = eepromSettings.getInt('video', 'video_mode', 0)
        refresh_rate = eepromSettings.getInt('video', 'refresh_rate', 0)

        audio_output = eepromSettings.getInt('audio', 'audio_output', 1)
        ac3 = eepromSettings.getInt('audio', 'ac3', 0)
        dts = eepromSettings.getInt('audio', 'dts', 0)

        # Safety check: ensure at least one resolution is enabled, default to 480p if all are disabled
        if resolution_480p == 0 and resolution_720p == 0 and resolution_1080i == 0:
            resolution_480p = 1

        logging.basicConfig(level=logging.DEBUG)
        logger = logging.getLogger(__name__)

        # Load or create EEPROM
        if os.path.exists(self.eepromFile):
            logger.info(f"Loading existing EEPROM from {self.eepromFile}")
            eeprom = EEPROM()
            try:
                eeprom.read_from_bin_file(self.eepromFile)
                logger.info("Successfully loaded existing EEPROM")
            except Exception as e:
                logger.warning(f"Failed to decrypt existing EEPROM ({e}), creating new one...")
                # Delete the corrupted EEPROM
                os.remove(self.eepromFile)
                # Create a fresh EEPROM
                eeprom = XemuEeprom.create_default_eeprom(region, video_standard, timezone, dvd_zone, language)
        else:
            logger.info(f"EEPROM file not found, creating new EEPROM with default values")
            # Create directory if it doesn't exist
            os.makedirs(os.path.dirname(self.eepromFile), exist_ok=True)
            eeprom = XemuEeprom.create_default_eeprom(region, video_standard, timezone, dvd_zone, language)

        # Log current state
        current_region = eeprom.data.XBERegion
        current_video_standard = eeprom.data.VideoStandard
        current_timezone = eeprom.data.TimeZoneBias
        current_dvd_zone = eeprom.data.DVDPlaybackKitZone
        current_language = eeprom.data.LanguageID

        # Extract video flags manually (bit manipulation)
        video_flags = eeprom.data.VideoFlags
        current_widescreen = (video_flags >> 16) & 1
        current_letterbox = (video_flags >> 20) & 1
        current_resolution_480p = (video_flags >> 19) & 1
        current_resolution_720p = (video_flags >> 17) & 1
        current_resolution_1080i = (video_flags >> 18) & 1
        current_refresh_rate = (video_flags >> 23) & 1

        # Extract audio flags manually (bit manipulation)
        audio_flags = eeprom.data.AudioFlags
        current_mono = audio_flags & 1
        current_surround = (audio_flags >> 1) & 1
        current_ac3 = (audio_flags >> 16) & 1
        current_dts = (audio_flags >> 17) & 1

        video_changed = False
        audio_changed = False

        # Modify the eeprom if chosen values are different from stored values in eeprom
        # Region
        if current_region != region:
            logger.info(f"Changing region from {XboxRegion(current_region).name} ({current_region}) to {XboxRegion(region).name} ({region})")
            eeprom.data.XBERegion = region
        else:
            logger.info(f"Region already set to {region}, no change needed")

        # Video Standard
        if current_video_standard != video_standard:
            logger.info(f"Changing video standard from {XboxVideo(current_video_standard).name} ({hex(current_video_standard)}) to {XboxVideo(video_standard).name} ({hex(video_standard)})")
            eeprom.data.VideoStandard = video_standard
        else:
            logger.info(f"Video standard already set to {hex(video_standard)}, no change needed")

        # Timezone
        if current_timezone != timezone:
            logger.info(f"Changing timezone from {XboxTimeZone(current_timezone).name} ({current_timezone}) to {XboxTimeZone(timezone).name} ({timezone})")
            eeprom.data.TimeZoneBias = timezone
        else:
            logger.info(f"Timezone already set to {XboxTimeZone(timezone).name} ({timezone}), no change needed")

        # DST (Daylight Saving Time) configuration
        if disable_dst:
            logger.info("Disabling automatic DST adjustment")
            # Disable DST by setting DST dates and bias to zero
            eeprom.data.TimeZoneStdDate = 0
            eeprom.data.TimeZoneDltDate = 0 
            eeprom.data.TimeZoneStdBias = 0
            eeprom.data.TimeZoneDltBias = 0
        else:
            # Keep default DST settings (or current EEPROM values)
            logger.info("Automatic DST adjustment enabled (using EEPROM defaults)")

        # DVD zone
        if current_dvd_zone != dvd_zone:
            logger.info(f"Changing DVD zone from {XboxDvdZone(current_dvd_zone).name} ({current_dvd_zone}) to {XboxDvdZone(dvd_zone).name} ({dvd_zone})")
            eeprom.data.DVDPlaybackKitZone = dvd_zone
        else:
            logger.info(f"DVD zone already set to {dvd_zone}, no change needed")

        # Language
        if current_language != language:
            logger.info(f"Changing language from {XboxLanguage(current_language).name} ({current_language}) to {XboxLanguage(language).name} ({language})")
            eeprom.data.LanguageID = language
        else:
            logger.info(f"Language already set to {language}, no change needed")

        # Set video mode (0=normal, 1=letterbox, 2=widescreen)
        current_video_mode = 0

        if current_widescreen:
            current_video_mode = 2
        elif current_letterbox:
            current_video_mode = 1

        if current_video_mode != video_mode:
            logger.info(f"Changing video mode from {current_video_mode} to {video_mode}")

            # Clear both widescreen and letterbox flags
            video_flags &= ~(1 << 16)  # Clear widescreen
            video_flags &= ~(1 << 20)  # Clear letterbox

            # Set the appropriate flag based on video_mode
            if video_mode == 2:  # Widescreen
                video_flags |= (1 << 16)
            elif video_mode == 1:  # Letterbox
                video_flags |= (1 << 20)
            # video_mode == 0 means normal (both flags cleared)

            video_changed = True

        if current_resolution_480p != resolution_480p:
            logger.info(f"Changing resolution 480p from {current_resolution_480p} to {resolution_480p}")

            if resolution_480p:
                video_flags |= (1 << 19)
            else:
                video_flags &= ~(1 << 19)

            video_changed = True

        if current_resolution_720p != resolution_720p:
            logger.info(f"Changing resolution 720p from {current_resolution_720p} to {resolution_720p}")

            if resolution_720p:
                video_flags |= (1 << 17)
            else:
                video_flags &= ~(1 << 17)

            video_changed = True

        if current_resolution_1080i != resolution_1080i:
            logger.info(f"Changing resolution 1080i from {current_resolution_1080i} to {resolution_1080i}")

            if resolution_1080i:
                video_flags |= (1 << 18)
            else:
                video_flags &= ~(1 << 18)

            video_changed = True

        # Set refresh rate (0=50hz, 1=60hz)
        if current_refresh_rate != refresh_rate:
            refresh_rate_name = "60Hz" if refresh_rate else "50Hz"
            current_refresh_rate_name = "60Hz" if current_refresh_rate else "50Hz"
            logger.info(f"Changing refresh rate from {current_refresh_rate_name} to {refresh_rate_name}")

            if refresh_rate:
                video_flags |= (1 << 23)
            else:
                video_flags &= ~(1 << 23)

            video_changed = True

        if video_changed:
            eeprom.data.VideoFlags = video_flags

        # Set audio output mode (0=mono, 1=stereo, 2=surround)
        current_audio_output = 1  # Default stereo
        if current_mono:
            current_audio_output = 0
        elif current_surround:
            current_audio_output = 2

        if current_audio_output != audio_output:
            audio_output_names = ["MONO", "STEREO", "SURROUND"]
            logger.info(f"Changing audio output from {audio_output_names[current_audio_output]} to {audio_output_names[audio_output]}")

            # Clear both mono and surround flags
            audio_flags &= ~1  # Clear mono bit
            audio_flags &= ~(1 << 1)  # Clear surround bit

            # Set the appropriate flag based on audio_output
            if audio_output == 0:  # Mono
                audio_flags |= 1
            elif audio_output == 2:  # Surround
                audio_flags |= (1 << 1)
            # audio_output == 1 means stereo (both flags cleared)

            audio_changed = True

        # Set AC3/DTS flags
        if current_ac3 != ac3:
            logger.info(f"Changing AC3 from {current_ac3} to {ac3}")

            if ac3:
                audio_flags |= (1 << 16)
            else:
                audio_flags &= ~(1 << 16)

            audio_changed = True

        if current_dts != dts:
            logger.info(f"Changing DTS from {current_dts} to {dts}")

            if dts:
                audio_flags |= (1 << 17)
            else:
                audio_flags &= ~(1 << 17)

            audio_changed = True

        if audio_changed:
            eeprom.data.AudioFlags = audio_flags

        # Only save EEPROM if changes were made or if it's new
        needs_save = video_changed or audio_changed or not os.path.exists(self.eepromFile)
        needs_save = needs_save or (current_region != region) or (current_video_standard != video_standard) 
        needs_save = needs_save or (current_timezone != timezone) or (current_dvd_zone != dvd_zone)
        needs_save = needs_save or (current_language != language)

        if needs_save:
            logger.info("Saving EEPROM changes...")
            # Encrypt and save the modified eeprom
            encrypted = eeprom.encrypt()

            with open(self.eepromFile, 'wb') as outfile:
                outfile.write(encrypted)

            logger.info(f"EEPROM saved to {self.eepromFile}")
        else:
            logger.info("No EEPROM changes needed, skipping save")
