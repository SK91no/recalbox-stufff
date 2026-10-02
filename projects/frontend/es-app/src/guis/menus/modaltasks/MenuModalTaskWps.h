//
// Created by bkg2k on 13/11/24.
//
#pragma once

#include <guis/menus/base/ItemEditable.h>
#include <guis/menus/base/ItemSwitch.h>
#include <WindowManager.h>
#include "IMenuModelTask.h"

class MenuModalTaskWPS : private IMenuModalTask<bool, bool>
{
  public:
    static void CreateWPS(WindowManager& window, ItemEditable& ssid, ItemEditable& password, ItemSwitch& connect)
    {
      // Create new instance
      IMenuModalTask* task = new MenuModalTaskWPS(window, ssid, password, connect);
      window.pushGui(task);
      task->StartTask();
    }

  private:
    //! Constructor
    MenuModalTaskWPS(WindowManager& window, ItemEditable& ssid, ItemEditable& password, ItemSwitch& connect);

    //! SSID editor item
    ItemEditable& mSSID;
    //! Password editor item
    ItemEditable& mPassword;
    //! Password editor item
    ItemSwitch& mConnect;
    //! WPS returned SSID
    String mSSISString;
    //! WPS returned PSK
    String mPasswordString;

    /*
     * Task/Long operations
     */

    /*!
     * @brief Execture network operation
     * @param parameter Network operation required to execute
     * @return True if the operation was successful
     */
    bool TaskExecute(const bool& parameter) final;

    /*!
     * @brief Receive the status of network operations
     * @param parameter original input parameter
     * @param result Result state
     */
    void TaskComplete(const bool& result) final;
};
