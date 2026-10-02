#pragma once

#include <components/IList.h>
#include <components/ITextListComponentOverlay.h>
#include <utils/String.h>
#include <memory>
#include <audio/AudioScheduler.h>
#include "rendering/fonts/Alignment.h"
#include <rendering/fonts/FontManager.h>

struct TextListData
{
  int textWidth = -1;
  signed char colorId;
  signed char colorBackgroundId;
  HorizontalAlignment hzAlignement;
  bool useHzAlignment;
};

template <typename T> class ITextListComponentInterface
{
  public:
    //! Default destructor
    virtual ~ITextListComponentInterface() = default;

    /*!
     * @brief Called when the cursor move to a new position
     * @param index New position index
     * @param data New position user data
     */
    virtual void OnCursorChanged(int index, T data) = 0;
};

//A graphical list. Supports multiple colors for rows and scrolling.
template <typename T>
class TextListComponent : public IList<TextListData, T>
                        , public Themable
{
protected:
  using IList<TextListData, T>::mEntries;
  using IList<TextListData, T>::listUpdate;
  using IList<TextListData, T>::listInput;
  using IList<TextListData, T>::listRenderTitleOverlay;
  using IList<TextListData, T>::getTransform;
  using IList<TextListData, T>::mSize;
  using IList<TextListData, T>::mCursor;
  using typename IList<TextListData, T>::Entry;

public:
  using IList<TextListData, T>::size;
  using IList<TextListData, T>::isScrolling;
  using IList<TextListData, T>::stopScrolling;

  explicit TextListComponent(WindowManager& window);

  void SetOverlayInterface(ITextListComponentOverlay<T>* overlay) { mOverlay = overlay; }

  [[nodiscard]] NewFont* getFont() const { return mFont; }

  bool ProcessInput(const InputCompactEvent& event) override;
  void Update(int deltaTime) override;
  void Render(const Transform4x4f& parentTrans) override;

  void add(const String& name, const T& obj, int colorId);
  void add(const String& name, const T& obj, int colorId, int colorBackgroundId, HorizontalAlignment alignment);
  void add(const String& name, const T& obj, int colorId, HorizontalAlignment alignment);
  bool changeTextAt(int index, const String& name);
  void changeBackgroundColorAt(int index, int colorIndex);
  int Lookup(T object);

  void PopScrollbar() { mBarTimer = sBarFadeTime + sBarFixedTime; }

  //! Get alignment
  [[nodiscard]] HorizontalAlignment Alignment() const { return mAlignment; }

  void setSelectedAt(int index, const T& object) { mEntries[index].object = object; }
  void setAlignment(HorizontalAlignment align) { mAlignment = align; }
  void setCursorChangedCallback(const std::function<void(CursorState)>& func) { mCursorChangedCallback = func; }
  void SetCursorChangedCallback(ITextListComponentInterface<T>* interface) { mCursorChangedCallbackInterface = interface; }
  void setFont(NewFont* font) { mFont = font; OnFontChanged(); }
  void SetDefaultSelectorHeight() { mSelectorHeight = Math::roundi((float)mFont->Height() * mLineSpacing); }
  virtual void OnFontChanged() {};

  [[nodiscard]] bool IsUppercase() const { return mUppercase; }
  void setUppercase(bool uppercase)
  {
    (void)uppercase;
    mUppercase = true; // TODO: Check
    for (auto& entry : mEntries)
      entry.data.textWidth = -1;
  }

  void setUseL1R1(bool value) { mUseL1R1 = value; }

  void setAutoAlternate(bool state) { mAutoAlternate = state; }
  void setShiftSelectedTextColor(bool invert) { mShiftSelectedTextColor = invert; }
  void setSelectorHeight(float selectorScale) { mSelectorHeight = (selectorScale != 0) ? selectorScale : EntryHeight(); }
  void setSelectorOffsetY(float selectorOffsetY) { mSelectorOffsetY = selectorOffsetY; }
  void setSelectorColor(unsigned int color) { mSelectorColor = color; updateBarColor(); }
  void setSelectedColor(unsigned int color) { mSelectedColor = color; updateBarColor(); }
  void setScrollSound(AudioScheduler::AudioHandle sound) { mScrollSound = sound; }
  void setColorAt(unsigned int id, unsigned int color) { mColors[id] = color; }
  void setSound(AudioScheduler::AudioHandle sound) { mScrollSound = sound; }
  void setLineSpacing(float lineSpacing) { mLineSpacing = lineSpacing; }
  void setHorizontalMargin(float horizontalMargin) { mHorizontalMargin = Math::round(horizontalMargin); }

  [[nodiscard]] float EntryHeight() const
  {
    int fontHeight = mFont->Height();
    int extraSpace = Math::roundi(mFont->Height() * mLineSpacing) - fontHeight;
    return (float)((fontHeight + extraSpace + 1) & -2);
  }
  [[nodiscard]] float Spacing() const { return mLineSpacing; }
  [[nodiscard]] float FontHeight() const { return mFont->Height(); }
  [[nodiscard]] unsigned int Color(unsigned int id) const { return mColors[id]; }

  [[nodiscard]] float getHorizontalMargin() const { return mHorizontalMargin; }



  protected:
    void onScroll(int amt) override { (void)amt; AudioScheduler::Instance().PlaySound(mScrollSound); }
    void onCursorChanged(const CursorState& state) override;

    /*
     * Themable implementation
     */

    /*!
     * @brief Apply theme element to this nine-patch image
     * @param element Theme element
     * @param properties Properties to set
     */
    void OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties) override;

    /*!
     * @brief Return theme element type
     * @return Element type
     */
    [[nodiscard]] ThemeElementType GetThemeElementType() const override { return ThemeElementType::TextList; }
    
  private:
    using IList<TextListData, T>::setColor;

  void updateBarColor()
  {
    unsigned char lr = mSelectorColor >> 24;
    unsigned char lg = (mSelectorColor >> 16) & 0xFF;
    unsigned char lb = (mSelectorColor >> 8) & 0xFF;
    unsigned char la = mSelectedColor & 0xFF;
    unsigned char rr = mSelectedColor >> 24;
    unsigned char rg = (mSelectedColor >> 16) & 0xFF;
    unsigned char rb = (mSelectedColor >> 8) & 0xFF;
    unsigned char ra = mSelectedColor & 0xFF;
    mBarColor = (((lr + rr) >> 1) << 24) |
                (((lg + rg) >> 1) << 16) |
                (((lb + rb) >> 1) << 8) |
                ((la + ra) >> 1);
  }

  enum class MarqueeSteps
  {
    LeftPause,     //!< Start with a smart pause
    ScrollToRight, //!< Scroll until the last char is visible
    RightPause,    //!< Smart pause again to let the user read the end
    RollOver,      //!< Scroll until text #2 left part is on the start position and then restart
  };

  static constexpr int sBarFixedTime = 200;
  static constexpr int sBarFadeTime = 600;
  static constexpr int sMarqueeSpeed1 = 80; // In pixel per seconds
  static constexpr int sMarqueeSpeed2 = 160; // In pixel per seconds
  static constexpr int sMarqueePause = 1000; // In milliseconds
  static constexpr unsigned int COLOR_ID_COUNT = 8;

  ImageComponent mSelectorImage;
  unsigned int mColors[COLOR_ID_COUNT];
  NewFont* mFont;
  std::function<void(CursorState state)> mCursorChangedCallback;
  ITextListComponentInterface<T>* mCursorChangedCallbackInterface;
  AudioScheduler::AudioHandle mScrollSound;

  ITextListComponentOverlay<T>* mOverlay;

  Array<float> mLeftOverlayWidth;
  Array<float> mRightOverlayWidth;

  int mMarqueeOffset;
  int mMarqueeTime;
  MarqueeSteps mMarqueeStep;
  HorizontalAlignment mAlignment;

  float mHorizontalMargin;
  float mLineSpacing;
  float mSelectorHeight;
  float mSelectorOffsetY;
  unsigned int mSelectorColor;
  unsigned int mSelectedColor;
  unsigned int mBarColor;
  int mBarTimer;
  bool mShowBar;
  bool mUppercase;
  bool mShiftSelectedTextColor;
  bool mAutoAlternate;
  bool mUseL1R1;
};

