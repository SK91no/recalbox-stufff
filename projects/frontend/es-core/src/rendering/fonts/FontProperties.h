//
// Created by bkg2k on 19/09/2020.
//
#pragma once

#include <utils/os/fs/Path.h>
#include <Renderer.h>

class FontProperties
{
  public:
    FontProperties()
      : mFontPath()
      , mHeight(0.05)
    {
    }

    explicit FontProperties(float height)
      : mFontPath()
      , mHeight(height)
    {
    }

    explicit FontProperties(const Path& fontPath, float height)
      : mFontPath(fontPath)
      , mHeight(height)
    {
    }

    /*
     * Getters
     */

    // Gert file pathb
    [[nodiscard]] const Path& GetPath() const { return mFontPath; }
    //! Get font height in percent of screen hieght
    [[nodiscard]] float HeightFactor() const { return mHeight; }
    //! Get font height in pixels
    [[nodiscard]] float Height() const { return mHeight * Renderer::Instance().DisplayHeightAsFloat(); }

  private:
    Path mFontPath; //!< Font Path (empty = default fond)
    float mHeight;  //!< Size in screen height ratio
};
