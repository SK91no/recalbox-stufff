//
// Created by bkg2k on 16/12/23.
//
#pragma once

#include "utils/os/fs/Path.h"
#include "utils/storage/HashMap.h"
#include "utils/os/system/Mutex.h"

class ThemeFileCache
{
  public:
    /*!
     * @brief Get file content from cache. If the file is not cached, it's loaded
     * @param path File path
     * @return File content
     */
    String& File(const Path& path);

    //! Clear caches
    void Clear();

  private:
    //! Main cache
    HashMap<Path, String> mCache;
    //! Cache lock
    Mutex mLocker;
};
