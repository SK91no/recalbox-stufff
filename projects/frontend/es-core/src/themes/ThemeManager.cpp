//
// Created by bkg2k on 12/12/23.
//

#include "ThemeManager.h"
#include "RootFolders.h"
#include "guis/GuiSyncWaiter.h"
#include "utils/locale/LocaleHelper.h"
#include "guis/GuiMsgBox.h"
#include "emulators/EmulatorManager.h"
#include <systems/SystemData.h>
#include "EmulationStation.h"
#include "Upgrade.h"
#include "systems/SystemManager.h"
#include <hardware/Board.h>

ThemeManager::ThemeManager(IGlobalVariableResolver& globalResolver)
  : StaticLifeCycleControler<ThemeManager>("theme-manager")
  , mMain(mCache, nullptr, globalResolver, *this)
  , mMenuCache(mMenu)
  , mVersion(0)
  , mRecalboxVersion(0)
  , mCompatibility(ThemeData::Compatibility::None)
  , mResolutions(ThemeData::Resolutions::None)
  , mWaiter(nullptr)
  , mGlobalResolver(globalResolver)
{
  srandom(SDL_GetTicks());
  ThemeSupport::InitializeStatics();
}

void ThemeManager::Initialize(WindowManager* window)
{
  DoThemeChange(window);
}

ThemeManager::~ThemeManager()
{
  for(auto& kv : mSystem)
    delete kv.second;
}

Path ThemeManager::GetThemeRootPath()
{
  // Check theme validity
  String theme = RecalboxConf::Instance().GetThemeFolder();
  ThemeList list = AvailableThemes();
  if (!list.contains(theme))
    RecalboxConf::Instance().SetThemeFolder(list.begin()->second.FilenameWithoutExtension());

  return list[RecalboxConf::Instance().GetThemeFolder()];
}

void ThemeManager::LoadMainTheme()
{
  // Locate main theme file
  Path root;
  mRootPath = GetThemeRootPath();
  if (root = mRootPath / sRootThemeFile; !root.Exists())
  {
    // Try locating in subfolders
    for(const Path& sub : mRootPath.GetDirectoryContent(true))
      if ((sub / sRootThemeFile).Exists())
      {
        root = sub;
        break;
      }
  }

  mMain.Reset();
  ThemeData::FetchCompatibility(root, mCompatibility, mResolutions, mName, mAuthor, mVersion, mRecalboxVersion);
  mMain.LoadMain(root);
  mMenu.Load(mMain);
}

void ThemeManager::LoadSystemTheme(const SystemData& system)
{
  Path systemPath;
  // Check root/<system>/theme.xml first
  if (systemPath = mRootPath / system.Descriptor().ThemeFolder() / sRootThemeFile; !systemPath.Exists())
    // Then check root/default/theme.xml
    if (systemPath = mRootPath / sDefaultSystemThemeFolder / sRootThemeFile; !systemPath.Exists())
      // Then check root/theme.xml
      systemPath = mRootPath / sRootThemeFile;

  ThemeData& systemTheme = CreateOrGetSystem(&system);
  systemTheme.Reset();
  systemTheme.LoadSystem(system.Descriptor().ThemeFolder(), systemPath);
}

const SystemData* ThemeManager::ThreadPoolRunJob(const SystemData*& feed)
{
  LoadSystemTheme(*feed);
  return feed;
}

