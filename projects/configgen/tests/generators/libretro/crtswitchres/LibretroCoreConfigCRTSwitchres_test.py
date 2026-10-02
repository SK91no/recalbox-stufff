from unittest.mock import patch, PropertyMock

import pytest
from configgen.Emulator import Emulator, ExtraArguments
from configgen.crt.RGBDual2 import RGBDual2
from configgen.crtswitchres.CRTTypes import RetroarchScreenTypeIndex
from configgen.generators.libretro.crtswitchres.LibretroConfigCRTSwitchres import LibretroConfigCRTSwitchres
from configgen.generators.libretro.crtswitchres.LibretroCoreConfigCRTSwitchres import LibretroCoreConfigCRTSwitchres
from configgen.settings.keyValueSettings import keyValueSettings


@pytest.fixture(autouse=True)
def before(mocker):
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=False, new_callable=mocker.PropertyMock)

def configureForCrt(emulator: Emulator, crtvideostandard="auto", crtresolutiontype="auto", crtscreentype="15kHz",
                    crtadaptor="recalboxrgbdual", crtregion="auto", crtscanlines="", rotation=0, verticalgame=False,
                    crtsuperrez = "1", crtsignaltype = "RGB", widescreen=False, avoid_interlaced = False, avoid_interlaced_on_tate_and_handhelds=False,
                    hdgame = False, crthandheldformat = "fullscreen", avoid_low_freq_modes = False):
    recalCrtConf = keyValueSettings("")
    recalCrtConf.setBool("options.video.avoidinterlaced", avoid_interlaced)
    recalCrtConf.setBool("options.video.avoidinterlacedontatehandhelds", avoid_interlaced_on_tate_and_handhelds)
    recalCrtConf.setBool("options.video.avoidlowfreqmodes", avoid_low_freq_modes)
    recalboxConf = keyValueSettings("")
    recalboxConf.setString("global.crthandheldformat", crthandheldformat)
    emulator.configure(recalboxConf,
                       ExtraArguments("", "", "", "", "", "", "", "", "", crtvideostandard, crtresolutiontype,
                                      crtscreentype,
                                      crtadaptor, crtregion,
                                      crtscanlines, rotation=rotation, verticalgame=verticalgame,
                                      crtsuperrez=crtsuperrez, crtsignaltype=crtsignaltype, hdgame=hdgame),
                       recalCrtConf)
    emulator.WideScreenMode = widescreen
    return emulator


def test_given_genesis_core_then_disable_overscan_crop():
    emulator = configureForCrt(
        Emulator(name='mastersystem', videoMode='720x240', ratio='auto', emulator='libretro', core='genesisplusgx'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtvideostandard="auto",
        crtadaptor="recalboxrgbdual2")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)

    assert config["genesis_plus_gx_overscan"] == '"full"'


def test_given_gambatte_core_then_set_colorization():
    emulator = configureForCrt(
        Emulator(name='gb', videoMode='720x240', ratio='auto', emulator='libretro', core='gambatte'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtvideostandard="auto",
        crtadaptor="recalboxrgbdual2")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)

    assert config["gambatte_gb_colorization"] == '"internal"'


def test_given_gamecube_then_force_480px_fb():
    emulator = configureForCrt(
        Emulator(name='gamecube', videoMode='1920x1080', ratio='auto', emulator='libretro', core='dolphin'),
        crtresolutiontype="480i", crtscreentype="15kHz", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)

    assert config["dolphin_efb_height"] == '480'

