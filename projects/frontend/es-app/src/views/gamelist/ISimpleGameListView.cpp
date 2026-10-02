#include "views/gamelist/ISimpleGameListView.h"
#include <systems/SystemManager.h>
#include <guis/GuiControlHints.h>
#include "views/ViewController.h"
#include "RotationManager.h"
#include "views/MenuFilter.h"
#include "guis/menus/MenuProvider.h"
#include "network/Networks.h"
#include "guis/ItemTracker/GuiQuickJump.h"
#include <usernotifications/NotificationManager.h>

ISimpleGameListView::ISimpleGameListView(WindowManager& window, SystemManager& systemManager, SystemData& system, const IGlobalVariableResolver& resolver, PictogramCaches& flagCache)
  : Gui(window)
  , mConf(RecalboxConf::Instance())
  , mPictogramCaches(flagCache)
  , mResolver(resolver)
  , mSystem(system)
  , mSystemManager(systemManager)
  , mHeaderText(window)
  , mHeaderImage(window)
  , mThemeExtras(window)
  , mVerticalMove(false)
  , mListRefreshRequired(false)
  , mLastUse(TextureHolder::Tick()) // Start fresh: a just-created view must not be the oldest candidate
{
  setSize(Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat());

  mHeaderText.setText("Logo Text");
  mHeaderText.setSize(mSize.x(), 0);
  mHeaderText.setPosition(0, 0);
  mHeaderText.setAlignment(::Alignment::Center);
  mHeaderText.setDefaultZIndex(50);

  mHeaderImage.setResize(0, mSize.y() * 0.185f);
  mHeaderImage.setOrigin(0.5f, 0.0f);
  mHeaderImage.setPosition(mSize.x() / 2, 0);
  mHeaderImage.setDefaultZIndex(50);

  addChild(&mHeaderText);
}

void ISimpleGameListView::SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface)
{
  mHeaderImage.DoApplyThemeElement(theme, getName(), "logo", ThemePropertyCategory::All);
  mHeaderText.DoApplyThemeElement(theme, getName(), "logoText", ThemePropertyCategory::All);

  // Remove old theme extras
  for (ThemeExtras::Extra& extra : mThemeExtras.Extras())
    removeChild(&extra.Component());
  // Add new theme extras
  mThemeExtras.AssignExtras(theme, getName(), theme.GetExtras(getName(), mWindow, nullptr), refreshOnly);
  for (ThemeExtras::Extra& extra : mThemeExtras.Extras())
    addChild(&extra.Component());

  if (mHeaderImage.hasImage())
  {
    removeChild(&mHeaderText);
    addChild(&mHeaderImage);
  }
  else
  {
    addChild(&mHeaderText);
    removeChild(&mHeaderImage);
  }
  (void)interface;
}

bool ISimpleGameListView::ProcessInput(const InputCompactEvent& event)
{
  bool hideSystemView = RecalboxConf::Instance().GetStartupHideSystemView();
  FileData* cursor = Gamelist().GetSelectedGame();

  // *TEMP* USE SELECT TO OPEN FAST TRACKER
  if (event.HotkeyNorthReleased())
  {
    ShowQuickJump();
    return true;
  }

  // Let the list process its events
  if (Gamelist().ProcessListInput(event)) return true;

  // BACK to PARENT FOLDER or PARENT SYSTEM
  if (event.CancelReleased())
  {
    clean();
    if (!Gamelist().IsInRootFolder())
    {
      // Back to parent folder
      FolderData* current = Gamelist().GetCurrentFolder();
      mLastPositionInFolder[current] = cursor;
      FolderData* newFolder = Gamelist().ExitFolder();
      populateList(*newFolder, nullptr);
      Gamelist().SetSelectedGame(mLastPositionInFolder[newFolder], true);
    }
    else if (!hideSystemView)
    {
      if(mSystem.Rotatable())
      {
        RotationType rotation = RotationType::None;
        if(RotationManager::ShouldRotateTateExit(rotation))
        {
          mWindow.Rotate(rotation);
        }
      }
      onFocusLost();
      ViewController::Instance().goToSystemView(&mSystem);
    }
    return true;
  }

  // TOGGLE FAVORITES
  if (event.WestReleased() && MenuFilter::ShouldEnableFeature(MenuFilter::Favorites) && RecalboxConf::Instance().GetEnableEditFavorites())
  {
    if (cursor->IsGame())
    {
      ViewController::Instance().ToggleFavorite(cursor);
      UpdateHelpBar();
    }
    return true;
  }

  // Check vertical move
  if (event.AnyPrimaryUpPressed() || event.AnyPrimaryDownPressed()) mVerticalMove = true;
  if (event.AnyPrimaryUpReleased() || event.AnyPrimaryDownReleased()) mVerticalMove = false;

  // MOVE to NEXT GAMELIST
  if (event.AnyPrimaryRightReleased())
  {
    if (RecalboxConf::Instance().GetQuickSystemSelect() && !hideSystemView && !mVerticalMove)
    {
      clean();
      onFocusLost();
      ViewController::Instance().goToNextGameList();
    }
    return true;
  }

  // MOVE to PREVIOUS GAMELIST
  if (event.AnyPrimaryLeftReleased())
  {
    if (RecalboxConf::Instance().GetQuickSystemSelect() && !hideSystemView && !mVerticalMove)
    {
      clean();
      onFocusLost();
      ViewController::Instance().goToPrevGameList();
    }
    return true;
  }

  // NETPLAY
  if (event.NorthReleased() && MenuFilter::ShouldEnableFeature(MenuFilter::Netplay)
      && (Gamelist().GetSelectedGame()->System().Descriptor().HasNetPlayCores()))
  {
    if (Networks::Instance().HasIP(Networks::IPVersion::Both, Networks::Interfaces::Both))
    {
      clean();
      MenuProvider::ShowMenu(MenuContainerType::NetplayHost, InheritableContext(&mSystem, Gamelist().GetSelectedGame(), this));
    }
    else
      mWindow.displayMessage(_("There is no network available.\nPlease connect your recalbox and try again."));
    return true;
  }
  if (event.NorthReleased() && MenuFilter::ShouldEnableFeature(MenuFilter::P2K))
  {
    FileData* fd = Gamelist().GetSelectedGame();
    if (fd != nullptr)
      if (fd->HasP2K())
        mWindow.pushGui(new GuiControlHints(mWindow, fd->RomPath()));
    return true;
  }
  if (event.StartReleased() && MenuFilter::ShouldDisplayMenu(MenuFilter::Menu::GamelistOptions))
  {
    clean();
    MenuProvider::ShowMenu(MenuContainerType::Gamelist, InheritableContext(&mSystem, Gamelist().GetSelectedGame(), this));
    return true;
  }

  bool result = Gui::ProcessInput(event);

  return result;
}

