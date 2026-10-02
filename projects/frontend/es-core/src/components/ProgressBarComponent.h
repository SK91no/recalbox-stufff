#pragma once

#include "components/base/Component.h"
#include <functional>
#include "components/NinePatchComponent.h"
#include <utils/String.h>
#include <rendering/fonts/Font.h>

class ProgressBarComponent : public Component
{
  private:
    NewFont* mFont;

    long long mMaxValue;
    long long mCurrentValue;

    unsigned int mEmptyColor;
    unsigned int mFillColor;
    unsigned int mTextColor;

  public:
    ProgressBarComponent(WindowManager&window, long long maxvalue);

	  void setMaxValue(long long maxvalue)
    {
	    if (maxvalue < 1) maxvalue = 1;
	    if (maxvalue < mCurrentValue) maxvalue = mCurrentValue;
	    mMaxValue = maxvalue;
    }

    void setCurrentValue(long long value)
    {
	    if (value < 0) value = 0;
	    if (value > mMaxValue) value = mMaxValue;
	    mCurrentValue = value;
    }

    [[nodiscard]] long long getMaxValue() const { return mMaxValue; }

	  [[nodiscard]] long long getCurrentValue() const { return mCurrentValue; }

	  [[nodiscard]] String getText() const { return String((float)mCurrentValue * 100.0f / (float)mMaxValue, 2).Append('%'); }

    void Update(int deltaTime) override;
    void Render(const Transform4x4f& parentTrans) override;
};
