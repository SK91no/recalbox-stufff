//
// Created by bkg2k on 10/12/2020.
//
#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

class IFontFaceProvider
{
  public:
    /*!
     * @brief Get default font face
     * @return Default font face - Never fails, always return a valid face
     */
    virtual FT_Face GetDefaultFontFace(float heightInPixel) = 0;

    /*!
     * @brief Get specific font face
     * @param path Font file path
     * @return Font face - May be nullptr
     */
    virtual FT_Face GetFontFace(const Path& path, float heightInPixel) = 0;
};
