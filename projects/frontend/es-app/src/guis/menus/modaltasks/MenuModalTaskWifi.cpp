//
// Created by bkg2k on 13/11/24.
//

#include "MenuModalTaskWifi.h"
#include "recalbox/RecalboxSystem.h"
#include "utils/locale/LocaleHelper.h"
#include "network/Networks.h"

MenuModalTaskWIFI::MenuModalTaskWIFI(WindowManager& window, bool activate)
  : IMenuModalTask<bool, bool>(window, "", activate)
  , mActivate(activate)
{
}

bool MenuModalTaskWIFI::TaskExecute(const bool& activate)
{
  // No config? exit immediately
  RecalboxConf& conf = RecalboxConf::Instance();
  bool hasConfig = conf.GetWifiConnect() && !conf.GetWifiSSID().empty() && !conf.GetWifiKey().empty();

  // Disconnect ?
  if (!activate)
  {
    SetText(hasConfig ? _("Disconnecting from WIFI...") : _("Disabling WIFI..."));
    Thread::Sleep(1000);
    return RecalboxSystem::disableWifi();
  }

  // Connect/Reconnect
  SetText(hasConfig ? _("Connecting to WIFI...") : _("Enabling WIFI..."));
  RecalboxSystem::disableWifi();
  if (!RecalboxSystem::enableWifi()) return false;
  // No config? exit immediately
  if (!hasConfig) return true;
  // Wait 10s for IP
  for(int i = 15 * (1000 / 20); --i >= 0; Thread::Sleep(20))
  {
    SetText(_("Waiting for IP address... (%is)").Replace("%i", String(i / (1000 / 20))));
    if (Networks::Instance().HasIP(Networks::IPVersion::Both, Networks::Interfaces::Wifi)) return true;
  }
  return false;
}

void MenuModalTaskWIFI::TaskComplete(const bool& result)
{
  if (mActivate)
  {
    RecalboxConf& conf = RecalboxConf::Instance();
    bool hasConfig = conf.GetWifiConnect() && !conf.GetWifiSSID().empty() && !conf.GetWifiKey().empty();
    if (hasConfig) mWindow.displayMessage(result ? _("WIFI CONNECTION OK!") : _("CONNECTION ERROR !\nPlease check your SSID and Password."));
  }
}
