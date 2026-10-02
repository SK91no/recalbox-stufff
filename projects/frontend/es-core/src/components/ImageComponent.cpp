#include <components/ImageComponent.h>
#include <utils/Log.h>
#include <Renderer.h>
#include <help/Help.h>
#include <themes/ThemeData.h>
#include <utils/locale/LocaleHelper.h>
#include <rendering/textures/TextureManager.h>

ImageComponent::ImageComponent(WindowManager& window, const Path& imagePath, ImgProps props)
  : ThemableComponent(window)
  , mTargetSize(0, 0)
  , mPath(imagePath)
  , mFlipX(false)
  , mFlipY(false)
  , mTiled(false)
  , mTopLeftColor(0xFFFFFFFF)
  , mTopRightColor(0xFFFFFFFF)
  , mBottomLeftColor(0xFFFFFFFF)
  , mBottomRightColor(0xFFFFFFFF)
  , mOriginColor(0xFFFFFFFF)
  , mProps(props)
  , mFadeOpacity(0.0f)
  , mTopAlpha(0.0f)
  , mBottomAlpha(0.0f)
  , mColorNotSet(true)
  , mFading(false)
  , mVisible(true)
  , mResizeRequired(!imagePath.IsEmpty())
  , mNewPathRequired(!imagePath.IsEmpty())
{
  if (!imagePath.IsEmpty()) setImage(imagePath, false);
}

void ImageComponent::resize()
{
  if (!mTexture.IsReady()) return;

  const Vector2f textureSize = mTexture.Size().toFloat();
  if (textureSize.isZero()) return;

  if (mTiled)
  {
    mSize = mTargetSize;
  }
  else
  {
    if (hasFlag(mProps, ImgProps::KeepRatio))
    {
      mSize = textureSize;

      // Adjust auto-size values
      if (mTargetSize.x() == 0) { mTargetSize.x() = (mTargetSize.y() * textureSize.x()) / textureSize.y(); }
      if (mTargetSize.y() == 0) { mTargetSize.y() = (mTargetSize.x() * textureSize.y()) / textureSize.x(); }

      Vector2f resizeScale((mTargetSize.x() / mSize.x()), (mTargetSize.y() / mSize.y()));

      if (resizeScale.x() < resizeScale.y())
      {
        mSize[0] *= resizeScale.x();
        mSize[1] *= resizeScale.x();
      }
      else
      {
        mSize[0] *= resizeScale.y();
        mSize[1] *= resizeScale.y();
      }

      // for SVG rasterization, always calculate width from rounded height (see comment above)
      mSize[1] = Math::round(mSize[1]);
      mSize[0] = (mSize[1] / textureSize.y()) * textureSize.x();
    }
    else
    {
      // if both components are set, we just stretch
      // if no components are set, we don't resize at all
      mSize = mTargetSize.isZero() ? textureSize : mTargetSize;

      // if only one component is set, we resize in a way that maintains aspect ratio
      // for SVG rasterization, we always calculate width from rounded height (see comment above)
      if (mTargetSize.x() == 0 && mTargetSize.y() != 0)
      {
        mSize[1] = Math::round(mTargetSize.y());
        mSize[0] = (mSize.y() / textureSize.y()) * textureSize.x();
      }
      else if (mTargetSize.x() != 0 && mTargetSize.y() == 0)
      {
        mSize[1] = Math::round((mTargetSize.x() / textureSize.x()) * textureSize.y());
        mSize[0] = (mSize.y() / textureSize.y()) * textureSize.x();
      }
    }
  }

  // mSize.y() should already be rounded
  mTexture.SetTargetSize(Math::roundi(mSize.x()), Math::roundi(mSize.y()));

  onSizeChanged();
}

void ImageComponent::OnPathChanged()
{
}

