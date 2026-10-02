//
// Created by bkg2k on 30/03/25.
//
#pragma once

#include "utils/os/fs/Path.h"
#include "rendering/opengl/Colors.h"

class QuickJumpTheme
{
  public:
  private:
    Path mFrame;
    Colors::ColorRGBA mTextColor;     //!< Unselected text color
    Colors::ColorRGBA mSelectedColor; //!< Selected text color
    Colors::ColorRGBA mSelectorColor; //!< Color of selection rectangle

};
