//
// Created by bkg2k on 20/10/22.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

enum CrtResolution: uint
{
  rNone, //!< Null element
  r224p, //!< PAL + NTSC 224p
  r240p, //!< PAL + NTSC 224p
  r288p, //!< PAL only
  r480i, //!< NTSC interlaced hires
  r576i, //!< PAL interlaced hires
  r480p, //!< 31kHz
  r240p120Hz, //!< 31kHz 240
  r320x240p, //!< 320x240
  r384x288p, //!< 384x288
  r1920x480p, //!< 1920x480 (31kHz superrez)
  _rCount, //!< Counter
};

enum Offset: uint
{
  Left,
  Right,
  Up,
  Down,
  Count
};
