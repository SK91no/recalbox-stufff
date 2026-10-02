//
// Created by bkg2k on 16/04/25.
//

#include "MenuDeserializer.h"
#include <utils/Files.h>

void MenuDeserializer::CheckMenuReferences()
{
  for(const auto& menu : mMenuDefinitions)
    for(const ItemDefinition& items : menu.second.mItems)
      if (items.Category() == ItemDefinition::ItemCategory::SubMenu)
        if (!mMenuDefinitions.contains(items.MenuType()))
        { LOG(LogFatal) << "[MenuProvider] Unknown reference to menu '" << (int)items.MenuType() << "' in menu captioned '" << menu.second.mCaption << '\''; }
}


void MenuDeserializer::CheckProperties(const String& parentId, const Array<const char*>& allowedProperties, const XmlNode& node)
{
  for(const XmlAttribute& attribute : node.attributes())
  {
    bool found = false;
    for(const char* allowed : allowedProperties)
      if (strcmp(allowed, attribute.name()) == 0) { found = true; break; }
    if (!found)
    { LOG(LogFatal) << "[MenuProvider] Unknown attribute " << attribute.name() << " in item " << node.name() << " of parent menu id " << parentId; }
  }
}

bool MenuDeserializer::DeserializeXml(const Path& xmlPath)
{
  String xml = Files::LoadFile(xmlPath);

  if (xml.empty()) return false;
  { LOG(LogDebug) << "[MenuProvider] Menu definitions loaded from " << xmlPath; }

  XmlDocument gameList;
  XmlResult result = gameList.load_string(xml.data());
  if (!result) { LOG(LogFatal) << "[MenuProvider] Cannot parse " << xmlPath << " file!"; return false; }

  XmlNode menus = gameList.child("menus");
  if (menus != nullptr)
    for (const XmlNode node: menus.children())
    {
      String name(node.name());
      if (name == "menu") DeserializeMenu("root", node, nullptr);
      else { LOG(LogError) << "[MenuProvider] Unknown menu node \"" << name << "\", skipping."; return false; }
    }
  else { LOG(LogError) << "[MenuProvider] No menu definition in \"" << xmlPath; return false; }

  return true;
}

void MenuDeserializer::DeserializeMenus(const Path& menuPath)
{
  mMenuDefinitions.clear();

  Path betaPath("/recalbox/share/system/.emulationstation/menu.xml");
  if (!DeserializeXml(menuPath))
    #if defined(BETA) || defined(DEBUG)
    if (!DeserializeXml(betaPath))
      #endif
      if (!DeserializeXml(Path(":/menu.xml")))
      { LOG(LogFatal) << "[MenuProvider] Failed to load a valid menu definition file !"; }

  // Post-processing
  CheckMenuReferences();

  { LOG(LogInfo) << "[MenuProvider] " << mMenuDefinitions.size() << " menu definitions loaded."; }
}

void MenuDeserializer::DeserializeMenu(const String& parentId, const XmlNode& menu, std::vector<ItemDefinition>* parentItems)
{
  // Syntax check
  static Array<const char*> properties { "id", "caption", "altCaptionIf", "animated", "help" , "grayedHelp", "foreach", "if", "grayedif", "icon", "include" };
  CheckProperties(parentId, properties, menu);

  String id = Xml::AttributeAsString(menu, "id", "");
  LOG(LogTrace) << "[MenuProvider] Deserialize menu " << id;
  if (id.empty()) { LOG(LogError) << "[MenuProvider] No id provided ! Menu skipped"; return; }
  MenuContainerType type = MenuConverters::MenuFromString(id);
  if (mMenuDefinitions.contains(type)) { LOG(LogFatal) << "[MenuProvider] Menu '" << id << "' redefined ! Menu skipped"; }
  String caption = Xml::AttributeAsString(menu, "caption", "<NO TITLE>");
  if (caption.empty()) { LOG(LogError) << "[MenuProvider] No title provided for menu id: " << id; }
  String altCaptionIf = Xml::AttributeAsString(menu, "altCaptionIf", "");
  bool animated = Xml::AttributeAsBool(menu, "animated", false);
  String help = Xml::AttributeAsString(menu, "help", "");
  String grayedHelp = Xml::AttributeAsString(menu, "grayedHelp", "");
  String condition = Xml::AttributeAsString(menu, "if", "");
  String grayedCondition = Xml::AttributeAsString(menu, "grayedif", "");
  String iconName = Xml::AttributeAsString(menu, "icon", "");
  String stringForEach = Xml::AttributeAsString(menu, "foreach", "none");
  ItemDefinition::ForEach forEach = ItemDefinition::ForEachFromString(stringForEach);
  bool include = Xml::AttributeAsBool(menu, "include", false);

  std::vector<ItemDefinition> items;
  for (const XmlNode node: menu.children())
  {
    String name = node.name();
    // Deserialize inner submenu
    if (name == "menu") DeserializeMenu(id, node, &items);
      // Menu reference
    else if (name == "menuref") DeserializeMenuRef(id, items, node);
      // Item
    else if (name == "item") DeserializeItem(id, items, node);
      // Setting
    else if (name == "setting") DeserializeSetting(id, items, node);
      // Header
    else if (name == "header") DeserializeHeader(id, items, node);
      // Error
    else { LOG(LogFatal) << "[MenuProvider] Unknown menu node " << name; }
  }

  #ifdef TEST_ONLY
  SelectedMenuIconType icon = SelectedMenuIconType::Unknown;
  #else
  SelectedMenuIconType icon = MenuThemeData::MenuIcons::IconFromString(iconName);
  #endif
  mMenuDefinitions.insert(type, MenuDefinition(type, icon, caption, altCaptionIf, std::move(items), help, grayedHelp, forEach, condition, grayedCondition, animated, include));
  if (parentItems != nullptr)
    parentItems->push_back(ItemDefinition(type, icon, caption, altCaptionIf, help, grayedHelp, forEach, condition, grayedCondition, include));
}

