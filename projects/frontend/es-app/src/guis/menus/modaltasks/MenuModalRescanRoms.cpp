//
// Created by bkg2k on 11/03/25.
//

#include "MenuModalRescanRoms.h"
#include "guis/GuiMsgBoxScroll.h"
#include "guis/GuiScraperRun.h"
#include "patreon/PatronInfo.h"
#include <systems/SystemManager.h>
#include <scraping/ScraperFactory.h>
#include <emulators/run/GameRunner.h>
#include <guis/GuiInfoPopupSeamlessScraper.h>

MenuModalRescanRoms::MenuModalRescanRoms(WindowManager& window, SystemManager& systemManager, const SystemArray& systems)
  : IMenuModalTask<SystemArray, bool>(window, _("Refreshing gamelists..."), systems)
  , mSystemManager(systemManager)
  , mSystems(systems)
  , mTotalAdded(0)
  , mTotalRemoved(0)
  , mCurrentAdded(0)
  , mCurrentRemoved(0)
{
}

bool MenuModalRescanRoms::TaskExecute(const SystemArray& systems)
{
  // Release working thread on FileData structures
  SetText(_("Preparing gamelists..."));
  // Remove all virtual systems from list
  for(int i = mSystems.Count(); --i >= 0; )
    if (mSystems[i]->IsVirtual())
      mSystems.Delete(i);

  // Rescan disk, looking for new games
  for(SystemData* system : systems)
    mSystemManager.RescanSystem(system, this);

  return true;
}

void MenuModalRescanRoms::TaskComplete(const bool& result)
{
  (void)result;

  // After work, refresh all lists by faking a Top filter refresh
  (void)mSystemManager.UpdatedTopLevelFilter();

  // Final popup
  String modalText;
  if (mSystems.Count() == 1) modalText = (_F(_("Scan completed for system {0}.")) / mSystems.First()->FullName())();
  else modalText = _("Scan completed for all your systems.");
  modalText.Append(String::CRLF).Append(String::CRLF)
           .Append(mTotalRemoved == 0 ? _("No game has been removed from your gamelists") :
                   (_F(_N("{0} game has been removed from your gamelists", "{0} games have been removed from your gamelists", mTotalRemoved)) / mTotalRemoved)()).Append(String::CRLF)
           .Append(mTotalAdded == 0 ? _("No game has been added to your gamelists") :
                   (_F(_N("{0} game has been added to your gamelists", "{0} games have been added to your gamelists", mTotalAdded)) / mTotalAdded)());
  if (mTotalAdded != 0 && PatronInfo::Instance().IsPatron())
    modalText.Append(String::CRLF).Append(String::CRLF)
             .Append(_("Would you like to scrape newly added games now, using your current scraper configuration?"));

  if (mTotalAdded == 0 || !PatronInfo::Instance().IsPatron())
    mWindow.pushGui(new GuiMsgBoxScroll(mWindow, _("GAMELIST UPDATE COMPLETED"), modalText, _("OK"), nullptr));
  else
  {
    // Prapare capture variable as pointers to avoid capturing this
    FileData::List games = std::move(mAddedGames);
    mWindow.pushGui(new GuiMsgBoxScroll(mWindow, _("GAMELIST UPDATE COMPLETED"), modalText, _("LATER"), nullptr,
                                        _("SCRAPE NOW"),[games] { MenuModalRescanRoms::RunScraper(games); }));
  }
}

void MenuModalRescanRoms::SystemScanStart(const SystemData& system)
{
  SetText((_F(_("Scanning {0} - 0 Added  - 0 Removed")) / system.FullName())(), true);
  mCurrentAdded = 0;
  mCurrentRemoved = 0;
}

void MenuModalRescanRoms::SystemScanProgress(const SystemData& system, const Path& path)
{
  SetText((_F(_("Scanning {0}... {1} Added - {2} Removed")) / system.FullName() / mCurrentAdded / mCurrentAdded)(), true);
  String spath(path.ToString());
  if (spath.StartsWith("/recalbox/share_init/")) spath.Remove(LEGACY_STRING("/recalbox/share_init/"));
  else if (spath.StartsWith("/recalbox/share/")) spath.Remove(LEGACY_STRING("/recalbox/share/"));
  SetSecondaryText(spath);
}

void MenuModalRescanRoms::SystemScanGameAdded(const SystemData& system, const Path& path, FileData& game)
{
  (void)system;
  (void)path;
  mTotalAdded++;
  mCurrentAdded++;
  if (!game.System().IsScreenshots())
    mAddedGames.Add(&game);
}

void MenuModalRescanRoms::SystemScanGameRemoved(const SystemData& system, FileData& game)
{
  (void)system;
  (void)game;
  mTotalRemoved++;
  mCurrentRemoved++;
}

void MenuModalRescanRoms::SystemScanEnd(SystemData& system)
{
  if (mCurrentAdded != 0 || mCurrentRemoved != 0)
  {
    SetText((_F(_("Saving gamelist for {0}...")) / system.FullName())(), true);
    SetSecondaryText((_F(_("Added games: {0} - Removed games: {1}")) / mCurrentAdded / mCurrentRemoved)());
    system.UpdateGamelistXml();
    Thread::Sleep(1000);
  }
}

void MenuModalRescanRoms::RunScraper(FileData::List games)
{
  GuiInfoPopupSeamlessScraper::Instance().AddGames(games);
}

