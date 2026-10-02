#include <games/GameFilesUtils.h>
#include "MenuSearchLicenceSelector.h"
#include "MenuTools.h"
#include "games/FileData.h"
#include <views/ViewController.h>

#include <utility>
#include "utils/locale/LocaleHelper.h"
#include "guis/SearchForcedOptions .h"
#include "guis/GuiSearch.h"

MenuSearchLicenceSelector::MenuSearchLicenceSelector(WindowManager& window,
                                                     SystemManager& systemManager,
                                                     FileData& game,
                                                     std::vector <String> licences)
  : Menu(window, InheritableContext(), _("SEARCH BY LICENCE"), GetFooter(game))
  , mLicences(std::move(licences))
  , mSystemManager(systemManager),
   mSystemData(&game.System())
{}

void MenuSearchLicenceSelector::BuildMenuItems()
{
  int i=-1;

  for (auto& licence : mLicences)
  {
    AddAction(licence, _("SEARCH"), i++, true, this, _("Search games by licence"));
  }
}

String MenuSearchLicenceSelector::GetFooter(FileData& game)
{
  String gameName(game.Name());
  gameName.Append(" (").Append(game.RomPath().Filename()).Append(')');
  return _("GAME %s").Replace("%s", gameName.UpperCaseUTF8());
}

void MenuSearchLicenceSelector::MenuActionTriggered(ItemAction& item, int id)
{
  (void)item;
  int i=-1;
  for (auto& licence : mLicences) {
    if (id == i++)
    {
      SearchForcedOptions forcedOptions = SearchForcedOptions(licence,
                                                              FolderData::FastSearchContext::Licences, true);
      mWindow.pushGui(new GuiSearch(mWindow, mSystemManager,mSystemData, &forcedOptions));
    }
  }
}














