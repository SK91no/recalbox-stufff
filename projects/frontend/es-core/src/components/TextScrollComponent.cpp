//
// Created by bkg2k on 04/12/23.
//

#include "TextScrollComponent.h"

#include "Renderer.h"
#include "utils/Log.h"
#include "WindowManager.h"
#include "themes/ThemeData.h"
#include "utils/Files.h"
#include <rendering/fonts/FontManager.h>

TextScrollComponent::TextScrollComponent(WindowManager&window)
  : ThemableComponent(window)
  , mFont(&FontManager::Instance().FromDefault(FONT_SIZE_MEDIUM, false))
  , mStep(ScrollSteps::LeftPause)
  , mMarqueeTime(0)
  , mOffset(0)
  , mTextWidth(0)
  , mTextHeight(0)
  , mColor(0x000000FF)
  , mOriginColor(0x000000FF)
  , mBgColor(0)
  , mColorOpacity(0xFF)
  , mBgColorOpacity(0)
  , mAlignment(::Alignment::CenterLeft)
  , mEffectiveAlignment(::Alignment::CenterLeft)
  , mRTL(false)
  , mRenderBackground(false)
  , mUppercase(false)
{
  mPosition = Vector3f::Zero();
  mSize = Vector2f::Zero();
}

TextScrollComponent::TextScrollComponent(WindowManager&window, const String& text, NewFont* font, unsigned int color)
  : TextScrollComponent(window)
{
  mFont = font;
  mText = text;
  mColor = color;
  mColorOpacity = (unsigned char)(color & 0xFF);
  mOriginColor = color;
  onTextChanged();
}

TextScrollComponent::TextScrollComponent(WindowManager&window, const String& text, NewFont* font, unsigned int color, ::Alignment align)
  : TextScrollComponent(window)
{
  mFont = font;
  mText = text;
  mColor = color;
  mColorOpacity = (unsigned char)(color & 0xFF);
  mOriginColor = color;
  mAlignment = align;
  onTextChanged();
}

TextScrollComponent::TextScrollComponent(WindowManager&window, const String& text, NewFont* font, unsigned int color, ::Alignment align,
                             Vector3f pos, Vector2f size, unsigned int bgcolor)
  : TextScrollComponent(window, text, font, color, align)
{
  mBgColor = bgcolor;
  mBgColorOpacity = (unsigned char)(bgcolor & 0xFF);
  mPosition = pos;
  mSize = size;
}

void TextScrollComponent::setFont(NewFont* font)
{
  if (font != mFont)
  {
    mFont = font;
    onTextChanged();
  }
}

void TextScrollComponent::setColor(unsigned int color)
{
  if (mColor != color)
  {
    mColor = color;
    mColorOpacity = mColor & 0x000000FF;
  }
}

//  Set the color of the background box
void TextScrollComponent::setBackgroundColor(unsigned int color)
{
  if (mBgColor != color)
  {
    mBgColor = color;
    mBgColorOpacity = mBgColor & 0x000000FF;
  }
}

//  Scale the opacity
void TextScrollComponent::setOpacity(unsigned char opacity)
{
  // This method is mostly called to do fading in-out of the Text component element.
  // Therefore, we assume here that opacity is a fractional value (expressed as an int 0-255),
  // of the opacity originally set with setColor() or setBackgroundColor().

  unsigned char o = (unsigned char)((float)opacity / 255.f * (float) mColorOpacity);
  mColor = (mColor & 0xFFFFFF00) | o;

  unsigned char bgo = (unsigned char)((float)opacity / 255.f * (float)mBgColorOpacity);
  mBgColor = (mBgColor & 0xFFFFFF00) | bgo;

  Component::setOpacity(opacity);
}

void TextScrollComponent::setText(const String& text)
{
  if (mText != text)
  {
    mText = text;
    onTextChanged();
  }
}

void TextScrollComponent::setUppercase(bool uppercase)
{
  if (uppercase != mUppercase)
  {
    mUppercase = uppercase;
    onTextChanged();
  }
}

void TextScrollComponent::Update(int deltaTime)
{
  mOffset = 0;
  if ((float)mTextWidth > mSize.x())
  {
    int width = (int)mSize.x();
    if (mTextWidth > width)
    {
      switch (mStep)
      {
        case ScrollSteps::LeftPause:
        {
          mOffset = 0;
          if (mMarqueeTime > sScrollPause) { mMarqueeTime = 0; mStep = ScrollSteps::ScrollToRight; }
          break;
        }
        case ScrollSteps::ScrollToRight:
        {
          mOffset = (mMarqueeTime * sScrollSpeed1) / 1000;
          if (mOffset >= mTextWidth - width) { mMarqueeTime = 0; mOffset = mTextWidth - width; mStep = ScrollSteps::RightPause;}
          break;
        }
        case ScrollSteps::RightPause:
        {
          mOffset = mTextWidth - width;
          if (mMarqueeTime > sScrollPause) { mMarqueeTime = 0; mStep = ScrollSteps::RollOver; }
          break;
        }
        case ScrollSteps::RollOver:
        {
          mOffset = (mTextWidth - width) + ((mMarqueeTime * sScrollSpeed2) / 1000);
          if (mOffset >= mTextWidth + ((int)mSize.x() / 4)) { mMarqueeTime = 0; mOffset = 0; mStep = ScrollSteps::LeftPause;}
          break;
        }
        default: break;
      }       
      mMarqueeTime += deltaTime;
    }
  }

  Component::Update(deltaTime);
}

