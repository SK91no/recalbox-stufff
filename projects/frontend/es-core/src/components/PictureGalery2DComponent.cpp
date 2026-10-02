//
// Created by bkg2k on 13/10/23.
//

#include <components/PictureGalery2DComponent.h>
#include <Renderer.h>
#include <help/Help.h>
#include <themes/ThemeData.h>
#include <rendering/textures/TextureManager.h>
#include <input/InputCompactEvent.h>

PictureGalery2DComponent::PictureGalery2DComponent(WindowManager&window, const Path::PathList& imagePath, ImgProps props, IChangeNotifications* callbackInterface)
  : ThemableComponent(window)
  , mCallbackInterface(callbackInterface)
  , mPlaceHolder(*this)
  , mVerticalRatio(0.00f)
  , mHorizontalRatio(0.05f)
  , mSlot(0)
  , mTargetSlot(0)
  , mProps(props)
  , mVisible(true)
{
  int index = 0;
  for(const Path& path : imagePath)
    setImage(0, index++, path, false);
  mPlaceHolder.Set(Path(":/themes/placeholder.svg"), ImgProps::ImmediateLoad, false);
}

void PictureGalery2DComponent::setImage(int slot, int index, const Path& path, bool tiled)
{
  // Fill in empty indexes with no image
  while((int)mImages.size() <= slot) mImages.push_back(ImageList());
  // Add image
  mImages[slot].AddImage(index, path, mProps, *this, tiled);
}

void PictureGalery2DComponent::setPlaceHolderImage(const Path& path, bool tiled)
{
  mPlaceHolder.Set(path, ImgProps::ImmediateLoad, tiled);
  mPlaceHolder.Load();
}

void PictureGalery2DComponent::setColor(unsigned int color)
{
  for(ImageList& list : mImages)
    for(ImageSpec& img : list.Images())
      img.mColorShift = color;
}

void PictureGalery2DComponent::setColor(int slot, int index, unsigned int color)
{
  if ((unsigned int)slot < mImages.size())
    if (ImageList& list = mImages[slot]; (unsigned int)index < list.Images().size())
      list.Images()[index].mColorShift = color;
}

void PictureGalery2DComponent::setOpacity(unsigned char opacity)
{
  mOpacity = opacity;
}

void PictureGalery2DComponent::Render(const Transform4x4f& parentTrans)
{
  if (mThemeDisabled || !mVisible) return;

  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  Rectangle baseArea(0.f, 0.f, mSize.x(), mSize.y());
  if ((mBackgroundColor & 0xFF) != 0)
    Renderer::DrawRectangle(baseArea, mBackgroundColor);

  if (mImages.empty()) return;

  Renderer::Instance().Clip(trans, mSize);

  for(int s = 3; --s >= 0; )
  {
    Rectangle area(baseArea);
    area.Contract(mSize.x() / 10.f, mSize.y() / 10.f).Round();

    ImageList& imageList = mImages[Slot(s - 1)];
    int imageCount = (int)imageList.Images().size();
    std::vector<ImageSpec>& images = imageList.Images();
    int offsetX = Math::roundi(area.Width() + (area.Width() * mHorizontalRatio));
    int moveX = Offset();
    moveX = (moveX * (int)mSize.x()) >> 16;

    area.Move((float)(((s - 1) * offsetX) - moveX), 0);
    if (area.Left() > baseArea.Right() ||  (area.Right() < baseArea.Left())) continue;

    float offsetY = Math::round(area.Height() + (area.Height() * mVerticalRatio));
    int referenceY = (int)area.Top();
    int referenceHeight = (int)offsetY * 2;

    int realIndex = imageList.Index();
    int moveY = imageList.Offset();
    moveY = (moveY * (int)offsetY) >> 16;

    int halpha = 255 - Math::clampi((Math::absi((int)(mSize.x() / 10.f) - (int)area.Left()) * 255) / referenceHeight, 0, 255);

    bool keepRatio = hasFlag(mProps, ImgProps::KeepRatio);
    if (mOpacity > 0)
    {
      area.Move(0, -offsetY - (float)moveY);
      int alpha = 255 - Math::clampi((Math::absi(referenceY - (int)area.Top()) * 255) / referenceHeight, 0, 255);
      ImageSpec& prev = images[realIndex == 0 ? imageCount - 1 : realIndex - 1];
      bool ready = fadeIn(prev);
      (ready ? prev : mPlaceHolder).mTexture.Render((int)area.Left(), (int)area.Top(), (int)area.Width(), (int)area.Height(), keepRatio, false, prev.GetColor((mOpacity * alpha * halpha) >> 16, ready));

      area.Move(0, (offsetY * 2));
      alpha = 255 - Math::clampi((Math::absi(referenceY - (int)area.Top()) * 255) / referenceHeight, 0, 255);
      ImageSpec& next = images[realIndex >= imageCount - 1 ? 0 : realIndex + 1];
      ready = fadeIn(next);
      (ready ? next : mPlaceHolder).mTexture.Render((int)area.Left(), (int)area.Top(), (int)area.Width(), (int)area.Height(), keepRatio, false, next.GetColor((mOpacity * alpha * halpha) >> 16, ready));

      area.Move(0, -offsetY);
      alpha = 255 - Math::clampi((Math::absi(referenceY - (int)area.Top()) * 255) / referenceHeight, 0, 255);
      ImageSpec& middle = images[realIndex];
      ready = fadeIn(middle);
      (ready ? middle : mPlaceHolder).mTexture.Render((int)area.Left(), (int)area.Top(), (int)area.Width(), (int)area.Height(), keepRatio, false, middle.GetColor((mOpacity * alpha * halpha) >> 16, ready));
    }
  }
  Renderer::Instance().Unclip();
}

