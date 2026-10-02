#include "RecalboxConf.h"
#include <utils/Files.h>
#include <usernotifications/NotificationManager.h>

static Path recalboxConfFile("/recalbox/share/system/recalbox.conf");
static Path recalboxConfFileInit("/recalbox/share_init/system/recalbox.conf");

RecalboxConf::RecalboxConf()
  : IniFile(recalboxConfFile, recalboxConfFileInit, "RecalboxConf Class", false, true),
    StaticLifeCycleControler<RecalboxConf>("RecalboxConf")
{
}

void RecalboxConf::OnSave() const
{
  NotificationManager::Instance().Notify(Notification::ConfigurationChanged, recalboxConfFile.ToString());
}

String RecalboxConf::GetLanguage()
{
  String locale = RecalboxConf::Instance().GetSystemLanguage().LowerCase();
  return (locale.length() == 5) ? locale.SubString(0, 2) : "en";
}

String RecalboxConf::GetCountry()
{
  String locale = RecalboxConf::Instance().GetSystemLanguage().LowerCase();
  return (locale.length() == 5) ? locale.SubString(3, 2) : "us";
}

RecalboxConf::UpdateType RecalboxConf::UpdateTypeFromString(const String& ut)
{
  if (ut == "patron"            ) return UpdateType::Patron;
  if (ut == "patron-alpha"      ) return UpdateType::PatronAlpha;
  if (ut == "alpha"             ) return UpdateType::Alpha;
  if (ut == "rrgbd2"            ) return UpdateType::RRGBD2;
  if (ut == "jamma"             ) return UpdateType::RRGBD2;
  if (ut == "jvs"               ) return UpdateType::JVS;
  return UpdateType::Stable;
}

const String& RecalboxConf::UpdateTypeFromEnum(UpdateType type)
{
  switch(type)
  {
    case UpdateType::Patron: { static String s("patron"); return s; }
    case UpdateType::PatronAlpha: { static String s("patron-alpha"); return s; }
    case UpdateType::Alpha: { static String s("alpha"); return s; }
    case UpdateType::RRGBD2: { static String s("rrgbd2"); return s; }
    case UpdateType::JVS: { static String s("jvs"); return s; }
    case UpdateType::Stable:
    default: break;
  }
  static String s("stable");
  return s;
}

RecalboxConf::SoftPatching RecalboxConf::SoftPatchingFromString(const String& softpatching)
{
  if (softpatching == "auto") return SoftPatching::Auto;
  if (softpatching == "select") return SoftPatching::Select;
  if (softpatching == "launchLast") return SoftPatching::LaunchLast;

  return SoftPatching::Disable;
}

const String& RecalboxConf::SoftPatchingFromEnum(SoftPatching softpatching)
{
  switch(softpatching)
  {
    case SoftPatching::Auto: { static String s("auto"); return s; }
    case SoftPatching::Select: { static String s("select"); return s; }
    case SoftPatching::LaunchLast: { static String s("launchLast"); return s; }
    case SoftPatching::Disable:
    default: break;
  }
  static String s("disable");
  return s;
}

RecalboxConf::Transition RecalboxConf::TransitionFromString(const String& transition)
{
  if (transition == "fade") return Transition::Fade;
  if (transition == "instant") return Transition::Instant;

  return Transition::Slide;
}

const String& RecalboxConf::TransitionFromEnum(RecalboxConf::Transition transition)
{
  switch(transition)
  {
    case Transition::Fade: { static String s("fade"); return s; }
    case Transition::Instant: { static String s("instant"); return s; }
    case Transition::Slide:
    default: break;
  }
  static String s("slide");
  return s;
}

RecalboxConf::Screensaver RecalboxConf::ScreensaverFromString(const String& screensaver)
{
  if (screensaver == "black") return Screensaver::Black;
  if (screensaver == "demo") return Screensaver::Demo;
  if (screensaver == "gameclip") return Screensaver::Gameclip;
  if (screensaver == "suspend") return Screensaver::Suspend;
  if (screensaver == "bouncing") return Screensaver::Bouncing;

  return Screensaver::Dim;
}

const String& RecalboxConf::ScreensaverFromEnum(RecalboxConf::Screensaver screensaver)
{
  switch(screensaver)
  {
    case Screensaver::Demo: { static String s("demo"); return s; }
    case Screensaver::Gameclip: { static String s("gameclip"); return s; }
    case Screensaver::Suspend: { static String s("suspend"); return s; }
    case Screensaver::Black: { static String s("black"); return s; }
    case Screensaver::Bouncing: { static String s("bouncing"); return s; }
    case Screensaver::Dim:
    default: break;
  }
  static String s("dim");
  return s;
}

