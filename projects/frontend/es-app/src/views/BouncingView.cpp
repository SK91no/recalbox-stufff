//
// Created by Pit64 on 27/03/2026.
//

#include "views/BouncingView.h"
#include <input/InputCompactEvent.h>
#include <help/Help.h>
#include <views/ViewController.h>
#include <random>

namespace
{
  std::mt19937& RandomEngine()
  {
    static std::mt19937 engine(std::random_device{}());
    return engine;
  }
}

BouncingView::BouncingView(WindowManager& window)
  : Gui(window)
  , mWindow(window)
  , mImage(window, ImgProps::KeepRatio | ImgProps::ImmediateLoad)
  , mX(0)
  , mY(0)
  , mSpeedX(0.f)
  , mSpeedY(0.f)
  , mImageWidth(0)
  , mImageHeight(0)
{
  // Load logo and compute target size (10% of screen height, keep ratio)
  float targetHeight = Renderer::Instance().DisplayHeightAsFloat() * 0.15f;
  mImage.setResize(0.f, targetHeight);
  mImage.setImage(Path(":/Recalbox_logo_White.svg"), false, true);

  // Cache image size - use target size as fallback if image not ready
  mImageWidth = mImage.getSize().x() > 0 ? mImage.getSize().x() : targetHeight * 3.f;
  mImageHeight = mImage.getSize().y() > 0 ? mImage.getSize().y() : targetHeight;

  // Random starting position
  float screenW = Renderer::Instance().DisplayWidthAsFloat();
  float screenH = Renderer::Instance().DisplayHeightAsFloat();
  mSpeedX = screenW / 12800.f;
  mSpeedY = screenH / 10800.f;
  std::uniform_real_distribution<float> distX(0.f, screenW - mImageWidth);
  std::uniform_real_distribution<float> distY(0.f, screenH - mImageHeight);
  mX = distX(RandomEngine());
  mY = distY(RandomEngine());

  PickRandomColor();
}

void BouncingView::Reset()
{
  float screenW = Renderer::Instance().DisplayWidthAsFloat();
  float screenH = Renderer::Instance().DisplayHeightAsFloat();
  mSpeedX = screenW / 12800.f;
  mSpeedY = screenH / 10800.f;
  std::uniform_real_distribution<float> distX(0.f, screenW - mImageWidth);
  std::uniform_real_distribution<float> distY(0.f, screenH - mImageHeight);
  mX = distX(RandomEngine());
  mY = distY(RandomEngine());

  PickRandomColor();
}

void BouncingView::PickRandomColor()
{
  std::uniform_int_distribution<int> dist(0, static_cast<int>(sizeof(sColors) / sizeof(sColors[0])) - 1);
  mImage.setColorShift(sColors[dist(RandomEngine())]);
}

void BouncingView::Update(int elapsed)
{
  Component::Update(elapsed);

  float dx = mSpeedX * static_cast<float>(elapsed);
  float dy = mSpeedY * static_cast<float>(elapsed);

  mX += dx;
  mY += dy;

  float screenW = Renderer::Instance().DisplayWidthAsFloat();
  float screenH = Renderer::Instance().DisplayHeightAsFloat();
  // Bounce on right/left
  if (mX + mImageWidth >= screenW) { mX = screenW - mImageWidth; mSpeedX = -mSpeedX; PickRandomColor(); }
  else if (mX <= 0)                { mX = 0;                     mSpeedX = -mSpeedX; PickRandomColor(); }

  // Bounce on bottom/top
  if (mY + mImageHeight >= screenH) { mY = screenH - mImageHeight; mSpeedY = -mSpeedY; PickRandomColor(); }
  else if (mY <= 0)                 { mY = 0;                      mSpeedY = -mSpeedY; PickRandomColor(); }
}

void BouncingView::Render(const Transform4x4f& parentTrans)
{
  Transform4x4f trans = parentTrans * getTransform();

  // Black background
  Renderer::SetMatrix(trans);
  Renderer::DrawRectangle(0, 0, Renderer::Instance().DisplayWidthAsInt(), Renderer::Instance().DisplayHeightAsInt(), 0xFF000000);

  // Draw the logo at the current position
  mImage.setPosition(mX, mY);
  mImage.Render(trans);
}

bool BouncingView::ProcessInput(const InputCompactEvent& event)
{
  // Any button press exits the screensaver
  if (event.AnythingPressed())
  {
    mWindow.DoWake();
    ViewController::Instance().BackToPreviousView();
    return true;
  }
  return true;
}

bool BouncingView::CollectHelpItems(Help& help)
{
  help.Clear();
  return true;
}