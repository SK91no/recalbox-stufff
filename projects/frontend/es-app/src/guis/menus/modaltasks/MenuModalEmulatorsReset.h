//
// Created by bkg2k on 25/11/24.
//
#pragma once

#include "IMenuModelTask.h"
#include "utils/os/system/IThreadPoolWorkerInterface.h"
#include "utils/os/system/Signal.h"
#include "utils/os/system/ThreadPool.h"
#include "hardware/devices/DeviceInitializationParameters.h"
#include "guis/GuiMsgBox.h"

//! Forward declarations
class SystemManager;
class WindowManager;

class MenuModalEmulatorsReset : private IMenuModalTask<bool, bool>
{
  public:
    static void InitiateEmulatorsReset(WindowManager* window)
    {
      auto DoIt = [window]
      {
        IMenuModalTask* task = new MenuModalEmulatorsReset(*window);
        window->pushGui(task);
        task->StartTask();
      };
      auto ReallyWant = [window, DoIt]
      {
        String text = (_F("\u26a0 {0} \u26a0\n\n{1}") / _("WARNING!") / _("YOU'RE ONE CLICK AWAY FROM RESETTING YOUR EMULATOR SETTINGS TO FACTORY DEFAULTS!\n\nARE YOU REALLY SURE YOU WANT TO DO THIS?"))();
        window->pushGui(new GuiMsgBox(*window, text, _("NO"), nullptr, _("YES"), DoIt ));
      };
      window->pushGui(new GuiMsgBox(*window, _("RESET EMULATOR SETTINGS\n\nYOU'RE ABOUT TO RESET ALL EMULATOR SETTINGS TO THEIR DEFAULT VALUES.\nYOU RECALBOX SETTINGS AND FILES WON'T BE AFFECTED.\n\nTHIS OPERATION CANNOT BE UNDONE!\nARE YOU SURE YOU WANT TO RESET ALL YOUR EMULATOR SETTINGS?"),
                                   _("NO"), nullptr, _("YES"), ReallyWant));
    }

  private:
    //! Constructor
    explicit MenuModalEmulatorsReset(WindowManager& window)
      : IMenuModalTask<bool, bool>(window, _("EMULATOR SETTINGS RESET IN PROGRESS"), false)
    {}

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