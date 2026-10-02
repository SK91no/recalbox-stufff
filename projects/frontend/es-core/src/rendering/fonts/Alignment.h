//
// Created by bkg2k on 13/09/2020.
//
#pragma once

enum class HorizontalAlignment : char
{
  Left   = 0,  //!< Left
  Center = 1, //!< Centered
  Right  = 2, //!< Right
};

enum class VerticalAlignment : char
{
  Top    = 0, //!< Top
  Center = 1, //!< Centered
  Bottom = 2, //!< Bottom
};

enum class Alignment : char
{
  TopLeft      = ((int)VerticalAlignment::Top    << 2) | (int)HorizontalAlignment::Left,   //!< Top Left
  TopCenter    = ((int)VerticalAlignment::Top    << 2) | (int)HorizontalAlignment::Center, //!< Top Centered
  TopRight     = ((int)VerticalAlignment::Top    << 2) | (int)HorizontalAlignment::Right,  //!< Top Right
  CenterLeft   = ((int)VerticalAlignment::Center << 2) | (int)HorizontalAlignment::Left,   //!< Centered Left
  Center       = ((int)VerticalAlignment::Center << 2) | (int)HorizontalAlignment::Center, //!< Centered horizontally & vertically
  CenterRight  = ((int)VerticalAlignment::Center << 2) | (int)HorizontalAlignment::Right,  //!< Centered Right
  BottomLeft   = ((int)VerticalAlignment::Bottom << 2) | (int)HorizontalAlignment::Left,   //!< Bottom Left
  BottomCenter = ((int)VerticalAlignment::Bottom << 2) | (int)HorizontalAlignment::Center, //!< Bottom Centered
  BottomRight  = ((int)VerticalAlignment::Bottom << 2) | (int)HorizontalAlignment::Right,  //!< Bottom Right
};

inline Alignment AlignmentMergeVerticalAndHorizontal(VerticalAlignment v, HorizontalAlignment h)
{
  return ((Alignment)(((int)(v) << 2) | (int)(h)));
}

inline VerticalAlignment AlignmentExtractVertical(Alignment vh)
{
  return ((VerticalAlignment)((int)(vh) >> 2));
}

inline HorizontalAlignment AlignmentExtractHorizontal(Alignment vh)
{
  return ((HorizontalAlignment)((int)(vh) & 3));
}

inline Alignment AlignmentReplaceVertical(Alignment vh, VerticalAlignment v)
{
  return ((Alignment)(((int)(vh) & 0x3) | ((int)(v) << 2)));
}

inline Alignment AlignmentReplaceHorizontal(Alignment vh, HorizontalAlignment h)
{
  return ((Alignment)(((int)(vh) & 0xC) | (int)(h)));
}

inline HorizontalAlignment AlignmentInvertHorizontal(HorizontalAlignment h) noexcept
{
  if (h == HorizontalAlignment::Left)  return HorizontalAlignment::Right;
  if (h == HorizontalAlignment::Right) return HorizontalAlignment::Left;
  return h;
}

inline Alignment AlignmentInvertHorizontal(Alignment a) noexcept
{
  return AlignmentReplaceHorizontal(a, AlignmentInvertHorizontal(AlignmentExtractHorizontal(a)));
}
