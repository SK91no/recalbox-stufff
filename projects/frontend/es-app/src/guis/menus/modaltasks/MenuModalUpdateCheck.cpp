//
// Created by bkg2k on 25/11/24.
//

#include "MenuModalUpdateCheck.h"
#include <Upgrade.h>

bool MenuModalUpdateCheck::TaskExecute(const bool& parameter)
{
  (void)parameter;
  Upgrade::Instance().DoManualCheck(this);
  bool result = mSignal.WaitSignal(15000);
  Upgrade::Instance().DoManualCheck(nullptr);
  return result;
}

void MenuModalUpdateCheck::TaskComplete(const bool& result)
{
  (void)result;
  String message = result ? (mUpdateStatus ? (_F(_("A new version {0} is available!")) / mVersion).ToString() : _("No new version available yet.")) : _("Cannot check update ! Ensure your Recalbox is connected to the Internet, and try later.");
  mWindow.displayMessage(message);
  mSourceMenu.RequestMenuRefresh();
}

void MenuModalUpdateCheck::ManualCheckResponse(bool hasUpdate, const String& version)
{
  mUpdateStatus = hasUpdate;
  mVersion = version;
  mSignal.Fire();
}
