//
// Created by bkg2k on 16/02/2026.
//

#include "PictureExplorerComponent.h"

#include "help/Help.h"
#include "input/InputCompactEvent.h"
#include "rendering/textures/TextureManager.h"
#include "utils/locale/LocaleHelper.h"

PictureExplorerComponent::PictureExplorerComponent(WindowManager& window, NewFont& font, const Path& picturePath)
  : Component(window)
  , mPath(picturePath)
  , mFont(font)
  , mImageInitialized(false)
  , mGlobalAlpha(0)
  , mMaxZoom(0)
  , mMinZoom(0)
  , mStepZoom(0)
  , mCurrentZoom(0)
  , mTargetZoom(0)
  , mHDirection(0)
  , mVDirection(0)
  , mMoveDelay(0)
  , mHBarTimer(0)
  , mVBarTimer(0)
  , mScrollbarThickness(Renderer::Instance().DisplayWidthAsInt() > 1024 ? 6 : (Renderer::Instance().DisplayWidthAsInt() > 480 ? 4 : 2))
  , mZoomPercentDisplayTimer(0)
{
  if (picturePath.Exists())
    LoadPicture();
}

PictureExplorerComponent& PictureExplorerComponent::SetPicturePath(const Path& picturePath)
{
  mPath = picturePath;
  mImageInitialized = false;
  mGlobalAlpha = 0;
  LoadPicture();
  return *this;
}

bool PictureExplorerComponent::ProcessInput(const InputCompactEvent& event)
{
  if (mThemeDisabled) return false;

  // Sustained moves
  if (event.UpPressed()) { mMoveDelay = sMoveDelay; mVDirection = 1; return true; }
  if (event.DownPressed()) { mMoveDelay = sMoveDelay; mVDirection = -1; return true; }
  if (event.LeftPressed()) { mMoveDelay = sMoveDelay; mHDirection = 1; return true; }
  if (event.RightPressed()) { mMoveDelay = sMoveDelay; mHDirection = -1; return true; }
  if (event.J1UpPressed()) { mMoveDelay = sMoveDelay; mVDirection = 4; return true; }
  if (event.J1DownPressed()) { mMoveDelay = sMoveDelay; mVDirection = -4; return true; }
  if (event.J1LeftPressed()) { mMoveDelay = sMoveDelay; mHDirection = 4; return true; }
  if (event.J1RightPressed()) { mMoveDelay = sMoveDelay; mHDirection = -4; return true; }
  // Single step moves
  if (event.UpReleased() && mMoveDelay != 0) { mVDirection = mHDirection = mMoveDelay = 0; MovePage(0, 1, 0); return true; }
  if (event.DownReleased() && mMoveDelay != 0) { mVDirection = mHDirection = mMoveDelay = 0; MovePage(0, -1, 0); return true; }
  if (event.LeftReleased() && mMoveDelay != 0) { mVDirection = mHDirection = mMoveDelay = 0; MovePage(1, 0, 0); return true; }
  if (event.RightReleased() && mMoveDelay != 0) { mVDirection = mHDirection = mMoveDelay = 0; MovePage(-1, 0, 0); return true; }
  // Stop moves
  if (event.UpReleased() || event.DownReleased()) { mVDirection = 0; return true; }
  if (event.LeftReleased() || event.RightReleased()) { mHDirection = 0; return true; }

  // Zoom in/out
  if (event.NorthReleased()) { ZoomPage(true); return true; }
  if (event.WestReleased()) { ZoomPage(false); return true; }

  return Component::ProcessInput(event);
}

void PictureExplorerComponent::Update(int deltaTime)
{
  if (mThemeDisabled) return;

  // Global alpha
  if (mImageInitialized)
    mGlobalAlpha = Math::clampi(mGlobalAlpha + deltaTime, 0, 255);

  // Scrollbar
  if (mHBarTimer > 0) if (mHBarTimer -= deltaTime; mHBarTimer < 0) mHBarTimer = 0;
  if (mVBarTimer > 0) if (mVBarTimer -= deltaTime; mVBarTimer < 0) mVBarTimer = 0;

  // Sustained move
  if (mMoveDelay > 0) { if (mMoveDelay -= deltaTime; mMoveDelay < 0) mMoveDelay = 0; return; }
  if (mVDirection != 0 || mHDirection != 0) MovePage(mHDirection, mVDirection, deltaTime);

  // OSD
  if (mZoomPercentDisplayTimer > 0)
    if (mZoomPercentDisplayTimer -= deltaTime; mZoomPercentDisplayTimer < 0) mZoomPercentDisplayTimer = 0;

  // Zoom animation
  AnimateZoom();
}

