DEFAULT_INI = "/usr/bin/vpinball/assets/Default_VPinballX.ini"

default_player = {
    # VPX defaults to windowed (0); force borderless fullscreen (1). Not 2
    # (exclusive): it needs an exact FSWidth/FSHeight match or falls back anyway.
    "PlayfieldFullScreen": "1",
    "SoundDevice": "-1",
    "BGSet": "2",
    "BallTrail": "",
    "BallTrailStrength": "",
}

default_pinmame = {
    "PinMAMEPath": "./",
}

default_editor = {
    "SelectTableOnStart": "0",
    "SelectTableOnPlayerClose": "0",
    "AutoSaveOn": "0",
}

# Every profile must list the *same* keys: _load() writes values without clearing
# the previous ones, so a key set by one profile would otherwise stick after the
# user switches profile. Empty string = leave VPX/table default.
profile_perf = {
    # 1 = Performance
    "1": {
        "FXAA": "0",
        "Sharpen": "0",
        "DisableAO": "1",
        "DynamicAO": "0",
        "SSRefl": "0",
        "PFReflection": "3",
        "ForceAnisotropicFiltering": "0",
        "AlphaRampAccuracy": "5",
        "AAFactor": "",
        "MSAASamples": "",
        "ForceBloomOff": "",
        "ForceMotionBlurOff": "",
        "MaxTexDimension": "",
    },
    # 2 = Default (table values)
    "2": {
        "FXAA": "",
        "Sharpen": "",
        "DisableAO": "",
        "DynamicAO": "",
        "SSRefl": "",
        "PFReflection": "",
        "ForceAnisotropicFiltering": "",
        "AlphaRampAccuracy": "",
        "AAFactor": "",
        "MSAASamples": "",
        "ForceBloomOff": "",
        "ForceMotionBlurOff": "",
        "MaxTexDimension": "",
    },
    # 3 = Quality
    "3": {
        "FXAA": "3",
        "Sharpen": "2",
        "DisableAO": "0",
        "DynamicAO": "1",
        "SSRefl": "1",
        "PFReflection": "5",
        "ForceAnisotropicFiltering": "1",
        "AlphaRampAccuracy": "10",
        "AAFactor": "",
        "MSAASamples": "",
        "ForceBloomOff": "",
        "ForceMotionBlurOff": "",
        "MaxTexDimension": "",
    },
    # 4 = Raspberry Pi (low-end). Most aggressive: kills every optional GPU pass
    # and downscales big textures to spare the limited VRAM/fillrate.
    "4": {
        "FXAA": "0",                      # no post-process AA
        "Sharpen": "0",                   # no sharpening
        "DisableAO": "1",                 # no ambient occlusion
        "DynamicAO": "0",
        "SSRefl": "0",                    # no screen-space reflections
        "PFReflection": "2",             # static reflections only (no GPU cost)
        "ForceAnisotropicFiltering": "0",
        "AlphaRampAccuracy": "3",        # coarser ball/ramp tessellation
        "AAFactor": "1.0",               # no supersampling
        "MSAASamples": "0",              # MSAA off
        "ForceBloomOff": "1",            # no bloom
        "ForceMotionBlurOff": "1",       # no ball motion blur
        # 4K textures downscaled to 2048: VRAM is plain RAM on the Pi (UMA), so
        # this quarters the resident size of big textures and avoids OOM freezes
        # on heavy tables. The "Corrupted file" spam once blamed on this option
        # came from tables whose datablock lies about the image size, not from
        # the downscale (m_realWidth keeps the original size since 10.8).
        "MaxTexDimension": "2048",
    },
}
