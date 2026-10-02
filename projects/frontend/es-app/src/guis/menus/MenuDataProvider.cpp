//
// Created by bkg2k on 07/02/25.
//

#include "MenuDataProvider.h"
#include "netplay/DefaultPasswords.h"
#include "games/GameFilesUtils.h"
#include "Upgrade.h"
#include "systems/arcade/ArcadeVirtualSystems.h"
#include "ResolutionAdapter.h"
#include "LibretroRatio.h"
#include "scraping/ScraperFactory.h"
#include <guis/menus/MenuTools.h>
#include <audio/AudioController.h>
#include <views/ViewController.h>
#include <algorithm>

#include "hardware/cardreader/CardReader.h"
#include "hardware/JvsCab.h"
#include "utils/Files.h"

SelectorEntry<StorageDevices::Device>::List MenuDataProvider::GetShareEntries(int& currentIndex)
{
  SelectorEntry<StorageDevices::Device>::List list;
  int index = 0;
  for(const StorageDevices::Device& device : StorageDevices::Instance().Refresh().GetShareDevices())
  {
    String display = device.DisplayableName();
    if (device.Size != 0)
      display.Append(" (").Append(_("Free"))
             .Append(' ').Append(device.HumanFree())
             .Append('/').Append(device.HumanSize()).Append(')');
    list.push_back({ display, device, device.Current });
    if (device.Current) currentIndex = index;
    index++;
  }
  return list;
}

SelectorEntry<String>::List MenuDataProvider::GetCultureEntries()
{
  return SelectorEntry<String>::List
    ({
       { "ﺔﻐﻠﻟﺍ ﺔﻴﺑﺮﻌﻟﺍ" , "ar_YE" }, // /!\ This name is written in reversed final form to be readable, even in non-arabic locales (arabic processing code disabled)
       { "CATALÀ"        , "ca_ES" },
       { "čeština"       , "cs_CZ" },
       { "DEUTSCH"       , "de_DE" },
       { "DEUTSCH (CH)"  , "de_CH" },
       { "ελληνικά"      , "el_GR" },
       { "ENGLISH"       , "en_US" },
       { "ENGLISH (UK)"  , "en_GB" },
       { "ESPAÑOL"       , "es_ES" },
       { "EUSKARA"       , "eu_ES" },
       { "PERSIAN"       , "fa_IR" },
       { "FRANÇAIS"      , "fr_FR" },
       { "GALICIAN"      , "gl_ES" },
       { "MAGYAR"        , "hu_HU" },
       { "ITALIANO"      , "it_IT" },
       { "日本語"         , "ja_JP" },
       { "한국어"         , "ko_KR" },
       { "Lëtzebuergesch", "lb_LU" },
       { "latviešu"      , "lv_LV" },
       { "BOKMAL"        , "nb_NO" },
       { "NEDERLANDS"    , "nl_NL" },
       { "NORSK"         , "nn_NO" },
       { "POLSKI"        , "pl_PL" },
       { "PORTUGUES"     , "pt_BR" },
       { "ROMANIAN"      , "ro_RO" },
       { "Русский"       , "ru_RU" },
       { "SLOVAK"        , "sk_SK" },
       { "SVENSKA"       , "sv_SE" },
       { "TÜRKÇE"        , "tr_TR" },
       { "UKRAINIAN"     , "uk_UA" },
       { "简体中文"       , "zh_CN" },
       { "正體中文"       , "zh_TW" },
     });
}

SelectorEntry<String>::List MenuDataProvider::GetKeyboardEntries()
{
  return SelectorEntry<String>::List
    ({
       { "DE (German QWERTZ)"  , "de" },
       { "DK (Denmark QWERTY)" , "dk" },
       { "ES (Spanish QWERTY)" , "es" },
       { "FR (French AZERTY)"  , "fr" },
       { "IT (Italian QWERTY)" , "it" },
       { "US (Standard QWERTY)", "us" },
     });
}

SelectorEntry<RecalboxConf::VirtualKeyboardType>::List MenuDataProvider::GetVirtualKeyboardEntries()
{
  return SelectorEntry<RecalboxConf::VirtualKeyboardType>::List
    ({
       { _("Arcade wheel"),         RecalboxConf::VirtualKeyboardType::Wheel },
       { _("Classic keyboard"),     RecalboxConf::VirtualKeyboardType::Classic },
       { _("Simplified keyboard"),  RecalboxConf::VirtualKeyboardType::Simplified },
     });
}

SelectorEntry<String>::List MenuDataProvider::GetTimeZones()
{
  // Get timezones
  static Path timezoneRoot("/usr/share/zoneinfo/posix");
  Path::PathList timezones = timezoneRoot.RecurseDirectoryContent(false);
  // Remove base path
  bool dummy;
  for(Path& tz : timezones) tz = tz.MakeRelative(timezoneRoot, dummy);
  // Sort
  std::sort(timezones.begin(), timezones.end());

  // Build final structure
  SelectorEntry<String>::List result;
  for(Path& tz : timezones)
    result.push_back({ String(tz.ToString()).Replace('/', " - ", 3), tz.ToString() });

  return result;
}

SelectorEntry<String>::List MenuDataProvider::GetKodiResolutionsEntries(void* caller)
{
  SelectorEntry<String>::List result;

  result.push_back({ _("NATIVE"), "default" });
  result.push_back({ _("USE GLOBAL"), "" });
  for(const ResolutionAdapter::Resolution& resolution : GetResolutions(caller, false))
  {
    String reso = resolution.ToRawString();
    result.push_back({ resolution.ToString(), reso });
  }
  return result;
}

SelectorEntry<ScraperNameOptions>::List MenuDataProvider::GetScraperNameOptionsEntries()
{
  SelectorEntry<ScraperNameOptions>::List list;
  list.push_back({ _("Scraper results"), ScraperNameOptions::GetFromScraper });
  list.push_back({ _("Raw filename"), ScraperNameOptions::GetFromFilename });
  list.push_back({ _("Undecorated filename"), ScraperNameOptions::GetFromFilenameUndecorated });
  return list;
}

