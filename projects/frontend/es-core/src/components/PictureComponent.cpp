//
// Created by bkg2k on 13/10/23.
//

#include <components/PictureComponent.h>
#include <Renderer.h>
#include <help/Help.h>
#include <themes/ThemeData.h>
#include <rendering/textures/TextureManager.h>

PictureComponent::PictureComponent(WindowManager&window, const Path& imagePath, ImgProps props, int marginH, int marginV)
  : ThemableComponent(window)
  , mMarginH(marginH)
  , mMarginV(marginV)
  , mColorShift(0xFFFFFFFF)
  , mProps(props)
  , mFadeOpacity(0.0f)
  , mFading(false)
  , mVisible(true)
  , mTiled(false)
  , mPathChanged(false)
{
  if (!imagePath.IsEmpty()) setImage(imagePath);
}

void PictureComponent::setImage(const Path& path, bool tiled)
{
  if ((mPath == path) && (mTiled == tiled)) return;
  mPath = path;
  mTiled = tiled;

  if (path.IsEmpty() || !TextureManager::Exists(path)) mTexture.Clear();
  else mPathChanged = true;
}

void PictureComponent::setColor(unsigned int color)
{
  mColorShift = color;
  // Grab the opacity from the color shift because we may need to apply it if
  // fading textures in
  mOpacity = color & 0xff;
}

void PictureComponent::setOpacity(unsigned char opacity)
{
  mOpacity = opacity;
  mColorShift = (mColorShift >> 8 << 8) | mOpacity;
}

void PictureComponent::Render(const Transform4x4f& parentTrans)
{
  if (mThemeDisabled || !mVisible) return;

  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  if (mPathChanged)
  {
    TextureHolder::Properties textureProperties = TextureHolder::Properties::None;
    if (hasFlag(mProps, ImgProps::ImmediateLoad)) textureProperties |= TextureHolder::Properties::ImmediateLoad;
    if (hasFlag(mProps, ImgProps::NoCache)) textureProperties |= TextureHolder::Properties::NoCache;
    if (hasFlag(mProps, ImgProps::NoFiltering)) textureProperties |= TextureHolder::Properties::NoFiltering;
    mTexture = TextureManager::Instance().Create(mPath, Math::roundi(mSize.x()), Math::roundi(mSize.y()), textureProperties);
    if (hasFlag(mProps, ImgProps::ImmediateLoad)) TextureManager::Instance().RequestTextureLoading(mTexture, true);
    mPathChanged = false;
  }

  int w = Math::roundi(mSize.x());
  int h = Math::roundi(mSize.y());
  if (w > 2 * mMarginH && h > 2 * mMarginV && mOpacity > 0)
    if (fadeIn(mTexture.MakeReady()))
      mTexture.Render(mMarginH, mMarginV, w - (2 * mMarginH), h - (2 * mMarginV), hasFlag(mProps, ImgProps::KeepRatio), false, mColorShift);

  Component::renderChildren(trans);
}

bool PictureComponent::fadeIn(bool textureLoaded)
{
  if (!hasFlag(mProps, ImgProps::ImmediateLoad))
  {
    if (!textureLoaded)
    {
      // Start the fade if this is the first time we've encountered the unloaded texture
      if (!mFading)
      {
        // Start with a zero opacity and flag it as fading
        mFadeOpacity = 0;
        mFading = true;
        // Set the colours to be translucent
        mColorShift = (mColorShift >> 8 << 8) | 0;
      }
    }
    else if (mFading)
    {
      // The texture is loaded and we need to fade it in. The fade is based on the frame rate
      // and is 1/4 second if running at 60 frames per second although the actual value is not
      // that important
      int opacity = mFadeOpacity + (255 / 15);
      // See if we've finished fading
      if (opacity >= 255) { mFadeOpacity = 255; mFading = false; }
      else mFadeOpacity = (unsigned char)opacity;
      // Apply the combination of the target opacity and current fade
      float newOpacity = (float)mOpacity * ((float)mFadeOpacity / 255.0f);
      mColorShift = (mColorShift >> 8 << 8) | (unsigned char)newOpacity;
    }
  }
  return textureLoaded;
}

void PictureComponent::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
  if (hasFlag(properties, ThemePropertyCategory::Size))
  {
    Vector2f scale = getParent() != nullptr ? getParent()->getSize() : Vector2f(
      Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat());
    if (element.HasProperty(ThemePropertyName::MaxSize))
    {
      setKeepRatio(true);
      setSize(element.AsVector(ThemePropertyName::MaxSize) * scale);
    }
  }

  if (hasFlag(properties, ThemePropertyCategory::Path))
    setImage(element.HasProperty(ThemePropertyName::Path) ? element.AsPath(ThemePropertyName::Path) : Path::Empty,
             (element.HasProperty(ThemePropertyName::Tile) && element.AsBool(ThemePropertyName::Tile)));

  if (hasFlag(properties, ThemePropertyCategory::Color))
    mColorShift = element.HasProperty(ThemePropertyName::Color) ? (unsigned int)element.AsInt(ThemePropertyName::Color) : 0xFFFFFFFF;
}