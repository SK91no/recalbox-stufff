#pragma once

#include <components/base/ThemableComponent.h>
#include "rendering/fonts/Font.h"

class ThemeData;

// Used to display text.
// TextComponent::setSize(x, y) works a little differently than most components:
//  * (0, 0)                     - will automatically calculate a size that fits the text on one line (expand horizontally)
//  * (x != 0, 0)                - wrap text so that it does not reach beyond x. Will automatically calculate a vertical size (expand vertically).
//  * (x != 0, y <= fontHeight)  - will truncate text so it fits within this box.
class TextComponent : public ThemableComponent
{
public:
    static constexpr float sStupidLegacyHeightThreshold = 1.2f;
	explicit TextComponent(WindowManager&window);
  TextComponent(WindowManager&window, const String& text, NewFont* font, unsigned int color);
  TextComponent(WindowManager&window, const String& text, NewFont* font, unsigned int color, ::Alignment align);
	TextComponent(WindowManager&window, const String& text, NewFont* font, unsigned int color, ::Alignment align,
                Vector3f pos, Vector2f size, unsigned int bgcolor);

  ~TextComponent() override = default;

	void setFont(NewFont* font);
	void setUppercase(bool uppercase);
	void onSizeChanged() override;
	void setText(const String& text);
	void setColor(unsigned int color) override;
	void setOriginColor(unsigned int color) { mOriginColor = color; }
	unsigned int getOriginColor() override { return mOriginColor; }

	void setAlignment(::Alignment align) { mAlignment = align; UpdateEffectiveAlignment(); }
  void setVerticalAlignment(::VerticalAlignment align) { mAlignment = AlignmentReplaceVertical(mAlignment, align); UpdateEffectiveAlignment(); }
  void setHorizontalAlignment(::HorizontalAlignment align) { mAlignment = AlignmentReplaceHorizontal(mAlignment, align); UpdateEffectiveAlignment(); }
  void setMultiline(bool multiline) { mIsMultiline = multiline; onTextChanged(); }

	void setLineSpacing(float spacing);
	void setBackgroundColor(unsigned int color);

	void Render(const Transform4x4f& parentTrans) override;

	[[nodiscard]] String getValue() const override { return mText; }
	void setValue(const String& value) override { setText(value); }

	[[nodiscard]] unsigned char getOpacity() const override {	return (unsigned char)(mColor & 0xFF); }

  void setOpacity(unsigned char opacity) override;
	
	[[nodiscard]] NewFont* getFont() const { return mFont; }

    /*!
     * @brief Apply theme element to this text component
     * @param element Theme element
     * @param properties Properties to set
     */
    void OnApplyThemeElement(const ThemeElement& theme, ThemePropertyCategory properties) override;

    /*!
     * @brief Return theme element type
     * @return Element type
     */
    [[nodiscard]] ThemeElementType GetThemeElementType() const override { return ThemeElementType::Text; }

  private:
    //! Set text bold
    void SetBold(bool state) { mFontStyle.SetBold(state); }

    //! Set text italic
    void SetItalic(bool state) { mFontStyle.SetItalic(state); }

    //! Recompute mEffectiveAlignment from mAlignment and mRTL (call after either changes)
    void UpdateEffectiveAlignment() { mEffectiveAlignment = mRTL ? AlignmentInvertHorizontal(mAlignment) : mAlignment; }

  private:
    void onTextChanged();

    void calculateExtent();

    NewFont* mFont;
    FontEffect mFontStyle;
    NewFont::TextChunkList mChunks;
    String mText;
    unsigned int mColor;
    unsigned int mOriginColor;
    unsigned int mBgColor;
    float mLineSpacing;
    unsigned char mColorOpacity;
    unsigned char mBgColorOpacity;
    ::Alignment mAlignment;
    //! Effective alignment: mAlignment with horizontal component inverted when mRTL
    ::Alignment mEffectiveAlignment;
    //! True when the first strong character in the current text is RTL (Arabic/Farsi/Urdu)
    bool mRTL;
    bool mUppercase;
    bool mAutoCalcExtentX;
    bool mAutoCalcExtentY;
    bool mClipped;
    bool mIsMultiline;
};