SelectorEntry<ScrapingMethod>::List MenuDataProvider::GetScrapingMethods()
{
  SelectorEntry<ScrapingMethod>::List list;
  list.push_back({ _("All Games"), ScrapingMethod::All });
  list.push_back({ _("Only missing image"), ScrapingMethod::AllIfNothingExists });
  return list;
}

SelectorEntry<ScraperType>::List MenuDataProvider::GetScrapersEntries()
{
  SelectorEntry<ScraperType>::List list;
  for(const auto& kv : ScraperFactory::GetScraperList())
    list.push_back({ kv.second, kv.first });
  return list;
}

SelectorEntry<SystemData*>::List MenuDataProvider::GetScrapableSystemsEntries()
{
  SelectorEntry<SystemData*>::List list;
  for(SystemData* system : mSystemManager.VisibleSystemList())
  {
    if (!system->IsScreenshots()) // Only screenshot doesn't need to be scraped
      if (system->HasScrapableGame())
        list.push_back({ system->FullName(), system, ScraperFactory::Instance().HasSystem(system) });
  }
  return list;
}

SelectorEntry<ScreenScraperEnums::ScreenScraperImageType>::List MenuDataProvider::GetScraperImagesEntries()
{
  SelectorEntry<ScreenScraperEnums::ScreenScraperImageType>::List list;
  list.push_back({ _("In-game screenshot"), ScreenScraperEnums::ScreenScraperImageType::ScreenshotIngame });
  list.push_back({ _("Title screenshot"),   ScreenScraperEnums::ScreenScraperImageType::ScreenshotTitle });
  list.push_back({ _("ScreenScraper Mix V1"),ScreenScraperEnums::ScreenScraperImageType::MixV1 });
  list.push_back({ _("ScreenScraper Mix V2"), ScreenScraperEnums::ScreenScraperImageType::MixV2 });
  return list;
}

SelectorEntry<ScreenScraperEnums::ScreenScraperBoxType>::List MenuDataProvider::GetScraperBoxesEntries()
{
  SelectorEntry<ScreenScraperEnums::ScreenScraperBoxType>::List list;
  list.push_back({ _("2D Box"), ScreenScraperEnums::ScreenScraperBoxType::Box2d });
  list.push_back({ _("3D Box"), ScreenScraperEnums::ScreenScraperBoxType::Box3d });
  return list;
}

SelectorEntry<ScreenScraperEnums::ScreenScraperLogoType>::List MenuDataProvider::GetScraperLogoEntries()
{
  SelectorEntry<ScreenScraperEnums::ScreenScraperLogoType>::List list;
  list.push_back({ _("Clear"), ScreenScraperEnums::ScreenScraperLogoType::Clear });
  list.push_back({ _("Carbon"), ScreenScraperEnums::ScreenScraperLogoType::Carbon });
  list.push_back({ _("Steel"), ScreenScraperEnums::ScreenScraperLogoType::Steel });
  list.push_back({ _("Short Marquee (2:1)"), ScreenScraperEnums::ScreenScraperLogoType::ShortMarquee });
  list.push_back({ _("Long Marquee (4:1)"), ScreenScraperEnums::ScreenScraperLogoType::LongMarquee });
  list.push_back({ _("SteamGrid (composite logo)"), ScreenScraperEnums::ScreenScraperLogoType::SteamGrid });
  return list;
}

SelectorEntry<ScreenScraperEnums::ScreenScraperVideoType>::List MenuDataProvider::GetScraperVideosEntries()
{
  SelectorEntry<ScreenScraperEnums::ScreenScraperVideoType>::List list;
  list.push_back({ _("No video"), ScreenScraperEnums::ScreenScraperVideoType::None });
  list.push_back({ String(_("Optimized video")).Append(" (").Append(_("RECOMMENDED")).Append(')'), ScreenScraperEnums::ScreenScraperVideoType::Optimized });
  list.push_back({ _("Original video"), ScreenScraperEnums::ScreenScraperVideoType::Raw});
  return list;
}

SelectorEntry<ScreenScraperEnums::ScreenScraperRegionPriority>::List MenuDataProvider::GetScraperRegionOptionsEntries()
{
  SelectorEntry<ScreenScraperEnums::ScreenScraperRegionPriority>::List list;
  list.push_back({_("DETECTED REGION"), ScreenScraperEnums::ScreenScraperRegionPriority::DetectedRegion });
  list.push_back({_("FAVORITE REGION"), ScreenScraperEnums::ScreenScraperRegionPriority::FavoriteRegion });
  return list;
}

SelectorEntry<Regions::GameRegions>::List MenuDataProvider::GetRegionEntries(bool allowNone)
{
  SelectorEntry<Regions::GameRegions>::List list;
  for(Regions::GameRegions region : Regions::AvailableRegions())
  {
    if (Regions::GameRegions::Unknown == region)
    {
      if (allowNone) list.push_back({ _("NONE"), region });
    }
    else
      list.push_back({ Regions::RegionFullName(region), region });
  }
  return list;
}

SelectorEntry<Regions::OrderedMasterRegions>::List MenuDataProvider::GetMasterRegionEntries()
{
  SelectorEntry<Regions::OrderedMasterRegions>::List list;
  for(int i = 0; i < (int)Regions::OrderedMasterRegions::__Count; ++i)
    list.push_back({ Regions::OrderedMasterRegionsFromEnum((Regions::OrderedMasterRegions)i), (Regions::OrderedMasterRegions)i });
  return list;
}

SelectorEntry<Languages>::List MenuDataProvider::GetScraperLanguagesEntries()
{
  SelectorEntry<Languages>::List list;
  for(Languages language : LanguagesTools::AvailableLanguages())
  {
    if (language == Languages::Unknown) continue;
    list.push_back({ LanguagesTools::LanguagesFullName(language), language });
  }
  return list;
}

SelectorEntry<RecalboxConf::SoftPatching>::List& MenuDataProvider::GetSoftpatchingEntries()
{
  static SelectorEntry<RecalboxConf::SoftPatching>::List list
    {
      { _("AUTO"), RecalboxConf::SoftPatching::Auto },
      { _("LAUNCH LAST"), RecalboxConf::SoftPatching::LaunchLast },
      { _("SELECT"), RecalboxConf::SoftPatching::Select },
      { _("DISABLE"), RecalboxConf::SoftPatching::Disable },
    };
  return list;
}

