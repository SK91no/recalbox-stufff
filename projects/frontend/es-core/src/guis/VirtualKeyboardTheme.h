//
// Resolved styling for the virtual keyboards, read from the theme element
// "keyboard" of a given view. Each field defaults to the value set by the
// keyboard before LoadFromView() and is overridden only when the theme provides
// it. Sizes are never themed (always decided by the keyboard).
//
#pragma once

#include <utils/String.h>
#include <utils/os/fs/Path.h>

class ThemeData;

class VirtualKeyboardTheme
{
  public:
    unsigned int KeyColor               = 0x303030F0u; //!< Key background
    unsigned int KeySelectedColor       = 0xC02020FFu; //!< Selected key background
    unsigned int KeyTextColor           = 0xFFFFFFFFu; //!< Key glyph / char text
    unsigned int KeyDisabledColor       = 0x707070FFu; //!< Disabled key text
    unsigned int KeyModifierColor       = 0x4488FFFFu; //!< Modifier active once (classic)
    unsigned int KeyModifierLockedColor = 0x224488FFu; //!< Modifier locked (classic)
    unsigned int TitleColor             = 0xFFFF80FFu; //!< Title text
    unsigned int EditTextColor          = 0xFFFFFFFFu; //!< Edited text
    Path         FontPath;                             //!< Theme font face (empty = none)
    bool         HasFont = false;                      //!< True if the theme provides a font face

    /*!
     * @brief Override fields from the "keyboard" element of the given view, using
     * the provided theme. Missing properties keep their current value. No sizes.
     * @param theme Theme to read from (main theme for the keyboards)
     * @param view Theme view name the keyboard is shown over
     */
    void LoadFromTheme(const ThemeData& theme, const String& view);

    /*!
     * @brief Convenience overload using the current main theme.
     * @param view Theme view name the keyboard is shown over
     */
    void LoadFromView(const String& view);
};