RecalboxConf::Menu RecalboxConf::MenuFromString(const String& menu)
{
  if (menu == "bartop") return Menu::Bartop;
  if (menu == "none") return Menu::None;
  return Menu::Default;
}

const String& RecalboxConf::MenuFromEnum(RecalboxConf::Menu menu)
{
  switch (menu)
  {
    case Menu::Bartop: { static String sBartop = "bartop"; return sBartop; }
    case Menu::None:   { static String sNone = "none"; return sNone; }
    case Menu::Default: default: break;
  }
  static String sDefault = "default";
  return sDefault;
}

RecalboxConf::Relay RecalboxConf::RelayFromString(const String& relay)
{
  if (relay == "nyc") return Relay::NewYork;
  if (relay == "madrid") return Relay::Madrid;
  if (relay == "montreal") return Relay::Montreal;
  if (relay == "saopaulo") return Relay::Saopaulo;
  return Relay::None;
}

const String& RecalboxConf::RelayFromEnum(RecalboxConf::Relay relay)
{
  switch (relay)
  {
    case Relay::NewYork:  { static String sNewYork = "nyc"; return sNewYork; }
    case Relay::Madrid:   { static String sMadrid = "madrid"; return sMadrid; }
    case Relay::Montreal: { static String sMontreal = "montreal"; return sMontreal; }
    case Relay::Saopaulo: { static String sSauPaulo = "saopaulo"; return sSauPaulo; }
    case Relay::None: default: break;
  }
  static String sDefault = "";
  return sDefault;
}

RecalboxConf::PadOSDType RecalboxConf::PadOSDTypeFromString(const String& pad)
{
  if (pad == "megadrive") return PadOSDType::MD;
  if (pad == "xbox") return PadOSDType::XBox;
  if (pad == "playstation") return PadOSDType::PSX;
  if (pad == "nintendo64") return PadOSDType::N64;
  if (pad == "dreamcast") return PadOSDType::DC;
  return PadOSDType::Snes;
}

const String& RecalboxConf::PadOSDTypeFromEnum(PadOSDType type)
{
  switch(type)
  {
    case PadOSDType::MD: { static String s = "megadrive"; return s; }
    case PadOSDType::XBox: { static String s = "xbox"; return s; }
    case PadOSDType::PSX: { static String s = "playstation"; return s; }
    case PadOSDType::N64: { static String s = "nintendo64"; return s; }
    case PadOSDType::DC: { static String s = "dreamcast"; return s; }
    case PadOSDType::Snes:
    default: break;
  }
  static String sDefault = "snes";
  return sDefault;
}

SystemSorting RecalboxConf::SystemSortingFromString(const String& systemSorting)
{
  if (systemSorting == "default")                        return SystemSorting::Default;
  if (systemSorting == "name")                           return SystemSorting::Name;
  if (systemSorting == "releasedate")                    return SystemSorting::ReleaseDate;
  if (systemSorting == "1type2name")                     return SystemSorting::SystemTypeThenName;
  if (systemSorting == "1type2releasedate")              return SystemSorting::SystemTypeThenReleaseDate;
  if (systemSorting == "1manufacturer2name")             return SystemSorting::ManufacturerThenName;
  if (systemSorting == "1manufacturer2releasedate")      return SystemSorting::ManufacturerThenReleaseData;
  if (systemSorting == "1type2manufacturer3name")        return SystemSorting::SystemTypeThenManufacturerThenName;
  if (systemSorting == "1type2manufacturer3releasedate") return SystemSorting::SystemTypeThenManufacturerThenReleasdeDate;
  return SystemSorting::Default;
}

const String& RecalboxConf::SystemSortingFromEnum(SystemSorting systemSorting)
{
  switch (systemSorting)
  {
    case SystemSorting::Default:                                    { static String string("default"); return string; }
    case SystemSorting::Name:                                       { static String string("name"); return string; }
    case SystemSorting::ReleaseDate:                                { static String string("releasedate"); return string; }
    case SystemSorting::SystemTypeThenName:                         { static String string("1type2name"); return string; }
    case SystemSorting::SystemTypeThenReleaseDate:                  { static String string("1type2releasedate"); return string; }
    case SystemSorting::ManufacturerThenName:                       { static String string("1manufacturer2name"); return string; }
    case SystemSorting::ManufacturerThenReleaseData:                { static String string("1manufacturer2releasedate"); return string; }
    case SystemSorting::SystemTypeThenManufacturerThenName:         { static String string("1type2manufacturer3name"); return string; }
    case SystemSorting::SystemTypeThenManufacturerThenReleasdeDate: { static String string("1type2manufacturer3releasedate"); return string; }
  }
  static String sDefault = "none";
  return sDefault;
}

