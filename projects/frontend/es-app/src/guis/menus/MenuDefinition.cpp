//
// Created by bkg2k on 06/03/25.
//

#include "MenuDefinition.h"
#include <utils/storage/HashMap.h>
#include <utils/Log.h>

ItemDefinition::FileType ItemDefinition::FileTypeFromString(const String& typeString)
{
  static HashMap<String, ItemDefinition::FileType> sStringToFileType
  {
    { "ini",  ItemDefinition::FileType::SimpleIni },
    { "sini", ItemDefinition::FileType::SectionIni },
  };
  if (ItemDefinition::FileType* t = sStringToFileType.try_get(typeString); t != nullptr) return *t;
  { LOG(LogFatal) << "[MenuProvider] Unknown setting file type ID " << typeString; }
  __builtin_unreachable();
}

ItemDefinition::SettingType ItemDefinition::SettingTypeFromString(const String& settingTypeString)
{
  static HashMap<String, ItemDefinition::SettingType> sStringToSettingType
    {
      { "list",   ItemDefinition::SettingType::List },
      { "nbool",  ItemDefinition::SettingType::NumericBoolean },
      { "tbool",  ItemDefinition::SettingType::TrueFalseBoolean },
      { "ybool",  ItemDefinition::SettingType::YesNoBoolean },
      { "cbool",  ItemDefinition::SettingType::CustomBoolean },
      { "range",  ItemDefinition::SettingType::Range },
    };
  if (ItemDefinition::SettingType* t = sStringToSettingType.try_get(settingTypeString); t != nullptr) return *t;
  { LOG(LogFatal) << "[MenuProvider] Unknown setting file type ID " << settingTypeString; }
  __builtin_unreachable();
}

ItemDefinition::SettingProps ItemDefinition::SettingPropsFromString(const String& properties)
{
  SettingProps props = SettingProps::None;
  for(int i = properties.Count(); --i >= 0; )
    switch(properties[i])
    {
      case 'q' : props |= SettingProps::Singlequoted; break;
      case 'd' : props |= SettingProps::DoubleQuoted; break;
      case 'u' : props |= SettingProps::Uppercase; break;
      case 'l' : props |= SettingProps::Lowercase; break;
      default: { LOG(LogFatal) << "[MenuProvider] Unreconized setting proprerty in string " << properties; }
    }
  return props;
}

ItemDefinition::ForEach ItemDefinition::ForEachFromString(const String& forEach)
{
  static HashMap<String, ItemDefinition::ForEach> sStringToForEach
  {
    { "none",             ItemDefinition::ForEach::None },
    { "systems.all",      ItemDefinition::ForEach::SystemAll },
    { "systems.visible",  ItemDefinition::ForEach::SystemVisibles },
    { "devices.all",      ItemDefinition::ForEach::DevicesAll },
    { "devices.external", ItemDefinition::ForEach::DevicesExternals },
    { "devices.physical", ItemDefinition::ForEach::PhysicalDisks },
    { "scripts.all",      ItemDefinition::ForEach::ScriptsAll },
    { "theme.options",    ItemDefinition::ForEach::ThemeOptionAll },
  };
  if (ItemDefinition::ForEach* t = sStringToForEach.try_get(forEach); t != nullptr) return *t;
  { LOG(LogFatal) << "[MenuProvider] Unknown foreach iterator " << forEach; }
  __builtin_unreachable();
}
