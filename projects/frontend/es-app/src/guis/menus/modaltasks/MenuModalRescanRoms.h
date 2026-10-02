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

class MenuModalRescanRoms : private IMenuModalTask<SystemArray, bool>
                               , private ISystemScan
{
  public:
    static void CreateRescanRomsTask(WindowManager& window, SystemManager& systemManager, const SystemArray& systems)
    {
      // Create new instance
      IMenuModalTask* task = new MenuModalRescanRoms(window, systemManager, systems);
      window.pushGui(task);
      task->StartTask();
    }

  private:
    //! System manager reference
    SystemManager& mSystemManager;
    //! System list to refresh
    SystemArray mSystems;
    //! List of added games for optional scraping
    FileData::List mAddedGames;
    //! Total Added
    int mTotalAdded;
    //! Total Removed
    int mTotalRemoved;
    //! Currently added
    int mCurrentAdded;
    //! Currently removed Removed
    int mCurrentRemoved;

    //! Constructor
    explicit MenuModalRescanRoms(WindowManager& window, SystemManager& systemManager, const SystemArray & systems);

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
     * @param result Result state
     */
    void TaskComplete(const bool& result) final;

    /*
     * ISystemScan interface implementation
     */

    /*!
     * @brief Initiate the system scan
     * @param system System being scanned
     */
    void SystemScanStart(const SystemData& system) final;

    /*!
     * @brief Called once per file being scanned during all the scan process
     * @param system System being scanned
     * @param path Game that just has been scanned
     */
    void SystemScanProgress(const SystemData& system, const Path& path) final;

    /*!
     * @brief Called once per added file
     * @param system Target system
     * @param path Fullpath of added game
     * @param game Added game structure
     */
    void SystemScanGameAdded(const SystemData& system, const Path& path, FileData& game) final;

    /*!
     * @brief Called once per removed file
     * @param system Target system
     * @param game Removed game structure
     */
    void SystemScanGameRemoved(const SystemData& system, FileData& game) final;

    /*!
     * @brief Called at the end of scanning process
     * @param system System scanned
     */
    void SystemScanEnd(SystemData& system) final;

    //! Scrape new games
    static void RunScraper(FileData::List games);
};
