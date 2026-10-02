import re

from configgen.crt.CRTTypes import CRTResolution


class Mode:
    def __init__(self, timings: str = "", emulator_refresh: str = "60"):
        self.width: int = 0
        self.h_front_porch: int = 0
        self.h_sync: int = 0
        self.h_back_porch: int = 0
        self.height: int = 0
        self.v_front_porch: int = 0
        self.v_sync: int = 0
        self.v_back_porch: int = 0
        self.v_back_porch: int = 0
        self.framerate: float = 0
        self.interlaced: int = 0
        self.clock: int = 0
        self.ratio: int = 0
        self.emulator_refresh: str = emulator_refresh
        if timings != "":
            self.parse(timings)

    def parse(self, timings):
        mode_re = re.compile(r"(\d+) 1 (\d+) (\d+) (\d+) (\d+) 1 (\d+) (\d+) (\d+) 0 0 0 (\d+) (\d) (\d+) (\d)")
        match = mode_re.match(timings)
        if match:
            self.width = int(match.group(1))
            self.h_front_porch = int(match.group(2))
            self.h_sync = int(match.group(3))
            self.h_back_porch = int(match.group(4))
            self.height = int(match.group(5))
            self.v_front_porch = int(match.group(6))
            self.v_sync = int(match.group(7))
            self.v_back_porch = int(match.group(8))
            self.framerate = int(match.group(9))
            self.interlaced = int(match.group(10))
            self.clock = int(match.group(11))
            self.ratio = int(match.group(12))
        else:
            raise Exception("Invalid mode")

    def timings(self) -> str:
        return "{} 1 {} {} {} {} 1 {} {} {} 0 0 0 {} {} {} {}".format(self.width, self.h_front_porch,
                                                                      self.h_sync, self.h_back_porch,
                                                                      self.height, self.v_front_porch,
                                                                      self.v_sync, self.v_back_porch,
                                                                      int(self.framerate), self.interlaced,
                                                                      self.clock, self.ratio)

    def __eq__(self, o: object) -> bool:
        return isinstance(o, Mode) and o.timings() == self.timings() and o.emulator_refresh == self.emulator_refresh

    def __str__(self):
        return self.timings()

    def __repr__(self):
        return self.timings()

    def extractCRTResolution(self) -> [int, int]:
        return CRTResolution.fromResolution(self.width, self.height, self.framerate, self.interlaced == 1)