SelectorEntry<String>::List& MenuDataProvider::GetSuperGameBoyEntries()
{
  static SelectorEntry<String>::List list
    {
      { _("GAME BOY"), "gb" },
      { _("SUPER GAME BOY"), "sgb" },
      { _("ASK AT LAUNCH"), "ask" },
    };
  return list;
}

SelectorEntry<String>::List& MenuDataProvider::GetJvsCabTypeEntries()
{
  static SelectorEntry<String>::List list
    {
      { _("Not configured"),       JvsCab::sNotConfigured },
      { _("Joystick"),             JvsCab::sJoystick },
      { _("Race"),                 JvsCab::sRace },
      { _("Gun"),                  JvsCab::sGun },
      { _("Crazy Taxi"),           JvsCab::sCrazyTaxi },
      { _("18 Wheeler"),           JvsCab::sEighteenWheeler },
      { _("Jambo! Safari"),        JvsCab::sJamboSafari },
      { _("House of the Dead 3"),  JvsCab::sHouseOfTheDead3 },
      { _("Lupin the Third"),      JvsCab::sLupin3 },
      { _("New Net City"),         JvsCab::sNewNetCity },
      { _("Naomi NUC"),            JvsCab::sNaomiNUC },
      { _("Vewlix"),               JvsCab::sVewlix },
      { _("Confidential Mission"), JvsCab::sConfidentialMission },
      { _("Gun cab (custom)"),     JvsCab::sGunCustom },
      { _("Blast City"),           JvsCab::sJoyBlast },
      { _("Net City"),             JvsCab::sNetCity },
    };
  return list;
}

SelectorEntry<String>::List MenuDataProvider::GetRatioEntries()
{
  SelectorEntry<String>::List list;
  for (const auto& ratio : LibretroRatio::GetRatio())
    list.push_back({ ratio.first, ratio.second });

  return list;
}

SelectorEntry<String>::List& MenuDataProvider::GetShaderSetEntries()
{
  static SelectorEntry<String>::List list
    {
      { _("NONE"), "none" },
      { _("CRT CURVED"), "crtcurved" },
      { _("SCANLINES"), "scanlines" },
      { _("RETRO"), "retro" },
    };
  return list;
}

SelectorEntry<String>::List MenuDataProvider::GetShadersEntries()
{
  SelectorEntry<String>::List list;
  MenuTools::ShaderList shaderList = MenuTools::ListShaders();
  list.push_back({ _("NONE"), "" });
  for (const MenuTools::Shader& shader : shaderList)
    list.push_back({ shader.Displayable, shader.ShaderPath.ToString() });
  return list;
}

SelectorEntry<String>::List MenuDataProvider::GetCrtHandheldFormatEntries()
{
  static SelectorEntry<String>::List list
    {
        { _("FULLSCREEN"), "fullscreen" },
        { _("ORIGINAL"), "original" },
      };
  return list;
}

SelectorEntry<RecalboxConf::Relay>::List& MenuDataProvider::GetMitmEntries()
{
  static SelectorEntry<RecalboxConf::Relay>::List list
    {
      { _("NONE"), RecalboxConf::Relay::None },
      { _("NEW YORK"), RecalboxConf::Relay::NewYork },
      { _("MADRID"), RecalboxConf::Relay::Madrid },
      { _("MONTREAL"), RecalboxConf::Relay::Montreal },
      { _("SAOPAULO"), RecalboxConf::Relay::Saopaulo },
    };
  return list;
}

SelectorEntry<RecalboxConf::UpdateType>::List MenuDataProvider::GetUpdateTypeEntries()
{
  SelectorEntry<RecalboxConf::UpdateType>::List list;

  list.push_back({ "stable", RecalboxConf::UpdateType::Stable });
  list.push_back({ "patron", RecalboxConf::UpdateType::Patron });
  list.push_back({ "patron-alpha", RecalboxConf::UpdateType::PatronAlpha });
  if (RecalboxConf::Instance().GetUpdateType() == RecalboxConf::UpdateType::Alpha)
    list.push_back({ "alpha" , RecalboxConf::UpdateType::Alpha });
  if (RecalboxConf::Instance().GetUpdateType() == RecalboxConf::UpdateType::RRGBD2)
    list.push_back({ "rrgbd2" , RecalboxConf::UpdateType::RRGBD2 });
  if (RecalboxConf::Instance().GetUpdateType() == RecalboxConf::UpdateType::JVS)
    list.push_back({ "jvs" , RecalboxConf::UpdateType::JVS });

  return list;
}

SelectorEntry<RecalboxConf::Screensaver>::List MenuDataProvider::GetTypeEntries()
{
  SelectorEntry<RecalboxConf::Screensaver>::List list;

  RecalboxConf::Screensaver type = mConf.GetScreenSaverType();
  if (Board::Instance().HasSuspendResume())
    list.push_back({ _("suspend"), RecalboxConf::Screensaver::Suspend, type == RecalboxConf::Screensaver::Suspend });
  list.push_back({ _("dim"), RecalboxConf::Screensaver::Dim, type == RecalboxConf::Screensaver::Dim });
  list.push_back({ _("black"), RecalboxConf::Screensaver::Black, type == RecalboxConf::Screensaver::Black });
  list.push_back({ _("demo"), RecalboxConf::Screensaver::Demo, type == RecalboxConf::Screensaver::Demo });
  list.push_back({ _("gameclip"), RecalboxConf::Screensaver::Gameclip, type == RecalboxConf::Screensaver::Gameclip });
  list.push_back({ _("bouncing"), RecalboxConf::Screensaver::Bouncing, type == RecalboxConf::Screensaver::Bouncing });

  return list;
}

SelectorEntry<String>::List MenuDataProvider::GetSystemListAsString()
{
  SelectorEntry<String>::List list;

  for (const SystemData* system : mSystemManager.AllSystems())
    if (system->HasGame() && !system->Descriptor().IsPort())
      list.push_back({ system->FullName(), system->Name(), mConf.IsInScreenSaverSystemList(system->Name()) });

  return list;
}

