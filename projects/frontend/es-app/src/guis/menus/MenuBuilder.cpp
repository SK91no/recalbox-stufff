//
// Created by bkg2k on 21/10/24.
//

#include "MenuBuilder.h"
#include "Upgrade.h"
#include "MenuProvider.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiInfoPopup.h"
#include "MainRunner.h"
#include "guis/menus/tasks/MenuTaskRefreshConnection.h"
#include "guis/menus/tasks/MenuTaskRefreshIp.h"
#include "guis/menus/tasks/MenuTaskRefreshSsid.h"
#include "games/GameFilesUtils.h"
#include "guis/menus/tasks/MenuTaskRefreshAudioOutputs.h"
#include "guis/menus/tasks/MenuTaskSynchronizeVolumes.h"
#include "systems/DownloaderManager.h"
#include "netplay/DefaultPasswords.h"
#include "utils/SectionFile.h"
#include "hardware/JvsCab.h"
#include "guis/GuiScraperRun.h"
#include <guis/menus/base/ItemText.h>
#include <guis/menus/base/ItemSelector.h>
#include <guis/menus/base/ItemSlider.h>
#include <guis/menus/base/ItemBar.h>
#include <guis/menus/base/ItemSwitch.h>
#include <guis/menus/base/ItemRating.h>
#include <guis/menus/base/ItemAction.h>
#include <guis/menus/base/ItemEditable.h>
#include <guis/menus/base/ItemSubMenu.h>
#include <guis/menus/base/ItemHeader.h>
#include <systems/SystemManager.h>
#include <patreon/PatronInfo.h>
#include <audio/AudioController.h>

MenuBuilder::~MenuBuilder()
{
  bool reboot = false;
  bool relaunch = false;
  bool bootConf = false;

  // Flush pending events
  for(auto& kv : mItemsTypes)
    kv.second->Flush();

  // Check reboot/relaunch
  for(const auto& link : mItemsLinks)
    if (link.first->HasChanged())
    {
      reboot |= link.second->Reboot();
      relaunch |= link.second->Relaunch();
      bootConf |= link.second->BootConf();
    }

  if (reboot)
  {
    mWindow.pushGui(new GuiMsgBox(mWindow, _("THE SYSTEM WILL NOW REBOOT"),
                                  _("OK"), []{ MainRunner::RequestQuit(MainRunner::ExitState::NormalReboot); },
                                  _("LATER"), std::bind(MenuBuilder::RebootPending, &mWindow)));
  }
  else if (relaunch)
  {
    mWindow.pushGui(
      new GuiMsgBox(mWindow, _("Recalbox interface must restart to apply your changes."),
                    _("OK"), []{ MainRunner::RequestQuit(MainRunner::ExitState::Relaunch); },
                    _("LATER"), std::bind(MenuBuilder::RebootPending, &mWindow)));
  }

  if (bootConf)
  {
    RecalboxSystem::MakeBootReadWrite();
    Files::CopyFile(mConf.FilePath(), Path("/boot/recalbox-backup.conf"));
    RecalboxSystem::MakeBootReadOnly();
  }

  // Stop tasks
  ClearTasks();

  // Callback interface
  if (Context().HasGameInterface() && Context().HasGame())
    Context().GameInterface()->GameMetadataChanged(*Context().Game());
}

void MenuBuilder::ClearTasks()
{
  // Stop background tasks
  for(IMenuBackgroundTask* task : mTasks)
  {
    task->TaskStop();
    delete task;
  }
  mTasks.Clear();
}

void MenuBuilder::RebootPending(WindowManager* window)
{
  static bool pending = false;
  if (!pending)
  {
    window->InfoPopupAdd(new GuiInfoPopup(*window, _("A reboot is required to apply pending changes."), 10000, PopupType::Reboot));
    pending = true;
  }
}

void MenuBuilder::BuildThemeOptionSelector(const InheritableContext& context, const ItemDefinition& item, ThemeOption* option)
{
  if (option == nullptr || !option->IsValid()) return;

  String themeFolder = mConf.GetThemeFolder(); IniFile::PurgeKey(themeFolder);
  String value = RecalboxConf::Instance().GetThemeOption(themeFolder, option->Name());

  bool found = false;
  String realSelected;
  std::vector<ThemeOption::Value> values = option->SortedValue();
  // Set selection
  for(const ThemeOption::Value& s : values) if (s.Raw() == value) { found = true; realSelected = s.Raw(); break; }
  if (!found && !values.empty()) realSelected = values.front().Raw();

  // Build list
  SelectorEntry<String>::List list;
  for (const ThemeOption::Value& s : values)
    list.push_back({ s.IsIndexed() ? s.Translated().SubString(s.TextStart()) : s.Translated(), s.Raw(), s.Raw() == realSelected });

  // Create menu entry
  ItemSelector<String>* existing = AsList<String>(item.Type());
  if (existing != nullptr) existing->ChangeSelectionItems(list, realSelected, list.front().mValue).SetSelectable(true);
  else AddList<String>(context, item, list, realSelected, list.front().mValue, false, true);
}

bool MenuBuilder::EvaluateIdentifier(const String& identifier)
{
  typedef bool (*IdentifierMethod)(MenuBuilder*);
  static HashMap<String, IdentifierMethod> sMethodTable
  {
    { "hardware.crt"           , [](MenuBuilder* This) { return This->mResolver.HasCrt(); } },
    { "hardware.crtattached"   , [](MenuBuilder* This) { return This->mResolver.HasCrtAttached(); } },
    { "hardware.crthat"        , [](MenuBuilder* This) { return This->mResolver.HasCrtHatInstalled(); } },
    { "hardware.crtinterlaced" , [](MenuBuilder* This) { return This->mResolver.HasCrtInterlaced(); } },
    { "hardware.rrgbj"         , [](MenuBuilder* This) { return This->mResolver.HasJamma(); } },
    { "hardware.rrgbd"         , [](MenuBuilder* This) { return This->mResolver.HasRRGBD(); } },
    { "hardware.rrgbd2"        , [](MenuBuilder* This) { return This->mResolver.HasRRGBD2(); } },
    { "hardware.cardreader.available" , [](MenuBuilder* This) { return This->mResolver.HasCardReaderAvailable(); } },
    { "hardware.cardreader.plugged"   , [](MenuBuilder* This) { return This->mResolver.HasCardReaderPlugged(); } },
    { "hardware.jvs"           , [](MenuBuilder* This) { return This->mResolver.HasJVS(); } },
    { "hardware.jvs.hdmionly"  , [](MenuBuilder* This) { return This->mResolver.HasJVS() && JvsCab::IsHdmiOnly(); } },
    { "hardware.jvs.khz15only" , [](MenuBuilder* This) { return This->mResolver.HasJVS() && JvsCab::IsKhz15Only(); } },
    { "hardware.jvs.guncab"    , [](MenuBuilder* This) { return This->mResolver.HasJVS() && JvsCab::IsGunCab(); } },
    { "hardware.hdmi"          , [](MenuBuilder* This) { return !This->mResolver.HasCrtAttached(); } },
    { "display.overscan"       , [](MenuBuilder* This) { return This->mResolver.HasCrt() && !This->mResolver.HasJamma() && !This->mResolver.HasJVS(); } },
    { "display.tate"           , [](MenuBuilder* This) { return This->mResolver.IsTate(); } },
    { "display.tateright"      , [](MenuBuilder* This) { return This->mResolver.IsTateRight(); } },
    { "display.tateleft"       , [](MenuBuilder* This) { return This->mResolver.IsTateLeft(); } },
    { "display.qvga"           , [](MenuBuilder* This) { return This->mResolver.IsQVGA(); } },
    { "display.vga"            , [](MenuBuilder* This) { return This->mResolver.IsVGA(); } },
    { "display.hd"             , [](MenuBuilder* This) { return This->mResolver.IsHD(); } },
    { "display.fhd"            , [](MenuBuilder* This) { return This->mResolver.IsFHD(); } },
    { "system.available"       , [](MenuBuilder* This) { return This->Context().HasSystem(); } },
    { "game.available"         , [](MenuBuilder* This) { return This->Context().HasGame(); } },
    { "system.isvirtual"       , [](MenuBuilder* This) { return This->Context().HasSystem() && This->Context().System()->IsVirtual(); } },
    { "system.isarcade"        , [](MenuBuilder* This) { return This->Context().HasSystem() && This->Context().System()->IsArcade(); } },
    { "system.isport"          , [](MenuBuilder* This) { return This->Context().HasSystem() && This->Context().System()->IsPorts(); } },
    { "system.isconsole"       , [](MenuBuilder* This) { return This->Context().HasSystem() && This->Context().System()->Descriptor().Type() == SystemDescriptor::SystemType::Console; } },
    { "system.ishandheld"      , [](MenuBuilder* This) { return This->Context().HasSystem() && This->Context().System()->Descriptor().Type() == SystemDescriptor::SystemType::Handheld; } },
    { "system.iscomputer"      , [](MenuBuilder* This) { return This->Context().HasSystem() && This->Context().System()->Descriptor().Type() == SystemDescriptor::SystemType::Computer; } },
    { "system.isfantasy"       , [](MenuBuilder* This) { return This->Context().HasSystem() && This->Context().System()->Descriptor().Type() == SystemDescriptor::SystemType::Fantasy; } },
    { "system.isengine"        , [](MenuBuilder* This) { return This->Context().HasSystem() && This->Context().System()->Descriptor().Type() == SystemDescriptor::SystemType::Engine; } },
    { "system.isfavorites"     , [](MenuBuilder* This) { return This->Context().HasSystem() && This->Context().System()->IsFavorite(); } },
    { "system.islastplayed"    , [](MenuBuilder* This) { return This->Context().HasSystem() && This->Context().System()->IsLastPlayed(); } },
    { "system.isscreenshots"   , [](MenuBuilder* This) { return This->Context().HasSystem() && This->Context().System()->IsScreenshots(); } },
    { "system.isalwaysflat"    , [](MenuBuilder* This) { return This->Context().HasSystem() && This->Context().System()->IsAlwaysFlat(); } },
    { "system.isselfsorted"    , [](MenuBuilder* This) { return This->Context().HasSystem() && This->Context().System()->IsSelfSorted(); } },
    { "system.hasdownloader"   , [](MenuBuilder* This) { return This->Context().HasSystem() && (DownloaderManager::HasDownloader(*This->Context().System())); } },
    { "game.isgame"            , [](MenuBuilder* This) { return This->Context().HasGame() && This->Context().Game()->IsGame(); } },
    { "game.isfolder"          , [](MenuBuilder* This) { return This->Context().HasGame() && This->Context().Game()->IsFolder(); } },
    { "game.isreadonly"        , [](MenuBuilder* This) { return This->Context().HasGame() && This->Context().Game()->TopAncestor().ReadOnly(); } },
    { "game.hasmanual"         , [](MenuBuilder* This) { return This->Context().HasGame() && This->Context().Game()->Metadata().HasManual(); } },
    { "game.hasmap"            , [](MenuBuilder* This) { return This->Context().HasGame() && This->Context().Game()->Metadata().HasMap(); } },
    { "game.hastips"           , [](MenuBuilder* This) { return This->Context().HasGame() && This->Context().Game()->Metadata().HasTipsAndTricks(); } },
    { "game.hassavestate"      , [](MenuBuilder* This) { return This->Context().HasGame() && !GameFilesUtils::GetGameSaveStateFiles(*This->Context().Game()).empty(); } },
    { "game.hasslicences"      , [](MenuBuilder* This) { return This->Context().HasGame() && This->Context().Game()->Metadata().HasLicences(); } },
    { "game.hassoftpatch"      , [](MenuBuilder* This) { return This->Context().HasGame() && !GameFilesUtils::GetSoftPatches(This->Context().Game()).empty(); } },
    { "game.hassoftpatchcore"  , [](MenuBuilder* This) { return This->Context().HasGame() && EmulatorManager::GetGameEmulator(*This->Context().Game()).CoreInfo().Softpatching(); } },
    { "game.hasmediafiles"     , [](MenuBuilder* This) { return This->Context().HasGame() && !GameFilesUtils::GetMediaFiles(*This->Context().Game()).empty(); } },
    { "game.hasextrafiles"     , [](MenuBuilder* This) { return This->Context().HasGame() && !GameFilesUtils::GetGameExtraFiles(*This->Context().Game()).empty(); } },
    { "game.hassavefiles"      , [](MenuBuilder* This) { return This->Context().HasGame() && !GameFilesUtils::GetGameSaveFiles(*This->Context().Game()).empty(); } },
    { "game.system.isvirtual"  , [](MenuBuilder* This) { (void)This; return false; } },
    { "game.system.isarcade"   , [](MenuBuilder* This) { return This->Context().HasGame() && This->Context().Game()->System().IsArcade(); } },
    { "game.system.isport"     , [](MenuBuilder* This) { return This->Context().HasGame() && This->Context().Game()->System().IsPorts(); } },
    { "game.system.isconsole"  , [](MenuBuilder* This) { return This->Context().HasGame() && This->Context().Game()->System().Descriptor().Type() == SystemDescriptor::SystemType::Console; } },
    { "game.system.ishandheld" , [](MenuBuilder* This) { return This->Context().HasGame() && This->Context().Game()->System().Descriptor().Type() == SystemDescriptor::SystemType::Handheld; } },
    { "game.system.iscomputer" , [](MenuBuilder* This) { return This->Context().HasGame() && This->Context().Game()->System().Descriptor().Type() == SystemDescriptor::SystemType::Computer; } },
    { "game.system.isfantasy"  , [](MenuBuilder* This) { return This->Context().HasGame() && This->Context().Game()->System().Descriptor().Type() == SystemDescriptor::SystemType::Fantasy; } },
    { "game.system.isengine"   , [](MenuBuilder* This) { return This->Context().HasGame() && This->Context().Game()->System().Descriptor().Type() == SystemDescriptor::SystemType::Engine; } },
    { "menu.bartop"            , [](MenuBuilder* This) { return This->mResolver.IsBartopModeOrHigher(); } },
    { "menu.nomenu"            , [](MenuBuilder* This) { return This->mResolver.IsNoMenuMode(); } },
    { "menu.debug"             , [](MenuBuilder* This) { return This->mConf.GetDebugMenus(); } },
    { "feature.wifi"           , [](MenuBuilder* This) { return This->mConf.GetWifiEnabled(); } },
    { "feature.editfavorite"   , [](MenuBuilder* This) { return This->mConf.GetEnableEditFavorites(); } },
    { "feature.bootongame"     , [](MenuBuilder* This) { return This->mConf.GetBootOnGameEnabled(); } },
    { "feature.kodienabled"    , [](MenuBuilder* This) { return This->mConf.GetKodiEnabled(); } },
    { "feature.kodiexists"     , [](MenuBuilder* This) { (void)This; return RecalboxSystem::kodiExists(); } },
    { "feature.hasuserscripts" , [](MenuBuilder* This) { (void)This; return NotificationManager::Instance().HasManualScript(); } },
    { "storage.hasextradevices", [](MenuBuilder* This) { return This->mProvider.SystemManager().GetMountMonitor().AllMountPoints().size() > 1; } },
    { "state.ispatron"         , [](MenuBuilder* This) { (void)This; return PatronInfo::Instance().IsPatron(); } },
    { "state.isbeta"           , [](MenuBuilder* This) { return This->mBeta; } },
    { "board.haspowerbutton"   , [](MenuBuilder* This) { (void)This; return !Case::CurrentCase().CanShutdownFromMenu(); } },
    { "board.ispcboard"        , [](MenuBuilder* This) { (void)This; return Board::Instance().IsPC(); } },
    { "board.ispiboard"        , [](MenuBuilder* This) { (void)This; return Board::Instance().IsPi(); } },
    { "board.isanbernicboard"  , [](MenuBuilder* This) { (void)This; return Board::Instance().IsAnbernic(); } },
    { "board.isodroidboard"    , [](MenuBuilder* This) { (void)This; return Board::Instance().IsOdroid(); } },
    { "board.ispi02board"      , [](MenuBuilder* This) { (void)This; return ({ BoardType t = Board::Instance().GetBoardType(); t == BoardType::Pi02; }); } },
    { "board.ispi3board"       , [](MenuBuilder* This) { (void)This; return ({ BoardType t = Board::Instance().GetBoardType(); t == BoardType::Pi3 || t == BoardType::Pi3plus; }); } },
    { "board.ispi4board"       , [](MenuBuilder* This) { (void)This; return ({ BoardType t = Board::Instance().GetBoardType(); t == BoardType::Pi4 || t == BoardType::Pi400; }); } },
    { "board.ispi5board"       , [](MenuBuilder* This) { (void)This; return ({ BoardType t = Board::Instance().GetBoardType(); t == BoardType::Pi5 || t == BoardType::Pi500; }); } },
    { "board.issteamdeck"      , [](MenuBuilder* This) { (void)This; return ({ BoardType t = Board::Instance().GetBoardType(); t == BoardType::SteamDeckLCD || t == BoardType::SteamDeckOLED; }); } },
    { "feature.hasvulkan"      , [](MenuBuilder* This) { (void)This; return Board::Instance().HasVulkanSupport(); } },
    { "board.hasbrightness"    , [](MenuBuilder* This) { (void)This; return Board::Instance().HasBrightnessSupport(); } },
    { "board.handheldboard"    , [](MenuBuilder* This) { (void)This; return Board::IsHandheldSystem(); } },
    { "board.homeboard"        , [](MenuBuilder* This) { (void)This; return Board::IsHomeSystem(); } },
    { "case"                   , [](MenuBuilder* This) { (void)This; return Case::SupportedManualCases().empty(); } },
    { "theme.hasdefaulttransition", [](MenuBuilder* This) { (void)This; return ThemeManager::Instance().Main().hasDefaultTransition(); } },
  };

  IdentifierMethod* method = sMethodTable.try_get(identifier);
  if (method != nullptr) return (*method)(this);

  { LOGT(LogDebug) << "[MenuBuilder] Unknown identifier " << identifier << ". False evaluation assumed."; }
  return false;
}

