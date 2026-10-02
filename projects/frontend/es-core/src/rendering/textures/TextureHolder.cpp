//
// Created by thierry.imbert on 10/01/2020.
//

#include <rendering/textures/TextureManager.h>
#include "utils/os/system/Mutex.h"
#include "utils/Log.h"
#include "utils/Files.h"
#include "TextureManager.h"
#include "utils/math/Misc.h"
#include "TextureHolder.h"
#include "nanosvg/nanosvg.h"
#include "nanosvg/nanosvgrast.h"
#include <Renderer.h>
#include <FreeImage.h>

TextureHolder::TextureHolder(const Path& path, int targetWidth, int targetHeight, TextureHolder::Properties properties, IHttpConfiguration& httpConfigure)
  : mImage(*CreateImage(path, httpConfigure))
  , mLastUse(0)
  , mErrorCount(0)
  , mReferenceCount(0)
  , mGLTextureID(0)
  , mWidth(0)
  , mHeight(0)
  , mTargetWidth(targetWidth)
  , mTargetHeight(targetHeight)
  , mProperties(properties)
  , mStatus(Status::Unloaded)
  , mIsSVG(IdentifySVG(path))
{
  TextureManager::Instance().TextureHolderCreated(this);
}

TextureHolder::TextureHolder(const Path& path, TextureHolder::Properties properties, IHttpConfiguration& httpConfigure)
  : TextureHolder(path, 0, 0, properties, httpConfigure)
{
}

TextureHolder::~TextureHolder()
{
  TextureManager::Instance().TextureHolderDestroyed(this);
  Unload();
  delete &mImage;
}

bool TextureHolder::UpdateFromRawData(const unsigned char* data, int width, int height)
{
  // First time init or reset
  if (mWidth != width || mHeight != height)
  {
    // Already exists?
    if ((mWidth | mHeight) != 0)
      Unload();

    // First time
    mWidth = width;
    mHeight = height;
    mGLTextureID = Renderer::CreateGLTexture((mProperties & Properties::NoFiltering) != 0);
  }

  {
    // Update only
    Mutex::AutoLock lock(mMutex);
    Renderer::UploadRGBA(mGLTextureID, width, height, data);
  }
  mStatus = Status::Loaded;
  return true;
}

RenderingError TextureHolder::LoadContent()
{
  if (mImage.FilePath().IsEmpty()) return RenderingError::NoResource;

  Mutex::AutoLock lock(mMutex);
  int width = mImage.Width();
  int height = mImage.Height();

  if (mIsSVG)
  {
    // Get SVG size
    float svgWidth = (float)width;
    float svgHeight = (float)height;

    // We want to rasterize this texture at a specific resolution. If the source size
    // variables are set then use them otherwise set them from the parsed file
    if ((mTargetWidth == 0) && (mTargetHeight == 0))
    {
      mTargetWidth = (int)svgWidth;
      mTargetHeight = (int)svgHeight;
    }
    width = mTargetWidth;
    height = mTargetHeight;

    // Auto-scale width to keep aspect
    if (width == 0)
      width = Math::roundi(((float) height / svgHeight) * svgWidth);
      // Auto-scale height to keep aspect
    else if (height == 0)
      height = Math::roundi(((float) width / svgWidth) * svgHeight);
      // Resize to keep aspect ratio
    else
    {
      float ratio = Math::min((float) width / svgWidth, (float) height / svgHeight);
      width = Math::roundi(ratio * svgWidth);
      height = Math::roundi(ratio * svgHeight);
    }

    // DO NOT write the rasterized size back into mTargetWidth/mTargetHeight.
    //
    // Doing so turned this function into a feedback loop: the ratio-adjusted,
    // rounded-down size became the *request* for the next rasterization. Every
    // time the GL context is destroyed and textures reload (launching a game),
    // a non-square SVG lost another fraction of a pixel, monotonically.
    //
    // RatingComponent showed it best: it draws a 5*h wide area with the star
    // tiled, so the star count is 5*h / textureWidth. As the star shrank the
    // count crept past 5 — 5.26, then 5.55 — and the same happened vertically.
    //
    // Keeping the request intact makes rasterization idempotent: same request
    // in, same size out, however many times the context is recreated. It also
    // preserves the "0 means auto" convention above, which the write-back
    // destroyed by replacing the 0 with a computed size.
  }

  // Set real size
  mWidth = width;
  mHeight = height;

  return mImage.Rasterize(width, height) ?
         RenderingError::NoError :
         RenderingError::NoResource;
}