ScraperType RecalboxConf::ScraperTypeFromString(const String& menu)
{
  if (menu == "ScreenScraper") return ScraperType::ScreenScraper;
  if (menu == "Recalbox")      return ScraperType::Recalbox;
  if (menu == "TheGameDB")     return ScraperType::TheGameDB;
  return ScraperType::ScreenScraper;
}

const String& RecalboxConf::ScraperTypeFromEnum(ScraperType type)
{
  static String defaultString("ScreenScraper");
  switch(type)
  {
    case ScraperType::ScreenScraper: { return defaultString; }
    case ScraperType::Recalbox: { static String string("Recalbox"); return string; }
    case ScraperType::TheGameDB: { static String string("TheGameDB"); return string; }
    default: break;
  }
  return defaultString;
}

RecalboxConf::GamelistDecoration RecalboxConf::GamelistDecorationFromString(const String& gameDecoration)
{
  // Default value when the key does not exist
  if (gameDecoration.ToTrim().empty()) return GamelistDecoration::Regions;

  GamelistDecoration result = GamelistDecoration::None;
  for(const String& item : gameDecoration.Split(','))
  {
    if (item == "types") result |= GamelistDecoration::SupportType;
    if (item == "supports") result |= GamelistDecoration::SupportNumber;
    if (item == "regions") result |= GamelistDecoration::Regions;
    if (item == "players") result |= GamelistDecoration::Players;
    if (item == "genre") result |= GamelistDecoration::Genre;
    if (item == "none") { result = GamelistDecoration::None; break; }
  }
  return result;
}

String RecalboxConf::GamelistDecorationFromEnum(RecalboxConf::GamelistDecoration gameDecoration)
{
  String result;
  if (hasFlag(gameDecoration, GamelistDecoration::SupportType)) result.Append("types");
  if (hasFlag(gameDecoration, GamelistDecoration::SupportNumber)) { if (!result.empty()) result.Append(','); result.Append("supports"); }
  if (hasFlag(gameDecoration, GamelistDecoration::Regions)) { if (!result.empty()) result.Append(','); result.Append("regions"); }
  if (hasFlag(gameDecoration, GamelistDecoration::Players)) { if (!result.empty()) result.Append(','); result.Append("players"); }
  if (hasFlag(gameDecoration, GamelistDecoration::Genre)) { if (!result.empty()) result.Append(','); result.Append("genre"); }
  if (gameDecoration == GamelistDecoration::None) result.Assign("none");
  return result;
}

RecalboxConf::GamelistLayout RecalboxConf::GamelistLayoutFromString(const String& ut)
{
  if (ut == "legacy"  ) return GamelistLayout::Legacy;
  if (ut == "logo"    ) return GamelistLayout::LogoList;
  if (ut == "carousel") return GamelistLayout::Carousel;
  if (ut == "grid"    ) return GamelistLayout::Grid;
  // No value: the system has no layout of its own and follows the global one
  return GamelistLayout::Default;
}

const String& RecalboxConf::GamelistLayoutFromEnum(GamelistLayout type)
{
  switch(type)
  {
    // Kept first: Carousel and Grid fall through to Legacy on purpose
    case GamelistLayout::Default:  { static String s; return s; }
    case GamelistLayout::LogoList: { static String s("logo"); return s; }
    case GamelistLayout::Carousel: //{ static String s("carousel"); return s; }
    case GamelistLayout::Grid:     //{ static String s("grid"); return s; }
    case GamelistLayout::Legacy:
    default: break;
  }
  static String s("legacy");
  return s;
}

AudioMode RecalboxConf::AudioModeFromString(const String& audioMode)
{
  if (audioMode == "musiconly") return AudioMode::MusicsOnly;
  if (audioMode == "videosoundonly") return AudioMode::VideosSoundOnly;
  if (audioMode == "musicandvideosound") return AudioMode::MusicsAndVideosSound;
  if (audioMode == "none") return AudioMode::None;
  return AudioMode::MusicsXorVideosSound;
}

const String& RecalboxConf::AudioModeFromEnum(AudioMode audioMode)
{
  switch (audioMode)
  {
    case AudioMode::MusicsXorVideosSound: break;
    case AudioMode::MusicsOnly:           { static String sScraper = "musiconly";           return sScraper; }
    case AudioMode::VideosSoundOnly:      { static String sFileName = "videosoundonly";     return sFileName; }
    case AudioMode::MusicsAndVideosSound: { static String sFileName = "musicandvideosound"; return sFileName; }
    case AudioMode::None:                 { static String sFileName = "none";               return sFileName;  }
  }
  static String sFileName = "musicxorvideosound";
  return sFileName;
}

