//
// Created by bkg2k on 13/10/23.
//

#include <components/StackablePictureComponent.h>
#include <utils/Log.h>
#include <Renderer.h>
#include <help/Help.h>
#include <themes/ThemeData.h>
#include <utils/locale/LocaleHelper.h>
#include <rendering/textures/TextureManager.h>

StackablePictureComponent::StackablePictureComponent(WindowManager&window, const Path::PathList& imagePath, ImgProps props)
  : ThemableComponent(window)
  , mProps(props)
  , mVisible(true)
{
  int index = 0;
  for(const Path& path : imagePath)
    setImage(index++, path, false);
}

void StackablePictureComponent::setImage(int index, const Path& path, bool tiled)
{
  // Fill in empty indexes with no image
  while((int)mImages.size() < index) mImages.push_back(ImageSpec(*this));
  // Add or set Image
  if (index == (int)mImages.size()) mImages.push_back(ImageSpec(*this, path, mProps, tiled));
  else mImages[index].Set(path, mProps, tiled);
}

void StackablePictureComponent::setColor(unsigned int color)
{
  for(ImageSpec& img : mImages)
    img.mColorShift = color;
}

void StackablePictureComponent::setColor(int index, unsigned int color)
{
  if ((unsigned int)index < mImages.size())
  {
    ImageSpec& img = mImages[index];
    img.mColorShift = color;
  }
}

void StackablePictureComponent::setOpacity(unsigned char opacity)
{
  mOpacity = opacity;
}

void StackablePictureComponent::Render(const Transform4x4f& parentTrans)
{
  if (mThemeDisabled || !mVisible) return;

  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  if (mOpacity > 0)
    for(ImageSpec& img : mImages)
      if (img.mVisible)
        if (fadeIn(img))
          img.mTexture.Render(0, 0, Math::roundi(mSize.x()), Math::roundi(mSize.y()), hasFlag(mProps, ImgProps::KeepRatio), false, img.GetColor(mOpacity));

  Component::renderChildren(trans);
}

bool StackablePictureComponent::fadeIn(ImageSpec& image)
{
  bool ready = image.mTexture.MakeReady();
  if (!hasFlag(mProps, ImgProps::ImmediateLoad))
  {
    if (!ready)
    {
      // Start the fade if this is the first time we've encountered the unloaded texture
      if (!image.mFading)
      {
        // Start with a zero opacity and flag it as fading
        image.mFadeOpacity = 0;
        image.mFading = true;
      }
    }
    else
    {
      // Cached textures arrive ready on the first call: arm the fade so opacity ramps
      // in instead of staying at 0 (which renders the image fully transparent).
      if (!image.mFading && image.mFadeOpacity == 0) image.mFading = true;
      if (image.mFading)
      {
        // The texture is loaded and we need to fade it in. The fade is based on the frame rate
        // and is 1/4 second if running at 60 frames per second although the actual value is not
        // that important
        int opacity = image.mFadeOpacity + (255 / 15);
        // See if we've finished fading
        if (opacity >= 255) { image.mFadeOpacity = 255; image.mFading = false; }
        else image.mFadeOpacity = (unsigned char)opacity;
      }
    }
  }
  return ready;
}

void StackablePictureComponent::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
  if (hasFlag(properties, ThemePropertyCategory::Size))
  {
    Vector2f scale = getParent() != nullptr ? getParent()->getSize() : Vector2f(
      Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat());
    if (element.HasProperty(ThemePropertyName::MaxSize))
    {
      mProps |= ImgProps::KeepRatio;
      setSize(element.AsVector(ThemePropertyName::MaxSize) * scale);
    }
  }

  //if (hasFlag(properties, ThemePropertyCategory::Path))
  //  setImage(element.HasProperty(ThemePropertyName::Path) ? element.AsPath(ThemePropertyName::Path) : Path::Empty,
  //           (element.HasProperty(ThemePropertyName::Tile) && element.AsBool(ThemePropertyName::Tile)));

  if (hasFlag(properties, ThemePropertyCategory::Color))
    setColor(element.HasProperty(ThemePropertyName::Color) ? (unsigned int)element.AsInt(ThemePropertyName::Color) : 0xFFFFFFFF);
}

void StackablePictureComponent::ImageSpec::Load()
{
  if (mPath.IsEmpty() || !TextureManager::Exists(mPath)) mTexture.Clear();
  else
  {
    TextureHolder::Properties textureProperties = TextureHolder::Properties::None;
    if (hasFlag(mProps, ImgProps::ImmediateLoad)) textureProperties |= TextureHolder::Properties::ImmediateLoad;
    if (hasFlag(mProps, ImgProps::NoCache)) textureProperties |= TextureHolder::Properties::NoCache;
    if (hasFlag(mProps, ImgProps::NoFiltering)) textureProperties |= TextureHolder::Properties::NoFiltering;
    mTexture = TextureManager::Instance().Create(mPath, Math::roundi(mParent.getWidth()), Math::roundi(mParent.getHeight()), textureProperties);
    if (hasFlag(mProps, ImgProps::ImmediateLoad)) TextureManager::Instance().RequestTextureLoading(mTexture, true);
  }
}