void ThemeManager::DoThemeChange(WindowManager* window, bool force)
{
  DateTime start;
  Path newPath = GetThemeRootPath();
  if (mRootPath.IsEmpty()) { LOGT(LogInfo) << "[ThemeManager] Loading initial theme: " << newPath; }
  else if (newPath != mRootPath) { LOGT(LogInfo) << "[ThemeManager] Switching to new theme: " << newPath; }
  else { LOGT(LogInfo) << "[ThemeManager] Current theme options havre changed. Refreshing theme: " << newPath; }

  //! Clear cache if required
  bool update = (newPath == mRootPath);
  if (!update || force) mCache.Clear();

  if (!mRootPath.IsEmpty())
  {
    mWaiter = new GuiSyncWaiter(*window, update ? _("Updating current theme...") :
                               (_F(_("Loading new theme {0}")) / newPath.FilenameWithoutExtension()).ToString());
    mWaiter->Show();
    mWaitBarReference = DateTime();
  }

  // Reset the shared pools: all previous string/PropertyBag indices become stale, but every
  // ThemeData is Reset and re-parsed below before anything reads them again.
  mStringPool.Reset();
  mBagPool.Reset();

  // Reload Main themes
  LoadMainTheme();
  // Reload system themes
  if (!mSystem.empty())
  {
    ThreadPool<const SystemData*, const SystemData*> pool(this, "theme-loader", false);
    for (auto& kv: mSystem) pool.PushFeed(kv.first);
    pool.Run(-2, false);
  }
  DateTime start2;
  { LOGT(LogWarning) << "[ThemeManager] Load time: " << (start2 - start).ToMillisecondsString() << " ms"; }
  // Refresh
  NotifyThemeChanged(update);
  { LOGT(LogWarning) << "[ThemeManager] Refresh time: " << (DateTime() - start2).ToMillisecondsString() << " ms"; }

  if (mWaiter != nullptr)
  {
    mWaiter->Hide();
    delete mWaiter;
    mWaiter = nullptr;
  }
}

void ThemeManager::ThemeSwitchTick()
{
  if (mWaiter != nullptr)
    if (DateTime now; (now - mWaitBarReference).TotalMilliseconds() > 300)
    {
      mWaiter->Refresh();
      mWaitBarReference = now;
    }
}

void ThemeManager::NotifyThemeChanged(bool refreshOnly)
{
  for(IThemeSwitchable* switchable : mSwitchables)
    if (switchable != nullptr)
    {
      if (SystemData* system = switchable->SystemTheme(); system != nullptr)
        switchable->SwitchToTheme(CreateOrGetSystem(system), refreshOnly, this);
      else
        switchable->SwitchToTheme(mMain, refreshOnly, this);
    }
}

ThemeData& ThemeManager::CreateOrGetSystem(const SystemData* system)
{
  Mutex::AutoLock locker(mSystemLocker);
  ThemeData** theme = mSystem.try_get(system);
  if (theme == nullptr) theme = &(mSystem[system] = new ThemeData(mCache, system, mGlobalResolver, *this));
  return **theme;
}

HashMap<String, Path> ThemeManager::AvailableThemes()
{
  HashMap<String, Path> sets;

  static Path paths[] =
    {
      RootFolders::TemplateRootFolder / "/system/.emulationstation/themes",
      RootFolders::DataRootFolder     / "/themes",
      RootFolders::DataRootFolder     / "/system/.emulationstation/themes"
    };

  for (const Path& master : paths)
  {
    Path::PathList list = master.GetDirectoryContent();
    for(const Path& path : list)
      if (path.IsDirectory() && (path / sRootThemeFile).Exists())
      {
        String name = path.FilenameWithoutExtension();
        while(sets.contains(name))
          if (name.Count() >= 3 && name[name.Count() - 2] == '#' && name[name.Count() - 3] == ' ') name[name.Count()-2]++;
          else name.Append(" #2");
        sets[name] = path;
      }
  }

  return sets;
}

