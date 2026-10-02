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
#include "IManualUpdateCheckInterface.h"
#include "guis/menus/MenuBuilder.h"

//! Forward declarations
class SystemManager;
class WindowManager;

class MenuModalUpdateCheck : private IMenuModalTask<bool, bool>
                           , public  IManualUpdateCheckInterface
{
  public:
    static void InitiateManualUpdateCheck(WindowManager& window, MenuBuilder& sourceMenu)
    {
      IMenuModalTask* task = new MenuModalUpdateCheck(window, sourceMenu);
      window.pushGui(task);
      task->StartTask();
    }

  private:
    //! Source menu
    MenuBuilder& mSourceMenu;
    //! Signal
    Signal mSignal;
    //! Update status
    bool mUpdateStatus;
    //! Version
    String mVersion;

    //! Constructor
    explicit MenuModalUpdateCheck(WindowManager& window, MenuBuilder& sourceMenu)
      : IMenuModalTask<bool, bool>(window, _("CHECKING UPDATE..."), false)
      , mSourceMenu(sourceMenu)
      , mUpdateStatus(false)
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

    /*
     * IManualUpdateCheckInterface implementation
     */

    /*!
     * @brief Called when the update check ends
     * @param hasUpdate True if an update is available
     * @param version Update version or current version if no update is available
     */
    void ManualCheckResponse(bool hasUpdate, const String& version) final;
};