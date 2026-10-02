from configgen.Emulator import Emulator
from configgen.generators.libretro.libretroRetroarch import LibretroRetroarch
from configgen.settings.keyValueSettings import keyValueSettings


def make_retroarch(name, core, rom=""):
    system = Emulator(name=name, videoMode='1920x1080', ratio='auto', emulator='libretro', core=core)
    return LibretroRetroarch(system, keyValueSettings(""), {}, False, keyValueSettings(""), rom)


def test_naomi2_pi5_gets_bigger_audio_buffer(mocker):
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    settings = keyValueSettings("")
    make_retroarch('naomi2', 'flycast-next').manage_audio_buffer(settings)

    assert settings.getString("audio_latency", "") == "256"
    assert settings.getString("audio_rate_control_delta", "") == "0.010"


def test_naomi2_pi5_beachspikers_gets_deeper_buffer(mocker):
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    settings = keyValueSettings("")
    make_retroarch('naomi2', 'flycast-next', '/recalbox/share/roms/naomi2/beachspi.zip').manage_audio_buffer(settings)

    assert settings.getString("audio_latency", "") == "512"
    assert settings.getString("audio_rate_control_delta", "") == "0.010"


def test_non_naomi2_keeps_ra_default_buffer(mocker):
    # Written every launch so the naomi2 bump never leaks to the next non-naomi2 game.
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    settings = keyValueSettings("")
    make_retroarch('snes', 'snes9x').manage_audio_buffer(settings)

    assert settings.getString("audio_latency", "") == "64"
    assert settings.getString("audio_rate_control_delta", "") == "0.005"


def test_naomi2_not_pi5_keeps_ra_default_buffer(mocker):
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=False, new_callable=mocker.PropertyMock)
    settings = keyValueSettings("")
    make_retroarch('naomi2', 'flycast-next').manage_audio_buffer(settings)

    assert settings.getString("audio_latency", "") == "64"