SelectorEntry<RecalboxConf::Transition>::List MenuDataProvider::GetThemeTransitionEntries()
{
  SelectorEntry<RecalboxConf::Transition>::List list;

  RecalboxConf::Transition originalTransition = mConf.GetThemeTransition();
  list.push_back({ _("SLIDE"), RecalboxConf::Transition::Slide, originalTransition == RecalboxConf::Transition::Slide });
  list.push_back({ _("FADE"), RecalboxConf::Transition::Fade, originalTransition == RecalboxConf::Transition::Fade });
  list.push_back({ _("INSTANT"), RecalboxConf::Transition::Instant, originalTransition == RecalboxConf::Transition::Instant });

  return list;
}

SelectorEntry<RecalboxConf::Transition>::List MenuDataProvider::GetGameTransitionEntries()
{
  SelectorEntry<RecalboxConf::Transition>::List list;

  RecalboxConf::Transition originalTransition = mConf.GetGameTransition();
  list.push_back({ _("ZOOM"), RecalboxConf::Transition::Slide, originalTransition == RecalboxConf::Transition::Slide });
  list.push_back({ _("FADE"), RecalboxConf::Transition::Fade, originalTransition == RecalboxConf::Transition::Fade });
  list.push_back({ _("INSTANT"), RecalboxConf::Transition::Instant, originalTransition == RecalboxConf::Transition::Instant });

  return list;
}

SelectorEntry<String>::List MenuDataProvider::GetThemeRegionEntries()
{
  SelectorEntry<String>::List list;

  String region = mConf.GetThemeRegion();
  list.push_back({ _("Europe"), "eu", region == "eu" });
  list.push_back({ _("USA"), "us", region == "us" });
  list.push_back({ _("Japan"), "jp", region == "jp" });

  return list;
}

ThemeSpec MenuDataProvider::GetCurrentTheme()
{
  ThemeManager::ThemeList themelist = ThemeManager::AvailableThemes();
  if (themelist.contains(mConf.GetThemeFolder())) return { .Name=mConf.GetThemeFolder(), .FolderPath=themelist[mConf.GetThemeFolder()] };
  return { .Name=String::Empty, .FolderPath=Path::Empty };
}

ThemeSpec MenuDataProvider::GetDefaultTheme()
{
  ThemeManager::ThemeList themelist = ThemeManager::AvailableThemes();
  if (themelist.contains(ThemeManager::sDefaultThemeFolder)) return { .Name=ThemeManager::sDefaultThemeFolder, .FolderPath=themelist[ThemeManager::sDefaultThemeFolder] };
  return { .Name=themelist.begin()->first, .FolderPath=themelist.begin()->second };
}

SelectorEntry<SystemSorting>::List& MenuDataProvider::GetSystemSortingEntries()
{
  static SelectorEntry<SystemSorting>::List list
    {
      { _("DEFAULT")                                    , SystemSorting::Default                                    },
      { _("Name")                                       , SystemSorting::Name                                       },
      { _("RELEASE DATE")                               , SystemSorting::ReleaseDate                                },
      { _("TYPE, THEN NAME")                            , SystemSorting::SystemTypeThenName                         },
      { _("TYPE, THEN RELEASE DATE")                    , SystemSorting::SystemTypeThenReleaseDate                  },
      { _("MANUFACTURER, THEN NAME")                    , SystemSorting::ManufacturerThenName                       },
      { _("MANUFACTURER, THEN RELEASE DATE")            , SystemSorting::ManufacturerThenReleaseData                },
      { _("TYPE, THEN MANUFACTURER, THEN NAME")         , SystemSorting::SystemTypeThenManufacturerThenName         },
      { _("TYPE, THEN MANUFACTURER, THEN RELEASE DATE") , SystemSorting::SystemTypeThenManufacturerThenReleasdeDate },
    };
  return list;
}

SelectorEntry<SystemData*>::List MenuDataProvider::GetSystemShown()
{
  SelectorEntry<SystemData*>::List result;
  for(SystemData* system : mSystemManager.AllSystems())
    if (!system->IsPorts() && !system->IsVirtual())
      result.push_back({ system->FullName(), system, !mConf.GetSystemIgnore(*system) });
  std::ranges::sort(result, [](const SelectorEntry<SystemData*>& a, const SelectorEntry<SystemData*>& b) { return a.mText < b.mText; });
  return result;
}

SelectorEntry<ThemeSpec>::List MenuDataProvider::GetThemeEntries(WindowManager& window)
{
  // Get theme list
  ThemeManager::ThemeList themelist = ThemeManager::AvailableThemes();

  String currentVersionString = Upgrade::CurrentVersion();
  int cut = (int)currentVersionString.find_first_not_of("0123456789.");
  if (cut >= 0) currentVersionString.Delete(cut, INT32_MAX);

  // Sort names
  //! Theme spec list
  std::vector<ThemeSpec> sortedNames;
  for (const auto& kv : themelist) sortedNames.push_back({ .Name=kv.first, .FolderPath=kv.second });
  std::ranges::sort(sortedNames, [](const ThemeSpec& a, const ThemeSpec& b) { return a.Name.ToLowerCase() < b.Name.ToLowerCase(); });

  SelectorEntry<ThemeSpec>::List list;
  for (const ThemeSpec& theme : sortedNames)
  {
    String displayableName;
    bool compatible = ThemeManager::Instance().AnalyseAndSwitch(window, theme.FolderPath, displayableName, false);

    // (v) - compatibility match else /!\ sign - compatibility mismatch
    displayableName.Insert(0, compatible ? "\uF1C0 " : "\uF1CA ");

    list.push_back({ displayableName, theme });
  }

  return list;
}