void MenuBuilder::BuildMenuItems()
{
  // Clear previous items if any
  mItemsTypes.clear();
  mItemsLinks.clear();
  ClearTasks();

  BuildMenuItemsFrom(mDefinition);

  if (Count() == 0)
    Menu::AddText("EMPTY MENU", "CHECK MENU.XML!");
  else
    for(IMenuBackgroundTask* task : mTasks)
      task->TaskStart();
}

void MenuBuilder::BuildMenuItemsFrom(const MenuDefinition& menuDefinition)
{
  // Create items
  for(const ItemDefinition& item : menuDefinition.mItems)
  {
    // Préparing loop
    Array<const void*> contextData;
    switch(item.Iterator())
    {
      case ItemDefinition::ForEach::SystemAll: for(SystemData* system : mProvider.SystemManager().AllSystems()) contextData.Add(system); break;
      case ItemDefinition::ForEach::SystemVisibles: for(SystemData* system : mProvider.SystemManager().VisibleSystemList()) contextData.Add(system); break;
      case ItemDefinition::ForEach::DevicesAll: for(const StorageDevices::Device& device : StorageDevices::Instance().Refresh().GetStorageDevices()) contextData.Add(&device); break;
      case ItemDefinition::ForEach::DevicesExternals: for(const StorageDevices::Device& device : StorageDevices::Instance().Refresh().GetStorageDevices()) if (device.Type == StorageDevices::Types::External) contextData.Add(&device); break;
      case ItemDefinition::ForEach::PhysicalDisks: for(const PhysicalDevices::Device& device : PhysicalDevices::Instance().Refresh().GetDevices()) contextData.Add(&device); break;
      case ItemDefinition::ForEach::ScriptsAll: for(const ScriptDescriptor& script : mProvider.LoadUserScripts()) contextData.Add(&script); break;
      case ItemDefinition::ForEach::ThemeOptionAll: for(const ThemeOption* option : ThemeManager::Instance().Main().GetSubsets()) contextData.Add(option); break;
      case ItemDefinition::ForEach::None:
      default: break;
    }
    // Ignore empty contexts
    if (item.Iterator() != ItemDefinition::ForEach::None && contextData.Empty()) continue;

    // Loop
    do
    {
      // Inherit context from the menu
      InheritableContext context(Context());
      // Then merge appropriate context data
      switch(item.Iterator())
      {
        case ItemDefinition::ForEach::SystemAll:
        case ItemDefinition::ForEach::SystemVisibles: context.Merge(InheritableContext((SystemData*)contextData.First())); break;
        case ItemDefinition::ForEach::DevicesAll:
        case ItemDefinition::ForEach::DevicesExternals: context.Merge(InheritableContext((StorageDevices::Device*)contextData.First())); break;
        case ItemDefinition::ForEach::PhysicalDisks: context.Merge(InheritableContext((PhysicalDevices::Device*)contextData.First())); break;
        case ItemDefinition::ForEach::ScriptsAll: context.Merge(InheritableContext((ScriptDescriptor*)contextData.First())); break;
        case ItemDefinition::ForEach::ThemeOptionAll: context.Merge(InheritableContext((ThemeOption*)contextData.First())); break;
        case ItemDefinition::ForEach::None:
        default: break;
      }

        // Add item
      switch (item.Category())
      {
        case ItemDefinition::ItemCategory::SubMenu:
        {
          // Condition?
          if (item.HasCondition() && !SimpleExpressionEvaluator(*this).Evaluate(item.Condition())) break;
          // Custom create?
          if (!AddCustomSubMenu(context, item))
          {
            if (item.MenuInclude())
            {
              if (!item.RawCaption().empty()) AddHeader(item.Caption(this));
              BuildMenuItemsFrom(mProvider.GetMenuDefinition(item.MenuType()));
            }
            else
            {
              bool defaultGrayed = false;
              if (item.HasGrayedCondition())
                if (SimpleExpressionEvaluator(*this).Evaluate(item.GrayedCondition()))
                  defaultGrayed = true;
              ItemSubMenu* submenu = (item.Icon() != MenuThemeData::MenuIcons::Type::Unknown) ?
                                     Menu::AddSubMenu(item.Caption(this), item.Icon(), (int) item.MenuType(), &mProvider, item.Help(), defaultGrayed, item.UnselectableHelp()) :
                                     Menu::AddSubMenu(item.Caption(this), (int) item.MenuType(), &mProvider, item.Help(),defaultGrayed, item.UnselectableHelp());
              // Merge context
              submenu->MergeContext(context).ReplaceParameters();
            }
          }
          break;
        }
        case ItemDefinition::ItemCategory::Item:
        {
          // Condition?
          if (item.HasCondition()) if (!SimpleExpressionEvaluator(*this).Evaluate(item.Condition())) break;
          AddItem(context, item);
          break;
        }
        case ItemDefinition::ItemCategory::Header:
        {
          AddHeader(item.Caption(this))->MergeContext(context);
          break;
        }
        case ItemDefinition::ItemCategory::Setting:
        {
          // Condition?
          if (item.HasCondition()) if (!SimpleExpressionEvaluator(*this).Evaluate(item.Condition())) break;
          AddSetting(context, item);
          break;
        }
        default:
        { LOG(LogFatal) << "[MenuProvider] Unknown menu item !"; }
      }

      // Next context loop?
      if (!contextData.Empty())
        contextData.Delete(0);

    } while(!contextData.Empty());
  }
}

