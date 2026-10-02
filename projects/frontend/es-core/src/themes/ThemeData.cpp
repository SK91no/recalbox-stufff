#include "ThemeData.h"
#include "pugixml/pugixml.hpp"
#include <components/VideoComponent.h>
#include <MainRunner.h>
#include "components/BoxComponent.h"
#include "utils/Files.h"
#include "components/MarkdownText.h"
#include <rendering/textures/TextureManager.h>

HashSet<String> ThemeData::mNoProcessAttributes;

// helper
unsigned int ThemeData::getHexColor(const char* str, const pugi::xml_node& node)
{
  if(str == nullptr)
  {
    { LOGT(LogError) << "[Theme] Empty color"; }
    return 0xFFFFFFFF;
  }

  String string('$');
  string.Append(str);
  int val = 0;
  if ((string.Count() != 7 && string.Count() != 9) || !string.TryAsInt(val))
    { LOGT(LogError) << "[Theme] " << FileList(node) << "Invalid color (bad length, \"" + String(str) + "\" - must be 6 or 8)"; }

  if (string.Count() == 7) val = (val << 8) | 0xFF;
  return (unsigned int)val;
}

ThemeData::ThemeData(ThemeFileCache& cache, const SystemData* system, IGlobalVariableResolver& globalResolver, IThemeContextualVariableResolver& contextualResolver)
  : mCache(cache)
  , mSystem(system)
  , mVariables(contextualResolver, system)
  , mResolver(contextualResolver)
  , mVersion(0)
  , mLangageCode("en")
  , mCountryCode("us")
  , mLangageCodeInteger((int)'e' | ((int)'n' >> 8))
  , mLanguageCountryCodeInteger((int)'e' | ((int)'n' >> 8) | ((int)'U' >> 16) | ((int)'S' >> 24))
  , mRegionCodeInteger(0)
  , mGlobalResolver(globalResolver)
  , mGameResolver(nullptr)
  , mCompatiblity(Compatibility::None)
  , mValid(false)
  , mDefaultTrans(false)
{
  mSystemThemeFolder.clear();
  mRandomPath.clear();

  String lccc = RecalboxConf::Instance().GetSystemLanguage().LowerCase();
  if (lccc.size() >= 5)
  {
    int pos = lccc.Find('_');
    if (pos >= 2 && pos < (int) lccc.size() - 1)
    {
      mLangageCode = lccc.SubString(0, pos);
      mCountryCode = lccc.SubString(pos + 1).UpperCase();
      mLangageCodeInteger = (int)mLangageCode[0] | ((int)mLangageCode[1] << 8);
      mLanguageCountryCodeInteger = mLangageCodeInteger | ((int)mCountryCode[0] << 16) | ((int)mCountryCode[1] << 24);
    }
  }

  if (mNoProcessAttributes.empty())
  {
    mNoProcessAttributes.insert("if");
    mNoProcessAttributes.insert("name");
    mNoProcessAttributes.insert("value");
    mNoProcessAttributes.insert("region");
    mNoProcessAttributes.insert("extra");
    mNoProcessAttributes.insert("showIf");
  }
}

bool ThemeData::CheckThemeOption(String& selected, const String& subset) const
{
  if (mSubSets.empty()) return false;

  // Empty subset?
  ThemeOption* option = GetSubset(subset);
  if (option == nullptr || !option->IsValid()) return false;

  // Try to fix the value if not found
  std::vector<ThemeOption::Value> sortedValues = option->SortedValue();
  for(const ThemeOption::Value& s : sortedValues)
    if (s.Raw() == selected)
      return false;

  // Take first value
  selected = sortedValues.front().Raw();
  return true;
}

void ThemeData::loadFile(const String& systemThemeFolder, const Path& path)
{
  if(!path.Exists())
  {
    { LOGT(LogError) << "[Theme] " << FileList() << "File does not exist!"; }
    return;
  }

  mVersion = 0;
  mSystemThemeFolder = systemThemeFolder;
  mThemeName = path.IsDirectory() ? path.Filename() : path.Directory().Filename();

  // Region is set at the begining of the main theme loading
  String region = RecalboxConf::Instance().GetThemeRegion();
  if      (region == "jp") mRegionCodeInteger = 'J' | ((int)'P' << 8);
  else if (region == "eu") mRegionCodeInteger = 'E' | ((int)'U' << 8);
  else mRegionCodeInteger = 'U' | ((int)'S' << 8);

  pugi::xml_document doc;
  pugi::xml_parse_result res = doc.load_string(mCache.File(path).data());
  if(!res) { LOGT(LogError) << "[Theme] " << FileList() << "XML parsing error: \n    " + String(res.description()); }

  pugi::xml_node root = doc.child("theme");
  if(!root) { LOGT(LogError) << "[Theme] " << FileList() << "Missing <theme> tag!"; }
  mCompatiblity = ExtractCompatibility(root);

  Clear(path);

  // Extract options
  if (mSystem == nullptr)
  {
    // Get options
    CrawlThemeSubSets(path);

    // Finalize options
    for(const auto& kv : mSubSets)
      mSubSets.try_get(kv.first)->Finalize();

    // Check user preferences
    bool needSave = false;
    for(const auto& kv : mSubSets)
    {
      const ThemeOption& opt = kv.second;
      String currentValue = RecalboxConf::Instance().GetThemeOption(mThemeName, opt.Name());
      if (CheckThemeOption(currentValue, opt.Name())) { RecalboxConf::Instance().SetThemeOption(mThemeName, opt.Name(), currentValue); needSave = true; }
    }
    if (needSave) RecalboxConf::Instance().Save();
  }

  parseIncludes(root, mSystem == nullptr);
  parseViews(root);
  parseFeatures(root);
  mIncludePathStack.pop_back();


  // Validate the theme
  mValid = true;
}