def test_given_cpc_game_on_31kHz_remove_overscan():
    emulator = configureForCrt(
        Emulator(name='amstradcpc', videoMode='1920x1080', ratio='auto', emulator='libretro', core='cap32'),
        crtresolutiontype="480", crtscreentype="kHz31", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert config["cap32_scr_crop"] == '"enabled"'

    emulator = configureForCrt(
        Emulator(name='amstradcpc', videoMode='1920x1080', ratio='auto', emulator='libretro', core='cap32'),
        crtresolutiontype="auto", crtscreentype="kHz31", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert config["cap32_scr_crop"] == '"enabled"'

    emulator = configureForCrt(
        Emulator(name='amstradcpc', videoMode='1920x1080', ratio='auto', emulator='libretro', core='cap32'),
        crtresolutiontype="auto", crtscreentype="kHzTriFreq", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert config["cap32_scr_crop"] == '"enabled"'

    emulator = configureForCrt(
        Emulator(name='amstradcpc', videoMode='1920x1080', ratio='auto', emulator='libretro', core='cap32'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert config["cap32_scr_crop"] == '"disabled"'

def test_given_dosbox_pure_then_set_refresh_rate():
    emulator = configureForCrt(
        Emulator(name='dos', videoMode='1920x1080', ratio='auto', emulator='libretro', core='dosbox_pure'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtvideostandard="ntsc", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert config["dosbox_pure_force60fps"] == '"true"'

    emulator = configureForCrt(
        Emulator(name='dos', videoMode='1920x1080', ratio='auto', emulator='libretro', core='dosbox_pure'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtvideostandard="pal", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert config["dosbox_pure_force60fps"] == '"50"'

def test_given_scummvm_then_set_refresh_rate():
    emulator = configureForCrt(
        Emulator(name='scummvm', videoMode='1920x1080', ratio='auto', emulator='libretro', core='scummvm'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtvideostandard="auto", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert config["scummvm_framerate"] == '"60 Hz"'

    emulator = configureForCrt(
        Emulator(name='scummvm', videoMode='1920x1080', ratio='auto', emulator='libretro', core='scummvm'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtvideostandard="ntsc", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert config["scummvm_framerate"] == '"60 Hz"'

    emulator = configureForCrt(
        Emulator(name='scummvm', videoMode='1920x1080', ratio='auto', emulator='libretro', core='scummvm'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtvideostandard="pal", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert config["scummvm_framerate"] == '"50 Hz"'

def test_given_x68000_on_rgb_dual2_then_balance_adpcm_and_opm_volumes():
    emulator = configureForCrt(
        Emulator(name='x68000', videoMode='1920x1080', ratio='auto', emulator='libretro', core='px68k'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert config["px68k_adpcm_vol"] == '"11"'
    assert config["px68k_opm_vol"] == '"11"'


def test_given_x68000_on_rgb_jamma_then_balance_adpcm_and_opm_volumes():
    emulator = configureForCrt(
        Emulator(name='x68000', videoMode='1920x1080', ratio='auto', emulator='libretro', core='px68k'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbjamma", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert config["px68k_adpcm_vol"] == '"11"'
    assert config["px68k_opm_vol"] == '"11"'


def test_given_x68000_on_jvs_then_balance_adpcm_and_opm_volumes():
    emulator = configureForCrt(
        Emulator(name='x68000', videoMode='1920x1080', ratio='auto', emulator='libretro', core='px68k'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxjvs", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert config["px68k_adpcm_vol"] == '"11"'
    assert config["px68k_opm_vol"] == '"11"'


def test_given_x68000_on_another_adapter_then_leave_the_core_volumes_alone():
    emulator = configureForCrt(
        Emulator(name='x68000', videoMode='1920x1080', ratio='auto', emulator='libretro', core='px68k'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert "px68k_adpcm_vol" not in config
    assert "px68k_opm_vol" not in config


def test_given_another_core_on_rgb_dual2_then_no_px68k_volume_is_written():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert "px68k_adpcm_vol" not in config
    assert "px68k_opm_vol" not in config


def test_given_gearsystem_then_set_overscan():
    emulator = configureForCrt(
        Emulator(name='mastersystem', videoMode='1920x1080', ratio='auto', emulator='libretro', core='gearsystem'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtvideostandard="auto", crtsuperrez="1")
    config = LibretroCoreConfigCRTSwitchres().createConfigFor(emulator)
    assert config["gearsystem_overscan"] == '"Full (284 width)"'