void MenuBuilder::MenuItemShow(ItemBase& item)
{
  // Refresh system option submenu
  if (item.IsSubMenu() && (MenuContainerType)item.Identifier() == MenuContainerType::AdvancedSingleEmulatorOptions)
    item.ChangeLabel(mProvider.SystemNameWithEmulator(item.Context().System()));
}

bool MenuBuilder::AddCustomSubMenu(const InheritableContext& itemContext, const ItemDefinition& subMenuItem)
{
  (void)itemContext;
  switch(subMenuItem.MenuType())
  {
    case MenuContainerType::StorageDevice:
    case MenuContainerType::AdvancedSingleEmulatorOptions:
    case MenuContainerType::Download:
    case MenuContainerType::Main:
    case MenuContainerType::SystemSettings:
    case MenuContainerType::StorageList:
    case MenuContainerType::Update:
    case MenuContainerType::Controllers:
    case MenuContainerType::CardReader:
    case MenuContainerType::RGBDual:
    case MenuContainerType::Crt:
    case MenuContainerType::RGBDual2:
    case MenuContainerType::RGBJAMMA:
    case MenuContainerType::JVS:
    case MenuContainerType::UI:
    case MenuContainerType::Screensaver:
    case MenuContainerType::Theme:
    case MenuContainerType::ThemeOptions:
    case MenuContainerType::PopupSettings:
    case MenuContainerType::GameFilters:
    case MenuContainerType::Lists:
    case MenuContainerType::InGame:
    case MenuContainerType::NetplayPasswords:
    case MenuContainerType::Arcade:
    case MenuContainerType::Tate:
    case MenuContainerType::Advanced:
    case MenuContainerType::BootSettings:
    case MenuContainerType::VirtualSystems:
    case MenuContainerType::Resolutions:
    case MenuContainerType::GamelistLayouts:
    case MenuContainerType::AdvancedEmulatorOptionsList:
    case MenuContainerType::Kodi:
    case MenuContainerType::Sound:
    case MenuContainerType::Network:
    case MenuContainerType::Scraper:
    case MenuContainerType::Gamelist:
    case MenuContainerType::Quit:
    case MenuContainerType::ChooseKodiLobby:
    case MenuContainerType::UserScripts:
    case MenuContainerType::InstallList:
    case MenuContainerType::EditGame:
    case MenuContainerType::DeleteOptions:
    case MenuContainerType::DeleteAdvanced:
    case MenuContainerType::SelectSoftpatch:
    case MenuContainerType::NetplayHost:
    case MenuContainerType::NetplayClient:
    case MenuContainerType::EmulatorSpecificSettings:
      break;
    case MenuContainerType::_Error_: { LOG(LogFatal) << "[MenuBuilder] Pretty impossible to see this error :)"; }
  }
  return false;
}

void MenuBuilder::AddSetting(const InheritableContext& itemContext, const ItemDefinition& item)
{
  assert(Context().HasSystem() && "Settings need system context !");

  // Check emulator/core
  const SystemData& system = *Context().System();
  const EmulatorList& list = system.Descriptor().EmulatorTree();
  bool belongsToSystem = false;
  for(int e = list.Count(); --e >= 0; )
    if (list.EmulatorAt(e).Name() == item.SettingEmulator())
      if (list.EmulatorAt(e).HasCore(item.SettingCore()))
      {
        belongsToSystem = true;
        break;
      }
  if (!belongsToSystem) return;

  // Try to read value
  String value;
  if (item.SettingTargetFile().Exists())
    switch(item.SettingFileType())
    {
      case ItemDefinition::FileType::SectionIni:
      {
        SectionFile f(item.SettingTargetFile(), true);
        String section;
        String key;
        if (item.SettingKey().Extract('/', section, key, true)) value = f.AsString(section, key);
        else value = f.AsString(key);
        break;
      }
      case ItemDefinition::FileType::SimpleIni:
      {
        SectionFile f(item.SettingTargetFile(), true);
        value = f.AsString(item.SettingKey());
        break;
      }
      case ItemDefinition::FileType::Unknown:
      default: { LOG(LogFatal) << "[MenuBuilder] Abnormal ItemDefinition::FileType::Unknown file type or unprocessed type found."; break; }
    }
  if (value.empty()) value = item.SettingDefault();

  switch(item.SettingValueProps())
  {
    case ItemDefinition::SettingProps::Singlequoted: value.Trim('\''); break;
    case ItemDefinition::SettingProps::DoubleQuoted: value.Trim('"'); break;
    case ItemDefinition::SettingProps::None:
    case ItemDefinition::SettingProps::Uppercase: value.UpperCase(); break;
    case ItemDefinition::SettingProps::Lowercase: value.LowerCase(); break;
    default: break;
  }

  // Get default grayed state
  bool defaultGrayed = false;
  if (item.HasGrayedCondition())
    if (SimpleExpressionEvaluator(*this).Evaluate(item.GrayedCondition()))
      defaultGrayed = true;

  // Create context
  InheritableContext context(itemContext);
  context.Merge(InheritableContext(&item.SettingDef()));

  // Create menu item
  switch(item.SettingValueType())
  {
    case ItemDefinition::SettingType::List:
    {
      String subItemValue;
      String subItemDisplay;
      SelectorEntry<String>::List subItems;
      for(const String& subItem : item.SettingValues().Split('|', true))
        if (subItem.Extract(':', subItemValue, subItemDisplay, true))
          subItems.push_back({ _S(subItemDisplay), subItemValue });
        else
          subItems.push_back({ subItem, subItem });
      AddList<String>(context, item, subItems, value, item.SettingDefault(), defaultGrayed, true);
      break;
    }
    case ItemDefinition::SettingType::NumericBoolean:
    {
      AddSwitch(context, item, value == "1", defaultGrayed, true);
      break;
    }
    case ItemDefinition::SettingType::TrueFalseBoolean:
    {
      AddSwitch(context, item, value.LowerCase() == "true", defaultGrayed, true);
      break;
    }
    case ItemDefinition::SettingType::YesNoBoolean:
    {
      AddSwitch(context, item, value.LowerCase() == "yes", defaultGrayed, true);
      break;
    }
    case ItemDefinition::SettingType::CustomBoolean:
    {
      String yes;
      String no;
      if (!item.SettingValues().Extract(':', yes, no, true)) { LOG(LogWarning) << "[MenuBuilder] Abnormal values for custom boolean setting: " << value; return; }
      AddSwitch(context, item, value.LowerCase() == yes.LowerCase(), defaultGrayed, true);
      break;
    }
    case ItemDefinition::SettingType::Range:
    {
      String::List limits = item.SettingValues().Split(':');
      if (limits.size() < 2 or limits.size() > 4) { LOG(LogWarning) << "[MenuBuilder] Abnormal values for Range setting: " << value; return; }
      float minRange = 0;
      float maxRange = 1;
      float step = 1;
      String unit;
      if (!limits[0].TryAsFloat(minRange)) { LOG(LogWarning) << "[MenuBuilder] Abnormal values for Range setting: " << value; return; }
      if (!limits[1].TryAsFloat(maxRange)) { LOG(LogWarning) << "[MenuBuilder] Abnormal values for Range setting: " << value; return; }
      if (limits.size() > 2)
        if (!limits[2].TryAsFloat(step)) { LOG(LogWarning) << "[MenuBuilder] Abnormal values for Range setting: " << value; return; }
      if (limits.size() > 3)
      {
        unit = limits[3];
        if (unit.StartsWith('+')) { unit.TrimLeft('+'); value.Remove(unit); }
      }
      AddSlider(context, item, minRange, maxRange, step, item.SettingDefault().AsFloat(), value.Trim().AsFloat(), unit, defaultGrayed, true);
      break;
    }
    case ItemDefinition::SettingType::Unknown:
    default: { LOG(LogFatal) << "[MenuBuilder] Abnormal ItemDefinition::SettingType::Unknown type or unprocessed type found."; break; }
  }
}