RenderingError TextureHolder::Load()
{
  if (mStatus == Status::Loaded) return RenderingError::NoError;
  mStatus = Status::Loading;

  // Load RGB data only if not already loaded for non-free textures
  if (mImage.GetBitmapData().Empty() && (mProperties & Properties::Free) == 0)
  {
    // Load
    RenderingError error = LoadContent();
    if (error != RenderingError::NoError)
    {
      mStatus = Status::Error;
      return error;
    }
  }
  mStatus = Status::Loaded;
  Touch();

  // Upload to GPU
  if (mGLTextureID == 0 && Renderer::IsContextThread())
    return Prepare();

  return RenderingError::NoError;
}

void TextureHolder::Unload()
{
  Mutex::AutoLock lock(mMutex);

  // Free GPU ram
  if (mGLTextureID != 0)
  {
    LOG(LogDebug) << "[TextureHolder] Unload GPU texture of " << (FilePath().IsEmpty() ? "<Free texture>" : FilePath().ToString());
    Renderer::DestroyGLTexture(mGLTextureID);
    mGLTextureID = 0;
  }

  // Set status
  mStatus = Status::Unloaded;
}

void TextureHolder::UnloadCaches()
{
  LOG(LogDebug) << "[TextureHolder] Unload texture caches of " << (FilePath().IsEmpty() ? "<Free texture>" : FilePath().ToString());
  Mutex::AutoLock lock(mMutex);

  mImage.ClearCaches();
}

long long TextureHolder::TryEvict(bool alsoCpu)
{
  // Non-blocking: a busy texture (mid load/download/render holds its lock) is skipped, never waited
  // on. This guarantees the eviction path cannot block while holding the manager-level storage lock.
  if (!mMutex.TryLock()) return -1;
  long long freed = GPUConsummed();             // read before freeing
  if (alsoCpu) freed += CPUConsummed();
  Unload();                                      // free GPU (recursive lock, ok)
  if (alsoCpu) UnloadCaches();                   // also drop CPU cache (recursive lock, ok)
  mMutex.UnLock();
  return freed;
}

RenderingError TextureHolder::Prepare()
{
  // Already prepared
  if (mGLTextureID != 0)
    return RenderingError::NoError;

  // Free texture - Just create texture area
  if ((mProperties & Properties::Free) != 0)
  {
    RenderingError error;
    { // Lock
      Mutex::AutoLock lock(mMutex);
      mGLTextureID = Renderer::CreateGLTexture((mProperties & Properties::NoFiltering) != 0);

      // Create area
      error = ((mProperties & Properties::Alpha) != 0) ?
               Renderer::UploadAlpha(mGLTextureID, mTargetWidth, mTargetHeight, nullptr) :
               Renderer::UploadRGBA(mGLTextureID, mTargetWidth, mTargetHeight, nullptr);

      if (error == RenderingError::NoError)
      {
        mWidth = mTargetWidth;
        mHeight = mTargetHeight;
        mStatus = Status::Loaded;
      }
      else if (error == RenderingError::OutOfGPUMemory)
      {
        LOG(LogWarning) << "[TextureHolder] Cannot bind texture #" << (FilePath().IsEmpty() ? "Unknown" : FilePath().ToChars()) << ": Out of GPU memory";
        Renderer::DestroyGLTexture(mGLTextureID);
        mGLTextureID = 0;
      }
    }

    // Free oldest textures OUTSIDE the mMutex scope: UnloadTheOldest takes the manager-level
    // mMutexStorage, and the lock hierarchy is mMutexStorage -> tex.mMutex. Calling it while holding
    // tex.mMutex would invert that order and deadlock against FreeAllCPUResources / eviction.
    if (error == RenderingError::OutOfGPUMemory)
      (void)TextureManager::Instance().UnloadTheOldest(TextureUnload::GpuOnly);

    return error;
  }

  // Normal texture: Upload RGB data
  RenderingError error;
  { // Lock
    Mutex::AutoLock lock(mMutex);

    // Check bitmap availability BEFORE creating the GL texture.
    // If we create the texture first and then fail, mGLTextureID would be non-zero but point to an
    // empty texture, causing all subsequent Render() calls to use that empty texture (white display).
    if (mImage.GetBitmapData().Count() < (mWidth * mHeight * 4))
    {
      // Reset status so the texture can be re-queued and reloaded from disk
      mStatus = Status::Unloaded;
      return RenderingError::NoResource;
    }

    mGLTextureID = Renderer::CreateGLTexture((mProperties & Properties::NoFiltering) != 0);

    error = Renderer::UploadRGBA(mGLTextureID, mWidth, mHeight, mImage.GetBitmapData().BufferReadOnly());
    if (error == RenderingError::OutOfGPUMemory)
    {
      LOG(LogWarning) << "[TextureHolder] Cannot bind texture #" << (FilePath().IsEmpty() ? "Unknown" : FilePath().ToChars()) << ": Out of GPU memory";
      Renderer::DestroyGLTexture(mGLTextureID);
      mGLTextureID = 0;
    }

    Touch();
  }

  // Free oldest textures OUTSIDE the mMutex scope (see above: avoids mMutexStorage/tex.mMutex deadlock)
  if (error == RenderingError::OutOfGPUMemory)
    (void)TextureManager::Instance().UnloadTheOldest(TextureUnload::GpuOnly);

  return error;
}

