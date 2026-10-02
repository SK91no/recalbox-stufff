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

class MenuModalFactoryReset : private IMenuModalTask<bool, bool>
{
  public:
    static void InitiateFactoryReset(WindowManager* window)
    {
      auto DoIt = [window]
      {
        IMenuModalTask* task = new MenuModalFactoryReset(*window);
        window->pushGui(task);
        task->StartTask();
      };
      auto ReallyWant = [window, DoIt]
      {
        String text = (_F("\u26a0 {0} \u26a0\n\n{1}") / _("WARNING!") / _("YOU'RE ONE CLICK AWAY FROM RESETTING YOUR RECALBOX TO FACTORY SETTINGS!\n\nARE YOU REALLY SURE YOU WANT TO DO THIS?"))();
        window->pushGui(new GuiMsgBox(*window, text, _("NO"), nullptr, _("YES"), DoIt ));
      };
      window->pushGui(new GuiMsgBox(*window, _("RESET TO FACTORY SETTINGS\n\nYOU'RE ABOUT TO RESET RECALBOX AND EMULATOR SETTINGS TO DEFAULT VALUES.\nALL YOUR DATA LIKE GAMES, SAVES, MUSIC, SCREENSHOTS AND SO ON, WILL BE KEPT.\n\nTHIS OPERATION CANNOT BE UNDONE!\nARE YOU SURE YOU WANT TO RESET ALL YOUR SETTINGS?"),
                                   _("NO"), nullptr, _("YES"), ReallyWant));
    }

    /*!
     * @brief Reinstall files that have been kept after a reset factory
     */
    static void ReinstallFilesAfterReboot();

  private:
    //! Flag path
    static Path sFlagPath;
    //! Static map of file being saved before a reset factory for reinstallation
    static HashMap<Path, Path> sBackups;

    //! Constructor
    explicit MenuModalFactoryReset(WindowManager& window)
      : IMenuModalTask<bool, bool>(window, _("FACTORY RESET IN PROGRESS"), false)
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
    void TaskComplete(const bool& result) final { (void)result; }
};