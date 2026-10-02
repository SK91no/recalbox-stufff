//
// Created by bkg2k on 12/11/24.
//
#pragma once

#include "IMenuModelTask.h"
#include "utils/os/system/IThreadPoolWorkerInterface.h"
#include "utils/os/system/Signal.h"
#include "utils/os/system/ThreadPool.h"

//! Forward declarations
class SystemManager;
class WindowManager;

class MenuModalHashRom : private IMenuModalTask<bool, bool>
                       , private IThreadPoolWorkerInterface<FileData*, FileData*>
{
  public:
    static void CreateRomHasher(WindowManager& window, SystemManager& systemManager)
    {
      // Create new instance
      IMenuModalTask* task = new MenuModalHashRom(window, systemManager);
      window.pushGui(task);
      task->StartTask();
    }

  private:
    //! Constructor
    MenuModalHashRom(WindowManager& window, SystemManager& systemManager);

    //! System manager reference
    SystemManager& mSystemManager;
    //! Thread pool for // CRC computations
    ThreadPool<FileData*, FileData*> mThreadPool;
    //! Event
    Signal mWaiter;
    //! Syncher
    Mutex mSyncher;

    //! Total games
    int mTotalGames;
    //! Remaining games
    int mRemainingGames;
    //! Sored progress
    int mPreviousProgressPercent;

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

    /*
     * Thread pool
     */

    /*!
     * @brief The main runner. Implement here the task to process a feed object
     * @param feed Feed object to process
     * @return Result Object
     */
    FileData* ThreadPoolRunJob(FileData*& feed) final;
};