void ImageComponent::setImage(const Path& path, bool tile, bool noloaddelai)
{
  if (mPath != path || (mTexture.Valid() && (mTiled != tile)))
  {
    mPath = path;
    mTiled = tile;
    mNewPathRequired = true;
    if (noloaddelai) StartLoading();
    mResizeRequired = true;
    if (noloaddelai) resize();
  }
}

void ImageComponent::setResize(float width, float height)
{
  if (width != mTargetSize.x() || height != mTargetSize.y())
  {
    mTargetSize.Set(width, height);
    mResizeRequired = true;
  }
}

void ImageComponent::setNormalisedSize(float width, float height)
{
  Vector2f pos = denormalise(width, height);
  setSize(pos.x(), pos.y());
}

void ImageComponent::setFlipX(bool flip)
{
  if (flip != mFlipX)
  {
    mFlipX = flip;
  }
}

void ImageComponent::setFlipY(bool flip)
{
  if (flip != mFlipY)
  {
    mFlipY = flip;
  }
}

void ImageComponent::setColorShift(unsigned int color)
{
  if (color != mTopLeftColor)
  {
    mTopLeftColor = color;
    mTopRightColor = color;
    mBottomLeftColor = color;
    mBottomRightColor = color;
    // Grab the opacity from the color shift because we may need to apply it if
    // fading textures in
    mOpacity = color & 0xff;

    if (mColorNotSet)
    {
      setOriginColor(mTopLeftColor);
      mColorNotSet = false;
    }
  }
}

void ImageComponent::setColor(unsigned int color)
{
  setColorShift(color);
}

void ImageComponent::Render(const Transform4x4f& parentTrans)
{
  if (mThemeDisabled || !mVisible) return;

  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  // Lazy path initialisation
  if (mNewPathRequired)
  {
    StartLoading();
  }
  // Lazy resising
  if (mResizeRequired && mTexture.IsReady())
  {
    mOriginalSize = mSize;
    resize();
    mPosition.round();
    mResizeRequired = false;
  }

  if (mTexture.Valid() && mOpacity > 0)
  {
    // actually draw the image
    // The bind() function returns false if the texture is not currently loaded. A blank
    // texture is bound in this case but we want to handle a fade so it doesn't just 'jump' in
    // when it finally loads
    fadeIn(mTexture.MakeReady());
    if (mTexture.IsReady())
    {
      // Combine alphas
      float alpha = (float)mOpacity / 255.f;
      if (mFading) alpha *= (float)mFadeOpacity / 255.f;
      // Recompose colors
      Colors::ColorRGBA topLeftColor = (mTopLeftColor & 0xFFFFFF00) | (int)((float)(mTopLeftColor & 0xFF) * alpha);
      Colors::ColorRGBA topRightColor = (mTopRightColor & 0xFFFFFF00) | (int)((float)(mTopRightColor & 0xFF) * alpha);
      Colors::ColorRGBA bottomLeftColor = (mBottomLeftColor & 0xFFFFFF00) | (int)((float)(mBottomLeftColor & 0xFF) * alpha);
      Colors::ColorRGBA bottomRightColor = (mBottomRightColor & 0xFFFFFF00) | (int)((float)(mBottomRightColor & 0xFF) * alpha);

      int deltaX = 0; //mOriginalSize.x() != 0 ? (int)(mOriginalSize.x() - mSize.x()) : 0;
      int deltaY = 0; //mOriginalSize.y() != 0 ? (int)(mOriginalSize.y() - mSize.y()) : 0;

      mTexture.Render(deltaX / 2, deltaY / 2, (int)mSize.x(), (int)mSize.y(),
                      hasFlag(mProps, ImgProps::KeepRatio), mTiled, mFlipX, mFlipY,
                      topLeftColor, topRightColor, bottomRightColor, bottomLeftColor,
                      mTopAlpha, mBottomAlpha);
    }
  }

  Component::renderChildren(trans);
}