template<typename T>
int TextListComponent<T>::Lookup(T object)
{
  for(int i = mEntries.size(); --i >= 0; )
    if (mEntries[i].object == object)
      return i;
  return -1;
}

template <typename T>
TextListComponent<T>::TextListComponent(WindowManager& window)
  :	IList<TextListData, T>(window)
  , Themable(this)
  , mSelectorImage(window)
  , mColors{ 0x0000FFFF, 0x00FF00FF}
  , mFont(&FontManager::Instance().FromDefault(FONT_SIZE_MEDIUM, false))
  , mCursorChangedCallbackInterface(nullptr)
  , mScrollSound(0)
  , mOverlay(nullptr)
  , mMarqueeOffset(0)
  , mMarqueeTime(0)
  , mMarqueeStep(MarqueeSteps::LeftPause)
  , mAlignment(HorizontalAlignment::Center)
  , mHorizontalMargin(0)
  , mLineSpacing(1.4f)
  , mSelectorHeight(mLineSpacing * mFont->Height())
  , mSelectorOffsetY(0)
  , mSelectorColor(0x000000FF)
  , mSelectedColor(0)
  , mBarColor(0x00000080)
  , mBarTimer(-1)
  , mShowBar(false)
  , mUppercase(false)
  , mShiftSelectedTextColor(false)
  , mAutoAlternate(false)
  , mUseL1R1(false)
{
}