void MenuBuilder::AddItem(const InheritableContext& context, const ItemDefinition& item)
{
  bool isBeta =
    #if defined(BETA) || defined(DEBUG)
    true
    #else
    false
  #endif
  ;

  // Get default grayed state
  bool defaultGrayed = false;
  if (item.HasGrayedCondition())
    if (SimpleExpressionEvaluator(*this).Evaluate(item.GrayedCondition()))
      defaultGrayed = true;

  // Build items
  switch(item.Type())
  {
    // Version & platform
    case MenuItemType::RecalboxVersion: { AddText(context, item, MenuProvider::GetVersionString()); break; }
    case MenuItemType::RecalboxArch: { AddText(context, item, MenuProvider::GetArchString()); break; }
    case MenuItemType::UsedShare:
    {
      const StorageDevices::Device share = StorageDevices::Instance().GetShareDevices().front();
      String text = _S((_F(_("{0} free of {1}")) / share.HumanFree() / share.HumanSize())());
      AddBar(context, item, text, (float)share.Free / (float)share.Size, defaultGrayed);
      break;
    }
    case MenuItemType::ShareDevice:
    {
      // Storage device
      int currentIndex = 0;
      SelectorEntry<StorageDevices::Device>::List list = MenuDataProvider::GetShareEntries(currentIndex);
      if (!list.empty())
        AddList<StorageDevices::Device>(context, item, list, list[currentIndex].mValue, list[currentIndex].mValue, false);
      break;
    }
    case MenuItemType::DeviceList: break;
    case MenuItemType::Language: { AddList<String>(context, item, MenuDataProvider::GetCultureEntries(), mConf.GetSystemLanguage(), "en_US", defaultGrayed); break; }
    case MenuItemType::TimeZone: { AddList<String>(context, item, MenuDataProvider::GetTimeZones(), mConf.GetSystemTimezone(), "UTC", defaultGrayed); break; }
    case MenuItemType::Keyboard: { AddList<String>(context, item, MenuDataProvider::GetKeyboardEntries(), mConf.GetSystemKbLayout(), "us", defaultGrayed); break; }
    case MenuItemType::VirtualKeyboard: { AddList<RecalboxConf::VirtualKeyboardType>(context, item, MenuDataProvider::GetVirtualKeyboardEntries(), mConf.GetVirtualKeyboardType(), RecalboxConf::VirtualKeyboardType::Wheel, defaultGrayed); break; }
    case MenuItemType::Shutdown:
    case MenuItemType::FastShutdown:
    case MenuItemType::Restart:
    case MenuItemType::RunKodi:
    case MenuItemType::OpenLobby: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::EnableKodi: { AddSwitch(context, item, mConf.GetKodiEnabled(), defaultGrayed); break; }
    case MenuItemType::KodiResolution: { AddList<String>(context, item, mDataProvider.GetKodiResolutionsEntries(this), !mConf.IsDefinedKodiVideoMode() ? String::Empty : mConf.GetKodiVideoMode(), String::Empty, defaultGrayed); break; }
    case MenuItemType::KodiOnStartup: { AddSwitch(context, item, mConf.GetKodiAtStartup(), defaultGrayed); break; }
    case MenuItemType::KodiOnX: { AddSwitch(context, item, mConf.GetKodiXButton(), defaultGrayed); break; }
    case MenuItemType::CheckBios:
    case MenuItemType::ShowLicense:
    case MenuItemType::SystemInformation: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::ScrapeFrom: { AddList<ScraperType>(context, item, MenuDataProvider::GetScrapersEntries(), mConf.GetScraperSource(), ScraperType::ScreenScraper, defaultGrayed); break; }
    case MenuItemType::ScrapeAuto: { AddSwitch(context, item, mConf.GetScraperAuto(), defaultGrayed); break; }
    case MenuItemType::ScrapeOptionNameFrom: { AddList<ScraperNameOptions>(context, item, MenuDataProvider::GetScraperNameOptionsEntries(), mConf.GetScraperNameOptions(), ScraperNameOptions::GetFromScraper, defaultGrayed); break; }
    case MenuItemType::ScrapeOptionImage: { AddList<ScreenScraperEnums::ScreenScraperImageType>(context, item, MenuDataProvider::GetScraperImagesEntries(), mConf.GetScreenScraperMainMedia(), ScreenScraperEnums::ScreenScraperImageType::MixV2, defaultGrayed); break; }
    case MenuItemType::ScrapeOptionVideo: { AddList<ScreenScraperEnums::ScreenScraperVideoType>(context, item, MenuDataProvider::GetScraperVideosEntries(), mConf.GetScreenScraperVideo(), ScreenScraperEnums::ScreenScraperVideoType::None, defaultGrayed); break; }
    case MenuItemType::ScrapeOptionBox: { AddList<ScreenScraperEnums::ScreenScraperBoxType>(context, item, MenuDataProvider::GetScraperBoxesEntries(), mConf.GetScreenScraperBox(), ScreenScraperEnums::ScreenScraperBoxType::Box2d, defaultGrayed); break; }
    case MenuItemType::ScrapeOptionLogo: { AddList<ScreenScraperEnums::ScreenScraperLogoType>(context, item, MenuDataProvider::GetScraperLogoEntries(), mConf.GetScreenScraperLogo(), ScreenScraperEnums::ScreenScraperLogoType::Clear, defaultGrayed); break; }
    case MenuItemType::ScrapeOptionRegionPriority: { AddList<ScreenScraperEnums::ScreenScraperRegionPriority>(context, item, MenuDataProvider::GetScraperRegionOptionsEntries(), mConf.GetScreenScraperRegionPriority(), ScreenScraperEnums::ScreenScraperRegionPriority::DetectedRegion, defaultGrayed); break; }
    case MenuItemType::ScrapeOptionRegionFavorite: { AddList<Regions::GameRegions>(context, item, MenuDataProvider::GetRegionEntries(false), mConf.GetScreenScraperRegion(), Regions::GameRegions::WOR, defaultGrayed); break; }
    case MenuItemType::ScrapeOptionLanguageFavorite: { AddList<Languages>(context, item, MenuDataProvider::GetScraperLanguagesEntries(), LanguagesTools::GetScrapingLanguage(), Languages::EN, defaultGrayed); break; }
    case MenuItemType::ScrapeOptionManual: { AddSwitch(context, item, mConf.GetScreenScraperWantManual(), defaultGrayed); break; }
    case MenuItemType::ScrapeOptionMap: { AddSwitch(context, item, mConf.GetScreenScraperWantMaps(), defaultGrayed); break; }
    case MenuItemType::ScrapeOptionTips: { AddSwitch(context, item, mConf.GetScreenScraperWantTips(), defaultGrayed); break; }
    case MenuItemType::ScrapeOptionP2K: { AddSwitch(context, item, mConf.GetScreenScraperWantP2K(), defaultGrayed); break; }
    case MenuItemType::ScrapeOptionUsername:
    {
      if (!mItemsTypes.contains(MenuItemType::ScrapeFrom)) { LOG(LogError) << "[MenuBuilder] MenuItemType::ScrapeOptionUsername must be created AFTER MenuItemType::ScrapeFrom."; }
      ScraperType type = mItemsTypes[MenuItemType::ScrapeFrom]->AsList<ScraperType>()->SelectedValue();
      AddEditor(context, item, ScraperFactory::GetLogin(type), false, !ScraperFactory::HasCredentials(type));
      break;
    }
    case MenuItemType::ScrapeOptionPassword:
    {
      if (!mItemsTypes.contains(MenuItemType::ScrapeFrom)) { LOG(LogError) << "[MenuBuilder] MenuItemType::ScrapeOptionPassword must be created AFTER MenuItemType::ScrapeFrom."; }
      ScraperType type = mItemsTypes[MenuItemType::ScrapeFrom]->AsList<ScraperType>()->SelectedValue();
      AddEditor(context, item, ScraperFactory::GetPassword(type), true, !ScraperFactory::HasCredentials(type));
      break;
    }
    case MenuItemType::ScrapeFilter: { AddList<ScrapingMethod>(context, item, MenuDataProvider::GetScrapingMethods(), ScraperFactory::Instance().ScrapingMethod(), ScrapingMethod::AllIfNothingExists, defaultGrayed); break; }
    case MenuItemType::ScrapeSystems: { AddMultiList<SystemData*>(context, item, mDataProvider.GetScrapableSystemsEntries(), defaultGrayed); break; }
    case MenuItemType::ScrapeRun: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::Rewind: { AddSwitch(context, item, mConf.GetGlobalRewind(), defaultGrayed); break; }
    case MenuItemType::SoftPatching: { AddList<RecalboxConf::SoftPatching>(context, item, MenuDataProvider::GetSoftpatchingEntries(), mConf.GetGlobalSoftpatching(), RecalboxConf::SoftPatching::Disable, defaultGrayed); break; }
    case MenuItemType::ShowSaveStates: { AddSwitch(context, item, mConf.GetGlobalShowSaveStateBeforeRun(), defaultGrayed); break; }
    case MenuItemType::AutoSave: { AddSwitch(context, item, mConf.GetGlobalAutoSave(), defaultGrayed); break; }
    case MenuItemType::PressTwice: { AddSwitch(context, item, mConf.GetGlobalQuitTwice(), defaultGrayed); break; }
    case MenuItemType::SuperGameboyMode: { AddList<String>(context, item, MenuDataProvider::GetSuperGameBoyEntries(), mConf.GetSuperGameBoy(), "gb", defaultGrayed); break; }
    case MenuItemType::GameRatio: { AddList<String>(context, item, MenuDataProvider::GetRatioEntries(), mConf.GetGlobalRatio(), "auto", defaultGrayed); break; }
    case MenuItemType::RecalboxOverlays: { AddSwitch(context, item, mConf.GetGlobalRecalboxOverlays(), defaultGrayed); break; }
    case MenuItemType::Smooth: { AddSwitch(context, item, mConf.GetGlobalSmooth(), defaultGrayed); break;}
    case MenuItemType::IntegerScale: { AddSwitch(context, item, mConf.GetGlobalIntegerScale(), defaultGrayed); break; }
    case MenuItemType::ShaderSet: { AddList<String>(context, item, MenuDataProvider::GetShaderSetEntries(), mConf.GetGlobalShaderSet(), "none", defaultGrayed); break; }
    case MenuItemType::AdvancedShaders:{ AddList<String>(context, item, MenuDataProvider::GetShadersEntries(), mConf.GetGlobalShaders(), "", defaultGrayed); break; }
    case MenuItemType::HDMode:{ AddSwitch(context, item, mConf.GetGlobalHDMode(), defaultGrayed); break; }
    case MenuItemType::WideScreen: { AddSwitch(context, item, mConf.GetGlobalWidescreenMode(), defaultGrayed); break; }
    case MenuItemType::AutoBlitter: { AddList<String>(context, item, mDataProvider.GetAutoBlitterEntries(), mConf.GetGlobalAutoBlitter(), mConf.GetGlobalAutoBlitterDefault(), defaultGrayed); break; }
    case MenuItemType::VulkanDriver: { AddSwitch(context, item, mConf.GetGlobalVulkanDriver(), defaultGrayed); break; }
    case MenuItemType::Retroachievements: { AddSwitch(context, item, mConf.GetRetroAchievementOnOff(), defaultGrayed); break; }
    case MenuItemType::RetroachievementsHardcore: { AddSwitch(context, item, mConf.GetRetroAchievementHardcore(), defaultGrayed); break; }
    case MenuItemType::RetroachievementsUsername: { AddEditor(context, item, mConf.GetRetroAchievementLogin(), false, defaultGrayed); break; }
    case MenuItemType::RetroachievementsPassword: { AddEditor(context, item, mConf.GetRetroAchievementPassword(), true, defaultGrayed); break; }
    case MenuItemType::RetroachievementsSounds: { AddSwitch(context, item, mConf.GetRetroAchievementSounds(), defaultGrayed); break; }
    case MenuItemType::Netplay: { AddSwitch(context, item, mConf.GetNetplayEnabled(), defaultGrayed); break; }
    case MenuItemType::NetplayNickname: { AddEditor(context, item, mConf.GetNetplayLogin(), false, defaultGrayed); break; }
    case MenuItemType::NetplayPort: { AddEditor(context, item, String(mConf.GetNetplayPort()), false, defaultGrayed, false, VKCharFilter::Numeric); break; }
    case MenuItemType::NetplayRelay: { AddList<RecalboxConf::Relay>(context, item, MenuDataProvider::GetMitmEntries(), mConf.GetNetplayRelay(), RecalboxConf::Relay::None, defaultGrayed); break; }
    case MenuItemType::HashRoms:
    case MenuItemType::Changelog: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::UpdateCheck: { AddSwitch(context, item, mConf.GetUpdatesEnabled(), defaultGrayed); break; }
    case MenuItemType::UpdateAvailable: { AddText(context, item, Upgrade::Instance().PendingUpdate() ? Upgrade::Instance().NewVersion() : _("NO")); break; }
    case MenuItemType::UpdateChangelog:
    case MenuItemType::UpdateStart: { AddAction(context, item, true, !Upgrade::Instance().PendingUpdate()); break; }
    case MenuItemType::UpdateType: { if (isBeta || PatronInfo::Instance().IsPatron()) AddList<RecalboxConf::UpdateType>(context, item, MenuDataProvider::GetUpdateTypeEntries(), mConf.GetUpdateType(), RecalboxConf::UpdateType::Stable, defaultGrayed); break; }
    case MenuItemType::UpdateCheckNow: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::NetworkStatus: { AddTask(new MenuTaskRefreshConnection(*AddText(context, item, _("TESTING CONNECTION...")))); break; }
    case MenuItemType::NetworkIP: { AddTask(new MenuTaskRefreshIP(*AddText(context, item, _("UNAVAILABLE")))); break; }
    case MenuItemType::NetworkHostname: { AddEditor(context, item, mConf.GetHostname(), false, defaultGrayed); break; }
    case MenuItemType::NetworkEnableWIFI: { AddSwitch(context, item, mConf.GetWifiEnabled(), defaultGrayed); break; }
    case MenuItemType::NetworkConnect: { AddSwitch(context, item, mConf.GetWifiConnect(), defaultGrayed); break; }
    case MenuItemType::NetworkPickSSID: { AddTask(new MenuTaskRefreshSSID(*AddList<String>(context, item, { { _("NO WIFI ACCESS POINT"), String::Empty } }, mConf.GetWifiSSID(), String::Empty, defaultGrayed))); break; }
    case MenuItemType::NetworkSSID: { AddEditor(context, item, mConf.GetWifiSSID(), false, defaultGrayed); break; }
    case MenuItemType::NetworkPassword: { AddEditor(context, item, mConf.GetWifiKey(), true, defaultGrayed); break; }
    case MenuItemType::NetworkWPS: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::ShowOnlyLastVersions: { AddSwitch(context, item, mConf.GetShowOnlyLatestVersion(), defaultGrayed); break; }
    case MenuItemType::ShowOnlyFavorites: { AddSwitch(context, item, mConf.GetFavoritesOnly(), defaultGrayed); break; }
    case MenuItemType::ShowFavoriteFirst: { AddSwitch(context, item, mConf.GetFavoritesFirst(), defaultGrayed); break; }
    case MenuItemType::ShowHiddenGames: { AddSwitch(context, item, mConf.GetShowHidden(), defaultGrayed); break; }
    case MenuItemType::ShowMahjongCasino: { AddSwitch(context, item, !mConf.GetHideBoardGames(), defaultGrayed); break; }
    case MenuItemType::ShowAdultGames: { AddSwitch(context, item, !mConf.GetFilterAdultGames(), defaultGrayed); break; }
    case MenuItemType::ShowPreinstalledGames:  { AddSwitch(context, item, !mConf.GetGlobalHidePreinstalled(), defaultGrayed); break; }
    case MenuItemType::Show3PlayerGames: { AddSwitch(context, item, mConf.GetShowOnly3PlusPlayers(), defaultGrayed); break; }
    case MenuItemType::ShowOnlyYoko: { AddSwitch(context, item, mConf.GetShowOnlyYokoGames(), defaultGrayed); break; }
    case MenuItemType::ShowOnlyTate: { AddSwitch(context, item, mConf.GetShowOnlyTateGames(), defaultGrayed); break; }
    case MenuItemType::ShowNonGames: { AddSwitch(context, item, !mConf.GetHideNoGames(), defaultGrayed); break; }
    case MenuItemType::ScreensaverTimeout: { AddSlider(context, item, 0.f, 30.f, 1.f, 5.f,  (float)mConf.GetScreenSaverTime(), "m", defaultGrayed); break; }
    case MenuItemType::ScreensaverType: { AddList<RecalboxConf::Screensaver>(context, item, mDataProvider.GetTypeEntries(), mConf.GetScreenSaverType(), RecalboxConf::Screensaver::Dim, defaultGrayed); break; }
    case MenuItemType::ScreensaverSystems: { AddMultiList<String>(context, item, mDataProvider.GetSystemListAsString(), defaultGrayed); break; }
    case MenuItemType::PopupHelpDuration: { AddSlider(context, item, 0.f, 10.f, 1.f, 10.f, (float)mConf.GetPopupHelp(), "s", defaultGrayed); break; }
    case MenuItemType::PopupMusic: { AddSlider(context, item, 0.f, 10.f, 1.f, 5.f, (float)mConf.GetPopupMusic(), "s", defaultGrayed); break; }
    case MenuItemType::PopupNetplay: { AddSlider(context, item, 0.f, 10.f, 1.f, 8.f, (float)mConf.GetPopupNetplay(), "s", defaultGrayed); break; }
    case MenuItemType::ThemeManager:
    case MenuItemType::OpenJukebox: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::ThemeSet: { AddList<ThemeSpec>(context, item, MenuDataProvider::GetThemeEntries(mWindow), mDataProvider.GetCurrentTheme(), MenuDataProvider::GetDefaultTheme(), defaultGrayed); break; }
    case MenuItemType::ThemeCarousel: { AddSwitch(context, item, mConf.GetThemeCarousel(), defaultGrayed); break; }
    case MenuItemType::ThemeTransition: { AddList<RecalboxConf::Transition>(context, item, mDataProvider.GetThemeTransitionEntries(), mConf.GetThemeTransition(), RecalboxConf::Transition::Slide, defaultGrayed); break; }
    case MenuItemType::GameTransition: { AddList<RecalboxConf::Transition>(context, item, mDataProvider.GetGameTransitionEntries(), mConf.GetGameTransition(), RecalboxConf::Transition::Slide, defaultGrayed); break; }
    case MenuItemType::ThemeRegion: { AddList<String>(context, item, mDataProvider.GetThemeRegionEntries(), mConf.GetThemeRegion(), "us", defaultGrayed); break; }
    case MenuItemType::ThemeOptions: { assert(context.HasOption() && "No theme option context");  BuildThemeOptionSelector(context, item, context.Option()); break; }
    case MenuItemType::Brightness: { AddSlider(context, item, 0.f, 8.f, 1.f, 6.f, (float)mConf.GetBrightness(), "", defaultGrayed); break; }
    case MenuItemType::SystemSorting: { AddList<SystemSorting>(context, item, MenuDataProvider::GetSystemSortingEntries(), mConf.GetSystemSorting(), SystemSorting::Default, defaultGrayed); break; }
    case MenuItemType::QuickSelectSystem: { AddSwitch(context, item, mConf.GetQuickSystemSelect(), defaultGrayed); break; }
    case MenuItemType::ListFastMoveEnabled: { AddSwitch(context, item, mConf.GetListFastMoveEnabled(), defaultGrayed); break; }
    case MenuItemType::OnScreenHelp: { AddSwitch(context, item, mConf.GetShowHelp(), defaultGrayed); break; }
    case MenuItemType::SwapValidateCancel: { AddSwitch(context, item, mConf.GetSwapValidateAndCancel(), defaultGrayed); break; }
    case MenuItemType::OSDClock: { AddSwitch(context, item, mConf.GetClock(), defaultGrayed); break; }
    case MenuItemType::OneGameOneRom: { AddSwitch(context, item, mConf.GetOneGameOneRom(), defaultGrayed); break; }
    case MenuItemType::RomPreferredRegion: { AddList<Regions::GameRegions>(context, item, MenuDataProvider::GetRegionEntries(false), Regions::Clamp(mConf.GetRomPreferredRegion()), Regions::GameRegions::EU, defaultGrayed); break; }
    case MenuItemType::RomSecondPreferredRegion: { AddList<Regions::GameRegions>(context, item, MenuDataProvider::GetRegionEntries(true), Regions::Clamp(mConf.GetRomSecondPreferredRegion()), Regions::GameRegions::EU, defaultGrayed); break; }
    case MenuItemType::RomPreferredFallback: { AddList<Regions::OrderedMasterRegions>(context, item, MenuDataProvider::GetMasterRegionEntries(), mConf.GetRomPreferredFallback(), Regions::OrderedMasterRegions::WorEuUsJp, defaultGrayed); break; }
    case MenuItemType::DisplayByFilename: { AddSwitch(context, item, mConf.GetDisplayByFileName(), defaultGrayed); break; }
    case MenuItemType::UpdateGamelists: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::ShowSystems: { AddMultiList<SystemData*>(context, item, mDataProvider.GetSystemShown(), defaultGrayed); break; }
    case MenuItemType::ArcadeEnhancedView: { AddSwitch(context, item, mConf.GetArcadeViewEnhanced(), defaultGrayed); break; }
    case MenuItemType::ArcadeFoldClones: { AddSwitch(context, item, mConf.GetArcadeViewFoldClones(), defaultGrayed); break; }
    case MenuItemType::ArcadeHideBios: { AddSwitch(context, item, mConf.GetArcadeViewHideBios(), defaultGrayed); break; }
    case MenuItemType::ArcadeHideNonWorking: { AddSwitch(context, item, mConf.GetArcadeViewHideNonWorking(), defaultGrayed); break; }
    case MenuItemType::ArcadeUseOfficielNames: { AddSwitch(context, item, mConf.GetArcadeUseDatabaseNames(), defaultGrayed); break; }
    case MenuItemType::ArcadeManufacturerSystems: { AddMultiList<String>(context, item, MenuDataProvider::GetManufacturersVirtualEntries(), defaultGrayed); break; }
    case MenuItemType::ArcadeAggregateEnable: { AddSwitch(context, item, mConf.GetCollectionArcade(), defaultGrayed); break; }
    case MenuItemType::ArcadeAggregateOriginal: { AddSwitch(context, item, mConf.GetCollectionArcadeHideOriginals(), !mConf.GetCollectionArcade()); break; }
    case MenuItemType::BootOnKodi: { AddSwitch(context, item, mConf.GetKodiAtStartup(), defaultGrayed); break; }
    case MenuItemType::BootDoNotScan: { AddSwitch(context, item, mConf.GetStartupGamelistOnly(), defaultGrayed); break; }
    case MenuItemType::AllowBootOnGame: { AddSwitch(context, item, mConf.GetBootOnGameEnabled(), defaultGrayed); break; }
    case MenuItemType::BootOnSystem: { AddList<String>(context, item, mDataProvider.GetBootSystemEntries(), mConf.GetStartupSelectedSystem(), SystemManager::sFavoriteSystemShortName, defaultGrayed); break; }
    case MenuItemType::BootOnGamelist: { AddSwitch(context, item, mConf.GetStartupStartOnGamelist(), defaultGrayed); break; }
    case MenuItemType::BootShowVideo: { AddSwitch(context, item, mConf.GetSplashEnabled(), defaultGrayed); break; }
    case MenuItemType::HideSystemView: { AddSwitch(context, item, mConf.GetStartupHideSystemView(), defaultGrayed); break; }
    case MenuItemType::VirtualAllGames: { AddSwitch(context, item, mConf.GetCollectionAllGames(), defaultGrayed); break; }
    case MenuItemType::VirtualMultiplayer: { AddSwitch(context, item, mConf.GetCollectionMultiplayer(), defaultGrayed); break; }
    case MenuItemType::VirtualLastPlayed: { AddSwitch(context, item, mConf.GetCollectionLastPlayed(), defaultGrayed); break; }
    case MenuItemType::VirtualLightgun: { AddSwitch(context, item, mConf.GetCollectionLightGun(), defaultGrayed); break; }
    case MenuItemType::VirtualDial: { AddSwitch(context, item, mConf.GetCollectionDial(), defaultGrayed); break; }
    case MenuItemType::VirtualTrackball: { AddSwitch(context, item, mConf.GetCollectionTrackball(), defaultGrayed); break; }
    case MenuItemType::VirtualPorts: { AddSwitch(context, item, mConf.GetCollectionPorts(), defaultGrayed); break;}
    case MenuItemType::VirtualPerGenre: { AddMultiList<GameGenres>(context, item, mDataProvider.GetMultiGenreEntries(), defaultGrayed); break; }
    case MenuItemType::AdvDebugLogs: { AddSwitch(context, item, mConf.GetDebugLogs(), defaultGrayed); break; }
    case MenuItemType::AdvShowFPS: { AddSwitch(context, item, mConf.GetGlobalShowFPS(), defaultGrayed); break; }
    case MenuItemType::ResolutionGlobal: { AddList<String>(context, item, mDataProvider.GetGlobalResolutionEntries(this), mConf.GetGlobalVideoMode(), ResolutionAdapter().DefaultResolution().ToString(), defaultGrayed); break; }
    case MenuItemType::ResolutionFrontEnd: { AddList<String>(context, item, mDataProvider.GetResolutionEntries(this), mConf.GetEmulationstationVideoMode(), String::Empty, defaultGrayed); break; }
    case MenuItemType::ResolutionEmulators: { assert(context.HasSystem() && "No system context"); AddList<String>(context, item, mDataProvider.GetResolutionEntries(this), mConf.GetSystemVideoModeNoDefault(*context.System()), String::Empty, defaultGrayed, true); break; }
    case MenuItemType::DeviceFreeSpace: { AddBar(context, item, (_F(_("{0} FREE")) / Context().Device()->HumanFree())(), 1.0f - (float)((double)Context().Device()->Free / (double)Context().Device()->Size), defaultGrayed); break; }
    case MenuItemType::DeviceName: { AddText(context, item, Context().Device()->DisplayableShortName())->Parent().SetTitle(Context().Device()->DisplayableShortName()); break; }
    case MenuItemType::DeviceFS: { AddText(context, item, Context().Device()->FileSystem); break; }
    case MenuItemType::DeviceNetworkAccess: { AddText(context, item, Context().Device()->MountPoint.empty() ? _("NO ACCESS") : String(R"(\\RECALBOX\share\externals\)").Append(Path(Context().Device()->MountPoint).Filename())); break; }
    case MenuItemType::DeviceUsable: { AddText(context, item, Context().Device()->RecalboxCompatible() ? (Context().Device()->IsNTFS() ? _("YES, BUT NOT RECOMMENDED") : _("YES")) : _("NO")); break; }
    case MenuItemType::DeviceInitialize:
    {
      bool alreadyInitialized = mProvider.SystemManager().HasRomStructure(Path(Context().Device()->MountPoint), Context().Device()->ReadOnly);
      AddAction(context, item, true, !Context().Device()->RecalboxCompatible() || alreadyInitialized); break;
    }
    case MenuItemType::WebManagerEnable: { AddSwitch(context, item, mConf.GetSystemManagerEnabled(), defaultGrayed); break; }
    case MenuItemType::FactoryReset:
    case MenuItemType::EmulatorsReset: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::Overclock: { AddList<Overclocking::Overclock>(context, item, MenuDataProvider::GetOverclockEntries(), Board::Instance().GetOverclocking().GetCurrentOverclock(), Overclocking::NoOverclock(), defaultGrayed); break; }
    case MenuItemType::PerSystemEmulator: { String emulatorAndCore; String defaultEmulatorAndCore; AddList(context, item, mDataProvider.GetEmulatorEntries(Context().System(), emulatorAndCore, defaultEmulatorAndCore), emulatorAndCore, defaultEmulatorAndCore, defaultGrayed); break; }
    case MenuItemType::PerSystemRatio: { AddList(context, item, MenuDataProvider::GetRatioEntries(), mConf.GetSystemRatio(*Context().System()), String::Empty, defaultGrayed); break; }
    case MenuItemType::PerSystemSmooth: { AddSwitch(context, item, mConf.GetSystemSmooth(*Context().System()), defaultGrayed); break; }
    case MenuItemType::PerSystemRewind: { AddSwitch(context, item, mConf.GetSystemRewind(*Context().System()), defaultGrayed); break; }
    case MenuItemType::PerSystemAutoLoadSave: { AddSwitch(context, item, mConf.GetSystemAutoSave(*Context().System()), defaultGrayed); break; }
    case MenuItemType::PerSystemShaderSet: { AddList(context, item, MenuDataProvider::GetShaderSetEntries(), mConf.GetSystemShaderSet(*Context().System()), String::Empty, defaultGrayed); break; }
    case MenuItemType::PerSystemShaders: { AddList(context, item, MenuDataProvider::GetShadersEntries(), mConf.GetSystemShaders(*Context().System()), String("none"), defaultGrayed); break; }
    case MenuItemType::PerSystemCrtHandheldFormat: { AddList(context, item, MenuDataProvider::GetCrtHandheldFormatEntries(), mConf.GetSystemCrtHandheldFormat(*Context().System()), mConf.GetGlobalCrtHandheldFormatDefault(), defaultGrayed); break; }
    case MenuItemType::CrtHandheldFormat: { AddList(context, item, MenuDataProvider::GetCrtHandheldFormatEntries(), mConf.GetGlobalCrtHandheldFormat(), mConf.GetGlobalCrtHandheldFormatDefault(), defaultGrayed); break; }
    case MenuItemType::BootloaderUpdate: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::CRVersion:
    {
      String version = CardReader::Instance().GetCardReaderVersion();
      if(CardReader::Instance().GetCardReaderVersion() != CardReader::Instance().GetModuleVersion())
        version.Append(' ').Append(_F(_("({0} AVAILABLE)")) / CardReader::Instance().GetModuleVersion());
      else
        version.Append(" (").Append(_("UP TO DATE")).Append(")");
      AddText(context, item, version);
      break;
    }
    case MenuItemType::CRConsoleMode: { AddSwitch(context, item, mConf.GetCRConsoleMode(), false); break; }
    case MenuItemType::CRConsoleModeExitMode: { AddList<String>(context, item, MenuDataProvider::GetCRConsoleModeExit(), mConf.GetCRConsoleModeExit(), mConf.GetCRConsoleModeExitDefault(), false); break; }
    case MenuItemType::CRConsoleModeVideo: { AddList<String>(context, item, MenuDataProvider::GetCRConsoleModeVideo(), mConf.GetCRConsoleModeVideo(), mConf.GetCRConsoleModeVideoDefault(), false); break;}
    case MenuItemType::CRConsoleModeSound: { AddList<String>(context, item, MenuDataProvider::GetCRConsoleModeSound(), mConf.GetCRConsoleModeSound(), mConf.GetCRConsoleModeSoundDefault(), false); break;}
    case MenuItemType::CrtAdapter: { AddList<CrtAdapterType>(context, item, MenuDataProvider::GetDacEntries(), mResolver.HasRRGBD() ? CrtAdapterType::RGBDual : CrtConf::Instance().GetSystemCRT(), CrtAdapterType::None, defaultGrayed); break; }
    case MenuItemType::CrtMenuResolution: { AddList<String>(context, item, MenuDataProvider::GetEsResolutionEntries(), mCrtConf.GetSystemCRTResolution(), mCrtConf.GetSystemCRTResolutionDefault(), defaultGrayed); break; }
    case MenuItemType::CrtScreenType:
    {
      // Polymorphic item
      if (mResolver.HasJamma() || mResolver.HasJVS() || Board::Instance().CrtBoard().MustForce31kHzByDefault())
      {
        const bool onHdmi  = !mResolver.HasCrtAttached();
        const bool grayed  = Board::Instance().CrtBoard().MustForce15kHz()
                          || Board::Instance().CrtBoard().MustForce31kHz()
                          || onHdmi;
        const auto current = Board::Instance().CrtBoard().GetHorizontalFrequency();
        if (onHdmi)
          AddList<ICrtInterface::HorizontalFrequency>(context, item, { { "HDMI", current } }, current, current, true);
        else
          AddList<ICrtInterface::HorizontalFrequency>(context, item, MenuDataProvider::GetHorizontalOutputFrequency(), current, ICrtInterface::HorizontalFrequency::Auto, grayed);
      }
      break;
    }
    case MenuItemType::CrtForced50hzDip:
    {
        String result(Board::Instance().CrtBoard().MustForce50Hz() ? _("ON") : _("OFF"));
        if (Board::Instance().CrtBoard().HasForced50hzSupport()) AddText(context, item, result);
        break;
    }
    case MenuItemType::CrtForced31kHzDip:
    {
      String result(Board::Instance().CrtBoard().MustForce31kHzByDefault() ? _("ON") : _("OFF"));
      if (Board::Instance().CrtBoard().HasForced31khzSupport()) AddText(context, item, result);
      break;
    }
    case MenuItemType::CrtForcedCompositeDip:
    {
      String result(Board::Instance().CrtBoard().MustForceComposite() ? _("ON") : _("OFF"));
      if (Board::Instance().CrtBoard().HasForcedCompositeSupport()) AddText(context, item, result);
      break;
    }
    case MenuItemType::CrtForceComposite: { AddSwitch(context, item, mCrtConf.GetSystemCRTForceComposite(), defaultGrayed); break; }
    case MenuItemType::CrtCompositeStandard: { AddList<String>(context, item, { { "NTSC", "0" }, { "NTSC 4.43", "1" }, { "NTSC-J", "2" }, { "PAL", "3" } }, mCrtConf.GetSystemCRTCompositeStandard(), mCrtConf.GetSystemCRTCompositeStandardDefault(),  defaultGrayed); break; }
    case MenuItemType::CrtHDMIPriority:
    {
      const bool hdmiLocked = JvsCab::IsHdmiOnly();
      AddSwitch(context, item, hdmiLocked ? true : mCrtConf.GetSystemCRTForceHDMI(), defaultGrayed || hdmiLocked);
      break;
    }
    case MenuItemType::CrtSelectRefreshAtLaunch: { AddSwitch(context, item, mCrtConf.GetSystemCRTGameRegionSelect(), defaultGrayed); break; }
    case MenuItemType::CrtSelectResolutionAtLaunch: { AddSwitch(context, item, mCrtConf.GetSystemCRTGameResolutionSelect(), defaultGrayed); break; }
    case MenuItemType::CrtSuperrezMultiplier: { AddList<String>(context, item, MenuDataProvider::GetSuperRezEntries(), mCrtConf.GetSystemCRTSuperrez(), mCrtConf.GetSystemCRTSuperrezDefault(),  defaultGrayed); break; }
    case MenuItemType::CrtForceJack: { AddSwitch(context, item, mCrtConf.GetSystemCRTForceJack(), defaultGrayed); break; }
    case MenuItemType::CrtScanline240in480: { AddList<CrtScanlines>(context, item, MenuDataProvider::GetScanlinesEntries(), mCrtConf.GetSystemCRTScanlines31kHz(), CrtScanlines::None, Board::Instance().CrtBoard().GetHorizontalFrequency() < ICrtInterface::HorizontalFrequency::KHz31); break;}
    case MenuItemType::CrtDemoAuto240: { AddSwitch(context, item, mCrtConf.GetSystemCRTRunDemoAndAutoIn240pOn31kHz(), Board::Instance().CrtBoard().GetHorizontalFrequency() < ICrtInterface::HorizontalFrequency::KHz31); break; }
    case MenuItemType::CrtWideScreen: { AddSwitch(context, item, mCrtConf.GetSystemCRTWideScreen(), defaultGrayed); break; }
    case MenuItemType::CrtSelectSignalAtLaunch: { AddSwitch(context, item, mCrtConf.GetSystemCRTSignalSelect(), defaultGrayed); break; }
    case MenuItemType::CrtAvoidInterlaced: { AddSwitch(context, item, mCrtConf.GetSystemCRTAvoidInterlaced(), !Board::Instance().CrtBoard().HasInterlacedSupport()); break; }
    case MenuItemType::CrtAvoidLowFreqModes: { AddSwitch(context, item, mCrtConf.GetSystemCRTAvoidLowFreqModes(), Board::Instance().CrtBoard().MustForce50Hz() || Board::Instance().CrtBoard().MustForceComposite()); break; }
    case MenuItemType::CrtAvoidInterlacedTateOnYokoAndHandhelds: { AddSwitch(context, item, mCrtConf.GetSystemCRTAvoidInterlacedOnTateOnYokoAndHandhelds(), !Board::Instance().CrtBoard().HasInterlacedSupport()); break; }
    case MenuItemType::JammaSound: { AddList<String>(context, item, { { "JACK/MONO", "0" }, { "JACK/PINS", "1" } }, CrtConf::Instance().GetSystemCRTJammaAmpDisable() ? "1" : "0", "0", defaultGrayed); break; }
    case MenuItemType::JammaAmpBoost: { AddList<String>(context, item, { { "default", "0" }, { "+6dB", "1" }, { "+12dB", "2" }, { "+16dB", "3" } }, CrtConf::Instance().GetSystemCRTJammaMonoAmpBoost(), "0", defaultGrayed); break;}
    case MenuItemType::JammaPanelType: { const bool grayed = defaultGrayed || (mResolver.HasJVS() && !JvsCab::SupportsPanelType()); AddList<String>(context, item, { { "2 buttons", "2" }, { "3 buttons", "3" }, { "4 buttons", "4" }, { "5 buttons", "5" }, { "6 buttons", "6" } }, CrtConf::Instance().GetSystemCRTJammaPanelButtons(), "2", grayed); break; }
    case MenuItemType::JammaNeogeoLayoutP1: { AddList<String>(context, item, { { "Default", "neodefault" }, { "Line", "line" }, { "Square", "square" } }, CrtConf::Instance().GetSystemCRTJammaNeogeoLayoutP1(), "neodefault", defaultGrayed); break; }
    case MenuItemType::JammaNeogeoLayoutP2: { AddList<String>(context, item, { { "Default", "neodefault" }, { "Line", "line" }, { "Square", "square" } }, CrtConf::Instance().GetSystemCRTJammaNeogeoLayoutP2(), "neodefault", defaultGrayed); break; }
    case MenuItemType::Jamma4PlayerMode: { AddSwitch(context, item, mCrtConf.GetSystemCRTJamma4Players(), defaultGrayed); break; }
    case MenuItemType::JammaCredit: { AddSwitch(context, item, mCrtConf.GetSystemCRTJammaStartBtn1Credit(), defaultGrayed); break; }
    case MenuItemType::JammaHotkey: { AddSwitch(context, item, mCrtConf.GetSystemCRTJammaHKOnStart(), defaultGrayed); break; }
    case MenuItemType::JammaVolume: { AddSwitch(context, item, mCrtConf.GetSystemCRTJammaSoundOnStart(), defaultGrayed); break; }
    case MenuItemType::JammaExit: { AddSwitch(context, item, mCrtConf.GetSystemCRTJammaExitOnStart(), defaultGrayed); break; }
    case MenuItemType::JammaAutoFire: { AddSwitch(context, item, mCrtConf.GetSystemCRTJammaAutoFire(), defaultGrayed); break; }
    case MenuItemType::JammaDualJoystick: { AddSwitch(context, item, mCrtConf.GetSystemCRTJammaDualJoysticks(), defaultGrayed); break; }
    case MenuItemType::JammaPinE27: { AddSwitch(context, item, mCrtConf.GetSystemCRTJammaButtonsOnJamma() != "6", defaultGrayed); break; }
    case MenuItemType::JammaReset: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::JvsCabType: { const bool grayed = defaultGrayed || !JvsCab::FirmwareUiDisabled(); AddList<String>(context, item, MenuDataProvider::GetJvsCabTypeEntries(), JvsCab::CurrentValue(), JvsCab::DefaultTypeValue(), grayed); break; }
    case MenuItemType::JvsStartQuickExit: { AddSwitch(context, item, JvsCab::StartQuickExit(), defaultGrayed); break; }
    case MenuItemType::JvsSkipBootLogo: { AddSwitch(context, item, JvsCab::SkipBootLogo(), defaultGrayed); break; }
    case MenuItemType::JvsFlipGunX: { AddSwitch(context, item, JvsCab::FlipGunX(), defaultGrayed); break; }
    case MenuItemType::JvsFlipGunY: { AddSwitch(context, item, JvsCab::FlipGunY(), defaultGrayed); break; }
    case MenuItemType::JvsPlayersOnPanel: { const bool grayed = defaultGrayed || !JvsCab::SupportsPlayersOnPanel(); AddList<String>(context, item, { { "1", "1" }, { "2", "2" } }, JvsCab::CurrentPlayersOnPanel(), JvsCab::DefaultPlayersOnPanelValue(), grayed); break; }
    case MenuItemType::JvsLinkMode: { const bool grayed = defaultGrayed || !JvsCab::SupportsLinkMode(); AddList<String>(context, item, { { "1 vs 1", JvsCab::sLinkOneVsOne }, { "2 vs 2", JvsCab::sLinkTwoVsTwo } }, JvsCab::CurrentLinkMode(), JvsCab::DefaultLinkModeValue(), grayed); break; }
    case MenuItemType::CrtReducedLantency: { AddSwitch(context, item, mConf.GetGlobalReduceLatency(), defaultGrayed); break; }
    case MenuItemType::CrtRunAhead: { AddSwitch(context, item, mConf.GetGlobalRunAhead(), defaultGrayed); break; }
    case MenuItemType::CrtScreenCalibration: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::Case: { AddList<String>(context, item, MenuDataProvider::GetCasesEntries(), Case::CurrentCase().ShortName(), String::Empty, false); break; }
    case MenuItemType::UserScript: { assert(context.HasScript() && "No script context"); AddAction(context, item, true, defaultGrayed, true); break; }
    case MenuItemType::InstallDevice: { assert(context.HasPhysicalDevice() && "No physical disk context"); AddAction(context, item, true, defaultGrayed || context.PhysicalDevice()->IsBootDevice, true); break; }
    case MenuItemType::SoundSystemVolumem: { AddTask(new MenuTaskSynchronizeVolumes(*AddSlider(context, item, 0.f, 100.f, 1.f, (float)AudioController::Instance().GetVolume(), 80.f, "%", defaultGrayed), MenuTaskSynchronizeVolumes::VolumeType::System)); break; }
    case MenuItemType::SoundMusicVolume: { AddTask(new MenuTaskSynchronizeVolumes(*AddSlider(context, item, 0.f, 100.f, 1.f, (float)AudioController::Instance().GetMusicVolume(), 80.f, "%", defaultGrayed), MenuTaskSynchronizeVolumes::VolumeType::Music)); break; }
    case MenuItemType::SoundAudioMode: { AddList<AudioMode>(context, item, MenuDataProvider::GetAudioModeEntries(), mConf.GetAudioMode(), AudioMode::MusicsXorVideosSound, defaultGrayed); break; }
    case MenuItemType::SoundAudioOutput: { AddTask(new MenuTaskRefreshAudioOutputs(*AddList<String>(context, item, MenuDataProvider::GetAudioOutputEntries(), AudioController::Instance().GetActivePlaybackName(), String::Empty, defaultGrayed))); break; }
    case MenuItemType::SoundBTPairing: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::FlattenFolders: { AddSwitch(context, item, mConf.GetSystemFlatFolders(*Context().System()), defaultGrayed); break; }
    case MenuItemType::JumpToLetter: //{ AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::SearchInSystem: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::SortGames: { AddList<FileSorts::Sorts>(context, item, MenuDataProvider::GetSortEntries(*Context().System()), mConf.GetSystemSort(*Context().System()), FileSorts::Sorts::FileNameAscending, defaultGrayed); break; }
    case MenuItemType::DecorateGames: { AddMultiList<RecalboxConf::GamelistDecoration>(context, item, mDataProvider.GetDecorationEntries(*Context().System()), defaultGrayed); break; }
    case MenuItemType::GamelistLayout: { AddList<RecalboxConf::GamelistLayout>(context, item, mDataProvider.GetGamelistContainerTypeEntries(*Context().System()), mConf.GetSystemGamelistLayoutNoDefault(*Context().System()), RecalboxConf::GamelistLayout::Default, defaultGrayed); break; }
    case MenuItemType::GamelistLayoutGlobal: { AddList<RecalboxConf::GamelistLayout>(context, item, mDataProvider.GetGlobalGamelistLayoutEntries(), mConf.ResolvedGlobalGamelistLayout(), RecalboxConf::GamelistLayout::Legacy, defaultGrayed); break; }
    case MenuItemType::GamelistLayoutPerSystem:
    {
      // The system comes from the item's own iteration, not from the menu
      if (!context.HasSystem()) break;
      const SystemData& system = *context.System();
      // One entry per system, so several items share this type: duplicates
      // are expected here, as they are for the per system resolutions.
      AddList<RecalboxConf::GamelistLayout>(context, item, mDataProvider.GetGamelistContainerTypeEntries(system), mConf.GetSystemGamelistLayoutNoDefault(system), RecalboxConf::GamelistLayout::Default, defaultGrayed, true);
      break;
    }
    case MenuItemType::HighlightRegion: { AddList<Regions::GameRegions>(context, item, MenuDataProvider::GetRegionEntries(*Context().System()), Regions::Clamp(mConf.GetSystemRegionFilter(*Context().System())), Regions::GameRegions::Unknown, defaultGrayed); break; }
    case MenuItemType::SearchOtherVersions: //{ AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::SearchByLicence: //{ AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::RunSaveStates:
    case MenuItemType::ShowManual: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::SetGameToBootOn: { AddSwitch(context, item, Context().Game()->RomPath().ToString() == mConf.GetBootOnGameGamePath(), defaultGrayed); break; }
    case MenuItemType::SetGameForCard: { AddSwitch(context, item, Context().Game()->RomPath().ToString() == CardReader::Instance().CurrentCardRomPath(), defaultGrayed); break; }
    case MenuItemType::DeleteGame: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::MetadataSetEmulator: { String emulatorAndCore; AddList<String>(context, item, MenuDataProvider::GetEmulatorEntries(*Context().Game(), emulatorAndCore), emulatorAndCore, emulatorAndCore, defaultGrayed); break; }
    case MenuItemType::MetadataSoftpatching: { AddList<Path>(context, item, MenuDataProvider::GetPatchEntries(*Context().Game()), Context().Game()->Metadata().LastPatch(), Path("original"), defaultGrayed); break; }
    case MenuItemType::MetadataRatio: { AddList<String>(context, item, MenuDataProvider::GetRatioEntries(), Context().Game()->Metadata().Ratio(), "auto", defaultGrayed); break; }
    case MenuItemType::MetadataName: { AddEditor(context, item, Context().Game()->Metadata().Name(), false, defaultGrayed); break; }
    case MenuItemType::MetadataRating: { AddRating(context, item, Context().Game()->Metadata().Rating(), defaultGrayed); break; }
    case MenuItemType::MetadataGenre: { AddList<GameGenres>(context, item, MenuDataProvider::GetSingleGenreEntries(), Context().Game()->Metadata().GenreId(), GameGenres::None, defaultGrayed); break; }
    case MenuItemType::MetadataDescription: { AddEditor(context, item, Context().Game()->Metadata().Description(), false, defaultGrayed); break; }
    case MenuItemType::MetadataFavorite: { AddSwitch(context, item, Context().Game()->Metadata().Favorite(), defaultGrayed); break; }
    case MenuItemType::MetadataHidden: { AddSwitch(context, item, Context().Game()->Metadata().Hidden(), defaultGrayed); break; }
    case MenuItemType::MetadataAdult: { AddSwitch(context, item, Context().Game()->Metadata().Adult(), defaultGrayed); break; }
    case MenuItemType::MetadataRotation: { AddSwitch(context, item, Context().Game()->Metadata().Rotation() != RotationType::None, defaultGrayed); break; }
    case MenuItemType::MetadataScrape: { AddAction(context, item, true, GuiScraperRun::IsRunning() || defaultGrayed); break; }
    case MenuItemType::MetastatPlaytime:
    {
      int seconds = Context().Game()->Metadata().TotalPlayTime();
      if (int hours = seconds / 3600; hours > 0) AddText(context, item, _N("%i HOUR", "%i HOURS", hours).Replace("%i", String(hours)));
      else if (int minutes = seconds / 60; minutes > 0) AddText(context, item, _N("%i MINUTE", "%i MINUTES", minutes).Replace("%i", String(minutes)));
      else AddText(context, item, _N("%i SECOND", "%i SECONDS", seconds).Replace("%i", String(seconds)));
      break;
    }
    case MenuItemType::MetastatPlaycount: { int count = Context().Game()->Metadata().PlayCount(); AddText(context, item, _N("%i TIME", "%i TIMES", count).Replace("%i", String(count)), defaultGrayed); break; }
    case MenuItemType::MetastatLastplayed:
    {
      String text;
      TimeSpan diff = DateTime() - Context().Game()->Metadata().LastPlayed();
      if (diff.IsNegative() || diff.TotalDays() > 3560) text = _("never");
      else if (diff.TotalSeconds() <  2) text = _("just now");
      else if (diff.TotalSeconds() < 60) text = _N("%i sec ago", "%i secs ago", diff.TotalSeconds()).Replace("%i", String((int)diff.TotalSeconds()));
      else if (diff.TotalMinutes() < 60) text = _N("%i min ago", "%i mins ago", diff.TotalMinutes()).Replace("%i", String((int)diff.TotalMinutes()));
      else if (diff.TotalHours()   < 24) text = _N("%i hour ago", "%i hours ago", diff.TotalHours()).Replace("%i", String((int)diff.TotalHours()));
      else text =  _N("%i day ago", "%i days ago", diff.TotalDays()).Replace("%i", String((int)diff.TotalDays()));
      AddText(context, item, text, defaultGrayed);
      break;
    }
    case MenuItemType::DeleteGameFiles: { int count = 1 + (int)GameFilesUtils::GetGameSubFiles(*Context().Game()).size(); AddText(context, item, (_F(_N("{0} file", "{0} files", count)) / count).ToString(), defaultGrayed); break; }
    case MenuItemType::DeleteGameMedia: { int count = (int)GameFilesUtils::GetMediaFiles(*Context().Game()).size(); AddText(context, item, (_F(_N("{0} file", "{0} files", count)) / count).ToString(), defaultGrayed); break; }
    case MenuItemType::DeleteFileExtra: { int count = (int)GameFilesUtils::GetGameExtraFiles(*Context().Game()).size(); AddText(context, item, (_F(_N("{0} file", "{0} files", count)) / count).ToString(), defaultGrayed); break; }
    case MenuItemType::DeleteFileSaves: { int count = (int)GameFilesUtils::GetGameSaveFiles(*Context().Game()).size(); AddText(context, item, (_F(_N("{0} file", "{0} files", count)) / count).ToString(), defaultGrayed); break; }
    case MenuItemType::DeleteSelectGameFiles: { AddMultiList<Path>(context, item, MenuDataProvider::GetGameFilesToDelete(*Context().Game()), defaultGrayed, false, true); break; }
    case MenuItemType::DeleteSelectGameMedia: { AddMultiList<Path>(context, item, MenuDataProvider::GetMediaFilesToDelete(*Context().Game()), defaultGrayed, false, true); break; }
    case MenuItemType::DeleteSelectFileExtra: { AddMultiList<Path>(context, item, MenuDataProvider::GetExtraFilesToDelete(*Context().Game()), defaultGrayed, false, true); break; }
    case MenuItemType::DeleteSelectFileSaves: { AddMultiList<Path>(context, item, MenuDataProvider::GetSaveFilesToDelete(*Context().Game()), defaultGrayed, false, true); break; }
    case MenuItemType::DeleteGo: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::DeleteCancel: { AddAction(context, item, false, defaultGrayed); break; }
    case MenuItemType::EnableFavorites: { AddSwitch(context, item, mConf.GetEnableEditFavorites(), defaultGrayed); break; }
    case MenuItemType::SoftpatchingRunOriginal:
    case MenuItemType::SoftpatchingRunPatched: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::SoftpatchingSelectPatch: { AddList<Path>(context, item, MenuDataProvider::GetPatchesEntries(*Context().Game()), Context().Game()->Metadata().LastPatch(), Path::Empty, defaultGrayed); break; }
    case MenuItemType::NetplayPassword:
    {
      for(int i = 0; i < DefaultPasswords::sPasswordCount; i++)
        AddEditor(InheritableContext(context).Merge(InheritableContext(i + 1)), item, ({ String pwd = mConf.GetNetplayPasswords(i); pwd.empty() ? DefaultPasswords::sDefaultPassword[i] : pwd; }), false, defaultGrayed, true);
      break;
    }
    case MenuItemType::NetplayStart: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::NetplaySelectPlayerPassword: { AddList<int>(context, item, mDataProvider.GetNetplayPasswords(), mConf.GetNetplayPasswordLastForPlayer(), -1, defaultGrayed); break; }
    case MenuItemType::NetplaySelectViewerPassword: { AddList<int>(context, item, mDataProvider.GetNetplayPasswords(), mConf.GetNetplayPasswordLastForViewer(), -1, defaultGrayed); break; }
    case MenuItemType::NetplayCancel: { AddAction(context, item, false, defaultGrayed); break; }
    case MenuItemType::NetplayJoinAsPlayer:
    case MenuItemType::NetplayJoinAsViewer: { AddAction(context, item, true, defaultGrayed); break; }
    case MenuItemType::NetplayUsePassword: { AddList<int>(context, item, mDataProvider.GetNetplayPasswords(), mConf.GetNetplayPasswordClient(), -1, defaultGrayed); break; }
    case MenuItemType::DownloadGamePack:
    {
      // From a menu w/ global system context, add only once system
      if (context.HasSystem())
        AddAction(context, item, true, !context.System()->Descriptor().HasDownloader(), false);
      // Otherwise, add every single system with a downloader
      else
        for(SystemData* system : mProvider.SystemManager().AllSystems())
          if (system->Descriptor().HasDownloader())
            AddAction(InheritableContext(context).Merge(InheritableContext(system)), item, true, defaultGrayed, true);
      break;
    }
    case MenuItemType::_Settings_:
    case MenuItemType::_Error_:
    default: { LOG(LogFatal) << "[MenuProvider] Cannot create menu item: " << MenuConverters::ItemToString(item.Type()); break; }
  }
}

