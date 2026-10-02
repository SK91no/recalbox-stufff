#pragma once

#include "components/base/Component.h"
#include <functional>
#include <utils/String.h>
#include <rendering/fonts/Font.h>
#include "guis/menus/base/NinePatchImage.h"

class ButtonComponent : public Component
{
	public:
		ButtonComponent(WindowManager&window, const String& text, const String& helpText, const std::function<void()>& func, bool upperCase);
	  ButtonComponent(WindowManager&window, const String& text, const String& helpText, const std::function<void()>& func)
	    : ButtonComponent(window, text, helpText, func, true)
	  {
	  }

		void setPressedFunc(std::function<void()> f);

		void setEnabled(bool enable);

		bool ProcessInput(const InputCompactEvent& event) override;
		void Render(const Transform4x4f& parentTrans) override;

		void setText(const String& text, const String& helpText, bool upperCase = true, bool resize = true, bool doUpdateHelpPrompts = true);
		void autoSizeFont();
		[[nodiscard]] bool isTextOverlapping() const;

		[[nodiscard]] const String& getText() const { return mText; };
		[[nodiscard]] const std::function<void()>& getPressedFunc() const { return mPressedFunc; };

		void onFocusGained() override;
		void onFocusLost() override;

		void setColorShift(unsigned int color) { mModdedColor = color; mNewColor = true; }
		void removeColorShift() { mNewColor = false; }

	  bool CollectHelpItems(Help& help) override;

	  //! Get font
	  [[nodiscard]] NewFont* getFont() const { return mFont; }

	private:
		NewFont* mFont;
		std::function<void()> mPressedFunc;

		bool mFocused;
		bool mEnabled;
		bool mNewColor = false;
		unsigned int mTextColorFocused;
		unsigned int mTextColorUnfocused;
		unsigned int mModdedColor;
		unsigned int mColor;
	  int mTextWidth;
	  int mTextHeight;

		[[nodiscard]] unsigned int getCurTextColor() const;

		String mText;
		String mHelpText;
		NinePatchImage mBoxEmpty;
		NinePatchImage mBoxFilled;

		static Texture GetTexture(bool filled);
};