bool ThemeManager::AnalyseAndSwitch(WindowManager& window, const Path& themePath, [[out]] String& name, bool switchTheme, int* rawThemeVersion)
{
  // Current state
  bool tate = mGlobalResolver.IsTate();
  ThemeData::Compatibility currentCompatibility = (mGlobalResolver.HasJamma() ? ThemeData::Compatibility::Jamma : ThemeData::Compatibility::None) |
                                                  (mGlobalResolver.HasCrt() ? ThemeData::Compatibility::Crt : ThemeData::Compatibility::None) |
                                                  (mGlobalResolver.HasHDMI() ? ThemeData::Compatibility::Hdmi : ThemeData::Compatibility::None);
  ThemeData::Resolutions currentResolution = mGlobalResolver.IsQVGA() ? ThemeData::Resolutions::QVGA :
                                             mGlobalResolver.IsVGA() ? ThemeData::Resolutions::VGA :
                                             mGlobalResolver.IsHD() ? ThemeData::Resolutions::HD :
                                             ThemeData::Resolutions::FHD;

  // Fetched data
  int recalboxVersion = 0; // Unknown version
  int themeVersion = 0; // Unknown version
  ThemeData::Compatibility compatibility = ThemeData::Compatibility::Hdmi;
  ThemeData::Resolutions resolutions = ThemeData::Resolutions::HD | ThemeData::Resolutions::FHD;
  String displayableName;
  String author;

  if (ThemeData::FetchCompatibility(themePath / ThemeManager::sRootThemeFile, compatibility, resolutions, displayableName, author, themeVersion, recalboxVersion))
  {
    bool compatible = ((currentCompatibility & compatibility) != 0) &&
                      ((currentResolution & resolutions) != 0) &&
                      (!tate || hasFlag(compatibility, ThemeData::Compatibility::Tate)) &&
                      (recalboxVersion >= sRecalboxMinimumCompatibilityVersion);
    if (switchTheme)
    {
      if (!compatible)
      {
        String modeIssue = (currentCompatibility & compatibility) == 0 ?
                           String("- ").Append(_("You display {0} is not in the list of this theme's supported displays:"))
                                       .Replace("{0}",ThemeData::GetDisplayList(currentCompatibility))
                                       .Append(' ').Append(ThemeData::GetDisplayList(compatibility)).Append(String::CRLF) : String::Empty;
        String resolutionIssue = (currentResolution & resolutions) == 0 ?
                                 String("- ").Append(_("You current resolution {0} is not in the list of this theme's supported resolutions:"))
                                             .Replace("{0}",ThemeData::GetResolutionList(currentResolution))
                                             .Append(' ').Append(ThemeData::GetResolutionList(resolutions)).Append(String::CRLF) : String::Empty;
        String tateIssue = tate && !hasFlag(compatibility, ThemeData::Compatibility::Tate) ?
                           String("- ").Append(_("You're in TATE mode and this theme does not seem to support TATE.")).Append(String::CRLF) : String::Empty;
        String versionIssue = (recalboxVersion < sRecalboxMinimumCompatibilityVersion) ?
                              String("- ").Append(_("The theme version is too old and the theme may not work properly.")).Append(String::CRLF) : String::Empty;
        String message = _("This theme may have one or more compatibility issues with your current display:\n").Append(modeIssue).Append(resolutionIssue).Append(tateIssue).Append(versionIssue).Append(String::CRLF).Append(_("Are you sure to activate this theme?"));
        String themeName = themePath.Filename();
        window.pushGui(new GuiMsgBox(window, message, _("YES"), [&window, themeName]{ RecalboxConf::Instance().SetThemeFolder(themeName).Save(); ThemeManager::Instance().DoThemeChange(&window); }, _("NO"), {}));
      }
      else
      {
        RecalboxConf::Instance().SetThemeFolder(themePath.Filename()).Save();
        DoThemeChange(&window);
      }
    }
    if (themeVersion != 0) displayableName.Append(" (").Append(themeVersion >> 8).Append('.').Append(themeVersion & 0xFF).Append(')');

    name = displayableName;
    if (rawThemeVersion != nullptr) *rawThemeVersion = themeVersion;
    return compatible;
  }

  name = "<not a theme>";
  return false;
}

