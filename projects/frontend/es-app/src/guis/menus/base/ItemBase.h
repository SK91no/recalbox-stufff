//
// Created by bkg2k on 20/08/24.
//
#pragma once

#include <utils/String.h>
#include <themes/MenuThemeData.h>
#include "IOverlay.h"
#include "InheritableContext.h"
#include <../data/Resources.h>
#include <guis/menus/base/IMenuInterface.h>

// Forward declaration
class Menu;
class ItemHeader;
class ItemSubMenu;
class ItemSwitch;
class ItemSlider;
class ItemText;
class ItemEditable;
class ItemRating;
class ItemAction;
class ItemBar;
template<typename T> class ItemSelector;

//! Menu item structure
class ItemBase : public IOverlay
{
  public:
    enum class Type : char
    {
      Header,   //!< Header
      SubMenu,  //!< Sub menu
      Text,     //!< Non interactive text
      Switch,   //!< On/Off switch
      Slider,   //!< Slider
      List,     //!< Single value among many
      Multi,    //!< Multiple values among many
      Editor,   //!< Text editor
      Rating,   //!< Rating stars
      Action,   //!< Action item
      Bar,      //!< Progress bar
    };

    //! Allow inherited component to flush pending values
    virtual void Flush() {}

    //! Get item type
    [[nodiscard]] Type ItemType() const { return mType; }
    //! Get item identifier
    [[nodiscard]] int Identifier() const { return mIdentifier; }
    //! Check if this item has a valid icon
    [[nodiscard]] bool HasValidIcon() const { return mHasIcon; }
    //! Icon
    [[nodiscard]] MenuThemeData::MenuIcons::Type Icon() const { return mIcon; }
    //! Direct Icon path
    [[nodiscard]] const Path& IconPath() const { return mIconDirectPath; }
    //! Label
    [[nodiscard]] const String& Label() const { return mLabel; }
    //! Help
    [[nodiscard]] const String& HelpText() const { return mHelp; }
    //! Help unselectable
    [[nodiscard]] const String& HelpUnselectableText() const { return mHelpUnselectable; }
    //! Help available ?
    [[nodiscard]] bool HasHelpText() const { return !mHelp.empty(); }
    //! Help "unselectable" available ?
    [[nodiscard]] bool HasHelpUnselectableText() const { return !mHelpUnselectable.empty(); }
    //! Parent menu
    [[nodiscard]] Menu& Parent() const { return mParent; }

    /*
     * Generic setter
     */

    /*!
     * @brief Change item label (left text)
     * @param newLabel
     */
    void ChangeLabel(const String& newLabel)
    {
      mLabel = newLabel;
      LabelChanged();
    }

    /*!
     * @brief Set item selectable or unselectable
     * @param selectable True to make the item selectable, false to make it grayed & unselectable
     */
    void SetSelectable(bool selectable) { mIsUnselectable = !selectable; mDataProvider.ItemSelectabilityChanged(*this); }

    /*!
     * @brief Replace {SYSTEM.NAME}, {GAME.NAME} & others, by the values from the given context
     * @param from source Context
     * @param to Destination string within which to replace parameters
     */
    static void ReplaceParameters(InheritableContext from, String& to);

    /*!
     * @brief Replace {SYSTEM.NAME} and {GAME.NAME} by the values from the current context
     * @return This
     */
    ItemBase& ReplaceParameters();

    /*!
     * @brief Replace one parameter "{0}" in label/help/unselectableHelp strings
     * @param parameter1 Parameter to substitute to substring "{0}"
     * @return This
     */
    ItemBase& ReplaceParameters(const String& parameter1, bool uppercase);
    /*!
     * @brief Replace two parameters "{0}" and "{1}" in label/help/unselectableHelp strings
     * @param parameter1 Parameter to substitute to substring "{0}"
     * @param parameter2 Parameter to substitute to substring "{1}"
     * @return This
     */
    ItemBase& ReplaceParameters(const String& parameter1, const String& parameter2, bool uppercase);
    /*!
     * @brief Replace two parameters "{0}", "{1}" and "{2}" in label/help/unselectableHelp strings
     * @param parameter1 Parameter to substitute to substring "{0}"
     * @param parameter2 Parameter to substitute to substring "{1}"
     * @param parameter3 Parameter to substitute to substring "{2}"
     * @return This
     */
    ItemBase& ReplaceParameters(const String& parameter1, const String& parameter2, const String& parameter3, bool uppercase);

