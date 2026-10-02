/* 
 * File:   Upgrade.cpp
 * Author: matthieu
 * 
 * Created on 6 February 2015, 11:40
 */

#include "utils/network/HttpClient.h"
#include <utils/Files.h>
#include "Upgrade.h"
#include "EmulationStation.h"
#include "RecalboxConf.h"
#include "utils/locale/LocaleHelper.h"
#include "recalbox/RecalboxSystem.h"
#include "guis/GuiUpdateRecalbox.h"
#include "network/Networks.h"
#include <patreon/PatronInfo.h>
#include <guis/GuiInfoPopup.h>

String Upgrade::CurrentVersion()
{
  String version = Files::LoadFile(Path(sLocalVersionFile)).Trim(" \t\r\n");
  return version.empty() ? String(PROGRAM_VERSION_STRING).Trim() : version;
}

Upgrade::UpdatePopup* Upgrade::UpdatePopup::mInstance = nullptr;

Upgrade::Upgrade(WindowManager& window, bool firstRun)
  : StaticLifeCycleControler<Upgrade>("upgrade")
  , mWindow(window)
  , mSender(*this)
  , mFirstRun(firstRun)
  , mManualCheckInterface(nullptr)
{
  Thread::Start("Upgrade");
}

Upgrade::~Upgrade()
{
  mSignal.Fire();
  Thread::Stop();
}

void Upgrade::Run()
{
  try
  {
    // Wait for patron response
    PatronInfo::Instance().WaitForAuthentication(*this);

    // First check at 15s
    int waitForSeconds = mFirstRun ? 15 : 3600;
    while (IsRunning())
    {
      if (mSignal.WaitSignal(waitForSeconds * 1000LL))
        if (mManualCheckInterface == nullptr)
          return;

      // Next checks, once an hour
      waitForSeconds = 3600;

      // Wait for network being available
      while(IsRunning())
      {
        if (mSignal.WaitSignal(5000LL)) return;
        if (Networks::Instance().HasIP(Networks::IPVersion::Both, Networks::Interfaces::Both)) break;
      }
      if (!IsRunning()) return;

      // Do we have to update?
      mRemoteVersion = GetRemoteVersion();
      mLocalVersion = Files::LoadFile(Path(sLocalVersionFile)).Trim();
      mRemoteReleaseNote = GetRemoteReleaseVersion();
      mLocalReleaseNote = Files::LoadFile(Path(sLocalReleaseNoteFile)).Trim();

      bool hasUpdate = false;
      if (ValidateVersion(mRemoteVersion))
      {
        if (mRemoteVersion != mLocalVersion)
        {
          { LOG(LogInfo) << "[Update] Remote version " << mRemoteVersion << " does not match local version " << mLocalVersion << ". Update available!"; }

          // Popup, always shown
          mPopupMessage = _("AN UPDATE IS AVAILABLE FOR YOUR RECALBOX");
          mPopupMessage += "\n";
          mPopupMessage += mRemoteVersion;
          mPopupMessage += "\n\n";
          mPopupMessage += _("You're strongly recommended to update your Recalbox.\nNo support will be provided for older versions!");

          // Message box only if the option is on
          if (RecalboxConf::Instance().GetUpdatesEnabled() && mManualCheckInterface == nullptr)
          {
            while (mWindow.HasGui() || mWindow.isSleeping())
              Thread::Sleep(5000);

            mMessageBoxMessage = _("NEW VERSION:");
            mMessageBoxMessage += " ";
            mMessageBoxMessage += mRemoteVersion;

            if (!mRemoteReleaseNote.empty())
            {
              mMessageBoxMessage += "\n\n";
              mMessageBoxMessage += _("CHANGELOG");
              mMessageBoxMessage += ":\n\n";
              mMessageBoxMessage += mRemoteReleaseNote;
            }
          }
          hasUpdate = true;
        }
        else { LOG(LogInfo) << "[Update] Remote version match local version. No update."; }
      }
      else { LOG(LogError) << "[Update] Invalid remote version! " << mRemoteVersion; }

      if (mManualCheckInterface != nullptr || hasUpdate)
        mSender.Send(hasUpdate);
    }
  }
  catch(std::exception& ex)
  {
    { LOG(LogError) << "[Update] Upgrade thread crashed."; }
    { LOG(LogError) << "[Update] Exception: " << ex.what(); }
  }
}

void Upgrade::ReceiveSyncMessage(bool updateAvaiable)
{
  if (mManualCheckInterface != nullptr)
  {
    mManualCheckInterface->ManualCheckResponse(updateAvaiable, NewVersion());
    mManualCheckInterface = nullptr;
    return;
  }

  // Volatile popup
  if (updateAvaiable)
    mWindow.InfoPopupAdd(new GuiInfoPopup(mWindow, mPopupMessage, 10, PopupType::Recalbox));

  // Messagebox
  if (updateAvaiable)
  {
    if (!mMessageBoxMessage.empty())
      UpdatePopup::Show(&mWindow, this, mMessageBoxMessage);
  }
  else mWindow.displayMessage(mMessageBoxMessage);
}

