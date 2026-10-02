//
// Created by bkg2k on 29/03/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "Png.h"
#include <FreeImage.h>
#include <utils/Files.h>

bool Png::sIsRedirectionInitialized = false;

void Png::Load()
{
  Mutex::AutoLock lock(mLocker);

  if (!LoadResource(mContent))
  {
    LOG(LogError) << "[Png] Error loading PNG image " << mPath.ToString();
    return;
  }
}

void Png::ClearCaches()
{
  mContent.Clear();
  mBitmap.Clear();
  mWidth = mHeight = 0;
}

int Png::Height()
{
  if (mHeight == 0) GetSize();
  return mHeight;
}

int Png::Width()
{
  if (mWidth == 0) GetSize();
  return mWidth;
}

void Png::GetSize()
{
  if (mContent.Empty()) Load();
  if (mContent.Empty()) { LOG(LogError) << "[Png] Failed to load image. Path: " << mPath; return; }

  Mutex::AutoLock lock(mLocker);
  FIMEMORY * fiMemory = FreeImage_OpenMemory((BYTE *)mContent.ByteBufferReadOnly(), (DWORD)mContent.Count());
  if (fiMemory == nullptr)
  {
    LOG(LogError) << "[Png] Failed to load image from memory!";
    return;
  }

  //detect the filetype from data
  FREE_IMAGE_FORMAT format = FreeImage_GetFileTypeFromMemory(fiMemory);
  if (format != FIF_UNKNOWN && (FreeImage_FIFSupportsReading(format) != 0))
  {
    //file type is supported. load image
    FIBITMAP* fiBitmap = FreeImage_LoadFromMemory(format, fiMemory);
    if (fiBitmap != nullptr)
    {
      mWidth = (int)FreeImage_GetWidth(fiBitmap);
      mHeight = (int)FreeImage_GetHeight(fiBitmap);
      FreeImage_Unload(fiBitmap);
    }
    else LOG(LogError) << "[Png] Failed to load image from memory!";
  }
  else LOG(LogError) << "[Png] File type " << (format == FIF_UNKNOWN ? "unknown" : "unsupported") << "!";

  // free FIMEMORY again
  FreeImage_CloseMemory(fiMemory);
}

void Png::DoRedirectFreeLibraryOutputMessages()
{
  FreeImage_SetOutputMessage(CustomOutputMessage);
}

void Png::CustomOutputMessage(FREE_IMAGE_FORMAT format, const char* message)
{
  LOG(LogError) << "[Png] FreeImage Error: " << message << " - Format : " << format;
}

bool Png::Rasterize(int /*width*/, int /*height*/)
{
  if (!mBitmap.Empty()) return true;
  if (mContent.Empty()) Load();

  Mutex::AutoLock lock(mLocker);
  FIMEMORY * fiMemory = FreeImage_OpenMemory((BYTE *)mContent.ByteBufferReadOnly(), (DWORD)mContent.Count());
  if (fiMemory == nullptr)
  {
    LOG(LogError) << "[Png] Failed to load image from memory!";
    return false;
  }

  //detect the filetype from data
  FREE_IMAGE_FORMAT format = FreeImage_GetFileTypeFromMemory(fiMemory);
  if (format != FIF_UNKNOWN && (FreeImage_FIFSupportsReading(format) != 0))
  {
    //file type is supported. load image
    FIBITMAP* fiBitmap = FreeImage_LoadFromMemory(format, fiMemory);
    if (fiBitmap != nullptr)
    {
      //loaded. convert to 32bit if necessary
      if (FreeImage_GetBPP(fiBitmap) != 32)
      {
        FIBITMAP* fiConverted = FreeImage_ConvertTo32Bits(fiBitmap);
        if (fiConverted != nullptr)
        {
          //free original bitmap data
          FreeImage_Unload(fiBitmap);
          fiBitmap = fiConverted;
        }
      }
      int width = (int)FreeImage_GetWidth(fiBitmap);
      int height = (int)FreeImage_GetHeight(fiBitmap);
      // loop through scanlines and add all pixel data to the return vector
      // this is necessary, because width*height*bpp might not be == pitch
      // do on-the-fly argb to abgr convertion
      mBitmap.ExpandTo(width * height * 4);
      unsigned char* tempData = mBitmap.BufferReadWrite();
      int w = width;
      for (int y = height; --y >= 0; )
      {
        unsigned int* argb = (unsigned int*)FreeImage_GetScanLine(fiBitmap, y);
        unsigned int* abgr = (unsigned int*)(tempData + (y * width * 4));
        for (int x = w ; --x >= 0;)
        {
          unsigned int c = argb[x];
          abgr[x] = (c & 0xFF00FF00) | ((c & 0xFF) << 16) | ((c >> 16) & 0xFF);
        }
      }
      //free bitmap data
      FreeImage_Unload(fiBitmap);
    }
    else LOG(LogError) << "[Png] Failed to load image from memory!";
  }
  else LOG(LogError) << "[Png] File type " << (format == FIF_UNKNOWN ? "unknown" : "unsupported") << "!";
  // free FIMEMORY again
  FreeImage_CloseMemory(fiMemory);

  return !mBitmap.Empty();
}