void PictureExplorerComponent::Render(const Transform4x4f& parentTrans)
{
  if (mThemeDisabled) return;

  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  // Render PDF image
  if (InitializeDisplay())
  {
    Renderer::Instance().Clip(trans, mSize);

    mPageTexture.Render(Math::roundi(mPageArea.Left()), Math::roundi(mPageArea.Top()),
                        Math::roundi(mPageArea.Width()), Math::roundi(mPageArea.Height()), true, false, 0xFFFFFF00 | mGlobalAlpha);

    // Render zoom OSD
    if (mZoomPercentDisplayTimer > 0)
    {
      Rectangle displayArea(0.f, 0.f, mSize.x(), mSize.y());
      String page = (_F(_("Zoom {0}%")) / (int)(mTargetZoom / mMaxZoom * 200.f))();
      int alpha = Math::clampi(mZoomPercentDisplayTimer, 0, 255);
      unsigned int white = 0xFFFFFF00 | alpha;
      mFont.DrawText(page, displayArea, -1, 0, white, Alignment::Center, mDefaultEffect);
      mFont.DrawText(page, displayArea, 1, 0, white, Alignment::Center, mDefaultEffect);
      mFont.DrawText(page, displayArea, 0, -1, white, Alignment::Center, mDefaultEffect);
      mFont.DrawText(page, displayArea, 0, 1, white, Alignment::Center, mDefaultEffect);
      mFont.DrawText(page, displayArea, 0,0 , alpha, Alignment::Center, mDefaultEffect);
    }

    // Draw vertical bar
    if (mVBarTimer > 0)
      if (float ratio = mSize.y() / mPageArea.Height(); ratio < 1.f)
      {
        int barHeight = Math::roundi(ratio * mSize.y());
        Renderer& renderer = Renderer::Instance();
        if ((barHeight < renderer.DisplayHeightAsInt() / 40) && (mSize.y() >= renderer.DisplayHeightAsFloat() / 20.f)) barHeight = renderer.DisplayHeightAsInt() / 40;
        int yOffset = (int)((mSize.y() - (float)barHeight - 4.f) * (-mPageArea.Top() / (mPageArea.Height() - mSize.y())));
        int alpha = Math::clampi(mVBarTimer / 2, 0, 255);
        Renderer::DrawRectangle((int) mSize.x() - 4 - mScrollbarThickness, 1 + yOffset, mScrollbarThickness + 2, barHeight + 2, 0xFFFFFF00 | alpha);
        Renderer::DrawRectangle((int) mSize.x() - 3 - mScrollbarThickness, 2 + yOffset, mScrollbarThickness, barHeight, alpha);
      }

    // Draw horizontal bar
    if (mHBarTimer > 0)
      if (float ratio = mSize.x() / mPageArea.Width(); ratio < 1.f)
      {
        int barWidth = Math::roundi(ratio * mSize.x());
        Renderer& renderer = Renderer::Instance();
        if ((barWidth < renderer.DisplayHeightAsInt() / 40) && (mSize.x() >= renderer.DisplayHeightAsFloat() / 20.f)) barWidth = renderer.DisplayHeightAsInt() / 40;
        int xOffset = (int)((mSize.x() - (float)barWidth - 4.f) * (-mPageArea.Left() / (mPageArea.Width() - mSize.x())));
        int alpha = Math::clampi(mVBarTimer / 2, 0, 255);
        Renderer::DrawRectangle(1 + xOffset, (int) mSize.y() - 4 - mScrollbarThickness, barWidth + 2, mScrollbarThickness + 2, 0xFFFFFF00 | alpha);
        Renderer::DrawRectangle(2 + xOffset, (int) mSize.y() - 3 - mScrollbarThickness, barWidth, mScrollbarThickness, alpha);
      }

    Renderer::Instance().Unclip();
  }
}

bool PictureExplorerComponent::CollectHelpItems(Help& help)
{
  help.Set(HelpType::AllDirections, _("MOVE"))
      .Set(HelpType::X, _("ZOOM +"))
      .Set(HelpType::Y, _("ZOOM -"));
  return true;
}

void PictureExplorerComponent::LoadPicture()
{
  // Unsized component
  if (mSize.isZero() || !mPath.Exists()) return;

  // Create texture and upload data
  mPageTexture = TextureManager::Instance().Create(mPath, 0, 0, TextureHolder::Properties::NoCache);
}

bool PictureExplorerComponent::InitializeDisplay()
{
  if (!mImageInitialized)
    if (mImageInitialized = mPageTexture.MakeReady(); mImageInitialized)
    {
      Vector2i textureSize = mPageTexture.Size();
      // Initialize zoom
      mCurrentZoom = mTargetZoom = mSize.x() / (float)textureSize.x();
      if (mCurrentZoom < 1.f) mCurrentZoom = mTargetZoom = 1.0;
      mMaxZoom = mCurrentZoom * 2.f;
      mMinZoom = mCurrentZoom / 4.f;
      mStepZoom = mCurrentZoom / 4.f;
      // Reset position
      mPageArea.Set(0, 0, mCurrentZoom * (float)textureSize.x(), mCurrentZoom * (float)textureSize.y()).Round();
      // Reset alpha
      mGlobalAlpha = 0;
    }

  return mImageInitialized;
}

