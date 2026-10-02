//
// Created by bkg2k on 03/10/23.
//

#include <SDL_timer.h>
#include "FpsOSD.h"
#include <Renderer.h>
#include <rendering/fonts/FontManager.h>

#include "hardware/Board.h"

FpsOSD::FpsOSD(WindowManager& window, const Options& options, Side side)
  : BaseOSD(window, side, false)
  , mOptions(options)
  , mFPSFont(Renderer::Instance().Is480pOrLower() && Board::Instance().CrtBoard().IsCrtAdapterAttached()
             ? &FontManager::Instance().FromFile(Path(FONT_PATH_CRT), 7, true)
             : &FontManager::Instance().FromFile(Path(FONT_PATH_REGULAR), (FONT_SIZE_SMALL), false))
  , mFrameStart(0)
  , mFrameTimingComputations(0)
  , mFrameTimingTotal(0)
  , mTimingIndex(0)
  , mRecordedTimings(0)
{
  memset(mFrameStart, 0, sizeof(mFrameStart));
  memset(mFrameTimingComputations, 0, sizeof(mFrameTimingComputations));
  memset(mFrameTimingTotal, 0, sizeof(mFrameTimingTotal));

  Vector2f size = mFPSFont->TextSize(" 00.0 Fps (00.0%) ");
  mFPSArea = Rectangle(0.f, 0.f, size.x(), size.y());
}

void FpsOSD::RecordStopFrame()
{
  if (mTimingIndex == 0)
    if (mFrameStart[mTimingIndex] == 0) return;
  mFrameTimingComputations[mTimingIndex] = (int)SDL_GetTicks() - mFrameStart[mTimingIndex];
  mTimingIndex = (mTimingIndex + 1) & sMaxFrameTimingMask;
  if (++mRecordedTimings >= sMaxFrameTiming) mRecordedTimings = sMaxFrameTiming - 1;
}

void FpsOSD::RecordStartFrame()
{
  mFrameStart[mTimingIndex] = (int)SDL_GetTicks();
  if (mRecordedTimings != 0)
  {
    int previous = (mTimingIndex - 1) & sMaxFrameTimingMask;
    mFrameTimingTotal[previous] = mFrameStart[mTimingIndex] - mFrameStart[previous];
  }
}

float FpsOSD::CalculateFPS()
{
  float fps = 0;
  for(int i = mRecordedTimings; --i >= 0; )
    fps += (float)mFrameTimingTotal[(mTimingIndex - (i + 1)) & sMaxFrameTimingMask];
  fps /= (float)mRecordedTimings;
  return 1000.f / fps;
}

float FpsOSD::CalculateFramePercentage()
{
  float percent = 0;
  for(int i = mRecordedTimings; --i >= 0; )
  {
    int index = (mTimingIndex - (i + 1)) & sMaxFrameTimingMask;
    percent += (float)mFrameTimingComputations[index] /  (float)mFrameTimingTotal[index];
  }
  percent /= (float)mRecordedTimings;
  return percent * 100.f;
}

void FpsOSD::Render(const Transform4x4f& parentTrans)
{
  (void)parentTrans;
  float fps = CalculateFPS();
  float percent = CalculateFramePercentage();
  String s = (_F(" {0} Fps ({1}%) ") / _FOV(Frac, 1) / fps / percent).ToString();
  Renderer::DrawRectangle(mFPSArea, 0x000000C0);
  mFPSFont->DrawText(s, mFPSArea, 0xFFFFFFFF, ::Alignment::Center);
}

