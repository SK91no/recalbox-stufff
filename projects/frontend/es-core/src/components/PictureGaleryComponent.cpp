//
// Created by bkg2k on 13/10/23.
//

#include <components/PictureGaleryComponent.h>
#include <Renderer.h>
#include <themes/ThemeData.h>
#include <rendering/textures/TextureManager.h>
#include <input/InputCompactEvent.h>

PictureGaleryComponent::PictureGaleryComponent(WindowManager&window, ImgProps props, IChangeNotifications* callbackInterface)
  : ThemableComponent(window)
  , mCallbackInterface(callbackInterface)
  , mBackgroundColor(0)
  , mSelectorColor(0x80)
  , mVerticalRatio(0.00f)
  , mHorizontalRatio(0.05f)
  , mSlot(0)
  , mTargetSlot(0)
  , mItemGap(0.2f)
  , mProps(props)
  , mVisible(true)
{
}

void PictureGaleryComponent::SetImage(int slot, const Path& path)
{
  // Fill in empty indexes with no image
  while((int)mImages.size() <= slot) mImages.push_back(ImageSpec());
  mImages[slot] = ImageSpec(path, (int)mSize.y(), mProps);
}

void PictureGaleryComponent::setColor(unsigned int color)
{
  for(ImageSpec& img : mImages)
    img.mColorShift = color;
}

void PictureGaleryComponent::SetColor(int slot, unsigned int color)
{
  if ((unsigned int)slot < mImages.size())
     mImages[slot].mColorShift = color;
}

void PictureGaleryComponent::setOpacity(unsigned char opacity)
{
  mOpacity = opacity;
}

void PictureGaleryComponent::Render(const Transform4x4f& parentTrans)
{
  if (mThemeDisabled || !mVisible) return;

  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  // Global clipping
  Renderer::Instance().Clip(trans, mSize);

  // Get y center
  Rectangle globalArea(0.f, 0.f, mSize.x(), mSize.y());
  int squareSize = (int)mSize.y();
  float radius = (float)squareSize / 20;
  int x = (int)(globalArea.XCenter() - ((float)squareSize / 2.f));
  x -= ((Offset() * squareSize) >> 16);

  // Render background
  if (mBackgroundColor != 0)
    Renderer::DrawRoundRectangle(globalArea, radius, mBackgroundColor);

  // Get forward item count
  int xMax = x;
  int xSpace = Math::roundi((float)squareSize * mItemGap);
  int forwardCount = 1;
  int xLimit = (int)globalArea.Right();
  while (xMax + squareSize < xLimit) { xMax += squareSize + xSpace; forwardCount++; }
  // Get backward item count
  int backwardCount = Math::roundi((((float)x - globalArea.Left()) / (float)(squareSize +xSpace)) + .5f);

  int mainIndex = Index();

  // Draw selector
  int enlargement = Math::roundi((float)squareSize / 10.f);
  Renderer::DrawRoundRectangle((int)globalArea.XCenter() - ((squareSize + enlargement) / 2), 0,
                               (squareSize + enlargement), (int)globalArea.Height(), radius, mSelectorColor);

  // Foreground loop
  int xItem = xMax;
  for (int i = forwardCount; --i >= -backwardCount; )
  {
    // Get item rectangle
    Rectangle itemArea((float)xItem, globalArea.Top(), (float)squareSize, globalArea.Height());
    int index = (mainIndex + i) % (int)mImages.size();
    if (index < 0) index += (int)mImages.size();

    // Get image
    ImageSpec& image = mImages[index];
    if (bool fade = fadeIn(image); fade)
      image.mTexture.Render((int)itemArea.Left(), (int)itemArea.Top(), (int)itemArea.Width(), (int)itemArea.Height(), true, false, image.GetColor(0xFF, fade));

    // Previous y position
    xItem -= squareSize + xSpace;
  }

  // Scrollbar
  Renderer::SetMatrix(trans);
  //RenderScrollbar(globalArea, mainIndex);

  // Unclip
  Renderer::Instance().Unclip();
}

bool PictureGaleryComponent::fadeIn(ImageSpec& image)
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
    else if (image.mFading)
    {
      // The texture is loaded and we need to fade it in. The fade is based on the frame rate
      // and is 1/4 second if running at 60 frames per second although the actual value is not
      // that important
      int opacity = image.mFadeOpacity + (255 / 15);
      // See if we've finished fading
      if (opacity >= 255) { image.mFadeOpacity = 255; image.mFading = false; }
      else image.mFadeOpacity = (unsigned char)opacity;
    }
    else image.mFadeOpacity = 0xFF;
  }
  return ready;
}

void PictureGaleryComponent::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
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

  if (hasFlag(properties, ThemePropertyCategory::Color))
    setColor(element.HasProperty(ThemePropertyName::Color) ? (unsigned int)element.AsInt(ThemePropertyName::Color) : 0xFFFFFFFF);
}

void PictureGaleryComponent::Update(int deltaTime)
{
  // Move slot
  Move();
  Component::Update(deltaTime);
}

bool PictureGaleryComponent::ProcessInput(const InputCompactEvent& event)
{
  if (mImages.empty()) return false;

  // Process slot
  if (mImages.size() > 1)
  {
    if (event.AnyPrimaryLeftReleased()) { mTargetSlot -= 0x10000; return true; }
    if (event.AnyPrimaryRightReleased()) { mTargetSlot += 0x10000; return true; }
  }

  return false;
}

void PictureGaleryComponent::Move()
{
  if (!mImages.empty())
  {
    int minStep = Math::roundi(65536.f / (mSize.y() * (1.f + mItemGap)));
    if (mSlot != mTargetSlot)
    {
      int oldSlot = Index();
      int move = (mTargetSlot - mSlot) >> 2;
      if      (mTargetSlot - mSlot < 0) move = Math::min(move, -minStep);
      else if (mTargetSlot - mSlot > 0) move = Math::max(move,  minStep);
      mSlot += move;
      if (Math::absi(mTargetSlot - mSlot) < minStep) mSlot = mTargetSlot;
      if (int newSlot = Index(); newSlot != oldSlot)
        if (mCallbackInterface != nullptr)
          mCallbackInterface->PictureGalerySlotChanged(newSlot);
    }
    else
    {
      if (mSlot < 0)
      {
        while (mSlot < 0) mSlot += (int) mImages.size() << 16;
        mTargetSlot = mSlot;
      }
      else if (mSlot > (int) mImages.size() << 16)
      {
        mSlot %= (int) mImages.size() << 16;
        mTargetSlot = mSlot;
      }
    }
  }
}

void PictureGaleryComponent::ImageSpec::Load(int squareSize)
{
  if (mPath.IsEmpty() || !TextureManager::Exists(mPath)) mTexture.Clear();
  else
  {
    TextureHolder::Properties textureProperties = TextureHolder::Properties::None;
    if (hasFlag(mProps, ImgProps::ImmediateLoad)) textureProperties |= TextureHolder::Properties::ImmediateLoad;
    if (hasFlag(mProps, ImgProps::NoCache)) textureProperties |= TextureHolder::Properties::NoCache;
    if (hasFlag(mProps, ImgProps::NoFiltering)) textureProperties |= TextureHolder::Properties::NoFiltering;
    mTexture = TextureManager::Instance().Create(mPath, squareSize, squareSize, textureProperties);
    if (hasFlag(mProps, ImgProps::ImmediateLoad)) TextureManager::Instance().RequestTextureLoading(mTexture, true);
  }
}
