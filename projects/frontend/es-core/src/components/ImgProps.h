//
// Created by bkg2k on 23/07/25.
//
#pragma once

#include <utils/cplusplus/Bitflags.h>

enum class ImgProps : int
{
  None          = 0, //!< No props
  KeepRatio     = 1, //!< Keep image ratio
  ImmediateLoad = 2, //!< Load image immediately, int eh calling thread, no delayed load
  NoCache       = 4, //!< Underlying texture is not cached (useful while displaying game screenshots for example)
  NoFiltering   = 8, //!< Underlying texture is not filtered when stretched
};

DEFINE_BITFLAG_ENUM(ImgProps, int)