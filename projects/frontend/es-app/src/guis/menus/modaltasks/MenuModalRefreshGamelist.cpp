//
// Created by bkg2k on 11/03/25.
//

#include "MenuModalRefreshGamelist.h"
#include "guis/GuiMsgBoxScroll.h"
#include "guis/GuiScraperRun.h"
#include <systems/SystemManager.h>
#include <scraping/ScraperFactory.h>
#include <emulators/run/GameRunner.h>

MenuModalRefreshGamelist::MenuModalRefreshGamelist(WindowManager& window, SystemManager& systemManager, const SystemArray& systems)
  : IMenuModalTask<SystemArray, bool>(window, _("Refreshing gamelists..."), systems)
  , mSystemManager(systemManager)
  , mSystems(systems)
{
}

bool MenuModalRefreshGamelist::TaskExecute(const SystemArray& systems)
{
  // Release working thread on FileData structures
  SetText(_("Preparing gamelists..."));
  // Remove all virtual systems from list
  for(int i = mSystems.Count(); --i >= 0; )
    if (mSystems[i]->IsVirtual())
      mSystems.Delete(i);

  // Reload gamelists
  for(SystemData* system : systems)
  {
    SetText((_F(_("Reloading {0} gamelist...")) / system->FullName())());
    mSystemManager.RescanGamelists(system);
  }

  return true;
}

void MenuModalRefreshGamelist::TaskComplete(const bool& result)
{
  (void)result;

  // After work, refresh all lists by faking a Top filter refresh
  (void)mSystemManager.UpdatedTopLevelFilter();
}
