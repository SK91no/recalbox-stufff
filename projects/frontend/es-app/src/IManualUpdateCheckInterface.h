//
// Created by bkg2k on 17/04/25.
//
#pragma once

#include <utils/String.h>

class IManualUpdateCheckInterface
{
  public:
    //! Default constructor
    virtual ~IManualUpdateCheckInterface() = default;

    //! Called back when a new version has been checked. The method is guaranteed to be called from the main thread
    virtual void ManualCheckResponse(bool hasUpdate, const String& version) = 0;
};