void ThemeData::CrawlSubsets(const pugi::xml_node& root)
{
  for (pugi::xml_node node = root.child("subset"); node != nullptr; node = node.next_sibling("subset"))
    if (Condition(node))
    {
      String subSet = mVariables.Resolve(node.attribute("subset").as_string());
      ThemeOption* option = mSubSets.try_get(subSet);
      if (option == nullptr) { mSubSets.insert(subSet, ThemeOption(subSet)); option = mSubSets.try_get(subSet); }
      if (option != nullptr)
        for (pugi::xml_attribute& attribute : node.attributes())
        {
          String name = attribute.name();
          int locale = ExtractLocalizedCode(name);
          if ((locale == 0 || locale == mLangageCodeInteger || locale == mLanguageCountryCodeInteger || locale == mRegionCodeInteger))
          {
            if (name == "title") option->SetUserFriendlyName(mVariables.Resolve(attribute.as_string()));
            if (name == "help")  option->SetUserFriendlyHelp(mVariables.Resolve(attribute.as_string()));
          }
        }
    }
}

void ThemeData::parseVariables(const pugi::xml_node& root)
{
  bool found = false;
  for (pugi::xml_node bloc = root.child("variables"); bloc != nullptr; bloc = bloc.next_sibling("variables"))
    if (Condition(bloc))
      for (pugi::xml_node var = bloc.child("variable"); var != nullptr; var = var.next_sibling("variable"))
        if (Condition(var))
        {
          pugi::xml_attribute nameAttribute = var.attribute("name");
          pugi::xml_attribute valueAttribute = var.attribute("value");
          if (!nameAttribute || !valueAttribute) { LOGT(LogError) << "[Theme] " << FileList(var) << " Variable missing name or value attribute!"; continue; }
          String name = nameAttribute.as_string();
          String value = valueAttribute.as_string();
          if (name.empty()) { LOGT(LogError) << "[Theme] " << FileList(var) << " Variable has empty name!"; continue; }
          mVariables.Add(name, value);
          found = true;
        }
  if (found) mVariables.ResolveUserVariables();
}

void ThemeData::parseIncludes(const pugi::xml_node& root, bool mainTheme)
{
  parseVariables(root);

  for (pugi::xml_node node = root.child("include"); node != nullptr; node = node.next_sibling("include"))
    if (Condition(node))
      if (parseSubset(node))
      {
        pugi::xml_attribute pathAttribute = node.attribute("path");
        String str = mVariables.Resolve((bool)pathAttribute ? pathAttribute.as_string() : node.text().get());
        { LOGT(LogTrace) << "[Theme] Include " << str; }
        resolveSystemVariable(mIncludePathStack.back().Directory(), mSystemThemeFolder, str, mRandomPath);
        str.Trim();
        { LOGT(LogTrace) << "[Theme]   Resolves to: " << str; }
        //workaround for an issue in parseincludes introduced by variable implementation
        if (!str.Contains("//"))
        {
          Path path = Path(str).ToAbsolute(mIncludePathStack.back().Directory());
          if(!TextureManager::Exists(path))
          {
            { LOGT(LogWarning) << "[Theme] Included file \"" << str << "\" not found! (resolved to \"" << path.ToString() << "\")"; }
            continue;
          }

          String errorString; errorString += "    from included file \"" + str + "\":\n    ";

          mIncludePathStack.push_back(path);

          pugi::xml_document includeDoc;
          pugi::xml_parse_result result = includeDoc.load_string(mCache.File(path).data());
          if(!result) { LOGT(LogError) << "[Theme] " << FileList(node) << errorString + "Error parsing file: \n    " + result.description(); }

          pugi::xml_node newRoot = includeDoc.child("theme");
          if(!newRoot) { LOGT(LogError) << "[Theme] " << FileList(node) << errorString + "Missing <theme> tag!"; }

          parseIncludes(newRoot, mainTheme);
          parseViews(newRoot);
          parseFeatures(newRoot);

          mIncludePathStack.pop_back();
        }
      }
}

bool ThemeData::parseSubset(const pugi::xml_node& node)
{
  // No subset?
  if (node.attribute("subset") == nullptr) return true;

  const String subsetAttr = mVariables.Resolve(node.attribute("subset").as_string());
  String nameAttr;
  for (pugi::xml_attribute& attribute : node.attributes())
  {
    String name = attribute.name();
    //int locale = ExtractLocalizedCode(name);
    if (name == "name")
      //if (locale == mLangageCodeInteger || locale == mLanguageCountryCodeInteger || locale == mRegionCodeInteger)
        nameAttr = mVariables.Resolve(attribute.as_string());
  }
  //if (nameAttr.empty()) nameAttr = mVariables.Resolve(node.attribute("name").as_string());

  return (nameAttr == RecalboxConf::Instance().GetThemeOption(mThemeName, subsetAttr));
}

void ThemeData::parseFeatures(const pugi::xml_node& root)
{
  for (pugi::xml_node node = root.child("feature"); node != nullptr; node = node.next_sibling("feature"))
  {
    if(!node.attribute("supported"))
    { LOGT(LogError) << "[Theme] " << FileList(node) << "Feature missing \"supported\" attribute!"; continue; }

    const String supportedAttr = mVariables.Resolve(node.attribute("supported").as_string());
    if (ThemeSupport::SupportedFeatures().contains(supportedAttr))
      parseViews(node);
  }
}

void ThemeData::parseViews(const pugi::xml_node& root)
{
  // parse views
  for (pugi::xml_node node = root.child("view"); node != nullptr; node = node.next_sibling("view"))
  {
    if(!node.attribute("name")) { LOGT(LogError) << "[Theme] " << FileList(node) << "View missing \"name\" attribute!"; continue; }
    if (!Condition(node)) continue;
    for(String& viewName : mVariables.Resolve(node.attribute("name").as_string()).LowerCase().Split(','))
    {
      viewName.Trim();
      if (ThemeSupport::SupportedViews().contains(viewName))
      {
        ThemeView& view = mViews.insert(std::pair<String, ThemeView>(viewName, ThemeView())).first->second;
        parseView(node, view, false);
      }
    }
  }
}

