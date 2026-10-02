#include "components/ButtonComponent.h"
#include "Renderer.h"
#include "WindowManager.h"
#include "themes/MenuThemeData.h"
#include <themes/ThemeManager.h>
#include <rendering/fonts/FontManager.h>
#include "rendering/textures/TextureManager.h"

ButtonComponent::ButtonComponent(WindowManager&window, const String& text, const String& helpText, const std::function<void()>& func, bool upperCase)
  : Component(window)
  , mFont(&FontManager::Instance().FromDefault(FONT_SIZE_MEDIUM, false))
  , mFocused(false)
  , mEnabled(true)
  , mTextColorFocused(0xFFFFFFFF), mTextColorUnfocused(0x777777FF)
  , mModdedColor(0)
	, mTextWidth(0)
	, mTextHeight(0)
  , mBoxEmpty(GetTexture(false))
  , mBoxFilled(GetTexture(true))
{
  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
	mFont = menuTheme.Text().font;
	mTextColorUnfocused = menuTheme.Text().color;
	mTextColorFocused = menuTheme.Text().selectedColor;
	mColor = menuTheme.Text().color;

	setPressedFunc(func);
	setText(text, helpText, upperCase);
}

Texture ButtonComponent::GetTexture(bool isfilled)
{
  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
	Path empty = menuTheme.Elements().FromType(MenuThemeData::IconElement::Type::Button);
	Path filled = menuTheme.Elements().FromType(MenuThemeData::IconElement::Type::ButtonFilled);

	if (isfilled) return TextureManager::Instance().Create(filled.IsEmpty() ? Path(":/button_filled.png") : filled);
	return TextureManager::Instance().Create(empty.IsEmpty() ? Path(":/button.png") : empty);
}


void ButtonComponent::setPressedFunc(std::function<void()> f)
{
	mPressedFunc = std::move(f);
}

bool ButtonComponent::ProcessInput(const InputCompactEvent& event)
{
	if(event.ValidReleased())
	{
		if(mPressedFunc && mEnabled)
			mPressedFunc();
		return true;
	}

	return Component::ProcessInput(event);
}

void ButtonComponent::setText(const String& text, const String& helpText, bool upperCase, bool resize, bool doUpdateHelpPrompts)
{
	mText = upperCase ? text.ToUpperCaseUTF8() : text;
	mText.Trim();
	mHelpText = helpText;

	if (resize)
	{
		float minWidth = mFont->TextSize("BUTON").x() + 12;
		setSize(Math::max((float)mFont->TextWidth(mText) + 12.f, minWidth), (float)mFont->Height() + 8.f);
	}

	if (doUpdateHelpPrompts)
    UpdateHelpBar();
}

void ButtonComponent::autoSizeFont()
{
	if (mFont->TextWidth(mText) + 6 > (int)getSize().x())
  {
		mFont = &FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false);
		onSizeChanged();
	}
	if (mFont->TextWidth(mText) + 6 > (int)getSize().x())
  {
		mFont = &FontManager::Instance().FromDefault(FONT_SIZE_EXTRASMALL, false);
		onSizeChanged();
	}
}

bool ButtonComponent::isTextOverlapping() const
{
	// + 2 to keep a small space between text and border
	return mFont->TextWidth(mText) + 2  > (int)getSize().x();
}

void ButtonComponent::onFocusGained()
{
	mFocused = true;
  mWindow.UpdateHelpSystem();
}

void ButtonComponent::onFocusLost()
{
	mFocused = false;
  mWindow.UpdateHelpSystem();
}

void ButtonComponent::setEnabled(bool enabled)
{
	mEnabled = enabled;
}

void ButtonComponent::Render(const Transform4x4f& parentTrans)
{
  Transform4x4f trans = (parentTrans * getTransform()).round();
	Renderer::SetMatrix(trans);

	unsigned int color = mColor;
	if (!mEnabled || !mPressedFunc) color = 0x770000FF;
	else if (mNewColor) color = mModdedColor;
	Rectangle area(0.f, 0.f, mSize.x(), mSize.y());
	(mFocused ? mBoxFilled : mBoxEmpty).Render(area, color);
  mFont->DrawText(mText, area, getCurTextColor(), ::Alignment::Center);

	renderChildren(trans);
}

unsigned int ButtonComponent::getCurTextColor() const
{
	if(!mFocused) return mTextColorUnfocused;
  return mTextColorFocused;
}

bool ButtonComponent::CollectHelpItems(Help& help)
{
  if (mFocused)
    help.Set(Help::Valid(), mHelpText.empty() ? mText : mHelpText);
  return true;
}
