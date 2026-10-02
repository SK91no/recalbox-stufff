//
// Created by bkg2k on 30/03/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include <utils/Files.h>
#include "IImage.h"
#include "../../../../data/Resources.h"
#include "utils/network/HttpClient.h"

bool IImage::LoadResource(ByteArray& to)
{
  const Path& path = Trim(mPath);
  // Internal resource?
  if (path.ToChars()[0] == ':')
  {
    // Embedded
    const Res2hEntry** found = res2hMap.try_get(path.ToString());
    if (found != nullptr)
    {
      const Res2hEntry& embeddedEntry = **found;
      { // Lock
        Mutex::AutoLock lock(mLocker);
        to.ExpandTo((int)embeddedEntry.size + 1);
        memcpy(to.BufferReadWrite(), embeddedEntry.data, embeddedEntry.size);
        to((int)embeddedEntry.size) = 0;
      }
      return true;
    }
  }
  else if (path.StartWidth("http://") || path.StartWidth("https://"))
  {
    HttpClient http;
    String result;
    mHttpConfiguration.ConfigureHttpClient(path.ToString(), http);
    if (http.Execute(path.ToString(), result, true, 3))
    {
      to.AddItems((unsigned char*)result.data(), result.Count());
      return true;
    }
  }
  else
  {
    // Check & load resource
    if (!path.Exists()) return false;
    ByteArray content = Files::LoadFile(path, true);
    if (content.Empty()) return false;

    { // Lock
      Mutex::AutoLock lock(mLocker);
      to = std::move(content);
      return true;
    }
  }
  return false;
}

Path IImage::Trim(const Path& path)
{
  String spath = path.ToString();
  if (int pos = spath.Find('|'); pos > 0)
  {
    spath.Delete(0, pos + 1);
    return Path(spath);
  }
  return path;
}
