//
// Created by xizor on 30/03/18.
//

#include "GuiInfoPopupBase.h"
#include "Renderer.h"
#include <SDL_timer.h>
#include <themes/MenuThemeData.h>
#include <components/ScrollableContainer.h>
#include <themes/ThemeManager.h>
#include <hardware/Board.h>

GuiInfoPopupBase::GuiInfoPopupBase(WindowManager& window, bool selfProcessed, int duration, PopupType icon, int gridWidth, int gridHeight, float widthRatio)
  : Gui(window)
  , mGrid(window, Vector2i(gridWidth, gridHeight))
  , mFrame(window, Path(":/frame.png"))
  , mFrameColor(0)
  , mType(icon)
  , mCorner(Corner::TopRight)
  , mTargetOffset(0)
  , mDuration(duration < 0 ? -1 : duration * 1000)
  , mFadeTimeAccumulator(0)
  , mMaxAlpha(0xFF)
  , mWidthRatio(widthRatio)
  , mState(State::FadeIn)
  , mInitialized(false)
  , mSelfProcessed(selfProcessed)
{
}

void GuiInfoPopupBase::Initialize()
{
  if (mInitialized) return;
  mInitialized = true;

  float maxWidth = mWidthRatio * Renderer::Instance().DisplayWidthAsFloat() * (Renderer::Instance().Is480pOrLower() ? 0.6f : 0.2f);
  float maxHeight = Renderer::Instance().DisplayHeightAsFloat() * (Renderer::Instance().Is480pOrLower() ? 0.6f : 0.4f);

  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

  mMaxAlpha = (int)menuTheme.Background().color & 0xFF;
  mFrameColor = menuTheme.Background().color;

  // add a padding to the box
  int paddingX = (int) (Renderer::Instance().DisplayWidthAsFloat() * 0.02f);
  int paddingY = (int) (Renderer::Instance().DisplayHeightAsFloat() * 0.02f);

  float msgHeight = AddComponents(mWindow, mGrid, maxWidth, maxHeight, paddingX, paddingY);
  mGrid.setSize(maxWidth + (float)paddingX, msgHeight + (float)paddingY);

  mCorner = Corner::TopRight;
  float posX = (Renderer::Instance().DisplayWidthAsFloat() * (Board::Instance().CrtBoard().IsCrtAdapterAttached() ? 0.94f : 0.98f )) - mGrid.getSize().x();
  float posY = Renderer::Instance().DisplayHeightAsFloat();

  setPosition(posX, posY, 0);
  setSize(mGrid.getSize());

  mFrame.setImagePath(menuTheme.Background().path);
  mFrame.setCenterColor(mFrameColor);
  mFrame.setEdgeColor(mFrameColor);
  mFrame.fitTo(mGrid.getSize(), Vector3f::Zero(), Renderer::Instance().Is480pOrLower() ? Vector2f(-24, -24) :Vector2f(-32, -32));
  addChild(&mFrame);

  addChild(&mGrid);
}

void GuiInfoPopupBase::Render(const Transform4x4f& parentTrans)
{
  (void)parentTrans;

	// we use identity as we want to render on a specific window position, not on the view
	Transform4x4f trans = getTransform() * Transform4x4f::Identity();

  Renderer::SetMatrix(trans);
  renderChildren(trans);
}

void GuiInfoPopupBase::Update(int delta)
{
  updateChildren(delta);

  // Update position
  switch(mCorner)
  {
    case Corner::TopRight:
    case Corner::TopLeft:
    {
      float targetY = (Renderer::Instance().DisplayHeightAsFloat() * (Board::Instance().CrtBoard().IsCrtAdapterAttached() ? 0.06f : 0.02f )) + (float)mTargetOffset;
      float diff = (mPosition.y() - targetY) * .85f;
      if (diff >= -2.0f && diff <= 2.0f) diff = 0;
      mPosition.y() = targetY + diff;
      break;
    }
    case Corner::BottomRight:
    case Corner::BottomLeft:
    {
      float targetY = (Renderer::Instance().DisplayHeightAsFloat() * (Board::Instance().CrtBoard().IsCrtAdapterAttached() ? 0.94f : 0.98f)) - mSize.y() - (float)mTargetOffset;
      float diff = (mPosition.y() - targetY) * .85f;
      if (diff >= -2.0f && diff <= 2.0f) diff = 0;
      mPosition.y() = targetY + diff;
      break;
    }
  }

  // Process life cycle
  switch(mState)
  {
    case State::FadeIn:
    {
      mFadeTimeAccumulator += delta;
      int alpha = Math::clampi((mFadeTimeAccumulator * mMaxAlpha) / sFadeTime, 0, 255);
      mGrid.setOpacity(alpha);
      mFrame.setEdgeColor((mFrameColor & 0xffffff00) | alpha);
      mFrame.setCenterColor((mFrameColor & 0xffffff00) | alpha);
      if (mFadeTimeAccumulator >= sFadeTime) SetState(State::Running);
      break;
    }
    case State::Running:
    {
      if (mDuration >= 0)
        if (mDuration -= delta; mDuration < 0)
          SetState(State::FadeOut);
      break;
    }
    case State::FadeOut:
    {
      mFadeTimeAccumulator += delta;
      int alpha = 255 - Math::clampi((mFadeTimeAccumulator * mMaxAlpha) / sFadeTime, 0, 255);
      mGrid.setOpacity(alpha);
      mFrame.setEdgeColor((mFrameColor & 0xffffff00) | alpha);
      mFrame.setCenterColor((mFrameColor & 0xffffff00) | alpha);
      if (mFadeTimeAccumulator >= sFadeTime) SetState(State::Stopped);
      break;
    }
    case State::Stopped: break;
  }
}

void GuiInfoPopupBase::SlideOffset(int size)
{
  // Update position
  switch(mCorner)
  {
    case Corner::TopRight:
    case Corner::TopLeft:
    {
      mTargetOffset -= size;
      break;
    }
    case Corner::BottomRight:
    case Corner::BottomLeft:
    {
      mTargetOffset += size;
      break;
    }
  }
}


