//
// Created by bkg2k on 20/01/2026.
//
#pragma once
#include <input/InputDevice.h>

class IDeviceConfigured
{
  public:
    //! Default destructor
    virtual ~IDeviceConfigured() = default;

    /*!
     * @brief Called when a device has been configured
     * @param device Configured device
     */
    virtual void DeviceConfigured(const InputDevice& device) = 0;
};
