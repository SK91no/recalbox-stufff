//
// Created by bkg2k on 02/10/23.
//
#pragma once

#include <memory>
#include "BaseOSD.h"
#include <RecalboxConf.h>
#include <input/InputMapper.h>
#include <rendering/fonts/Font.h>

class PadOSD : public BaseOSD
             , public RecalboxConf::IPadOSDNotification
{
  public:
    /*!
     * @brief Constructor
     * @param window Window manager
     */
    explicit PadOSD(WindowManager& window, Side side);

    //! Update pad icon on change
    void UpdatePadIcon();

    /*!
     * @brief Force OSD activation whatever user configuration says
     * @param forced True to force activation
     */
    void ForcedPadOSDActivation(bool forced) { mForcedActive = forced; UpdateActiveFlag(); }

    /*
     * Component override
     */

    //! Process input
    bool ProcessInput(const InputCompactEvent& event) override;

    //! Update fadings
    void Update(int deltaTime) override;

    //! Draw
    void Render(const Transform4x4f& parentTrans) override;

    /*
     * BaseOSD implementation
     */

    //! Get max width
    [[nodiscard]] int OSDAreaWidth() const override;

    //! Get height
    [[nodiscard]] int OSDAreaHeight() const override;

    //! Visible?
    [[nodiscard]] bool IsActive() const override { return mActive; };

  private:
    //! Pad color
    static constexpr unsigned int sColor = 0xFFFFFF00;
    //! Maximum Alpha when a pad is "activated"
    static constexpr int sMaxAlpha = 255;
    //! Minimum alpha when a pad is idle
    static constexpr int sMinAlpha = 64;

    //! Mapper reference
    InputMapper& mMapper;

    //! Pad Font
    NewFont* mFont;
    //! Pad Alpha
    int mAlpha[Input::sMaxInputDevices];
    //! Pad unicode char
    String::Unicode mPadChar;
    //! Pad count
    int mPadCount;
    //! Active?
    bool mActive;
    //! Forced Active?
    bool mForcedActive;

    //! Callback when the pad OSD option changes (typed watcher)
    void ConfigurationPadOSDChanged(const bool& value) override { (void)value; UpdateActiveFlag(); }

    //! Update mActive
    void UpdateActiveFlag();
};
