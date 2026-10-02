//
// Created by bkg2k on 30/08/24.
//

#include "RatingImage.h"
#include "Renderer.h"

void RatingImage::PrepareDrawingTo(float x, float y, float value)
{
  if (value == mLastValue && x == mLastX && y == mLastY) return;

  mLastValue = Math::clamp(value, 0.f, 1.f);
  mLastX = x;
  mLastY = y;

  const float h = (float)mStarHeight;
  const float w = Math::round(h * mLastValue * sStarCount);
  const float fw = (float)mStarLineWidth;

  mVertices[0].Target.Set(x, y);
  mVertices[0].Source.Set(0.0f, 1.0f);
  mVertices[1].Target.Set(x + w, y + h);
  mVertices[1].Source.Set(mLastValue * sStarCount, 0.0f);
  mVertices[2].Target.Set(x, y + h);
  mVertices[2].Source.Set(0.0f, 0.0f);

  mVertices[3] = mVertices[0];
  mVertices[4].Target.Set(x + w, y);
  mVertices[4].Source.Set(mLastValue * sStarCount, 1.0f);
  mVertices[5] = mVertices[1];

  mVertices[6] = mVertices[4];
  mVertices[7].Target.Set(x + fw, y + h);
  mVertices[7].Source.Set((float)sStarCount, 0.0f);
  mVertices[8] = mVertices[1];

  mVertices[9] = mVertices[6];
  mVertices[10].Target.Set(x + fw, y);
  mVertices[10].Source.Set((float)sStarCount, 1.0f);
  mVertices[11] = mVertices[7];
}

void RatingImage::Render(float x, float y, float value, Colors::ColorRGBA color)
{
  PrepareDrawingTo(x, y, value);
  if (mFilledStar.MakeReady())
    Renderer::DrawTexturedTriangles(mFilledStar.Identifier(), &mVertices[0], color, 6, true);
  if (mUnfilledStar.MakeReady())
    Renderer::DrawTexturedTriangles(mUnfilledStar.Identifier(), &mVertices[6], color, 6, true);
}
