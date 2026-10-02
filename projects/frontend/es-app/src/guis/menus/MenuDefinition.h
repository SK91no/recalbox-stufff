//
// Created by bkg2k on 23/10/24.
//
#pragma once

#include "MenuItemType.h"
#include <utils/String.h>
#include <utils/eval/SimpleExpressionEvaluator.h>
#include <utils/locale/LocaleHelper.h>
#include <utils/cplusplus/Bitflags.h>

#ifndef TEST_ONLY
  #include "themes/MenuThemeData.h"
  #define SelectedMenuIconType MenuThemeData::MenuIcons::Type
#else
enum class SelectedMenuIconType
{
  Unknown = -1,
  Kodi = 0,
  System,
  RecalboxRGBDual,
  Updates,
  InGame,
  Controllers,
  Ui,
  Download,
  Arcade,
  Tate,
  Sound,
  Network,
  Scraper,
  Advanced,
  Quit,
  Restart,
  Shutdown,
  Fastshutdown,
  License,
  Lists,
  Bios,
};
#endif

// Single item definition
class ItemDefinition
{
  public:
    enum class ItemCategory
    {
      Item,
      SubMenu,
      Header,
      Setting,
    };

    enum class ForEach
    {
      None,             //!< No iterator
      SystemAll,        //!< Iterate on all systems
      SystemVisibles,   //!< Iterate on visible systems
      DevicesAll,       //!< Iterate on devices
      DevicesExternals, //!< Iterate on external devices
      PhysicalDisks,    //!< Iterate on physical disk devices
      ScriptsAll,       //!< Iterate on all scripts
      ThemeOptionAll,   //!< Iterate on all available theme options
    };

    enum class SettingType
    {
      Unknown,
      List,
      NumericBoolean,
      TrueFalseBoolean,
      YesNoBoolean,
      CustomBoolean,
      Range,
    };

    enum class SettingProps
    {
      None         = 0,
      Singlequoted = 1,
      DoubleQuoted = 2,
      Uppercase    = 4,
      Lowercase    = 8,
    };

    enum class FileType
    {
      Unknown,
      SimpleIni,
      SectionIni,
    };

    // Settings
    struct SettingDefinition
    {
      String mEmulator;            //!< Emulator name
      String mCore;                //!< Core name
      const Path mTargetFile;      //!< Target settings file
      const String mKey;           //!< Key to access in settings file
      const String mValues;        //!< Possible values (if applicable)
      const String mDefault;       //!< Default value if key or file do not exist
      FileType mFileType;          //!< Target settings file type
      SettingType mType;           //!< key/value type
      SettingProps mProps;         //!< value properties

      // Default constructor
      SettingDefinition() : mFileType(FileType::Unknown), mType(SettingType::Unknown), mProps(SettingProps::None) {}

      // Setting constructor
      SettingDefinition(const String& core, const Path& file, FileType fileType, const String& key, SettingType settingType, SettingProps props, const String& values, const String& defaultValue)
        : mTargetFile(file)
        , mKey(key)
        , mValues(values)
        , mDefault(defaultValue)
        , mFileType(fileType)
        , mType(settingType)
        , mProps(props)
      {
        if (!core.Extract(':', mEmulator, mCore, true))
          mEmulator = mCore = core;
      }
    };

    /*!
     * @brief Convert file type from string to enum
     * @param typeString Stringized filez type
     * @return File type
     */
    static FileType FileTypeFromString(const String& typeString);

    /*!
     * @brief Convert setting type from string to enum
     * @param typeString Stringized Setting type
     * @return Setting type
     */
    static SettingType SettingTypeFromString(const String& settingTypeString);

    /*!
     * @brief Convert setting properties from string to enum
     * @param properties Stringized properties
     * @return Properties
     */
    static SettingProps SettingPropsFromString(const String& properties);

    /*!
     * @brief Convert foreach from string to enum
     * @param properties Stringized foreach
     * @return foreach enum
     */
    static ForEach ForEachFromString(const String& forEach);

    /*!
     * @brief Header constructor
     * @param caption header caption
     */
    explicit ItemDefinition(const String& caption)
      : mCaption(caption)
      , mIcon(SelectedMenuIconType::Unknown)
      , mType(ItemCategory::Header)
      , mItemType(MenuItemType::_Error_)
      , mMenuType(MenuContainerType::_Error_)
      , mIterator(ForEach::None)
      , mReboot(false)
      , mRelaunch(false)
      , mBootConf(false)
      , mInclude(false)
    {}

