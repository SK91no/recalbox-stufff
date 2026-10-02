//
// Created by bkg2k on 15/12/23.
//
#pragma once

#include "IThemeSwitchTick.h"

// Forward declaration
class SystemData;
class ThemeData;
class ThemeSwitcher;

class IThemeSwitchable
{
  public:
    //! Constructor
    IThemeSwitchable();

    //! Default destructor
    virtual ~IThemeSwitchable();

    /*!
     * @brief Implementation instruct the Theme switcher to use a specific system theme
     * Returning nullptr means the global theme will be used
     * @return SystemData or nullptr
     */
    [[nodiscard]] virtual SystemData* SystemTheme() const = 0;

    /*!
     * @brief Called when theme switch.
     * @param theme New Theme
     * @param refreshOnly True if the theme dit not change and the implementation must refresh only (i.e: apply new values)
     */
    virtual void SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface) = 0;
};