void MenuDeserializer::DeserializeMenuRef(const String& parentId, std::vector<ItemDefinition>& items, const XmlNode& menuRef)
{
  // Syntax check
  static Array<const char*> properties { "id", "icon", "caption", "altCaptionIf", "foreach", "if", "grayedif", "help", "grayedHelp", "include" };
  CheckProperties(parentId, properties, menuRef);

  // Get & check attributes
  String ref = Xml::AttributeAsString(menuRef, "id", "");
  LOG(LogTrace) << "[MenuProvider] Deserialize menuref " << ref;
  if (ref.empty()) { LOG(LogFatal) << "[MenuProvider] Menu reference missing or empty in parent menu " << parentId << ". Submenu skipped"; }
  MenuContainerType menu = MenuConverters::MenuFromString(ref);
  String iconName = Xml::AttributeAsString(menuRef, "icon", "");
  #ifdef TEST_ONLY
  SelectedMenuIconType icon = SelectedMenuIconType::Unknown;
  #else
  SelectedMenuIconType icon = MenuThemeData::MenuIcons::IconFromString(iconName);
  #endif

  // Lookup existing menu definitiuon
  MenuDefinition* def = mMenuDefinitions.try_get(menu);
  if (def == nullptr) { LOG(LogFatal) << "[MenuProvider] Menu " << ref << " must be fully defined before its first reference!"; __builtin_unreachable(); }

  // Override
  String caption = Xml::AttributeAsString(menuRef, "caption", def->mCaption);
  String altCaptionIf = Xml::AttributeAsString(menuRef, "altCaptionIf", def->mCaptionIf);
  String help = Xml::AttributeAsString(menuRef, "help", def->mHelp);
  String grayedHelp = Xml::AttributeAsString(menuRef, "grayedHelp", def->mUnselectableHelp);
  String condition = Xml::AttributeAsString(menuRef, "if", def->mCondition);
  String grayedCondition = Xml::AttributeAsString(menuRef, "grayedif", def->mGrayedCondition);
  String stringForEach = Xml::AttributeAsString(menuRef, "foreach", "none");
  ItemDefinition::ForEach forEach = ItemDefinition::ForEachFromString(stringForEach);
  bool   include = Xml::AttributeAsBool(menuRef, "include", def->mInclude);

  // Store
  items.push_back(ItemDefinition(menu, icon, caption, altCaptionIf, help, grayedHelp, forEach, condition, grayedCondition, include));
}

void MenuDeserializer::DeserializeItem(const String& parentId, std::vector<ItemDefinition>& items, const XmlNode& item)
{
  // Syntax check
  static const Array<const char*> properties { "id", "caption", "caption2", "altCaptionIf", "help" , "grayedHelp", "foreach", "if", "grayedif", "icon", "relaunch", "reboot", "bootConf" };
  CheckProperties(parentId, properties, item);

  // Get & check attributes
  String typeString = Xml::AttributeAsString(item, "id", "");
  LOG(LogTrace) << "[MenuProvider] Deserialize item " << typeString;
  if (typeString.empty()) { LOG(LogFatal) << "[MenuProvider] Item identifier missing or empty in parent menu " << parentId << ". Item skipped"; }
  MenuItemType type = MenuConverters::ItemFromString(typeString);
  if (type == MenuItemType::_Error_) { LOG(LogFatal) << "[MenuProvider] Unknown Item identifier " << typeString << " in parent menu " << parentId << ". Item skipped"; }
  String caption = Xml::AttributeAsString(item, "caption", "<MISSING CAPTION>");
  if (caption.empty()) { LOG(LogFatal) << "[MenuProvider] Item missing caption in parent menu " << parentId << '.'; }
  String altCaptionIf = Xml::AttributeAsString(item, "altCaptionIf", "");
  String caption2 = Xml::AttributeAsString(item, "caption2", "");
  String help = Xml::AttributeAsString(item, "help", "");
  String grayedHelp = Xml::AttributeAsString(item, "grayedHelp", "");
  String condition = Xml::AttributeAsString(item, "if", "");
  String grayedCondition = Xml::AttributeAsString(item, "grayedif", "");
  String iconName = Xml::AttributeAsString(item, "icon", "");
  String stringForEach = Xml::AttributeAsString(item, "foreach", "none");
  ItemDefinition::ForEach forEach = ItemDefinition::ForEachFromString(stringForEach);
  bool relaunch = Xml::AttributeAsBool(item, "relaunch", false);
  bool reboot = Xml::AttributeAsBool(item, "reboot", false);
  bool bootConf = Xml::AttributeAsBool(item, "bootConf", false);

  // Store
  #ifdef TEST_ONLY
  SelectedMenuIconType icon = SelectedMenuIconType::Unknown;
  #else
  SelectedMenuIconType icon = MenuThemeData::MenuIcons::IconFromString(iconName);
  #endif
  items.push_back(ItemDefinition(type, icon, caption, caption2, altCaptionIf, help, grayedHelp, forEach, condition, grayedCondition, relaunch, reboot, bootConf));
}