ThemeItemType ThemeData::ExtractLimitedVisibility(const String& visibility)
{
  if (visibility.empty()) return ThemeItemType::None;
  if (int pos = visibility.Find(','); pos > 0)
  {
    ThemeItemType result = ThemeItemType::None;
    for (const String& v: visibility.Split(','))
      if (v == "game") result |= ThemeItemType::Game;
      else if (v == "folder") result |= ThemeItemType::Folder;
      else if (v == "header") result |= ThemeItemType::Header;
    return result;
  }
  if (visibility == "game") return ThemeItemType::Game;
  if (visibility == "folder") return ThemeItemType::Folder;
  if (visibility == "header") return ThemeItemType::Header;
  return ThemeItemType::None;
}

void ThemeData::parseView(const pugi::xml_node& root, ThemeView& view, bool forcedExtra)
{
  for (pugi::xml_node node = root.first_child(); node != nullptr; node = node.next_sibling())
  {
    String typeString(node.name());
    // Process sub folder extra
    if (typeString == "extras" && !forcedExtra)
    {
      parseView(node, view, true);
      continue;
    }

    if (!node.attribute("name"))
    { LOGT(LogError) << "[Theme] " << FileList(node) << "Element of type \"" << typeString << R"(" missing "name" attribute!)"; continue; }
    if (!IsMatchingLocaleOrRegionOrNeutral(typeString)) continue;

    // Process normal item type
    ThemeElementType* type = ThemeSupport::ElementType().try_get(typeString);
    if (type == nullptr) { LOGT(LogError) << "[Theme] " << FileList(node) << "Unknown element of type \"" << typeString << "\"!"; continue; }
    const ThemePropertyNameBits* properties = ThemeSupport::ElementMap().try_get(*type);
    if (properties == nullptr) { LOGT(LogError) << "[Theme] " << FileList(node) << " Cannot get properties of element of type \"" << typeString << "\"!"; continue; }

    if (IsMatchingRegionOldTag(node))
    {
      // Limited view
      bool extra = node.attribute("extra").as_bool(false) || forcedExtra;
      ThemeItemType limitedVisibility = ExtractLimitedVisibility(mVariables.Resolve(node.attribute("showIf").as_string()));

      // Run through multiple names
      String names = mVariables.Resolve(node.attribute("name").as_string());
      String name;
      for(bool again = true; again;)
      {
        if (again = names.Extract(',', name, names, true); !again) name = names.Trim();
        int* elementIndex = view.mElements.try_get(name);
        if (elementIndex == nullptr)
        {
          view.mElements[name] = (int) view.mElementArray.size();
          view.mElementArray.push_back(ThemeElement(name, *type, extra, limitedVisibility, mVariables));
          parseElement(node, *properties, view.mElementArray.back());
        }
        else
        {
          view.mElementArray[*elementIndex].MergeVisibility(limitedVisibility);
          parseElement(node, *properties, view.mElementArray[*elementIndex].MergeExtra(extra));
        }
      }
    }
  }
}

bool ThemeData::IsMatchingRegionOldTag(const pugi::xml_node& node) const
{
  // No region?
  if (node.attribute("region") == nullptr) return true;

  for(String& region : mVariables.Resolve(node.attribute("region").as_string()).UpperCase().Split(','))
    if (region.Trim(); region.size() == 2)
      if (((((int)region[1] << 8) | region[0]) & 0xDFDF) == mRegionCodeInteger)
        return true;
  return false;
}

void ThemeData::parseElement(const pugi::xml_node& root, const ThemePropertyNameBits& typeList, ThemeElement& element)
{
  String elementNode = mVariables.Resolve(root.attribute("name").as_string());
  if (!Condition(root)) return;
  // process node attributes
  for (pugi::xml_attribute& attribute : root.attributes())
  {
    String name = attribute.name();
    if (mNoProcessAttributes.contains(name)) continue;
    // Localized?
    ThemePropertyName* property = nullptr;
    bool localized = false;
    if (!IsPropertyMatchingLocaleOrRegionOrNeutral(element, name, property, localized)) continue;
    // Check object property
    if (property == nullptr || !typeList.IsSet(*property))
    {
      { LOGT(LogError) << "[Theme] " << FileList(root) << "Unknown property type \"" + name + "\" (for element " << elementNode << " of type " << root.name() << ")."; }
      continue;
    }
    // Process
    parseProperty(root, elementNode, *property, mVariables.Resolve(attribute.as_string()).Trim(), element);
    if (localized && element.HasProperty(*property)) element.SetLocalized(*property); // Match!
  }
  // Process sub nodes
  for (pugi::xml_node node = root.first_child(); node != nullptr; node = node.next_sibling())
  {
    String name = node.name();
    // Localized?
    ThemePropertyName* property = nullptr;
    bool localized = false;
    if (!IsPropertyMatchingLocaleOrRegionOrNeutral(element, name, property, localized)) continue;
    // Check object property
    if (property == nullptr || !typeList.IsSet(*property))
    {
      { LOGT(LogError) << "[Theme] " << FileList(node) << "Unknown property type \"" + name + "\" (for element " << elementNode << " of type " << root.name() << ")."; }
      continue;
    }
    // Get value from attribute or text
    String value = mVariables.Resolve(node.attribute("value") != nullptr ? node.attribute("value").as_string() : node.text().as_string());
    // Process
    if (Condition(node))
    {
      parseProperty(node, elementNode, *property, value.Trim(), element);
      if (localized && element.HasProperty(*property)) element.SetLocalized(*property); // Match!
    }
  }
}

bool ThemeData::IsMatchingLocaleOrRegionOrNeutral(String& name) const
{
  int locale = ExtractLocalizedCode(name);
  // No locale/region
  if (locale == 0) return true;
  // Match?
  return locale == mLangageCodeInteger || locale == mLanguageCountryCodeInteger || locale == mRegionCodeInteger;
}

