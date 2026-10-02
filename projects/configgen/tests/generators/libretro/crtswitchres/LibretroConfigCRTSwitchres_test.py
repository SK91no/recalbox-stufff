from unittest.mock import patch, PropertyMock

import pytest
from configgen.Emulator import Emulator, ExtraArguments
from configgen.crt.RGBDual2 import RGBDual2
from configgen.crtswitchres.CRTTypes import RetroarchScreenTypeIndex
from configgen.generators.libretro.crtswitchres.LibretroConfigCRTSwitchres import LibretroConfigCRTSwitchres
from configgen.settings.keyValueSettings import keyValueSettings
from configgen.utils.videoMode import deleteCrtDesktopRes
from tests.Givens import givenThoseFiles


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



def test_given_15khz_on_dual_should_enable_retroarch_switchres():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value

def test_given_15khz_on_jamma_should_enable_retroarch_switchres():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbjamma")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15Jamma.value

def test_given_dynamic_superrez_should_enable_superrez():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual", crtsuperrez="1")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution_super"] == "1"

def test_given_native_should_enable_native():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution_super"] == "0"

def test_naomi2_pi5_31khz_forces_vulkan_on_the_480p_desktop_mode(mocker):
    # The compatible-screen rule alone forces vulkan (global.vulkandriver, off by default here, is
    # irrelevant). vulkan can't CRT-modeswitch/superrez: switchres is disabled and the mode is imposed
    # to the driver as its desktop mode, then removed once the game is over.
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    desktopRes = mocker.patch('configgen.generators.libretro.crtswitchres.LibretroConfigCRTSwitchres.createCrtDesktopRes')
    emulator = configureForCrt(
        Emulator(name='naomi2', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next'),
        crtresolutiontype="auto", crtscreentype="kHz31", crtadaptor="recalboxrgbdual")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "clubkrt.zip")

    assert config_lines["video_driver"] == '"vulkan"'
    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.disabled.value
    assert desktopRes.call_args[0][0] == "640x480"
    assert deleteCrtDesktopRes in post


def test_naomi2_pi5_31khz_under_vulkan_never_pins_the_mode_by_size(mocker):
    # A pinned video_fullscreen_x/y makes KHR_display look for a mode of that size and abort on an
    # interlaced one ("Failed to create KHR_display surface"): the desktop mode is the only pin.
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    mocker.patch('configgen.generators.libretro.crtswitchres.LibretroConfigCRTSwitchres.createCrtDesktopRes')
    emulator = configureForCrt(
        Emulator(name='naomi2', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next'),
        crtresolutiontype="auto", crtscreentype="kHz31", crtadaptor="recalboxrgbdual")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "clubkrt.zip")

    assert config_lines["video_fullscreen_x"] == ''
    assert config_lines["video_fullscreen_y"] == ''


