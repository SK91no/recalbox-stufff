//
// Created by bkg2k on 24/05/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#pragma once

#include "themes/ThemeManager.h"
#include <rendering/fonts/MarkdownFontHolder.h>

class MarkdownText : public ThemableComponent
{
  public:
    /*!
     * @brief Constructor
     * @param window Window manager
     */
    explicit MarkdownText(WindowManager& window)
      : MarkdownText(window, String::Empty, *ThemeManager::Instance().Menu().SmallText().font, ThemeManager::Instance().Menu().SmallText().color, ::Alignment::TopLeft, true)
    {}

    /*!
     * @brief Constructor
     * @param window Window manager
     * @param text Text
     * @param color Color template
     * @param alignment Alignment
     */
    MarkdownText(WindowManager& window, const String& text, Colors::ColorARGB color, ::Alignment alignment)
      : MarkdownText(window, text, *ThemeManager::Instance().Menu().SmallText().font, color, alignment, false)
    {}

    /*!
     * @brief Constructor
     * @param window Window manager
     * @param text Text
     * @param color Color template
     * @param alignment Alignment
     * @param autoscroll Auto-scrolling if text height > size height
     */
    MarkdownText(WindowManager& window, const String& text, Colors::ColorARGB color, ::Alignment alignment, bool autoscroll)
      : MarkdownText(window, text, *ThemeManager::Instance().Menu().SmallText().font, color, alignment, autoscroll)
    {}

    /*!
     * @brief Constructor
     * @param window Window manager
     * @param text Text
     * @param font Font template
     * @param color Color template
     * @param alignment Alignment
     */
    MarkdownText(WindowManager& window, const String& text, const NewFont& font, Colors::ColorARGB color, ::Alignment alignment)
      : MarkdownText(window, text, font, color, alignment, false)
    {}

    /*!
     * @brief Constructor
     * @param window Window manager
     * @param text Text
     * @param font Font template
     * @param color Color template
     * @param alignment Alignment
     * @param autoscroll Auto-scrolling if text height > size height
     */
    MarkdownText(WindowManager& window, const String& text, const NewFont& font, Colors::ColorARGB color, ::Alignment alignment, bool autoscroll)
      : ThemableComponent(window)
      , mText(text)
      , mColor(color)
      , mBackgroundColor(0)
      , mColorOpacity(color & 0xFF)
      , mBackgroundColorOpacity(0)
      , mUppercase(false)
      , mUpDownScrolling(false)
      , mTextWidth(0)
      , mTextHeight(0)
      , mInterlineRatio(0.1f)
      , mPadding(2.f)
      , mScrollOffset(0)
      , mScrollTime(0)
      , mLastDirection(0)
      , mScrollStep(ScrollSteps::LeftPause)
      , mAlignment(alignment)
      , mEffectiveAlignment(alignment)
      , mRTL(false)
      , mManualScrollingTimer(0)
      , mAutoScroll(autoscroll)
      , mOriginalAutoScroll(autoscroll)
    {
      GetAllFontsFrom(font.GetPath(), font.GetPath(), 0);
    }

    /*
     * Accessors
     */

    //! Get text
    [[nodiscard]] const String& Text() const { return mText; }

    //! Get interline ratio
    [[nodiscard]] float Interline() const { return mInterlineRatio; }

    //! Width
    [[nodiscard]] int Width() const { return mTextWidth + Math::round(2 * mPadding); }
    //! Height
    [[nodiscard]] int Height() const { return mTextHeight + Math::round(2 * mPadding); }

    //! Width
    [[nodiscard]] int TextWidth() const { return mTextWidth; }
    //! Height
    [[nodiscard]] int TextHeight() const { return mTextHeight; }

    /*
     * Setters
     */

    //! Set Font
    void SetFont(const NewFont* font) { GetAllFontsFrom(font->GetPath(), font->GetPath(), 0); RebuildRequired(); }
    //! Set text
    MarkdownText& SetText(const char* text) { SetText(String(text)); return *this; }
    //! Set text
    MarkdownText& SetText(const String& text);
    //! Set interline ratio (in font height)
    MarkdownText& SetInterline(float ratio) { mInterlineRatio = ratio; return *this; }
    //! Set auto scrolling
    MarkdownText& SetAutoscroll(bool autoscroll) { mAutoScroll = autoscroll; return *this; }
    //! Set color
    MarkdownText& SetColor(Colors::ColorRGBA color) { mColor = color; return *this; }
    //! Set background color
    MarkdownText& SetBackgroundColor(Colors::ColorRGBA color) { mBackgroundColor = color; return *this; }
    //! Set padding
    MarkdownText& SetPadding(float padding) { mPadding = padding; return *this; }
    //! Set up/down control
    MarkdownText& SetUpDownScrolling(bool enable) { mUpDownScrolling = enable; return *this; }

    // Chunk properties
    enum class DrawingProperties : unsigned char
    {
        None           = 0,
        FontBold       = 1,  //!< Drow the chunk bold
        FontItalic     = 2,  //!< Drow the chunk italic
        FontStrikeThru = 4,  //!< Drow the chunk Strike through
        FontUnderline  = 8,  //!< Drow the chunk Underline
        Separator      = 16,  //!< Draw a separator line after the chunk
        Bullet         = 32, //!< Draw a bullet in front of this chunk
        Numbered       = 64, //!< Draw a number in front of this chunk
    };