    /*
     * Filtering
     */

    //! Check if this item is selectable
    [[nodiscard]] bool IsSelectable() const { return !mIsUnselectable; }

    //! Check of the item is displayable - TODO: implement filtering here
    [[nodiscard]] bool IsDisplayable() const { return !mLabel.empty(); }

    /*
     * Identification
     */

    bool IsHeader()  const { return mType == Type::Header;  }
    bool IsSubMenu() const { return mType == Type::SubMenu; }
    bool IsText()    const { return mType == Type::Text;    }
    bool IsSwitch()  const { return mType == Type::Switch;  }
    bool IsSlider()  const { return mType == Type::Slider;  }
    bool IsList()    const { return mType == Type::List;    }
    bool IsMulti()   const { return mType == Type::Multi;   }
    bool IsEditor()  const { return mType == Type::Editor;  }
    bool IsRating()  const { return mType == Type::Rating;  }
    bool IsAction()  const { return mType == Type::Action;  }
    bool IsBar()     const { return mType == Type::Bar;     }

    /*
     * Get typed children
     */

    ItemHeader* AsHeader()                          const { return mType == Type::Header ? (ItemHeader*)(this) : nullptr; }
    ItemSubMenu* AsSubMenu()                        const { return mType == Type::SubMenu ? (ItemSubMenu*)(this) : nullptr; }
    ItemText* AsText()                              const { return mType == Type::Text ? (ItemText*)(this) : nullptr; }
    ItemSwitch* AsSwitch()                          const { return mType == Type::Switch ? (ItemSwitch*)(this) : nullptr; }
    ItemSlider* AsSlider()                          const { return mType == Type::Slider ? (ItemSlider*)(this) : nullptr; }
    template<typename T> ItemSelector<T>* AsList()  const { return mType == Type::List ? (ItemSelector<T>*)(this) : nullptr; }
    template<typename T> ItemSelector<T>* AsMulti() const { return mType == Type::Multi ? (ItemSelector<T>*)(this) : nullptr; }
    ItemEditable* AsEditor()                        const { return mType == Type::Editor ? (ItemEditable*)(this) : nullptr; }
    ItemRating* AsRating()                          const { return mType == Type::Rating ? (ItemRating*)(this) : nullptr; }
    ItemAction* AsAction()                          const { return mType == Type::Action ? (ItemAction*)(this) : nullptr; }
    ItemBar* AsBar()                                const { return mType == Type::Bar ? (ItemBar*)(this) : nullptr; }

    /*!
     * @brief Notification of an input event
     * @param event Compact event
     * @return Implementation must return true if it consumed the event.
     */
    virtual bool ProcessInput(const InputCompactEvent& event) = 0;

    /*!
     * @brief Called once per frame
     * @param elapsed elaspsed time from last update call
     */
    virtual void Update(int elapsed) = 0;

    /*!
     * @brief Fill help list
     * @param help Help list to fill
     */
    virtual void CollectHelpItems(Help& help) = 0;

    /*!
     * @brief Instruct the caller whether the underlying value has changed since the iten has been created
     * if the item has nbo associated value, just retyurn false
     * @return True if the underlying value of the iten has changed since its creation
     */
    virtual bool HasChanged() = 0;

    /*
     * Focus processing
     */

    /*!
     * @brief Called when the item gain focus
     */
    virtual void GainFocus() {}

    /*!
     * @brief Called when the item Lose focus
     */
    virtual void LoseFocus() {}

    /*
     * Context
     */

    //! Merge the given context with the inherited one
    ItemBase& MergeContext(const InheritableContext& context) { mContext.Merge(context); return *this; }