bool TextureHolder::UploadAlpha(int width, int height, const void* data)
{
  // Touch
  Touch();

  // Bind
  if (Prepare() == RenderingError::NoError)
    if (Renderer::UploadAlpha(mGLTextureID, width, height, data) == RenderingError::NoError)
    {
      mWidth = mTargetWidth = width;
      mHeight = mTargetHeight = height;
      mStatus = Status::Loaded;
      return true;
    }
  return false;
}

bool TextureHolder::UploadRGBA(int width, int height, const void* data)
{
  // Touch
  Touch();

  // Bind
  if (Prepare() == RenderingError::NoError)
    if (Renderer::UploadRGBA(mGLTextureID, width, height, data) == RenderingError::NoError)
    {
      mWidth = mTargetWidth = width;
      mHeight = mTargetHeight = height;
      mStatus = Status::Loaded;
      return true;
    }
  return false;
}

bool TextureHolder::UploadAlphaPart(int x, int y, int width, int height, const void* data)
{
  // Touch
  Touch();

  // Bind
  if (Prepare() == RenderingError::NoError)
    return Renderer::UploadAlphaPart(mGLTextureID, x, y, width, height, data) == RenderingError::NoError;
  return false;
}

bool TextureHolder::UploadRGBAPart(int x, int y, int width, int height, const void* data)
{
  // Touch
  Touch();

  // Bind
  if (Prepare() == RenderingError::NoError)
    return Renderer::UploadRGBAPart(mGLTextureID, x, y, width, height, data) == RenderingError::NoError;
  return false;
}

void TextureHolder::Render(const Vertex vertex[], const ColorByte color[], int vertexCount, bool tiled, Renderer::TextureType type)
{
  // Touch
  Touch();

  // Bind
  if (Prepare() == RenderingError::NoError)
    Renderer::DrawTexturedTriangles(mGLTextureID, vertex, color, vertexCount, tiled, type);
}

void TextureHolder::Render(const Vertex vertex[], unsigned int color, int vertexCount, bool tiled, Renderer::TextureType type)
{
  // Touch
  Touch();

  // Bind
  if (Prepare() == RenderingError::NoError)
    Renderer::DrawTexturedTriangles(mGLTextureID, vertex, color, vertexCount, tiled, type);
}

Vector2i TextureHolder::Size()
{
  // Touch
  Touch();

  return { mWidth, mHeight };
}

void TextureHolder::SetTargetSize(int width, int height, bool force)
{
  // Ignore same size
  if ((mTargetWidth == width) && (mTargetHeight == height)) return;

  if (mIsSVG)
  {
    // SVG may be rasterized again only in a higher size
    if ((width < mTargetWidth || height < mTargetHeight) && !force)
    {
      LOG(LogWarning) << "[TextureHolder] Texture " << (FilePath().IsEmpty() ? "Unknown" : FilePath().ToChars()) << " request a resize from (" << mTargetWidth << ", " << mTargetHeight << ") to (" << width << ", " << height << "). Request ignored";
      return;
    }

    if (mStatus == Status::Loaded)
    {
      Unload();
      FreeCPUCaches();
    }
  }

  mTargetWidth = width;
  mTargetHeight = height;
}

/*void TextureHolder::SetPath(const Path& path)
{
  // Ignore same path
  if (path == mPath) return;

  if ((mProperties & Properties::Mutable) != 0)
  {
    LOG(LogError) << "Trying to change path of a non-mutable texture!";
    return;
  }

  if (mStatus == Status::Loaded)
  {
    Unload();
    FreeCPUCaches();
  }

  mPath = path;
  mIsSVG = Strings::ToLowerASCII(path.Extension()) == ".svg";
}*/

