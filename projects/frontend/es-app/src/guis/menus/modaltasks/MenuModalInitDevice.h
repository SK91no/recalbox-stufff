//
// Created by bkg2k on 25/11/24.
//
#pragma once

#include "IMenuModelTask.h"
#include "utils/os/system/IThreadPoolWorkerInterface.h"
#include "utils/os/system/Signal.h"
#include "utils/os/system/ThreadPool.h"
#include "hardware/devices/DeviceInitializationParameters.h"

//! Forward declarations
class SystemManager;
class WindowManager;

class MenuModalInitDevice : private IMenuModalTask<DeviceInitializationParameters, bool>
{
  public:
    static void CreateDeviceInitializer(WindowManager& window, const DeviceMount& device, USBInitializationAction action)
    {
      // Create new instance
      IMenuModalTask* task = new MenuModalInitDevice(window, device, action);
      window.pushGui(task);
      task->StartTask();
    }

  private:
    //! Constructor
    explicit MenuModalInitDevice(WindowManager& window, const DeviceMount& device, USBInitializationAction action);

    /*
     * Task/Long operations
     */

    /*!
     * @brief Execture network operation
     * @param parameter Network operation required to execute
     * @return True if the operation was successful
     */
    bool TaskExecute(const DeviceInitializationParameters& parameter) final;

    /*!
     * @brief Receive the status of network operations
     * @param parameter original input parameter
     * @param result Result state
     */
    void TaskComplete(const bool& result) final;
};