const HashMap<String, ThemeManager::ThemeContextGlobalResolver>& ThemeManager::GetThemeContextGlobalResolverMap()
{
  static const HashMap<String, ThemeManager::ThemeContextGlobalResolver> sMap
  {
    { "root",               [](String& string, IGlobalVariableResolver& /*resolver*/) { string = ThemeManager::Instance().mRootPath.ToString(); } },
    { "recalbox.version",   [](String& string, IGlobalVariableResolver& /*resolver*/) { string = Upgrade::CurrentVersion(); } },
    { "recalbox.built",     [](String& string, IGlobalVariableResolver& /*resolver*/) { string = PROGRAM_BUILT_STRING; } },
    { "settings.language",  [](String& string, IGlobalVariableResolver& /*resolver*/) { string = RecalboxConf::Instance().GetSystemLanguage(); if (int p = string.Find('_'); p > 0) string.Delete(p, string.Count() - p); } },
    { "settings.locale",    [](String& string, IGlobalVariableResolver& /*resolver*/) { string = RecalboxConf::Instance().GetSystemLanguage(); } },
    { "settings.region",    [](String& string, IGlobalVariableResolver& /*resolver*/) { string = RecalboxConf::Instance().GetThemeRegion(); } },
    { "settings.timezone",  [](String& string, IGlobalVariableResolver& /*resolver*/) { string = RecalboxConf::Instance().GetSystemTimezone(); } },
    { "hardware.board",     [](String& string, IGlobalVariableResolver& /*resolver*/) { string = Board::GetBoardName(); } },
    { "hardware.crt",       [](String& string, IGlobalVariableResolver& resolver) { string = resolver.HasCrt() ? "yes" : "no"; } },
    { "hardware.jamma",     [](String& string, IGlobalVariableResolver& resolver) { string = resolver.HasJamma() ? "yes" : "no"; } },
    { "hardware.ispc",      [](String& string, IGlobalVariableResolver& resolver) { string = resolver.IsPC() ? "yes" : "no"; } },
    { "hardware.ispi",      [](String& string, IGlobalVariableResolver& resolver) { string = resolver.IsPi() ? "yes" : "no"; } },
    { "hardware.isodroid",  [](String& string, IGlobalVariableResolver& resolver) { string = resolver.IsOdroid() ? "yes" : "no"; } },
    { "hardware.isanbernic",[](String& string, IGlobalVariableResolver& resolver) { string = resolver.IsAnbernic() ? "yes" : "no"; } },
    { "display.tate",       [](String& string, IGlobalVariableResolver& resolver) { string = resolver.IsTate() ? "yes" : "no"; } },
    { "display.tateleft",   [](String& string, IGlobalVariableResolver& resolver) { string = resolver.IsTateLeft() ? "yes" : "no"; } },
    { "display.tateright",  [](String& string, IGlobalVariableResolver& resolver) { string = resolver.IsTateRight() ? "yes" : "no"; } },
    { "display.overscan",   [](String& string, IGlobalVariableResolver& resolver) { string = resolver.HasCrt() && !resolver.HasJamma() ? "yes" : "no"; } },
    { "display.resolution", [](String& string, IGlobalVariableResolver& /*resolver*/) { int height = Renderer::Instance().RealDisplayHeightAsInt(); string = height > 920 ? "fhd" : height > 576 ? "hd" : height > 288 ? "vga" : "qvga"; } },
  };
  return sMap;
}

void ThemeManager::RandomBetween(String& string, const String::List& params)
{
  if (params.empty()) { LOG(LogError) << "[ThemeManager] Empty list in random.between: " << string; string.clear(); }
  else string = params[random() % params.size()];
}

void ThemeManager::RandomRange(String& string, const String::List& params)
{
  if (params.size() < 2) { LOG(LogError) << "[ThemeManager] Not enough parameters in random.range : " << string; string.clear(); }
  else if (int min; !params[0].TryAsInt(min)) { LOG(LogError) << "[ThemeManager] Lower value is not an integer in random.range : " << string; string.clear(); }
  else if (int max; !params[1].TryAsInt(max)) { LOG(LogError) << "[ThemeManager] higher value is not an integer in random.range : " << string; string.clear(); }
  else if (max <= min) { LOG(LogError) << "[ThemeManager] min value must be lower than the max valuer in random.range : " << string; string.clear(); }
  else string.Assign((int)(random() % (max - min + 1)) + min);
}

const HashMap<String, ThemeManager::ThemeContextGlobalParameterizedResolver>& ThemeManager::GetThemeContextGlobalParameterizedResolverMap()
{
  static const HashMap<String, ThemeManager::ThemeContextGlobalParameterizedResolver> sMap
    {
      { "random.between", RandomBetween },
      { "random.range",   RandomRange },
    };
  return sMap;
}

