//
// Created by bkg2k on 29/03/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <utils/os/fs/Path.h>
#include <utils/storage/Array.h>
#include <rendering/images/IImage.h>
#include "nanosvg/nanosvg.h"

class Svg : public IImage
{
  public:
    /*!
     * @brief Default constructor
     * @param svgPath Path to .svg file
     */
    explicit Svg(const Path& svgPath, IHttpConfiguration& httpConfigure)
      : IImage(svgPath, httpConfigure)
      , mImage(nullptr)
    {
    }

    //! Destructor
    ~Svg() override { ClearCaches(); }

    /*
     * IImage implementation
     */

    /*!
     * @brief Render the SVG into an array of ARGB pixels
     * @param width Requested width
     * @param height Requested height
     * @return True if the output bitmap is available, false otherwize
     */
    bool Rasterize(int width, int height) final;

    //! Get rasterized bitmap data
    [[nodiscard]] const ByteArray& GetBitmapData() const final { return mBitmap; };

    //! Get image width
    int Width() final;

    //! Get image height
    int Height() final;

    //! Get cache size, in byte
    [[nodiscard]] int CacheSize() const final { return mContent.Count() + mBitmap.Count(); }

    //! Clear any cache to free memory
    void ClearCaches() final;

  private:
    //! SVG file content
    ByteArray mContent;
    //! SVG Rasterized data
    ByteArray mBitmap;

    //! SVG main handler
    NSVGimage* mImage;

    //! Load content into cache
    void Load();
};



