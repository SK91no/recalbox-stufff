//
// Created by bkg2k on 17/10/24.
//
#pragma once

#include "MenuItemType.h"
#include "guis/menus/base/Menu.h"
#include "utils/eval/SimpleExpressionEvaluator.h"
#include "MenuDefinition.h"
#include "ResolutionAdapter.h"
#include "guis/GuiWaitLongExecution.h"
#include "usernotifications/NotificationManager.h"
#include "guis/GuiScraperSingleGameRun.h"
#include "themes/ThemeSpec.h"
#include "MenuEventProcessor.h"
#include "MenuPersistantData.h"
#include "MenuDeserializer.h"
#include <systems/SystemManager.h>
#include <hardware/devices/storage/StorageDevices.h>
#include <scraping/scrapers/ScrapingMethod.h>
#include <guis/menus/MenuBuilder.h>
#include <guis/menus/MenuDataProvider.h>
#include <Upgrade.h>
#include <hardware/Overclocking.h>
#include <hardware/Case.h>
#include <hardware/Board.h>

class MenuProvider : public StaticLifeCycleControler<MenuProvider>
                   // Sync/delayed message
                   , public ISyncMessageReceiver<const ItemSelectorBase<ThemeSpec>*>
                   // Scraping callback
                   , public GuiScraperSingleGameRun<Menu*>::IScrapingComplete
                   // Item callbacks
                   , public IActionTriggered
                   , public ISwitchChanged
                   , public IEditableChanged
                   , public ISubMenuSelected
                   , public ISliderChanged
                   , public IRatingChanged
                   // Selector callbacks
                   , public ISingleSelectorChanged<String>
                   , public ISingleSelectorChanged<StorageDevices::Device>
                   , public ISingleSelectorChanged<ScreenScraperEnums::ScreenScraperImageType>
                   , public ISingleSelectorChanged<ScreenScraperEnums::ScreenScraperBoxType>
                   , public ISingleSelectorChanged<ScreenScraperEnums::ScreenScraperLogoType>
                   , public ISingleSelectorChanged<ScreenScraperEnums::ScreenScraperVideoType>
                   , public ISingleSelectorChanged<ScreenScraperEnums::ScreenScraperRegionPriority>
                   , public ISingleSelectorChanged<Languages>
                   , public ISingleSelectorChanged<Regions::GameRegions>
                   , public ISingleSelectorChanged<Regions::OrderedMasterRegions>
                   , public ISingleSelectorChanged<ScraperNameOptions>
                   , public ISingleSelectorChanged<ScraperType>
                   , public ISingleSelectorChanged<ScrapingMethod>
                   , public ISingleSelectorChanged<RecalboxConf::SoftPatching>
                   , public ISingleSelectorChanged<RecalboxConf::Relay>
                   , public ISingleSelectorChanged<RecalboxConf::VirtualKeyboardType>
                   , public ISingleSelectorChanged<RecalboxConf::UpdateType>
                   , public ISingleSelectorChanged<RecalboxConf::Screensaver>
                   , public ISingleSelectorChanged<ThemeSpec>
                   , public ISingleSelectorChanged<SystemSorting>
                   , public ISingleSelectorChanged<Overclocking::Overclock>
                   , public ISingleSelectorChanged<CrtAdapterType>
                   , public ISingleSelectorChanged<ICrtInterface::HorizontalFrequency>
                   , public ISingleSelectorChanged<CrtScanlines>
                   , public ISingleSelectorChanged<AudioMode>
                   , public ISingleSelectorChanged<String::Unicode>
                   , public ISingleSelectorChanged<FileSorts::Sorts>
                   , public ISingleSelectorChanged<Path>
                   , public ISingleSelectorChanged<GameGenres>
                   , public ISingleSelectorChanged<int>
                   , public ISingleSelectorChanged<RecalboxConf::Transition>
                   , public ISingleSelectorChanged<RecalboxConf::GamelistLayout>
                   , public IMultiSelectorChanged<SystemData*>
                   , public IMultiSelectorChanged<String>
                   , public IMultiSelectorChanged<Path>
                   , public IMultiSelectorChanged<GameGenres>
                   , public IMultiSelectorChanged<RecalboxConf::GamelistDecoration>
{
  public:
    //! Constructor
    MenuProvider(WindowManager& window, SystemManager& systemManager, const Path& menuPath, IGlobalVariableResolver& resolver)
      : StaticLifeCycleControler<MenuProvider>("menuprovider")
      , mDataProvider(mPersistantData, systemManager)
      , mEventProcessor(mPersistantData)
      , mWindow(window)
      , mSystemManager(systemManager)
      , mResolver(resolver)
      , mConf(RecalboxConf::Instance())
      , mCrtConf(CrtConf::Instance())
      , mSender(*this)
      , mMenuFilePath(menuPath)
    {
      mDeserializer.DeserializeMenus(menuPath);
    }

    /*!
     * @brief Build and show the given menu. Log & abort() if the menu does not exist
     * @param identifier Menu name, as defined in the xml file
     */
    static void ShowMenu(MenuContainerType identifier, const InheritableContext& context)
    {
      MenuProvider& This = Instance();
      This.BuildMenu(identifier, context, nullptr);
    }

    const MenuDefinition& GetMenuDefinition(MenuContainerType menuType)
    {
      const MenuDefinition* menu = mDeserializer.MenuDefinitions().try_get(menuType);
      if (menu != nullptr) return *menu;
      { LOG(LogFatal) << "[MenuProvider] Unknown submenu identifier '" << (int)menuType; }
      __builtin_unreachable();
    }

    /*
     * Accessors
     */

    //! Get current system
    [[nodiscard]] ::SystemManager& SystemManager() const { return mSystemManager; }

    /*
     * Menu helpers
     */

    /*!
     * @brief Check if a menu is selectable or must be grayed. All submenu are selectable by defayt
     * @param identifier Submenu identifier
     * @return True to make the menu selectable, false otherwose
     */
    bool IsMenuUnselectable(MenuContainerType identifier);

    /*
     * Menu fillers
     */

    //! Get version string
    static String GetVersionString() { return Upgrade::CurrentVersion(); }
    //! Get version string
    static String GetArchString()
    {
      String model = Board::GetBoardName();
      if (Board::Instance().IsPi())
      {
        int memory = Board::Instance().TotalMemory();
        if (memory < 1024) model.Append(' ').Append(Board::Instance().TotalMemory()).Append(_("MB"));
        else model.Append(' ').Append(Board::Instance().TotalMemory() >> 10).Append(_("GB"));
      }
      model.Append(' ').Append(Board::GetDataBusSize()).Append("bits");
      return model;
    }


    //! Format system name with emulator/core
    String SystemNameWithEmulator(const SystemData* system);

    /*
     * Menu filler helpers
     */

    //! Get visible system list
    const SystemManager::List& VisibleSystems() const { return mSystemManager.VisibleSystemList(); }

    //! Get all systems, visible or not
    const SystemManager::List& AllSystems() const { return mSystemManager.AllSystems(); }

    /*!
     * @brief Load and store user script structure locally, so that they can be accessed by items via context pointers
     * @return script list structures
     */
    const ScriptDescriptorList& LoadUserScripts();

    #if defined(BETA) || defined(DEBUG)
      void Reload() { mDeserializer.DeserializeMenus(mMenuFilePath); }
    #endif

  private:
    //! Pointer to RecalboxConf::SetBool methods
    typedef RecalboxConf& (RecalboxConf::*SetBoolMethod)(const bool&);

    //! Menu persistant data (used between menu instance or between construction & events)
    MenuPersistantData mPersistantData;
    //! Menu data provider
    MenuDataProvider mDataProvider;
    //! Menu event receiver & processor
    MenuEventProcessor mEventProcessor;


    //! Window manager reference
    WindowManager& mWindow;
    //! SystemManager reference
    ::SystemManager& mSystemManager;
    //! Variable resolver reference
    IGlobalVariableResolver& mResolver;
    //! Reclabox configuration reference
    RecalboxConf& mConf;
    //! Crt configuration reference
    CrtConf& mCrtConf;

    //! Menu deserializer
    MenuDeserializer mDeserializer;

    //! Message
    SyncMessageSender<const ItemSelectorBase<ThemeSpec>*> mSender;

    //! Menu path
    Path mMenuFilePath;

    /*
     * Object required by menus
     */
    //! Last selected theme
    ThemeSpec mLastSelectedTheme;
    //! User scripts
    ScriptDescriptorList mUserScripts;

    /*!
     * @brief Build & display a menu from a string identifier
     * @param identifier Menu identifier
     * @param context Context to merge
     * @param itemParent Item that created this menu - may be null
     */
    void BuildMenu(MenuContainerType identifier, const InheritableContext& context, const ItemBase* itemParent);

    /*!
     * @brief Build & display a menu from a menu definition
     * @param definition Menu definition
     * @param context Context to merge
     * @param itemParent Item that created this menu - may be null
     */
    void BuildMenu(const MenuDefinition& definition, const InheritableContext& context, const ItemBase* itemParent);

    /*!
     * @brief Core setting changed
     * @param setting Setting definition
     * @param value New value
     */
    void SettingChanged(const ItemDefinition::SettingDefinition& setting, const String& value);

    /*
     * ISyncReceiveMessage<void> implementation
     */

    /*!
     * @brief Update theme options
     */
    void ReceiveSyncMessage(const ItemSelectorBase<ThemeSpec>* const& sourceItem) final;

    /*
     * Co-Items helpers
     */

    //! Get the parent builder of the current itam
    MenuBuilder& Builder(const ItemBase& item) { return ((MenuBuilder&)item.Parent()); }

    bool CoItemExists(const ItemBase& fromCoItem, MenuItemType itemType)
    {
      return Builder(fromCoItem).HasItem(itemType);
    }

    ItemSwitch& CoItemAsSwitch(const ItemBase& fromCoItem, MenuItemType itemType)
    {
      ItemSwitch* result = Builder(fromCoItem).AsSwitch(itemType);
      if (result != nullptr) return *result;
      LOG(LogFatal) << "[MenuProvider] Requested item " << (int)itemType << " does not exists or not of type Switch";
      __builtin_unreachable();
    }

    ItemSlider& CoItemAsSlider(const ItemBase& fromCoItem, MenuItemType itemType)
    {
      ItemSlider* result = Builder(fromCoItem).AsSlider(itemType);
      if (result != nullptr) return *result;
      LOG(LogFatal) << "[MenuProvider] Requested item " << (int)itemType << " does not exists or not of type Slider";
      __builtin_unreachable();
    }

    ItemText& CoItemAsText(const ItemBase& fromCoItem, MenuItemType itemType)
    {
      ItemText* result = Builder(fromCoItem).AsInfoText(itemType);
      if (result != nullptr) return *result;
      LOG(LogFatal) << "[MenuProvider] Requested item " << (int)itemType << " does not exists or not of type Text";
      __builtin_unreachable();
    }

    ItemEditable& CoItemAsEditor(const ItemBase& fromCoItem, MenuItemType itemType)
    {
      ItemEditable* result = Builder(fromCoItem).AsEditor(itemType);
      if (result != nullptr) return *result;
      LOG(LogFatal) << "[MenuProvider] Requested item " << (int)itemType << " does not exists or not of type Editable";
      __builtin_unreachable();
    }

    template<typename T> ItemSelector<T>& CoItemAsMulti(const ItemBase& fromCoItem, MenuItemType itemType)
    {
      ItemSelector<T>* result = Builder(fromCoItem).AsMulti<T>(itemType);
      if (result != nullptr) return *result;
      LOG(LogFatal) << "[MenuProvider] Requested item " << (int)itemType << " does not exists or not of type Multi";
      __builtin_unreachable();
    }

    template<typename T> ItemSelector<T>& CoItemAsList(const ItemBase& fromCoItem, MenuItemType itemType)
    {
      ItemSelector<T>* result = Builder(fromCoItem).AsList<T>(itemType);
      if (result != nullptr) return *result;
      { LOG(LogFatal) << "[MenuProvider] Requested item " << (int)itemType << " does not exists or not of type List"; }
      __builtin_unreachable();
    }

    /*
     * Tool method
     */

    /*!
     * @brief Check high level filter after the given boolean is modifier, and revert to the original value
     * if the new filter hide all games
     * @param value New value passed by adress so that it can be reverted
     * @param setter Boolean setter in RecalboxConf that set the boolean filter
     * @param message Optional message (passing nullptr display a generic message in cas eof error)
     * @param inverted True to process the value as a inverted value when calling the setter
     */
    void CheckHighLevelFilter(bool& value, SetBoolMethod setter, const char* message, bool inverted);

    /*!
     * @brief Rebuild theme option items
     * @param item Menu Item to change content
     * @param selected Selected values
     * @param items Values
     */
    void ChangeThemeOptionSelector(ItemSelector<String>& item, const String& selected, const String::List& items);

    /*!
     * @brief Try to sort item list by their front numeric value when all items have numeric values
     * If at least one item has no numeric value, the list is sorted alphanumerically
     * @param list String list to sort
     * @return Sorted list
     */
    bool TrySortNumerically(SelectorEntry<String>::List& list);

    /*!
     * @brief Do jamma configuration reset
     * @param menu Menu object
     */
    void DoJammaReset(MenuBuilder& menu);

    /*!
     * @brief Run & select menu calibration
     */
    void RunScreenCalibration();

    /*
     * GuiScraperSingleGameRun<Menu*>::IScrapingComplete implementation
     */

    virtual void ScrapingComplete(FileData& game, MetadataType changedMetadata, Menu* menu) final;

    /*
     * Sub menus
     */

    void SubMenuSelected(const ItemSubMenu& item, int id) final;

    /*
     * IRatingChanged implementation
     */

    void MenuRatingMoved(ItemRating& item, int id, float value) final;

    /*
     * IActionTriggered implementation
     */

    void MenuActionTriggered(ItemAction& item, int id) final;

    /*
     *  ISwitchChanged implementation
     */

    void MenuSwitchChanged(const ItemSwitch& item, bool& value, int id) final;

    /*
     * IOptionListComponent<String> implementation
     */

    void MenuSingleChanged(ItemSelectorBase<String>& item, int id, int index, const String& value) final;

    /*
     * IOptionListComponent<StorageDevices::Device> implementation
     */

    void MenuSingleChanged(ItemSelectorBase<StorageDevices::Device>& item, int id, int index, const StorageDevices::Device& value) final;

    /*
     * ISingleSelectorChanged<ScreenScraperEnums::ScreenScraperImageType> implementations
     */

    void MenuSingleChanged(ItemSelectorBase<ScreenScraperEnums::ScreenScraperImageType>& item, int id, int index, const ScreenScraperEnums::ScreenScraperImageType& value) final;

    /*
     * ISingleSelectorChanged<ScreenScraperEnums::ScreenScraperBoxType> implementations
     */

    void MenuSingleChanged(ItemSelectorBase<ScreenScraperEnums::ScreenScraperBoxType>& item, int id, int index, const ScreenScraperEnums::ScreenScraperBoxType& value) final;

    /*
     * ISingleSelectorChanged<ScreenScraperEnums::ScreenScraperVideoType> implementations
     */

    void MenuSingleChanged(ItemSelectorBase<ScreenScraperEnums::ScreenScraperVideoType>& item, int id, int index, const ScreenScraperEnums::ScreenScraperVideoType& value) final;

    /*
     * ISingleSelectorChanged<ScreenScraperEnums::ScreenScraperRegionPriority> implementations
     */

    void MenuSingleChanged(ItemSelectorBase<ScreenScraperEnums::ScreenScraperRegionPriority>& item, int id, int index, const ScreenScraperEnums::ScreenScraperRegionPriority& value) final;

    /*
     * ISingleSelectorChanged<Regions::GameRegions> implementations
     */

    void MenuSingleChanged(ItemSelectorBase<Regions::GameRegions>& item, int id, int index, const Regions::GameRegions& value) final;

    /*
     * ISingleSelectorChanged<Regions::OrderedMasterRegions> implementations
     */

    void MenuSingleChanged(ItemSelectorBase<Regions::OrderedMasterRegions>& item, int id, int index, const Regions::OrderedMasterRegions& value) final;

    /*
     * ISingleSelectorChanged<Languages> implementations
     */

    void MenuSingleChanged(ItemSelectorBase<Languages>& item, int id, int index, const Languages& value) final;

    /*
     * ISingleSelectorChanged<ScraperNameOptions> implementation
     */

    void MenuSingleChanged(ItemSelectorBase<ScraperNameOptions>& item, int id, int index, const ScraperNameOptions& value) final;

    /*
     * ISingleSelectorChanged<ScraperFactory::ScraperType> implementation
     */

    void MenuSingleChanged(ItemSelectorBase<ScraperType>& item, int id, int index, const ScraperType& value) final;

    /*
     * ISingleSelectorChanged<ScraperType> implementation
     */

    void MenuSingleChanged(ItemSelectorBase<ScrapingMethod>& item, int id, int index, const ScrapingMethod& value) final;

    /*
     * ISingleSelectorChanged<RecalboxConf::SoftPatching> implementation
     */

    void MenuSingleChanged(ItemSelectorBase<RecalboxConf::SoftPatching>& item, int id, int index, const RecalboxConf::SoftPatching& value) final;

    /*
     * ISingleSelectorChanged<RecalboxConf::Relay> implementation
     */

    void MenuSingleChanged(ItemSelectorBase<RecalboxConf::Relay>& item, int id, int index, const RecalboxConf::Relay& value) final;

    /*
     * ISingleSelectorChanged<RecalboxConf::VirtualKeyboardType> implementation
     */

    void MenuSingleChanged(ItemSelectorBase<RecalboxConf::VirtualKeyboardType>& item, int id, int index, const RecalboxConf::VirtualKeyboardType& value) final;

    /*
     * IOptionListComponent<String> implementation
     */

    void MenuSingleChanged(ItemSelectorBase<RecalboxConf::Screensaver>& item, int id, int index, const RecalboxConf::Screensaver& value) final;

    /*
     * IOptionListComponent<RecalboxConf::UpdateType> implementation
     */

    void MenuSingleChanged(ItemSelectorBase<RecalboxConf::UpdateType>& item, int id, int index, const RecalboxConf::UpdateType& value) final;

    /*
     * IOptionListComponent<ThemeSpec> implementation
     */

    void MenuSingleChanged(ItemSelectorBase<ThemeSpec>& item, int id, int index, const ThemeSpec& value) final;

    /*
     * ISingleSelectorChanged<SystemSorting> implementation
     */

    void MenuSingleChanged(ItemSelectorBase<SystemSorting>& item, int id, int index, const SystemSorting& value) final;

    /*
     * ISingleSelectorChanged<RecalboxConf::GamelistLayout> implementation
     */

    void MenuSingleChanged(ItemSelectorBase<RecalboxConf::GamelistLayout>& item, int id, int index, const RecalboxConf::GamelistLayout& value) final;

    /*
     * IMultiSelectorChanged<SystemData*> implementation
     */

    void MenuMultiChanged(const ItemSelectorBase<SystemData*>& item, int id, int index, const std::vector<SystemData*>& value) final;

    /*
     * IOptionListMultiComponent<String> implementation
     */

    void MenuMultiChanged(const ItemSelectorBase<String>& item, int id, int index, const std::vector<String>& value) final;

    /*
     * IOptionListMultiComponent<Path> implementation
     */
    void MenuMultiChanged(const ItemSelectorBase<Path>& item, int id, int index, const std::vector<Path>& value) final;

    /*
     * IOptionListMultiComponent<GameGenres> implementation
     */

    void MenuMultiChanged(const ItemSelectorBase<GameGenres>& item, int id, int index, const std::vector<GameGenres>& value) final;

    /*
     * IOptionListMultiComponent<RecalboxConf::GamelistDecoration> implementation
     */

    void MenuMultiChanged(const ItemSelectorBase<RecalboxConf::GamelistDecoration>& item, int id, int index, const std::vector<RecalboxConf::GamelistDecoration>& value) final;

    /*
     * IEditableChanged
     */

    void MenuEditableChanged(ItemEditable& item, int id, const String& newText) final;

    /*
     * Slider
     */
    void MenuSliderMoved(ItemSlider& item, int id, float value) final;

    /*
     * ISingleSelectorChanged<Overclocking::Overclock>
     */
    void MenuSingleChanged(ItemSelectorBase<Overclocking::Overclock>& item, int id, int index, const Overclocking::Overclock& value) final;

    /*
     * ISingleSelectorChanged<CrtAdapterType>
     */
    void MenuSingleChanged(ItemSelectorBase<CrtAdapterType>& item, int id, int index, const CrtAdapterType& value) final;

    /*
     * ISingleSelectorChanged<ICrtInterface::HorizontalFrequency>
     */
    void MenuSingleChanged(ItemSelectorBase<ICrtInterface::HorizontalFrequency>& item, int id, int index, const ICrtInterface::HorizontalFrequency &value) final;

    /*
     * ISingleSelectorChanged<CrtScanlines>
     */
    void MenuSingleChanged(ItemSelectorBase<CrtScanlines>& item, int id, int index, const CrtScanlines& value) final;

    /*
     * ISingleSelectorChanged<AudioMode>
     */

    void MenuSingleChanged(ItemSelectorBase<AudioMode>& item, int id, int index, const AudioMode& value) final;

    /*
     * ISingleSelectorChanged<String::Unicode>
     */
    void MenuSingleChanged(ItemSelectorBase<String::Unicode>& item, int id, int index, const String::Unicode& value) final;

    /*
     * ISingleSelectorChanged<FileSorts::Sorts>
     */
    void MenuSingleChanged(ItemSelectorBase<FileSorts::Sorts>& item, int id, int index, const FileSorts::Sorts& value) final;

    /*
     * ISingleSelectorChanged<Path>
     */
    void MenuSingleChanged(ItemSelectorBase<Path>& item, int id, int index, const Path& value) final;

    /*
     * ISingleSelectorChanged<GameGenres>
     */
    void MenuSingleChanged(ItemSelectorBase<GameGenres>& item, int id, int index, const GameGenres& value) final;

    /*
     * ISingleSelectorChanged<int>
     */
    void MenuSingleChanged(ItemSelectorBase<int>& item, int id, int index, const int& value) final;

    /*
     * ISingleSelectorChanged<RecalboxConf::Transition>
     */

    void MenuSingleChanged(ItemSelectorBase<RecalboxConf::Transition>& item, int id, int index, const RecalboxConf::Transition& value);

    /*
     * ISingleSelectorChanged<ScreenScraperEnums::ScreenScraperLogoType>
     */

    void MenuSingleChanged(ItemSelectorBase<ScreenScraperEnums::ScreenScraperLogoType>& item, int id, int index, const ScreenScraperEnums::ScreenScraperLogoType& value);
};
