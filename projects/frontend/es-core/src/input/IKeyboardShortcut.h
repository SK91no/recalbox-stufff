//
// Created by bkg2k on 16/12/23.
//
#pragma once

class IKeyboardShortcut
{
  public:
    //! possible shortcut
    enum class Shortcut
    {
      Quit,
      Refresh,
      Search,
      MenuTemplate,
      MenuReload,
    };

    //! default destructor
    virtual ~IKeyboardShortcut() = default;

    /*!
     * @brief Called when a shortcut has been pressed and released
     * @param shortcut Shortcut type
     */
    virtual void ShortcutTriggered(Shortcut shortcut) = 0;
};
