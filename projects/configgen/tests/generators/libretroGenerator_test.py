#!/usr/bin/env python
import shutil
from pathlib import Path

import pytest
import configgen.generators.libretro.libretroConfigurations as libretroConfigurations
import configgen.generators.libretro.libretroGenerator as libretroGenerator
import configgen.generators.libretro.libretroLightGuns as libretroLightGuns
import configgen.generators.libretro.libretroControllers as libretroControllers
from configgen.Command import Command
from configgen.Emulator import Emulator, ExtraArguments
from configgen.crt.Mode import Mode
from configgen.generators.libretro.libretroGenerator import LibretroGenerator
from configgen.settings.keyValueSettings import keyValueSettings
import configgen.controllers.controller as controllersConfig
from tests.Givens import givenThoseFiles
from tests.generators.FakeArguments import Arguments
from unittest.mock import MagicMock, patch


def function_returning_list():
    return ('toto')

@pytest.fixture
def emulator():
    libretroConfigurations.recalboxFiles.retroarchRoot = 'tests/tmp/ra'
    libretroConfigurations.recalboxFiles.retroarchCustom = 'tests/tmp/ra-custom.cfg'
    libretroConfigurations.recalboxFiles.retroarchCoreCustom = 'tests/tmp/ra-core-custom.cfg'
    libretroGenerator.recalboxFiles.BIOS = 'tests/tmp/bios/'
    libretroLightGuns.esLightGun = 'tests/resources/lightgun.xml'
    libretroLightGuns.GAME_INFO_PATH = 'tests/resources/es_state.inf'

#    map_controller_buttons_mock = MagicMock(return_value=fake_button_mapping())
#    libretroControllers.LibretroControllers._MapControllerButtons = map_controller_buttons_mock

    return LibretroGenerator()


@pytest.fixture
def emulator_mayflash():
    libretroLightGuns.recalboxFiles.esLightGun = 'tests/tmp/lightgun.xml'
    libretroConfigurations.recalboxFiles.retroarchRoot = 'tests/tmp/ra'
    libretroConfigurations.recalboxFiles.retroarchCustom = 'tests/tmp/ra-custom.cfg'
    libretroConfigurations.recalboxFiles.retroarchCoreCustom = 'tests/tmp/ra-core-custom.cfg'
    libretroGenerator.recalboxFiles.BIOS = 'tests/tmp/bios/'
    shutil.copyfile('tests/resources/lightgun.xml', 'tests/tmp/lightgun.xml')

    return LibretroGenerator()


