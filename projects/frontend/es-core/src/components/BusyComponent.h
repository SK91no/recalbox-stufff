#pragma once

#include <utils/os/system/Mutex.h>
#include <components/base/Component.h>
#include <components/ComponentGrid.h>
#include <components/NinePatchComponent.h>
#include <components/TextComponent.h>
#include "ProgressBarComponent.h"

class AnimatedImageComponent;

class BusyComponent : public Component
{
  public:
    explicit BusyComponent(WindowManager&window);
    ~BusyComponent() override = default;

    void onSizeChanged() override;
    String getText();

    void Render(const Transform4x4f& parentTrans) override;

    [[nodiscard]] float RealWidth() const { return mFrame.getSize().x(); }
    [[nodiscard]] float RealHeight() const { return mFrame.getSize().y(); }

    void Enable(bool enable) { mEnabled = enable; }

  void SetFont(NewFont* font)
  {
    mText->setFont(font);
    onSizeChanged();
  }

    /*!
     * @brief Set main text
     * @param txt Text to set
     */
    void SetText(const String& txt, bool keepHighestWidth = false);

    /*!
     * @brief Set secondary animation text
     * @param text text to set
     */
    void SetSecondaryText(const String& txt);

    /*!
     * @brief Set optionnal progress bar value and make the progress bar to show
     * @param progress Progress value
     */
    void SetProgress(int progress);

  private:
    NinePatchComponent mFrame;
    ComponentGrid mGrid;

    std::shared_ptr<AnimatedImageComponent> mAnimation;
    std::shared_ptr<TextComponent> mText;
    std::shared_ptr<TextComponent> mSubText;
    std::shared_ptr<ProgressBarComponent> mProgress;

    Mutex mMutex;
    String mThreadMessage;
    String mThreadMessage2;
    float mHighestWidth;
    bool mThreadMessagechanged;
    bool mEnabled;
    bool mKeepHighestWidth;
    bool mShowProgressBar;
};