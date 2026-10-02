//
// Created by bkg2k on 12/09/2020.
//
#pragma once

#include "utils/math/Vector2f.h"
#include "utils/math/Vector2i.h"
#include "utils/math/Misc.h"

class Rectangle
{
  public:
    Rectangle()
      : mX(0)
      , mY(0)
      , mW(0)
      , mH(0)
    {
    }

    explicit Rectangle(const Vector2f& size)
      : mX(0)
      , mY(0)
      , mW(size.x())
      , mH(size.y())
    {
    }

    Rectangle(float width, float height)
      : mX(0)
      , mY(0)
      , mW(width)
      , mH(height)
    {
    }

    Rectangle(const Vector2f& topleft, const Vector2f& widthheight)
      : mX(topleft.x())
      , mY(topleft.y())
      , mW(widthheight.x())
      , mH(widthheight.y())
    {
    }

    Rectangle(const Vector2i& topleft, const Vector2i& widthheight)
      : mX((float)topleft.x())
      , mY((float)topleft.y())
      , mW((float)widthheight.x())
      , mH((float)widthheight.y())
    {
    }

    Rectangle(float x, float y, float width, float height)
      : mX(x)
      , mY(y)
      , mW(width)
      , mH(height)
    {
    }

    Rectangle(int x, int y, int width, int height)
      : mX((float)x)
      , mY((float)y)
      , mW((float)width)
      , mH((float)height)
    {
    }

    [[nodiscard]] float Left()   const { return mX; }
    [[nodiscard]] float Top()    const { return mY; }
    [[nodiscard]] float Right()  const { return mX + mW; }
    [[nodiscard]] float Bottom() const { return mY + mH; }
    [[nodiscard]] float Width()  const { return mW; }
    [[nodiscard]] float Height() const { return mH; }

    Rectangle& SetLeft(float x)
    {
      mX = x;
      return *this;
    }

    Rectangle& SetTop(float y)
    {
      mY = y;
      return *this;
    }

    Rectangle& SetWidth(float w)
    {
      mW = w;
      return *this;
    }

    Rectangle& SetHeight(float h)
    {
      mH = h;
      return *this;
    }

    Rectangle& Set(float x, float y, float w, float h)
    {
      mX = x;
      mY = y;
      mW = w;
      mH = h;
      return *this;
    }

    Rectangle& SetCoordinates(float x, float y)
    {
      mX = x;
      mY = y;
      return *this;
    }

    Rectangle& SetSize(float w, float h)
    {
      mW = w;
      mH = h;
      return *this;
    }

    Rectangle SizeFactor(float r)
    {
      mW *= r;
      mH *= r;
      return *this;
    }

    Rectangle& Swallow(const Rectangle& other)
    {
      if (other.mX < mX) { mW += mX - other.mX; mX = other.mX; }
      if (other.mY < mY) { mH += mY - other.mY; mY = other.mX; }
      if (other.mX + other.mW > mX + mW) mW = other.mX + other.mW - mX;
      if (other.mY + other.mH > mY + mH) mW = other.mY + other.mH - mY;
      return *this;
    }

    Rectangle& Swallow(float x, float y, float w, float h)
    {
      if (x < mX) { mW += mX - x; mX = x; }
      if (y < mY) { mH += mY - y; mY = x; }
      if (x + w > mX + mW) mW = x + w - mX;
      if (y + h > mY + mH) mW = y + h - mY;
      return *this;
    }

    Rectangle& Contract(const Vector2f& by)
    {
      mX += by.x();
      mY += by.y();
      mW -= by.x() * 2;
      mH -= by.y() * 2;

      if (mW < 0) { mX += mW / 2; mW = 0; }
      if (mH < 0) { mY += mH / 2; mH = 0; }

      return *this;
    }

    Rectangle& Expand(const Vector2f& by)
    {
      mX -= by.x();
      mY -= by.y();
      mW += by.x() * 2;
      mH += by.y() * 2;

      if (mW < 0) { mX += mW / 2; mW = 0; }
      if (mH < 0) { mY += mH / 2; mH = 0; }

      return *this;
    }

    Rectangle& Contract(float deltaX, float deltaY)
    {
      mX += deltaX;
      mY += deltaY;
      mW -= deltaX * 2;
      mH -= deltaY * 2;

      if (mW < 0) { mX += mW / 2; mW = 0; }
      if (mH < 0) { mY += mH / 2; mH = 0; }

      return *this;
    }

    Rectangle& Expand(float deltaX, float deltaY)
    {
      mX -= deltaX;
      mY -= deltaY;
      mW += deltaX * 2;
      mH += deltaY * 2;

      if (mW < 0) { mX += mW / 2; mW = 0; }
      if (mH < 0) { mY += mH / 2; mH = 0; }

      return *this;
    }