SelectorEntry<String>::List MenuDataProvider::GetManufacturersVirtualEntries()
{
  SelectorEntry<String>::List result;
  const RecalboxConf& conf = RecalboxConf::Instance();

  for(const std::pair<String, String>& rawIdentifier : ArcadeVirtualSystems::GetVirtualArcadeSystemListExtended())
  {
    String identifier(SystemManager::sArcadeManufacturerPrefix);
    identifier.Append(rawIdentifier.first).Replace('\\', '-');
    result.push_back({ rawIdentifier.second, identifier, conf.IsInCollectionArcadeManufacturers(identifier) });
  }
  return result;
}

SelectorEntry<String>::List MenuDataProvider::GetBootSystemEntries()
{
  SelectorEntry<String>::List list;

  // For each activated system
  for (SystemData* system : mSystemManager.VisibleSystemList())
    list.push_back({ system->FullName(), system->Name() });

  return list;
}

SelectorEntry<GameGenres>::List MenuDataProvider::GetMultiGenreEntries()
{
  SelectorEntry<GameGenres>::List list;

  for(const GameGenres genre : Genres::GetOrderedList())
  {
    String longName = Genres::GetFullName(genre);
    String prefix = Genres::IsSubGenre(genre) ? "   \u21B3 " : "";
    //Path icon = Path(Genres::GetResourcePath(genre));
    list.push_back({ prefix.Append(_S(longName)), genre, mConf.IsInCollectionGenre(SystemManager::BuildGenreSystemName(genre)) });
  }
  return list;
}

SelectorEntry<GameGenres>::List MenuDataProvider::GetSingleGenreEntries()
{
  SelectorEntry<GameGenres>::List list;

  for(const GameGenres genre : Genres::GetOrderedList())
  {
    String longName = Genres::GetFullName(genre);
    String prefix = Genres::IsSubGenre(genre) ? "   \u21B3 " : "";
    list.push_back({ prefix.Append(_S(longName)), genre });
  }
  return list;
}

SelectorEntry<String>::List MenuDataProvider::GetGlobalResolutionEntries(void* caller)
{
  SelectorEntry<String>::List result;
  for(const ResolutionAdapter::Resolution& resolution : GetResolutions(caller, true))
  {
    String reso = resolution.ToRawString();
    result.push_back({ resolution.ToString(), reso });
  }
  return result;
}

SelectorEntry<String>::List MenuDataProvider::GetResolutionEntries(void* caller)
{
  SelectorEntry<String>::List result;
  result.push_back({ _("USE GLOBAL").Append(" (").Append(GetResolutionText(mConf.GetGlobalVideoMode())).Append(')'), "" });
  result.push_back({ _("NATIVE").Append(" (").Append(GetResolutionText("default")).Append(')'), "default" });

  for(const ResolutionAdapter::Resolution& resolution : GetResolutions(caller, true))
  {
    String reso = resolution.ToRawString();
    result.push_back({ resolution.ToString(), reso });
  }
  return result;
}

SelectorEntry<Overclocking::Overclock>::List MenuDataProvider::GetOverclockEntries()
{
  SelectorEntry<Overclocking::Overclock>::List list;

  // Add entries
  const Overclocking::List& oc = Board::Instance().GetOverclocking().GetOverclockList();
  for(const Overclocking::Overclock& overclock : oc)
  {
    String label = String(overclock.Description()).Append(" (", 2).Append(overclock.Frequency()).Append(" Mhz)", 5).Append(overclock.Hazardous() ? " \u26a0" : "");
    list.push_back({ label, overclock, false });
  }
  std::ranges::sort(list, [](const SelectorEntry<Overclocking::Overclock>& a, const SelectorEntry<Overclocking::Overclock>& b) { return a.mValue.Frequency() < b.mValue.Frequency(); });
  // Add none
  if (!list.empty())
    list.insert(list.begin(), { _("NONE"), Overclocking::NoOverclock(), false });

  return list;
}

SelectorEntry<String>::List MenuDataProvider::GetEmulatorEntries(const SystemData* system, String& emulatorAndCore, String& defaultEmulatorAndCore)
{
  SelectorEntry<String>::List list;

  String currentEmulator(mConf.GetSystemEmulator(*system));
  String currentCore    (mConf.GetSystemCore(*system));
  String defaultEmulator;
  String defaultCore;
  MenuTools::EmulatorAndCoreList eList =
    MenuTools::ListEmulatorAndCore(*system, defaultEmulator, defaultCore, currentEmulator, currentCore);
  if (!eList.empty())
    for (const MenuTools::EmulatorAndCore& emulator : eList)
    {
      list.push_back({ emulator.Displayable, emulator.Identifier });
      if (emulator.Selected) emulatorAndCore = emulator.Identifier;
      if (defaultEmulatorAndCore.empty()) defaultEmulatorAndCore = emulator.Identifier;
    }

  return list;
}

SelectorEntry<CrtAdapterType>::List MenuDataProvider::GetDacEntries()
{
  SelectorEntry<CrtAdapterType>::List list;
  CrtAdapterType currentCrtBoard = Board::Instance().CrtBoard().GetCrtAdapter();
  if(currentCrtBoard == CrtAdapterType::RGBDual || currentCrtBoard == CrtAdapterType::RGBDual2 || currentCrtBoard == CrtAdapterType::RGBJamma)
  {
    list.push_back( { Board::Instance().CrtBoard().Name(), currentCrtBoard, true } );
    return list;
  }

  CrtAdapterType selectedDac = CrtConf::Instance().GetSystemCRT();

  static struct
  {
    const char* Name;
    CrtAdapterType Type;
  }
    Adapters[] =
    {
      { .Name="Recalbox RGB Dual", .Type=CrtAdapterType::RGBDual },
      { .Name="Recalbox RGB Jamma", .Type=CrtAdapterType::RGBJamma },
      { .Name="VGA666", .Type=CrtAdapterType::Vga666 },
      { .Name="RGBPi", .Type=CrtAdapterType::RGBPi },
      { .Name="Pi2SCART", .Type=CrtAdapterType::Pi2Scart },
    };

  // Always push none
  list.push_back( { _("NONE"), CrtAdapterType::None, selectedDac == CrtAdapterType::None } );
  // Push all adapters or only one if it is automatically detected
  const ICrtInterface& crt = Board::Instance().CrtBoard();
  for(const auto& item : Adapters)
  {
    if (!crt.HasBeenAutomaticallyDetected() || crt.GetCrtAdapter() == item.Type)
      list.push_back( { item.Name, item.Type, selectedDac == item.Type } );
  }

  return list;
}