String Upgrade::GetDomainName()
{
  if (!mDomainName.empty()) return mDomainName;

  // Select DNS to query

  RecalboxConf::UpdateType update = RecalboxConf::Instance().GetUpdateType();
  // If target has been set to patron, we set it as not existing, to avoid the upgrade if the key is not valid
  if (update == RecalboxConf::UpdateType::Patron)
    update = RecalboxConf::UpdateType::Stable;
  // And if we are a patron, we can upgrade
  if (PatronInfo::Instance().IsPatron())
  {
    if ( update != RecalboxConf::UpdateType::Alpha
      && update != RecalboxConf::UpdateType::RRGBD2
      && update != RecalboxConf::UpdateType::JVS
      && update != RecalboxConf::UpdateType::PatronAlpha)
      update = RecalboxConf::UpdateType::Patron;
    else
      { LOG(LogDebug) << "[Update] Patron connected but the patron update channel has not been set" ; }
  }
  else if (update == RecalboxConf::UpdateType::PatronAlpha || PatronInfo::Instance().IsKeyPresent())
  {
    { LOG(LogDebug) << "[Update] Key present or patron-alpha selected, but not patron. Will not upgrade" ; }
    return "";
  }
  String target(RecalboxConf::UpdateTypeFromEnum(update));
  String domain(target);
  domain.Append(sUpgradeDNS);

  { LOG(LogDebug) << "[Update] " << (PatronInfo::Instance().IsPatron() ? "As a patron" : "") << " updates.type implied dns to use: " << domain; }

  // Query TXT
  mDomainName = Networks::QueryDNSRecord(domain);

  return mDomainName;
}

String Upgrade::GetRemoteVersion()
{
  // Get version
  String url = ReplaceMachineParameters(sVersionPatternUrl, String::Empty);

  String version;
  HttpClient request;
  for(int i = 3; --i >= 0; )
    if (request.Execute(url, version))
    {
      int returnCode = request.GetLastHttpResponseCode();
      if (returnCode == 200) break;  // Exit for
      if (returnCode >= 500 && returnCode <= 599) { Thread::Sleep(5000); continue; } // Next loop
      { LOG(LogError) << "[Update] Error getting remote version: " << url << " - got: " << request.GetLastHttpResponseCode() ; }
      version.clear();
      break;
    }
    else
    {
      { LOG(LogError) << "[Update] Error getting remote version: " << url; }
      version.clear();
    }

  if (version.StartsWith('<')) version.clear();
  version.Trim(" \t\r\n");
  { LOG(LogDebug) << "[Update] Remote version: " << version << " (" << url << ')'; }

  // Return version
  return version;
}

String Upgrade::TarUrl()
{
  // Get url
  String url = ReplaceMachineParameters(sTarDownloadPatternUrl, String::Empty);
  { LOG(LogDebug) << "[Update] Tar file url: " << url; }

  return url;
}

String Upgrade::ImageUrl()
{
  // Get url
  String url = ReplaceMachineParameters(sDownloadPatternUrl, String::Empty);
  { LOG(LogDebug) << "[Update] Image file url: " << url; }

  return url;
}

String Upgrade::HashUrl()
{
  // Get url
  String url = ReplaceMachineParameters(sDownloadPatternUrl, ".sha1");
  { LOG(LogDebug) << "[Update] Hash file url: " << url; }

  return url;
}

String Upgrade::ReplaceMachineParameters(const String& url, const String& ext)
{
  String result(url);

  // Get domain
  GetDomainName();

  // Get arch
  String arch = Files::LoadFile(Path(sLocalArchFile));
  if (arch == "xu4") arch = "odroidxu4";

  // Get uuid
  String uuid = Files::LoadFile(Path(sLocalUUID)).Trim();
  if (uuid.empty()) uuid = "";

  // Replacements
  result.Replace("#DOMAIN#", mDomainName)
        .Replace("#ARCH#", arch)
        .Replace("#UUID#", uuid)
        .Replace("#EXT#", ext);

  return result;
}

bool Upgrade::ValidateVersion(const String& version)
{
  static String _allowedCharacters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._- ()/";
  if (!version.empty())
    if (version[0] >= '0' && version[0] <= '9')
      return (version.find_first_not_of(_allowedCharacters) == String::npos);

  return false;
}

String Upgrade::GetRemoteReleaseVersion()
{
  // Get version
  String url = ReplaceMachineParameters(sReleasenotePatternUrl, String::Empty);

  String releaseNote;
  HttpClient request;
  for(int i = 3; --i >= 0; )
    if (request.Execute(url, releaseNote))
    {
      int returnCode = request.GetLastHttpResponseCode();
      if (returnCode == 200) break;  // Exit for
      if (returnCode >= 500 && returnCode <= 599) { Thread::Sleep(5000); continue; } // Next loop
      { LOG(LogError) << "[Update] Error getting remote release note: " << url << " - got: " << request.GetLastHttpResponseCode() ; }
      releaseNote.clear();
      break;
    }
    else
    {
      { LOG(LogError) << "[Update] Error getting remote release note: " << url; }
      releaseNote.clear();
    }

  { LOG(LogDebug) << "[Update] Remote release note: " << releaseNote << " (" << url << ')'; }

  if (releaseNote.StartsWith('<')) releaseNote.clear();
  else releaseNote.Trim();

  // Return version
  return releaseNote;
}

void Upgrade::DoManualCheck(IManualUpdateCheckInterface* callback)
{
  if (callback == nullptr) mManualCheckInterface = nullptr;
  else
  {
    mDomainName = "";
    mManualCheckInterface = callback;
    mSignal.Fire();
  }
}
