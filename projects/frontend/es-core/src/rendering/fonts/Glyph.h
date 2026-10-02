//
// Created by bkg2k on 13/12/2020.
//
#pragma once

#include "rendering/textures/Texture.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

//! Glyph structure hold graph data of a single Unicode character
struct Glyph
{
  Texture mTexture; //!< Texture reference
  float mX;         //!< X position in texture (texel)
  float mY;         //!< Y position in texture (texel)
  float mW;         //!< Width in texture (texel)
  float mH;         //!< Height in texture (texel)
  float mAdvanceX;  //!< Horizontal move to next character
  float mBearingX;  //!< Horizontal bearing
  float mBearingY;  //!< Vertical bearing
  float mWP;        //!< Width in pixel
  float mHP;        //!< Height in pixel

  /*!
   * @brief Fill the glyph texture
   * @param texture Texture
   * @param tw Texture Width
   * @param th Texture Height
   * @param x Glyph X position in pixel
   * @param y Glyph Y position in pixel
   * @param glyph Glyph structure
   */
  Glyph* Set(Texture& texture, int tw, int th, int x, int y, FT_GlyphSlot glyph, FT_BitmapGlyph glyphBitmap, bool embold)
  {
    assert(texture.IsReady());
    mTexture = texture;
    mX = ((float)x + 0.0f) / (float)tw;
    mY = ((float)y + 0.0f) / (float)th;
    mWP = (float)(glyphBitmap->bitmap.width);
    mW = (mWP - 0.f) / (float)tw;
    mHP = (float)(glyphBitmap->bitmap.rows);
    mH = (mHP - 0.f) / (float)th;
    mAdvanceX = ((float)glyph->metrics.horiAdvance / 64.f) + (embold ? 1.f : 0.f);
    mBearingX = (float)glyph->metrics.horiBearingX / 64.f;
    mBearingY = (float)glyph->metrics.horiBearingY / 64.f;
    return this;
  }

  [[nodiscard]] bool Ready() const { return mTexture.IsReady(); }
};