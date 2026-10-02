//
// Created by bkg2k on 29/03/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <utils/os/fs/Path.h>
#include <utils/storage/Array.h>
#include <utils/os/system/Mutex.h>
#include <rendering/textures/IHttpConfiguration.h>

class IImage
{
  public:
    /*!
     * @brief Default construtor
     * @param imagePath Image file path
     */
    explicit IImage(const Path& imagePath, IHttpConfiguration& httpConfigure)
      : mPath(imagePath)
      , mHttpConfiguration(httpConfigure)
    {
    }

    //! Destructor
    virtual ~IImage() = default;

    //! Get image width - or 0 if it can't be loaded
    virtual int Width() = 0;

    //! Get image height - or 0 if it can't be loaded
    virtual int Height() = 0;

    //! Get cache size, in byte
    [[nodiscard]] virtual int CacheSize() const = 0;

    //! Clear any cache to free mémory
    virtual void ClearCaches() = 0;

    /*!
     * @brief Render the file into an actual array of ARGB pixels
     * @param requestedWidth Requested width for vector formats - Ignored on pixel formats
     * @param requestedHeight Requested height for vector formats - Ignored on pixel formats
     * @return True if the output bitmap is available, false otherwize
     */
    virtual bool Rasterize(int requestedWidth, int requestedHeight) = 0;

    //! Get rasterized bitmap data
    [[nodiscard]] virtual const ByteArray& GetBitmapData() const = 0;

    //! Get path
    [[nodiscard]] const Path& FilePath() const { return mPath; }

  protected:
    //! Image file path
    const Path mPath;

    //! Operation locker
    Mutex mLocker;

    //! Http provider
    IHttpConfiguration& mHttpConfiguration;

    /*!
     * @brief Load the resource pointed by mPath into the given ByteArray
     * @param to Target ByteArray
     * @return True if the resource has been properly loaded, false otherwise
     */
    bool LoadResource(ByteArray& to);

  private:
    /*!
     * @brief Trip extra argument in the path, starting with '?' (illegal character in path)
     * @param path Source path
     * @return Trimmed path
     */
    static Path Trim(const Path& path);
};

