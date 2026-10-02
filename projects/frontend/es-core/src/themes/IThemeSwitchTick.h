//
// Created by bkg2k on 03/01/24.
//
#pragma once

class IThemeSwitchTick
{
  public:
    //! desfault destructor
    virtual ~IThemeSwitchTick() = default;

    /*!
     * @brief Called by IThemeSwitchable implementation every time they update something slow
     * This is used by the theme manager to update wait bars while switching theme
     */
    virtual void ThemeSwitchTick() = 0;
};