bool ThemeData::IsPropertyMatchingLocaleOrRegionOrNeutral(ThemeElement& element, String& name, ThemePropertyName*& property, [[out]] bool& matchLocalization) const
{
  int locale = ExtractLocalizedCode(name);
  if (locale == 0) // No locale/region
  {
    property = ThemeSupport::PropertyName().try_get(name);
    return property == nullptr || !element.IsAlreadyLocalized(*property); // If already localized, ignore non-localized text
  }
  // Property is localized/regionalized
  if (locale != mLangageCodeInteger && locale != mLanguageCountryCodeInteger && locale != mRegionCodeInteger) return false;
  property = ThemeSupport::PropertyName().try_get(name);
  matchLocalization = true;
  return true;
}

void ThemeData::parseProperty(const pugi::xml_node& node, const String& elementName, ThemePropertyName propertyName, String& value, ThemeElement& element)
{
  // Check variables
  ThemeContexts context = mResolver.AnalyseContextualVariables(value);
  if (!isEmpty(context)) element.AddVariableProperty(propertyName, value, context);
  else switch(ThemeSupport::PropertyTypeFromName(propertyName))
  {
    case ThemePropertyType::NormalizedPair:
    {
      float x = 0;
      float y = 0;
      if (PropertyBag::ConvertPair(value, x, y))
        element.AddVectorProperty(propertyName, x, y);
      else
        { LOGT(LogError) << "[Theme] " << FileList(node) << "invalid normalized pair (property \"" << ThemeSupport::ReversePropertyName(propertyName) << "\", value \"" << value << "\")"; }
      break;
    }
    case ThemePropertyType::String:
    {
      resolveSystemVariable(mIncludePathStack.back().Directory(), mSystemThemeFolder, value, mRandomPath);
      element.AddStringProperty(propertyName, value);
      break;
    }
    case ThemePropertyType::Path:
    {
      // Patch for older logo compatibility
      if (value.EndsWith("/logo.svg")) if (value.Contains("/recalbox-next/")) value.Assign("${system.logo}");
      resolveSystemVariable(mIncludePathStack.back().Directory(), mSystemThemeFolder, value, mRandomPath);
      Path path = Path(value).ToAbsolute(mIncludePathStack.back().Directory());
      if (TextureManager::Exists(path))
        element.AddStringProperty(propertyName, path.ToString());
      break;
    }
    case ThemePropertyType::Color:
    {
      element.AddIntProperty(propertyName, (int) getHexColor(value.c_str(), node));
      break;
    }
    case ThemePropertyType::Float:
    {
      float floatVal = 0;
      if (!value.TryAsFloat(floatVal))
      { LOGT(LogError) << "[Theme] " << FileList(node) << "invalid float value (property \"" << ThemeSupport::ReversePropertyName(propertyName) << "\", value \"" + value + "\")"; }
      element.AddFloatProperty(propertyName, floatVal);
      break;
    }
    case ThemePropertyType::Boolean:
    {
      // only look at first char
      char first = value.empty() ? '\0' : value[0];
      element.AddBoolProperty(propertyName, (first == '1' || first == 't' || first == 'T' || first == 'y' || first == 'Y'));
      break;
    }
    default:
    { LOGT(LogError) << "[Theme] " << FileList(node) << "Unknown ThemeSupport::ElementPropertyType for \"" << elementName << "\", property " << ThemeSupport::ReversePropertyName(propertyName); }
  }
}

const ThemeElement* ThemeData::Element(const String& viewName, const String& elementName, ThemeElementType expectedType, ThemeElementType expectedType2) const
{
  ThemeView* view = mViews.try_get(viewName);
  if (view == nullptr) return nullptr; // not found
  int* elementIndex = view->mElements.try_get(elementName);
  if (elementIndex == nullptr) return nullptr;
  ThemeElement* element = &(view->mElementArray[*elementIndex]);

  if(element->Type() == expectedType || element->Type() == expectedType2 || expectedType == ThemeElementType::Polymorphic) return element;

  { LOGT(LogWarning) << "[Theme] Mismatched theme type for [" << viewName << "." << elementName << "] - expected \"" << ThemeElementTypeToString(expectedType) << "\", got \"" << ThemeElementTypeToString(element->Type()) << "\""; }
  return nullptr;
}

void ThemeData::LoadMain(const Path& root)
{
  { LOGT(LogInfo) << "[Theme] Loading main theme from: " << root; }
  loadFile(String::Empty, root);
}

void ThemeData::LoadSystem(const String& systemFolder, const Path& root)
{
  { LOGT(LogInfo) << "[Theme] Loading " << mSystem->FullName() <<"' system theme from: " << root; }
  loadFile(systemFolder, root);
}

void ThemeData::RefreshContextualExtraProperties(ThemeExtras::List& extras, const String& view) const
{
  for(ThemeExtras::Extra& extra : extras)
  {
    const ThemeElement* elem = Element(view, extra.Name(), extra.Component().GetThemeElementType(), extra.Component().GetThemeElementType2());

    // Refresh properties
    if (elem != nullptr && elem->HasUnresolvedVariables())
      extra.Component().DoApplyThemeElement(*this, view, extra.Name(), ThemePropertyCategory::All);

    // Visibility ?
    if (const FileData* game = mVariables.GetGameContext(); game != nullptr && elem->HasLimitedVisiblity())
    {
      bool hidden = true;
      if (hasFlag(elem->LimitedVisibility(), ThemeItemType::Game) && game->IsGame()) hidden = false;
      else if (hasFlag(elem->LimitedVisibility(), ThemeItemType::Folder) && game->IsFolder()) hidden = false;
      else if (hasFlag(elem->LimitedVisibility(), ThemeItemType::Header) && game->IsHeader()) hidden = false;
      extra.Component().setThemeDisabled(hidden);
    }
  }
}

