//
// Created by bkg2k on 13/11/24.
//

#include "MenuModalTaskWps.h"
#include "recalbox/RecalboxSystem.h"
#include "utils/locale/LocaleHelper.h"
#include "network/Networks.h"

MenuModalTaskWPS::MenuModalTaskWPS(WindowManager& window, ItemEditable& ssid, ItemEditable& password, ItemSwitch& connect)
  : IMenuModalTask<bool, bool>(window, "", false)
  , mSSID(ssid)
  , mPassword(password)
  , mConnect(connect)
{

}

bool MenuModalTaskWPS::TaskExecute(const bool& parameter)
{
  (void)parameter;

  { LOG(LogDebug) << "[WPS] Start WPS connection"; }
  SetText(_("Reseting WIFI configuration..."));
  { LOG(LogDebug) << "[WPS] " << "Reseting WIFI configuration..."; }
  if (!RecalboxSystem::enableWifi()) return false;
  SetText(_("Waiting for WPS configuration..."));
  { LOG(LogDebug) << "[WPS] " << "Waiting for WPS configuration..."; }
  if (!RecalboxSystem::getWifiWps()) return false;
  Thread::Sleep(2000); // Give time to read the message :)
  String ipResult;
  for (int i = 30; --i >= 0;)
  {
    SetText(_("Waiting for IP address... (%is)").Replace("%i", String(i)));
    { LOG(LogDebug) << "[WPS] " << GetText(); }
    if (Networks::Instance().HasIP(Networks::IPVersion::IPv4, Networks::Interfaces::Wifi)) break;
    if (i < 5 && Networks::Instance().HasIP(Networks::IPVersion::IPv6, Networks::Interfaces::Both)) break;
    Thread::Sleep(1000);
  }
  if (!Networks::Instance().HasIP(Networks::IPVersion::Both, Networks::Interfaces::Wifi)) return false;
  SetText(_("Saving WIFI configuration"));
  { LOG(LogDebug) << "[WPS] " << "Saving WIFI configuration"; }
  if (!RecalboxSystem::saveWifiWps()) return false;
  Thread::Sleep(2000); // Give time to read the message :)
  SetText(_("Fetching WIFI parameters"));
  { LOG(LogDebug) << "[WPS] " << "Fetching WIFI parameters"; }
  if (!RecalboxSystem::getWifiConfiguration(mSSISString, mPasswordString)) return false;
  Thread::Sleep(2000); // Give time to read the message :)

  { LOG(LogInfo) << "[WPS] WPS Configuration OK!"; }
  return true;
}

void MenuModalTaskWPS::TaskComplete(const bool& result)
{
  if (result)
  {
    RecalboxConf::Instance().SetWifiSSID(mSSISString)
                            .SetWifiKey(mPasswordString)
                            .SetWifiEnabled(true).Save();
    mConnect.SetState(true, false);
    mSSID.SetText(mSSISString, false);
    mPassword.SetText(mPasswordString, false);
    mWindow.displayMessage(_("WPS CONFIGURATION SUCCESSFUL!"));
  }
  else mWindow.displayMessage(_("NO WPS CONFIGURATION FOUND!"));
}