    /*!
     * @brief Process right joystick text scrolling control
     * @param event Compact event
     * @return Implementation must return true if it consumed the event.
     */
    bool ProcessInput(const InputCompactEvent& event) override;

    /*!
     * @brief Called once per frame, after Update
     * Implement your own Render() to draw your own components or over-drawings
     * First, execute: 	Transform4x4f trans = (parentTrans * getTransform()).round();
     *                  Renderer::setMatrix(trans);
     * Then draw your components
     * Finally, call your base.Render(trans) to draw animations and childrens
     * @param parentTrans Transformation
     */
    void Render(const Transform4x4f& parentTrans) override;

    /*!
     * @brief Apply auto scrolling
     * @param deltaTime Elapsed time from the previous frame, in millisecond
     */
    void Update(int deltaTime) final;

  protected:
    void onSizeChanged() override;

    //! Scale opacity
    void setOpacity(unsigned char opacity) final;

    /*!
     * @brief Apply theme element to this text component
     * @param theme Theme
     * @param properties Properties to set
     */
    void OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties) override;

    /*!
     * @brief Return theme element type
     * @return Element type
     */
    [[nodiscard]] ThemeElementType GetThemeElementType() const override { return ThemeElementType::Markdown; }

    //! Override setValue from component
    void setValue(const String& text) final { SetText(text); }
    //! Override setColor from component
    void setColor(unsigned int color) final { mColor = color; mColorOpacity = color & 0xFF; }
    //! Override setBackgroundColor from component
    void setBackgroundColor(unsigned int color) { mBackgroundColor = color; mBackgroundColorOpacity = color & 0xFF; }
    //! Set uppercase
    void setUppercase(bool uppercase) { if (mUppercase != uppercase) { mUppercase = uppercase; RebuildRequired(); } }

    //! Text
    String mText;
    //! Text
    String mDisplayedText;
    //! Color
    Colors::ColorARGB mColor;
    //! Color
    Colors::ColorARGB mBackgroundColor;
    //! Opacity front
    unsigned char mColorOpacity;
    //! Opacity back
    unsigned char mBackgroundColorOpacity;
    //! Uppercase
    bool mUppercase;
    //! Control scrolling w/ up/down ?
    bool mUpDownScrolling;

    //! Font holder
    MarkdownFontHolder mFontHolder;

  private:
    static String sBullet;

    enum class ScrollSteps
    {
      LeftPause,     //!< Start with a smart pause
      ScrollToRight, //!< Scroll until the last char is visible
      RightPause,    //!< Smart pause again to let the user read the end
      RollOver,      //!< Scroll until text #2 left part is on the start position and then restart
    };

    static constexpr int sScrollSpeed1 = 40; // In pixel per seconds
    static constexpr int sScrollSpeed2 = 80; // In pixel per seconds
    static constexpr int sScrollPause = 5000; // In milliseconds
    static constexpr int sManualScrollInactivity = 10000; // In milliseconds

    // Chunk to draw
    struct Chunk
    {
      NewFont* Font;          //!< Font used to draw this chunk
      int XOffsetOrigin;      //!< Original offset from the left
      int XOffset;            //!< Offset from the left (might be shifted by alignment)
      int YOffset;            //!< Offset from the top
      int Width;              //!< Width of the chunk
      int OffsetInText;       //!< Start offset in text
      int Length;             //!< Char length
      unsigned char Number;   //!< Number to draw
      DrawingProperties Draw; //!< Drawing properties of this chunk
    } __attribute__((packed));

    //! Text chunks
    Array<Chunk> mChunks;

    //! Text Width
    int mTextWidth;
    //! Text Height
    int mTextHeight;
    //! Interline1
    float mInterlineRatio;
    //! Padding
    float mPadding;

    //! Vertical scroll offset
    int mScrollOffset;
    //! Scroll time accumulator (ms)
    int mScrollTime;
    //! Scrolling direction
    int mLastDirection;
    //! Scrolling step
    ScrollSteps mScrollStep;

    //! Alignment as set by theme/caller (source of truth)
    ::Alignment mAlignment;
    //! Effective alignment: mAlignment with horizontal component inverted when mRTL
    ::Alignment mEffectiveAlignment;
    //! True when the first strong character in the current text is RTL
    bool mRTL;

    //! Manual scrolling timer
    int mManualScrollingTimer;
    //! Autoscroll?
    bool mAutoScroll;
    //! Original autoscroll flag
    bool mOriginalAutoScroll;

    //! Recompute mEffectiveAlignment from mAlignment and mRTL
    void UpdateEffectiveAlignment() { mEffectiveAlignment = mRTL ? AlignmentInvertHorizontal(mAlignment) : mAlignment; }

    /*!
     * @brief No rebuild required
     */
    void RebuildRequired();

    /*!
     * @brief Change chunk alignment
     */
    void RealignChunks();

    /*!
     * @brief Get all Markdown font from
     * @param baseFontFile Regular font
     * @param fixedFontFile Fixed font
     * @param size Font size in screen ratio, or 0.f for default size
     */
    void GetAllFontsFrom(const Path& baseFontFile, const Path& fixedFontFile, float size);

    /*!
     * @brief Draw all Markdown lines
     * @param inner Rectangle to draw in
     * @param offset Y Offset
     */
    void RenderMarkdown(const Rectangle& inner, int offset);
};

DEFINE_BITFLAG_ENUM(MarkdownText::DrawingProperties, unsigned char)
