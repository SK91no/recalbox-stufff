//
// Created by bkg2k on 07/02/25.
//
#pragma once

#include "MenuPersistantData.h"
#include "CrtConf.h"
#include "guis/menus/base/SelectorEntry.h"
#include "hardware/devices/storage/StorageDevices.h"
#include "scraping/scrapers/ScrapingMethod.h"
#include "themes/ThemeSpec.h"
#include "themes/ThemeData.h"
#include "hardware/crt/ICrtInterface.h"
#include "hardware/Overclocking.h"
#include "ResolutionAdapter.h"
#include "systems/SystemManager.h"

class MenuDataProvider
{
  public:
    MenuDataProvider(MenuPersistantData& persistantData, SystemManager& systemManager)
      : mSystemManager(systemManager)
      , mPersistantData(persistantData)
      , mConf(RecalboxConf::Instance())
      , mCrtConf(CrtConf::Instance())
      , mPreviousCaller(nullptr)
      , mPreviousFilter(false)
    {
    }

    /*
     * Data getter
     */

    //! Get Storage List
    static SelectorEntry<StorageDevices::Device>::List GetShareEntries(int& currentIndex);
    //! Get Culture List
    static SelectorEntry<String>::List GetCultureEntries();
    //! Get Keyboard List
    static SelectorEntry<String>::List GetKeyboardEntries();
    //! Get Virtual Keyboard type list (Wheel / Classic)
    static SelectorEntry<RecalboxConf::VirtualKeyboardType>::List GetVirtualKeyboardEntries();
    //! Get TimeZone List
    static SelectorEntry<String>::List GetTimeZones();
    //! Get resolution list for Kodi
    SelectorEntry<String>::List GetKodiResolutionsEntries(void* caller);
    //! Get available scraping methods
    static SelectorEntry<ScrapingMethod>::List GetScrapingMethods();
    //! Get scrapable system list
    SelectorEntry<SystemData*>::List GetScrapableSystemsEntries();
    //! Get available scrappers
    static SelectorEntry<ScraperType>::List GetScrapersEntries();
    //! Get name retrieval methods when scraping
    static SelectorEntry<ScraperNameOptions>::List GetScraperNameOptionsEntries();
    //! Get available scraped image types
    static SelectorEntry<ScreenScraperEnums::ScreenScraperImageType>::List GetScraperImagesEntries();
    //! Get available scraped box types
    static SelectorEntry<ScreenScraperEnums::ScreenScraperBoxType>::List GetScraperBoxesEntries();
    //! Get available scraped video types
    static SelectorEntry<ScreenScraperEnums::ScreenScraperVideoType>::List GetScraperVideosEntries();
    //! Get available scraped logo types
    static SelectorEntry<ScreenScraperEnums::ScreenScraperLogoType>::List GetScraperLogoEntries();
    //! Get scraped region priority
    static SelectorEntry<ScreenScraperEnums::ScreenScraperRegionPriority>::List GetScraperRegionOptionsEntries();
    //! Get favorite region when scrping
    static SelectorEntry<Regions::GameRegions>::List GetRegionEntries(bool allowNone);
    //! Get ordered master region list
    static SelectorEntry<Regions::OrderedMasterRegions>::List GetMasterRegionEntries();
    //! Get available scraped languages
    static SelectorEntry<Languages>::List GetScraperLanguagesEntries();
    //! Get Softpatching option list
    static SelectorEntry<RecalboxConf::SoftPatching>::List& GetSoftpatchingEntries();
    //! Get Bameboy mod entries
    static SelectorEntry<String>::List& GetSuperGameBoyEntries();
    //! Get JVS cab type entries
    static SelectorEntry<String>::List& GetJvsCabTypeEntries();
    //! Get libretro ratios
    static SelectorEntry<String>::List GetRatioEntries();
    //! Get Shaders List
    static SelectorEntry<String>::List GetShadersEntries();
    //! Get Crt Handhelds Format entries
    static SelectorEntry<String>::List GetCrtHandheldFormatEntries();
    //! Get shader sets List
    static SelectorEntry<String>::List& GetShaderSetEntries();
    //! Get Netplay relay server list
    static SelectorEntry<RecalboxConf::Relay>::List& GetMitmEntries();
    //! Get Update type List
    static SelectorEntry<RecalboxConf::UpdateType>::List GetUpdateTypeEntries();
    //! Get screensavers
    SelectorEntry<RecalboxConf::Screensaver>::List GetTypeEntries();
    //! Getstatic  all system list as string
    SelectorEntry<String>::List GetSystemListAsString();
    //! Get all available themes
    static SelectorEntry<ThemeSpec>::List GetThemeEntries(WindowManager& window);
    //! Get current theme
    ThemeSpec GetCurrentTheme();
    //! Get default theme
    static ThemeSpec GetDefaultTheme();
    // Get theme transitions
    SelectorEntry<RecalboxConf::Transition>::List GetThemeTransitionEntries();
    // Get game transitions
    SelectorEntry<RecalboxConf::Transition>::List GetGameTransitionEntries();
    // Get theme regions
    SelectorEntry<String>::List GetThemeRegionEntries();
    //! Get hidable system list
    SelectorEntry<SystemData*>::List GetSystemShown();
    //! Get System sorting list
    static SelectorEntry<SystemSorting>::List& GetSystemSortingEntries();
    //! Get manufacturer list
    static SelectorEntry<String>::List GetManufacturersVirtualEntries();
    //! Boot system list
    SelectorEntry<String>::List GetBootSystemEntries();
    //! Game genre list for multiple selection
    SelectorEntry<GameGenres>::List GetMultiGenreEntries();
    //! Game genre list for single static selection
    static SelectorEntry<GameGenres>::List GetSingleGenreEntries();
    //! Get global resolution list
    SelectorEntry<String>::List GetGlobalResolutionEntries(void* caller);
    //! Get frontend resolution list
    SelectorEntry<String>::List GetResolutionEntries(void* caller);
    //! Get overcstatic static lock list
    static SelectorEntry<Overclocking::Overclock>::List GetOverclockEntries();
    //! Get emulator list for the given system
    SelectorEntry<String>::List GetEmulatorEntries(const SystemData* system, String& emulatorAndCore, String& defaultEmulatorAndCore);
    //! Get RGstatic static static B Adapter
    static SelectorEntry<CrtAdapterType>::List GetDacEntries();
    // Get Crt Frostatic ntend rstatic esolution
    static SelectorEntry<String>::List GetEsResolutionEntries();
    //! Crt horizontal frequencies
    static SelectorEntry<ICrtInterface::HorizontalFrequency>::List GetHorizontalOutputFrequency();
    //! Get CRT's super resolution list
    static const SelectorEntry<String>::List& GetSuperRezEntries();
    //! Get CRT's scanline list
    static const SelectorEntry<CrtScanlines>::List& GetScanlinesEntries();
    //! Get Cstatic ases list
    static SelectorEntry<String>::List GetCasesEntries();
    //! Get audio output list
    static SelectorEntry<String>::List GetAudioOutputEntries();
    //! Get audio mode list
    static const SelectorEntry<AudioMode>::List& GetAudioModeEntries();
    //! Get decorations list
    SelectorEntry<RecalboxConf::GamelistDecoration>::List GetDecorationEntries(const SystemData& system);
    //! Get gamelist container types, the first one meaning "follow the global layout"
    SelectorEntry<RecalboxConf::GamelistLayout>::List GetGamelistContainerTypeEntries(const SystemData& system);
    //! Get the layouts that apply to every system having no layout of its own
    SelectorEntry<RecalboxConf::GamelistLayout>::List GetGlobalGamelistLayoutEntries();
    //! Get region list
    static SelectorEntry<Regions::GameRegions>::List GetRegionEntries(const SystemData& system);
    //! Get sort list according to the given system
    static SelectorEntry<FileSorts::Sorts>::List GetSortEntries(const SystemData& system);
    //! Get the emulator/core list for the given game, and return default emulator/core in emulatorAndCore
    static SelectorEntry<String>::List GetEmulatorEntries(FileData& game, String& emulatorAndCore);
    //! Get the list of available patches for the given game
    static SelectorEntry<Path>::List GetPatchEntries(FileData& game);
    //! Get game file list of the given game
    static SelectorEntry<Path>::List GetGameFilesToDelete(FileData& game);
    //! Get media file list associatd to the gstatic iven game
    static SelectorEntry<Path>::List GetMediaFilesToDelete(FileData& game);
    //! Get extra file list associatd to the given game
    static SelectorEntry<Path>::List GetExtraFilesToDelete(FileData& game);
    //! Get Save file list associatd to the gstatic iven game
    static SelectorEntry<Path>::List GetSaveFilesToDelete(FileData& game);
    //! Get softpatch list for the gstatic static iven game
    static SelectorEntry<Path>::List GetPatchesEntries(FileData& game);
    //! Get list of netplay passwords, including "none"
    SelectorEntry<int>::List GetNetplayPasswords();
    //! Get auto blitter entries
    static SelectorEntry<String>::List& GetAutoBlitterEntries();
    static SelectorEntry<String>::List& GetCRConsoleModeExit();
    static SelectorEntry<String>::List GetCRConsoleModeVideo();
    static SelectorEntry<String>::List& GetCRConsoleModeSound();