    /*!
     * @brief Item constructor
     * @param itemType Item type
     * @param icon Icon name - empty for no icon
     * @param caption Item caption
     * @param help Regular help string
     * @param unselectableHelp Help string when the item is grayed
     * @param condition Dynamic condition for the item to appear in the menu
     */
    ItemDefinition(const MenuItemType itemType, SelectedMenuIconType icon, const String& caption, const String& caption2, const String& altCaptionIf, const String& help, const String& unselectableHelp, ForEach forEach, const String& condition, const String& grayedcondition, bool relaunch, bool reboot, bool bootConf)
      : mCaption(caption)
      , mCaption2(caption2)
      , mCaptionIf(altCaptionIf)
      , mHelp(help)
      , mUnselectableHelp(unselectableHelp)
      , mCondition(condition)
      , mGrayedCondition(grayedcondition)
      , mIcon(icon)
      , mType(ItemCategory::Item)
      , mItemType(itemType)
      , mMenuType(MenuContainerType::_Error_)
      , mIterator(forEach)
      , mReboot(reboot)
      , mRelaunch(relaunch)
      , mBootConf(bootConf)
      , mInclude(bootConf)
    {}

    /*!
     * @brief Submenu constructor
     * @param menu Menu identifier
     * @param icon Icon name - empty for no icon
     * @param caption Menu caption
     * @param help Regular help string
     * @param unselectableHelp Help string when the item is grayed
     * @param condition Dynamic condition for the submenu to appear in the menu
     */
    ItemDefinition(const MenuContainerType& menu, SelectedMenuIconType icon, const String& caption, const String& altCaptionIf, const String& help, const String& unselectableHelp, ForEach forEach, const String& condition, const String& grayedcondition, bool include)
      : mCaption(caption)
      , mCaptionIf(altCaptionIf)
      , mHelp(help)
      , mUnselectableHelp(unselectableHelp)
      , mCondition(condition)
      , mGrayedCondition(grayedcondition)
      , mIcon(icon)
      , mType(ItemCategory::SubMenu)
      , mItemType(MenuItemType::_Error_)
      , mMenuType(menu)
      , mIterator(forEach)
      , mReboot(false)
      , mRelaunch(false)
      , mBootConf(false)
      , mInclude(include)
    {}

    /*!
     * @brief Setting constructor
     * @param caption Menu caption
     * @param help Regular help string
     * @param core emulator:core
     * @param file setting file path
     * @param fileType file type
     * @param key configuration key
     * @param settingType configuration type
     * @param values configuration possible values if applicable
     * @param defaultValue configuration default value if the value does not exist in the file
     */
    ItemDefinition(const String& caption, const String& help, const String& grayedHelp, const String& condition, const String& grayedCondition, const String& core, const Path& file, FileType fileType, const String& key, SettingType settingType, SettingProps settingProps, const String& values, const String& defaultValue)
      : mCaption(caption)
      , mHelp(help)
      , mUnselectableHelp(grayedHelp)
      , mCondition(condition)
      , mGrayedCondition(grayedCondition)
      , mIcon(SelectedMenuIconType::Unknown)
      , mType(ItemCategory::Setting)
      , mItemType(MenuItemType::_Settings_)
      , mMenuType(MenuContainerType::_Error_)
      , mIterator(ForEach::None)
      , mReboot(false)
      , mRelaunch(false)
      , mBootConf(false)
      , mInclude(false)
      , mSetting(core, file, fileType, key, settingType, settingProps, values, defaultValue)
    {
    }

    /*
     * Accessors
     */

    [[nodiscard]] ItemCategory Category() const { return mType; }
    [[nodiscard]] MenuItemType Type() const { return mItemType; }
    [[nodiscard]] String Help() const { return _S(mHelp); }
    [[nodiscard]] String UnselectableHelp() const { return _S(mUnselectableHelp); }
    [[nodiscard]] MenuContainerType MenuType() const { return mMenuType; }
    [[nodiscard]] bool MenuInclude() const { return mInclude; }
    [[nodiscard]] const String& Condition() const { return mCondition; }
    [[nodiscard]] bool HasCondition() const { return !mCondition.empty(); }
    [[nodiscard]] const String& GrayedCondition() const { return mGrayedCondition; }
    [[nodiscard]] bool HasGrayedCondition() const { return !mGrayedCondition.empty(); }
    [[nodiscard]] SelectedMenuIconType Icon() const { return mIcon; }
    [[nodiscard]] ForEach Iterator() const { return mIterator; }
    [[nodiscard]] bool Reboot() const { return mReboot; }
    [[nodiscard]] bool Relaunch() const { return mRelaunch; }
    [[nodiscard]] bool BootConf() const { return mBootConf; }
    [[nodiscard]] const String& RawCaption() const { return mCaption; }
    [[nodiscard]] String Caption2() const { return _S(mCaption2); }
    [[nodiscard]] String Caption(SimpleExpressionEvaluator::IIdentifierEvaluator* evaluator) const
    {
      if (!mCaptionIf.empty())
      {
        String condition;
        String caption;
        for(String& subCondition : mCaptionIf.Split('|'))
          if (subCondition.Extract(';', condition, caption, true))
            if (SimpleExpressionEvaluator(*evaluator).Evaluate(condition))
              return _S(caption);
      }
      // No previous condition was true, take the caption
      return _S(mCaption);
    }