RecalboxConf::VirtualKeyboardType RecalboxConf::VirtualKeyboardTypeFromString(const String& kind)
{
  if (kind == "classic") return VirtualKeyboardType::Classic;
  if (kind == "simplified") return VirtualKeyboardType::Simplified;
  return VirtualKeyboardType::Wheel;
}

const String& RecalboxConf::VirtualKeyboardTypeFromEnum(RecalboxConf::VirtualKeyboardType kind)
{
  switch (kind)
  {
    case VirtualKeyboardType::Classic: { static String sClassic = "classic"; return sClassic; }
    case VirtualKeyboardType::Simplified: { static String sSimplified = "simplified"; return sSimplified; }
    case VirtualKeyboardType::Wheel: default: break;
  }
  static String sWheel = "wheel";
  return sWheel;
}

DefineSystemGetterSetterImplementation(Emulator, String, String, sSystemEmulator, "", "")
DefineSystemGetterSetterImplementation(Core, String, String, sSystemCore, "", "")
DefineSystemGetterSetterImplementation(Ratio, String, String, sSystemRatio, GetGlobalRatio(), "")
DefineSystemGetterSetterImplementation(Smooth, bool, Bool, sSystemSmooth, GetGlobalSmooth(), false)
DefineSystemGetterSetterImplementation(Rewind, bool, Bool, sSystemRewind, GetGlobalRewind(), false)
DefineSystemGetterSetterImplementation(AutoSave, bool, Bool, sSystemAutoSave, GetGlobalAutoSave(), false)
DefineSystemGetterSetterImplementation(Shaders, String, String, sSystemShaders, GetGlobalShaders(), "")
DefineSystemGetterSetterImplementation(ShaderSet, String, String, sSystemShaderSet, GetGlobalShaderSet(), "")
DefineSystemGetterSetterImplementation(CrtHandheldFormat, String, String, sSystemCrtHandheldFormat, GetGlobalCrtHandheldFormat(), "")
DefineSystemGetterSetterImplementation(Ignore, bool, Bool, sSystemIgnore, false, false)
DefineSystemGetterSetterImplementation(DemoInclude, bool, Bool, sSystemDemoInclude, false, false)
DefineSystemGetterSetterImplementation(DemoDuration, int, Int, sSystemDemoDuration, GetGlobalDemoDuration(), 0)
DefineSystemGetterSetterImplementation(VideoMode, String, String, sSystemVideoMode, GetGlobalVideoMode(), "")

DefineEmulationStationSystemGetterSetterImplementation(FilterAdult, bool, Bool, sSystemFilterAdult, GetFilterAdultGames())
DefineEmulationStationSystemGetterSetterImplementation(FlatFolders, bool, Bool, sSystemFlatFolders, false)
DefineEmulationStationSystemGetterSetterNumericEnumImplementation(Sort, FileSorts::Sorts, sSystemSort, FileSorts::Sorts::FileNameAscending)
DefineEmulationStationSystemGetterSetterNumericEnumImplementation(RegionFilter, Regions::GameRegions, sSystemRegionFilter, Regions::GameRegions::Unknown)

DefineEmulationStationSystemListGetterSetterImplementation(ArcadeSystemHiddenManufacturers, sArcadeSystemHiddenManufacturers, "")

DefineEmulationStationSystemGetterSetterEnumImplementation(RecalboxConf, GamelistDecoration, GamelistDecoration, "decoration", GamelistDecoration)
DefineEmulationStationSystemGetterSetterEnumInheritedImplementation(RecalboxConf, GamelistLayout, GamelistLayout, "subtype", GamelistLayout, ResolvedGlobalGamelistLayout(), GamelistLayout::Default)

void RecalboxConf::Watch(const String& key, IRecalboxConfChanged& callback)
{
  mWatchers[key].Add(&callback);
}

// Global keys self-register into GlobalDispatchRegistry() at static-init through the setter macros,
// so this is a pure lookup -> typed setter (which fires watchers). No maintained list.
bool RecalboxConf::SetByKey(const String& key, const String& value)
{
  StringSetterThunk* thunk = GlobalDispatchRegistry().try_get(key);
  if (thunk == nullptr) return false;
  (*thunk)(value);
  return true;
}

// System keys self-register into SystemDispatchRegistry() at static-init through the *Implementation
// macros (namespace scope). The thunk resolves the system and calls the typed system setter.
bool RecalboxConf::SetSystemByKey(SystemData& system, const String& shortKey, const String& value)
{
  SystemSetterThunk* thunk = SystemDispatchRegistry().try_get(shortKey);
  if (thunk == nullptr) return false;
  (*thunk)(system, value);
  return true;
}

