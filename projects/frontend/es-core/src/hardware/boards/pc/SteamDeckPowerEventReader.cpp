//
// Created by bkg2k on 01/11/2020.
// Modified by davidb2111 for the SteamDeck
//

#include "SteamDeckPowerEventReader.h"
#include <utils/Log.h>
#include <linux/input.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <utils/datetime/DateTime.h>
#include <MainRunner.h>

SteamDeckPowerEventReader::SteamDeckPowerEventReader(HardwareMessageSender& messageSender)
  : mSender(messageSender)
  , mFileHandle(0)
  , mWaitFor(WaitFor::Press)
{
}

SteamDeckPowerEventReader::~SteamDeckPowerEventReader()
{
  StopReader();
}

void SteamDeckPowerEventReader::StartReader()
{
  { LOG(LogDebug) << "[SteamDeck] Power button manager requested to start."; }
  Start("SteamDeckPower");
}

void SteamDeckPowerEventReader::StopReader()
{
  { LOG(LogDebug) << "[SteamDeck] Power button manager requested to stop."; }
  Stop();
}

void SteamDeckPowerEventReader::Break()
{
  if (mFileHandle >= 0)
  {
    { LOG(LogDebug) << "[SteamDeck] Breaking power button thread."; }
    mFileHandle = -1;
  }
}

void SteamDeckPowerEventReader::Run()
{
  { LOG(LogInfo) << "[SteamDeck] Running background power button manager."; }
  while(IsRunning())
  {
    mFileHandle = open(sInputEventPath, O_RDONLY);
    if (mFileHandle < 0)
    {
      { LOG(LogError) << "[SteamDeck] Error opening " << sInputEventPath << ". Retry in 5s..."; }
      sleep(5);
      continue;
    }

    //input_event pressEvent {};
    while(IsRunning())
    {
      // Poll
      struct pollfd poller { .fd = mFileHandle, .events = POLLIN, .revents = 0 };
      if (poll(&poller, 1, 100) != 1 || (poller.revents & POLLIN) == 0)
      {
        // If the button is pressed
        if (mWaitFor == WaitFor::Release)
        {
          mSender.Send(BoardType::SteamDeckLCD, MessageTypes::PowerButtonPressed, 20); // steamdeck powerbutton thraws 2 events
          mWaitFor = WaitFor::Ignore; // Ignore releasing the button
        }
        continue;
      }

      // Read event
      struct input_event event {};
      if (read(mFileHandle, &event, sizeof(event)) != sizeof(event))
      {
        close(mFileHandle);
        // Error while the file handle is ok means a true read error
        if (mFileHandle >= 0)
        {
          { LOG(LogError) << "[SteamDeck] Error reading " << sInputEventPath << ". Retrying"; }
          continue;
        }
        // If file handle NOK, we're instructed to quit
        { LOG(LogInfo) << "[SteamDeck] Power event reader ordered to stop."; }
        break;
      }

      // Power button pressed? (Steam Deck automatically send a release event right after)
      if ((event.type == 1) && (event.code == sPowerKeyCode) && (event.value == 0))
      {
        mWaitFor = WaitFor::Release;
      }
    }
  }
}

void SteamDeckPowerEventReader::Suspend()
{
  { LOG(LogInfo) << "[SteamDeck] SUSPEND!"; }
  mWaitFor = WaitFor::Ignore; // Ignore next event when waking up!
                              //
  std::fstream state;
	state.open("/sys/power/state", std::ios::out);
  if (!state)
  {
    LOG(LogError) << "[SteamDeck] Suspend failed:";
  }else
  {
    state << "mem" ;
    state.close();
  }

  { LOG(LogInfo) << "[SteamDeck] WAKEUP!"; }
  mSender.Send(BoardType::SteamDeckLCD, MessageTypes::Resume);
}
