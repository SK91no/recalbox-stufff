//
// Created by davidb2111 on 31/08/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include "guis/menus/base/Menu.h"
#include "guis/GuiWaitLongExecution.h"

// Forward declaration
class SystemManager;
class SystemData;

class MenuSoundPair : public Menu
                       , ILongExecution<String, bool>
                       , IActionTriggered
{
  public:
    /*!
     * @brief Default constructor
     * @param window Global window
     */
    explicit MenuSoundPair(WindowManager& window, const String::List& deviceList);

    //! Build menu items
    void BuildMenuItems() final;

  private:
    enum class Components
    {
      Devices,
    };

    //! Device list
    String::List mDevices;

    /*
     * ILongExecution implementation
     */

    /*!
     * @brief Try to pair a device
     * @param from Source interface
     * @param parameter Device to pair
     * @return True if the device is paired, false otherwise
     */
    bool Execute(GuiWaitLongExecution<String, bool>& from, const String& parameter) override;

    /*!
     * @brief Process result of pairing
     * @param parameter Device to paired
     * @param result Pairing result
     */
    void Completed(const String& parameter, const bool& result) override;

    /*
     * ISubMenuSelected implementation
     */

    void MenuActionTriggered(ItemAction& item, int id) override;
};
