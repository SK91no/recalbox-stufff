//
// Created by bkg2k on 10/11/24.
//
#pragma once

#include "IMenuBackgroundTask.h"
#include <utils/os/system/Thread.h>
#include <guis/menus/base/ItemSelector.h>

class MenuTaskRefreshIP : public IMenuBackgroundTask
                        , private ISyncMessageReceiver<bool>
{
  public:
    MenuTaskRefreshIP(ItemText& ip)
      : IMenuBackgroundTask("RefreshIP")
      , mSender(*this)
      , mIP(ip)
    {
    }

  private:
    //! Sender
    SyncMessageSender<bool> mSender;
    //! "Connected" Menu item
    ItemText& mIP;
    //! Last IP
    String mLastIP;
    //! IP Guardian
    Mutex mIPGuard;

    /*
     * Thread
     */

    void TaskRun() final;

    /*
     * Synchronous event
     */

    /*!
     * @brief Receive message from the command thread
     * @param game Game data
     */
    void ReceiveSyncMessage(bool connected) final;
};
