//
// Created by bkg2k on 10/11/24.
//
#pragma once

#include "IMenuBackgroundTask.h"
#include <guis/menus/base/ItemSelector.h>

class MenuTaskRefreshConnection : public IMenuBackgroundTask
                                , private ISyncMessageReceiver<bool>
{
  public:
    MenuTaskRefreshConnection(ItemText& connected)
      : IMenuBackgroundTask("RefreshCnct")
      , mSender(*this)
      , mConnected(connected)
      , mLastPingStatus(false)
    {
    }

  private:
    //! Sender
    SyncMessageSender<bool> mSender;
    //! "Connected" Menu item
    ItemText& mConnected;
    //! Last ping status
    bool mLastPingStatus;

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
