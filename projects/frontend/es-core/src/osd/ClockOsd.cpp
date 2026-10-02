//
// Created by bkg2k on 14/10/24.
//

#include "ClockOsd.h"
#include <themes/ThemeManager.h>
#include <rendering/fonts/FontManager.h>
#include <hardware/Board.h>

ClockOSD::ClockOSD(WindowManager& window, Side side)
  : BaseOSD(window, side, false)
  , mClockFont(Renderer::Instance().Is480pOrLower() && Board::Instance().CrtBoard().IsCrtAdapterAttached()
               ? &FontManager::Instance().FromFile(Path(FONT_PATH_CRT), 7, true)
               : &FontManager::Instance().FromFile(Path(FONT_PATH_REGULAR), (FONT_SIZE_SMALL), false))
{
  Vector2f size = mClockFont->TextSize(String(' ').Append(Clock()).Append(' '));
  mClockArea = Rectangle(0, 0, (int)size.x(), (int)size.y());
}

String ClockOSD::Clock()
{
  time_t t = time(nullptr);
  char str[256]; strftime(str, sizeof(str), "%X", localtime(&t));
  return String(str);
}

void ClockOSD::Render(const Transform4x4f& parentTrans)
{
  (void)parentTrans;
  //Renderer::DrawRectangle(mClockArea, 0x000000C0);
  mClockFont->DrawText(Clock(), mClockArea, 0xFFFFFFFF, ::Alignment::Center);
}