template <typename T>
void TextListComponent<T>::Render(const Transform4x4f& parentTrans)
{
  Transform4x4f trans = parentTrans * getTransform();
  trans.round();
  Vector2f lSize(mSize);
  lSize.round();

  if(size() == 0) return;

  const float entrySize = EntryHeight(); // Math::round(mFont->Height() * mLineSpacing);

  int startEntry = 0;

  //number of entries that can fit on the screen simultaniously
  int screenCount = (int)lSize.y() / (int)entrySize; //(int)(lSize.y() / entrySize + 0.5f);

  if(size() >= screenCount)
  {
    startEntry = mCursor - (screenCount / 2);
    if(startEntry < 0) startEntry = 0;
    if(startEntry >= size() - screenCount) startEntry = size() - screenCount;
  }

  int listCutoff = startEntry + screenCount;
  if(listCutoff >= size())	listCutoff = size() - 1;

  // clip to inside margins
  Vector3f dim(lSize.x(), lSize.y(), 0);
  dim = trans * dim - trans.translation(); dim.round();
  Renderer::Instance().ClipAbsolute(Rectangle(trans.translation().x(), trans.translation().y() - 1, dim.x(), dim.y()));

  // draw selector bar & background
  Renderer::SetMatrix(trans);
  for (int i = startEntry; i <= listCutoff; i++)
  {
    typename IList<TextListData, T>::Entry& entry = mEntries[i];
    if (mCursor == i)
    {
      if (mSelectorImage.hasImage())
      {
        mSelectorImage.setPosition(0.f, ((float)(i - startEntry) * entrySize) + mSelectorOffsetY, 0.f);
        mSelectorImage.Render(trans);
      }
      else
        Renderer::DrawRectangle(0.f, ((float)(i - startEntry) * entrySize) + mSelectorOffsetY, lSize.x(), mSelectorHeight, mSelectorColor);
    }
    /// Draw light/dark alternatively
    else if (mAutoAlternate && entry.data.colorBackgroundId < 0)
      Renderer::DrawRectangle(0.f, ((float) (i - startEntry) * entrySize) + mSelectorOffsetY, lSize.x(), mSelectorHeight, (i & 1) != 0 ? 0xFFFFFF10 : 0x00000010);
    // Draw background
    else if ((unsigned int) entry.data.colorBackgroundId < COLOR_ID_COUNT)
      Renderer::DrawRectangle(0.f, ((float) (i - startEntry) * entrySize) + mSelectorOffsetY, lSize.x(), mSelectorHeight, mColors[entry.data.colorBackgroundId]);
  }

  // Draw text items
  float y = 0;
  float leftMargin = 0.f;
  float rightMargin = 0.f;
  float previousLeftMargin = 0.f;
  float previousRightMargin = 0.f;
  FontEffect effect;
  for (int i = startEntry; i <= listCutoff; i++)
  {
    typename IList<TextListData, T>::Entry& entry = mEntries[i];
    unsigned int color = (mCursor == i && (mSelectedColor != 0)) ? mSelectedColor : mColors[mShiftSelectedTextColor && mCursor == i ? entry.data.colorId + 1 : entry.data.colorId];
    if (entry.data.textWidth < 0)
      entry.data.textWidth = mFont->TextWidth(mUppercase ? entry.name.ToUpperCaseUTF8() : entry.name);

    leftMargin = mHorizontalMargin;
    rightMargin = mHorizontalMargin;
    if (mOverlay != nullptr)
    {
      // overlay?
      leftMargin += (mLeftOverlayWidth(i - startEntry) = mOverlay->OverlayGetLeftOffset(entry.object, entry.data.textWidth));
      rightMargin += (mRightOverlayWidth(i - startEntry) = mOverlay->OverlayGetRightOffset(entry.object, entry.data.textWidth));
      //if (leftMargin)
      if (leftMargin != previousLeftMargin || rightMargin != previousRightMargin)
      {
        Renderer::Instance().Unclip();
        Renderer::Instance().ClipAbsolute(Rectangle(trans.translation().x() + leftMargin, trans.translation().y(), dim.x() - (leftMargin + rightMargin), dim.y()));
      }
      previousLeftMargin = leftMargin;
      previousRightMargin = rightMargin;
    }

    // Draw text
    Rectangle textArea(leftMargin, y, lSize.x() - (leftMargin + rightMargin), entrySize);
    float textXOffset = (mCursor == i) ? (float)-mMarqueeOffset : 0.f;

    ::Alignment align = AlignmentMergeVerticalAndHorizontal(VerticalAlignment::Center, entry.data.useHzAlignment ? entry.data.hzAlignement : mAlignment);
    mFont->DrawText(mUppercase ? entry.name.ToUpperCaseUTF8() : entry.name, textArea, textXOffset, 0.f, color, align, effect);

    if (mCursor == i && mMarqueeOffset > 0)
    {
      textXOffset += entry.data.textWidth + (entry.data.textWidth / 4);
      mFont->DrawText(mUppercase ? entry.name.ToUpperCaseUTF8() : entry.name, textArea, textXOffset, 0.f, color, align, effect);
    }

    y += entrySize;
  }

  Renderer::Instance().Unclip();

  // Overlay?
  if (mOverlay != nullptr)
  {
    Renderer::Instance().ClipAbsolute(Rectangle(trans.translation().x(), trans.translation().y(), dim.x(), dim.y()));

    // Draw overlay
    y = 0;
    for (int i = startEntry; i <= listCutoff; i++)
    {
      typename IList<TextListData, T>::Entry& entry = mEntries[i];

      Vector3f position(mHorizontalMargin, y, 0);
      Vector2f size(mSize.x() - (2 * mHorizontalMargin), entrySize);

      unsigned int color = (mCursor == i && (mSelectedColor != 0)) ? mSelectedColor : mColors[entry.data.colorId];

      Transform4x4f drawTrans = trans;
      drawTrans.translate(position);
      Renderer::SetMatrix(drawTrans);

      mOverlay->OverlayApply(drawTrans, mLeftOverlayWidth[i - startEntry], mRightOverlayWidth[i - startEntry], entry.data.textWidth, Rectangle(0.f, 0.f, size.x(), size.y()), entry.object, i, color);

      y += entrySize;
    }

    Renderer::Instance().Unclip();
  }

  // Draw bar
  if (mShowBar || mBarTimer >= 0)
  {
    int verticalSize = (int)dim.y() - 4;
    int maxOffset = (int)mEntries.size() - screenCount;
    if (maxOffset > 0)
    {
      int barHeight = (int) (((float) verticalSize * (float) screenCount) / (float) mEntries.size());

      if ((barHeight < Renderer::Instance().DisplayHeightAsInt() / 40) &&
          (verticalSize >= Renderer::Instance().DisplayHeightAsInt() / 20))
        barHeight = Renderer::Instance().DisplayHeightAsInt() / 40;
      int yOffset = (int) (((float) startEntry * (float) (verticalSize - barHeight)) / (float)maxOffset);

      int alpha = mShowBar || mBarTimer > sBarFadeTime ? 0xFF : (255 * mBarTimer) / sBarFadeTime;

      int screenWidth = Renderer::Instance().DisplayWidthAsInt();
      int barWidth = screenWidth > 1024 ? 6 : screenWidth > 480 ? 4 : 2;
      Renderer::SetMatrix(trans);
      Renderer::DrawRectangle((int) mSize.x() - 2 - barWidth, 2 + yOffset, barWidth, barHeight, (mBarColor & 0xFFFFFF00) | alpha);
    }
  }

  listRenderTitleOverlay(trans);

  Component::renderChildren(trans);
}

