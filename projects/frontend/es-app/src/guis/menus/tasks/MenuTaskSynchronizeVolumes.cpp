//
// Created by bkg2k on 19/12/24.
//

#include "MenuTaskSynchronizeVolumes.h"
#include <audio/AudioController.h>

void MenuTaskSynchronizeVolumes::TaskRun()
{
  while(Running())
  {
    // Check
    switch(mType)
    {
      case VolumeType::System: { int realVolume = AudioController::Instance().GetVolume(); if (realVolume != Math::roundi(mVolumeSlider.Value())) mSender.Send(realVolume); break; }
      case VolumeType::Music: { int realVolume = AudioController::Instance().GetMusicVolume(); if (realVolume != Math::roundi(mVolumeSlider.Value())) mSender.Send(realVolume); break; }
      default: { LOG(LogError) << "[MenuTaskSynchronizeVolumes] Unprocessed volume type"; }
    }
    // Sleep a bit
    for(int i = 1000 / 20; --i >= 0; ) if (Running()) Thread::Sleep(20);
  }
}

void MenuTaskSynchronizeVolumes::ReceiveSyncMessage(int value)
{
  mVolumeSlider.SetValue((float)value, false);
  // Check
  switch(mType)
  {
    case VolumeType::System: { RecalboxConf::Instance().SetAudioVolume(value).Save(); break; }
    case VolumeType::Music: { RecalboxConf::Instance().SetAudioMusicVolume(value).Save(); break; }
    default: { LOG(LogError) << "[MenuTaskSynchronizeVolumes] Unprocessed volume type"; }
  }
}
