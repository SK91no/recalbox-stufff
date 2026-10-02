from configgen.Emulator import Emulator, ExtraArguments
from configgen.generators.libretro.libretroGenerator import LibretroGenerator
from configgen.settings.keyValueSettings import keyValueSettings
from configgen.utils.AutoBlitter import AutoBlitter


def configure(system: Emulator, autoBlitter: str = "none"):
    opts = keyValueSettings("")
    opts.setString("global.autoblitter", autoBlitter)
    system.configure(opts, ExtraArguments('1920x1080', "", "", "", "", "", "", "", "", "", "", "", ""))
    return system

def test_given_mushisam_and_fbneo_and_recalbox_autoblitter_then_create_core_config(mocker):
    coreConfig = LibretroGenerator().createFBNeoBlitterConfig(configure(Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'), AutoBlitter.Recalbox), "mushisam.zip")

    assert coreConfig["fbneo-cpu-speed-adjust"] == '"100%"'
    assert coreConfig["fbneo-dipswitch-mushisam-Blitter_Delay"] == '"60"'


def test_given_mushisam_and_fbneo_and_viku_autoblitter_then_create_core_config(mocker):
    coreConfig = LibretroGenerator().createFBNeoBlitterConfig(configure(Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'), AutoBlitter.Viku), "mushisam.zip")

    assert coreConfig["fbneo-cpu-speed-adjust"] == '"40%"'
    assert coreConfig["fbneo-dipswitch-mushisam-Blitter_Delay"] == '"57"'


def test_given_any_fbneo_game_and_gbneo_then_restore_cpu_speed(mocker):
    coreConfig = LibretroGenerator().createFBNeoBlitterConfig(
        configure(Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'), AutoBlitter.Noone),"mushisam.zip")
    assert coreConfig["fbneo-cpu-speed-adjust"] == '"100%"'

def test_given_any_fbneo_game_and_gbneo_then_restore_cpu_speed(mocker):
    coreConfig = LibretroGenerator().createFBNeoBlitterConfig(configure(Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo')), "any.zip")
    assert coreConfig["fbneo-cpu-speed-adjust"] == '"100%"'