/*
 * Following method store & check menu items
 * Most of the menu items need to be singletons and are so checked against duplication
 * However some contextualized items **are** duplicated and recognized only by their context
 * Such items do not need to be checked and are not stored in type => item map so that they cannot be retrieved
 * later using their identifier.
 */

String MenuBuilder::Extend(const InheritableContext& context, const String& rawText)
{
  String result(rawText);
  ItemBase::ReplaceParameters(context, result);
  return result;
}

void MenuBuilder::CheckDuplicate(const ItemDefinition& item, bool acceptDuplicates)
{
  if (!acceptDuplicates)
   if (mItemsTypes.contains(item.Type()))
      { LOG(LogFatal) << "[MenuBuilder] Item of type " << (int)item.Type() << " captioned '" << item.RawCaption() << "' is duplicated in current menu!"; }
}

ItemText* MenuBuilder::AddText(const InheritableContext& itemContext, const ItemDefinition& item, const String& text, bool acceptDuplicates)
{
  CheckDuplicate(item, acceptDuplicates);
  ItemText* object = Menu::AddText(item.Caption(this), text, item.Help());
  object->MergeContext(itemContext).ReplaceParameters();
  mItemsLinks[object] = &item;
  if (!acceptDuplicates) mItemsTypes[item.Type()] = object;
  return object;
}

