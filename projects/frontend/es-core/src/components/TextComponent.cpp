#include "components/TextComponent.h"
#include "WindowManager.h"
#include "themes/ThemeData.h"
#include "utils/Files.h"
#include <rendering/fonts/FontManager.h>

TextComponent::TextComponent(WindowManager&window)
  : ThemableComponent(window)
  , mFont(&FontManager::Instance().FromDefault(FONT_SIZE_MEDIUM, false))
  , mColor(0x000000FF)
  , mOriginColor(0x000000FF)
  , mBgColor(0)
  , mLineSpacing(1.2f)
  , mColorOpacity(0xFF)
  , mBgColorOpacity(0)
  , mAlignment(::Alignment::CenterLeft)
  , mEffectiveAlignment(::Alignment::CenterLeft)
  , mRTL(false)
  , mUppercase(false)
  , mAutoCalcExtentX(true)
  , mAutoCalcExtentY(true)
  , mClipped(false)
  , mIsMultiline(false)
{
  mPosition = Vector3f::Zero();
  mSize = Vector2f::Zero();
}

TextComponent::TextComponent(WindowManager&window, const String& text, NewFont* font, unsigned int color)
  : TextComponent(window)
{
  mFont = font;
  mText = text;
  mColor = color;
  mColorOpacity = (unsigned char)(color & 0xFF);
  mOriginColor = color;
  onTextChanged();
}

TextComponent::TextComponent(WindowManager&window, const String& text, NewFont* font, unsigned int color, ::Alignment align)
  : TextComponent(window)
{
  mFont = font;
  mText = text;
  mColor = color;
  mColorOpacity = (unsigned char)(color & 0xFF);
  mOriginColor = color;
  mAlignment = align;
  onTextChanged();
}

TextComponent::TextComponent(WindowManager&window, const String& text, NewFont* font, unsigned int color, ::Alignment align,
                             Vector3f pos, Vector2f size, unsigned int bgcolor)
  : TextComponent(window, text, font, color, align)
{
  mBgColor = bgcolor;
  mBgColorOpacity = (unsigned char)(bgcolor & 0xFF);
  mPosition = pos;
  mSize = size;
  onTextChanged();
}

void TextComponent::onSizeChanged()
{
  mAutoCalcExtentX = (getSize().x() == 0);
  mAutoCalcExtentY = (getSize().y() == 0);
  onTextChanged();
}

void TextComponent::setFont(NewFont* font)
{
  if (mFont != font)
  {
    mFont = font;
    onTextChanged();
  }
}

void TextComponent::setColor(unsigned int color)
{
  if (color != mColor)
  {
    mColor = color;
    mColorOpacity = mColor & 0x000000FF;
  }
}

//  Set the color of the background box
void TextComponent::setBackgroundColor(unsigned int color)
{
  if (mBgColor != color)
  {
    mBgColor = color;
    mBgColorOpacity = mBgColor & 0x000000FF;
  }
}

//  Scale the opacity
void TextComponent::setOpacity(unsigned char opacity)
{
  if (opacity != mOpacity)
  {
    // This method is mostly called to do fading in-out of the Text component element.
    // Therefore, we assume here that opacity is a fractional value (expressed as an int 0-255),
    // of the opacity originally set with setColor() or setBackgroundColor().

    unsigned char o = (unsigned char) ((float) opacity / 255.f * (float) mColorOpacity);
    mColor = (mColor & 0xFFFFFF00) | o;

    unsigned char bgo = (unsigned char) ((float) opacity / 255.f * (float) mBgColorOpacity);
    mBgColor = (mBgColor & 0xFFFFFF00) | bgo;

    Component::setOpacity(opacity);
  }
}

void TextComponent::setText(const String& text)
{
  if (mText != text)
  {
    mText = text;
    onTextChanged();
  }
}

void TextComponent::setUppercase(bool uppercase)
{
  if (uppercase != mUppercase)
  {
    mUppercase = uppercase;
    onTextChanged();
  }
}

void TextComponent::Render(const Transform4x4f& parentTrans)
{
  if (mThemeDisabled) return;
  
  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  Rectangle area(0.f, 0.f, mSize.x(), mSize.y());
  //Renderer::DrawRectangle(area, 0x00000080);
  if ((mBgColor & 0xFF) != 0)
    Renderer::DrawRectangle(area, mBgColor);

  mChunks.Effect = mFontStyle;
  mFont->DrawText(mChunks, area, 0.f, 0.f, mColor, mEffectiveAlignment, mLineSpacing - 1.0f);
}

void TextComponent::calculateExtent()
{
  if (mAutoCalcExtentX)
    mSize = mFont->TextSize(mUppercase ? mText.ToUpperCaseUTF8() : mText);
  else if(mAutoCalcExtentY)
  {
    if (mIsMultiline)
      mSize.y() = (float)(mFont->SplitText(mUppercase ? mText.ToUpperCaseUTF8() : mText, (int) mSize.x()).Chunks.Count() - 1) * (float)mFont->Height() * mLineSpacing + (float)mFont->Height();
    else
      mSize.y() = (float)mFont->Height();
  }
}

void TextComponent::onTextChanged()
{
  String text = mText;
  if (mUppercase) text.UpperCaseUTF8();
  if (mFont == nullptr) return;

  mRTL = FontManager::Instance().IsArabicLocale() && ArabicShaper::StartsRTL(text);
  UpdateEffectiveAlignment();

  calculateExtent();

  bool addAbbrev = false;
  if (!mIsMultiline)
  {
    int newline = text.Find('\n');
    addAbbrev = newline >= 0;
    if (addAbbrev) text.Delete(newline, INT32_MAX); // single line of text - stop at the first newline since it'll mess everything up
  }

  Vector2f size = mFont->TextSize(text);
  if (!mIsMultiline && (mSize.x() != 0) && (size.x() > mSize.x() || addAbbrev))
    text = mFont->TextEllipsis(text, (int)mSize.x()); // abbreviate text
  mChunks = mFont->SplitText(text, (int)mSize.x());
}

void TextComponent::setLineSpacing(float spacing)
{
  mLineSpacing = spacing;
}

void TextComponent::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
  if (hasFlag(properties, ThemePropertyCategory::Color))
  {
    setColor(element.HasProperty(ThemePropertyName::Color) ? (unsigned int)element.AsInt(ThemePropertyName::Color) : mColor);
    setBackgroundColor(element.HasProperty(ThemePropertyName::BackgroundColor) ? (unsigned int)element.AsInt(ThemePropertyName::BackgroundColor) : mBgColor);
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

    setMultiline(element.HasProperty(ThemePropertyName::Multiline) ? element.AsBool(ThemePropertyName::Multiline) : mIsMultiline);
    setUppercase(element.HasProperty(ThemePropertyName::ForceUppercase) ? element.AsBool(ThemePropertyName::ForceUppercase) : mUppercase);
    mAlignment = element.HasProperty(ThemePropertyName::Alignment) ? element.AsAlignment(ThemePropertyName::Alignment) : Alignment::CenterLeft;
  }

  if (hasFlag(properties, ThemePropertyCategory::Font))
  {
    if (element.HasProperty(ThemePropertyName::FontPath)) setFont(&FontManager::Instance().FromTheme(element, properties, mFont));
    if (element.HasProperty(ThemePropertyName::FontStyle)) mFontStyle.Deserialize(element.AsString(ThemePropertyName::FontStyle));
    setLineSpacing(element.HasProperty(ThemePropertyName::LineSpacing) ? element.AsFloat(ThemePropertyName::LineSpacing) * 0.8f : mLineSpacing);
  }

  onTextChanged();
}
