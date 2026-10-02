//
// Created by bkg2k on 29/03/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <FreeImage.h>
#include <utils/os/fs/Path.h>
#include <utils/storage/Array.h>
#include <rendering/images/IImage.h>

class Png : public IImage
{
  public:
    /*!
     * @brief Default constructor
     * @param svgPath Path to .png file
     */
    explicit Png(const Path& svgPath, IHttpConfiguration& httpConfigure)
      : IImage(svgPath, httpConfigure)
      , mWidth(0)
      , mHeight(0)
    {
      if (!sIsRedirectionInitialized)
      {
        DoRedirectFreeLibraryOutputMessages();
        sIsRedirectionInitialized = true;
      }
    }

    //! Destructor
    ~Png() override { ClearCaches(); }

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
    //! Png file content
    ByteArray mContent;
    //! Png Bitmap
    ByteArray mBitmap;

    //! Cached width
    int mWidth;
    //! Cached height
    int mHeight;

    //! Load content into cache
    void Load();

    //! Get image size
    void GetSize();

    //! Static FreeLibrary message redirection is initialized ?
    static bool sIsRedirectionInitialized;

    //! Static redirection
    static void DoRedirectFreeLibraryOutputMessages();

    //! Static message receiver
    static void CustomOutputMessage(FREE_IMAGE_FORMAT format, const char* message);
};



