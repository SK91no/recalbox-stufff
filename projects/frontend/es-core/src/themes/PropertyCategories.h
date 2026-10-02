//
// Created by bkg2k on 11/08/2019.
//
#pragma once

#include "utils/cplusplus/Bitflags.h"

enum class ThemePropertyCategory : unsigned int
{
    Path           = (1 << 0),
    Position       = (1 << 1),
    Size           = (1 << 2),
    Origin         = (1 << 3),
    Color          = (1 << 4),
    Font           = (1 << 5),
    Sound          = (1 << 6),
    Text           = (1 << 7),
    ZIndex         = (1 << 8),
    Rotation       = (1 << 9),
    Display        = (1 << 10),
    Effects        = (1 << 11),

    All            = 0xFFFFFFFF,
};

DEFINE_BITFLAG_ENUM(ThemePropertyCategory, unsigned int)