ThemeExtras::List ThemeData::GetExtras(const String& view, WindowManager& window, IVideoComponentAction* videoInterface) const
{
  ThemeExtras::List comps;

  ThemeView* viewTheme = mViews.try_get(view);
  if (viewTheme == nullptr) return comps;

  bool uniqueVideo = false;
  for(const ThemeElement& elem : viewTheme->mElementArray)
  {
    if(elem.Extra())
    {
      ThemableComponent* comp = nullptr;
      switch(elem.Type())
      {
        case ThemeElementType::Image: comp = new ImageComponent(window); break;
        case ThemeElementType::Box: comp = new BoxComponent(window); break;
        case ThemeElementType::Video: if (!uniqueVideo) { comp = new VideoComponent(window, videoInterface); uniqueVideo = true; } break;
        case ThemeElementType::Text: comp = new TextComponent(window); break;
        case ThemeElementType::ScrollText: comp = new TextScrollComponent(window); break;
        case ThemeElementType::Markdown: comp = new MarkdownText(window); break;
        case ThemeElementType::TextList:
        case ThemeElementType::LogoList:
        case ThemeElementType::CarouselList:
        case ThemeElementType::GridList:
        case ThemeElementType::Container:
        case ThemeElementType::NinePatch:
        case ThemeElementType::DateTime:
        case ThemeElementType::Rating:
        case ThemeElementType::Sound:
        case ThemeElementType::HelpSystem:
        case ThemeElementType::Carousel:
        case ThemeElementType::MenuBackground:
        case ThemeElementType::MenuIcons:
        case ThemeElementType::MenuSwitch:
        case ThemeElementType::MenuSlider:
        case ThemeElementType::MenuButton:
        case ThemeElementType::MenuText:
        case ThemeElementType::MenuTextSmall:
        case ThemeElementType::MenuSize:
        case ThemeElementType::None:
        case ThemeElementType::Polymorphic:
        case ThemeElementType::MenuSection:
        case ThemeElementType::Keyboard:
        default: break;
      }

      if (comp != nullptr)
      {
        comp->setDefaultZIndex(10);
        comp->DoApplyThemeElement(*this, view, elem.Name(), ThemePropertyCategory::All);
        comps.push_back({ elem.Name(), elem.Type(), comp });
      }
      else { LOGT(LogWarning) << "[Theme] Extra type unknown: " << ThemeElementTypeToString(elem.Type()); }
    }
  }

  return comps;
}

void ThemeData::CrawlThemeSubSets(const Path& themeRootPath)
{
  if (themeRootPath.Exists())
  {
    // Crawl
    mIncludePathStack.push_back(themeRootPath);
    pugi::xml_document doc;
    doc.load_string(mCache.File(themeRootPath).data());
    pugi::xml_node root = doc.child("theme");
    CrawlIncludes(root);
    mIncludePathStack.pop_back();
  }
}

void ThemeData::CrawlIncludes(const pugi::xml_node& root)
{
  CrawlSubsets(root);

  for (pugi::xml_node node = root.child("include"); node != nullptr; node = node.next_sibling("include"))
    if (Condition(node))
    {
      String subSetName = mVariables.Resolve(node.attribute("subset").as_string());
      String subSetRawValue = mVariables.Resolve(node.attribute("name").as_string());
      String subSetTransValue;
      for (pugi::xml_attribute& attribute : node.attributes())
      {
        String name = attribute.name();
        int locale = ExtractLocalizedCode(name);
        if (name == "name")
          if (locale == mLangageCodeInteger || locale == mLanguageCountryCodeInteger || locale == mRegionCodeInteger)
            subSetTransValue = mVariables.Resolve(attribute.as_string());
      }
      if (!subSetName.empty() && !subSetRawValue.empty())
      {
        if (ThemeOption* option = mSubSets.try_get(subSetName); option != nullptr)
          option->AddValue(subSetRawValue, subSetTransValue);
        else
          mSubSets.insert(subSetName, ThemeOption(subSetName).AddValue(subSetRawValue, subSetTransValue));
      }

      pugi::xml_attribute pathAttribute = node.attribute("path");
      Path relPath(mVariables.Resolve((bool)pathAttribute ? pathAttribute.as_string() : node.text().get()));
      Path path = relPath.ToAbsolute(mIncludePathStack.back().Directory());
      mIncludePathStack.push_back(path);
      pugi::xml_document includeDoc;
      includeDoc.load_string(mCache.File(path).data());
      pugi::xml_node newRoot = includeDoc.child("theme");
      CrawlIncludes(newRoot);
      mIncludePathStack.pop_back();
    }
}

RecalboxConf::Transition ThemeData::getThemeTransition() const
{
  mDefaultTrans = false;
  const auto* elem = Element("system", "systemcarousel", ThemeElementType::Carousel, ThemeElementType::None);
  if (elem != nullptr)
    if (elem->HasProperty(ThemePropertyName::DefaultTransition))
      {
      mDefaultTrans = true;
      return RecalboxConf::TransitionFromString(elem->AsString(ThemePropertyName::DefaultTransition));
      }
  return RecalboxConf::Instance().GetThemeTransition();
}

bool ThemeData::isFolderHandled() const
{
  const auto* elem = Element("detailed", "md_folder_name", ThemeElementType::Text, ThemeElementType::None);
  return elem != nullptr && elem->HasProperty(ThemePropertyName::Pos);
}

String ThemeData::FileList()
{
  String result = "from theme \"" + mIncludePathStack.front().ToString() + "\"\n";
  for (auto it = mIncludePathStack.begin() + 1; it != mIncludePathStack.end(); it++)
    result += "  (from included file \"" + (*it).ToString() + "\")\n";
  result += "    ";
  return result;
}

String ThemeData::FileList(const pugi::xml_node& node)
{
  String result = String("Path : ").Append((node.path('/')))
                                            .Append(String::CRLF)
                                            .Append(FileList());
  return result;
}

void ThemeData::Reset()
{
  mViews.clear();
  mSubSets.clear();
  mIncludePathStack.clear();
  mVersion = 0.f;
  mSystemThemeFolder.clear();
  mRandomPath.clear();
}