void TextScrollComponent::Render(const Transform4x4f& parentTrans)
{
  if (mThemeDisabled) return;

  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  if (mRenderBackground)
    Renderer::DrawRectangle(0.f, 0.f, mSize.x(), mSize.y(), mBgColor);

  if (mText.empty()) return;

  float xOff = 0;
  float yOff = 0;
  switch(AlignmentExtractVertical(mEffectiveAlignment))
  {
    case ::VerticalAlignment::Center: yOff = (getSize().y() - (float)mFont->Height()) / 2.0f; break;
    case ::VerticalAlignment::Bottom: yOff = (getSize().y() - (float)mFont->Height()); break;
    case ::VerticalAlignment::Top:
    default: break;
  }
  switch(AlignmentExtractHorizontal(mEffectiveAlignment))
  {
    case HorizontalAlignment::Center: if ((float)mTextWidth <= mSize.x()) xOff = (getSize().x() - (float)mTextWidth) / 2.0f; break;
    case HorizontalAlignment::Right: xOff = (getSize().x() - (float)mTextWidth); break;
    case HorizontalAlignment::Left:
    default: break;
  }
  Vector3f off(xOff + (float)((AlignmentExtractHorizontal(mEffectiveAlignment) == HorizontalAlignment::Right) ? mOffset : - mOffset), yOff, 0);

  // Get clipping area
  Renderer::Instance().Clip(trans, mSize);

  // Render text
  trans.translate(off);
  trans.round();
  Renderer::SetMatrix(trans);
  Rectangle area(0.f, 0.f, Renderer::Instance().DisplayWidthAsFloat(), mSize.y());
  mFont->DrawText(mDisplayableText, area, mColor, ::Alignment::TopLeft, mFontStyle);
  if (mOffset != 0)
  {
    float subOffset = (float)mTextWidth + (mSize.x() / 4.f);
    if (AlignmentExtractHorizontal(mEffectiveAlignment) == ::HorizontalAlignment::Right) subOffset = -subOffset;
    trans.translate(subOffset, 0);
    trans.round();
    Renderer::SetMatrix(trans);
    mFont->DrawText(mDisplayableText, area, mColor, ::Alignment::TopLeft, mFontStyle);
  }

  Renderer::Instance().Unclip();
}

void TextScrollComponent::onTextChanged()
{
  mDisplayableText = mText;
  if (mUppercase) mDisplayableText.UpperCaseUTF8();

  mRTL = FontManager::Instance().IsArabicLocale() && ArabicShaper::StartsRTL(mDisplayableText);
  UpdateEffectiveAlignment();

  mTextWidth = mFont->TextWidth(mDisplayableText);
  mTextHeight = mFont->Height();
  mStep = ScrollSteps::LeftPause;
  mMarqueeTime = 0;
  onSizeChanged();
}

void TextScrollComponent::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
  if (hasFlag(properties, ThemePropertyCategory::Color))
  {
    setColor(element.HasProperty(ThemePropertyName::Color) ? (unsigned int)element.AsInt(ThemePropertyName::Color) : mColor);
    bool hasProp = element.HasProperty(ThemePropertyName::BackgroundColor);
    setBackgroundColor(hasProp ? (unsigned int)element.AsInt(ThemePropertyName::BackgroundColor) : mBgColor);
    setRenderBackground(hasProp);
  }

  if (hasFlag(properties, ThemePropertyCategory::Text | ThemePropertyCategory::Path))
  {
    if      (element.HasProperty(ThemePropertyName::Path))
    {
      String text = Files::LoadFile(element.AsPath(ThemePropertyName::Path));
      if (text.empty() && element.HasProperty(ThemePropertyName::Text)) text = element.AsString(ThemePropertyName::Text);
      setText(text);
    }
    else if (element.HasProperty(ThemePropertyName::Text)) setText(element.AsString(ThemePropertyName::Text));
    else                                                   setText(String::Empty);

    mAlignment = element.HasProperty(ThemePropertyName::Alignment) ? element.AsAlignment(ThemePropertyName::Alignment) : Alignment::CenterLeft;
    setUppercase(element.HasProperty(ThemePropertyName::ForceUppercase) ? element.AsBool(ThemePropertyName::ForceUppercase) : mUppercase);
  }

  if (hasFlag(properties, ThemePropertyCategory::Font))
  {
    if (element.HasProperty(ThemePropertyName::FontPath)) setFont(&FontManager::Instance().FromTheme(element, properties, mFont));
    if (element.HasProperty(ThemePropertyName::FontStyle)) mFontStyle.Deserialize(element.AsString(ThemePropertyName::FontStyle));
  }
}

void TextScrollComponent::onSizeChanged()
{
  Component::onSizeChanged();
  if ((int)mSize.y() < mFont->Height())
    switch(AlignmentExtractVertical(mEffectiveAlignment))
    {
      case VerticalAlignment::Top: mSize.y() = (float)mFont->Height(); break;
      case VerticalAlignment::Center: mPosition.y() -= ((float)mFont->Height() - mSize.y()) / 2; mPosition.round(); mSize.y() = (float)mFont->Height(); break;
      case VerticalAlignment::Bottom: mPosition.y() -= ((float)mFont->Height() - mSize.y()); mPosition.round(); mSize.y() = (float)mFont->Height(); break;
    }
}