ItemBar* MenuBuilder::AddBar(const InheritableContext& itemContext, const ItemDefinition& item, const String& text, float ratio, bool grayed, bool acceptDuplicates)
{
  CheckDuplicate(item, acceptDuplicates);
  ItemBar* object = Menu::AddBar(item.Caption(this), text, ratio, item.Help(), grayed, item.UnselectableHelp());
  object->MergeContext(itemContext).ReplaceParameters();
  mItemsLinks[object] = &item;
  if (!acceptDuplicates) mItemsTypes[item.Type()] = object;
  return object;
}

ItemAction* MenuBuilder::AddAction(const InheritableContext& itemContext, const ItemDefinition& item, bool positive, bool grayed, bool acceptDuplicates)
{
  CheckDuplicate(item, acceptDuplicates);
  ItemAction* object = Menu::AddAction(item.Icon(), item.Caption(this), item.Caption2(), (int)item.Type(), positive, &mProvider, item.Help(), grayed, item.UnselectableHelp());
  object->MergeContext(itemContext).ReplaceParameters();
  mItemsLinks[object] = &item;
  if (!acceptDuplicates) mItemsTypes[item.Type()] = object;
  return object;
}

ItemSwitch* MenuBuilder::AddSwitch(const InheritableContext& itemContext, const ItemDefinition& item, bool state, bool grayed, bool acceptDuplicates)
{
  CheckDuplicate(item, acceptDuplicates);
  ItemSwitch* object = Menu::AddSwitch(item.Caption(this), state, (int)item.Type(), &mProvider, item.Help(), grayed, item.UnselectableHelp());
  object->MergeContext(itemContext).ReplaceParameters();
  mItemsLinks[object] = &item;
  if (!acceptDuplicates) mItemsTypes[item.Type()] = object;
  return object;
}