bool ThemeManager::GetDefaultSystemLogo(const SystemData& system, Path& path)
{
  // Full Localized logo
  RecalboxConf& conf = RecalboxConf::Instance();
  path = Path("/recalbox/system/resources/logos") / (String(system.Name()).Append('-').Append(conf.GetSystemLanguage()).Append(".svg"));
  bool result = path.Exists();
  // Language only Localized logo
  if (!result)
  {
    path = Path("/recalbox/system/resources/logos") / (String(system.Name()).Append('-').Append(conf.GetSystemLanguage().SubString(0, 2)).Append(".svg"));
    // Regionalized logo
    if (result = path.Exists();!result)
    {
      path = Path("/recalbox/system/resources/logos") / (String(system.Name()).Append('-').Append(conf.GetThemeRegion()).Append(".svg"));
      // Default logo
      if (result = path.Exists();!result)
      {
        path = Path("/recalbox/system/resources/logos") / (String(system.Name()).Append(".svg"));
        result = path.Exists();
      }
    }
  }
  return result;
}

const HashMap<String, ThemeManager::ThemeContextDisplayedSystemResolver>& ThemeManager::GetThemeContextDisplayedSystemResolverMap()
{
  static const HashMap<String, ThemeManager::ThemeContextDisplayedSystemResolver> sMap
  {
    { "system",                [](String& string, const SystemData& system) { string = system.FullName(); } },
    { "system.logo",           [](String& string, const SystemData& system) { Path path; ThemeManager::GetDefaultSystemLogo(system, path); string = path.ToString(); } },
    { "system.name",           [](String& string, const SystemData& system) { string = system.Name(); } },
    { "system.manufacturer",   [](String& string, const SystemData& system) { string = system.Descriptor().Manufacturer(); } },
    { "system.releasedate",    [](String& string, const SystemData& system) { string = system.Descriptor().ReleaseDate().ToStringFormat("%YYYY-%MM"); } },
    { "system.type",           [](String& string, const SystemData& system) { string = SystemDescriptor::ConvertSystemTypeToString(system.Descriptor().Type()); } },
    { "system.type.name",      [](String& string, const SystemData& system) { string = SystemDescriptor::TypeFriendlyName(system.Descriptor().Type()); } },
    { "system.input.pad",      [](String& string, const SystemData& system) { string = SystemDescriptor::ConvertDeviceRequirementToString(system.Descriptor().PadRequirement()); } },
    { "system.input.mouse",    [](String& string, const SystemData& system) { string = SystemDescriptor::ConvertDeviceRequirementToString(system.Descriptor().MouseRequirement()); } },
    { "system.input.keyboard", [](String& string, const SystemData& system) { string = SystemDescriptor::ConvertDeviceRequirementToString(system.Descriptor().KeyboardRequirement()); } },
  };
  return sMap;
}

