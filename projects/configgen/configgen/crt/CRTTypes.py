import typing
from dataclasses import dataclass
from enum import Enum, StrEnum


class CRTResolution(StrEnum):
    # 60Hz
    p1920x240 = "p1920x240"
    p320x240 = "p320x240"
    p1920x224 = "p1920x224"
    p320x224 = "p320x224"
    i640x480 = "i640x480"
    # 50Hz
    p1920x288 = "p1920x288"
    p384x288 = "p384x288"
    i768x576 = "i768x576"
    # 31kHz
    p640x480 = "p640x480"
    p1920x240at120 = "p1920x240at120"
    p1920x480 = "p1920x480"

    @staticmethod
    def fromResolution(width: int, height: int, framerate: float, interlaced: bool) -> typing.Optional["CRTResolution"]:
        if height <= 224:
            if width < 480:
                return CRTResolution.p320x224
            else:
                return CRTResolution.p1920x224
        if height < 288:
            # this is between 224 and 288p, we use the 240p offsets
            if framerate > 100:
                return CRTResolution.p1920x240at120
            elif width <= 480:
                return CRTResolution.p320x240
            else:
                return CRTResolution.p1920x240
        if height == 288:
            if width <= 384:
                return CRTResolution.p384x288
            else:
                return CRTResolution.p1920x288
        if height == 480:
            if interlaced == 1:
                return CRTResolution.i640x480
            elif width > 640:
                return CRTResolution.p1920x480
            else:
                return CRTResolution.p640x480
        if height == 576:
            return CRTResolution.i768x576
        return None


class CRTSuperRez(StrEnum):
    dynamic = "1"
    native = "0"
    x6 = "1920"
    x8 = "2560"

    @staticmethod
    def fromString(value: str) -> typing.Optional["CRTSuperRez"]:
        if value == "1":
            return CRTSuperRez.dynamic
        if value == "0":
            return CRTSuperRez.native
        if value == "1920":
            return CRTSuperRez.x6
        if value == "2560":
            return CRTSuperRez.x8
        return CRTSuperRez.x6

CRTConfigurationByResolution = dict[str, dict[str, int]]

CRTSystemMode = typing.Tuple[str, str, str, str, str, int, int]
CRTArcadeMode = typing.Tuple[str, str, int, int, int]


@dataclass
class CRTArcadeGameV2:
    name: str
    core: str
    width: int
    height: int
    vfreq: float
    vertical: bool


class CRTVideoStandard(StrEnum):
    PAL = "pal"
    NTSC = "ntsc"
    AUTO = "auto"
    ALL = "all"

    @staticmethod
    def fromString(value: str):
        if value == "pal":
            return CRTVideoStandard.PAL
        if value == "ntsc":
            return CRTVideoStandard.NTSC
        return CRTVideoStandard.AUTO

class CRTAdapter(StrEnum):
    RECALBOXRGBDUAL = "recalboxrgbdual"
    RECALBOXRGBJAMMA = "recalboxrgbjamma"
    RECALBOXJVS = "recalboxjvs"
    OTHER = "other"
    NONE = "none"

    @staticmethod
    def fromString(value: str):
        if value == "recalboxrgbdual":
            return CRTAdapter.RECALBOXRGBDUAL
        if value == "recalboxrgbjamma":
            return CRTAdapter.RECALBOXRGBJAMMA
        if value == "recalboxjvs":
            return CRTAdapter.RECALBOXJVS
        if len(value) > 0:
            return CRTAdapter.OTHER
        return CRTAdapter.NONE

class CRTScanlines(StrEnum):
    LIGHT = "light"
    MEDIUM = "medium"
    HEAVY = "heavy"
    NONE = "none"

    @staticmethod
    def fromString(value: str):
        if value == "light":
            return CRTScanlines.LIGHT
        if value == "medium":
            return CRTScanlines.MEDIUM
        if value == "heavy":
            return CRTScanlines.HEAVY
        return CRTScanlines.NONE


class CRTRegion(StrEnum):
    AUTO = "auto"
    EU = "eu"
    US = "us"
    JP = "jp"

    @staticmethod
    def fromString(value: str):
        if value == "eu":
            return CRTRegion.EU
        if value == "us":
            return CRTRegion.US
        if value == "jp":
            return CRTRegion.JP
        return CRTRegion.AUTO


class CRTScreenType(StrEnum):
    k15 = "15kHz"
    k15Ext = "15kHzExt"
    k31 = "31kHz"

    @staticmethod
    def fromString(value: str):
        if value == "31kHz":
            return CRTScreenType.k31
        if value == "15kHzExt":
            return CRTScreenType.k15Ext
        return CRTScreenType.k15


class CRTResolutionType(StrEnum):
    Progressive = "progressive"
    Interlaced = "interlaced"
    DoubleFreq = "doublefreq"

    @classmethod
    def fromString(cls, value: str):
        for k, v in cls.__members__.items():
            if v == value:
                return v
        return CRTResolutionType.Progressive


@dataclass
class CRTSystem:
    core: str
    region: str
    display: str
    reztype: str
    mode_id: str
    viewport_width: int
    viewport_height: int

class CRTHandheldFormat(StrEnum):
    Fullscreen = "progressive"
    Original = "original"

    @classmethod
    def fromString(cls, value: str):
        for k, v in cls.__members__.items():
            if v == value:
                return v
        return CRTHandheldFormat.Fullscreen