bool ISimpleGameListView::CollectHelpItems(Help& help)
{
  bool hideSystemView = RecalboxConf::Instance().GetStartupHideSystemView();

  help.Set(Help::Valid(), _("LAUNCH"));

  if (Gamelist().HasGame())
  {
    if (Gamelist().GetSelectedGame()->System().Descriptor().HasNetPlayCores() && MenuFilter::ShouldEnableFeature(MenuFilter::Netplay))
      help.Set(HelpType::X, _("NETPLAY"));
    else
    {
      if (HasCurrentGameP2K() && MenuFilter::ShouldEnableFeature(MenuFilter::P2K))
        help.Set(HelpType::X, _("P2K CONTROLS"));
    }
    FileData* fd = Gamelist().GetSelectedGame();
    if ((!fd->TopAncestor().ReadOnly() || fd->TopAncestor().PreInstalled())
        && MenuFilter::ShouldEnableFeature(MenuFilter::Favorites)
        && RecalboxConf::Instance().GetEnableEditFavorites())
      help.Set(HelpType::Y, IsFavoriteSystem() ? _("Remove from favorite") : _("Favorite"));
  }

  if (!hideSystemView)
    help.Set(Help::Cancel(), _("BACK"));

  help.Set(HelpType::UpDown, _("CHOOSE"));

  if (RecalboxConf::Instance().GetQuickSystemSelect() && !hideSystemView)
    help.Set(HelpType::LeftRight, _("SYSTEM"));

  if (!RecalboxConf::Instance().GetQuickSystemSelect())
    help.Set(HelpType::LeftRight, _("PAGE UP/DOWN"));

  if (MenuFilter::ShouldDisplayMenu(MenuFilter::Main))
    help.Set(HelpType::Start, _("OPTIONS"));

  help.Set(HelpType::HkX, _("QUICK JUMP"));

  return true;
}

void ISimpleGameListView::QuickJumpTo(FileData* item)
{
  // Quick jump onto item
  Gamelist().SetSelectedGame(item, false);
}

void ISimpleGameListView::ShowQuickJump()
{
  mTrackedItems.Initialize(Gamelist().GetSelectedGame());
  for(FileData* item : GetHighLevelFileList())
    mTrackedItems.AddCandidate(GetRawDisplayName(*item), item);
  mTrackedItems.Finalize();
  mWindow.pushGui(new GuiQuickJump(mWindow, mTrackedItems, *this));
}

void ISimpleGameListView::OnGameSelected(IGamelistContainer& caller, FileData* game, int index)
{
  (void)caller;
  (void)game;
  (void)index;

  // Don't do anything on empty lists
  if (!Gamelist().HasGame()) return;
  // Notify browsing new game
  if (game != nullptr)
  {
    NotificationManager::Instance().Notify(*game, Notification::GamelistBrowsing);
    // Update current game information
    DoUpdateGameInformation(false);
    // Update theme context
    ThemeManager::ChangeThemeContext(getName(), mThemeExtras.Extras(), &mSystem, game);
  }
}

void ISimpleGameListView::OnGameValidated(IGamelistContainer& caller, FileData* game, int index)
{
  (void)caller;
  (void)index;
  clean(); // #TODO: Kill this non sense method when videos are no longer singleton
  launch(game);
}

void ISimpleGameListView::OnFolderValidated(IGamelistContainer& caller, FolderData* folder, int index)
{
  (void)caller;
  (void)index;
  clean(); // #TODO: Kill this non sense method when videos are no longer singleton
  if (folder->HasChildren())
  {
    Gamelist().EnterFolder(folder);
    populateList(*folder, nullptr);
    FileData** lastPosition = mLastPositionInFolder.try_get(folder);
    if (lastPosition != nullptr) Gamelist().SetSelectedGame(*lastPosition, true);
    else Gamelist().SetSelectedGameIndex(0, true);
  }
}