const HashMap<String, ThemeManager::ThemeContextGameAndSystemResolver>& ThemeManager::GetThemeContextGameAndSystemResolverMap()
{
  static const HashMap<String, ThemeManager::ThemeContextGameAndSystemResolver> sMap
  {
    { "game.system",                   [](String& string, const FileData& game) { string = game.System().FullName(); } },
    { "game.system.logo",              [](String& string, const FileData& game) { Path path; ThemeManager::GetDefaultSystemLogo(game.System(), path); string = path.ToString(); } },
    { "game.system.name",              [](String& string, const FileData& game) { string = game.System().Name(); } },
    { "game.system.manufacturer",      [](String& string, const FileData& game) { string = game.System().Descriptor().Manufacturer(); } },
    { "game.system.releasedate",       [](String& string, const FileData& game) { string = game.System().Descriptor().ReleaseDate().ToStringFormat("%YYYY-%MM"); } },
    { "game.system.type",              [](String& string, const FileData& game) { string = SystemDescriptor::ConvertSystemTypeToString(game.System().Descriptor().Type()); } },
    { "game.system.type.name",         [](String& string, const FileData& game) { string = SystemDescriptor::TypeFriendlyName(game.System().Descriptor().Type()); } },
    { "game.system.input.pad",         [](String& string, const FileData& game) { string = SystemDescriptor::ConvertDeviceRequirementToString(game.System().Descriptor().PadRequirement()); } },
    { "game.system.input.mouse",       [](String& string, const FileData& game) { string = SystemDescriptor::ConvertDeviceRequirementToString(game.System().Descriptor().MouseRequirement()); } },
    { "game.system.input.keyboard",    [](String& string, const FileData& game) { string = SystemDescriptor::ConvertDeviceRequirementToString(game.System().Descriptor().KeyboardRequirement()); } },
    { "game.file.path",                [](String& string, const FileData& game) { string = game.Metadata().Rom().ToString(); } },
    { "game.file.name",                [](String& string, const FileData& game) { string = game.Metadata().RomFileOnly().ToString(); } },
    { "game.file.stem",                [](String& string, const FileData& game) { string = game.Metadata().RomFileOnly().FilenameWithoutExtension(); } },
    { "game.name",                     [](String& string, const FileData& game) { string = game.System().IsTrueArcade() ? game.System().LookupArcadeName(&game) : game.Metadata().Name(); } },
    { "game.license",                  [](String& string, const FileData& game) { string = game.Metadata().Licences(); } },
    { "game.developer",                [](String& string, const FileData& game) { string = game.Metadata().Developer(); if (string.empty()) string = _("UNKNOWN"); } },
    { "game.publisher",                [](String& string, const FileData& game) { string = game.Metadata().Publisher(); if (string.empty()) string = _("UNKNOWN"); } },
    { "game.synopsis",                 [](String& string, const FileData& game) { string = game.Metadata().Description(); } },
    { "game.releasedate",              [](String& string, const FileData& game) { string = game.Metadata().ReleaseDateEpoc() != 0 ? game.Metadata().ReleaseDate().ToDateISO8601() : _("UNKNOWN"); } },
    { "game.players",                  [](String& string, const FileData& game) { string = game.Metadata().PlayersAsString(); } },
    { "game.players.min",              [](String& string, const FileData& game) { string = game.Metadata().PlayerMin(); } },
    { "game.players.max",              [](String& string, const FileData& game) { string = game.Metadata().PlayerMax(); } },
    { "game.genre.raw",                [](String& string, const FileData& game) { string = game.Metadata().Genre(); } },
    { "game.genre.normalized",         [](String& string, const FileData& game) { string = Genres::GetFullNameStandalone(game.Metadata().GenreId()); } },
    { "game.lastplayed",               [](String& string, const FileData& game) { string = game.Metadata().LastPlayedEpoc() != 0 ? game.Metadata().LastPlayed().ToDateISO8601() : _("NEVER"); } },
    { "game.timesplayed",              [](String& string, const FileData& game) { string = game.Metadata().PlayCountAsString(); } },
    { "game.totalplayed",              [](String& string, const FileData& game) { string = game.Metadata().TotalPlayTime() != 0 ? TimeSpan(game.Metadata().TotalPlayTime()).ToTimeString(false) : _("NONE"); } },
    { "game.rating.5",                 [](String& string, const FileData& game) { string = (int)(game.Metadata().Rating() * 5); } },
    { "game.rating.10",                [](String& string, const FileData& game) { string = (int)(game.Metadata().Rating() * 10); } },
    { "game.rating.100",               [](String& string, const FileData& game) { string = (int)(game.Metadata().Rating() * 100); } },
    { "game.support.type",             [](String& string, const FileData& game) { string = game.Metadata().SupportTypeAsString(); } },
    { "game.support.index",            [](String& string, const FileData& game) { string = game.Metadata().SupportIndex() != 0 ? game.Metadata().SupportIndexAsString() : String::Empty; } },
    { "game.support.side",             [](String& string, const FileData& game) { string = game.Metadata().SupportSideAsString(); } },
    { "game.support.total",            [](String& string, const FileData& game) { if (game.Metadata().SupportTotal() != 0) string = game.Metadata().SupportTotalAsString(); else string.Assign("UNKNOWN"); } },
    { "game.support.number",           [](String& string, const FileData& game) { String s; if (game.Metadata().SupportIndex() != 0) s = game.Metadata().SupportIndex(); if (game.Metadata().SupportSide() != SupportSides::None) s.Append(SupportSideToString(game.Metadata().SupportSide())); if (game.Metadata().SupportTotal() != 0) s.Append('/').Append(game.Metadata().SupportTotal()); string = s; } },
    { "game.isadult",                  [](String& string, const FileData& game) { string = game.Metadata().Adult() ? "yes" : "no"; } },
    { "game.ishidden",                 [](String& string, const FileData& game) { string = game.Metadata().Hidden() ? "yes" : "no"; } },
    { "game.isfavorite",               [](String& string, const FileData& game) { string = game.Metadata().Favorite() ? "yes" : "no"; } },
    { "game.islastversion",            [](String& string, const FileData& game) { string = game.Metadata().LatestVersion() ? "yes" : "no"; } },
    { "game.ispreinstalled",           [](String& string, const FileData& game) { string = game.Metadata().Preinstalled() ? "yes" : "no"; } },
    { "game.isnotagame",               [](String& string, const FileData& game) { string = game.Metadata().NoGame() ? "yes" : "no"; } },
    { "game.media.imagepath",          [](String& string, const FileData& game) { string = game.Metadata().Image().ToString(); } },
    { "game.media.boxpath",            [](String& string, const FileData& game) { string = game.Metadata().Box().ToString(); } },
    { "game.media.thumbpath",          [](String& string, const FileData& game) { string = game.Metadata().Image().ToString(); } },
    { "game.media.videopath",          [](String& string, const FileData& game) { string = game.Metadata().Video().ToString(); } },
    { "game.emulator.name",            [](String& string, const FileData& game) { EmulatorData data = EmulatorManager::GetGameEmulatorCached(game); string = data.Core() == data.Emulator() ? data.Core() : String(data.Emulator()).Append(' ').Append(data.Core()); } },
    { "game.emulator.islibretro",      [](String& string, const FileData& game) { EmulatorData data = EmulatorManager::GetGameEmulatorCached(game); string = data.IsLibretro() ? "yes" : "no"; } },
    { "game.emulator.hasnetplay",      [](String& string, const FileData& game) { EmulatorData data = EmulatorManager::GetGameEmulatorCached(game); string = data.CoreInfo().Netplay() ? "yes" : "no"; } },
    { "game.emulator.hassoftpatching", [](String& string, const FileData& game) { EmulatorData data = EmulatorManager::GetGameEmulatorCached(game); string = data.CoreInfo().Softpatching() ? "yes" : "no"; } },
    { "game.emulator.extensions",      [](String& string, const FileData& game) { EmulatorData data = EmulatorManager::GetGameEmulatorCached(game); string = data.CoreInfo().Extensions(); } },
    { "game.emulator.compatibility",   [](String& string, const FileData& game) { EmulatorData data = EmulatorManager::GetGameEmulatorCached(game); string = data.CoreInfo().CompatibilityRateAsString(); } },
    { "game.emulator.speed",           [](String& string, const FileData& game) { EmulatorData data = EmulatorManager::GetGameEmulatorCached(game); string = data.CoreInfo().AverageSpeedAsString(); } },
  };
  return sMap;
}

