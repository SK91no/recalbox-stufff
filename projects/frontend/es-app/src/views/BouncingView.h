//
// Created by Pit64 on 27/03/2026.
//
#pragma once

#include <guis/Gui.h>
#include <components/ImageComponent.h>
#include <Renderer.h>
#include <RecalboxConf.h>

class BouncingView : public Gui
{
  private:
    //! Reference to the window manager
    WindowManager& mWindow;

    //! Logo image
    ImageComponent mImage;

    //! Current position
    float mX;
    float mY;

    //! Speed (pixels per millisecond)
    float mSpeedX;
    float mSpeedY;

    //! Cached image size
    float mImageWidth;
    float mImageHeight;

    //! Available colors for bouncing
    static constexpr unsigned int sColors[] =
    {
      0xFF0000FF, // Red
      0x00FF00FF, // Green
      0x0000FFFF, // Blue
      0xFFFF00FF, // Yellow
      0xFF00FFFF, // Purple
      0x00FFFFFF, // Cyan
      0xFF8800FF, // Orange
      0x88FF00FF, // Lime
      0x0088FFFF, // Sky blue
      0xFF0088FF, // Pink
    };

    //! Pick a random color and apply it to the image
    void PickRandomColor();

  public:
    explicit BouncingView(WindowManager& window);
    ~BouncingView() override = default;

    /*
     * GUI overrides
     */
    void Update(int elapsed) override;
    void Render(const Transform4x4f& parentTrans) override;
    bool ProcessInput(const InputCompactEvent& event) override;
    bool CollectHelpItems(Help& help) override;

    static bool IsBouncingEnabled() { return RecalboxConf::Instance().GetScreenSaverType() == RecalboxConf::Screensaver::Bouncing; }

    //! Reinit the view
    void Reset();
};