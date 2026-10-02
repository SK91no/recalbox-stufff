//
// Created by bkg2k on 13/11/24.
//
#pragma once

#include <WindowManager.h>
#include "IMenuModelTask.h"
#include "guis/menus/base/ItemSwitch.h"

class MenuModalTaskWIFI : private IMenuModalTask<bool, bool>
{
  public:
    static void CreateWIFITask(WindowManager& window, bool activate)
    {
      // Create new instance
      IMenuModalTask* task = new MenuModalTaskWIFI(window, activate);
      window.pushGui(task);
      task->StartTask();
    }

  private:
    //! Activate?
    bool mActivate;

    //! Constructor
    MenuModalTaskWIFI(WindowManager& window, bool activate);

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