template <typename T>
bool TextListComponent<T>::ProcessInput(const InputCompactEvent& event)
{
  if(size() > 0)
  {
    if (event.AnyPrimaryDownPressed())
    {
      listInput(1);
      mShowBar = true;
      return true;
    }
    if (event.AnyPrimaryUpPressed())
    {
      listInput(-1);
      mShowBar = true;
      return true;
    }
    if (mUseL1R1)
    {
      if (event.R1Pressed())
      {
        listInput(10);
        mShowBar = true;
        return true;
      }
      if (event.L1Pressed())
      {
        listInput(-10);
        mShowBar = true;
        return true;
      }
    }
    if (event.AnyPrimaryDownReleased() || event.AnyPrimaryUpReleased() || event.R1Released() || event.L1Released())
    {
      mBarTimer = sBarFadeTime + sBarFixedTime;
      mShowBar = false;
      stopScrolling();
    }
  }

  return Component::ProcessInput(event);
}

template <typename T>
void TextListComponent<T>::Update(int deltaTime)
{
  if (mBarTimer > 0) mBarTimer -= deltaTime;

  listUpdate(deltaTime);
  if(!isScrolling() && size() > 0)
  {
    //if we're not scrolling and this object's text goes outside our size, marquee it!
    const int textWidth = (int)(mEntries[mCursor].data.textWidth) + 1;

    //it's long enough to marquee
    float fwidth = mSize.x() - (2 * mHorizontalMargin);
    if (mOverlay != nullptr) fwidth -= mOverlay->OverlayGetLeftOffset(mEntries[mCursor].object, textWidth) +
                                       mOverlay->OverlayGetRightOffset(mEntries[mCursor].object, textWidth);
    int width = (int)fwidth;

    if (textWidth > width)
    {
      switch (mMarqueeStep)
      {
        case MarqueeSteps::LeftPause:
        {
          mMarqueeOffset = 0;
          if (mMarqueeTime > sMarqueePause) { mMarqueeTime = 0; mMarqueeStep = MarqueeSteps::ScrollToRight; }
          break;
        }
        case MarqueeSteps::ScrollToRight:
        {
          mMarqueeOffset = (mMarqueeTime * sMarqueeSpeed1) / 1000;
          if (mMarqueeOffset >= textWidth - width) { mMarqueeTime = 0; mMarqueeOffset = textWidth - width; mMarqueeStep = MarqueeSteps::RightPause;}
          break;
        }
        case MarqueeSteps::RightPause:
        {
          mMarqueeOffset = textWidth - width;
          if (mMarqueeTime > sMarqueePause) { mMarqueeTime = 0; mMarqueeStep = MarqueeSteps::RollOver; }
          break;
        }
        case MarqueeSteps::RollOver:
        {
          mMarqueeOffset = textWidth - width + (mMarqueeTime * sMarqueeSpeed2) / 1000;
          if (mMarqueeOffset >= textWidth + mSize.x() / 4) { mMarqueeTime = 0; mMarqueeOffset = 0; mMarqueeStep = MarqueeSteps::LeftPause;}
          break;
        }
        default: break;
      }
      mMarqueeTime += deltaTime;
    }
  }

  Component::Update(deltaTime);
}