void MenuDeserializer::DeserializeSetting(const String& parentId, std::vector<ItemDefinition>& items, const XmlNode& item)
{
  // Syntax check
  static const Array<const char*> properties { "caption", "help", "grayedHelp", "if", "grayedIf", "core", "file", "fileType", "key", "type", "props", "values", "default" };
  CheckProperties(parentId, properties, item);

  // Get & check attributes
  String caption = Xml::AttributeAsString(item, "caption", "<MISSING CAPTION>");
  if (caption.empty()) { LOG(LogFatal) << "[MenuProvider] Setting missing caption in parent menu " << parentId << '.'; }
  LOG(LogTrace) << "[MenuProvider] Deserialize setting " << caption;

  String help = Xml::AttributeAsString(item, "help", "");
  String grayedHelp = Xml::AttributeAsString(item, "grayedHelp", "");
  String condition = Xml::AttributeAsString(item, "if", "");
  String grayedCondition = Xml::AttributeAsString(item, "grayedif", "");
  String core = Xml::AttributeAsString(item, "core", "");
  if (core.empty()) { LOG(LogFatal) << "[MenuProvider] Setting missing core in parent menu " << parentId << '.'; }
  String file = Xml::AttributeAsString(item, "file", "");
  if (file.empty()) { LOG(LogFatal) << "[MenuProvider] Setting missing file in parent menu " << parentId << '.'; }
  file.Replace("$home", "/recalbox/share/system")
      .Replace("$bios", "/recalbox/share/bios")
      .Replace("$saves", "/recalbox/share/saves")
      .Replace("$configs", "/recalbox/share/system/configs")
      .Replace("$.config", "/recalbox/share/system/.config");
  String fileTypeString = Xml::AttributeAsString(item, "fileType", "");
  ItemDefinition::FileType fileType = ItemDefinition::FileTypeFromString(fileTypeString);
  String key = Xml::AttributeAsString(item, "key", "");
  if (key.empty()) { LOG(LogFatal) << "[MenuProvider] Setting missing key in parent menu " << parentId << '.'; }
  String typeString = Xml::AttributeAsString(item, "type", "");
  ItemDefinition::SettingType type = ItemDefinition::SettingTypeFromString(typeString);
  String propsString = Xml::AttributeAsString(item, "props", "");
  ItemDefinition::SettingProps props = ItemDefinition::SettingPropsFromString(propsString);
  String value = Xml::AttributeAsString(item, "values", "");
  switch(type)
  {
    case ItemDefinition::SettingType::List: if (value.empty()) { LOG(LogFatal) << "[MenuProvider] Setting missing value in parent menu " << parentId << '.'; } break;
    case ItemDefinition::SettingType::NumericBoolean:
    case ItemDefinition::SettingType::TrueFalseBoolean:
    case ItemDefinition::SettingType::YesNoBoolean:
    case ItemDefinition::SettingType::CustomBoolean:
    case ItemDefinition::SettingType::Unknown:
    case ItemDefinition::SettingType::Range:
    default: break;
  }
  String defaultValue = Xml::AttributeAsString(item, "default", "");
  if (defaultValue.empty()) { LOG(LogFatal) << "[MenuProvider] Setting missing default value in parent menu " << parentId << '.'; }

  // Store
  items.push_back(ItemDefinition(caption, help, grayedHelp, condition, grayedCondition, core, Path(file), fileType, key, type, props, value, defaultValue ));
}

void MenuDeserializer::DeserializeHeader(const String& parentId, std::vector<ItemDefinition>& items, const XmlNode& header)
{
  // Syntax check
  static Array<const char*> properties { "caption" };
  CheckProperties(parentId, properties, header);

  String caption = Xml::AttributeAsString(header, "caption", "<MISSING CAPTION>");
  if (caption.empty()) { LOG(LogFatal) << "[MenuProvider] Header missing caption in parent menu " << parentId << '.'; }

  // Store
  items.push_back(ItemDefinition(caption));
}