    //! Get context
    const InheritableContext& Context() const { return mContext; }

  protected:
    //! Fire callback
    virtual void Fire() = 0;

    //! Replace parameters in the given string
    ItemBase& ReplaceParameters(String& to);

    //! Instruct inherited items the label has changed
    virtual void LabelChanged() = 0;

    //! Instruct inherited items the theme has changed, allowing them to redo appropriate calculations
    virtual void ThemeHasChanged() = 0;

    //! Minimal constructor
    ItemBase(Type type, Menu& parent, IMenuInterface& dataProvider, const String& label, const MenuThemeData& theme, int identifier, const String& help, bool unselectable, const String& helpUnselectable)
      : mParent(parent)
      , mLabel(label)
      , mHelp(help)
      , mHelpUnselectable(helpUnselectable)
      , mTheme(theme)
      , mDataProvider(dataProvider)
      , mType(type)
      , mIdentifier(identifier)
      , mIcon(MenuThemeData::MenuIcons::Type(0))
      , mHasIcon(false)
      , mIsUnselectable(unselectable)
    {}

    //! With icon
    ItemBase(Type type, Menu& parent, IMenuInterface& dataProvider, const String& label, const MenuThemeData& theme, int identifier, MenuThemeData::MenuIcons::Type icon, const String& help, bool unselectable, const String& helpUnselectable)
      : mParent(parent)
      , mLabel(label)
      , mHelp(help)
      , mHelpUnselectable(helpUnselectable)
      , mTheme(theme)
      , mDataProvider(dataProvider)
      , mType(type)
      , mIdentifier(identifier)
      , mIcon(icon)
      , mHasIcon(icon != MenuThemeData::MenuIcons::Type::Unknown && (theme.Icons().FromType(icon).Exists() || res2hMap.contains(theme.Icons().FromType(icon).ToString())))
      , mIsUnselectable(unselectable)
    {}

    //! With icon
    ItemBase(Type type, Menu& parent, IMenuInterface& dataProvider, const String& label, const MenuThemeData& theme, int identifier, Path icon, const String& help, bool unselectable, const String& helpUnselectable)
      : mParent(parent)
      , mLabel(label)
      , mHelp(help)
      , mHelpUnselectable(helpUnselectable)
      , mIconDirectPath(icon)
      , mTheme(theme)
      , mDataProvider(dataProvider)
      , mType(type)
      , mIdentifier(identifier)
      , mIcon(MenuThemeData::MenuIcons::Type::Kodi)
      , mHasIcon(icon.Exists() || res2hMap.contains(icon.ToString()))
      , mIsUnselectable(unselectable)
    {}

    InheritableContext mContext;                 //!< Context data
    Menu& mParent;                        //!< Parent menu
    String mLabel;                        //!< Menu item label
    String mHelp;                         //!< Optional help string
    String mHelpUnselectable;             //!< Optional help when the item is unselectable
    Path mIconDirectPath;                 //!< Direct path to icon (ignore type if path not empty)
    const MenuThemeData& mTheme;          //!< Menu theme
    IMenuInterface& mDataProvider;     //!< Menu data provider (provide general menu value to items)
    Type mType;                           //!< Menu item type
    int  mIdentifier;                     //!< Identifier from the template enum
    MenuThemeData::MenuIcons::Type mIcon; //!< Optional icon type
    bool mHasIcon;                        //!< Has a valid icon?
    bool mIsUnselectable;                 //!< True if the item is shown but not selectable

    /*!
     * @brief Analyse and modifie, or not, the dynamic right part size of an item
     * @param leftWidth Left part width (usually label, passed by the TextListComponent object)
     * @param rightDynamicWidth Right dynamic width, usually texts or sizeable graphic items
     * @param rightStaticWidth Right static width: margin and non sizeable graphic items
     * @return True if the rightDynamicWidth has been resized, false otherwise
     */
    bool CalibrateLeftAndRightParts(int leftWidth, int& rightDynamicWidth, int rightStaticWidth);
};

