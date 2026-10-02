//
// Created by bkg2k on 19/12/24.
//
#pragma once

#include "IMenuBackgroundTask.h"
#include "audio/IAudioNotification.h"
#include <guis/menus/base/ItemSlider.h>
#include <utils/sync/SyncMessageSender.h>

class MenuTaskSynchronizeVolumes : public IMenuBackgroundTask
                                 , private ISyncMessageReceiver<int>
{
  public:
    //! Volume type
    enum class VolumeType
    {
      System, //!< Global volume
      Music,  //!< Music only
    };

    /*!
     * @brief Constructor
     * @param slider Slider to adjust
     * @param type Volume type to watch
     */
    MenuTaskSynchronizeVolumes(ItemSlider& slider, VolumeType type)
      : IMenuBackgroundTask("RefreshAudio")
      , mSender(*this)
      , mVolumeSlider(slider)
      , mType(type)
    {
    }

  private:
    //! Sender
    SyncMessageSender<int> mSender;
    //! "Connected" Menu item
    ItemSlider& mVolumeSlider;
    //! Volume type
    VolumeType mType;

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
    void ReceiveSyncMessage(int value) final;
};
