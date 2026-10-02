//
// Created by bkg2k on 17/04/23.
//
#pragma once

#include <guis/IGuiDownloaderUpdater.h>
#include "utils/os/system/Thread.h"
#include "views/ViewController.h"

class BaseSystemDownloader : private Thread
                           , ISyncMessageReceiver<void>
{
  protected:
    //! Target system
    SystemData& mSystem;
    //! UI interface
    IGuiDownloaderUpdater& mUpdater;
    //! Stop ASAP?
    bool mStopAsap;

  public:
    //! Default required destructor
    ~BaseSystemDownloader() override = default;

    //! Constructor
    explicit BaseSystemDownloader(SystemData& system, IGuiDownloaderUpdater& updater)
      : mSystem(system)
      , mUpdater(updater)
      , mStopAsap(false)
      , mSender(*this)
    {};


    /*!
     * @brief Start download & install games
     */
    void StartDownload(const char* name)
    {
      // start the thread if not aleady done
      mSystem.PauseFileWatcher();
      Thread::Start(name);
    };

    /*!
     * @brief User cancelled: must quit ASAP!
     */
    void MustExitAsap() { mStopAsap = true; };

  protected:
    /*!
     * @brief Actually download & install
     */
    virtual void DownloadAndInstall() = 0;

    /*!
     * @brief Called once when the process is complete
     * @param stopped true if the process has been stopped by calling MustExitAsap
     */
    virtual void Completed(bool stopped) = 0;

  private:
    SyncMessageSender<void> mSender;

    /*
     * Thread Implementation
     */

    /*!
     * @brief Main thread routine
     */
    void Run() override
    {
      DownloadAndInstall();
      mSender.Send();
    };

    /*
     * ISyncMessageReceiver implementation
     */

    void ReceiveSyncMessage() final
    {
      Completed(mStopAsap);
      mSystem.ResumeFileWatcher();
      if (mSystem.Descriptor().IsPort())
        mSystem.Manager().RegeneratePortsSystem();
      ViewController::Instance().ForceGamelistRefresh(mSystem);
    }
};
