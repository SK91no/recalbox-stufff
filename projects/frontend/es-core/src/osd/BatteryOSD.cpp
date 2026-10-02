//
// Created by bkg2k on 03/10/23.
//

#include "BatteryOSD.h"
#include <rendering/fonts/FontManager.h>
#include <hardware/Board.h>
#include <Renderer.h>

BatteryOSD::BatteryOSD(WindowManager& window, Side side)
  : BaseOSD(window, side, false)
  , mFont(&FontManager::Instance().FromDefault(Renderer::Instance().Is240p() ? FONT_SIZE_LARGE : FONT_SIZE_MEDIUM, false))
  , mBatteryArea(mFont->TextSize("\uf1b4"))
  , mIcon(0)
  , mColor(0)
  , mHasBattery(Board::Instance().HasBattery())
  , mVisible(!RecalboxConf::Instance().GetBatteryHidden())
{
  // Expand 1 pixel in all directions since the icon is drawn 1 pixel shifted in all directions
  mBatteryArea.Expand(1.f, 1.f);
  // Subscription is automatic through the IBatteryHiddenNotification base constructor.
}

void BatteryOSD::Update(int deltaTime)
{
  (void)deltaTime;

  mIcon = 0xf1b4;
  mColor = 0xFFFFFFFF;
  if (!Board::Instance().IsBatteryCharging())
  {
    // icon
    int charge = Board::Instance().BatteryChargePercent();
    if (charge >= 66)      mIcon = 0xF1ba;
    else if (charge >= 33) mIcon = 0xF1b8;
    else if (charge >= 15) mIcon = 0xF1b1;
    else                   mIcon = 0xF1b5;
    // color
    if (charge < 15)
    {
      mColor = 0xFF8000FF;
      if ((charge < 10) && ((SDL_GetTicks() >> 8) & 3) == 0) mColor = 0xFF0000FF;
    }
  }
}

void BatteryOSD::Render(const Transform4x4f& parentTrans)
{
  (void)parentTrans;
  mFont->DrawChar(mIcon, 2, 1, 0xFF, ::Alignment::TopLeft);
  mFont->DrawChar(mIcon, 0, 1, 0xFF, ::Alignment::TopLeft);
  mFont->DrawChar(mIcon, 1, 2, 0xFF, ::Alignment::TopLeft);
  mFont->DrawChar(mIcon, 1, 0, 0xFF, ::Alignment::TopLeft);
  mFont->DrawChar(mIcon, 1, 1, mColor, ::Alignment::TopLeft);
}