SelectorEntry<String>::List MenuDataProvider::GetEsResolutionEntries()
{
  SelectorEntry<String>::List list;
  ICrtInterface::HorizontalFrequency currentScreen = Board::Instance().CrtBoard().GetHorizontalFrequency();

  std::string baseRez = CrtConf::Instance().GetSystemCRTResolution();
  list.push_back({ "auto", "0", baseRez == "0" });
  // Each monitor > KHz31 has 480p support
  if(currentScreen >= ICrtInterface::HorizontalFrequency::KHz31)
  {
    list.push_back({ "480p", "480", baseRez == "480" });
    if(Board::Instance().CrtBoard().Has120HzSupport())
      list.push_back({ "240p@120Hz", "240", baseRez == "240" });
  }

  // Tri and multi with 25
  if(currentScreen == ICrtInterface::HorizontalFrequency::KHzTriFreq || currentScreen == ICrtInterface::HorizontalFrequency::KHzMulti1525)
    list.push_back({ "384p", "384", baseRez == "384" });

  // All with 15khz
  if(currentScreen == ICrtInterface::HorizontalFrequency::KHzTriFreq
     || currentScreen == ICrtInterface::HorizontalFrequency::KHzMulti1525
     || currentScreen == ICrtInterface::HorizontalFrequency::KHz15
     || currentScreen == ICrtInterface::HorizontalFrequency::KHzMulti1531)
    list.push_back({ "240p", "240", baseRez == "240" });

  // All with 15khz
  if(currentScreen == ICrtInterface::HorizontalFrequency::KHz15 && Board::Instance().CrtBoard().HasInterlacedSupport())
    list.push_back({ "480i", "480", baseRez == "480" });

  return list;
}

SelectorEntry<ICrtInterface::HorizontalFrequency>::List MenuDataProvider::GetHorizontalOutputFrequency()
{
  SelectorEntry<ICrtInterface::HorizontalFrequency>::List frequencies;

  if (Board::Instance().CrtBoard().MustForce15kHz())
  {
    frequencies.emplace_back(
        ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHz15),
        ICrtInterface::HorizontalFrequency::KHz15);
    return frequencies;
  }

  if (Board::Instance().CrtBoard().MustForce31kHz())
  {
    frequencies.emplace_back(
        ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHz31),
        ICrtInterface::HorizontalFrequency::KHz31);
    return frequencies;
  }

  if (Board::Instance().CrtBoard().MustForce31kHzByDefault())
  {
    if (Board::Instance().CrtBoard().GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHz15)
    {
      frequencies.emplace_back( ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHz15), ICrtInterface::HorizontalFrequency::Auto );
      frequencies.emplace_back( ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHz31), ICrtInterface::HorizontalFrequency::KHz31 );
    }
    else if (Board::Instance().CrtBoard().GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHz31)
    {
      frequencies.emplace_back( ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHz31), ICrtInterface::HorizontalFrequency::Auto );
      frequencies.emplace_back( ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHz15), ICrtInterface::HorizontalFrequency::KHz15 );
    }
    else
    {
      frequencies.emplace_back( ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHz15), ICrtInterface::HorizontalFrequency::KHz15 );
      frequencies.emplace_back( ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHz31), ICrtInterface::HorizontalFrequency::KHz31 );
    }
  }
  else if (Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::JVS)
  {
    // JVS default is 31kHz (driver maps Auto → KHz31), so the explicit "15kHz"
    // entry must save KHz15 — not Auto — otherwise the driver re-forces 31kHz.
    frequencies.emplace_back( ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHz31), ICrtInterface::HorizontalFrequency::Auto );
    frequencies.emplace_back( ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHz15), ICrtInterface::HorizontalFrequency::KHz15 );
  }
  else
  {
    frequencies.emplace_back( ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHz15), ICrtInterface::HorizontalFrequency::Auto );
    frequencies.emplace_back( ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHz31), ICrtInterface::HorizontalFrequency::KHz31 );
  }

  frequencies.emplace_back( ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHzMulti1525), ICrtInterface::HorizontalFrequency::KHzMulti1525 );
  frequencies.emplace_back( ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHzMulti1531), ICrtInterface::HorizontalFrequency::KHzMulti1531 );
  frequencies.emplace_back( ICrtInterface::HorizontalFrequencyToHumanReadable(ICrtInterface::HorizontalFrequency::KHzTriFreq), ICrtInterface::HorizontalFrequency::KHzTriFreq );

  return frequencies;
}

const SelectorEntry<String>::List& MenuDataProvider::GetSuperRezEntries()
{
  if (Board::Instance().IsPi5Familly())
  {
    static SelectorEntry<String>::List listpi5
    {
        {"DYNAMIC (DEFAULT)", "1"},
        {"NATIVE", "0"},
        {"X6", "1920"},
        {"X8", "2560"},
    };
    return listpi5;
  }
  if (Board::Instance().IsPi4Familly())
  {
    static SelectorEntry<String>::List listpi4
    {
        {"X8 (DEFAULT)", "2560"},
        {"X6", "1920"},
        {"DYNAMIC", "1"},
        {"NATIVE", "0"},
    };
    return listpi4;
  }
  static SelectorEntry<String>::List list
  {
      {"X6 (DEFAULT)", "1920"},
      {"DYNAMIC", "1"},
      {"NATIVE", "0"},
  };
  return list;
}

const SelectorEntry<CrtScanlines>::List& MenuDataProvider::GetScanlinesEntries()
{
  static SelectorEntry<CrtScanlines>::List list
    {
      { _("NONE"), CrtScanlines::None },
      { _("LIGHT"), CrtScanlines::Light },
      { _("MEDIUM"), CrtScanlines::Medium },
      { _("HEAVY"), CrtScanlines::Heavy },
    };
  return list;
}

