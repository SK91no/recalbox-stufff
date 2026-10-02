//
// Created by bkg2k on 25/11/24.
//
#pragma once

#include <utility>

#include "hardware/devices/mount/MountDevice.h"

//! USB Initialization parameters
enum class USBInitializationAction
{
  None,           //! Do nothing
  OnlyRomFolders, //!< Only make roms folder for rom multi-sources
  CompleteShare,  //!< Initialize (copy) the whole current configuration to move to external share
};

class DeviceInitializationParameters
{
  public:
    //! Default constructor
    DeviceInitializationParameters(USBInitializationAction action, const DeviceMount& device)
      : mAction(action)
      , mDevice(device)
    {
    }

    //! Default constructor
    DeviceInitializationParameters()
      : mAction(USBInitializationAction::None)
      , mDevice()
    {
    }

    DeviceInitializationParameters(const DeviceInitializationParameters&) = default;
    DeviceInitializationParameters(DeviceInitializationParameters&&) = default;
    DeviceInitializationParameters& operator=(const DeviceInitializationParameters&) = default;
    DeviceInitializationParameters& operator=(DeviceInitializationParameters&&) = default;

    //! Get action
    [[nodiscard]] USBInitializationAction Action() const { return mAction; }

    //! Get device
    [[nodiscard]] const DeviceMount& Device() const { return mDevice; }

  private:
    //! Action to execute on the device
    USBInitializationAction mAction;
    // Device to initialize
    DeviceMount mDevice;
};

