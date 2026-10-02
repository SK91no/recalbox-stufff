//
// Created by bkg2k on 24/05/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "MarkdownText.h"
#include <rendering/fonts/FontManager.h>
#include <utils/markdown/MarkdownParser.h>
#include "rendering/opengl/Primitives.h"
#include "utils/Files.h"

String MarkdownText::sBullet("  \u26ab ");

MarkdownText& MarkdownText::SetText(const String& text)
{
  if (mText != text)
  {
    mText = text;
    RebuildRequired();
    mScrollStep = ScrollSteps::LeftPause; // Reset scroll if any
    mScrollOffset = mScrollTime = 0;
  }
  return *this;
}

void MarkdownText::Render(const Transform4x4f& parentTrans)
{
  if (mThemeDisabled) return;

  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  Rectangle outer(0.f, 0.f, mSize.x(), mSize.y());
  if ((mBackgroundColor & 0xFF) != 0)
  {
    Renderer::SetMatrix(trans);
    Renderer::DrawRectangle(outer, mBackgroundColor);
  }

  if (mFontHolder.IsValid()) // Do nothing until fonts are initialized
  {
    Rectangle inner = Rectangle(outer).Contract(mPadding, mPadding);
    inner.Round();
    Renderer::Instance().Clip(inner);

    // Vertical offset
    int offset = 0;
    switch(AlignmentExtractVertical(mAlignment))
    {
      case VerticalAlignment::Center: offset = ((int)mSize.y() - mTextHeight) / 2; break;
      case VerticalAlignment::Bottom: offset = (int)mSize.y() - mTextHeight + mScrollOffset; break;
      case VerticalAlignment::Top: offset = -mScrollOffset; break;
      default: break;
    }

    RenderMarkdown(inner, offset);
    if (mScrollOffset != 0)
      switch(AlignmentExtractVertical(mAlignment))
      {
        case VerticalAlignment::Bottom: RenderMarkdown(inner, offset - (mTextHeight + ((int)mSize.y() >> 2))); break;
        case VerticalAlignment::Top: RenderMarkdown(inner, offset + (mTextHeight + ((int)mSize.y() >> 2))); break;
        case VerticalAlignment::Center: // Do not autoscroll w/ vertical center alignment
        default: break;
      }

    Renderer::Instance().Unclip();
  }
}

void MarkdownText::RenderMarkdown(const Rectangle& inner, int offset)
{
  FontEffect effect;
  Colors::ColorARGB color = mColor;
  float thickness = Renderer::Instance().Is240p() ? 1.f : 2.f;
  for(int i = 0; i < mChunks.Count(); ++i)
  {
    const Chunk& chunk = mChunks(i);
    effect.Reset();
    effect.SetBold(hasFlag(chunk.Draw, DrawingProperties::FontBold));
    effect.SetItalic(hasFlag(chunk.Draw, DrawingProperties::FontItalic));

    // Text
    if ((float)(chunk.YOffset + offset) <= inner.Bottom() && (float)(chunk.YOffset + offset + chunk.Font->Height()) >= inner.Top())
    {
      if (hasFlag(chunk.Draw, DrawingProperties::Bullet))
        chunk.Font->DrawText(sBullet, inner, (float) chunk.XOffset, (float) (chunk.YOffset + offset), color, Alignment::TopLeft, effect);
      else
        chunk.Font->DrawText(mDisplayedText, chunk.OffsetInText, chunk.Length, inner, (float) chunk.XOffset, (float) (chunk.YOffset + offset), color, Alignment::TopLeft, effect);

      // Stroke?
      if (hasFlag(chunk.Draw, DrawingProperties::FontStrikeThru))
      {
        float Y = inner.Top() + (float)(chunk.YOffset + (chunk.Font->Height() >> 1)) + (float)offset;
        Renderer::DrawRectangle(inner.Left() + (float)chunk.XOffset, Y, (float)chunk.Width, thickness, color);
      }

      // Underline
      if (hasFlag(chunk.Draw, DrawingProperties::FontUnderline))
      {
        float Y = inner.Top() + (float)(chunk.YOffset + (chunk.Font->TopToBaseline() + 2)) + (float)offset;
        Renderer::DrawRectangle(inner.Left() + (float)chunk.XOffset, Y, (float)chunk.Width, thickness, color);
      }
    }

    // Separator?
    if (hasFlag(chunk.Draw, DrawingProperties::Separator))
    {
      float Y = inner.Top() + (float)(chunk.YOffset + chunk.Font->Height() + (mFontHolder.Default().Height() >> 1)) + (float)offset;
      if (Y <= inner.Bottom() && Y >= inner.Top())
        Renderer::DrawRectangle(inner.Left(), Y, inner.Width(), thickness, color);
    }
  }

}