void ThemeData::resolveSystemVariable(const Path& refPath, const String& systemThemeFolder, [[in,out]] String& path, String& randomPath)
{
  if (path.Contains('$'))
  {
    if (path.Contains("$system"))
    {
      Path systemPath(String(path).Replace("$system", systemThemeFolder));
      if (!systemPath.IsAbsolute()) systemPath = systemPath.ToAbsolute(refPath);
      if (!systemPath.Exists())
      {
        Path defaultPath(String(path).Replace("$system", ThemeManager::sDefaultSystemThemeFolder));
        if (!defaultPath.IsAbsolute()) defaultPath = defaultPath.ToAbsolute(refPath);
        if (defaultPath.Exists()) path.Replace("$system", ThemeManager::sDefaultSystemThemeFolder);
        else path.Replace("$system", systemThemeFolder);
      }
      else path.Replace("$system", systemThemeFolder);
    }
    path.Replace("$language", mLangageCode)
        .Replace("$country", mCountryCode);
    mGlobalResolver.ResolveVariableIn(path);
    if (mSystem != nullptr)
    {
      path.Replace("$fullname", mSystem->Descriptor().FullName())
          .Replace("$type", SystemDescriptor::ConvertSystemTypeToString(mSystem->Descriptor().Type()))
          .Replace("$pad", SystemDescriptor::ConvertDeviceRequirementToString(mSystem->Descriptor().PadRequirement()))
          .Replace("$keyboard", SystemDescriptor::ConvertDeviceRequirementToString(mSystem->Descriptor().KeyboardRequirement()))
          .Replace("$mouse", SystemDescriptor::ConvertDeviceRequirementToString(mSystem->Descriptor().MouseRequirement()))
          .Replace("$releaseyear", String(mSystem->Descriptor().ReleaseDate().Year()))
          .Replace("$netplay", mSystem->Descriptor().HasNetPlayCores() ? "yes" : "no")
          .Replace("$lightgun", mSystem->Descriptor().LightGun() ? "yes" : "no");
      if (mGameResolver != nullptr) mGameResolver->ResolveVariableIn(path);
    }
    PickRandomPath(path, randomPath);
  }
}

void ThemeData::PickRandomPath(String& value, String& randomPath)
{
  if (!value.Contains(sRandomMethod)) return;

  String args;
  if (value.Extract(sRandomMethod, ")", args, true))
    if (randomPath.empty())
    {
      String::List paths = args.Split(',');
      std::random_device rd;
      std::default_random_engine engine(rd());
      const int max = (int) paths.size();
      std::uniform_int_distribution<int> distrib { 0, max - 1 };
      randomPath = paths[distrib(engine)];
    }

  value.Replace(sRandomMethod + args + ')', randomPath);
}

int ThemeData::ExtractLocalizedCode(String& name)
{
  if (int pos = name.Find('.'); pos > 0 && pos < (int)name.size() - 2)
  {
    int result = (int)name[pos + 1] | ((int)name[pos + 2] << 8); // language code
    if (pos < (int) name.size() - 5)
      result |= ((int)name[pos + 1] << 16) | ((int)name[pos + 2] << 24);
    name.Delete(pos, INT32_MAX);
    return result;
  }
  return 0;
}

bool ThemeData::Condition(const pugi::xml_node& node)
{
  // if="condition"
  if (pugi::xml_attribute conditionAttribute = node.attribute("if"); conditionAttribute)
  {
    if (String condition = mVariables.Resolve(conditionAttribute.as_string()); condition.Trim().empty())
      { LOGT(LogError) << "[Theme] Empty condition! Assuming true."; }
    else
      if (SimpleTokenizer tokenizer(condition); !EvaluateExpression(tokenizer, 0)) return false;
  }

  // ifexists="path"
  if (pugi::xml_attribute conditionAttribute = node.attribute("ifexists"); conditionAttribute)
  {
    if (String path = mVariables.Resolve(conditionAttribute.as_string()); path.Trim().empty())
    { LOGT(LogError) << "[Theme] Empty path condition! Assuming true."; }
    else if (!Path(path).Exists()) return false;
  }

  // ifnotexists="path"
  if (pugi::xml_attribute conditionAttribute = node.attribute("ifnotexists"); conditionAttribute)
  {
    if (String path = mVariables.Resolve(conditionAttribute.as_string()); path.Trim().empty())
    { LOGT(LogError) << "[Theme] Empty path condition! Assuming true."; }
    else if (Path(path).Exists()) return false;
  }

  return true;
}

