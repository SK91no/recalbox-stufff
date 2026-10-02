//
// Created by bkg2k on 18/12/24.
//

#include "MenuTaskRefreshAudioOutputs.h"
#include "audio/AudioController.h"

void MenuTaskRefreshAudioOutputs::TaskRun()
{
  // Enable audio change notiofications
  AudioController::Instance().EnableNotification();
  AudioController::Instance().SetNotificationCallback(this);

  // Do nothing
  while(Running()) Thread::Sleep(20);

  // Disable audio change notiofications
  AudioController::Instance().DisableNotification();
  AudioController::Instance().ClearNotificationCallback();
}

void MenuTaskRefreshAudioOutputs::NotifyAudioChange()
{
  mSender.Send();
}

void MenuTaskRefreshAudioOutputs::ReceiveSyncMessage()
{
  // Get new autio output list
  SelectorEntry<String>::List list;
  for(const DeviceDescriptor& playback : AudioController::Instance().GetPlaybackList())
    list.push_back({ playback.DisplayableName, playback.InternalName });

  // Refresh menu item
  mAudioOutputs.ChangeSelectionItems(list);
}
