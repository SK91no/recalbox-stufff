#!/usr/bin/env python
import pytest
import os

from unittest.mock import patch, mock_open
from pprint import pprint

from Nvidia.NvidiaInstaller import GpuError, NvidiaInstaller

# versions file is in each nvidia update path (/usr/lib/updates/nvidia-470/versions)
# it contains a list of space separated strings matching the support level (production or legacy)
# version number short, long...
VERSIONS = 'production 123'

# the file supported-gpus.json is generated at the end of nvidia production driver BR package
# a py script is compute the supported-gpus.json of the driver version and generate a new json from it
# it's a simpler lighter version with a list of map of versions which contains a list of supported gpu
# deviceid. See example below
SUPPORTED_GPUS_JSON_PATH = "/recalbox/system/hardware/videocard/supported-gpus.json"
SUPPORTED_GPUS_JSON = '\
{ \
  "production": [\
    "0x4242"\
  ],\
  "42": [\
    "0x1234"\
  ]\
}'


def filename_to_mock_open(filename: str, *args, **kwargs):
    """Returns a `mock_open` object for the corresponding filename.

    :param filename: Name of file to open.
    :param args: Positional arguments passed into `open`, e.g. "r".
    :param kwargs: Keyword arguments passed into `open`.
    """
    return {
        SUPPORTED_GPUS_JSON_PATH: mock_open(read_data=SUPPORTED_GPUS_JSON)(),
        "/usr/lib/updates/nvidia-123/versions": mock_open(read_data=VERSIONS)(),
    }[filename]


def mock_open_multiple(on_open):
    """A nested `mock_open` which permits using multiple different `mock_open`
    objects depending upon the given parameters (e.g. filename). This allows
    us to patch some code that calls `open()` multiple times, and handle each
    call to `open()` differently.

    :param on_open: Function to call instead of `open`. The given function
        gets passed all the parameters that `open` would have received.
    """
    mock_files = mock_open()
    mock_files.side_effect = on_open
    mock_files.return_value = None
    return mock_files


def test_set_search_path():
    m = mock_open_multiple(on_open=filename_to_mock_open)
    with \
            patch("builtins.open", m), \
            patch('Nvidia.NvidiaInstaller.os.path.abspath', return_value=SUPPORTED_GPUS_JSON_PATH):
        ni = NvidiaInstaller("123")
        ni.set_search_path("/path/to/somewhere")

    assert ni.search_path == "/path/to/somewhere"


def test_load_supported_gpus_file():
    m = mock_open_multiple(on_open=filename_to_mock_open)
    with \
            patch("builtins.open", m), \
            patch('Nvidia.NvidiaInstaller.os.path.abspath', return_value=SUPPORTED_GPUS_JSON_PATH):
        ni = NvidiaInstaller("123")
        ni.load_supported_gpus_file()

    assert "0x4242" in ni.gpus_data["production"]


def test_is_gpu_supported_false():
    m = mock_open_multiple(on_open=filename_to_mock_open)
    with \
            patch("builtins.open", m), \
            patch('Nvidia.NvidiaInstaller.os.path.abspath', return_value=SUPPORTED_GPUS_JSON_PATH):
        ni = NvidiaInstaller("123")
        ni.load_supported_gpus_file()
        supported = ni.is_gpu_supported("0x1234")

    assert supported == False


def test_is_gpu_supported_true(mocker):
    m = mock_open_multiple(on_open=filename_to_mock_open)
    with \
            patch("builtins.open", m), \
            patch('Nvidia.NvidiaInstaller.os.path.abspath', return_value=SUPPORTED_GPUS_JSON_PATH):
        ni = NvidiaInstaller("123")
        ni.load_supported_gpus_file()
        supported = ni.is_gpu_supported("0x4242")

    assert supported == True


def test_is_gpu_supported_unknown(mocker):
    m = mock_open_multiple(on_open=filename_to_mock_open)
    with \
            patch("builtins.open", m), \
            patch('Nvidia.NvidiaInstaller.os.path.abspath', return_value=SUPPORTED_GPUS_JSON_PATH):
        ni = NvidiaInstaller("123")
        ni.load_supported_gpus_file()
        supported = ni.is_gpu_supported("0x0000")

    assert supported == False


def test_install_supported(mocker):
    mocker.patch.object(NvidiaInstaller, "do_install")

    m = mock_open_multiple(on_open=filename_to_mock_open)
    with \
            patch("builtins.open", m), \
            patch('Nvidia.NvidiaInstaller.os.path.abspath', return_value=SUPPORTED_GPUS_JSON_PATH):
        ni = NvidiaInstaller("123")
        try:
            ni.install("0x4242")
        except Exception as e:
            pytest.fail(f"Exception thrown: {e}")


def test_install_unsupported(mocker):
    mocker.patch.object(NvidiaInstaller, "do_install")

    m = mock_open_multiple(on_open=filename_to_mock_open)
    with \
            patch("builtins.open", m), \
            patch('Nvidia.NvidiaInstaller.os.path.abspath', return_value=SUPPORTED_GPUS_JSON_PATH):
        ni = NvidiaInstaller("123")
        try:
            ni.install("0x1234")
            pytest.fail("should not go here")
        except GpuError as e:
            assert e.message == "Controller not supported."
        except Exception as e:
            pytest.fail(f"Wrong exception: {e}")


def test_install_unknown(mocker):
    mocker.patch.object(NvidiaInstaller, "do_install")

    m = mock_open_multiple(on_open=filename_to_mock_open)
    with \
            patch("builtins.open", m), \
            patch('Nvidia.NvidiaInstaller.os.path.abspath', return_value=SUPPORTED_GPUS_JSON_PATH):
        ni = NvidiaInstaller("123")
        try:
            ni.install("0x0000")
            pytest.fail("should not go here")
        except GpuError as e:
            assert e.message == "Controller not supported."
        except Exception as e:
            pytest.fail(f"Wrong exception: {e}")


def test_get_file_list():
    m = mock_open_multiple(on_open=filename_to_mock_open)
    with \
            patch("builtins.open", m), \
            patch('Nvidia.NvidiaInstaller.os.path.abspath', return_value=SUPPORTED_GPUS_JSON_PATH):
        ni = NvidiaInstaller("123")
        fl = ni.get_file_list()

    assert type(fl) is dict
    assert type(fl["files"]) is list
    assert type(fl["libraries"]) is list
