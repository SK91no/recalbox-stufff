#!/usr/bin/env python
import configgen.recalboxFiles as recalboxFiles
from configgen.Emulator import Emulator
from configgen.settings.keyValueSettings import keyValueSettings
from configgen.utils.architecture import Architecture

# Languages
GAME_LANGUAGES = {
    "jp": 0,
    "en": 1,
    "fr": 2,
    "es": 3,
    "de": 4,
    "it": 5,
    "nl": 6,
    "pt": 7,
    "ru": 8,
    "kr": 9,
    "zh": 10,
    "zh_tw": 11,
}

SYSTEM_LANGUAGES = {
    "jp": "ja_JP",
    "en": "en_US",
    "fr": "fr_FR",
    "es": "es_ES",
    "de": "de_DE",
    "it": "it_IT",
    "nl": "nl_NL",
    "pt": "pt_PT",
    "ru": "ru_RU",
    "kr": "ko_KR",
    "zh": "zh_CN",
    "zh_tw": "zh_TW",
}

def GetLanguage() -> str:
    conf = keyValueSettings(recalboxFiles.recalboxConf)
    conf.loadFile(True)
    kl = conf.getString("system.kblayout", conf.getString("system.language", "en")[0:2]).lower()
    print("Keyboard: {}".format(kl))
    return kl

def GetFrontendLanguage() -> str:
    conf = keyValueSettings(recalboxFiles.recalboxConf)
    conf.loadFile(True)
    # Try to obtain from system language, then fallback to US
    frontendLanguage = conf.getString("system.language", "$")[0:2].lower()
    print("Frontend language: {}".format(frontendLanguage))
    return frontendLanguage

def writePPSSPPConfig(system: Emulator):

    configFile = recalboxFiles.CONF + '/ppsspp/PSP/SYSTEM/ppsspp.ini'

    gameLanguage = GetLanguage()
    systemLanguage = GetFrontendLanguage()
    from configgen.settings.iniSettings import IniSettings
    settings = IniSettings(configFile, True)
    settings.loadFile(True)
    # Display FPS
    settings.setInt("Graphics", 'iShowStatusFlags', 6 if system.ShowFPS else 0)  # bit 1: FPS counter, bit 2: speed
    settings.setInt("SystemParam", 'GameLanguage', GAME_LANGUAGES[gameLanguage] if gameLanguage in GAME_LANGUAGES else 1)
    settings.setString("General", 'Language', SYSTEM_LANGUAGES[systemLanguage] if systemLanguage in SYSTEM_LANGUAGES else "en_US")
    # PPSSPP reads its GPU backend from here, not from the SDL video driver.
    from configgen.utils.Vulkan import Vulkan
    settings.setString("Graphics", 'GraphicsBackend', "VULKAN" if Vulkan.vulkanAvailable() and system.VulkanDriver else "OPENGL")
    # Fix cracking sound
    if Architecture().isX64: 
        settings.setString("Sound", 'GameVolume', "99")
        settings.setString("Sound", 'AudioBufferSize', "512")
        settings.setString("Sound", 'ExtraAudioBuffering', "True")
    settings.saveFile()
