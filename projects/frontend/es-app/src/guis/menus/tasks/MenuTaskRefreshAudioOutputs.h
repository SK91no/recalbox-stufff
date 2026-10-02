//
// Created by bkg2k on 18/12/24.
//
#pragma once

#include "IMenuBackgroundTask.h"
#include "audio/IAudioNotification.h"
#include <guis/menus/base/ItemSelector.h>

class MenuTaskRefreshAudioOutputs : public IMenuBackgroundTask
                                  , private ISyncMessageReceiver<void>
                                  , private IAudioNotification
{
  public:
    MenuTaskRefreshAudioOutputs(ItemSelector<String>& audioOutputs)
      : IMenuBackgroundTask("RefreshAudio")
      , mSender(*this)
      , mAudioOutputs(audioOutputs)
    {
    }

  private:
    //! Sender
    SyncMessageSender<void> mSender;
    //! "Connected" Menu item
    ItemSelector<String>& mAudioOutputs;

    /*
     * Thread
     */

    void TaskRun() final;

    /*
     * Synchronous event
     */

    /*!
     * @brief Receive message from the command thread
     */
    void ReceiveSyncMessage() final;

    /*
     * Audio notification
     */

    /*!
     * @brief Called from PulseAudioController whenever a sink is added or removed
     */
    void NotifyAudioChange() final;
};