  private:
    //! SystemManager reference
    SystemManager& mSystemManager;

    //! Persistant data
    MenuPersistantData& mPersistantData;
    //! Reclabox configuration reference
    RecalboxConf& mConf;
    //! Crt configuration reference
    CrtConf& mCrtConf;

    //! Resolution Adapter
    ResolutionAdapter mResolutionAdapter;
    //! Resolution cache ruler
    void* mPreviousCaller;
    //! Resolution cache ruler
    bool mPreviousFilter;
    //! Last resolution list
    ResolutionAdapter::ResolutionList mResolutions;

    //! Get resolution list
    ResolutionAdapter::ResolutionList& GetResolutions(void* caller, bool filter)
    {
      if (caller != mPreviousCaller || filter != mPreviousFilter)
      {
        mPreviousCaller = caller;
        mPreviousFilter = filter;
        mResolutions = mResolutionAdapter.Resolutions(filter);
      }
      return mResolutions;
    }

    /*!
     * @brief Adjust resolution text used in menu:
     * - Empty resolution returns "UNSET"
     * - "default" returns the native resolution
     * - old RPi resolutions are returned as XxY
     * - Any other string is return "as is"
     * @param resolution
     * @return Resolution string
     */
    String GetResolutionText(const String& resolution)
    {
      if (resolution.empty()) return _("UNSET");
      if (resolution == "default")
      {
        ResolutionAdapter::Resolution res = mResolutionAdapter.DefaultResolution();
        return String(res.Width).Append('x').Append(res.Height);
      }
      Resolutions::SimpleResolution res = Resolutions::ConvertSimpleResolution(resolution);
      return String(res.Width).Append('x').Append(res.Height);
    }

};