void ImageComponent::fadeIn(bool textureLoaded)
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
      }
    }
    else if (mFading)
    {
      // The texture is loaded and we need to fade it in. The fade is based on the frame rate
      // and is 1/4 second if running at 60 frames per second although the actual value is not
      // that important
      int opacity = mFadeOpacity + (255 / 15);
      // See if we've finished fading
      if (opacity >= 255)
      {
        mFadeOpacity = 255;
        mFading = false;
      }
      else mFadeOpacity = (unsigned char) opacity;
    }
  }
}

void ImageComponent::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
  if (hasFlag(properties, ThemePropertyCategory::Size))
  {
    Vector2f scale = getParent() != nullptr ? getParent()->getSize() : Vector2f(
      Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat());

    if (element.HasProperty(ThemePropertyName::Size))
    {
      if (element.HasProperty(ThemePropertyName::KeepRatio))
        setKeepRatio(element.AsBool(ThemePropertyName::KeepRatio));
      setResize(element.AsVector(ThemePropertyName::Size) * scale);
    }
    else if (element.HasProperty(ThemePropertyName::MaxSize))
    {
      setKeepRatio(true);
      setResize(element.AsVector(ThemePropertyName::MaxSize) * scale);
    }
  }

  if (hasFlag(properties, ThemePropertyCategory::Effects))
    SetReflection(element.HasProperty(ThemePropertyName::Reflection) ? element.AsVector(ThemePropertyName::Reflection) : Vector2f());

  if (hasFlag(properties, ThemePropertyCategory::Path))
    setImage(element.HasProperty(ThemePropertyName::Path) ? element.AsPath(ThemePropertyName::Path) : Path::Empty,
             (element.HasProperty(ThemePropertyName::Tile) && element.AsBool(ThemePropertyName::Tile)));

  if (hasFlag(properties, ThemePropertyCategory::Color))
  {
    bool set = false;
    if (element.HasProperty(ThemePropertyName::Color)) { SetColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::Color)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorTop)) { SetTopColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorTop)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorBottom)) { SetBottomColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorBottom)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorLeft)) { SetLeftColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorLeft)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorRight)) { SetRightColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorRight)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorTopLeft)) { SetTopLeftColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorTopLeft)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorBottomRight)) { SetBottomRightColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorBottomRight)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorBottomLeft)) { SetBottomLeftColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorBottomLeft)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorTopRight)) { SetTopRightColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorTopRight)); set = true; }
    if (!set) setColor(0xFFFFFFFF); // No color = full white opaque
  }
}

bool ImageComponent::CollectHelpItems(Help& help)
{
  help.Set(Help::Valid(), _("SELECT"));
  return true;
}

void ImageComponent::setKeepRatio(bool keepRatio)
{
  bool hasKeepRatio = hasFlag(mProps, ImgProps::KeepRatio);
  if (hasKeepRatio != keepRatio)
  {
    mProps &= ~ImgProps::KeepRatio;
    if (keepRatio) mProps |= ImgProps::KeepRatio;
    mResizeRequired = true;
  }
}

void ImageComponent::Update(int deltaTime)
{
  (void)deltaTime;
}

void ImageComponent::StartLoading()
{
  if (mPath.IsEmpty() || !TextureManager::Exists(mPath)) mTexture.Clear();
  else
  {
    TextureHolder::Properties textureProperties = TextureHolder::Properties::None;
    if (hasFlag(mProps, ImgProps::ImmediateLoad)) textureProperties |= TextureHolder::Properties::ImmediateLoad;
    if (hasFlag(mProps, ImgProps::NoCache)) textureProperties |= TextureHolder::Properties::NoCache;
    if (hasFlag(mProps, ImgProps::NoFiltering)) textureProperties |= TextureHolder::Properties::NoFiltering;
    mTexture = TextureManager::Instance().Create(mPath, (int)mTargetSize.x(), (int)mTargetSize.y() , textureProperties);
    if (hasFlag(mProps, ImgProps::ImmediateLoad)) TextureManager::Instance().RequestTextureLoading(mTexture, false);
  }
  mNewPathRequired = false;
}