SelectorEntry<String>::List MenuDataProvider::GetCasesEntries()
{
  SelectorEntry<String>::List list;
  Case currentCase = Case::CurrentCase();
  if (currentCase.Automatic()) list.push_back({currentCase.DisplayName() + " (AUTO)", currentCase.ShortName(), true});
  else {
    for(const Case& theCase : Case::SupportedManualCases()) list.push_back({theCase.DisplayName(), theCase.ShortName(), theCase.Model() == currentCase.Model()});
  }
  return list;
}

SelectorEntry<String>::List MenuDataProvider::GetAudioOutputEntries()
{
  SelectorEntry<String>::List list;
  // Add all remaining stuff
  for(const DeviceDescriptor& playback : AudioController::Instance().GetPlaybackList())
    list.push_back({ _S(playback.DisplayableName), playback.InternalName });
  return list;
}

const SelectorEntry<AudioMode>::List& MenuDataProvider::GetAudioModeEntries()
{
  static SelectorEntry<AudioMode>::List list
    {
      { _("Musics or videos sound"), AudioMode::MusicsXorVideosSound },
      { _("Musics and videos sound"), AudioMode::MusicsAndVideosSound },
      { _("Musics only"), AudioMode::MusicsOnly },
      { _("Videos sound only"), AudioMode::VideosSoundOnly },
      { _("No sound"), AudioMode::None },
    };
  return list;
}

SelectorEntry<RecalboxConf::GamelistDecoration>::List MenuDataProvider::GetDecorationEntries(const SystemData& system)
{
  (void)system;
  SelectorEntry<RecalboxConf::GamelistDecoration>::List list;
  RecalboxConf::GamelistDecoration decorations = mConf.GetSystemGamelistDecoration(*ViewController::Instance().CurrentSystem());
  list.push_back({ _("Region flags"), RecalboxConf::GamelistDecoration::Regions, hasFlag(decorations, RecalboxConf::GamelistDecoration::Regions) });
  list.push_back({ _("Players"), RecalboxConf::GamelistDecoration::Players, hasFlag(decorations, RecalboxConf::GamelistDecoration::Players) });
  list.push_back({ _("Genres"), RecalboxConf::GamelistDecoration::Genre, hasFlag(decorations, RecalboxConf::GamelistDecoration::Genre) });
  list.push_back({ _("Support numbers"), RecalboxConf::GamelistDecoration::SupportNumber, hasFlag(decorations, RecalboxConf::GamelistDecoration::SupportNumber) });
  list.push_back({ _("Support types"), RecalboxConf::GamelistDecoration::SupportType, hasFlag(decorations, RecalboxConf::GamelistDecoration::SupportType) });
  return list;
}

//! Displayable name of a gamelist layout
static String GamelistLayoutName(RecalboxConf::GamelistLayout layout)
{
  switch(layout)
  {
    case RecalboxConf::GamelistLayout::LogoList: return _("Logo list");
    case RecalboxConf::GamelistLayout::Carousel: return _("Carousel");
    case RecalboxConf::GamelistLayout::Grid: return _("Grid");
    case RecalboxConf::GamelistLayout::Default:
    case RecalboxConf::GamelistLayout::Legacy:
    default: break;
  }
  return _("Text list");
}

SelectorEntry<RecalboxConf::GamelistLayout>::List MenuDataProvider::GetGamelistContainerTypeEntries(const SystemData& system)
{
  SelectorEntry<RecalboxConf::GamelistLayout>::List list;
  RecalboxConf::GamelistLayout type = mConf.GetSystemGamelistLayoutNoDefault(system);
  // The first entry follows the global layout and names it, so the user can see
  // what leaving it alone actually means.
  list.push_back({ _("DEFAULT").Append(" (").Append(GamelistLayoutName(mConf.ResolvedGlobalGamelistLayout())).Append(')'), RecalboxConf::GamelistLayout::Default, type == RecalboxConf::GamelistLayout::Default });
  list.push_back({ GamelistLayoutName(RecalboxConf::GamelistLayout::Legacy), RecalboxConf::GamelistLayout::Legacy, type == RecalboxConf::GamelistLayout::Legacy });
  list.push_back({ GamelistLayoutName(RecalboxConf::GamelistLayout::LogoList), RecalboxConf::GamelistLayout::LogoList, type == RecalboxConf::GamelistLayout::LogoList });
  if (system.Theme().HasCarouselView())
    list.push_back({ GamelistLayoutName(RecalboxConf::GamelistLayout::Carousel), RecalboxConf::GamelistLayout::Carousel, type == RecalboxConf::GamelistLayout::Carousel });
  if (system.Theme().HasGridView())
    list.push_back({ GamelistLayoutName(RecalboxConf::GamelistLayout::Grid), RecalboxConf::GamelistLayout::Grid, type == RecalboxConf::GamelistLayout::Grid });
  return list;
}

SelectorEntry<RecalboxConf::GamelistLayout>::List MenuDataProvider::GetGlobalGamelistLayoutEntries()
{
  SelectorEntry<RecalboxConf::GamelistLayout>::List list;
  // No system here, so carousel and grid follow the main theme
  const ThemeData& theme = ThemeManager::Instance().Main();
  RecalboxConf::GamelistLayout type = mConf.ResolvedGlobalGamelistLayout();
  list.push_back({ GamelistLayoutName(RecalboxConf::GamelistLayout::Legacy), RecalboxConf::GamelistLayout::Legacy, type == RecalboxConf::GamelistLayout::Legacy });
  list.push_back({ GamelistLayoutName(RecalboxConf::GamelistLayout::LogoList), RecalboxConf::GamelistLayout::LogoList, type == RecalboxConf::GamelistLayout::LogoList });
  if (theme.HasCarouselView())
    list.push_back({ GamelistLayoutName(RecalboxConf::GamelistLayout::Carousel), RecalboxConf::GamelistLayout::Carousel, type == RecalboxConf::GamelistLayout::Carousel });
  if (theme.HasGridView())
    list.push_back({ GamelistLayoutName(RecalboxConf::GamelistLayout::Grid), RecalboxConf::GamelistLayout::Grid, type == RecalboxConf::GamelistLayout::Grid });
  return list;
}