    Rectangle& Move(const Vector2f& by)
    {
      mX += by.x();
      mY += by.y();
      return *this;
    }

    Rectangle& Move(float deltaX, float deltaY)
    {
      mX += deltaX;
      mY += deltaY;
      return *this;
    }

    Rectangle& MoveLeft(float delta)
    {
      mX += delta;
      if (mW -= delta; mW < 0.0f) mW = 0.0f;
      return *this;
    }

    Rectangle& MoveTop(float delta)
    {
      mY += delta;
      if (mH -= delta; mH < 0.0f) mH = 0.0f;
      return *this;
    }

    Rectangle& MoveRight(float delta)
    {
      if (mW += delta; mW < 0.0f) mW = 0.0f;
      return *this;
    }

    Rectangle& MoveBottom(float delta)
    {
      if (mH += delta; mH < 0.0f) mH = 0.0f;
      return *this;
    }

    Rectangle& ResetReference()
    {
      mX = mY = 0;
      return *this;
    }

    Rectangle& Reset()
    {
      mX = mY = mW = mH = 0;
      return *this;
    }

    bool operator ==(const Rectangle& r) const
    {
      return ((mX == r.mX) && (mY == r.mY) && (mW == r.mW) && (mH == r.mH));
    }

    bool operator !=(const Rectangle& r) const
    {
      return ((mX != r.mX) || (mY != r.mY) || (mW != r.mW) || (mH != r.mH));
    }

    Rectangle& FlipOnScreen(int screenHeight)
    {
      mY = (float)screenHeight - mY - mH;
      return *this;
    }

    Rectangle& Intersect(const Rectangle& with)
    {
      if (mX < with.mX) { mW -= with.mX - mX; mX = with.mX; if (mW < 0) mW = 0; }
      if (mX + mW > with.mX + with.mW) { mW = with.mX + with.mW - mX; if (mW < 0) mW = 0; }
      if (mY < with.mY) { mH -= with.mY - mY; mY = with.mY; if (mH < 0) mH = 0; }
      if (mY + mH > with.mY + with.mH) { mH = with.mY + with.mH - mY; if (mH < 0) mH = 0; }
      return *this;
    }

    [[nodiscard]] bool IsIn(float x, float y) const
    {
      return x >= mX && x < (mX + mW) && y >= mY && y < (mY + mH);
    }

    Rectangle& Scale(float xratio, float yratio)
    {
      mX *= xratio;
      mY *= yratio;
      mW *= xratio;
      mH *= yratio;
      return *this;
    }

    Rectangle& Scale(float xratio, float yratio, float xreference, float yreference)
    {
      float xR = mX + mW * xreference;
      float yR = mY + mH * yreference;
      mX = xR - ((xR - mX) * xratio);
      mY = yR - ((yR - mY) * yratio);
      mW *= xratio;
      mH *= yratio;
      return *this;
    }

    Rectangle& Validate()
    {
      if (mW < 0) mW = 0;
      if (mH < 0) mH = 0;
      return *this;
    }

    Rectangle& Round()
    {
      float r = Math::round(mX + mW);
      mX = Math::round(mX);
      mW = r - mX;
      float b = Math::round(mY + mH);
      mY = Math::round(mY);
      mH = b - mY;
      return *this;
    }

    Rectangle& Ceil()
    {
      float r = Math::ceil(mX + mW);
      mX = Math::ceil(mX);
      mW = r - mX;
      float b = Math::ceil(mY + mH);
      mY = Math::ceil(mY);
      mH = b - mY;
      return *this;
    }

    [[nodiscard]] bool IsValid() const { return mW > 0 && mH > 0; }

    [[nodiscard]] float XCenter() const { return mX + mW / 2.f; }

    [[nodiscard]] float YCenter() const { return mY + mH / 2.f; }

    /*!
     * @brief Zoom all the rectangle's coordinate and size
     * @param multiplier Zoom coefficient
     * @return
     */
    Rectangle& ZoomAbsolute(float multiplier)
    {
      mX *= multiplier;
      mY *= multiplier;
      mW *= multiplier;
      mH *= multiplier;
      return *this;
    }

    /*!
     * @brief Zoop the rectangle, around the point defined by xRatio and yRatio, in rectangle ratio
     * @param multiplier Zoom coefficient
     * @param xRatio Width ratio
     * @param yRatio Height ratio
     * @return
     */
    Rectangle& ZoomRelative(float multiplier, float xRatio, float yRatio)
    {
      float xCenter = mW * xRatio;
      float yCenter = mH * yRatio;
      mX = (mX + xCenter) - (xCenter * multiplier);
      mY = (mY + yCenter) - (yCenter * multiplier);
      mW *= multiplier;
      mH *= multiplier;
      return *this;
    }

  private:
    float mX;
    float mY;
    float mW;
    float mH;
};
