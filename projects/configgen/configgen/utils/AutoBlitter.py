from enum import StrEnum


class AutoBlitter(StrEnum):
    Noone = "none"
    Recalbox = "recalbox"
    Viku = "viku"

    @classmethod
    def fromString(cls, value: str):
        for k, v in cls.__members__.items():
            if v == value:
                return v
        return AutoBlitter.Recalbox
