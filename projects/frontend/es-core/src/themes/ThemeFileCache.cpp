//
// Created by bkg2k on 16/12/23.
//

#include "ThemeFileCache.h"
#include <utils/Files.h>

String& ThemeFileCache::File(const Path& path)
{
  Mutex::AutoLock locker(mLocker);

  // Already cached ?
  String* content = mCache.try_get(path);
  if (content != nullptr) return *content;

  // Load
  mCache[path] = Files::LoadFile(path);
  return *mCache.try_get(path);
}

void ThemeFileCache::Clear()
{
  Mutex::AutoLock locker(mLocker);
  mCache.clear();
}