    [[nodiscard]] const String& SettingEmulator() const { return mSetting.mEmulator; }
    [[nodiscard]] const String& SettingCore() const { return mSetting.mCore; }
    [[nodiscard]] const Path& SettingTargetFile() const { return mSetting.mTargetFile; }
    [[nodiscard]] const String& SettingKey() const { return mSetting.mKey; }
    [[nodiscard]] const String& SettingValues() const { return mSetting.mValues; }
    [[nodiscard]] const String& SettingDefault() const { return mSetting.mDefault; }
    [[nodiscard]] FileType SettingFileType() const { return mSetting.mFileType; }
    [[nodiscard]] SettingType SettingValueType() const { return mSetting.mType; }
    [[nodiscard]] SettingProps SettingValueProps() const { return mSetting.mProps; }

    [[nodiscard]] const SettingDefinition& SettingDef() const { return mSetting; }

  private:
    // Menu items
    const String mCaption;                //!< Item/SubMenu/Header/setting caption
    const String mCaption2;               //!< Menu extra caption
    const String mCaptionIf;              //!< Conditional override captions
    const String mHelp;                   //!< Item/SubMenu regular help
    const String mUnselectableHelp;       //!< Item/SubMenu unselectable help
    const String mCondition;              //!< Dynamic condition
    const String mGrayedCondition;        //!< Dynamic grayed condition
    SelectedMenuIconType mIcon; //!< Icon
    const ItemCategory mType;             //!< Type
    const MenuItemType mItemType;         //!< Item type
    const MenuContainerType mMenuType;    //!< Submenu identifier
    const ForEach mIterator;              //!< Iterator
    const bool mReboot;                   //!< Must reboot on item change ?
    const bool mRelaunch;                 //!< Must relaunch on item change ?
    const bool mBootConf;                 //!< Changing this item requires to backup the conf in /boot ?
    const bool mInclude;                  //!< Include menu ? Valid only w/ sublenu item
    SettingDefinition mSetting;           //!< Settings
};

// Single menu structure
struct MenuDefinition
{
  const std::vector<ItemDefinition> mItems;           //!< Item definitions
  const String mCaption;                              //!< Menu caption
  const String mCaptionIf;                            //!< Conditional override captions
  const String mHelp;                                 //!< Item/SubMenu regular help
  const String mUnselectableHelp;                     //!< Item/SubMenu unselectable help
  const String mCondition;                            //!< Dynamic condition
  const String mGrayedCondition;                      //!< Dynamic grayed condition
  SelectedMenuIconType mIcon;               //!< Icon
  ItemDefinition::ForEach mIterator;                  //!< Iterator
  const MenuContainerType mType;                      //!< Menu type
  const bool mAnimated;                               //!< Menu animation ?
  const bool mInclude;                                //!< Include menu ?

  MenuDefinition(MenuContainerType type, SelectedMenuIconType icon, const String& caption, const String& altCaptionIf, std::vector<ItemDefinition>&& items,
                 const String& help, const String& unselectableHelp, ItemDefinition::ForEach forEach, const String& condition, const String& grayedcondition, bool animated, bool include)
    : mItems(items)
    , mCaption(caption)
    , mCaptionIf(altCaptionIf)
    , mHelp(help)
    , mUnselectableHelp(unselectableHelp)
    , mCondition(condition)
    , mGrayedCondition(grayedcondition)
    , mIcon(icon)
    , mIterator(forEach)
    , mType(type)
    , mAnimated(animated)
    , mInclude(include)
  {}
};

DEFINE_BITFLAG_ENUM(ItemDefinition::SettingProps, int)