void MarkdownText::RebuildRequired()
{
  mDisplayedText = mText;
  if (mUppercase) mDisplayedText.UpperCaseUTF8();

  MarkdownParser parser;
  parser.Parse(mDisplayedText);

  mChunks.Clear();
  mTextWidth = 0;
  const int paddedWidth = (int)(mSize.x() - (2.f * mPadding));
  int YOffset = 0;
  int XOffset = 0;
  int bulletOffset = 0;
  int fontHeight = 0;
  FontEffect effect;
  for(int i = 0; i < parser.ItemCount(); ++i)
  {
    // Get item
    const MarkdownItem& item = parser.Item(i);
    // Prepare effect
    effect.Reset();
    effect.SetBold(item.IsBold());
    effect.SetItalic(item.IsItalic());
    // Get font
    NewFont& font = mFontHolder.GetFrom(item);
    fontHeight = (int)((float)font.Height() * (mInterlineRatio + 1.f));
    // Bullets?
    if (item.HasBullet() || item.HasNumbered())
    {
      bulletOffset = font.TextWidth(sBullet);
      mChunks.Add({ .Font=&font, .XOffsetOrigin=XOffset, .XOffset=XOffset, .YOffset=YOffset, .Width=bulletOffset, .OffsetInText=0, .Length=0, .Number=0, .Draw=DrawingProperties::Bullet });
      XOffset += bulletOffset;
      if (XOffset >= mTextWidth) mTextWidth = XOffset;
    }
    // Check room
    int dummy = item.TextStartIndex();
    String::Unicode unicode = mDisplayedText.ReadUTF8(dummy);
    int firstCharWidth = font.TextWidth(String(unicode), effect);
    if (firstCharWidth >= paddedWidth - XOffset) { XOffset = bulletOffset; YOffset += fontHeight; }
    // Split
    NewFont::TextChunkList list = font.SplitText(mDisplayedText.data(),
                                                 item.TextStartIndex(),
                                                 item.TextEndIndex() - item.TextStartIndex() + 1,
                                                 XOffset,
                                                 paddedWidth,
                                                 effect);
    // Check if the first chunk is out of bound. If so, insert a new line & repeat the split
    if (!list.Chunks.Empty() && list.Chunks[0].Width + XOffset > paddedWidth)
    {
      XOffset = bulletOffset;
      YOffset += fontHeight;
      list = font.SplitText(mDisplayedText.data(),
                            item.TextStartIndex(),
                            item.TextEndIndex() - item.TextStartIndex() + 1,
                            0,
                            paddedWidth,
                            effect);
    }
    // Record chunks
    bool split = list.Chunks.Count() > 1;
    int lastChunk = list.Chunks.Count() - 1;
    for(int c = 0; c < list.Chunks.Count(); ++c)
    {
      const NewFont::TextChunk& chunk = list.Chunks[c];
      // Add drawing chunk
      DrawingProperties properties = (item.IsBold() ? DrawingProperties::FontBold : DrawingProperties::None) |
                                     (item.IsItalic() ? DrawingProperties::FontItalic : DrawingProperties::None) |
                                     (item.IsStrikeTrhu() ? DrawingProperties::FontStrikeThru : DrawingProperties::None) |
                                     (item.IsUnderline() ? DrawingProperties::FontUnderline : DrawingProperties::None) |
                                     (item.HasHorizontalRule() ? DrawingProperties::Separator : DrawingProperties::None);
      mChunks.Add({ .Font=&font, .XOffsetOrigin=XOffset, .XOffset=XOffset, .YOffset=YOffset, .Width=chunk.Width, .OffsetInText=chunk.Start, .Length=chunk.Length, .Number=(unsigned char)item.Number(), .Draw=properties });
      // Advance of next chunk
      XOffset += chunk.Width;
      if (XOffset >= mTextWidth) mTextWidth = XOffset;
      if (split && c != lastChunk)  { XOffset = bulletOffset; YOffset += fontHeight; }
    }

    // New line?
    if (item.HasNewLine()) { XOffset = bulletOffset = 0; YOffset += fontHeight; }
    // Empty line?
    if (item.HasEmptyLine()) YOffset += mFontHolder.Default().Height();
    // Separator?
    if (item.HasHorizontalRule()) YOffset += mFontHolder.Default().Height();
  }

  mRTL = FontManager::Instance().IsArabicLocale() && ArabicShaper::StartsRTL(mDisplayedText);
  UpdateEffectiveAlignment();
  if (AlignmentExtractHorizontal(mEffectiveAlignment) != HorizontalAlignment::Left)
    RealignChunks();

  // Get total text height
  mTextHeight = YOffset + fontHeight;
}