ThemeContexts ThemeManager::AnalyseContextualVariables(const String& source)
{
  ThemeContexts result = ThemeContexts::None;
  for(int start = 0;;)
  {
    int left = source.Find("${", 2, start); if (left < 0) break;
    int right = source.Find('}', left + 2); if (right < left) break;
    String variable = source.SubString(left + 2, right - (left + 2)).Trim().LowerCase();
    bool hasOffset = GetSystemOffset(variable);
    if (GetThemeContextGlobalResolverMap().contains(variable)) result |= ThemeContexts::Global;
    else if (HasParameter(variable, true) && GetThemeContextGlobalParameterizedResolverMap().contains(variable)) result |= ThemeContexts::Global;
    else if (GetThemeContextDisplayedSystemResolverMap().contains(variable)) result |= hasOffset ? ThemeContexts::DisplayedSystem : ThemeContexts::DisplayedSystemOffset;
    else if (GetThemeContextGameAndSystemResolverMap().contains(variable)) result |= ThemeContexts::GameAndSystem;
    else result |= ThemeContexts::UserVariable;
    start = right + 1;
  }
  return result;
}

void ThemeManager::ResolveContextualVariables(ThemeContexts contextSensibility, String& source, const SystemData* displayedSystem,
                                                 const FileData* displayedGame)
{
  for(int start = 0;;)
  {
    int left = source.Find("${", 2, start); if (left < 0) break;
    int right = source.Find('}', left + 2); if (right < left) break;
    String variable = source.SubString(left + 2, right - (left + 2)).Trim().LowerCase();
    bool replaced = false;
    for(;;)
    {
      if (hasFlag(contextSensibility, ThemeContexts::Global))
      {
        if (HasParameter(variable, false))
          if (String::List parameters; ExtractParameters(variable, parameters))
            if (ThemeContextGlobalParameterizedResolver* global = GetThemeContextGlobalParameterizedResolverMap().try_get(variable); global != nullptr)
            { (*global)(variable, parameters); replaced = true; break; }
        if (ThemeContextGlobalResolver* global = GetThemeContextGlobalResolverMap().try_get(variable); global != nullptr)
        { (*global)(variable, mGlobalResolver); replaced = true; break; }
      }
      if (hasFlag(contextSensibility, ThemeContexts::DisplayedSystem))
        if (displayedSystem != nullptr)
          if (ThemeContextDisplayedSystemResolver* system = GetThemeContextDisplayedSystemResolverMap().try_get(variable); system != nullptr)
          { (*system)(variable, *displayedSystem); replaced = true; break; }
      if (hasFlag(contextSensibility, ThemeContexts::DisplayedSystemOffset))
        if (displayedSystem != nullptr)
        {
          displayedSystem = ApplySystemOffset(variable, displayedSystem);
          if (ThemeContextDisplayedSystemResolver* system = GetThemeContextDisplayedSystemResolverMap().try_get(variable); system != nullptr)
          { (*system)(variable, *displayedSystem); replaced = true; break; }
        }
      if (hasFlag(contextSensibility, ThemeContexts::GameAndSystem))
        if (displayedGame != nullptr)
          if (ThemeContextGameAndSystemResolver* game = GetThemeContextGameAndSystemResolverMap().try_get(variable); game != nullptr)
          { (*game)(variable, *displayedGame); replaced = true; break; }
      break;
    }
    if (replaced) source.Delete(left, right - left + 1).Insert(left, variable);
    start = left + variable.Count();
  }
}

