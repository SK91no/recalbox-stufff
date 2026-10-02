//
// Created by bkg2k on 16/12/24.
//
#pragma once

class Networks;

class IWifiHotspotListChanged
{
  public:
    //! destructor
    virtual ~IWifiHotspotListChanged() = default;

    /*!
     * @brief Tell the callee the WIFI hotspot list has changed
     * New hotspot list is available in the Wifi class
     */
    virtual void WifiHotspotListChanged(Networks& wifi) = 0;
};