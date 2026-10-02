//
// Created by bkg2k on 21/11/2019.
//

#pragma once

#include <WindowManager.h>
#include <components/TextComponent.h>
#include "IProgressInterface.h"
#include <systems/ISystemLoadingPhase.h>

class SplashView : public Gui
                 , public IProgressInterface
                 , public ISystemLoadingPhase
{
  private:
    ImageComponent mLogo;
    TextComponent  mLoading;

    int mSystemCount;
    int mSystemLoaded;

    //! RGB Dual identified?
    bool mIsRecalboxRGBHat;
    Colors::ColorRGBA mBgColor;

    //! Darkmode
    Colors::ColorRGBA mDarkModeTextColor;
    Colors::ColorRGBA mDarkModeBgColor;
    Colors::ColorRGBA mDarkModeLoadingBarColor;
    Colors::ColorRGBA mDarkModeLoadingBarBgColor;
    bool mIsDarkMode;

  public:
    /*!
     * @brief Constructor
     * @param window Main Window instance
     */
    explicit SplashView(WindowManager& window);

    /*
     * IComponent implementation
     */

    /*!
     * @brief Called once per frame, after Update.
     * @param parentTrans Transformation
     */
    void Render(const Transform4x4f& parentTrans) final;

    /*!
     * @brief Fill in the help object
     * @param help Help object
     * @return true if the help ha sbeen filed
     */
    bool CollectHelpItems(Help& help) final;

    /*
     * ISystemLoadingPhase implementation
     */

    /*!
     * @brief System loading phase callback
     * @param phase Phase
     */
    void SystemLoadingPhase(Phase phase) override;

    /*
     * IProgressInterface implementation
     */

    /*!
     * @brief Set maximum value of the progress indicator
     * @param maximum maximum value
     */
    void SetMaximum(int maximum) override;

    /*!
     * @brief Update the progress indicator vith the current value
     * @param value curren tprogress value
     */
    void SetProgress(int value) override;

    /*!
     * @brief Increment the Progress value by 1.
     */
    void Increment() override;

    /*!
     * @brief Switch the splash into exit screen
     */
    void Quit();
};