//list management stuff
template <typename T>
void TextListComponent<T>::add(const String& name, const T& obj, int color)
{
  assert((unsigned int)color < COLOR_ID_COUNT);

  typename IList<TextListData, T>::Entry entry;
  entry.name = name;
  entry.object = obj;
  entry.data.colorId = color;
  entry.data.colorBackgroundId = -1;
  entry.data.useHzAlignment = false;
  ((IList< TextListData, T >*)this)->add(entry);
}

template <typename T>
void TextListComponent<T>::add(const String& name, const T& obj, int color, int colorBackground, HorizontalAlignment align)
{
  assert((unsigned int)color < COLOR_ID_COUNT);

  typename IList<TextListData, T>::Entry entry;
  entry.name = name;
  entry.object = obj;
  entry.data.colorId = color;
  entry.data.colorBackgroundId = colorBackground;
  entry.data.useHzAlignment = true;
  entry.data.hzAlignement = align;
  ((IList< TextListData, T >*)this)->add(entry);
}

template <typename T>
void TextListComponent<T>::add(const String& name, const T& obj, int color, HorizontalAlignment align)
{
  assert((unsigned int)color < COLOR_ID_COUNT);

  typename IList<TextListData, T>::Entry entry;
  entry.name = name;
  entry.object = obj;
  entry.data.colorId = color;
  entry.data.colorBackgroundId = -1;
  entry.data.useHzAlignment = true;
  entry.data.hzAlignement = align;
  ((IList< TextListData, T >*)this)->add(entry);
}

