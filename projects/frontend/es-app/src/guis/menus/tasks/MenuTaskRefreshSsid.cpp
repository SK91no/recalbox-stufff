//
// Created by bkg2k on 10/11/24.
//

#include "MenuTaskRefreshSsid.h"
#include "recalbox/RecalboxSystem.h"

void MenuTaskRefreshSSID::TaskRun()
{
  // Register to receive scan message
  Networks::Instance().Register(this);
  Networks::Instance().SetScan(true);

  // Just wait for the thread to end
  while(Running()) Thread::Sleep(20);

  // Stop registering scan messages
  Networks::Instance().Unregister(this);
  Networks::Instance().SetScan(false);
}

void MenuTaskRefreshSSID::ReceiveSyncMessage()
{
  // Rebuild list
  ItemSelector<String>::List newList;
  {
    Mutex::AutoLock lock(mListGuard);
    for(const Networks::HotSpot& ssid : mLastList)
      newList.push_back({Networks::FormatHotspot(ssid), ssid.Name() });
  }
  if (newList.empty()) newList.push_back({ RecalboxConf::Instance().GetWifiEnabled() ? _("SCANNING...") : _("NO WIFI ACCESS POINT"), String::Empty });

  // Refresh item
  String previousSelectedValue = mSSIDs.SelectedValue();
  mSSIDs.ChangeSelectionItems(newList, previousSelectedValue, String::Empty);
}