def test_naomi2_pi5_15khz_in_480_forces_vulkan_on_the_interlaced_desktop_mode(mocker):
    # A 15kHz screen asked for 480 lines gets 640x480i, a mode vulkan drives as well as the 31kHz 480p
    # one: naomi2 gets the smoother pacing there too instead of falling back to glcore.
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    desktopRes = mocker.patch('configgen.generators.libretro.crtswitchres.LibretroConfigCRTSwitchres.createCrtDesktopRes')
    emulator = configureForCrt(
        Emulator(name='naomi2', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next'),
        crtresolutiontype="480", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "initd.zip")

    assert config_lines["video_driver"] == '"vulkan"'
    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.disabled.value
    assert desktopRes.call_args[0][0] == "640x480i"
    assert deleteCrtDesktopRes in post
    # Filling the surface is right for an imposed mode too: 640x480i is already 4:3.
    assert config_lines["aspect_ratio_index"] == '"24"'


def test_naomi2_pi5_15khz_in_auto_takes_the_480i_mode_when_the_adapter_interlaces(mocker):
    # naomi2 renders 480 lines: left to decide, it goes to 480i rather than losing half of them in 240p.
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    desktopRes = mocker.patch('configgen.generators.libretro.crtswitchres.LibretroConfigCRTSwitchres.createCrtDesktopRes')
    emulator = configureForCrt(
        Emulator(name='naomi2', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "initd.zip")

    assert config_lines["video_driver"] == '"vulkan"'
    assert desktopRes.call_args[0][0] == "640x480i"


def test_naomi2_pi5_multisync_asked_for_480i_takes_the_interlaced_mode(mocker):
    # A screen that does both frequencies follows the user: 480i asked, 480i imposed, not 480p.
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    desktopRes = mocker.patch('configgen.generators.libretro.crtswitchres.LibretroConfigCRTSwitchres.createCrtDesktopRes')
    emulator = configureForCrt(
        Emulator(name='naomi2', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next'),
        crtresolutiontype="480i", crtscreentype="kHzMulti1531", crtadaptor="recalboxrgbdual2")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "initd.zip")

    assert desktopRes.call_args[0][0] == "640x480i"


def test_naomi2_pi5_multisync_in_auto_takes_the_progressive_mode(mocker):
    # Left to decide on a screen that reaches 31kHz, 480p beats 480i: same lines, no flicker.
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    desktopRes = mocker.patch('configgen.generators.libretro.crtswitchres.LibretroConfigCRTSwitchres.createCrtDesktopRes')
    emulator = configureForCrt(
        Emulator(name='naomi2', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next'),
        crtresolutiontype="auto", crtscreentype="kHzMulti1531", crtadaptor="recalboxrgbdual2")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "initd.zip")

    assert desktopRes.call_args[0][0] == "640x480"


def test_naomi2_pi5_forced_240p_imposes_the_60hz_240p_mode(mocker):
    # A user who asked for 240p keeps it, on the mode of his standard: left to choose, retroarch
    # takes the largest mode published, the 1920x288 at 50Hz that paces a 60Hz game at 50.
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    desktopRes = mocker.patch('configgen.generators.libretro.crtswitchres.LibretroConfigCRTSwitchres.createCrtDesktopRes')
    emulator = configureForCrt(
        Emulator(name='naomi2', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next'),
        crtresolutiontype="240", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "initd.zip")

    assert config_lines["video_driver"] == '"vulkan"'
    assert desktopRes.call_args[0][0] == "320x240"
    assert deleteCrtDesktopRes in post
    assert config_lines["aspect_ratio_index"] == '"24"'


def test_naomi2_pi5_forced_240p_in_pal_imposes_the_50hz_mode(mocker):
    # A 320x240 is a 60Hz mode: a PAL game gets the 288-line one instead of running 17% too fast.
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    desktopRes = mocker.patch('configgen.generators.libretro.crtswitchres.LibretroConfigCRTSwitchres.createCrtDesktopRes')
    emulator = configureForCrt(
        Emulator(name='naomi2', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next'),
        crtresolutiontype="240", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtvideostandard="pal")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "initd.zip")

    assert desktopRes.call_args[0][0] == "384x288"


def test_naomi2_pi5_15khz_imposes_the_240p_mode_when_interlacing_is_refused(mocker):
    # "avoid interlaced" removes the only 480 mode a 15kHz screen has: 240p is what is left to impose.
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    desktopRes = mocker.patch('configgen.generators.libretro.crtswitchres.LibretroConfigCRTSwitchres.createCrtDesktopRes')
    emulator = configureForCrt(
        Emulator(name='naomi2', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next'),
        crtresolutiontype="480", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", avoid_interlaced=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "initd.zip")

    assert config_lines["video_driver"] == '"vulkan"'
    assert desktopRes.call_args[0][0] == "320x240"


def test_flycast_hd_game_in_240p_keeps_the_pixel_perfect_gl_driver(mocker):
    # An HD game downscaled to 240p is switched to gl upstream for a pixel-perfect result, and
    # manage_scanlines picks its shader extension from that driver: flycast must not take it back.
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    emulator = configureForCrt(
        Emulator(name='naomigd', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next'),
        crtresolutiontype="240", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", hdgame=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "cvs2.zip")

    assert config_lines["video_driver"] == '"gl"'


def test_flycast_standard_game_stays_on_glcore(mocker):
    # Nothing asked for gl: flycast-next keeps the glcore it needs for switchres/superrez.
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    emulator = configureForCrt(
        Emulator(name='naomigd', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next'),
        crtresolutiontype="240", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", hdgame=False)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "cvs2.zip")

    assert config_lines["video_driver"] == '"glcore"'


def test_flycast_next_on_crt_is_glcore(mocker):
    # crtswitchres owns the engine: flycast-next (dreamcast, naomi…) on CRT → glcore for switchres.
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next'),
        crtresolutiontype="auto", crtscreentype="kHz31", crtadaptor="recalboxrgbdual")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "game.chd")

    assert config_lines["video_driver"] == '"glcore"'


def test_naomi2_pi5_31khz_forced_240p_still_asks_for_the_240p_mode(mocker):
    # Even on a 31kHz screen, the 240p asked for is what is requested; the driver refuses the mode
    # if the screen cannot take it, which is its call to make and not configgen's.
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    desktopRes = mocker.patch('configgen.generators.libretro.crtswitchres.LibretroConfigCRTSwitchres.createCrtDesktopRes')
    emulator = configureForCrt(
        Emulator(name='naomi2', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next'),
        crtresolutiontype="240", crtscreentype="kHz31", crtadaptor="recalboxrgbdual")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "clubkrt.zip")

    assert config_lines["video_driver"] == '"vulkan"'
    assert config_lines["video_fullscreen_x"] == ''
    assert desktopRes.call_args[0][0] == "320x240"


def test_given_rpi4_should_not_disable_interlaced(mocker):
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=False, new_callable=mocker.PropertyMock)

    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value
    assert config_lines["crt_switch_resolution_super"] == "0"
    assert config_lines["crt_switch_resolution_no_interlaced"] == 0

def test_given_tate_game_on_rotated_screen_should_rotate(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        rotation= 3, verticalgame=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["video_rotation"] == 3

def test_given_tate_game_on_yoko_screen_should_not_rotate(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        rotation=0, verticalgame=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["video_rotation"] == 0

def test_given_yoko_game_on_tate_screen_should_rotate(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        rotation=3, verticalgame=False)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["video_rotation"] == 3


def test_given_kHz15_screen_and_auto_resolution_then_configure_switchres(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHz15", crtresolutiontype="auto", crtsuperrez="1920")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value
    assert config_lines["crt_switch_resolution_super"] == "1920"
    assert config_lines["crt_switch_resolution_no_interlaced"] == 0

def test_given_kHz31_screen_and_auto_resolution_then_configure_switchres(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHz31", crtresolutiontype="auto", crtsuperrez="1920")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz31.value
    assert config_lines["crt_switch_resolution_super"] == "1920"
    assert config_lines["crt_switch_resolution_no_interlaced"] == 0

def test_given_kHz31_screen_and_doublefreq_resolution_then_configure_switchres(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHz31", crtresolutiontype="doublefreq")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz31at120.value
    assert config_lines["video_black_frame_insertion"] == '"1"'


def test_given_kHz31_yoko_screen_and_yoko_game_480p_and_scanlines_then_configure_scanlines(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHz31", crtresolutiontype="480", crtscanlines="heavy")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz31.value
    assert config_lines["video_shader_enable"] == '"true"'
    assert config_lines["video_shader_dir"] == '"/recalbox/share/shaders/"'
    assert config_lines["video_shader"] == 'rrgbd-scanlines-heavy.slangp'
    assert config_lines["video_driver"] == '"glcore"'

def test_given_kHz31_tate_screen_and_yoko_game_480p_and_scanlines_then_skip_scanlines(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHz31", crtresolutiontype="480", crtscanlines="heavy", verticalgame=False, rotation=3)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz31Forced480p.value
    assert config_lines["video_shader_enable"] == '"false"'

def test_given_kHz31_yoko_screen_and_tate_game_480p_and_scanlines_then_skip_scanlines(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHz31", crtresolutiontype="480", crtscanlines="heavy", verticalgame=True, rotation=0)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz31Forced480p.value
    assert config_lines["video_shader_enable"] == '"false"'

def test_given_kHz15_and_scanlines_then_skip_scanlines(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHz15", crtresolutiontype="auto", crtscanlines="heavy")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value
    assert config_lines["video_shader_enable"] == '"false"'

def test_given_multi1531_and_auto_res_then_use_auto(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHzMulti1531", crtresolutiontype="auto", crtscanlines="heavy")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value

def test_given_multi1531_and_480p_res_then_use_480p(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHzMulti1531", crtresolutiontype="480", crtscanlines="heavy")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz31.value

def test_given_multi1525_and_auto_res_then_use_auto(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHzMulti1525", crtresolutiontype="auto", crtscanlines="heavy")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value

def test_given_multi1525_and_240p_res_then_use_240p(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHzMulti1525", crtresolutiontype="240", crtscanlines="heavy")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15ForcedProgressive.value

def test_given_trisync_and_auto_res_then_use_auto(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHzTriFreq", crtresolutiontype="auto", crtscanlines="heavy")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value

def test_given_trisync_and_240p_res_then_use_240p(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHzTriFreq", crtresolutiontype="240", crtscanlines="heavy")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15ForcedProgressive.value

def test_given_trisync_and_480p_res_then_use_480p(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHzTriFreq", crtresolutiontype="480", crtscanlines="heavy")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz31.value

def test_given_trisync_and_force480p_res_then_use_scanlines(mocker):
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHzTriFreq", crtresolutiontype="480", crtscanlines="heavy")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz31.value
    assert config_lines["video_shader_enable"] == '"true"'
    assert config_lines["video_shader_dir"] == '"/recalbox/share/shaders/"'
    assert config_lines["video_shader"] == 'rrgbd-scanlines-heavy.slangp'


# Force 50Hz
def test_given_rrgb2_forced_50hz_then_should_choose_pal_mode_on_rgb():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='720x240', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15",  crtvideostandard="pal",
        crtadaptor="recalboxrgbdual2", crtsignaltype="auto")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.PAL.value

def test_given_forced_video_standard_should_force_snes9x_core_region():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15",  crtvideostandard="ntsc",
        crtadaptor="recalboxrgbdual")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert core_config["snes9x_region"] == '"ntsc"'

    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15",  crtvideostandard="pal",
        crtadaptor="recalboxrgbdual")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert core_config["snes9x_region"] == '"pal"'

# Missing tests about each video standard, the code has been copied from old crt system, so it was tested

def test_given_jap_region_should_force_genesisplusgx_core_region():
    emulator = configureForCrt(
        Emulator(name='megadrive', videoMode='1920x1080', ratio='auto', emulator='libretro', core='genesisplusgx'),
        crtresolutiontype="auto", crtscreentype="kHz15",  crtvideostandard="ntsc", crtregion="jp",
        crtadaptor="recalboxrgbdual")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "BareKnuckles.zip")

    assert core_config["genesis_plus_gx_region_detect"] == '"ntsc-j"'

def test_given_jap_region_should_force_picodrive_core_region():
    emulator = configureForCrt(
        Emulator(name='megadrive', videoMode='1920x1080', ratio='auto', emulator='libretro', core='picodrive'),
        crtresolutiontype="auto", crtscreentype="kHz15",  crtvideostandard="ntsc", crtregion="jp",
        crtadaptor="recalboxrgbdual")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "BareKnuckles.zip")

    assert core_config["picodrive_region"] == '"Japan NTSC"'

    emulator = configureForCrt(
        Emulator(name='megadrive', videoMode='1920x1080', ratio='auto', emulator='libretro', core='picodrive'),
        crtresolutiontype="auto", crtscreentype="kHz15",  crtvideostandard="ntsc", crtregion="us",
        crtadaptor="recalboxrgbdual")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "BareKnuckles.zip")

    assert core_config["picodrive_region"] == '"US"'

    emulator = configureForCrt(
        Emulator(name='megadrive', videoMode='1920x1080', ratio='auto', emulator='libretro', core='picodrive'),
        crtresolutiontype="auto", crtscreentype="kHz15",  crtvideostandard="pal", crtregion="eu",
        crtadaptor="recalboxrgbdual")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "BareKnuckles.zip")

    assert core_config["picodrive_region"] == '"Europe"'

def test_given_rgbjamma_then_should_force_ntsc_core_region():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15",  crtvideostandard="auto",
        crtadaptor="recalboxrgbjamma")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert core_config["snes9x_region"] == '"ntsc"'

# RGB DUAL 2 - Composite
def test_given_rrgb2_composite_progressive_then_should_choose_720x240p():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='720x240', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="240", crtscreentype="kHz15",  crtvideostandard="auto",
        crtadaptor="recalboxrgbdual2", crtsignaltype="Composite")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.disabled.value
    assert config_lines["video_fullscreen_x"] == 720
    assert config_lines["video_fullscreen_y"] == 240
    assert config_lines["video_scale_integer"] == '"true"'

def test_given_rrgb2_composite_auto_then_should_choose_720x240p():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='720x240', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15",  crtvideostandard="auto",
        crtadaptor="recalboxrgbdual2", crtsignaltype="Composite")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.disabled.value
    assert config_lines["video_fullscreen_x"] == 720
    assert config_lines["video_fullscreen_y"] == 240
    assert config_lines["video_scale_integer"] == '"true"'
    assert config_lines["aspect_ratio_index"] == '"20"'
    assert config_lines["video_aspect_ratio"] == '"2.83"'
    assert pre == [RGBDual2.setRatioStandard, RGBDual2.setVideoComposite]
    assert post == [RGBDual2.setRatioDefault, RGBDual2.setVideoDefault]

# RRGB2 Widescreen
def test_given_rrgb2_then_set_ratio_to_43_and_back_to_default():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='320x240', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15",  crtvideostandard="auto",
        crtadaptor="recalboxrgbdual2", crtsignaltype="RGB")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert pre == [RGBDual2.setRatioStandard]
    assert post == [RGBDual2.setRatioDefault]

def test_given_rrgb2_and_widescreen_core_then_set_ratio_to_43_and_back_to_default():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='720x240', ratio='auto', emulator='libretro', core='snes9x', ),
        crtresolutiontype="auto", crtscreentype="kHz15",  crtvideostandard="auto",
        crtadaptor="recalboxrgbdual2", crtsignaltype="RGB", widescreen=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert pre == [RGBDual2.setRatioWide]
    assert post == [RGBDual2.setRatioDefault]


def test_given_rrgb2_and_widescreen_and_composite_then_set_ratio_to_43_and_video_to_composite():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='720x240', ratio='auto', emulator='libretro', core='snes9x', ),
        crtresolutiontype="auto", crtscreentype="kHz15",  crtvideostandard="auto",
        crtadaptor="recalboxrgbdual2", crtsignaltype="Composite", widescreen=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert pre == [RGBDual2.setRatioWide, RGBDual2.setVideoComposite]
    assert post == [RGBDual2.setRatioDefault, RGBDual2.setVideoDefault]


# Force 50Hz
def test_given_rrgb2_forced_50hz_then_should_choose_pal_mode_on_composite():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='720x240', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15",  crtvideostandard="pal",
        crtadaptor="recalboxrgbdual2", crtsignaltype="Composite")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.disabled.value
    assert config_lines["video_fullscreen_x"] == 720
    assert config_lines["video_fullscreen_y"] == 288
    assert config_lines["video_scale_integer"] == '"true"'
    assert config_lines["aspect_ratio_index"] == '"20"'
    assert config_lines["video_aspect_ratio"] == '"2.83"'


# force 240p mode with 240 lines (and not 256 lines)
def test_given_480p_game_then_should_choose_240p_mode():
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='720x240', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="240", crtscreentype="kHz15",  crtvideostandard="auto", hdgame=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15Forced240p.value

def test_given_rpi5_and_rgb_dual_2_proto1_should_disable_interlaced_on_rgb(mocker):
    mocker.patch('configgen.crtswitchres.CRTTypes.CRTAdapter.supportsInterlaced', return_value=False)

    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="480", crtscreentype="kHz15", crtadaptor="recalboxrgbdual", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value
    assert config_lines["crt_switch_resolution_super"] == "0"
    assert config_lines["crt_switch_resolution_no_interlaced"] == 1

def test_given_composite_and_hd_selected_should_use_composite_interlaced(mocker):
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="480", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtsuperrez="0",
        crtsignaltype="Composite")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.disabled.value
    assert config_lines["video_fullscreen_x"] == 720
    assert config_lines["video_fullscreen_y"] == 480
    assert config_lines["video_scale_integer"] == '"true"'
    assert config_lines["video_vsync"] == '"false"'


def test_given_rpi5_and_rgbjamma2_should_not_disable_interlaced_on_rgb(mocker):
    givenThoseFiles(mocker, {"/sys/firmware/devicetree/base/recalboxrgbjamma/recalbox-rgb-jamma-2": ""})
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbjamma", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15Jamma.value
    assert config_lines["crt_switch_resolution_no_interlaced"] == 0


def test_given_rpi5_and_rrgbd2_and_disable_interlace_should_disable_interlaced_on_rgb(mocker):
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtsuperrez="0", avoid_interlaced=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Sonic.chd")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value
    assert config_lines["crt_switch_resolution_no_interlaced"] == 1


def test_given_rpi5_and_rrgbd2_and_disable_interlace_on_tate_should_not_disable_interlaced_on_yoko_game(mocker):
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtsuperrez="0", avoid_interlaced_on_tate_and_handhelds=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Sonic.chd")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value
    assert config_lines["crt_switch_resolution_no_interlaced"] == 0

def test_given_rrgbj2_and_forcep_should_force240p_modes():
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="240", crtscreentype="kHz15", crtadaptor="recalboxrgbjamma", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Sonic.chd")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15Forced240p.value

def test_given_crt_should_disable_threaded_video():
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="240", crtscreentype="kHz15", crtadaptor="recalboxrgbjamma", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Sonic.chd")

    assert config_lines["video_threaded"] == '"false"'


def test_given_240_on_kHzMulti1531_should_force15khz_with_no_interlaced():
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="240", crtscreentype="kHzMulti1531", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Sonic.chd")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15ForcedProgressive.value


def test_given_480i_on_kHzMulti1531_should_force15khz_with_interlaced():
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="480i", crtscreentype="kHzMulti1531", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Sonic.chd")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value

def test_given_480i_on_kHzTriFreq_should_force15khz_with_interlaced():
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="480i", crtscreentype="kHzTriFreq", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Sonic.chd")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value

def test_given_dreamcast_game_on_31kHz_240pat120hz_force240pat120():
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="doublefreq", crtscreentype="kHz31", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Sonic.chd")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz31at120.value


def test_given_trisync_and_rgb_dual_then_use_tv_tri_freq_mode():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHzTriFreq", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value

def test_given_trisync_and_rgb_jamma_then_use_arcade_tri_freq_mode():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHzTriFreq", crtadaptor="recalboxrgbjamma", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15Jamma.value


def test_given_1531_and_rgb_jamma_then_use_arcade_1531_mode():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHzMulti1531", crtadaptor="recalboxrgbjamma", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15Jamma.value

def test_given_1531_and_rgb_dual_then_use_tv_1531_mode():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHzMulti1531", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value


def test_given_1525_and_rgb_jamma_then_use_arcade_1525_mode():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHzMulti1525", crtadaptor="recalboxrgbjamma", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15Jamma.value

def test_given_1525_and_rgb_dual_then_use_tv_1525_mode():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHzMulti1525", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value


def test_given_1525_and_force_384p_then_use_384p_mode():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="384p", crtscreentype="kHzMulti1525", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "scud.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz25Forced384p.value

def test_given_trifreq_and_force_384p_then_use_384p_mode():
    emulator = configureForCrt(
        Emulator(name='model3', videoMode='1920x1080', ratio='auto', emulator='supermodel', core=''),
        crtresolutiontype="384p", crtscreentype="kHzTriFreq", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "scud.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz25Forced384p.value

def test_given_15khz_and_force_240p_on_psx_then_use_force15kHzprogressive_mode():
    emulator = configureForCrt(
        Emulator(name='psx', videoMode='1920x1080', ratio='auto', emulator='libretro', core='swanstation'),
        crtresolutiontype="240", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "ff7.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15ForcedProgressive.value


def test_given_multifreq_and_force_240p_on_psx_then_use_force15kHzprogressive_mode():
    emulator = configureForCrt(
        Emulator(name='psx', videoMode='1920x1080', ratio='auto', emulator='libretro', core='swanstation'),
        crtresolutiontype="240", crtscreentype="kHzTriFreq", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "ff7.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15ForcedProgressive.value

    emulator = configureForCrt(
        Emulator(name='psx', videoMode='1920x1080', ratio='auto', emulator='libretro', core='swanstation'),
        crtresolutiontype="240", crtscreentype="kHzMulti1525", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "ff7.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15ForcedProgressive.value

    emulator = configureForCrt(
        Emulator(name='psx', videoMode='1920x1080', ratio='auto', emulator='libretro', core='swanstation'),
        crtresolutiontype="240", crtscreentype="kHzMulti1531", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "ff7.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15ForcedProgressive.value

def test_given_multifreq_and_auto_on_psx_then_use_15khz_mode():
    emulator = configureForCrt(
        Emulator(name='psx', videoMode='1920x1080', ratio='auto', emulator='libretro', core='swanstation'),
        crtresolutiontype="auto", crtscreentype="kHzTriFreq", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "ff7.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value

    emulator = configureForCrt(
        Emulator(name='psx', videoMode='1920x1080', ratio='auto', emulator='libretro', core='swanstation'),
        crtresolutiontype="auto", crtscreentype="kHzMulti1525", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "ff7.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value

    emulator = configureForCrt(
        Emulator(name='psx', videoMode='1920x1080', ratio='auto', emulator='libretro', core='swanstation'),
        crtresolutiontype="auto", crtscreentype="kHzMulti1531", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "ff7.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value


def test_given_multifreq_and_240p_on_dreamcast_then_use_forced_240p_mode():
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="240", crtscreentype="kHzTriFreq", crtadaptor="recalboxrgbdual2", crtsuperrez="0", hdgame=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "crazy.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15Forced240p.value

    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="240", crtscreentype="kHzMulti1525", crtadaptor="recalboxrgbdual2", crtsuperrez="0", hdgame=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "crazy.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15Forced240p.value

    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="240", crtscreentype="kHzMulti1531", crtadaptor="recalboxrgbdual2", crtsuperrez="0", hdgame=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "crazy.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15Forced240p.value



# def test_given_rpi5_and_rrgbd2_and_disable_interlace_on_handheld_should_disable_interlaced_on_handhelds(mocker):
#     emulator = configureForCrt(
#         Emulator(name='gb', videoMode='1920x1080', ratio='auto', emulator='libretro', core='gambatte'),
#         crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtsuperrez="0",
#         avoid_interlaced_on_tate_and_handhelds=True)
#     config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.gb")
#
#     assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value
#     assert config_lines["crt_switch_resolution_no_interlaced"] == 1
#     assert config_lines["video_smooth"] == '"true"'
#

# def test_given_handhelds_on_15khz_should_disable_vsync():
#     emulator = configureForCrt(
#         Emulator(name='gb', videoMode='1920x1080', ratio='auto', emulator='libretro', core='gambatte'),
#         crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtsuperrez="0")
#     config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "SML.gb")
#
#     assert config_lines["video_vsync"] == '"false"'

# def test_given_rpi5_and_rrgbd2_and_disable_interlace_on_handheld_should_not_enable_smooth_on_31khz():
#     emulator = configureForCrt(
#         Emulator(name='gb', videoMode='1920x1080', ratio='auto', emulator='libretro', core='gambatte'),
#         crtresolutiontype="auto", crtscreentype="kHz31", crtadaptor="recalboxrgbdual2", crtsuperrez="0",
#         avoid_interlaced_on_tate_and_handhelds=True)
#     config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.gb")
#
#     assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz31.value
#     assert config_lines["crt_switch_resolution_no_interlaced"] == 0
#     assert config_lines["video_smooth"] == '"false"'

def test_given_15kHz_and_gb_then_use_forced_240p_mode():
    emulator = configureForCrt(
        Emulator(name='gb', videoMode='1920x1080', ratio='auto', emulator='libretro', core='gambatte'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtsuperrez="0", crthandheldformat="fullscreen")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "sml.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15Forced240p.value
    assert config_lines["video_smooth"] == '"true"'

def test_given_15kHz_50hz_and_gb_then_use_15khz_mode():
    emulator = configureForCrt(
        Emulator(name='gb', videoMode='1920x1080', ratio='auto', emulator='libretro', core='gambatte'),
        crtvideostandard="pal", crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtsuperrez="0", crthandheldformat="fullscreen")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "sml.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.PAL.value
    assert config_lines["video_smooth"] == '"true"'


def test_given_15kHz_and_gb_then_use_forced_240p_and_force_original_handheld_format():
    emulator = configureForCrt(
        Emulator(name='gb', videoMode='1920x1080', ratio='auto', emulator='libretro', core='gambatte'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtsuperrez="0", crthandheldformat="original")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "sml.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15Forced240p.value
    assert config_lines["video_smooth"] == '"false"'
    assert config_lines["video_scale_integer_axis"] == '"1"'
    assert config_lines["video_scale_integer"] == '"true"'


def test_given_non_15kHz_modes_and_gb_then_use_handheld_format():
    for screentype in ["kHz31", "kHzMulti1531", "kHzTriFreq"]:
        emulator = configureForCrt(
            Emulator(name='gb', videoMode='1920x1080', ratio='auto', emulator='libretro', core='gambatte'),
            crtresolutiontype="auto", crtscreentype=screentype, crtadaptor="recalboxrgbdual2", crtsuperrez="0", crthandheldformat="fullscreen")
        config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "sml.zip")

        assert config_lines["video_smooth"] == '"true"'
        assert config_lines["video_scale_integer_axis"] == ''
        assert config_lines["video_scale_integer"] == ''

def test_given_15kHz_and_gamegear_then_use_forced_240p_and_force_fullscreen_handheld_format_and_force_core_gearsystem():
    emulator = configureForCrt(
        Emulator(name='gamegear', videoMode='1920x1080', ratio='auto', emulator='libretro', core='genesisplusgx'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", crtsuperrez="0", crthandheldformat="fullscreen")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "sml.zip")

    assert emulator.Core == "gearsystem"
    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15Forced240p.value
    assert config_lines["video_smooth"] == '"true"'
    assert config_lines["video_scale_integer_axis"] == ''
    assert config_lines["video_scale_integer"] == ''

def test_given_15khz_and_nolowresmode_should_enable_switchrez_57hz_mode():
    emulator = configureForCrt(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual", avoid_low_freq_modes=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15ForcedProgressiveMin57Hz.value

def test_given_15khz_and_nolowresmode_should_not_enable_switchrez_57hz_mode_on_hd_games():
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual", avoid_low_freq_modes=True, hdgame=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15.value

def test_given_15khz_and_n64_should_disable_interlaced():
    emulator = configureForCrt(
        Emulator(name='n64', videoMode='1920x1080', ratio='auto', emulator='libretro', core='parallel_n64'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual", avoid_low_freq_modes=True, hdgame=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario64.zip")

    assert config_lines["crt_switch_resolution_no_interlaced"] == 1

def test_given_15khz_and_tate_game_should_fix_240p_mode_and_bilineair_filtering():
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", rotation=1)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "1943.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz15Forced240p.value
    assert config_lines["video_smooth"] == '"true"'

def test_given_31khz_and_tate_game_should_fix_480p_mode_and_bilinear_filtering():
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtresolutiontype="auto", crtscreentype="kHz31", crtadaptor="recalboxrgbdual2", rotation=1)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "1943.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz31Forced480p.value
    assert config_lines["video_smooth"] == '"true"'

    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtresolutiontype="auto", crtscreentype="kHzMulti1531", crtadaptor="recalboxrgbdual2", rotation=1)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "1943.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz31Forced480p.value
    assert config_lines["video_smooth"] == '"true"'

    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtresolutiontype="auto", crtscreentype="kHzTriFreq", crtadaptor="recalboxrgbdual2", rotation=1)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "1943.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz31Forced480p.value
    assert config_lines["video_smooth"] == '"true"'


def test_given_1525khz_and_tate_game_should_fix_480p_mode_and_bilinear_filtering():
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtresolutiontype="auto", crtscreentype="kHzMulti1525", crtadaptor="recalboxrgbdual2", rotation=1)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "1943.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz25Forced384p.value
    assert config_lines["video_smooth"] == '"true"'

def test_given_15khz_and_downscaled_game_should_use_gl_video_driver():
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtresolutiontype="240", crtscreentype="kHz15", crtadaptor="recalboxrgbdual2", hdgame=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "cvs2.zip")

    assert config_lines["video_driver"] == '"gl"'

def test_given_15khz_and_not_interlaced_then_downscaled_game_should_use_gl_video_driver():
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual", hdgame=True, avoid_interlaced=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "cvs2.zip")

    assert config_lines["video_driver"] == '"gl"'

def test_given_dosbox_15khz_and_480i_then_force_480i():
    emulator = configureForCrt(
        Emulator(name='dos', videoMode='1920x1080', ratio='auto', emulator='libretro', core='dosbox-pure'),
        crtresolutiontype="480", crtscreentype="kHz15", crtadaptor="recalboxrgbdual")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "fullthrottle.zip")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.ini.value

def test_given_dosbox_31khz_then_do_not_force_480i():
    emulator = configureForCrt(
        Emulator(name='dos', videoMode='1920x1080', ratio='auto', emulator='libretro', core='dosbox-pure'),
        crtresolutiontype="auto", crtscreentype="kHz31", crtadaptor="recalboxrgbdual")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "fullthrottle.zip")

    assert config_lines["crt_switch_resolution"] != RetroarchScreenTypeIndex.ini.value


# Scanlines pick shader ext matching the active video_driver: glcore+.slangp by default, gl+.glslp under pixel-perfect.

def test_given_kHz31_scanlines_no_hdgame_then_use_glcore_and_slangp():
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHz31", crtresolutiontype="480", crtscanlines="heavy")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "ptblank.zip")

    assert config_lines["video_driver"] == '"glcore"'
    assert config_lines["video_shader"] == 'rrgbd-scanlines-heavy.slangp'

def test_given_kHz31_scanlines_with_hdgame_force240_then_pixel_perfect_wins_no_scanline_shader():
    # Force240 + HDGame: pixel-perfect forces gl, manage_scanlines doesn't fire (only Force480/Auto).
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtscreentype="kHz31", crtresolutiontype="240", crtscanlines="heavy", hdgame=True)
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "cvs2.zip")

    assert config_lines["video_driver"] == '"gl"'
    assert config_lines.get("video_shader", "") == ""

def test_given_kHz15_hdgame_no_interlace_with_scanlines_then_keep_gl_and_glslp():
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="auto", crtscreentype="kHz15", crtadaptor="recalboxrgbdual", hdgame=True,
        avoid_interlaced=True, crtscanlines="heavy")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "cvs2.zip")

    assert config_lines["video_driver"] == '"gl"'
    # kHz15 isn't in manage_scanlines' eligible list, no shader override expected.
    assert config_lines.get("video_shader", "") == ""

def test_given_kHzTriFreq_scanlines_no_hdgame_then_use_glcore_and_slangp():
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHzTriFreq", crtresolutiontype="480", crtscanlines="medium")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert config_lines["video_driver"] == '"glcore"'
    assert config_lines["video_shader"] == 'rrgbd-scanlines-medium.slangp'

def test_given_kHz31_no_scanlines_does_not_force_video_driver():
    emulator = configureForCrt(
        Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fbneo'),
        crtscreentype="kHz31", crtresolutiontype="480")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Mario.smc")

    assert "video_driver" not in config_lines or config_lines["video_driver"] == ''

def test_given_jvs_should_avoid_interlaced():
    emulator = configureForCrt(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
        crtresolutiontype="480", crtscreentype="kHz31", crtadaptor="recalboxjvs", crtsuperrez="0")
    config_lines, core_config, pre, post = LibretroConfigCRTSwitchres().createConfigFor(emulator, "Sonic.chd")

    assert config_lines["crt_switch_resolution"] == RetroarchScreenTypeIndex.kHz31.value
    assert config_lines["crt_switch_resolution_no_interlaced"] == 1