ItemEditable* MenuBuilder::AddEditor(const InheritableContext& itemContext, const ItemDefinition& item, const String& editTitle, const String& text, bool masked, bool grayed, bool acceptDuplicates, VKCharFilter filter)
{
  CheckDuplicate(item, acceptDuplicates);
  ItemEditable* object = AddEditable(editTitle, item.Caption(this), text, (int)item.Type(), &mProvider, item.Help(), masked, grayed, item.UnselectableHelp(), filter);
  object->MergeContext(itemContext).ReplaceParameters();
  mItemsLinks[object] = &item;
  if (!acceptDuplicates) mItemsTypes[item.Type()] = object;
  return object;
}

ItemEditable* MenuBuilder::AddEditor(const InheritableContext& itemContext, const ItemDefinition& item, const String& text, bool masked, bool grayed, bool acceptDuplicates, VKCharFilter filter)
{
  CheckDuplicate(item, acceptDuplicates);
  ItemEditable* object = AddEditable(item.Caption(this), text, (int)item.Type(), &mProvider, item.Help(), masked, grayed, item.UnselectableHelp(), filter);
  object->MergeContext(itemContext).ReplaceParameters();
  mItemsLinks[object] = &item;
  if (!acceptDuplicates) mItemsTypes[item.Type()] = object;
  return object;
}