bool PictureGalery2DComponent::fadeIn(ImageSpec& image)
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

void PictureGalery2DComponent::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
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

  if (hasFlag(properties, ThemePropertyCategory::Path))
    setPlaceHolderImage(element.HasProperty(ThemePropertyName::Path) ? element.AsPath(ThemePropertyName::Path) : Path::Empty,
                        (element.HasProperty(ThemePropertyName::Tile) && element.AsBool(ThemePropertyName::Tile)));

  if (hasFlag(properties, ThemePropertyCategory::Color))
    setColor(element.HasProperty(ThemePropertyName::Color) ? (unsigned int)element.AsInt(ThemePropertyName::Color) : 0xFFFFFFFF);
}

void PictureGalery2DComponent::Update(int deltaTime)
{
  //! Move indexes in image lists
  for(ImageList& list : mImages)
    list.Move(Index(), mCallbackInterface);
  // Move slot
  Move();

  Component::Update(deltaTime);
}

bool PictureGalery2DComponent::ProcessInput(const InputCompactEvent& event)
{
  if (mImages.empty()) return false;

  // Process keyboard in the appropriate slot
  int slot = (mSlot + 32768) >> 16;
  while(slot < 0) slot += (int)mImages.size();
  slot %= (int)mImages.size();
  if (mImages[slot].ProcessInput(event)) return true;

  // Process slot
  if (mImages.size() > 1)
  {
    if (event.AnyPrimaryLeftReleased()) { mTargetSlot -= 0x10000; return true; }
    if (event.AnyPrimaryRightReleased()) { mTargetSlot += 0x10000; return true; }
  }

  return false;
}

void PictureGalery2DComponent::Move()
{
  if (!mImages.empty())
  {
    int minStep = 0x10000 >> 8;
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
          mCallbackInterface->PictureSliderSlotChanged(newSlot, mImages[newSlot].Index());
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

void PictureGalery2DComponent::ImageSpec::Load()
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

void PictureGalery2DComponent::ImageList::Move(int slot, IChangeNotifications* callbackInterface)
{
  if (!mImageList.empty())
  {
    int minStep = 0x10000 >> 8;
    if (mIndex != mTargetIndex)
    {
      int oldIndex = Index();
      int move = (mTargetIndex - mIndex) >> 2;
      if      (mTargetIndex - mIndex < 0) move = Math::min(move, -minStep);
      else if (mTargetIndex - mIndex > 0) move = Math::max(move,  minStep);
      mIndex += move;
      if (Math::absi(mTargetIndex - mIndex) < minStep) mIndex = mTargetIndex;
      if (int newIndex = Index(); newIndex != oldIndex)
        if (callbackInterface != nullptr)
          callbackInterface->PictureSliderImageChanged(slot, newIndex);
    }
    else
    {
      if (mIndex < 0)
      {
        while (mIndex < 0) mIndex += (int) mImageList.size() << 16;
        mTargetIndex = mIndex;
      }
      else if (mIndex > (int) mImageList.size() << 16)
      {
        mIndex %= (int) mImageList.size() << 16;
        mTargetIndex = mIndex;
      }
    }
  }
}

bool PictureGalery2DComponent::ImageList::ProcessInput(const InputCompactEvent& event)
{
  if (mImageList.empty()) return false;

  if (event.AnyPrimaryUpReleased()) { mTargetIndex -= 0x10000; return true; }
  if (event.AnyPrimaryDownReleased()) { mTargetIndex += 0x10000; return true; }
  return false;
}

void PictureGalery2DComponent::ImageList::AddImage(int index, const Path& path, ImgProps props, PictureGalery2DComponent& parent, bool tiled)
{
  while((int)mImageList.size() < index) mImageList.push_back(ImageSpec(parent));
  // Add or set Image
  if (index == (int)mImageList.size()) mImageList.push_back(ImageSpec(parent, path, props, tiled));
  else mImageList[index].Set(path, props, tiled);
}