ByteArray TextureHolder::DecodeImage(const ByteArray& data, int& width, int& height)
{
  ByteArray rawData;
  width = 0;
  height = 0;

  FIMEMORY * fiMemory = FreeImage_OpenMemory((BYTE *)data.ByteBufferReadOnly(), (DWORD)data.Count());
  if (fiMemory == nullptr)
  {
    LOG(LogError) << "[TextureHolder] Failed to load image from memory!";
    return rawData;
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
      width = (int)FreeImage_GetWidth(fiBitmap);
      height = (int)FreeImage_GetHeight(fiBitmap);
      // loop through scanlines and add all pixel data to the return vector
      // this is necessary, because width*height*bpp might not be == pitch
      // do on-the-fly argb to abgr convertion
      rawData.ExpandTo(width * height * 4);
      unsigned char* tempData = rawData.BufferReadWrite();
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
    else
      LOG(LogError) << "[TextureHolder] Error - Failed to load image from memory!";
  }
  else
    LOG(LogError) << "Error - File type " << (format == FIF_UNKNOWN ? "unknown" : "unsupported") << "!";
  // free FIMEMORY again
  FreeImage_CloseMemory(fiMemory);

  return rawData;
}

void TextureHolder::Render(int x, int y, int w, int h, bool keepratio, bool tiled, Renderer::TextureType type)
{
  // Touch
  Touch();

  // Bind
  if (Prepare() == RenderingError::NoError)
    Renderer::DrawTexture(mGLTextureID, mWidth, mHeight, x, y, w, h, keepratio, tiled, type);
}

void TextureHolder::Render(int x, int y, int w, int h, bool keepratio, bool tiled, unsigned char alpha, Renderer::TextureType type)
{
  // Touch
  Touch();

  // Bind
  if (Prepare() == RenderingError::NoError)
    Renderer::DrawTexture(mGLTextureID, mWidth, mHeight, x, y, w, h, keepratio, tiled, alpha, type);
}

void TextureHolder::Render(int x, int y, int w, int h, bool keepratio, bool tiled, Colors::ColorARGB color, Renderer::TextureType type)
{
  // Touch
  Touch();

  // Bind
  if (Prepare() == RenderingError::NoError)
    Renderer::DrawTexture(mGLTextureID, mWidth, mHeight, x, y, w, h, keepratio, tiled, color, type);
}

void TextureHolder::Render(int x, int y, int w, int h, bool flipX, bool flipY, bool keepratio, bool tiled,
                           Colors::ColorARGB color, Renderer::TextureType type)
{
  // Touch
  Touch();

  // Bind
  if (Prepare() == RenderingError::NoError)
    Renderer::DrawTexture(mGLTextureID, mWidth, mHeight, x, y, w, h, flipX, flipY, keepratio, tiled, color, type);
}

void TextureHolder::Render(int x, int y, int w, int h, bool keepratio, bool tiled, Colors::ColorARGB topleftcolor,
                           Colors::ColorARGB toprightcolor, Colors::ColorARGB bottomrightcolor,
                           Colors::ColorARGB bottomleftcolor, Renderer::TextureType type)
{
  // Touch
  Touch();

  // Bind
  if (Prepare() == RenderingError::NoError)
    Renderer::DrawTexture(mGLTextureID, mWidth, mHeight, x, y, w, h, keepratio, tiled, topleftcolor, toprightcolor, bottomrightcolor, bottomleftcolor, type);
}

void TextureHolder::Render(int x, int y, int w, int h, bool keepratio, bool tiled, bool flipOnX, bool flipOnY,
                           Colors::ColorARGB topleftcolor, Colors::ColorARGB toprightcolor,
                           Colors::ColorARGB bottomrightcolor, Colors::ColorARGB bottomleftcolor,
                           float topAlphaReflection, float bottomAlphaReflection, Renderer::TextureType type)
{
  // Touch
  Touch();

  // Bind
  if (Prepare() == RenderingError::NoError)
    Renderer::DrawTexture(mGLTextureID, mWidth, mHeight, x, y, w, h, keepratio, tiled, flipOnX, flipOnY, topleftcolor, toprightcolor, bottomrightcolor, bottomleftcolor, topAlphaReflection, bottomAlphaReflection, type);
}