ItemSlider*
MenuBuilder::AddSlider(const InheritableContext& itemContext, const ItemDefinition& item, float min, float max, float inc, float value, float defaultvalue,
                       const String& suffix, bool grayed, bool acceptDuplicates)
{
  CheckDuplicate(item, acceptDuplicates);
  ItemSlider* object = Menu::AddSlider(item.Caption(this), min, max, inc, value, defaultvalue, suffix, (int)item.Type(), &mProvider, item.Help(), grayed, item.UnselectableHelp());
  object->MergeContext(itemContext).ReplaceParameters();
  mItemsLinks[object] = &item;
  if (!acceptDuplicates) mItemsTypes[item.Type()] = object;
  return object;
}

ItemSubMenu* MenuBuilder::AddSubMenu(const InheritableContext& itemContext, const ItemDefinition& item, bool grayed)
{
  // Menu item are duplicable by default, and they are not stored.
  ItemSubMenu* object = Menu::AddSubMenu(item.Caption(this), (int)item.MenuType(), &mProvider, item.Help(), grayed, item.UnselectableHelp());
  object->MergeContext(itemContext).ReplaceParameters();
  return object;
}

ItemRating* MenuBuilder::AddRating(const InheritableContext& itemContext, const ItemDefinition& item, float rating, bool grayed, bool acceptDuplicates)
{
  CheckDuplicate(item, acceptDuplicates);
  ItemRating* object = Menu::AddRating(item.Caption(this), rating, (int)item.Type(), &mProvider, item.Help(), grayed, item.UnselectableHelp());
  object->MergeContext(itemContext).ReplaceParameters();
  mItemsLinks[object] = &item;
  if (!acceptDuplicates) mItemsTypes[item.Type()] = object;
  return object;
}

template<class T>
ItemSelector<T>* MenuBuilder::AddList(const InheritableContext& itemContext, const ItemDefinition& item, const SelectorEntry<T>::List& values, const T& value, const T& defaultValue, bool grayed, bool acceptDuplicates)
{
  CheckDuplicate(item, acceptDuplicates);
  ItemSelector<T>* object = Menu::AddList<T>(item.Caption(this), (int)item.Type(), &mProvider, values, value, defaultValue, item.Help(), grayed, item.UnselectableHelp());
  if (object != nullptr)
  {
    object->MergeContext(itemContext).ReplaceParameters();
    mItemsLinks[object] = &item;
    if (!acceptDuplicates) mItemsTypes[item.Type()] = object;
  }
  return object;
}

template<class T>
ItemSelector<T>* MenuBuilder::AddMultiList(const InheritableContext& itemContext, const ItemDefinition& item, const SelectorEntry<T>::List& values, bool grayed, bool acceptDuplicates, bool allowEmpty)
{
  CheckDuplicate(item, acceptDuplicates);
  ItemSelector<T>* object = Menu::AddMultiList<T>(item.Caption(this), (int)item.Type(), &mProvider, values, item.Help(), grayed, item.UnselectableHelp(), allowEmpty);
  if (object != nullptr)
  {
    object->MergeContext(itemContext).ReplaceParameters();
    mItemsLinks[object] = &item;
    if (!acceptDuplicates) mItemsTypes[item.Type()] = object;
  }
  return object;
}

