//
// Created by bkg2k on 03/10/23.
//

#include "BluetoothOSD.h"
#include "Renderer.h"
#include "themes/MenuThemeData.h"
#include <themes/ThemeManager.h>

BluetoothOSD::BluetoothOSD(WindowManager& window, BaseOSD::Side side)
  : BaseOSD(window, side, false)
  , mControllerIcon(window)
  , mBtTop(window)
  , mBtMiddle(window)
  , mBtBottom(window)
  , mRemaining(0)
  , mTotal(0)
  , mBarBackColor(0)
  , mBarForeColor(0)
{
  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
  mBarBackColor = menuTheme.Background().color;
  mBarForeColor = menuTheme.Text().color;

  float sw = Renderer::Instance().DisplayWidthAsFloat();
  float sh = Renderer::Instance().DisplayHeightAsFloat();

  // BT Icon
  mControllerIcon.setPosition(0, sh / 36);
  mControllerIcon.setResize(sw / 32, sw / 32);
  mControllerIcon.setKeepRatio(true);
  mControllerIcon.setColor(menuTheme.Background().color);
  mControllerIcon.setImage(Path(":/bluetooth/bt_controller.svg"), false);
  mBTArea = Rectangle(mControllerIcon.getPosition().v2(), mControllerIcon.getTargetSize());

  float x = Math::round(mControllerIcon.getPosition().x() - (mControllerIcon.getTargetSize().x() * mControllerIcon.getOrigin().x()) + (mControllerIcon.getTargetSize().x() / 2));
  mBtBottom.setOrigin(0.5, 0);
  mBtBottom.setPosition(x, mControllerIcon.getPosition().y() - (sh / 90));
  mBtBottom.setResize(sw / 85, sw / 85);
  mBtBottom.setKeepRatio(true);
  mBtBottom.setColor(menuTheme.Background().color);
  mBtBottom.setImage(Path(":/bluetooth/bt_bottom.svg"), false);
  mBTArea.Swallow(mBtBottom.getPosition().x() - (sw / 170), mControllerIcon.getPosition().y() - (sh / 90), sw / 85, sw / 85);

  mBtMiddle.setOrigin(0.5, 0);
  mBtMiddle.setPosition(x, mControllerIcon.getPosition().y() - (sh / 51));
  mBtMiddle.setResize(sw / 53, sw / 53);
  mBtMiddle.setKeepRatio(true);
  mBtMiddle.setColor(menuTheme.Background().color);
  mBtMiddle.setImage(Path(":/bluetooth/bt_middle.svg"), false);
  mBTArea.Swallow(mBtMiddle.getPosition().x() - (sw / 106), mControllerIcon.getPosition().y() - (sh / 51), sw / 53, sw / 53);

  mBtTop.setOrigin(0.5, 0);
  mBtTop.setPosition(x, mControllerIcon.getPosition().y() - (sh / 36));
  mBtTop.setResize(sw / 40, sw / 40);
  mBtTop.setKeepRatio(true);
  mBtTop.setColor(menuTheme.Background().color);
  mBtTop.setImage(Path(":/bluetooth/bt_top.svg"), false);
  mBTArea.Swallow(mBtTop.getPosition().x() - (sw / 80), mControllerIcon.getPosition().y() - (sh / 36), sw / 40, sw / 40);

  // Outlined Progress bar
  mProgressBar = Rectangle(mControllerIcon.getPosition().x() + mControllerIcon.getTargetSize().x() + (sh / 100),
                           mControllerIcon.getPosition().y() + (mControllerIcon.getTargetSize().y() / 2) - ((4 + sh / 100) / 2),
                           15 + (sw / 12),
                           4 + (sh / 100));
  mBTArea.Swallow(mProgressBar);
}

void BluetoothOSD::Update(int deltaTime)
{
  if (mRemaining > 0)
    mRemaining -= deltaTime;

  int percent = ((int)SDL_GetTicks() / 7) % 100;
  mBtBottom.setOpacity((int)(2.55f * (float)(30 + ((90 + percent) % 100))));
  mBtMiddle.setOpacity((int)(2.55f * (float)(30 + ((60 + percent) % 100))));
  mBtTop.setOpacity((int)(2.55f * (float)(30 + ((30 + percent) % 100))));
}

void BluetoothOSD::Render(const Transform4x4f& parentTrans)
{
  if (mRemaining > 0)
  {
    mBtBottom.Render(parentTrans);
    mBtMiddle.Render(parentTrans);
    mBtTop.Render(parentTrans);
    mControllerIcon.Render(parentTrans);

    Renderer::SetMatrix(parentTrans);
    Rectangle outer(mProgressBar);
    Renderer::DrawRectangle(outer, mBarBackColor);
    Renderer::DrawRectangle(outer.Contract(1, 1), mBarForeColor);
    outer.Contract(1, 1);
    Rectangle inner(outer.Left(), outer.Top(), outer.Width() * ((float) mRemaining / (float) mTotal), outer.Height());
    Renderer::DrawRectangle(inner, mBarBackColor);
  }
}
