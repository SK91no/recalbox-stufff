//
// Created by bkg2k on 11/11/24.
//

#include "MenuTaskRefreshIp.h"
#include "recalbox/RecalboxSystem.h"
#include "network/Networks.h"
#include <guis/menus/base/ItemText.h>

void MenuTaskRefreshIP::TaskRun()
{
  while(Running())
  {
    for(int i = 40 ; --i >= 0; )
      if (Running()) Thread::Sleep(20);
      else return;

    String ip;
    if (ip = Networks::Instance().IP(Networks::IPVersion::Both, Networks::Interfaces::Wifi); !ip.empty()) ip.Append(" (WIFI)");
    else if (ip = Networks::Instance().IP(Networks::IPVersion::Both, Networks::Interfaces::Ethernet); !ip.empty()) ip.Append(" (Ethernet)");

    if (!ip.empty()) { Mutex::AutoLock lock(mIPGuard); mLastIP = ip; }
    mSender.Send(!ip.empty());
  }
}

void MenuTaskRefreshIP::ReceiveSyncMessage(bool hasIP)
{
  Mutex::AutoLock lock(mIPGuard);
  if (hasIP) mIP.SetText(mLastIP);
  else mIP.SetText(_("UNAVAILABLE"));
}