void MarkdownText::RealignChunks()
{
  for(int start = 0; start < mChunks.Count(); )
  {
    // Find last chunk on the same line
    int y = mChunks[start].YOffset;
    int end = start + 1;
    while(end < mChunks.Count() && mChunks[end].YOffset == y) ++end;

    // Get line width
    int width = 0;
    for(int i = end; --i >= start; ) width += mChunks[i].Width;

    // Get offset
    int offset = 0;
    int componentWidth =(int)(mSize.x() - (2 * mPadding));
    switch(AlignmentExtractHorizontal(mEffectiveAlignment))
    {
      case HorizontalAlignment::Center: offset = (componentWidth - width) / 2; break;
      case HorizontalAlignment::Right: offset = componentWidth - width; break;
      case HorizontalAlignment::Left:
      default: break;
    }

    // Realign
    for(int i = end; --i >= start; ) mChunks(i).XOffset = mChunks(i).XOffsetOrigin + offset;

    // Next line
    start = end;
  }
}

void MarkdownText::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
  if (hasFlag(properties, ThemePropertyCategory::Color))
  {
    setColor(element.HasProperty(ThemePropertyName::Color) ? (unsigned int)element.AsInt(ThemePropertyName::Color) : mColor);
    setBackgroundColor(element.HasProperty(ThemePropertyName::BackgroundColor) ? (unsigned int)element.AsInt(ThemePropertyName::BackgroundColor) : mBackgroundColor);
  }

  if (hasFlag(properties, ThemePropertyCategory::Text | ThemePropertyCategory::Path))
  {
    if      (element.HasProperty(ThemePropertyName::Path))
    {
      String text = Files::LoadFile(element.AsPath(ThemePropertyName::Path));
      if (text.empty() && element.HasProperty(ThemePropertyName::Text)) text = element.AsString(ThemePropertyName::Text);
      SetText(text);
    }
    else if (element.HasProperty(ThemePropertyName::Text)) SetText(element.AsString(ThemePropertyName::Text));
    else                                                   SetText(String::Empty);
    if (element.HasProperty(ThemePropertyName::Alignment)) mAlignment = element.AsAlignment(ThemePropertyName::Alignment);
    else mAlignment = Alignment::TopLeft;
    UpdateEffectiveAlignment();
    RealignChunks();
    setUppercase(element.HasProperty(ThemePropertyName::ForceUppercase) ? element.AsBool(ThemePropertyName::ForceUppercase) : mUppercase);
  }

  if (hasFlag(properties, ThemePropertyCategory::Font))
  {
    mInterlineRatio = Math::clamp(element.HasProperty(ThemePropertyName::LineSpacing) ? element.AsFloat(ThemePropertyName::LineSpacing) - 1.0f : mInterlineRatio, 0.f, 5.f);
    if (element.HasProperty(ThemePropertyName::FontPath))
      GetAllFontsFrom(element.AsPath(ThemePropertyName::FontPath), Path::Empty,
                      element.HasProperty(ThemePropertyName::FontSize) ? element.AsFloat(ThemePropertyName::FontSize) : 0.f); // No fixed font in theme for now
  }
  RebuildRequired();
}

void MarkdownText::GetAllFontsFrom(const Path& baseFontFile, const Path& fixedFontFile, float size)
{
  FontManager& font = FontManager::Instance();
  if (size != 0.f)
    mFontHolder = MarkdownFontHolder(font.FromTemplateResized(baseFontFile, FontTemplate::Tiny, size),
                                     font.FromTemplateResized(baseFontFile, FontTemplate::Huge, size),
                                     font.FromTemplateResized(baseFontFile, FontTemplate::Large, size),
                                     font.FromTemplateResized(baseFontFile, FontTemplate::Medium, size),
                                     font.FromTemplateResized(baseFontFile, FontTemplate::Small, size),
                                     font.FromTemplateResized(fixedFontFile, FontTemplate::SmallFixed, size));
  else  
    mFontHolder = MarkdownFontHolder(font.FromTemplate(baseFontFile, FontTemplate::Tiny),
                                     font.FromTemplate(baseFontFile, FontTemplate::Huge),
                                     font.FromTemplate(baseFontFile, FontTemplate::Large),
                                     font.FromTemplate(baseFontFile, FontTemplate::Medium),
                                     font.FromTemplate(baseFontFile, FontTemplate::Small),
                                     font.FromTemplate(fixedFontFile, FontTemplate::SmallFixed));
}

