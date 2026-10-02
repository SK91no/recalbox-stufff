import subprocess
from enum import StrEnum


class RGBDual2:
    class Ratio(StrEnum):
        Wide = "169"
        Standard = "43"
        Default = "default"

    class Video(StrEnum):
        Composite = "vec"
        RGB = "dpi"
        Default = "default"

    @staticmethod
    def setRatio(ratio: Ratio):
        print("[RGBDUAL2]: switch to ratio {}".format(ratio.value))
        command = ["bash", "/recalbox/scripts/recalboxrgbdual2-utils.sh", "ratio", ratio.value]
        subprocess.run(command)

    @staticmethod
    def setRatioWide():
        RGBDual2.setRatio(RGBDual2.Ratio.Wide)

    @staticmethod
    def setRatioStandard():
        RGBDual2.setRatio(RGBDual2.Ratio.Standard)

    @staticmethod
    def setRatioDefault():
        RGBDual2.setRatio(RGBDual2.Ratio.Default)

    @staticmethod
    def setVideo(video: Video):
        print("[RGBDUAL2]: switch to video {}".format(video.value))
        command = ["bash", "/recalbox/scripts/recalboxrgbdual2-utils.sh", "video", video.value]
        subprocess.run(command)

    @staticmethod
    def setVideoComposite():
        RGBDual2.setVideo(RGBDual2.Video.Composite)

    @staticmethod
    def setVideoRGB():
        RGBDual2.setVideo(RGBDual2.Video.RGB)

    @staticmethod
    def setVideoDefault():
        RGBDual2.setVideo(RGBDual2.Video.Default)
