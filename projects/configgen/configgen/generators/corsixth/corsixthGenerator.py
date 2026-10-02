#!/usr/bin/env python
from configgen.Command import Command
import configgen.recalboxFiles as recalboxFiles
from configgen.Emulator import Emulator
from configgen.generators.Generator import Generator
from configgen.controllers.controller import ControllerPerPlayer
from configgen.settings.keyValueSettings import keyValueSettings


class CorsixTHGenerator(Generator):

    interpreterFile = recalboxFiles.ROMS + '/ports/Theme Hospital/CorsixTH.Lua'
    configFile = recalboxFiles.HOME + '/configs/corsixth/config.txt'

    LANGUAGES = \
    {
        "cs": "czech",
        "nl": "dutch",
        "en": "english",
        "fr": "french",
        "de": "german",
        "hu": "hungarian",
        "it": "italian",
        "ko": "korean",
        "nb": "norwegian",
        "pl": "polish",
        "pt": "portuguese",
        "ru": "russian",
        "es": "spanish",
        "sv": "swedish",
    }

    # Get keyboard layout
    @staticmethod
    def GetLanguage() -> str:
        conf = keyValueSettings(recalboxFiles.recalboxConf)
        conf.loadFile(True)
        # Try to obtain from keyboard layout, then from system language, then fallback to us
        kl = conf.getString("system.kblayout", conf.getString("system.language", "en")[0:2]).lower()
        return kl
    
    @staticmethod
    def GetFrontendLanguage() -> str:
        conf = keyValueSettings(recalboxFiles.recalboxConf)
        conf.loadFile(True)
        # Try to obtain from system language, then fallback to US or 
        frontendLanguage = conf.getString("system.language", "english")[0:2].upper()
        return frontendLanguage

    def generate(self, system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings, args) -> Command:

        """
        Load, override keys and save back emulator's configuration file
        This way, any modification is kept across emulator launches
        """
        
        import os
        romName = os.path.splitext(os.path.basename(args.rom))[0]
        homeDir = args.rom.replace("/" + romName + ".exe", "")
        
        language = self.GetLanguage()
        gameLanguage = self.LANGUAGES[language] if language in self.LANGUAGES else "english"

        # Config file
        conf = keyValueSettings(self.configFile, True)
        conf.loadFile(True).defineBool('true', 'false')

        conf.setString("adviser_disabled", "false")
        conf.setString("alien_dna_can_knock_on_doors", "false")
        conf.setString("alien_dna_must_stand", "true")
        conf.setString("alien_dna_only_by_emergency", "true")
        conf.setString("allow_blocking_off_areas", "false")
        conf.setString("allow_user_actions_while_paused", "false")
        conf.setString("announcement_volume", "0.5")
        conf.setString("audio", "true")
        conf.setString("audio_buffer_size", "2048")
        conf.setString("audio_channels", "2")
        conf.setString("audio_frequency", "22050")
        conf.setString("audio_music", "nil")
        conf.setString("campaigns", "nil")
        conf.setString("capture_mouse", "true")
        conf.setString("check_for_updates", "false")  # If enabled, can't download updates because of trunk version
        conf.setString("debug", "false")
        conf.setString("debug_falling", "false")
        conf.setString("direct_zoom", "nil")
        conf.setString("disable_fractured_bones_females", "true")
        conf.setString("enable_avg_contents", "false")
        conf.setString("free_build_mode", "false")
        conf.setString("fullscreen", "true")
        conf.setString("grant_wage_increase", "false")
        conf.setInt("height", 600)
        conf.setString("language", "[[" + gameLanguage + "]]")
        conf.setString("levels", "nil")
        conf.setString("movies", "true")
        conf.setString("music_volume", "0.5")
        conf.setString("new_graphics_folder", "nil")
        conf.setString("new_machine_extra_info", "true")
        conf.setString("play_announcements", "true")

        # If the file DATAM/DEMO.DAT exists, we are playing the demo
        if os.path.exists(homeDir + "/HOSP/DATAM/DEMO.DAT"):
            conf.setString("play_demo", "true")
        else:
            conf.setString("play_demo", "false")

        conf.setString("play_intro", "true")
        conf.setString("play_music", "true")
        conf.setString("play_sounds", "true")
        conf.setString("player_name", "[[PLAYER]]")
        conf.setString("prevent_edge_scrolling", "false")
        conf.setString("remove_destroyed_rooms", "false")
        conf.setString("right_mouse_scrolling", "false")
        conf.setString("room_information_dialogs", "true")
        conf.setString("savegames", "[=[" + recalboxFiles.SAVES + "/themehospital]=]")
        conf.setString("screenshots", "[=[" + recalboxFiles.SCREENSHOTS + "]=]")
        conf.setInt("scroll_speed", 2)
        conf.setString("scrolling_momentum", "0.8")
        conf.setInt("shift_scroll_speed", 4)
        conf.setString("sound_volume", "0.5")

        if os.path.exists(homeDir + "/HOSP/FluidR3.sf3"):
            conf.setString("soundfont", "[=[" + homeDir + "/HOSP/FluidR3.sf3]=]")
        else:
            conf.setString("soundfont", "nil")

        conf.setString("theme_hospital_install", "[=[" + homeDir + "/HOSP]=]")

        if system.ShowFPS:
            conf.setString("track_fps", "true")
        else:
            conf.setString("track_fps", "false")

        conf.setString("twentyfour_hour_clock", "true")
        conf.setString("unicode_font", "nil")
        conf.setString("use_new_graphics", "false")
        conf.setString("volume_opens_casebook", "false")
        conf.setInt("warmth_colors_display_default", 1)
        conf.setInt("width", 800)
        conf.setInt("zoom_speed", 80)
        
        conf.saveFile()

        # Command line arguments
        commandArray = [recalboxFiles.recalboxBins[system.Emulator], args.rom,
                        "--interpreter=" + self.interpreterFile,
                        "--config-file=" + self.configFile]

        # Add extra arguments
        if system.HasArgs: commandArray.extend(system.Args)

        return Command(videomode=system.VideoMode, array=commandArray)
