//
// Created by bkg2k on 10/11/24.
//
#pragma once

#include "utils/os/system/Thread.h"
#include "IMenuBackgroundTask.h"
#include "guis/menus/base/ItemSelector.h"
#include "network/IWifiHotspotListChanged.h"
#include "network/Networks.h"

class MenuTaskRefreshSSID : public IMenuBackgroundTask
                          , private ISyncMessageReceiver<void>
                          , private IWifiHotspotListChanged
                          , private RecalboxConf::IWifiEnabledNotification
{
  public:
    MenuTaskRefreshSSID(ItemSelector<String>& ssids)
      : IMenuBackgroundTask("RefreshSSID")
      , mSender(*this)
      , mSSIDs(ssids)
    {
      // Initialize list with current scan results
      mLastList = Networks::Instance().HotspotList();
      mSender.Send();
    }

  private:
    //! Sender
    SyncMessageSender<void> mSender;
    //! SSID list item
    ItemSelector<String>& mSSIDs;
    //! Last list
    std::vector<Networks::HotSpot> mLastList;
    //! Last list guardian
    Mutex mListGuard;

    /*
     * IMenuBackgroundTask
     */

    void TaskRun() final;

    /*
     * Synchronous event
     */

    /*!
     * @brief Receive message from the command thread
     * @param game Game data
     */
    void ReceiveSyncMessage() final;

    /*
     * IWifiHotspotListChanged implementation
     */

    /*!
     * @brief Tell the callee the WIFI hotspot list has changed
     * New hotspot list is available in the Wifi class
     */
    void WifiHotspotListChanged(Networks& wifi) final { mLastList = wifi.HotspotList(); mSender.Send(); }

    /*
     * RecalboxConf::IWifiEnabledNotification implementation
     */

    void ConfigurationWifiEnabledChanged(const bool& value) override { (void)value; mSender.Send(); }
};