float PictureExplorerComponent::CalculateRenderingZoom(float originalWidth, float originalHeight)
{
  // Get both ratio
  float xRatio = mSize.x() / originalWidth;
  float yRatio = mSize.y() / originalHeight;

  // PDF is smaller than the display area, set the rendering zoom to 1
  if (xRatio > 1.f && yRatio > 1.f) return 1.f;

  // Get the lower ratio and compute the rendering zoom
  if (xRatio < yRatio) return (mSize.x() * 2.f) / originalWidth;
  return (mSize.y() * 2.f) / originalHeight;
}

void PictureExplorerComponent::MovePage(int xDir, int yDir, int elapsedTime)
{
  float yMove = 0;
  float xMove = 0;
  if (elapsedTime == 0) elapsedTime = 20;
  Rectangle displayArea(0.f, 0.f, mSize.x(), mSize.y());
  displayArea.Round();

  // Move ?
  if ((yDir > 0 && mPageArea.Top() < displayArea.Top()) ||
      (yDir < 0 && mPageArea.Bottom() > displayArea.Bottom())) yMove = (float)(elapsedTime * yDir);
  if ((xDir > 0 && mPageArea.Left() < displayArea.Left()) ||
      (xDir < 0 && mPageArea.Right() > displayArea.Right())) xMove = (float)(elapsedTime * xDir);

  // Move !
  if (xMove != 0 || yMove != 0)
  {
    xMove = Math::round((xMove / 20.f) * (Renderer::Instance().DisplayHeightAsFloat() / 50.f));
    yMove = Math::round((yMove / 20.f) * (Renderer::Instance().DisplayHeightAsFloat() / 50.f));
    mPageArea.Move(xMove, yMove);
  }

  AdjustBounds();
}

void PictureExplorerComponent::ZoomPage(bool zoomIn)
{
  if (zoomIn) { if (mTargetZoom >= mMaxZoom) return; }
  else if (mTargetZoom <= mMinZoom) return;

  mTargetZoom += zoomIn ? mStepZoom : -mStepZoom;
  mTargetZoom = Math::clamp(mTargetZoom, mMinZoom, mMaxZoom);

  mZoomPercentDisplayTimer = sZoomPercentTimer;
}

void PictureExplorerComponent::AdjustBounds()
{
  Rectangle displayArea(0.f, 0.f, mSize.x(), mSize.y());

  // Adjust page bounds
  if (mPageArea.Width() > Math::round(displayArea.Width()))
  {
    if (mPageArea.Right() < displayArea.Right()) mPageArea.Move(displayArea.Right() - mPageArea.Right(), 0);
    if (mPageArea.Left() > displayArea.Left()) mPageArea.Move(displayArea.Left() - mPageArea.Left(), 0);
    mHBarTimer = sBarTimer;
  }
  else mPageArea.SetLeft((displayArea.Width() - mPageArea.Width()) / 2);

  if (mPageArea.Height() > Math::round(displayArea.Height()))
  {
    if (mPageArea.Bottom() < displayArea.Bottom()) mPageArea.Move(0, displayArea.Bottom() - mPageArea.Bottom());
    if (mPageArea.Top() > displayArea.Top()) mPageArea.Move(0, displayArea.Top() - mPageArea.Top());
    mVBarTimer = sBarTimer;
  }
  else mPageArea.SetTop((displayArea.Height() - mPageArea.Height()) / 2);
}

void PictureExplorerComponent::AnimateZoom()
{
  if (mTargetZoom == mCurrentZoom) return;

  // Zoom animation
  float height = (float)mPageTexture.Height();
  float minStep = Math::abs((height * mTargetZoom) - (height * mCurrentZoom));
  if (minStep <= 2.f) mCurrentZoom = mTargetZoom;
  else mCurrentZoom = (mCurrentZoom + mTargetZoom) / 2.f;

  Rectangle displayArea(0.f, 0.f, mSize.x(), mSize.y());
  float xRatio = (displayArea.XCenter() - mPageArea.Left()) / mPageArea.Width();
  float yRatio = (displayArea.YCenter() - mPageArea.Top()) / mPageArea.Height();

  Vector2i size = mPageTexture.Size();
  mPageArea.Set(displayArea.XCenter() - ((float)size.x() * xRatio), displayArea.YCenter() - ((float)size.y() * yRatio), (float)size.x(), (float)size.y())
           .ZoomRelative(mCurrentZoom, xRatio, yRatio);

  AdjustBounds();
}
