//
// Created by bkg2k on 11/03/25.
//
#pragma once

#include <utils/storage/Array.h>
#include "IMenuModelTask.h"
#include "systems/ISystemScan.h"

class SystemData;
class WindowManager;
class SystemManager;
typedef Array<SystemData*> SystemArray;

class MenuModalRefreshGamelist : private IMenuModalTask<SystemArray, bool>
{
  public:
    static void CreateRefreshGamelistTask(WindowManager& window, SystemManager& systemManager, const SystemArray& systems)
    {
      // Create new instance
      IMenuModalTask* task = new MenuModalRefreshGamelist(window, systemManager, systems);
      window.pushGui(task);
      task->StartTask();
    }

    //! Scrape new games
    static void RunScraper(WindowManager& window, SystemManager& systemManager, FileData::List games);

  private:
    //! System manager reference
    SystemManager& mSystemManager;
    //! System list to refresh
    SystemArray mSystems;

    //! Constructor
    explicit MenuModalRefreshGamelist(WindowManager& window, SystemManager& systemManager, const SystemArray & systems);

    /*
     * Task/Long operations
     */

    /*!
     * @brief Execture network operation
     * @param parameter Network operation required to execute
     * @return True if the operation was successful
     */
    bool TaskExecute(const SystemArray& parameter) final;

    /*!
     * @brief Receive the status of network operations
     * @param parameter original input parameter
     * @param result Result state
     */
    void TaskComplete(const bool& result) final;
};
