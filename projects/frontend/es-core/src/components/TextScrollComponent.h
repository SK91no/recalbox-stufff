//
// Created by bkg2k on 04/12/23.
//
#pragma once

#include "components/base/ThemableComponent.h"
#include <rendering/fonts/Font.h>

class ThemeData;

class TextScrollComponent : public ThemableComponent
{
  public:
    explicit TextScrollComponent(WindowManager& window);

    TextScrollComponent(WindowManager& window, const String& text, NewFont* font, unsigned int color);

    TextScrollComponent(WindowManager& window, const String& text, NewFont* font, unsigned int color, ::Alignment align);

    TextScrollComponent(WindowManager& window, const String& text, NewFont* font, unsigned int color, ::Alignment align,
                        Vector3f pos, Vector2f size, unsigned int bgcolor);

    ~TextScrollComponent() override = default;

    void setFont(NewFont* font);

    void setUppercase(bool uppercase);

    void setText(const String& text);

    void setColor(unsigned int color) override;

    void setOriginColor(unsigned int color) { mOriginColor = color; }

    unsigned int getOriginColor() override { return mOriginColor; }

    void setAlignment(::Alignment align) { mAlignment = align; UpdateEffectiveAlignment(); onTextChanged(); }
    void setVerticalAlignment(::VerticalAlignment align) { mAlignment = AlignmentReplaceVertical(mAlignment, align); UpdateEffectiveAlignment(); onTextChanged(); }
    void setHorizontalAlignment(::HorizontalAlignment align) { mAlignment = AlignmentReplaceHorizontal(mAlignment, align); UpdateEffectiveAlignment(); onTextChanged(); }

    void setBackgroundColor(unsigned int color);

    void setRenderBackground(bool render) { mRenderBackground = render; }

    void Render(const Transform4x4f& parentTrans) override;

    [[nodiscard]] String getValue() const override { return mText; }

    void setValue(const String& value) override { setText(value); }

    [[nodiscard]] unsigned char getOpacity() const override { return (unsigned char) (mColor & 0xFF); }

    void setOpacity(unsigned char opacity) override;

    [[nodiscard]] NewFont* getFont() const { return mFont; }

    void Update(int deltaTime) override;

    void onSizeChanged() override;

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
    [[nodiscard]] ThemeElementType GetThemeElementType() const override { return ThemeElementType::ScrollText; }

    /*!
     * @brief Return compatible element type
     * @return Element type
     */
    [[nodiscard]] ThemeElementType GetThemeElementType2() const override { return ThemeElementType::Text; }

    //! Set text bold
    TextScrollComponent& SetBold(bool state) { mFontStyle.SetBold(state); return *this; }

    //! Set text italic
    TextScrollComponent& SetItalic(bool state) { mFontStyle.SetItalic(state); return *this; }

  private:
    //! Recompute mEffectiveAlignment from mAlignment and mRTL (call after either changes)
    void UpdateEffectiveAlignment() { mEffectiveAlignment = mRTL ? AlignmentInvertHorizontal(mAlignment) : mAlignment; }

    static constexpr int sScrollSpeed1 = 80; // In pixel per seconds
    static constexpr int sScrollSpeed2 = 160; // In pixel per seconds
    static constexpr int sScrollPause = 2000; // In milliseconds

    enum class ScrollSteps
    {
      LeftPause,     //!< Start with a smart pause
      ScrollToRight, //!< Scroll until th elast char is visible
      RightPause,    //!< Smart pause again to let the user read the end
      RollOver,      //!< Scroll until text #2 left part is on the start position and then restart
    };

    void onTextChanged();

    NewFont* mFont;
    FontEffect mFontStyle;
    String mText;
    String mDisplayableText;
    ScrollSteps mStep;
    int mMarqueeTime;
    int mOffset;
    int mTextWidth;
    int mTextHeight;
    unsigned int mColor;
    unsigned int mOriginColor;
    unsigned int mBgColor;
    unsigned char mColorOpacity;
    unsigned char mBgColorOpacity;
    ::Alignment mAlignment;
    //! Effective alignment: mAlignment with horizontal component inverted when mRTL
    ::Alignment mEffectiveAlignment;
    //! True when the first strong character in the current text is RTL
    bool mRTL;
    bool mRenderBackground;
    bool mUppercase;
};