template <typename T>
bool TextListComponent<T>::changeTextAt(int index, const String& name)
{
  return ((IList< TextListData, T >*)this)->changeCursorName(index, name);
}

template <typename T>
void TextListComponent<T>::changeBackgroundColorAt(int index, int colorIndex)
{
  ((IList< TextListData, T >*)this)->getSelectedEntryAt(index).colorBackgroundId = colorIndex;
}

template <typename T>
void TextListComponent<T>::onCursorChanged(const CursorState& state)
{
  mMarqueeStep = MarqueeSteps::LeftPause;
  mMarqueeOffset = 0;
  mMarqueeTime = 0;

  if(mCursorChangedCallback) mCursorChangedCallback(state);
  if (!mEntries.empty())
    if (mCursorChangedCallbackInterface != nullptr) mCursorChangedCallbackInterface->OnCursorChanged(mCursor, mEntries[mCursor].object);
}

template <typename T>
void TextListComponent<T>::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
  if (hasFlag(properties, ThemePropertyCategory::Color))
  {
    if (element.HasProperty(ThemePropertyName::SelectorColor))  setSelectorColor((unsigned int)element.AsInt(ThemePropertyName::SelectorColor));
    if (element.HasProperty(ThemePropertyName::SelectedColor))  setSelectedColor((unsigned int)element.AsInt(ThemePropertyName::SelectedColor));
    if (element.HasProperty(ThemePropertyName::PrimaryColor))   setColorAt(0, (unsigned int)element.AsInt(ThemePropertyName::PrimaryColor));
    if (element.HasProperty(ThemePropertyName::SecondaryColor)) setColorAt(1, (unsigned int)element.AsInt(ThemePropertyName::SecondaryColor));
  }

  setFont(&FontManager::Instance().FromTheme(element, properties, mFont));

  if (hasFlag(properties, ThemePropertyCategory::Sound) && element.HasProperty(ThemePropertyName::ScrollSound))
  {
    Path soundPath = Path(element.AsString(ThemePropertyName::ScrollSound));
    setSound(AudioScheduler::Instance().LoadSound(soundPath));
  }

  if (hasFlag(properties, ThemePropertyCategory::Text))
  {
    if(element.HasProperty(ThemePropertyName::Alignment))
      setAlignment(AlignmentExtractHorizontal(element.AsAlignment(ThemePropertyName::Alignment)));
    if(element.HasProperty(ThemePropertyName::HorizontalMargin))
      mHorizontalMargin = Math::round(element.AsFloat(ThemePropertyName::HorizontalMargin) * (this->mParent ? this->mParent->getSize().x() : Renderer::Instance().DisplayWidthAsFloat()));

    if (element.HasProperty(ThemePropertyName::ForceUppercase))
      setUppercase(element.AsBool(ThemePropertyName::ForceUppercase));

    if(element.HasProperty(ThemePropertyName::LineSpacing)) setLineSpacing(element.AsFloat(ThemePropertyName::LineSpacing));
    if(element.HasProperty(ThemePropertyName::SelectorHeight)) setSelectorHeight(element.AsFloat(ThemePropertyName::SelectorHeight) * Renderer::Instance().DisplayHeightAsFloat());
    else setSelectorHeight(mFont->Height() * mLineSpacing);
    if(element.HasProperty(ThemePropertyName::SelectorOffsetY))
    {
      float scale = this->mParent ? this->mParent->getSize().y() : Renderer::Instance().DisplayHeightAsFloat();
      setSelectorOffsetY(element.AsFloat(ThemePropertyName::SelectorOffsetY) * scale);
    }
    else setSelectorOffsetY(0.0);
  }

  if (element.HasProperty(ThemePropertyName::SelectorImagePath))
  {
    Path path(element.AsString(ThemePropertyName::SelectorImagePath));
    bool tile = element.HasProperty(ThemePropertyName::SelectorImageTile) && element.AsBool(ThemePropertyName::SelectorImageTile);
    mSelectorImage.setImage(path, tile);
    mSelectorImage.setSize(mSize.x(), mSelectorHeight);
    mSelectorImage.setColorShift(mSelectorColor);
  }
  else mSelectorImage.setImage(Path(), false);
}