void MarkdownText::onSizeChanged()
{
  RebuildRequired();
  if (mSize.y() == 0) mSize.y() = (float)mTextHeight;
}

bool MarkdownText::ProcessInput(const InputCompactEvent& event)
{
  if (mThemeDisabled) return false;

  if (mTextHeight > (int)mSize.y())
  {
    if (event.J2DownPressed()) { mLastDirection = 1; mAutoScroll = false; return true; }
    if (event.J2UpPressed()) { mLastDirection = -1; mAutoScroll = false; return true; }
    if (event.J2DownReleased()) { mLastDirection = 0; mManualScrollingTimer = sManualScrollInactivity; return true; }
    if (event.J2UpReleased()) { mLastDirection = 0; mManualScrollingTimer = sManualScrollInactivity;  return true; }
  }

  if (mUpDownScrolling)
  {
    if (event.DownPressed()) { mLastDirection = 1; mAutoScroll = false; return true; }
    if (event.UpPressed()) { mLastDirection = -1; mAutoScroll = false; return true; }
    if (event.DownReleased()) { mLastDirection = 0; mManualScrollingTimer = sManualScrollInactivity; return true; }
    if (event.UpReleased()) { mLastDirection = 0; mManualScrollingTimer = sManualScrollInactivity;  return true; }
  }

  return Component::ProcessInput(event);
}

void MarkdownText::Update(int deltaTime)
{
  if (mThemeDisabled) return;

  int boxHeight = (int)mSize.y();

  if (mManualScrollingTimer > 0)
    if (mManualScrollingTimer -= deltaTime; mManualScrollingTimer <= 0)
    {
      mManualScrollingTimer = 0;
      mAutoScroll = mOriginalAutoScroll;
      mScrollStep = ScrollSteps::ScrollToRight;
      mScrollTime = (mScrollOffset * 1000) / sScrollSpeed1;
    }

  if (!mAutoScroll && mLastDirection != 0)
  {
    mScrollOffset += mLastDirection * (Renderer::Instance().DisplayHeightAsInt() / 100);
    if (mScrollOffset < 0) mScrollOffset = 0;
    if (mScrollOffset > mTextHeight - boxHeight) mScrollOffset = mTextHeight - boxHeight;
  }
  else if (mTextHeight > boxHeight && mManualScrollingTimer <= 0)
  {
    switch (mScrollStep)
    {
      case ScrollSteps::LeftPause:
      {
        mScrollOffset = 0;
        if (mScrollTime > sScrollPause) { mScrollTime = 0; mScrollStep = ScrollSteps::ScrollToRight; }
        break;
      }
      case ScrollSteps::ScrollToRight:
      {
        mScrollOffset = (mScrollTime * sScrollSpeed1) / 1000;
        if (mScrollOffset >= (mTextHeight - boxHeight)) { mScrollTime = 0; mScrollOffset = (mTextHeight - boxHeight); mScrollStep = ScrollSteps::RightPause;}
        break;
      }
      case ScrollSteps::RightPause:
      {
        mScrollOffset = (mTextHeight - boxHeight);
        if (mScrollTime > sScrollPause) { mScrollTime = 0; mScrollStep = ScrollSteps::RollOver; }
        break;
      }
      case ScrollSteps::RollOver:
      {
        mScrollOffset = (mTextHeight - boxHeight) + ((mScrollTime * sScrollSpeed2) / 1000);
        if (mScrollOffset >= mTextHeight + (boxHeight / 4)) { mScrollTime = 0; mScrollOffset = 0; mScrollStep = ScrollSteps::LeftPause;}
        break;
      }
      default: break;
    }
    mScrollTime += deltaTime;
  }

  Component::Update(deltaTime);
}

//  Scale the opacity
void MarkdownText::setOpacity(unsigned char opacity)
{
  if (opacity != mOpacity)
  {
    // This method is mostly called to do fading in-out of the Text component element.
    // Therefore, we assume here that opacity is a fractional value (expressed as an int 0-255),
    // of the opacity originally set with setColor() or setBackgroundColor().
    unsigned char o = (unsigned char) ((float) opacity / 255.f * (float) mColorOpacity);
    mColor = (mColor & 0xFFFFFF00) | o;
    o = (unsigned char) ((float) opacity / 255.f * (float) mBackgroundColorOpacity);
    mBackgroundColor = (mBackgroundColor & 0xFFFFFF00) | o;

    Component::setOpacity(opacity);
  }
}
