//
// Created by bkg2k on 21/10/24.
//
#pragma once

#include <SDL_timer.h>
#include <guis/menus/MenuDefinition.h>
#include "guis/menus/base/Menu.h"
#include "utils/eval/SimpleExpressionEvaluator.h"
#include "guis/menus/tasks/IMenuBackgroundTask.h"
#include "utils/locale/LocaleHelper.h"
#include "CrtConf.h"

// Forward declaration
class WindowManager;
class MenuProvider;
class MenuDataProvider;
class MenuEventProcessor;

class MenuBuilder : public Menu
                  , public SimpleExpressionEvaluator::IIdentifierEvaluator
{
  public:
    /*!
     * @brief Constructor
     * @param definition Menu definition
     * @param eventProcessor Menu event processor
     */
    MenuBuilder(WindowManager& window, const InheritableContext& context,
                MenuProvider& provider, MenuDataProvider& dataProvider, MenuEventProcessor& eventProcessor,
                const MenuDefinition& definition, IGlobalVariableResolver& resolver, const ItemBase* parentItem)
      : Menu(window, context, Extend(context, parentItem != nullptr ? parentItem->Label() : _S(definition.mCaption)),
             context.HasGame() ? context.Game()->Metadata().RomFileOnly().ToString() : String::Empty, definition.mAnimated)
      , mCrtConf(CrtConf::Instance())
      , mProvider(provider)
      , mDataProvider(dataProvider)
      , mEventProcesor(eventProcessor)
      , mResolver(resolver)
      , mDefinition(definition)
      , mBeta(false)
    {
      #if defined(BETA) || defined(DEBUG)
        mBeta = true;
      #endif
    }

    //! Destructor
    ~MenuBuilder();

    /*!
     * @brief Inherited classes must add items in here
     */
    void BuildMenuItems() final;

    /*!
     * @brief Build menu from the given menu definition w/o clearing current menu
     * allowing to include other menus
     */
    void BuildMenuItemsFrom(const MenuDefinition& menu);

    /*!
     * @brief Request menu refresh to reflect item changes
     */
    void RequestMenuRefresh() { RequestRebuildMenu(-1); }

    /*!
     * @brief Request a menu refresh after the given delay (ms). Use when an item
     * change triggers an async side-effect (e.g. JVS HID re-enumeration) and an
     * immediate refresh would read transient/stale sysfs values.
     */
    void RequestMenuRefreshIn(int delayMs) { mPendingRefreshAt = (int)SDL_GetTicks() + delayMs; }

    /*!
     * @brief Per-frame tick. Fires the delayed refresh when its target time has passed.
     * Uses an absolute timestamp (not deltaTime) because deltaTime can be huge when a
     * sub-GUI (selector dropdown) was on top and we just regained focus.
     */
    void Update(int deltaTime) override
    {
      if (mPendingRefreshAt >= 0 && (int)SDL_GetTicks() >= mPendingRefreshAt)
      {
        mPendingRefreshAt = -1;
        RequestMenuRefresh();
      }
      Menu::Update(deltaTime);
    }

    /*
     * Getter
     */

    bool HasItem(MenuItemType type)                                  const { return mItemsTypes.contains(type); }

    ItemHeader* AsHeader(MenuItemType type)                          const { if (ItemBase** base = mItemsTypes.try_get(type); base != nullptr) return (*base)->AsHeader(); return nullptr; }
    ItemSubMenu* AsSubMenu(MenuItemType type)                        const { if (ItemBase** base = mItemsTypes.try_get(type); base != nullptr) return (*base)->AsSubMenu(); return nullptr; }
    ItemText* AsInfoText(MenuItemType type)                          const { if (ItemBase** base = mItemsTypes.try_get(type); base != nullptr) return (*base)->AsText(); return nullptr; }
    ItemSwitch* AsSwitch(MenuItemType type)                          const { if (ItemBase** base = mItemsTypes.try_get(type); base != nullptr) return (*base)->AsSwitch(); return nullptr; }
    ItemSlider* AsSlider(MenuItemType type)                          const { if (ItemBase** base = mItemsTypes.try_get(type); base != nullptr) return (*base)->AsSlider(); return nullptr; }
    template<typename T> ItemSelector<T>* AsList(MenuItemType type)  const { if (ItemBase** base = mItemsTypes.try_get(type); base != nullptr) return (*base)->AsList<T>(); return nullptr; }
    template<typename T> ItemSelector<T>* AsMulti(MenuItemType type) const { if (ItemBase** base = mItemsTypes.try_get(type); base != nullptr) return (*base)->AsMulti<T>(); return nullptr; }
    ItemEditable* AsEditor(MenuItemType type)                        const { if (ItemBase** base = mItemsTypes.try_get(type); base != nullptr) return (*base)->AsEditor(); return nullptr; }
    ItemRating* AsRating(MenuItemType type)                          const { if (ItemBase** base = mItemsTypes.try_get(type); base != nullptr) return (*base)->AsRating(); return nullptr; }
    ItemAction* AsAction(MenuItemType type)                          const { if (ItemBase** base = mItemsTypes.try_get(type); base != nullptr) return (*base)->AsAction(); return nullptr; }
    ItemBar* AsBar(MenuItemType type)                                const { if (ItemBase** base = mItemsTypes.try_get(type); base != nullptr) return (*base)->AsBar(); return nullptr; }

    // Allow public call
    using Menu::RequestReboot;
    using Menu::RequestRelaunch;

  private:
    //! SDL_GetTicks() target time for delayed RequestMenuRefresh; <0 = disabled.
    int mPendingRefreshAt = -1;

    //! Crt Configuration
    CrtConf& mCrtConf;
    //! Provider
    MenuProvider& mProvider;
    //! Item data Provider
    MenuDataProvider& mDataProvider;
    //! Item event processor
    MenuEventProcessor& mEventProcesor;
    //! Global resolver
    IGlobalVariableResolver& mResolver;
    //!< Definition
    const MenuDefinition& mDefinition;

    //! Background tasks
    Array<IMenuBackgroundTask*> mTasks;

    //! Menu map: menu item to item definition
    HashMap<ItemBase*, const ItemDefinition*> mItemsLinks;
    //! Menu map: menu type to menu item
    HashMap<MenuItemType, ItemBase*> mItemsTypes;

    //! Beta/debug status
    bool mBeta;

    //! Check if an item is duplicated in the current menu. If yes, log & abort
    void CheckDuplicate(const ItemDefinition& item, bool acceptDuplicates);

    //! Add background task
    void AddTask(IMenuBackgroundTask* task) { mTasks.Add(task); }

    /*!
     * @brief Replace special tags in the given string
     * @param context Context from which to get information
     * @param rawText Input text
     * @return Output text with special tags replaced
     */
    static String Extend(const InheritableContext& context, const String& rawText);

    //! Stop & clear tasks
    void ClearTasks();

    /*
     * Component
     */

    void onShow() final
    {
      // NOP NOP - Cause  delayed message from ItemSelectors to crash because target no longer exists
      //RequestRebuildMenu(-1);
    }

    /*
     * Reboot/Relaunch management
     */

    /*!
     * @brief Called when the user delays the reboot action
     * @param window Window manager instance
     */
    static void RebootPending(WindowManager* window);

    /*
     * Item creation
     */

    //! Add given item according to its type
    void AddItem(const InheritableContext& itemContext, const ItemDefinition& item);

    //! Add given setting according to its type
    void AddSetting(const InheritableContext& itemContext, const ItemDefinition& item);

    /*!
     * @brief Add custom submenu(s)
     * @param subMenuItem SubMenu item
     * @return True if one or more submenu item have been added, false otherwise
     */
    bool AddCustomSubMenu(const InheritableContext& itemContext, const ItemDefinition& subMenuItem);

    /*!
     * @brief Called on every item when the menu is shown
     * @param item Item base
     */
    void MenuItemShow(ItemBase& item);

    //! Add submenu
    ItemSubMenu* AddSubMenu(const InheritableContext& itemContext, const ItemDefinition& item, bool grayed);
    //! Text item helper
    ItemText* AddText(const InheritableContext& itemContext, const ItemDefinition& item, const String& text, bool acceptDuplicates = false);
    //! Bar item helper
    ItemBar* AddBar(const InheritableContext& itemContext, const ItemDefinition& item, const String& text, float ratio, bool grayed, bool acceptDuplicates= false);
    //! Switch item helper
    ItemSwitch* AddSwitch(const InheritableContext& itemContext, const ItemDefinition& item, bool state, bool grayed, bool acceptDuplicates = false);
    //! Rating item helper
    ItemRating* AddRating(const InheritableContext& itemContext, const ItemDefinition& item, float rating, bool grayed, bool acceptDuplicates = false);
    //! Action item helper
    ItemAction* AddAction(const InheritableContext& itemContext, const ItemDefinition& item, bool positive, bool grayed, bool acceptDuplicates = false);
    //! Editor item helper
    ItemEditable* AddEditor(const InheritableContext& itemContext, const ItemDefinition& item, const String& editTitle, const String& text, bool masked, bool grayed, bool acceptDuplicates = false, VKCharFilter filter = VKCharFilter::All);
    ItemEditable* AddEditor(const InheritableContext& itemContext, const ItemDefinition& item, const String& text, bool masked, bool grayed, bool acceptDuplicates = false, VKCharFilter filter = VKCharFilter::All);
    //! Slider helper
    ItemSlider* AddSlider(const InheritableContext& itemContext, const ItemDefinition& item, float min, float max, float inc, float defaultvalue, float value, const String& suffix, bool grayed, bool acceptDuplicates = false);
    //! List item helper
    template<class T> ItemSelector<T>* AddList(const InheritableContext& itemContext, const ItemDefinition& item, const SelectorEntry<T>::List& values, const T& value, const T& defaultValue, bool grayed, bool acceptDuplicates = false);
    //! Multi List item helper
    template<class T> ItemSelector<T>* AddMultiList(const InheritableContext& itemContext, const ItemDefinition& item, const SelectorEntry<T>::List& values, bool grayed, bool acceptDuplicates = false, bool allowEmpty = false);

    /*
     * Special item creation / Refresh
     */

    /*!
     * @brief Build an Item selector for a particular theme option set
     * @param context Item context
     * @param item Menu item definition
     * @param option ThemeOption structure
     */
    void BuildThemeOptionSelector(const InheritableContext& context, const ItemDefinition& item, ThemeOption* option);

    /*!
     * @brief Try to sort item list by their front numeric value when all items have numeric values
     * If at least one item has no numeric value, the list is sorted alphanumerically
     * @param list String list to sort
     * @return Sorted list
     */
    static bool TrySortNumerically(String::List& list);

    /*
     * SimpleExpressionEvaluator::IIdentifierEvaluator
     */

    /*!
     * @brief Must return a boolean evaluation of the given identifier
     * @param identifier String identifier to evaluate
     * @return True or false depending of the identifier evaluation
     */
    bool EvaluateIdentifier(const String& identifier) final;
};


