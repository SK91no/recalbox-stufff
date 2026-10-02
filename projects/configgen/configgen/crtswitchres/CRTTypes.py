import typing
from dataclasses import dataclass
from enum import Enum, StrEnum

from configgen.utils.architecture import Architecture


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
    RECALBOXRGBDUAL2 = "recalboxrgbdual2"
    RECALBOXRGBJAMMA = "recalboxrgbjamma"
    RECALBOXJVS = "recalboxjvs"
    OTHER = "other"
    NONE = "none"

    @staticmethod
    def fromString(value: str):
        if value == "recalboxrgbdual":
            return CRTAdapter.RECALBOXRGBDUAL
        if value == "recalboxrgbdual2":
            return CRTAdapter.RECALBOXRGBDUAL2
        if value == "recalboxrgbjamma":
            return CRTAdapter.RECALBOXRGBJAMMA
        if value == "recalboxjvs":
            return CRTAdapter.RECALBOXJVS
        if len(value) > 0:
            return CRTAdapter.OTHER
        return CRTAdapter.NONE

    def supportsInterlaced(self) -> bool:
        if self == CRTAdapter.RECALBOXRGBDUAL2:
            import os.path
            return not os.path.isfile("/sys/firmware/devicetree/base/recalboxrgbdual2/avoid_interlaced")
        elif self == CRTAdapter.RECALBOXRGBJAMMA:
            import os.path
            return os.path.isfile("/sys/firmware/devicetree/base/recalboxrgbjamma/recalbox-rgb-jamma-2")
        elif self == CRTAdapter.RECALBOXJVS:
            try:
                with open("/sys/kernel/recalboxrgbdual2/interlaced_supported", "r") as f:
                    return f.read().strip() == "true"
            except OSError:
                return False
        else:
            return not Architecture().isPi5
    def supports50Hz(self) -> bool:
        return not (self == CRTAdapter.RECALBOXRGBJAMMA or self == CRTAdapter.RECALBOXJVS)

    def hasServiceButtons(self):
        return self == CRTAdapter.RECALBOXRGBJAMMA or self == CRTAdapter.RECALBOXJVS

    def supports15kHz(system) -> bool:
        if system.CRTScreenType == "kHz31":
            return False
        else:
            return True


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
    auto = "Auto"
    kHz15 = "kHz15"
    kHzMulti1525 = "kHzMulti1525"
    kHz31 = "kHz31"
    kHzMulti1531 = "kHzMulti1531"
    kHzTriFreq = "kHzTriFreq"

    @classmethod
    def fromString(cls, value: str):
        for k, v in cls.__members__.items():
            if v == value:
                return v
        return CRTScreenType.kHz15

class RetroarchScreenTypeIndex(Enum):
    disabled = 0
    kHz15 = 1
    kHzMulti1525 = 2
    kHz31 = 3
    kHzMulti1531 = 4
    kHzTriFreq = 5
    kHz31at120 = 6
    kHz15Jamma = 7
    kHz15Forced240p = 8
    PAL = 9
    NTSC = 10
    kHzTriFreqTV = 11
    kHzMulti1531TV = 12
    kHzMulti1525TV = 13
    kHz25Forced384p = 14
    kHz15ForcedProgressive = 15
    kHz15ForcedProgressiveMin57Hz = 16
    ini = 17
    kHz31Forced480p = 18
    kHz24Forced384p = 19

class CRTResolutionType(StrEnum):
    Auto = "auto"
    Force240 = "240"
    Force480 = "480"
    Force480i = "480i"
    DoubleFreq = "doublefreq"
    Force384 = "384p"

    @classmethod
    def fromString(cls, value: str):
        for k, v in cls.__members__.items():
            if v == value:
                return v
        return CRTResolutionType.Auto

class CRTSignalType(StrEnum):
    RGB = "RGB"
    Composite = "Composite"
    Auto = "auto"

    @classmethod
    def fromString(cls, value: str):
        for k, v in cls.__members__.items():
            if v == value:
                return v
        return CRTSignalType.RGB
