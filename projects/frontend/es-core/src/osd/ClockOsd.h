//
// Created by bkg2k on 14/10/24.
//
#pragma once

#include <memory>
#include "BaseOSD.h"
#include "Options.h"
#include <RecalboxConf.h>
#include <rendering/fonts/Font.h>
#include <rendering/opengl/Rectangle.h>

class ClockOSD : public BaseOSD
{
  public:
    /*!
     * @brief Constructor
     * @param window Window manager
     */
    explicit ClockOSD(WindowManager& window, Side side);

    /*
     * Component override
     */

    //! Draw
    void Render(const Transform4x4f& parentTrans) override;

    /*
     * BaseOSD implementation
     */

    //! Get max width
    [[nodiscard]] int OSDAreaWidth() const override { return (int)mClockArea.Width(); }

    //! Get height
    [[nodiscard]] int OSDAreaHeight() const override { return (int)mClockArea.Height(); }

    //! Visible?
    [[nodiscard]] bool IsActive() const override { return RecalboxConf::Instance().GetClock(); };

  private:
    //! FPS Font
    NewFont* mClockFont;
    //! FPS rectangle
    Rectangle mClockArea;

    // Return formatted string of current clock
    String Clock();
};
