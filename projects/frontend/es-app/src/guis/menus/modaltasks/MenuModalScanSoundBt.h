//
// Created by bkg2k on 20/12/24.
//
#pragma once

#include <utils/String.h>
#include "IMenuModelTask.h"
#include "utils/locale/LocaleHelper.h"

class MenuModalScanSoundBT : private IMenuModalTask<bool, String::List>
{
  public:
    static void CreateScanSoundBT(WindowManager& window)
    {
      // Create new instance
      IMenuModalTask* task = new MenuModalScanSoundBT(window);
      window.pushGui(task);
      task->StartTask();
    }

  private:
    //! Constructor
    explicit MenuModalScanSoundBT(WindowManager& window)
      : IMenuModalTask<bool, String::List>(window, _("DISCOVERING BLUETOOTH AUDIO DEVICES..."), false)
    {}

    /*
     * Task/Long operations
     */

    /*!
     * @brief Execture network operation
     * @param parameter Network operation required to execute
     * @return True if the operation was successful
     */
    String::List TaskExecute(const bool& parameter) final;

    /*!
     * @brief Receive the status of network operations
     * @param parameter original input parameter
     * @param result Result state
     */
    void TaskComplete(const String::List& result) final;
};