bool ThemeData::EvaluateExpression(SimpleTokenizer& tokenizer, int deepLevel) const
{
  bool result = false;
  bool inverter = false;
  for(SimpleTokenizer::Type previousType = tokenizer.TokenType();
      previousType != SimpleTokenizer::Type::End;
      previousType = tokenizer.Unary() ? previousType : tokenizer.TokenType())
    switch(tokenizer.Next())
    {
      case SimpleTokenizer::Type::Close:
      {
        if (deepLevel == 0)
        { LOGT(LogDebug) << "[Theme] Unexpected ) at index " << tokenizer.Index() << " in conditionnal string " << tokenizer.ParsedString(); }
        return result;
      }
      case SimpleTokenizer::Type::Open:
      case SimpleTokenizer::Type::Identifier:
      {
        // Evaluate token
        bool evaluated = tokenizer.TokenType() == SimpleTokenizer::Type::Identifier ?
                         EvaluateIdentifier(tokenizer) :
                         EvaluateExpression(tokenizer, deepLevel + 1);
        // Not?
        if (inverter) evaluated = !evaluated;
        // Apply operation
        if      (previousType == SimpleTokenizer::Type::And  ) result &= evaluated;
        else if (previousType == SimpleTokenizer::Type::Or   ) result |= evaluated;
        else if (previousType == SimpleTokenizer::Type::Start ||
                 previousType == SimpleTokenizer::Type::Open) result = evaluated;
        else { LOGT(LogDebug) << "[Theme] Syntax error at index " << tokenizer.Index() << " in conditional string " << tokenizer.ParsedString(); return false; }
        break;
      }
      case SimpleTokenizer::Type::And:
      case SimpleTokenizer::Type::Or:
      {
        if (previousType != SimpleTokenizer::Type::Identifier &&
            previousType != SimpleTokenizer::Type::Close)
        { LOGT(LogDebug) << "[Theme] Syntax error at index " << tokenizer.Index() << " in conditional string " << tokenizer.ParsedString(); return false; }
        inverter = false;
        break;
      }
      case SimpleTokenizer::Type::Not:
      {
        if (previousType != SimpleTokenizer::Type::And &&
            previousType != SimpleTokenizer::Type::Or &&
            previousType != SimpleTokenizer::Type::Start &&
            previousType != SimpleTokenizer::Type::Open)
        { LOGT(LogDebug) << "[Theme] Syntax error at index " << tokenizer.Index() << " in conditional string " << tokenizer.ParsedString(); return false; }
        inverter = !inverter;
        break;
      }
      case SimpleTokenizer::Type::Error:
      {
        { LOGT(LogDebug) << "[Theme] Syntax error at index " << tokenizer.Index() << " in conditional string " << tokenizer.ParsedString(); }
        return false;
      }
      case SimpleTokenizer::Type::Start:
      {
        { LOGT(LogDebug) << "[Theme] Internal logic error at index " << tokenizer.Index() << " in conditional string " << tokenizer.ParsedString(); }
        return false;
      }
      case SimpleTokenizer::Type::End:
      {
        if (deepLevel != 0)
        { LOGT(LogDebug) << "[Theme] Missing ) at index " << tokenizer.Index() << " in conditionnal string " << tokenizer.ParsedString(); }
        if (previousType != SimpleTokenizer::Type::Identifier &&
            previousType != SimpleTokenizer::Type::Close)
        { LOGT(LogDebug) << "[Theme] Missing operand at index " << tokenizer.Index() << " in conditionnal string " << tokenizer.ParsedString(); }
        break;
      }
    }
  // Hu?
  return result;
}

bool ThemeData::EvaluateIdentifier(const SimpleTokenizer& tokenizer) const
{
  bool evaluated = false;
  if      (tokenizer.Token() == "crt"       ) evaluated = mGlobalResolver.HasCrt();
  else if (tokenizer.Token() == "jamma"     ) evaluated = mGlobalResolver.HasJamma();
  else if (tokenizer.Token() == "overscan"  ) evaluated = mGlobalResolver.HasCrt() && !mGlobalResolver.HasJamma();
  else if (tokenizer.Token() == "tate"      ) evaluated = mGlobalResolver.IsTate();
  else if (tokenizer.Token() == "tateright" ) evaluated = mGlobalResolver.IsTateRight();
  else if (tokenizer.Token() == "tateleft"  ) evaluated = mGlobalResolver.IsTateLeft();
  else if (tokenizer.Token() == "qvga"      ) evaluated = mGlobalResolver.IsQVGA();
  else if (tokenizer.Token() == "vga"       ) evaluated = mGlobalResolver.IsVGA();
  else if (tokenizer.Token() == "hd"        ) evaluated = mGlobalResolver.IsHD();
  else if (tokenizer.Token() == "fhd"       ) evaluated = mGlobalResolver.IsFHD();
  else if (tokenizer.Token() == "virtual"   ) evaluated = mSystem != nullptr && mSystem->IsVirtual();
  else if (tokenizer.Token() == "arcade"    ) evaluated = mSystem != nullptr && mSystem->IsArcade();
  else if (tokenizer.Token() == "port"      ) evaluated = mSystem != nullptr && mSystem->IsMainPort();
  else if (tokenizer.Token() == "console"   ) evaluated = mSystem != nullptr && mSystem->Descriptor().Type() == SystemDescriptor::SystemType::Console;
  else if (tokenizer.Token() == "handheld"  ) evaluated = mSystem != nullptr && mSystem->Descriptor().Type() == SystemDescriptor::SystemType::Handheld;
  else if (tokenizer.Token() == "computer"  ) evaluated = mSystem != nullptr && mSystem->Descriptor().Type() == SystemDescriptor::SystemType::Computer;
  else if (tokenizer.Token() == "fantasy"   ) evaluated = mSystem != nullptr && mSystem->Descriptor().Type() == SystemDescriptor::SystemType::Fantasy;
  else if (tokenizer.Token() == "engine"    ) evaluated = mSystem != nullptr && mSystem->Descriptor().Type() == SystemDescriptor::SystemType::Engine;
  else if (tokenizer.Token() == "favorite"  ) evaluated = mSystem != nullptr && mSystem->IsFavorite();
  else if (tokenizer.Token() == "lastplayed") evaluated = mSystem != nullptr && mSystem->IsLastPlayed();
  else if (tokenizer.Token() == "bartop"    ) evaluated = mGlobalResolver.IsBartopModeOrHigher();
  else if (tokenizer.Token() == "nomenu"    ) evaluated = mGlobalResolver.IsNoMenuMode();
  else if (tokenizer.Token() == "ispc"      ) evaluated = mGlobalResolver.IsPC();
  else if (tokenizer.Token() == "ispi"      ) evaluated = mGlobalResolver.IsPi();
  else if (tokenizer.Token() == "isodroid"  ) evaluated = mGlobalResolver.IsOdroid();
  else if (tokenizer.Token() == "isanbernic") evaluated = mGlobalResolver.IsAnbernic();
  else { LOGT(LogDebug) << "[Theme] Unknown identifier " << tokenizer.Token() << " at index " << tokenizer.Index() << " in conditional string " << tokenizer.ParsedString() << ". False evaluation assumed."; }
  return evaluated;
}

ThemeData::Compatibility ThemeData::ExtractCompatibility(const String& comp)
{
  String compatibility(comp);

  Compatibility result = Compatibility::None;
  String item;
  for(;!compatibility.empty();)
  {
    if (!compatibility.Extract(',', item, compatibility, true))
    {
      item = compatibility.Trim();
      compatibility.clear();
    }
    if      (item == "hdmi" ) result |= Compatibility::Hdmi;
    else if (item == "jamma") result |= Compatibility::Jamma;
    else if (item == "crt"  ) result |= Compatibility::Crt;
    else if (item == "tate" ) result |= Compatibility::Tate;
  }
  if (result == Compatibility::None) result = Compatibility::Hdmi;
  return result;
}

