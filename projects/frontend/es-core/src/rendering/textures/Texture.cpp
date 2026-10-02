//
// Created by bkg2k on 14/01/2020.
//

#include "Texture.h"
#include "TextureManager.h"

bool Texture::UploadAlpha(int width, int height, const void* data) const
{
  if (mTexture != nullptr) return mTexture->UploadAlpha(width, height, data);
  LOG(LogError) << "Drawing into null texture!";
  assert(false);
  return false;
}

bool Texture::UploadRGBA(int width, int height, const void* data) const
{
  if (mTexture != nullptr) return mTexture->UploadRGBA(width, height, data);
  LOG(LogError) << "Drawing into null texture!";
  assert(false);
  return false;
}

bool Texture::UploadAlphaPart(int x, int y, int width, int height, const void* data) const
{
  if (mTexture != nullptr) return mTexture->UploadAlphaPart(x, y, width, height, data);
  LOG(LogError) << "Drawing into null texture!";
  assert(false);
  return false;
}

bool Texture::UploadRGBAPart(int x, int y, int width, int height, const void* data) const
{
  if (mTexture != nullptr) return mTexture->UploadRGBAPart(x, y, width, height, data);
  LOG(LogError) << "Drawing into null texture!";
  assert(false);
  return false;
}

void Texture::Render(const Vertex vertex[], const ColorByte color[], int vertexCount, bool tiled, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(vertex, color, vertexCount, tiled, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

void Texture::Render(const Vertex vertex[], unsigned int color, int vertexCount, bool tiled, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(vertex, color, vertexCount, tiled, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

Vector2i Texture::Size() const
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*(Texture*)this); ready = IsReady(); }
    // Ready?
    if (ready) return mTexture->Size();
  }
  else
  {
    LOG(LogError) << "Size request of null texture!";
    assert(false);
  }
  return { 0, 0 };
}

int Texture::Width() const
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*(Texture*)this); ready = IsReady(); }
    // Ready?
    if (ready) return mTexture->mWidth;
  }
  else
  {
    LOG(LogError) << "Size request of null texture!";
    assert(false);
  }
  return 0;
}

int Texture::Height() const
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*(Texture*)this); ready = IsReady(); }
    // Ready?
    if (ready) return mTexture->mHeight;
  }
  else
  {
    LOG(LogError) << "Size request of null texture!";
    assert(false);
  }
  return 0;
}

void Texture::SetTargetSize(int width, int height, bool force)
{
  if (mTexture != nullptr) mTexture->SetTargetSize(width, height, force);
  else
  {
    LOG(LogError) << "Set target size of null texture!";
    assert(false);
  }
}

/*void Texture::SetPath(const Path& path)
{
  if (mTexture != nullptr) mTexture->SetPath(path);
  else
  {
    LOG(LogError) << "Set path of null texture!";
    assert(false);
  }
}*/

bool Texture::MakeReady()
{
  if (mTexture != nullptr)
  {
    if (mTexture->Status() == TextureHolder::Status::Loaded) return mTexture->Prepare() == RenderingError::NoError;
    TextureManager::Instance().RequestTextureLoading(*this); // Lazy loading
  }
  return false;
}

void Texture::Render(int x, int y, int w, int h, bool keepratio, bool tiled, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(x, y, w, h, keepratio, tiled, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

void Texture::Render(int x, int y, bool tiled, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(x, y, tiled, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

void Texture::Render(int x, int y, int w, int h, bool tiled, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(x, y, w, h, tiled, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

void Texture::Render(int x, int y, int w, int h, bool keepratio, bool tiled, unsigned char alpha, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(x, y, w, h, keepratio, tiled, alpha, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

void Texture::Render(int x, int y, bool tiled, unsigned char alpha, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(x, y, tiled, alpha, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

void Texture::Render(int x, int y, int w, int h, bool tiled, unsigned char alpha, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(x, y, w, h, tiled, alpha, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

void Texture::Render(int x, int y, int w, int h, bool keepratio, bool tiled, Colors::ColorARGB color, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(x, y, w, h, keepratio, tiled, color, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

void Texture::Render(int x, int y, int w, int h, bool flipX, bool flipY, bool keepratio, bool tiled, Colors::ColorARGB color, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(x, y, w, h, flipX, flipY, keepratio, tiled, color, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

void Texture::Render(int x, int y, bool tiled, Colors::ColorARGB color, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(x, y, tiled, color, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

void Texture::Render(int x, int y, int w, int h, bool tiled, Colors::ColorARGB color, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(x, y, w, h, tiled, color, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

void Texture::Render(int x, int y, int w, int h, bool keepratio, bool tiled, Colors::ColorARGB topleftcolor,
                     Colors::ColorARGB toprightcolor, Colors::ColorARGB bottomrightcolor,
                     Colors::ColorARGB bottomleftcolor, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(x, y, w, h, keepratio, tiled, topleftcolor, toprightcolor, bottomrightcolor, bottomleftcolor, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

void Texture::Render(int x, int y, int w, int h, bool tiled, Colors::ColorARGB topleftcolor, Colors::ColorARGB toprightcolor,
                     Colors::ColorARGB bottomrightcolor, Colors::ColorARGB bottomleftcolor, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(x, y, w, h, tiled, topleftcolor, toprightcolor, bottomrightcolor, bottomleftcolor, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

void Texture::Render(int x, int y, int w, int h, bool keepratio, bool tiled, bool flipOnX, bool flipOnY,
                     Colors::ColorARGB topleftcolor, Colors::ColorARGB toprightcolor,
                     Colors::ColorARGB bottomrightcolor, Colors::ColorARGB bottomleftcolor, float topAlphaReflection,
                     float bottomAlphaReflection, Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(x, y, w, h, keepratio, tiled, flipOnX, flipOnY, topleftcolor, toprightcolor, bottomrightcolor, bottomleftcolor, topAlphaReflection, bottomAlphaReflection, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

void Texture::Render(int x, int y, int w, int h, bool tiled, bool flipOnX, bool flipOnY, Colors::ColorARGB topleftcolor,
                     Colors::ColorARGB toprightcolor, Colors::ColorARGB bottomrightcolor,
                     Colors::ColorARGB bottomleftcolor, float topAlphaReflection, float bottomAlphaReflection,
                     Renderer::TextureType type)
{
  if (mTexture != nullptr)
  {
    bool ready = IsReady();
    // Lazy or immediate loading
    if (!ready) { TextureManager::Instance().RequestTextureLoading(*this); ready = IsReady(); }
    // Ready?
    if (ready) mTexture->Render(x, y, w, h, tiled, flipOnX, flipOnY, topleftcolor, toprightcolor, bottomrightcolor, bottomleftcolor, topAlphaReflection, bottomAlphaReflection, type);
  }
  else
  {
    LOG(LogError) << "Drawing null texture!";
    assert(false);
  }
}