void ThemeManager::ChangeThemeContext(const String& view, ThemeExtras::List& list, SystemData* displayedSystem, FileData* gameDisplayed)
{
  ThemeData& theme = displayedSystem->Theme();
  theme.VariableHolder().SetGameContext(gameDisplayed);
  theme.RefreshContextualExtraProperties(list, view);
}

const SystemData* ThemeManager::ApplySystemOffset(String& variable, const SystemData* system)
{
  int pos = variable.Find('+'); if (pos < 0) pos = variable.Find('-');
  if (pos > 0)
    if (int offset = 0; variable.TryAsInt(pos + 1, offset))
      if (offset != 0)
      {
        if (variable[pos] == '-') offset = -offset;
        variable.Delete(pos, variable.Count() - pos);
        const SystemManager::List& list = system->Manager().VisibleSystemList();
        if (pos = list.IndexOf((SystemData*)system); pos >= 0)
        {
          pos += offset;
          while(pos > list.Count()) pos -= list.Count();
          while(pos < 0) pos += list.Count();
          return list[pos];
        }
      }
  return system;
}

bool ThemeManager::GetSystemOffset(String& variable)
{
  int pos = variable.Find('+'); if (pos < 0) pos = variable.Find('-');
  if (pos > 0)
    if (int offset = 0; variable.TryAsInt(pos + 1, offset))
    {
      variable.Delete(pos, variable.Count() - pos);
      return true;
    }
  return false;
}

bool ThemeManager::HasParameter(String& variable, bool remove)
{
  int pos = variable.Find('(');
  if (pos < 0 || pos >= variable.Count() - 2 || !variable.EndsWith(')')) return false;
  if (remove) variable.Delete(pos, variable.Count() - pos);
  return true;
}

bool ThemeManager::ExtractParameters(String& variable, String::List& parameters)
{
  String rawParams;
  if (!variable.Extract('(', ')', rawParams, false)) return false;
  parameters = rawParams.Split(',');
  for(String& s : parameters) s.Trim();
  if (int pos = variable.Find('('); pos >= 0)
    variable.Delete(pos, variable.Count() - pos);
  return true;
}