ThemeData::Resolutions ThemeData::ExtractResolutions(const String& res)
{
  String resolutions(res);

  Resolutions result = Resolutions::None;
  String item;
  for(;!resolutions.empty();)
  {
    if (!resolutions.Extract(',', item, resolutions, true))
    {
      item = resolutions.Trim();
      resolutions.clear();
    }
    if      (item == "qvga") result |= Resolutions::QVGA;
    else if (item == "vga" ) result |= Resolutions::VGA;
    else if (item == "hd"  ) result |= Resolutions::HD;
    else if (item == "fhd" ) result |= Resolutions::FHD;
  }
  if (result == Resolutions::None) result = Resolutions::FHD | Resolutions::HD;
  return result;
}

ThemeData::Compatibility ThemeData::ExtractCompatibility(const pugi::xml_node& node)
{
  pugi::xml_attribute compatibilityNode = node.attribute("compatibility");
  if (!compatibilityNode) return Compatibility::Hdmi;

  Compatibility result = ExtractCompatibility(compatibilityNode.as_string());
  if (result == Compatibility::None) result = Compatibility::Hdmi;
  return result;
}

ThemeData::Resolutions ThemeData::ExtractResolutions(const pugi::xml_node& node)
{
  pugi::xml_attribute resolutionNode = node.attribute("resolutions");
  if (!resolutionNode) return Resolutions::FHD | Resolutions::HD;

  Resolutions result = ExtractResolutions(resolutionNode.as_string());
  if (result == Resolutions::None) result = Resolutions::FHD | Resolutions::HD;
  return result;
}

bool ThemeData::FetchCompatibility(const Path& path, [[out]] ThemeData::Compatibility& compatibility, [[out]] Resolutions& resolutions, String& name, String& author, int& version, int& recalboxVersion)
{
  if (!path.Exists()) { LOGT(LogError) << "[Theme] " << path << " does not exist!"; return false; }

  pugi::xml_document doc;
  pugi::xml_parse_result res = doc.load_string(Files::LoadFile(path).data());
  if (!res) { LOGT(LogError) << "[Theme] XML parsing error: \n    " + String(res.description()); return false; }

  pugi::xml_node root = doc.child("theme");
  if (!root) { LOGT(LogError) << "[Theme] Missing <theme> tag!"; return false; }

  // Extract compatibility
  compatibility = ExtractCompatibility(root);

  // Extract resolutions
  resolutions = ExtractResolutions(root);

  // Extract name
  name = path.Directory().FilenameWithoutExtension();
  pugi::xml_attribute nameAttribute = root.attribute("name");
  if ((bool)nameAttribute)
    if (String newName = nameAttribute.as_string(); !newName.Trim().empty())
      name = newName;

  // Extract name
  author = "";
  pugi::xml_attribute authorAttribute = root.attribute("author");
  if ((bool)authorAttribute)
    if (String newAuthor = authorAttribute.as_string(); !newAuthor.Trim().empty())
      author = newAuthor;

  // Extract version
  version = 0;
  pugi::xml_attribute versionAttribute = root.attribute("version");
  if ((bool)versionAttribute)
    if (String newVersion = versionAttribute.as_string(); !newVersion.Trim().empty())
    {
      if (String major, minor; newVersion.Extract('.', major, minor, true)) version = (major.AsInt() << 8) + minor.AsInt();
      else                                                                  version = newVersion.AsInt() << 8;
    }

  // Extract recalbox version
  recalboxVersion = 0;
  pugi::xml_attribute recalboxAttribute = root.attribute("recalbox");
  if ((bool)recalboxAttribute)
    if (String newRecalbox = recalboxAttribute.as_string(); !newRecalbox.Trim().empty())
    {
      if (String major, minor; newRecalbox.Extract('.', major, minor, true)) recalboxVersion = (major.AsInt() << 8) + minor.AsInt();
      else                                                                   recalboxVersion = newRecalbox.AsInt() << 8;
    }

  return true;
}

String ThemeData::GetResolutionList(ThemeData::Resolutions resolutions)
{
  String result;
  if ((resolutions & ThemeData::Resolutions::QVGA) != 0) result = "QVGA (240p)";
  if ((resolutions & ThemeData::Resolutions::VGA ) != 0) { if (!result.empty()) result.Append(','); result.Append("VGA (480p)"); }
  if ((resolutions & ThemeData::Resolutions::HD  ) != 0) { if (!result.empty()) result.Append(','); result.Append("HD (720p)"); }
  if ((resolutions & ThemeData::Resolutions::FHD ) != 0) { if (!result.empty()) result.Append(','); result.Append("FHD (1080p)"); }
  return result;
}

String ThemeData::GetDisplayList(ThemeData::Compatibility display)
{
  String result;
  if ((display & ThemeData::Compatibility::Hdmi ) != 0) result = "HDMI";
  if ((display & ThemeData::Compatibility::Crt  ) != 0) { if (!result.empty()) result.Append(','); result.Append("CRT"); }
  if ((display & ThemeData::Compatibility::Jamma) != 0) { if (!result.empty()) result.Append(','); result.Append("JAMMA"); }
  return result;
}

String::List ThemeData::GetSubsetList() const
{
  String::List result;
  for(const auto& kv : mSubSets)
    result.push_back(kv.first);
  std::ranges::sort(result);
  return result;
}

Array<ThemeOption*> ThemeData::GetSubsets() const
{
  Array<ThemeOption*> result;
  for(const auto& kv : mSubSets)
    result.Add(mSubSets.try_get(kv.first));
  result.Sort([] (ThemeOption* const& a, ThemeOption* const& b) { return a->Index() - b->Index(); });
  return result;
}

