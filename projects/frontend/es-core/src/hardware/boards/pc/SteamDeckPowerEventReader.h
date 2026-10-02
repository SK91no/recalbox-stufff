//
// Created by bkg2k on 01/11/2020.
// Modified by davidb2111 for the SteamDeck
//
#pragma once

#include <utils/os/system/Thread.h>
#include <hardware/messaging/HardwareMessageSender.h>

class SteamDeckPowerEventReader : private Thread
{
  public:
    //! Constructor
    explicit SteamDeckPowerEventReader(HardwareMessageSender& messageSender);

    ~SteamDeckPowerEventReader() override;

    //! Start reading the power events
    void StartReader();

    //! Stop reading power events
    void StopReader();

  private:
    friend class SteamDeck;

    //! Wait for specific button state
    enum class WaitFor
    {
        Press,   //!< Waiting for a press event
        Release, //!< Waiting for a release event
        Ignore,  //!< Ignore next event
    };

    //! Event to read
    static constexpr const char* sInputEventPath = "/dev/input/event2";
    //! Power key code
    static constexpr int sPowerKeyCode = 116;

    //! Message sender
    HardwareMessageSender& mSender;
    //! File handle
    int mFileHandle;
    //! Wait event
    WaitFor mWaitFor;

    //! Break the thread
    void Break() override;

    //! Event reader
    void Run() override;

    //! Suspend
    void Suspend();
};