SelectorEntry<Regions::GameRegions>::List MenuDataProvider::GetRegionEntries(const SystemData& system)
{
  SelectorEntry<Regions::GameRegions>::List list;
  for(auto region : ViewController::Instance().GetOrCreateGamelistView((SystemData*)&system, false)->AvailableRegionsInGames())
  {
    String regionName = (region == Regions::GameRegions::Unknown) ? _("NONE") : Regions::RegionFullName(region);
    list.push_back({ regionName, region });
  }
  return list;
}

SelectorEntry<FileSorts::Sorts>::List MenuDataProvider::GetSortEntries(const SystemData& system)
{
  SelectorEntry<FileSorts::Sorts>::List list;

  // Get & check sort id
  FileSorts::SortSets set = system.IsVirtual() ? FileSorts::SortSets::MultiSystem :
                            system.Descriptor().IsArcade() ? FileSorts::SortSets::Arcade :
                            FileSorts::SortSets::SingleSystem;
  const Array<FileSorts::Sorts>& availableSorts = FileSorts::AvailableSorts(set);

  list.reserve(availableSorts.Count());
  FileSorts& sorts = FileSorts::Instance();
  for(FileSorts::Sorts sort : availableSorts)
    list.push_back({ sorts.Name(sort), sort });

  return list;
}

SelectorEntry<String>::List MenuDataProvider::GetEmulatorEntries(FileData& game, String& emulatorAndCore)
{
  SelectorEntry<String>::List list;

  String currentEmulator(game.Metadata().Emulator());
  String currentCore    (game.Metadata().Core());
  String defaultEmulator;
  String defaultCore;
  MenuTools::EmulatorAndCoreList eList =
    MenuTools::ListGameEmulatorAndCore(game, defaultEmulator, defaultCore, currentEmulator, currentCore);
  for (const MenuTools::EmulatorAndCore& emulator : eList)
  {
    list.push_back({ emulator.Displayable, emulator.Identifier });
    if (emulator.Selected) emulatorAndCore = emulator.Identifier;
  }

  return list;
}

SelectorEntry<Path>::List MenuDataProvider::GetPatchEntries(FileData& game)
{
  SelectorEntry<Path>::List list;
  Path::PathList patches = GameFilesUtils::GetSoftPatches(&game);

  list.push_back({ _("original"), Path("original") });
  for(auto& path : patches)
  {
    String patchName = path.Directory() == game.RomPath().Directory() ? path.Filename() + " (auto)" : path.Filename();
    list.push_back({ patchName, path });
  }
  return list;
}

SelectorEntry<Path>::List MenuDataProvider::GetGameFilesToDelete(FileData& game)
{
  SelectorEntry<Path>::List list;
  list.push_back({ game.RomPath().Filename(),  game.RomPath(), false });
  for (const auto& file : GameFilesUtils::GetGameSubFiles(game))
    list.push_back({ file.Filename(), file, false });
  return list;
}

SelectorEntry<Path>::List MenuDataProvider::GetMediaFilesToDelete(FileData& game)
{
  SelectorEntry<Path>::List list;
  if (game.Metadata().Image().Exists()) list.push_back({ _("Image"), game.Metadata().Image(), false });
  if (game.Metadata().Box().Exists())   list.push_back({ _("Box"), game.Metadata().Box(), false });
  if (game.Metadata().Video().Exists()) list.push_back({ _("Video"), game.Metadata().Video(), false });
  return list;
}

SelectorEntry<Path>::List MenuDataProvider::GetExtraFilesToDelete(FileData& game)
{
  SelectorEntry<Path>::List list;
  for (const auto& path : GameFilesUtils::GetGameExtraFiles(game))
    list.push_back({ path.Filename(), path, false });
  return list;
}

SelectorEntry<Path>::List MenuDataProvider::GetSaveFilesToDelete(FileData& game)
{
  SelectorEntry<Path>::List list;
  for (const auto& path : GameFilesUtils::GetGameSaveFiles(game))
    list.push_back({ path.Filename(), path, false });
  return list;
}

SelectorEntry<Path>::List MenuDataProvider::GetPatchesEntries(FileData& game)
{
  SelectorEntry<Path>::List list;
  for(auto& path : GameFilesUtils::GetSoftPatches(&game))
  {
    String patchName = path.Directory() == game.RomPath().Directory() ? path.Filename() + " (auto)" : path.Filename();
    list.push_back({ patchName, path });
  }
  return list;
}

SelectorEntry<int>::List MenuDataProvider::GetNetplayPasswords()
{
  SelectorEntry<int>::List list { { String('(').Append(_("NONE")).Append(')'), -1 } };
  for(int i = 0; i < DefaultPasswords::sPasswordCount; i++)
  {
    String password = mConf.GetNetplayPasswords(i);
    if (password.empty()) password = DefaultPasswords::sDefaultPassword[i];
    list.push_back({ password, i });
  }
  return list;
}

SelectorEntry<String>::List& MenuDataProvider::GetAutoBlitterEntries()
{
  static SelectorEntry<String>::List list
  {
    {_("NONE"), "none"},
    {_("RECALBOX (DEFAULT)"), "recalbox"},
    {"VIKU", "viku"},
  };
  return list;
}

SelectorEntry<String>::List& MenuDataProvider::GetCRConsoleModeExit()
{
  static SelectorEntry<String>::List list
    {
      {"HK + START", "hkstart"},
      {"START 5 SEC", "start5sec"},
    };
  return list;
}

SelectorEntry<String>::List MenuDataProvider::GetCRConsoleModeVideo()
{
  SelectorEntry<String>::List list {};
  for (const auto& file : Path(CardReader::VideoPath).GetDirectoryContent(false))
    if (file.Extension() == ".mp4")
      list.emplace_back( file.FilenameWithoutExtension(), file.FilenameWithoutExtension() );
  if (list.empty())
    list.emplace_back( _("NONE"), "none" );
  return list;
}

SelectorEntry<String>::List& MenuDataProvider::GetCRConsoleModeSound()
{
  static SelectorEntry<String>::List list
    {
        {"Bios", "bios"},
        {"Recalbox Music", "music"},
      };
  return list;
}