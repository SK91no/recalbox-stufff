//
// Created by bkg2k on 03/10/23.
//
#pragma once

#include "BaseOSD.h"
#include "rendering/opengl/Colors.h"
#include "rendering/opengl/Rectangle.h"
#include <RecalboxConf.h>
#include <memory>
#include <rendering/fonts/Font.h>

class BatteryOSD : public BaseOSD
                 , public RecalboxConf::IBatteryHiddenNotification
{
  public:
    /*!
     * @brief Constructor
     * @param window Window manager
     */
    explicit BatteryOSD(WindowManager& window, Side side);

    /*
     * Component override
     */

    //! Update fadings
    void Update(int deltaTime) override;

    //! Draw
    void Render(const Transform4x4f& parentTrans) override;

    /*
     * BaseOSD implementation
     */

    //! Get max width
    [[nodiscard]] int OSDAreaWidth() const override { return (int)mBatteryArea.Width(); }

    //! Get height
    [[nodiscard]] int OSDAreaHeight() const override { return (int)mBatteryArea.Height(); }

    //! Visible?
    [[nodiscard]] bool IsActive() const override { return mHasBattery && mVisible; }

  private:
    //! FPS Font
    NewFont* mFont;
    //! FPS rectangle
    Rectangle mBatteryArea;
    //! Icon
    String::Unicode mIcon;
    //! C olor
    Colors::ColorRGBA mColor;
    //! Has battery?
    bool mHasBattery;
    //! Visible?
    bool mVisible;

    //! Callback when the battery-hidden option changes (typed watcher; value is the new setting)
    void ConfigurationBatteryHiddenChanged(const bool& value) override { mVisible = !value; }
};