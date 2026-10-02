//
// Created by bkg2k on 29/03/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include <ImageIO.h>
#include "Svg.h"
#include "nanosvg/nanosvgrast.h"
#include <utils/math/Misc.h>

void Svg::Load()
{
  Mutex::AutoLock lock(mLocker);

  if (!LoadResource(mContent))
  {
    LOG(LogError) << "Error loading SVG image " << mPath.ToString();
    return;
  }

  static Mutex sGlobalLocker; // Avoid parsing issues due to non thread-safe parsing
  Mutex::AutoLock lock2(sGlobalLocker);
  mImage = nsvgParse((char*)mContent.BufferReadWrite(), "px", 96);
  if (mImage == nullptr)
  {
    LOG(LogError) << "Error parsing SVG image.";
  }
}

bool Svg::Rasterize(int width, int height)
{
  if (!mBitmap.Empty()) return true;
  if (mImage == nullptr) Load();
  if (mImage != nullptr)
  {
    Mutex::AutoLock lock(mLocker);
    mBitmap.ExpandTo(width * height * 4);
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    nsvgRasterize(rast, mImage, 0, 0, (float)height / mImage->height, mBitmap.BufferReadWrite(), width, height, width << 2);
    nsvgDeleteRasterizer(rast);
    ImageIO::flipPixelsVert((unsigned char*)mBitmap.BufferReadWrite(), width, height);
    return true;
  }

  return false;
}

int Svg::Width()
{
  if (mImage == nullptr) Load();
  if (mImage != nullptr) return Math::roundi(mImage->width);
  return 0;
}

int Svg::Height()
{
  if (mImage == nullptr) Load();
  if (mImage != nullptr) return Math::roundi(mImage->height);
  return 0;
}

void Svg::ClearCaches()
{
  Mutex::AutoLock lock(mLocker);
  nsvgDelete(mImage);
  mImage = nullptr;
  mContent.Clear();
  mBitmap.Clear();
}

