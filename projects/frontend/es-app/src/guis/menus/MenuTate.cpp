#include <RecalboxConf.h>
#include <guis/GuiMsgBox.h>
#include "MenuTate.h"
#include "guis/MenuMessages.h"
#include "recalbox/BootConf.h"
#include <views/ViewController.h>

MenuTate::MenuTate(WindowManager& window, SystemManager& systemManager)
  : Menu(window, InheritableContext(), _("TATE SETTINGS"))
  , mSystemManager(systemManager)
  , mOriginalTateEnabled(RecalboxConf::Instance().GetCollectionTate())
  , mOriginalGamesRotation(RotationUtils::FromUint(RecalboxConf::Instance().GetTateGameRotation()))
  , mOriginalSystemRotation(BootConf::Instance().GetRotation())
  , mOriginalTateOnly(RecalboxConf::Instance().GetShowOnlyTateGames())
{
}

void MenuTate::BuildMenuItems()
{
  // Enable virtual system
  AddSwitch(_("ENABLE TATE VIRTUAL SYSTEM"), mOriginalTateEnabled, (int)Components::TateEnabled, this, _(MENUMESSAGE_TATE_VIRTUALSYSTEM_MSG));

  // Enable virtual system
  bool hasTateGames = mSystemManager.HasVisibleTateGame();

  // Only tate games
  AddSwitch(_("DISPLAY ONLY TATE GAMES IN GAMELISTS"), mOriginalTateOnly, (int)Components::TateOnly, this, _(MENUMESSAGE_TATE_GAMELISTS_MSG),
            !hasTateGames, _("There is no TATE game available in your gamelists. Add TATE games and/or run the scraper to update TATE information") );

  // Rotate games
  RotationCapability cap = Board::Instance().GetRotationCapabilities();

  if(cap.rotationAvailable)
  {
    if(cap.defaultRotationWhenTate != RotationType::None)
    {
      // We have default rotation for this board, so we allow only changing rotation to none
      bool isAuto = (! RecalboxConf::Instance().IsDefinedTateGameRotation()) || mOriginalGamesRotation == cap.defaultRotationWhenTate;
      if(isAuto)
        mOriginalGamesRotation = cap.defaultRotationWhenTate;
      AddList<RotationType>(_("GAMES ROTATION"), (int)Components::TateGamesRotation, this,
                            SelectorEntry<RotationType>::List({
                                                                { "AUTO", cap.defaultRotationWhenTate, isAuto },
                                                                { "NONE", RotationType::None, !isAuto }
                                                              }), isAuto ? cap.defaultRotationWhenTate : RotationType::None, RotationType::None,
                            _(MENUMESSAGE_TATE_GAMESROTATION_MSG));
    }
    else
    {
      // Rotation
      AddList<RotationType>(_("GAMES ROTATION"), (int)Components::TateGamesRotation, this, GetRotationEntries(), mOriginalGamesRotation, RotationType::None, _(MENUMESSAGE_TATE_GAMESROTATION_MSG));
    }
    // Screen rotation
    if (cap.systemRotationAvailable)
      AddList<RotationType>(_("COMPLETE SYSTEM ROTATION"), (int)Components::TateCompleteSystemRotation, this, GetRotationEntries(), mOriginalSystemRotation, RotationType::None, _(MENUMESSAGE_TATE_SCREEN_ROTATION));
  }
}

MenuTate::~MenuTate()
{
  if (BootConf::Instance().GetRotation() != mOriginalSystemRotation)
    RequestRelaunch();
}

SelectorEntry<RotationType>::List MenuTate::GetRotationEntries()
{
  SelectorEntry<RotationType>::List list;
  list.push_back({ _("NONE"), RotationType::None });
  list.push_back({ _("LEFT"), RotationType::Left });
  list.push_back({ _("RIGHT"), RotationType::Right });
  list.push_back({ _("UPSIDEDOWN"), RotationType::Upsidedown });
  return list;
}

void MenuTate::MenuSingleChanged(ItemSelectorBase<RotationType>& item, int id, int index, const RotationType& value)
{
  (void)item;
  (void)index;
  if ((Components)id == Components::TateGamesRotation)
  {
    RecalboxConf::Instance().SetTateGameRotation((int)value).Save();
  }
  else if ((Components)id == Components::TateCompleteSystemRotation)
  {
    BootConf::Instance().SetRotation(value).Save();
  }
}

void MenuTate::MenuSwitchChanged(const ItemSwitch& item, bool& status, int id)
{
  (void)item;
  switch((Components)id)
  {
    case Components::TateEnabled:
    {
      RecalboxConf::Instance().SetCollectionTate(status).Save();
      mSystemManager.UpdateVirtualSystemsVisibility(VirtualSystemType::Tate,
                                                    status ? SystemManager::Visibility::ShowAndSelect : SystemManager::Visibility::Hide);
      break;
    }
    case Components::TateOnly:
    {
      RecalboxConf::Instance().SetShowOnlyTateGames(status);
      if (mSystemManager.UpdatedTopLevelFilter())
        RecalboxConf::Instance().Save();
      else
      {
        mWindow.displayMessage(_("There is no TATE game to show!No change recorded."));
        RecalboxConf::Instance().SetShowOnlyTateGames(!status);
        status = !status;
      }
      break;
    }
    case Components::TateGamesRotation:
    case Components::TateCompleteSystemRotation:
      break;
  }
}