@pytest.fixture
def system_nes():
    return Emulator(name='nes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='fceumm')


@pytest.fixture
def system_gb_with_overlays():
    gb = Emulator(name='gb', videoMode='1920x1080', ratio='auto', emulator='libretro', core='gb')
    recalbox_conf = keyValueSettings("", True)
    recalbox_conf.setString("global.recalboxoverlays", "1")
    gb.configure(recalbox_conf, ExtraArguments("", "", "", "", "", "", "", "", "", "", "", "", ""))
    return gb


@pytest.fixture
def system_snes():
    return Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x')


@pytest.fixture
def system_px68k():
    return Emulator(name='x68000', videoMode='1920x1080', ratio='auto', emulator='libretro', core='px68k')


@pytest.fixture
def system_quasi88():
    return Emulator(name='pc88', videoMode='1920x1080', ratio='auto', emulator='libretro', core='quasi88')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_snes(emulator, system_snes, controller_configuration):
    command = emulator.generate(system_snes, controller_configuration, keyValueSettings("", False),
                                Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/retroarch', '-L', '/usr/lib/libretro/snes9x_libretro.so',
                             '--config', 'tests/tmp/ra-custom.cfg',
                             '--appendconfig', 'tests/tmp/ra-custom.cfg.overrides.cfg',
                             'path/to/test']


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_nes_mayflash(emulator_mayflash, system_nes, controller_configuration, mocker):
    mocker.patch('os.path.exists', return_value=True)
    command = emulator_mayflash.generate(system_nes, controller_configuration, keyValueSettings("", False),
                                         Arguments('path/to/duckhunt'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/retroarch', '-L', '/usr/lib/libretro/fceumm_libretro.so',
                             '--config', 'tests/tmp/ra-custom.cfg',
                             '--appendconfig', 'tests/tmp/ra-custom.cfg.overrides.cfg',
                             'path/to/duckhunt']


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_px68k_hdd(emulator, system_px68k, controller_configuration, mocker):
    mocker.patch('os.path.getsize', return_value=2097152)
    command = emulator.generate(system_px68k, controller_configuration, keyValueSettings("", False),
                                Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/retroarch', '-L', '/usr/lib/libretro/px68k_libretro.so',
                             '--config', 'tests/tmp/ra-custom.cfg',
                             '--appendconfig', 'tests/tmp/ra-custom.cfg.overrides.cfg',
                             'path/to/test']


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_px68k_floppy(emulator, system_px68k, controller_configuration, mocker):
    mocker.patch('os.path.getsize', return_value=720000)
    command = emulator.generate(system_px68k, controller_configuration, keyValueSettings("", False),
                                Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/retroarch', '-L', '/usr/lib/libretro/px68k_libretro.so',
                             '--config', 'tests/tmp/ra-custom.cfg',
                             '--appendconfig', 'tests/tmp/ra-custom.cfg.overrides.cfg',
                             'path/to/test']


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_quasi88(emulator, system_quasi88, controller_configuration):
    command = emulator.generate(system_quasi88, controller_configuration, keyValueSettings("", False),
                                Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/retroarch', '-L', '/usr/lib/libretro/quasi88_libretro.so',
                             '--config', 'tests/tmp/ra-custom.cfg',
                             '--appendconfig', 'tests/tmp/ra-custom.cfg.overrides.cfg',
                             'path/to/test']


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_quasi88_multi_disk_1(emulator, system_quasi88, controller_configuration, mocker):
    mocker.patch("glob.glob", return_value=['path/to/test(Disk 1 of 5).zip',
                                            'path/to/test(Disk 2 of 5).zip',
                                            'path/to/test(Disk 3 of 5).zip',
                                            'path/to/test(Disk 4 of 5).zip',
                                            'path/to/test(Disk 5 of 5).zip'])
    command = emulator.generate(system_quasi88, controller_configuration, keyValueSettings("", False),
                                Arguments('path/to/test(Disk 1 of 5).zip'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/retroarch', '-L', '/usr/lib/libretro/quasi88_libretro.so',
                             '--config', 'tests/tmp/ra-custom.cfg',
                             '--appendconfig', 'tests/tmp/ra-custom.cfg.overrides.cfg',
                             '--subsystem', 'pc88_5_disk',
                             'path/to/test(Disk 1 of 5).zip',
                             'path/to/test(Disk 2 of 5).zip',
                             'path/to/test(Disk 3 of 5).zip',
                             'path/to/test(Disk 4 of 5).zip',
                             'path/to/test(Disk 5 of 5).zip']


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_quasi88_multi_disk_2(emulator, system_quasi88, controller_configuration, mocker):
    mocker.patch("glob.glob", return_value=['path/to/test(Disk 1 of 5)(Disk A).zip',
                                            'path/to/test(Disk 2 of 5)(Disk B).zip',
                                            'path/to/test(Disk 3 of 5)(Disk C).zip',
                                            'path/to/test(Disk 4 of 5)(Disk D).zip',
                                            'path/to/test(Disk 5 of 5)(Disk E).zip'])
    command = emulator.generate(system_quasi88, controller_configuration, keyValueSettings("", False),
                                Arguments('path/to/test(Disk 1 of 5)(Disk A).zip'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/retroarch', '-L', '/usr/lib/libretro/quasi88_libretro.so',
                             '--config', 'tests/tmp/ra-custom.cfg',
                             '--appendconfig', 'tests/tmp/ra-custom.cfg.overrides.cfg',
                             '--subsystem', 'pc88_5_disk',
                             'path/to/test(Disk 1 of 5)(Disk A).zip',
                             'path/to/test(Disk 2 of 5)(Disk B).zip',
                             'path/to/test(Disk 3 of 5)(Disk C).zip',
                             'path/to/test(Disk 4 of 5)(Disk D).zip',
                             'path/to/test(Disk 5 of 5)(Disk E).zip']


# @pytest.mark.usefixtures("controller_configuration")
# def test_crt_enabled_create_mode_configuration(mocker, emulator, system_snes, controller_configuration):
#     recalbox_conf = keyValueSettings("", True)
#     givenThoseFiles(mocker, {
#         "/recalbox/share/system/configs/crt/systems.txt": "snes,snes9x,pal,15kHz,progressive,snes:288@50p,1740,224\nsnes,snes9x,ntsc,15kHz,progressive,snes:240@60p,0,0",
#         "/recalbox/share/system/configs/crt/modes.txt": "snes:240@60p,1920 1 78 192 210 240 1 3 3 16 0 0 0 60 0 37730000 1,60.1\nsnes:288@50p,1920 1 78 192 210 288 1 3 3 16 0 0 0 50 0 37730000 1,50.1"})
#
#     system_snes.configure(recalbox_conf,
#                           ExtraArguments("", "", "", "", "", "", "", "", "auto", "progressive", "15kHz", "recalboxrgbdual",
#                                           "auto", False,
#                                          crt_horizontaloffset_p1920x240=-2, crt_verticaloffset_p1920x240=-2, crt_viewportwidth_p1920x240=-10,
#                                          crt_horizontaloffset_p1920x288=1, crt_verticaloffset_p1920x288=-2))
#
#     emulator.generate(system_snes, controller_configuration, recalbox_conf, Arguments('path/to/rom'))
#     generated_config = Path(libretroConfigurations.recalboxFiles.retroarchCustom).read_text()
#     assert 'crt_switch_timings_ntsc = "1920 1 90 192 198 240 1 5 3 14 0 0 0 60 0 37730000 1"' in generated_config
#     assert 'crt_switch_timings_pal = "1920 1 72 192 216 288 1 5 3 14 0 0 0 50 0 37730000 1"' in generated_config
#     assert 'custom_viewport_width_ntsc = 1780' in generated_config
#     assert 'custom_viewport_width_pal = 1740' in generated_config
#     assert 'video_refresh_rate_ntsc = "60.1"' in generated_config
#     assert 'video_refresh_rate_pal = "50.1"' in generated_config


# @pytest.mark.usefixtures("controller_configuration")
# def test_crt_enabled_create_overlay_configuration(mocker, emulator, system_gb_with_overlays, controller_configuration):
#     recalbox_conf = keyValueSettings("", True)
#     recalbox_conf.setString("global.recalboxoverlays", "1")
#
#     system_gb_with_overlays.configure(recalbox_conf,
#                                       ExtraArguments("", "", "", "", "", "", "", "", "auto", "progressive", "15kHz",
#                                                      "recalboxrgbdual"))
#
#     givenThoseFiles(mocker, {
#         "/recalbox/share/system/configs/crt/systems.txt": "gb,gb,pal,15kHz,progressive,gb:pal:240@50p,1740,224\ngb,gb,ntsc,15kHz,progressive,gb:ntsc:224@60p,0,0",
#         "/recalbox/share/system/configs/crt/modes.txt": "gb:pal:240@50p,1920 1 78 192 210 240 1 3 3 16 0 0 0 50 0 37730000 1,501\ngb:ntsc:224@60p,1920 1 78 192 210 224 1 3 3 16 0 0 0 60 0 37730000 1,60.1"})
#
#     # Easy way to fake overlay file on file system
#     mocker.patch('os.path.isfile', return_value=True)
#
#     commandLine: Command = emulator.generate(system_gb_with_overlays, controller_configuration, recalbox_conf,
#                                              Arguments('path/to/rom'))
#     assert "/recalbox/share_init/240poverlays/gb/gb.cfg" in commandLine.array[6]


@pytest.mark.usefixtures("controller_configuration")
def test_sgb_enabled_configure_core_for_sgb(mocker, emulator, controller_configuration):
    recalbox_conf = keyValueSettings("", True)

    gb = Emulator(name='gb', videoMode='1920x1080', ratio='auto', emulator='libretro', core='gb')
    gb.configure(recalbox_conf, ExtraArguments("", "", "", "", "", "", "", "", "", "", "", "", "", sgb=True))

    emulator.generate(gb, controller_configuration, recalbox_conf,
                                             Arguments('path/to/rom.zip'))

    retroarchConf = Path(libretroConfigurations.recalboxFiles.retroarchCustom).read_text()
    coreConf = Path(libretroConfigurations.recalboxFiles.retroarchCoreCustom).read_text()
    assert 'aspect_ratio_index = "22"' in retroarchConf
    assert 'mgba_gb_model = "Super Game Boy"' in coreConf
    assert 'mgba_sgb_borders = "ON"' in coreConf
    assert 'mesen-s_gbmodel = "Super Game Boy"' in coreConf


@pytest.mark.usefixtures("controller_configuration")
def test_sgb_disabled_configure_core_for_sgb(mocker, emulator, controller_configuration):
    recalbox_conf = keyValueSettings("", True)
    recalbox_conf.setString("global.ratio", "auto")
    gb = Emulator(name='gb', videoMode='1920x1080', ratio='auto', emulator='libretro', core='gb')
    gb.configure(recalbox_conf, ExtraArguments("", "", "", "", "", "", "", "", "", "", "", "", "", sgb=False))

    emulator.generate(gb, controller_configuration, recalbox_conf,
                                             Arguments('path/to/rom.zip'))

    retroarchConf = Path(libretroConfigurations.recalboxFiles.retroarchCustom).read_text()
    coreConf = Path(libretroConfigurations.recalboxFiles.retroarchCoreCustom).read_text()
    assert 'aspect_ratio_index' not in retroarchConf
    assert 'mgba_gb_model = "Autodetect"' in coreConf
    assert 'mgba_sgb_borders = "OFF"' in coreConf
    assert 'mesen-s_gbmodel = "Auto"' in coreConf

def test_naomi2_on_rpi5_set_alpha_sorting_per_strip(mocker, emulator, controller_configuration):
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True)

    recalbox_conf = keyValueSettings("", True)
    naomi2 = Emulator(name='naomi2', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next')
    naomi2.configure(recalbox_conf, ExtraArguments("", "", "", "", "", "", "", "", "", "", "", "", ""))

    emulator.generate(naomi2, controller_configuration, recalbox_conf,
                                             Arguments('path/to/rom.zip'))

    coreConf = Path(libretroConfigurations.recalboxFiles.retroarchCoreCustom).read_text()
    assert 'reicast_alpha_sorting = "per-strip (fast, least accurate)"' in coreConf
    assert 'reicast_anisotropic_filtering = "2"' in coreConf
    # naomi2 overrides the flycast overclock back to 200, and keeps frameskip off (non-Beach-Spikers).
    assert 'reicast_sh4clock = "200"' in coreConf
    assert 'reicast_auto_skip_frame = "disabled"' in coreConf

    naomi = Emulator(name='naomi', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next')
    naomi.configure(recalbox_conf, ExtraArguments("", "", "", "", "", "", "", "", "", "", "", "", ""))

    emulator.generate(naomi, controller_configuration, recalbox_conf,
                                             Arguments('path/to/rom.zip'))

    coreConf = Path(libretroConfigurations.recalboxFiles.retroarchCoreCustom).read_text()
    assert 'reicast_alpha_sorting = "per-triangle (normal)"' in coreConf
    assert 'reicast_anisotropic_filtering = "4"' in coreConf
    # naomi (non-naomi2 flycast) keeps the 300 overclock and no frameskip.
    assert 'reicast_sh4clock = "300"' in coreConf
    assert 'reicast_auto_skip_frame = "disabled"' in coreConf


def test_naomi2_beachspi_gets_frameskip(mocker, emulator, controller_configuration):
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True)
    recalbox_conf = keyValueSettings("", True)
    naomi2 = Emulator(name='naomi2', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next')
    naomi2.configure(recalbox_conf, ExtraArguments("", "", "", "", "", "", "", "", "", "", "", "", ""))

    emulator.generate(naomi2, controller_configuration, recalbox_conf,
                                             Arguments('roms/naomi2/beachspi.zip'))

    coreConf = Path(libretroConfigurations.recalboxFiles.retroarchCoreCustom).read_text()
    assert 'reicast_auto_skip_frame = "some"' in coreConf   # frameskip on for Beach Spikers only
    assert 'reicast_sh4clock = "200"' in coreConf           # still the naomi2 override


def test_hd_mode_change_core(mocker, emulator, controller_configuration):
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True)

    recalbox_conf = keyValueSettings("", True)
    recalbox_conf.setBool("global.hdmode", True)
    recalbox_conf.setBool("global.widescreenmode", True)

    dreamcast = Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='badcore')
    dreamcast.configure(recalbox_conf, ExtraArguments("", "", "", "", "", "", "", "", "", "", "", "", ""))

    command = emulator.generate(dreamcast, controller_configuration, recalbox_conf,
                                             Arguments('path/to/rom.zip'))

    coreConf = Path(libretroConfigurations.recalboxFiles.retroarchCoreCustom).read_text()
    assert 'reicast_internal_resolution = "1024x768"' in coreConf
    assert 'reicast_widescreen_hack = "enabled"' in coreConf
    assert '/usr/lib/libretro/flycast_libretro.so' in command.array


def test_widescreen_mode_disabled_tate(mocker, emulator, controller_configuration):
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True)

    recalbox_conf = keyValueSettings("", True)
    recalbox_conf.setBool("global.hdmode", True)
    recalbox_conf.setBool("global.widescreenmode", True)

    dreamcast = Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='badcore')
    dreamcast.configure(recalbox_conf, ExtraArguments("", "", "", "", "", "", "", "", "", "", "", "", "", rotation=1))

    command = emulator.generate(dreamcast, controller_configuration, recalbox_conf,
                                             Arguments('path/to/rom.zip'))

    print(command.array)
    coreConf = Path(libretroConfigurations.recalboxFiles.retroarchCoreCustom).read_text()
    assert 'reicast_internal_resolution = "1024x768"' in coreConf
    assert 'reicast_widescreen_hack = "disabled"' in coreConf
    assert '/usr/lib/libretro/flycast_libretro.so' in command.array



@pytest.mark.usefixtures("controller_configuration")
def test_the_slot_the_player_picked_is_handed_over_untouched(emulator, system_snes, controller_configuration):
    args = Arguments('path/to/test')
    args.entryslot = '0'
    command = emulator.generate(system_snes, controller_configuration, keyValueSettings("", False), args)
    assert '--entryslot' in command.array
    assert command.array[command.array.index('--entryslot') + 1] == '0'


@pytest.mark.usefixtures("controller_configuration")
def test_starting_without_a_state_asks_retroarch_for_no_slot(emulator, system_snes, controller_configuration):
    command = emulator.generate(system_snes, controller_configuration, keyValueSettings("", False),
                                Arguments('path/to/test'))
    assert '--entryslot' not in command.array
