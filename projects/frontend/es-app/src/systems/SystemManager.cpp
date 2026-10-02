//
// Created by bkg2k on 15/10/2019.
//

#include "SystemManager.h"
#include "SystemDescriptor.h"
#include "SystemDeserializer.h"
#include "LightGunDatabase.h"
#include "ControllerDatabase.h"
#include "games/classifications/Versions.h"
#include "utils/hash/Crc32.h"
#include "games/GameFilesUtils.h"
#include "hardware/Board.h"
#include <systems/arcade/ArcadeVirtualSystems.h>
#include <utils/os/system/ThreadPool.h>
#include <utils/os/fs/StringMapFile.h>
#include <utils/Files.h>
#include <dirent.h>
#include <algorithm>

SystemManager::RomSources SystemManager::GetRomSource(const SystemDescriptor& systemDescriptor, PortTypes port)
{
  RomSources roots;
  if (systemDescriptor.RomPath().ToString().Contains(sRootTag))
  {
    String rootTag(sRootTag);
    // Share_init roms
    Path root = Path(String(systemDescriptor.RomPath().ToString()).Replace(rootTag, sShareInitRomRoot));
    if (root.Exists() && port != PortTypes::ShareOnly) roots[root.ToString()] = true;

    if (port != PortTypes::ShareInitOnly)
    {
      // Share roms
      root = Path(String(systemDescriptor.RomPath().ToString()).Replace(rootTag, sShareRomRoot));
      // If port are not RO, ensure the Writable folder exists so that copying or downloading content is possible
      if (systemDescriptor.IsPort()) (void)root.CreatePath();

      if (root.Exists()) roots[root.ToString()] = false;
      // External mount points
      for(const Path& externalRoms : mMountPoints)
      {
        root = Path(String(systemDescriptor.RomPath().ToString()).Replace(rootTag, externalRoms.ToString()));
        if (root.Exists())
        {
          std::optional<DeviceMount> mount = mMountPointMonitoring.SizeOf(root);
          bool readOnly = (mount.has_value() && mount->ReadOnly());
          roots[root.ToString()] = readOnly;
        }
      }
    }
  }
  else
  {
    roots[systemDescriptor.RomPath().ToString()] = false;
    { LOG(LogInfo) << "[System] " << systemDescriptor.RomPath().ToString() << " is a standalone folder."; }
  }

  return roots;
}

void SystemManager::CheckAutoScraping(SystemData& system)
{
  class : public IParser
  {
    public:
      void Parse(FileData& game) override
      {
        static String png(LEGACY_STRING(".png"));
        if (game.IsGame())
          if (game.Metadata().Image().IsEmpty())
            if (game.RomPath().Extension().LowerCase() == png)
              game.Metadata().SetImagePath(game.RomPath());
      }
  } autoScraper;

  if ((system.IsAutoScrapable()))
    system.MasterRoot().ParseAllItems(autoScraper, true);
}

void SystemManager::CheckFolderOverriding(SystemData& system)
{
  class : public IParser
  {
    private:
      /*!
       * @brief Get localized text inside a text. Look for [lg] tags to mark start/end of localized texts
       * if the current language is not found, the method looks for [en].
       * if still not found, the whole text is returned
       * @param source Source text
       * @return localized text
       */
      static String LocalizedText(const String& source)
      {
        // Extract preferred language/region
        String locale = RecalboxConf::Instance().GetSystemLanguage().ToLowerCase();

        // Get start
        String key(']');
        key.Append(locale).Append(']');
        int start = source.Find(key);
        if (start < 0)
        {
          String language = (locale.Count() == 5) ? locale.SubString(0, 2) : "en";
          key.Assign('[').Append(language).Append(']');
          start = source.Find(key);
          if (start < 0)
          {
            key = "[en]";
            start = source.Find(key);
            if (start < 0) return source;
          }
        }

        // Get end
        int stop = source.Find('[', start + key.Count());
        if (stop < 0) stop = source.Count();

        // Trimming
        return source.SubString(start, stop - start).Trim();
      }

    public:
      /*!
       * @brief Looks for folder override files in the given folder.
       * If overridden images/texts are found, they are loaded to override empty or gamelist information
       * The methods looks for:
       * - .folder.picture.svg or .folder.picture.png
       * - .folder.description.txt
       * @param game folder data to override
       */
      void Parse(FileData& game) override
      {
        if (!game.IsFolder()) return;
        Path romPath = game.RomPath();
        // Override image
        Path fullPath = romPath / ".folder.picture.png";
        if (!fullPath.Exists()) return;

        game.Metadata().SetVolatileImagePath(fullPath);
        fullPath = romPath / ".folder.description.txt";
        String text = Files::LoadFile(fullPath);
        if (text.empty()) return;

        text = LocalizedText(text);
        if (!text.empty())
          game.Metadata().SetVolatileDescription(text);
      }
  } overrider;

  system.MasterRoot().ParseAllItems(overrider, false);
}

void SystemManager::BuildRegionData(SystemData& system)
{
  class RegionParser : public IParser
  {
    private:
      SystemData& mSystem;
      const FolderData* mLastFolder;
      const ArcadeDatabase* mLastDatabase;

    public:
      explicit RegionParser(SystemData& system)
        : mSystem(system)
        , mLastFolder(nullptr)
        , mLastDatabase(nullptr)
      {
      }

      void Parse(FileData& game) override
      {
        if (game.IsGame() /*&& !game.TopAncestor().ReadOnly()*/)
          if (!game.Metadata().Region().HasRegion())
          {
            if (game.System().IsTrueArcade() && !game.System().ArcadeDatabases().IsEmpty())
            {
              if (const FolderData* folder = game.Parent(); folder != mLastFolder)
              {
                mLastDatabase = mSystem.ArcadeDatabases().LookupDatabase(*folder);
                mLastFolder = folder;
              }
              if (mLastDatabase != nullptr)
              {
                bool nudity = false;
                if (const ArcadeGame* arcade = mLastDatabase->LookupGame(game); arcade != nullptr)
                {
                  game.Metadata().SetRegion(Regions::ExtractRegionsFromArcadeName(arcade->ArcadeName(), nudity));
                  if (nudity) game.Metadata().SetAdult(true);
                }
              }
              else { LOG(LogError) << "[SystemManager] No arcade database for " << game.RomPath(); }
            }
            else
              game.Metadata().SetRegion(Regions::ExtractRegionsFromFileName(game.RomPath()));
          }
      }
  } regionMetadata(system);

  system.MasterRoot().ParseAllItems(regionMetadata, true);
}

void SystemManager::BuildDynamicMetadata(SystemData& system)
{
  class : public IParser
  {
    private:
      //! Mini structure to store a game and its version
      struct VersionedGame
      {
        FileData*              Game;
        Versions::GameVersions Version;
        VersionedGame(FileData& game, Versions::GameVersions version)
          : Game(&game)
          , Version(version)
        {}
      };
      //! Keep the highest versioned FileData instance for a given key (game+regions)
      HashMap<String, VersionedGame> mHighestVersions;

    public:
      void Parse(FileData& game) override
      {
        if (game.IsGame() /*&& !game.TopAncestor().ReadOnly()*/)
        {
          // Highest version
          Path romPath(game.Metadata().RomFileOnly());
          String fileName(romPath.ToString());
          Versions::GameVersions version = Versions::ExtractGameVersionNoIntro(fileName);
          String gameNameWithRegion = romPath.FilenameWithoutDecorations().Append(Regions::Serialize4Regions(Regions::ExtractRegionsFromNoIntroName(fileName)));

          VersionedGame* previous = mHighestVersions.try_get(gameNameWithRegion);
          if (previous == nullptr)
          {
            game.Metadata().SetLatestVersion(true);
            mHighestVersions.insert_or_assign(gameNameWithRegion, VersionedGame(game, version));
          }
          else if (version > previous->Version)
          {
            previous->Game->Metadata().SetLatestVersion(false);
            game.Metadata().SetLatestVersion(true);
            mHighestVersions.insert_or_assign(gameNameWithRegion, VersionedGame(game, version));
          }

          // Disk/Disc/tape number/side
          fileName.LowerCase();
          if (int pos = fileName.Find(LEGACY_STRING("(dis")); pos > 0)
          {
            switch(fileName[pos + 4])
            {
              case 'c': // CD/DVD
              {
                int number = 0;
                if (fileName.TryAsInt(pos +  6, ' ', number) || fileName.TryAsInt(pos +  6, ')', number))
                  game.Metadata().SetSupportIndex(number);
                if (pos = fileName.Find(LEGACY_STRING("of "), pos + 7); pos > 0)
                  if (fileName.TryAsInt(pos +  3, ')', number)) game.Metadata().SetSupportTotal(number);
                break;
              }
              case 'k': // Disk
              {
                int number = 0;
                if (fileName.TryAsInt(pos +  6, ' ', number) || fileName.TryAsInt(pos +  6, ')', number))
                  game.Metadata().SetSupportIndex(number);
                else if (int c = fileName[pos + 6] | 32; c >= 'a' && c <= 'z')
                  game.Metadata().SetSupportIndex(number = c - 'a' + 1);
                if (pos = fileName.Find(LEGACY_STRING("of "), pos + 7); pos > 0)
                {
                  if (fileName.TryAsInt(pos +  3, ')', number)) game.Metadata().SetSupportTotal(number);
                  else if (fileName.TryAsInt(pos +  3, ' ', number))
                  {
                    game.Metadata().SetSupportTotal(number);
                    if (pos = fileName.Find(LEGACY_STRING("side "), pos + 4); pos > 0)
                    {
                      switch(fileName[pos + 5])
                      {
                        case 'a':
                        case '1': game.Metadata().SetSupportSide(SupportSides::A); break;
                        case 'b':
                        case '2': game.Metadata().SetSupportSide(SupportSides::B); break;
                        default: break;
                      }
                    }
                  }
                }
                /*else if (game.Metadata().SupportTotal() < number)
                  game.Metadata().SetSupportTotal(number);*/
                break;
              }
              default: break;
            }
          }
          else if (pos = fileName.Find(LEGACY_STRING("(side ")); pos > 0)
          {
            switch(fileName[pos + 6])
            {
              case 'a':
              case '1': game.Metadata().SetSupportSide(SupportSides::A); break;
              case 'b':
              case '2': game.Metadata().SetSupportSide(SupportSides::B); break;
              default: break;
            }
          }

          game.Metadata().SetSupportType(game.System().Descriptor().LookupSupportType(romPath.Extension()));

          // Not a game?
          game.Metadata().SetNoGame(game.Name().StartsWith(LEGACY_STRING("ZZZ")) || fileName.Contains(LEGACY_STRING("[BIOS]")));
        }
      }
  } dynamicMetadata;

  system.MasterRoot().ParseAllItems(dynamicMetadata, true);
}

void SystemManager::MakeSystemVisible(SystemData* system)
{
  int index = 0;
  for(int i = 0; i < mAllSystems.Count() && mAllSystems[i] != system; ++i)
    if (mAllSystems[i] == mVisibleSystems[index])
      if (++index >= mVisibleSystems.Count())
        break;
  mVisibleSystems.Insert(system, index);
}

void SystemManager::MakeSystemInvisible(SystemData* system)
{
  mVisibleSystems.Remove(system);
}

void SystemManager::InitializeSystem(SystemData* system)
{
  // Initialize only once
  if (system->IsInitialized()) return;

  // Try to populate!
  if (!system->HasGame())
  {
    DateTime start;
    if (system->IsVirtual()) PopulateVirtualSystem(system);
    else PopulateRegularSystem(system);
    { LOG(LogWarning) << "[SystemManager] Populate system " << system->Descriptor().FullName() << " took : " << (DateTime() - start).ToMillisecondsString(); }
  }

  // Zero-game system are not declared initialized - might be later
  if (system->HasGame())
  {
    if (!system->IsVirtual())
    {
      DateTime start;
      // Automatic data completion
      mDataCompleter.Push(system);
      // Game In Png?
      CheckAutoScraping(*system);
      // Overrides?
      CheckFolderOverriding(*system);
      // Dynamic data
      BuildDynamicMetadata(*system);
      // Arcade special processing?
      if (system->Descriptor().IsTrueArcade())
        system->LoadArcadeDatabase();
      // Build region data
      //if (system->IsTrueArcade())
      BuildRegionData(*system);
      { LOG(LogWarning) << "[SystemManager] Compute data for system " << system->Descriptor().FullName() << " took : " << (DateTime() - start).ToMillisecondsString(); }
    }
    // Load theme (not for port games)
    if (!system->IsPorts())
    {
      // Load theme once the Renderer is instantiated since there are dependencies top
      // several display elements (Fonts, etc.)
      if (Renderer::IsInstantiated())
        ThemeManager::Instance().LoadSystemTheme(*system);
    }

    // Set initialised
    system->SetInitialized();
  }
}

void SystemManager::PopulateVirtualSystem(SystemData* system)
{
  switch(system->VirtualType())
  {
    case VirtualSystemType::Ports: PopulatePortsSystem(system); break;
    case VirtualSystemType::Favorites: PopulateFavoriteSystem(system); break;
    case VirtualSystemType::LastPlayed: PopulateLastPlayedSystem(system); break;
    case VirtualSystemType::Multiplayers: PopulateMultiPlayerSystem(system); break;
    case VirtualSystemType::AllGames: PopulateAllGamesSystem(system); break;
    case VirtualSystemType::Lightgun: PopulateLightgunSystem(system); break;
    case VirtualSystemType::Tate: PopulateTateSystem(system); break;
    case VirtualSystemType::Dial: PopulateDialSystem(system); break;
    case VirtualSystemType::Trackball: PopulateTrackballSystem(system); break;
    case VirtualSystemType::Arcade: PopulateArcadeSystem(system); break;
    case VirtualSystemType::Genre: PopulateGenreSystem(system); break;
    case VirtualSystemType::ArcadeManufacturers: PopulateArcadeManufacturersSystem(system); break;
    case VirtualSystemType::None:
    default: { LOG(LogFatal) << "[SystemManager] Trying to populate unknown virtual system type"; break; }
  }
}

void SystemManager::RescanSystem(SystemData* system, ISystemScan* scanInterface)
{
  // No scan on virtual systems
  if (system->IsVirtual()) return;

  if (scanInterface != nullptr)
    scanInterface->SystemScanStart(*system);

  PortTypes port = PortTypes::None;
  if (system->Descriptor().IsPort())
    port = system->Descriptor().IsReadOnly() ? PortTypes::ShareInitOnly : PortTypes::ShareOnly;

  // Build root list
  for(const auto& rootPath : GetRomSource(system->Descriptor(), port))
  {
    RootFolderData& root = system->LookupOrCreateRootFolder(Path(rootPath.first),
                                                            RootFolderData::Ownership::All,
                                                            rootPath.second ? RootFolderData::Types::ReadOnly : RootFolderData::Types::None);
    FileData::StringMap doppelgangerWatcher;
    root.BuildDoppelgangerMap(doppelgangerWatcher, true);
    { LOG(LogInfo) << "[System] Rescan system: " << system->Descriptor().FullName() << " (from " << rootPath.first << ')'; }

    // Populate items from disk
    root.InvalidateAll();
    root.Metadata().Validate();
    system->ScanStorageFolder(root, doppelgangerWatcher, scanInterface);
    // Wipe invalid items
    root.RemoveInvalids(scanInterface);

    #ifdef DEBUG
    { LOG(LogInfo) << "[System] " << root.CountAll(false, FileData::Filter::None) << " games found for " << system->Descriptor().FullName() << " in " << rootPath.first; }
    #endif
  }

  // Refresh what needs to be
  system->RefreshArcadeDatabase();
  mDataCompleter.Push(system);

  // Callback
  if (scanInterface != nullptr)
    scanInterface->SystemScanEnd(*system);
  if (mSystemChangeNotifier != nullptr)
    mSystemChangeNotifier->UpdateSystem(system);

  // Unset dirty rom folders
  mDirtyRomFolders.erase(system);
}

void SystemManager::RescanGamelists(SystemData* system)
{
  // No scan on virtual systems
  if (system->IsVirtual()) return;

  PortTypes port = PortTypes::None;
  if (system->Descriptor().IsPort())
    port = system->Descriptor().IsReadOnly() ? PortTypes::ShareInitOnly : PortTypes::ShareOnly;

  for(const auto& rootPath : GetRomSource(system->Descriptor(), port))
  {
    RootFolderData& root = system->LookupOrCreateRootFolder(Path(rootPath.first),
                                                            RootFolderData::Ownership::All,
                                                            rootPath.second ? RootFolderData::Types::ReadOnly : RootFolderData::Types::None);
    FileData::StringMap doppelgangerWatcher;
    root.BuildDoppelgangerMap(doppelgangerWatcher, true);
    { LOG(LogInfo) << "[System] Reload system gamelist: " << system->Descriptor().FullName() << " (from " << rootPath.first << ')'; }

    // Validate all game already in ram
    root.ValidateAll();
    // Then scan gamelist & userlist
    system->ParseGamelistXml(root, doppelgangerWatcher);
    // Then remove invalid games, even if the user added them manually
    root.RemoveInvalids(nullptr);
  }

  mDirtyGamelists.erase(system);
}

void SystemManager::PopulateRegularSystem(SystemData* system)
{
  // Load system?
  if (!system->MustBeShown() &&           // Hidden system are not loaded yet
      !system->Descriptor().IsPort() &&   // But ports are always loaded
      !system->IsTrueArcade())            // And true arcade systems must me loaded to enable virtual systems
    return;

  PortTypes port = PortTypes::None;
  if (system->Descriptor().IsPort())
    port = system->Descriptor().IsReadOnly() ? PortTypes::ShareInitOnly : PortTypes::ShareOnly;

  // Build root list
  for(const auto& rootPath : GetRomSource(system->Descriptor(), port))
  {
    RootFolderData& root = system->LookupOrCreateRootFolder(Path(rootPath.first),
                                                            RootFolderData::Ownership::All,
                                                            rootPath.second ? RootFolderData::Types::ReadOnly : RootFolderData::Types::None);
    FileData::StringMap doppelgangerWatcher;
    { LOG(LogInfo) << "[System] Populating system: " << system->Descriptor().FullName() << " (from " << rootPath.first << ')'; }

    // Populate items from disk
    bool loadFromDisk = !RecalboxConf::Instance().GetStartupGamelistOnly() || system->IsScreenshots();
    if (loadFromDisk)
    {
      root.InvalidateAll();
      DateTime start;
      system->ScanStorageFolder(root, doppelgangerWatcher, nullptr);
      { LOG(LogWarning) << "[SystemManager] Scanning roms for system " << system->Descriptor().FullName() << " took : " << (DateTime() - start).ToMillisecondsString(); }
      // Unset dirty rom folder
      mDirtyRomFolders.erase(system);
    }

    // Populate items from gamelist.xml except for screenshots
    if (!system->IsScreenshots())
    {
      system->ParseGamelistXml(root, doppelgangerWatcher);
      // Unsert dirty gamelists
      mDirtyGamelists.erase(system);
    }

    // Wipe invalidated games (still in the XML list, but bo longer on disk)
    if (loadFromDisk)
      root.RemoveInvalids(nullptr);

    // Watch all rom folder & gamelists
    system->WatchAllRomFolders();
    system->WatchAllGamelists();
    mHub.AddWatcher(system->mFileWatcher, *this);
    mWatcherToSystem[&system->mFileWatcher] = system;

    #ifdef DEBUG
    { LOG(LogInfo) << "[System] " << root.CountAll(false, FileData::Filter::None) << " games found for " << system->Descriptor().FullName() << " in " << rootPath.first; }
    #endif
  }
}

void SystemManager::PopulateFavoriteSystem(SystemData* system)
{
  FolderData& root = system->LookupOrCreateRootFolder(Path(), RootFolderData::Ownership::None, RootFolderData::Types::Virtual);
  for(const SystemData* regular : mAllSystems)
    if (!regular->IsVirtual())
      if (FileData::List favs = regular->getFavorites(); !favs.Empty())
      {
        for (auto* favorite : favs) root.AddChild(favorite, false);
        { LOG(LogWarning) << "[System]   Get " << favs.Count() << " favorites for " << regular->Name() << "!"; }
      }
}

void SystemManager::RegeneratePortsSystem()
{
  // Lookup system
  for(SystemData* system : mAllSystems)
    if (system->VirtualType() == VirtualSystemType::Ports)
    {
      // Delete all games from the system
      FileData::List games = system->MasterRoot().GetAllItemsRecursively(false, FileData::Filter::None);
      for(FileData* game : games)
        RootFolderData::DeleteChild(game);
      // Recreate inner games
      PopulatePortsSystem(system);
    }
}

void SystemManager::PopulatePortsSystem(SystemData* systemPorts)
{
  if (systemPorts->MustBeShown())
  {
    // Lookup all non-empty arcade platforms
    List ports;
    FileData::StringMap doppelganger;
    for (SystemData* system : mAllSystems)
      if (system->Descriptor().IsPort())
        if (system->HasGame())
        {
          ports.Add(system);
          system->BuildDoppelgangerMap(doppelganger, false);
        }
    // Not empty?
    if (!ports.Empty())
    {
      // Remove port systems from the visible list and add to the hidden list
      for (SystemData* port: ports) MakeSystemInvisible(port);
      // Create meta-system
      PopulateVirtualSystemWithSystem(systemPorts, ports, doppelganger, false);
    }
  }
}

void SystemManager::PopulateLastPlayedSystem(SystemData* systemLastPlayed)
{
  class Filter: public IFilter
  {
    private:
      FileData::TopLevelFilter mFilter;
    public:
      Filter() : mFilter(FileData::BuildTopLevelFilter()) {}
      [[nodiscard]] bool ApplyFilter(const FileData& file) override
      {
        return file.Metadata().LastPlayedEpoc() != 0;
      }
  } filter;

  if (systemLastPlayed->MustBeShown())
    PopulateMetaSystemWithFilter(systemLastPlayed, &filter, nullptr);
}

void SystemManager::PopulateMultiPlayerSystem(SystemData* systemMultiPlayer)
{
  class Filter: public IFilter
  {
    private:
      FileData::TopLevelFilter mFilter;
    public:
      Filter() : mFilter(FileData::BuildTopLevelFilter()) {}
      [[nodiscard]] bool ApplyFilter(const FileData& file) override
      {
        return (file.Metadata().PlayerMin() > 1 || file.Metadata().PlayerMax() > 1);
      }
  } filter;

  if (systemMultiPlayer->MustBeShown())
    PopulateMetaSystemWithFilter(systemMultiPlayer, &filter, nullptr);
}

void SystemManager::PopulateAllGamesSystem(SystemData* systemAllGames)
{
  if (systemAllGames->MustBeShown())
  {
    FileData::StringMap doppelganger;
    for (SystemData* system: mAllSystems)
      if (!system->IsVirtual())
        system->BuildDoppelgangerMap(doppelganger, false);

    PopulateVirtualSystemWithSystem(systemAllGames, mAllSystems, doppelganger, true);
  }
}

void SystemManager::PopulateLightgunSystem(SystemData* systemLightGun)
{
  if (systemLightGun->MustBeShown())
  {
    LightGunDatabase database;
    PopulateMetaSystemWithFilter(systemLightGun, &database, nullptr);
  }
}

void SystemManager::PopulateTateSystem(SystemData* systemTate)
{
  class Filter: public IFilter
  {
    private:
      FileData::TopLevelFilter mFilter;
    public:
      Filter() : mFilter(FileData::BuildTopLevelFilter()) {}
      [[nodiscard]] bool ApplyFilter(const FileData& file) override
      {
        return (file.Metadata().Rotation() == RotationType::Left || file.Metadata().Rotation() == RotationType::Right);
      }
  } filter;

  if (systemTate->MustBeShown())
    PopulateMetaSystemWithFilter(systemTate, &filter, nullptr);
}

void SystemManager::PopulateDialSystem(SystemData* systemDial)
{
  if (systemDial->MustBeShown())
  {
    ControllerDatabase database;
    database.SetFilterType(ControlType::Dial);
    PopulateMetaSystemWithFilter(systemDial, &database, nullptr);
  }
}

void SystemManager::PopulateTrackballSystem(SystemData* systemTrackball)
{
  if (systemTrackball->MustBeShown())
  {
    ControllerDatabase database;
    database.SetFilterType(ControlType::Trackball);
    PopulateMetaSystemWithFilter(systemTrackball, &database, nullptr);
  }
}

void SystemManager::PopulateArcadeSystem(SystemData* systemArcade)
{
  if (systemArcade->MustBeShown())
  {
    List candidates;
    FileData::StringMap doppelganger;
    for (SystemData* arcade: mAllSystems)
      // Check the ignore flag anyway to not add systems that have been explicitely hidden
      // MustBeShown can't be trusted in such case because of 'hide originals"
      if (!RecalboxConf::Instance().GetSystemIgnore(*arcade))
        if (arcade->Descriptor().IsTrueArcade() && (arcade->Descriptor().Name() != "daphne"))
        {
          candidates.Add(arcade);
          // doppleganger must be built using file only
          // Let the virtual system re-create all intermediate folder and destroy them properly
          arcade->BuildDoppelgangerMap(doppelganger, false);
        }

    PopulateVirtualSystemWithSystem(systemArcade, candidates, doppelganger, true);
  }
}

void SystemManager::PopulateGenreSystem(SystemData* systemGenre)
{
  class Filter : public IFilter
  {
    private:
      GameGenres mGenre;
      bool mSubGenre;

    public:
      explicit Filter(GameGenres genre) : mGenre(genre), mSubGenre(Genres::IsSubGenre(genre)) {}
      [[nodiscard]] bool ApplyFilter(const FileData& file) override
      {
        if (mSubGenre) return file.Metadata().GenreId() == mGenre;
        return Genres::TopGenreMatching(file.Metadata().GenreId(), mGenre);
      }
  };

  // Lookup genre
  GameGenres genre = Genres::LookupFromName(String(systemGenre->Name()).Remove(sGenrePrefix));
  if (genre == GameGenres::None) { LOG(LogFatal) << "[SystemManager] Unable to lookup system genre!"; }

  if (systemGenre->MustBeShown())
  {
    Filter filter(genre);
    PopulateMetaSystemWithFilter(systemGenre, &filter, nullptr);
  }
}

void SystemManager::PopulateArcadeManufacturersSystem(SystemData* system)
{
  class Filter : public IFilter
  {
    private:
      const ArcadeDatabaseManager& mArcadeDatabaseManager;
      const ArcadeDatabase* mDatabase;
      const FolderData* mParent;
      String mManufacturerName;
      int mManufacturerIndex;
      HashMap<const ArcadeDatabase*, int> mManufacturerIndexesFastLookup;

    public:
      /*!
       * @brief Constructor
       * @param arcadeDatabaseManager Database manager from witch to retrieve arcade database
       * @param manufacturerName manufacturer name to lookup for games matching this manufacturer
       */
      explicit Filter(const ArcadeDatabaseManager& arcadeDatabaseManager, const String& manufacturerName)
        : mArcadeDatabaseManager(arcadeDatabaseManager)
        , mDatabase(nullptr)
        , mParent(nullptr)
        , mManufacturerName(manufacturerName)
        , mManufacturerIndex(-1)
      {
      }

      /*!
       * @brief Main filter method
       * @param file File to filter
       * @return True of the file is accepted, false if it's rejected
       */
      [[nodiscard]] bool ApplyFilter(const FileData& file) override
      {
        // Update the database regarding the emulator configured for the current Folder
        // After the database is updated, lookup the manufacturer index regarding the manufacturer name.
        // If neither no database, nor manufacturer index is found, ignore the current game
        if (mDatabase == nullptr || mParent != file.Parent())
        {
          if (mDatabase = mArcadeDatabaseManager.LookupDatabase(*(mParent = file.Parent())); mDatabase != nullptr)
          {
            if (int* cacheList = mManufacturerIndexesFastLookup.try_get(mDatabase); cacheList != nullptr) mManufacturerIndex = *cacheList;
            else mManufacturerIndexesFastLookup[mDatabase] = (mManufacturerIndex = mDatabase->RawManufacturerIndexFromName(
                mManufacturerName));
          }
          else return false; // No database for the current folder
        }
        // Valid data?
        if (mManufacturerIndex < 0) return false; // No manufacturer

        // Lookup the current game in the arcade database
        const ArcadeGame* arcade = mDatabase->LookupGame(file);
        // No arcade data for that game, it's an unknown game, ignore it
        if (arcade == nullptr) return false;
        // Not a regular arcade game?
        if (arcade->Hierarchy() == ArcadeGame::Type::Bios) return false;
        // Finally, compare the manufacturer index. If it's the right index, we got a game!
        return arcade->RawManufacturer().Contains(mManufacturerIndex);
      }
  };

  String identifier = system->Name();
  String manufacturer(identifier); manufacturer.Remove(sArcadeManufacturerPrefix).Replace('-', '\\');
  if (system->MustBeShown())
  {
    // Filter and insert items
    FileData::List allGames;
    FileData::StringMap doppelganger;
    for (const SystemData* regular: mAllSystems)
      if (regular->Descriptor().IsTrueArcade())
      {
        Filter filter(regular->ArcadeDatabases(), manufacturer);
        for (const RootFolderData* root: regular->MasterRoot().SubRoots())
          if (!root->Virtual())
          {
            FileData::List list = root->GetFilteredItemsRecursively(&filter, true, true);
            allGames.CopyFrom(list);
          }
        // dopplegagner must be build using file only
        // Let the virtual system re-create all intermediate folder and destroy them properly
        regular->BuildDoppelgangerMap(doppelganger, false);
      }

    // Not empty?
    if (!allGames.Empty())
      PopulateVirtualSystemWithGames(system, allGames, doppelganger);
  }
}

void SystemManager::PopulateMetaSystemWithFilter(SystemData* system, IFilter* filter, FileSorts::Comparer comparer)
{
  // Filter and insert items
  FileData::List allGames;
  FileData::StringMap doppelganger;
  for (SystemData* regular : mAllSystems)
    if (!regular->IsVirtual())
    {
      for (const RootFolderData* root : regular->MasterRoot().SubRoots())
        if (!root->Virtual())
        {
          FileData::List list = root->GetFilteredItemsRecursively(filter, true, true);
          allGames.CopyFrom(list);
        }
      // doppleganger must be built using file only
      // Let the virtual system re-create all intermediate folder and destroy them properly
      regular->BuildDoppelgangerMap(doppelganger, false);
    }

  // Not empty?
  if (!allGames.Empty())
  {
    // Sort if required
    if (comparer != nullptr) FolderData::Sort(allGames, comparer, true);
    // Populate!
    { LOG(LogInfo) << "[System] Populating " << system->FullName() << " meta-system"; }
    PopulateVirtualSystemWithGames(system, allGames, doppelganger);
  }
}

void SystemManager::PopulateVirtualSystemWithSystem(SystemData* system, const List & systems, FileData::StringMap& doppelganger, bool includesubfolder)
{
  RootFolderData& root = system->LookupOrCreateRootFolder(Path(), RootFolderData::Ownership::FolderOnly, RootFolderData::Types::Virtual);
  for(SystemData* source : systems)
    if (!source->IsVirtual())
      if (FileData::List all = includesubfolder ? source->getAllGames() : source->getTopGamesAndFolders(); !all.Empty())
      {
        { LOG(LogWarning) << "[System] Add games from " << source->Name() << " into " << system->FullName(); }
        for (auto* fd : all)
          system->LookupOrCreateGame(root, fd->TopAncestor().RomPath(), fd->RomPath(), fd->Type(), doppelganger);
      }
}

void SystemManager::PopulateVirtualSystemWithGames(SystemData* system, const FileData::List& games, FileData::StringMap& doppelganger)
{
  if (!games.Empty())
  {
    RootFolderData& root = system->CreateRootFolder(Path(), RootFolderData::Ownership::FolderOnly, RootFolderData::Types::Virtual);
    { LOG(LogWarning) << "[System] Add " << games.Count() << " games into " << system->FullName(); }
    for (auto* fd : games)
      system->LookupOrCreateGame(root, fd->TopAncestor().RomPath(), fd->RomPath(), fd->Type(), doppelganger);
  }
}

SystemData* SystemManager::CreateRegularSystem(const SystemDescriptor& systemDescriptor)
{
  // Visiblity
  String ignoreKey = systemDescriptor.Name(); ignoreKey.Append(".ignore");
  SystemData::DynamicVisibilityType isVisible;
  if (systemDescriptor.IsTrueArcade())
    isVisible = [ignoreKey]
    {
      bool arcadeCollectionOn = RecalboxConf::Instance().GetCollectionArcade();
      bool hideOriginals = RecalboxConf::Instance().GetCollectionArcadeHideOriginals();
      return (!arcadeCollectionOn || !hideOriginals) && !RecalboxConf::Instance().AsBool(ignoreKey);
    };
  else if (systemDescriptor.IsPort()) isVisible = [] { return false; }; // Ports are always invisible
  else isVisible = [ignoreKey] { return !RecalboxConf::Instance().AsBool(ignoreKey); };

  SystemData::DynamicBelongingType belonging = [](const FileData* game) { (void)game; return false; };

  // Create system
  SystemData::Properties properties = SystemData::Properties::Searchable;
  if (systemDescriptor.Name() == "pico8") properties |= SystemData::Properties::GameInPng;
  if (systemDescriptor.Name() == "imageviewer") properties = SystemData::Properties::GameInPng | SystemData::Properties::ScreenShots;
  if (systemDescriptor.IsPort()) properties |= SystemData::Properties::Ports;

  SystemData* result = new SystemData(*this, systemDescriptor, isVisible, belonging, properties);
  InitializeSystem(result);

  return result;
}

SystemData* SystemManager::CreateFavoriteSystem()
{
  SystemData::DynamicVisibilityType isVisible = []{ return true; };
  SystemData::DynamicBelongingType belonging = [](const FileData* game) { return game->Metadata().Favorite(); };
  SystemDescriptor descriptor;
  descriptor.SetSystemInformation("235d42c7-af11-49ad-a422-d37b52e3a899", sFavoriteSystemShortName, _("Favorites"))
            .SetPropertiesInformation("virtual", "no", "no", "no", "2020-01-01", "Multiple", false, false, false, "")
            .SetDescriptorInformation("", "", "", sFavoriteSystemShortName, "", "", false, false, false);
  SystemData* result = new SystemData(*this, descriptor, isVisible, belonging, SystemData::Properties::Virtual | SystemData::Properties::AlwaysFlat | SystemData::Properties::Favorite,
                                      MetadataType::Favorite, VirtualSystemType::Favorites);

  return result;
}

SystemData* SystemManager::CreatePortsSystem()
{
  SystemData::DynamicVisibilityType isVisible = [this]{ return RecalboxConf::Instance().GetCollectionPorts() || GetVisibleRegularSystemCount() == 0; };
  SystemData::DynamicBelongingType belonging = [](const FileData* game) { (void)game; return false; };
  SystemDescriptor descriptor;
  descriptor.SetSystemInformation("8cfef2bd-83e8-460b-85e5-432d4efa5257", sPortsSystemShortName, sPortsSystemFullName)
            .SetPropertiesInformation("virtual", "no", "no", "no", "2020-01-01", "Multiple", false, false, false, "")
            .SetDescriptorInformation("", "", "", sPortsSystemShortName, "", "", false, false, false);
  SystemData* result = new SystemData(*this, descriptor, isVisible, belonging, SystemData::Properties::Virtual | SystemData::Properties::Searchable,
                                      MetadataType::None, VirtualSystemType::Ports);
  mPorts = result;
  return result;
}

SystemData* SystemManager::CreateLastPlayedSystem()
{
  SystemData::DynamicVisibilityType isVisible = []{ return RecalboxConf::Instance().GetCollectionLastPlayed(); };
  SystemData::DynamicBelongingType belonging = [](const FileData* game) { return game->Metadata().LastPlayedEpoc() != 0; };
  SystemDescriptor descriptor;
  descriptor.SetSystemInformation("ddf12b17-a336-444a-9813-dd82f0649818", sLastPlayedSystemShortName, sLastPlayedSystemFullName)
            .SetPropertiesInformation("virtual", "no", "no", "no", "2020-01-01", "Multiple", false, false, false, "")
            .SetDescriptorInformation("", "", "", String("auto-").Append(sLastPlayedSystemShortName), "", "", false, false, false);
  SystemData* result = new SystemData(*this, descriptor, isVisible, belonging, SystemData::Properties::Virtual | SystemData::Properties::FixedSort | SystemData::Properties::AlwaysFlat,
                                      MetadataType::LastPlayed, VirtualSystemType::LastPlayed, FileSorts::Sorts::LastPlayedDescending);

  return result;
}

SystemData* SystemManager::CreateMultiPlayerSystem()
{
  SystemData::DynamicVisibilityType isVisible = []{ return RecalboxConf::Instance().GetCollectionMultiplayer(); };
  SystemData::DynamicBelongingType belonging = [](const FileData* game) { return game->Metadata().PlayerMax() > 1; };
  SystemDescriptor descriptor;
  descriptor.SetSystemInformation("c6d89f44-712a-4998-9e09-6fbd7cf10529", sMultiplayerSystemShortName, sMultiplayerSystemFullName)
            .SetPropertiesInformation("virtual", "no", "no", "no", "2020-01-01", "Multiple", false, false, false, "")
            .SetDescriptorInformation("", "", "", String("auto-").Append(sMultiplayerSystemShortName), "", "", false, false, false);
  SystemData* result = new SystemData(*this, descriptor, isVisible, belonging, SystemData::Properties::Virtual, MetadataType::Players, VirtualSystemType::Multiplayers);

  return result;
}

SystemData* SystemManager::CreateAllGamesSystem()
{
  SystemData::DynamicVisibilityType isVisible = []{ return RecalboxConf::Instance().GetCollectionAllGames(); };
  SystemData::DynamicBelongingType belonging = [](const FileData* game) { (void)game; return true; };
  SystemDescriptor descriptor;
  descriptor.SetSystemInformation("cec94df6-a965-41c3-9b51-f223612dc3d9", sAllGamesSystemShortName, sAllGamesSystemFullName)
            .SetPropertiesInformation("virtual", "no", "no", "no", "2020-01-01", "Multiple", false, false, false, "")
            .SetDescriptorInformation("", "", "", String("auto-").Append(sAllGamesSystemShortName), "", "", false, false, false);
  SystemData* result = new SystemData(*this, descriptor, isVisible, belonging, SystemData::Properties::Virtual, MetadataType::None, VirtualSystemType::AllGames);

  return result;
}

SystemData* SystemManager::CreateLightgunSystem()
{
  SystemData::DynamicVisibilityType isVisible = []{ return RecalboxConf::Instance().GetCollectionLightGun(); };
  SystemData::DynamicBelongingType belonging = [](const FileData* game) { (void)game; return false; };
  SystemDescriptor descriptor;
  descriptor.SetSystemInformation("2b6d3653-cd56-4f9a-86c0-62292216242b", sLightgunSystemShortName, sLightgunSystemFullName)
            .SetPropertiesInformation("virtual", "no", "no", "no", "2020-01-01", "Multiple", false, false, false, "")
            .SetDescriptorInformation("", "", "", String("auto-").Append(sLightgunSystemShortName), "", "", false, false, false);
  SystemData* result = new SystemData(*this, descriptor, isVisible, belonging, SystemData::Properties::Virtual | SystemData::Properties::AlwaysFlat, MetadataType::None, VirtualSystemType::Lightgun);

  return result;
}

SystemData* SystemManager::CreateTateSystem()
{
  SystemData::DynamicVisibilityType isVisible = []{ return RecalboxConf::Instance().GetCollectionTate(); };
  SystemData::DynamicBelongingType belonging = [](const FileData* game) { return game->Metadata().Rotation() == RotationType::Left || game->Metadata().Rotation() == RotationType::Right; };
  SystemDescriptor descriptor;
  descriptor.SetSystemInformation("7e5b2ff8-40fe-406d-88bd-d289c95e03f9", sTateSystemShortName, sTateSystemFullName)
            .SetPropertiesInformation("virtual", "no", "no", "no", "2020-01-01", "Multiple", false, false, false, "")
            .SetDescriptorInformation("", "", "", String("auto-").Append(sTateSystemShortName), "", "", false, false, false);
  SystemData* result = new SystemData(*this, descriptor, isVisible, belonging, SystemData::Properties::Virtual | SystemData::Properties::AlwaysFlat, MetadataType::Rotation, VirtualSystemType::Tate);

  return result;
}

SystemData* SystemManager::CreateDialSystem()
{
  SystemData::DynamicVisibilityType isVisible = []{ return RecalboxConf::Instance().GetCollectionDial(); };
  SystemData::DynamicBelongingType belonging = [](const FileData* game) { (void)game; return false; };
  SystemDescriptor descriptor;
  descriptor.SetSystemInformation("b3f9d7a2-1c84-4e6b-9f0a-5d2e8c7a4b16", sDialSystemShortName, sDialSystemFullName)
            .SetPropertiesInformation("virtual", "no", "no", "no", "2020-01-01", "Multiple", false, false, false, "")
            .SetDescriptorInformation("", "", "", String("auto-").Append(sDialSystemShortName), "", "", false, false, false);
  SystemData* result = new SystemData(*this, descriptor, isVisible, belonging, SystemData::Properties::Virtual | SystemData::Properties::AlwaysFlat, MetadataType::None, VirtualSystemType::Dial);

  return result;
}

SystemData* SystemManager::CreateTrackballSystem()
{
  SystemData::DynamicVisibilityType isVisible = []{ return RecalboxConf::Instance().GetCollectionTrackball(); };
  SystemData::DynamicBelongingType belonging = [](const FileData* game) { (void)game; return false; };
  SystemDescriptor descriptor;
  descriptor.SetSystemInformation("c7e4a1b8-2d95-4f7c-8a1b-6e3f9d8b5c27", sTrackballSystemShortName, sTrackballSystemFullName)
            .SetPropertiesInformation("virtual", "no", "no", "no", "2020-01-01", "Multiple", false, false, false, "")
            .SetDescriptorInformation("", "", "", String("auto-").Append(sTrackballSystemShortName), "", "", false, false, false);
  SystemData* result = new SystemData(*this, descriptor, isVisible, belonging, SystemData::Properties::Virtual | SystemData::Properties::AlwaysFlat, MetadataType::None, VirtualSystemType::Trackball);

  return result;
}

SystemData* SystemManager::CreateArcadeSystem()
{
  SystemData::DynamicVisibilityType isVisible = []{ return RecalboxConf::Instance().GetCollectionArcade(); };
  SystemData::DynamicBelongingType belonging = [](const FileData* game) { (void)game; return false; };
  SystemDescriptor descriptor;
  descriptor.SetSystemInformation("a68dedb7-e6b8-4a0b-b10c-1a1a85eec982", sArcadeSystemShortName, sArcadeSystemFullName)
            .SetPropertiesInformation("varcade", "no", "no", "no", "2020-01-01", "Multiple", false, false, false, "")
            .SetDescriptorInformation("", "", "", sArcadeSystemShortName, "", "", false, false, false);

  SystemData::Properties properties = SystemData::Properties::Virtual;
  bool hideOriginals = RecalboxConf::Instance().GetCollectionArcadeHideOriginals();
  if (hideOriginals) properties |= SystemData::Properties::Searchable;

  SystemData* result = new SystemData(*this, descriptor, isVisible, belonging, properties, MetadataType::None, VirtualSystemType::Arcade);

  return result;
}

SystemData* SystemManager::CreateGenreSystem(GameGenres genre)
{
  SystemData::DynamicVisibilityType isVisible = [genre]{ return RecalboxConf::Instance().IsInCollectionGenre(SystemManager::BuildGenreSystemName(genre)); };
  SystemData::DynamicBelongingType belonging = [genre](const FileData* game) { return game->Metadata().GenreId() == genre; };
  SystemDescriptor descriptor;
  String shortName = Genres::GetShortName(genre);
  String fullName = Genres::GetFullName(genre);
  descriptor.SetSystemInformation(String("475b94da-8fbc-488d-82df-554161af2997").Append(shortName), BuildGenreSystemName(genre), fullName)
            .SetPropertiesInformation("virtual", "no", "no", "no", "2020-01-01", "Multiple", false, false, false, "")
            .SetDescriptorInformation("", "", "", String("auto-").Append(shortName), "", "", false, false, false);
  SystemData* result = new SystemData(*this, descriptor, isVisible, belonging, SystemData::Properties::Virtual | SystemData::Properties::AlwaysFlat, MetadataType::GenreId, VirtualSystemType::Genre);

  return result;
}

SystemData* SystemManager::CreateArcadeManufacturersSystem(const String& manufacturer)
{
  String systemName = BuildArcadeManufacturerSystemName(manufacturer);
  SystemData::DynamicVisibilityType isVisible = [systemName]{ return RecalboxConf::Instance().IsInCollectionArcadeManufacturers(systemName); };
  SystemData::DynamicBelongingType belonging = [](const FileData* game) { (void)game; return false; };
  SystemDescriptor descriptor;
  descriptor.SetSystemInformation(String("475b94da-8fbc-488d-82df-554161af2997").Append(manufacturer), systemName, String(manufacturer).Replace('\\', " - "))
            .SetPropertiesInformation("varcade", "no", "no", "no", "2020-01-01", String(manufacturer).Replace('\\', " - "), false, false, false, "")
            .SetDescriptorInformation("", "", "", String("auto-arcade-").Append(manufacturer).Replace('\\','-').Remove("\u00A0"), "", "", false, false, false);
  SystemData* result = new SystemData(*this, descriptor, isVisible, belonging, SystemData::Properties::Virtual | SystemData::Properties::AlwaysFlat, MetadataType::None, VirtualSystemType::ArcadeManufacturers);
  return result;
}

void SystemManager::ManageArcadeVirtualSystem(bool startup)
{
  List added;
  List removed;
  bool arcadeCollectionOn = RecalboxConf::Instance().GetCollectionArcade();
  bool hideOriginals = RecalboxConf::Instance().GetCollectionArcadeHideOriginals();
  // All arcade systems
  for (int i = mAllSystems.Count(); --i >= 0;)
    if (SystemData* arcade = mAllSystems[i]; arcade->Descriptor().IsTrueArcade() && (arcade->Descriptor().Name() != "daphne"))
    {
      bool isVisible = mVisibleSystems.Contains(arcade);
      if (arcadeCollectionOn)
        if (hideOriginals)
         {
          if (isVisible) removed.Add(arcade);
          continue;
        }
      if (arcade->HasVisibleGame()) { if (!isVisible) added.Add(arcade); }
      else if (isVisible) removed.Add(arcade);
    }

  if (!startup)
    ApplySystemChanges(&added, &removed, nullptr, false);
  else
    for(SystemData* systemToHide : removed)
      MakeSystemInvisible(systemToHide);
}

void SystemManager::ManagePortsVirtualSystem()
{
  for (int i = mVisibleSystems.Count(); --i>= 0; )
    if (SystemData* port = mVisibleSystems[i]; port->Descriptor().IsPort())
      MakeSystemInvisible(port);
}

void SystemManager::ThreadPoolTick(int completed, int total)
{
  if (mProgressInterface != nullptr)
  {
    mProgressInterface->SetMaximum(total);
    mProgressInterface->SetProgress(completed);
  }
}

SystemData* SystemManager::ThreadPoolRunJob(SystemDescriptor& systemDescriptor)
{
  try
  {
    SystemData* newSys = CreateRegularSystem(systemDescriptor);
    { LOG(LogWarning) << "[System] Adding \"" << systemDescriptor.Name() << "\" in system list."; }
    return newSys;
  }
  catch(std::exception& ex)
  {
    { LOG(LogError) << "[System] System \"" << systemDescriptor.FullName() << "\" has raised an error. Ignored."; }
    { LOG(LogError) << "[System] Exception: " << ex.what(); }
  }
  return nullptr;
}

// Creates systems from information located in a config file
bool SystemManager::LoadSystemConfigurations(bool portableSystem, bool crt)
{
  // Remove any existing system & save (useful to save boot on game metadata)
  DeleteAllSystems(true, false);

  SystemDeserializer deserializer;
  bool loaded = deserializer.LoadSystems();
  // Is there at least
  if (!loaded)
  {
    { LOG(LogError) << "[System] No systemlist.xml file available!"; }
    return false;
  }

  DescriptorList list;
  for (int index = 0; index < deserializer.Count(); ++index)
    if (SystemDescriptor descriptor; deserializer.Deserialize(index, descriptor))
      if (!crt || descriptor.EmulatorTree().CRTAvailable())
        list.push_back(descriptor);

  return LoadSystems(list, portableSystem, false);
}

bool SystemManager::LoadSingleSystemConfigurations(const String& UUID)
{
  SystemDeserializer deserializer;
  bool loaded = deserializer.LoadSystems();
  // Is there at least
  if (!loaded)
  {
    { LOG(LogError) << "[System] No systemlist.xml file available!"; }
    return false;
  }

  DescriptorList list;
  for (int index = 0; index < deserializer.Count(); ++index)
    if (SystemDescriptor descriptor; deserializer.Deserialize(index, descriptor))
      if (descriptor.GUID() == UUID)
        list.push_back(descriptor);

  return LoadSystems(list, false, true);
}

bool SystemManager::LoadSystems(const DescriptorList& systemList, bool portableSystem, bool novirtuals)
{
  // Phase #1
  NotifyLoadingPhase(ISystemLoadingPhase::Phase::RegularSystems);

  DateTime start;

  // Get weight store
  StringMapFile weights(sWeightFilePath);
  weights.Load();
  // Create automatic thread-pool
  ThreadPool<SystemDescriptor, SystemData*> threadPool(this, "System-Load", false, 20);
  // Push system to process
  mSystemNameToSystemRootPath.clear();
  for (const SystemDescriptor& descriptor : systemList)
  {
    // Store root path
    mSystemNameToSystemRootPath[descriptor.RomPath().ToString()] = descriptor.Extension();
    // Get weight
    int weight = weights.GetInt(descriptor.FullName(), 0);
    // Push weighted system
    threadPool.PushFeed(descriptor, weight);
  }

  // Initialize external mount points first so that they can be used in system loading
  InitializeMountPoints();

  // Run the threadpool and automatically wait for all jobs to complete
  int count = threadPool.PendingJobs();
  if (mProgressInterface != nullptr) mProgressInterface->SetMaximum(count);
  #ifdef SLOW_LOADING
  threadPool.Run(1, false);
  #else
  threadPool.Run(-2, false);
  #endif
  // Push result
  { LOG(LogInfo) << "[System] Store visible systems"; }
  int index = 0;
  // Shrink & update weights
  mAllSystems.ExpandTo(count);
  mVisibleSystems.Clear();
  for(SystemData* result = nullptr; threadPool.PopResult(result, index); )
    if (result != nullptr)
    {
      mAllSystems(index) = result;
      if (result->HasVisibleGame()) mVisibleSystems.Add(result);
      weights.SetInt(result->FullName(), result->CountAll());
    }
  { LOG(LogInfo) << "[System] Final non-virtual visible systems: " << mAllSystems.Count(); }
  weights.Save();

  DateTime stop;
  { LOG(LogInfo) << "[System] Gamelist load time: " << String((stop-start).TotalMilliseconds()) << "ms"; }

  // Cleanup metadata
  MetadataDescriptor::CleanupHolders();

  // Phase #2
  NotifyLoadingPhase(ISystemLoadingPhase::Phase::VirtualSystems);

  if (!novirtuals)
  {
    // Load virtual systems
    LoadVirtualSystems(systemList, portableSystem);

    // Remove required systems
    ManagePortsVirtualSystem();
    ManageArcadeVirtualSystem(true);
  }

  // Sort systems based on conf option
  mOriginalOrderedSystems = mAllSystems;
  SystemSorting();

  // Phase #3
  NotifyLoadingPhase(ISystemLoadingPhase::Phase::Completed);
  ThreadPoolTick(1, 1);

  // Start the data completer
  mDataCompleter.Start();

  return mVisibleSystems.Count() != 0;
}

bool SystemManager::VirtualSystemNeedRefresh(const DescriptorList& systemList, VirtualSystemType type)
{
  switch(type)
  {
    case VirtualSystemType::Favorites:
    case VirtualSystemType::LastPlayed:
    case VirtualSystemType::Multiplayers:
    case VirtualSystemType::AllGames:
    case VirtualSystemType::Tate:
    case VirtualSystemType::Genre:
    case VirtualSystemType::Lightgun: return true;
    case VirtualSystemType::Dial:
    case VirtualSystemType::Trackball:
    {
      for(const SystemDescriptor& descriptor : systemList)
        if (descriptor.IsTrueArcade())
          return true;
      return false;
    }
    case VirtualSystemType::Ports:
    {
      for(const SystemDescriptor& descriptor : systemList)
        if (descriptor.IsPort())
          return true;
      return false;
    }
    case VirtualSystemType::Arcade:
    case VirtualSystemType::ArcadeManufacturers:
    {
      for(const SystemDescriptor& descriptor : systemList)
        if (descriptor.IsTrueArcade())
          return true;
      return false;
    }
    case VirtualSystemType::None:
    default: break;
  }
  return false;
}

void SystemManager::LoadVirtualSystems(const DescriptorList& systemList, bool portableSystem)
{
  // Create automatic thread-pool
  ThreadPool<VirtualSystemDescriptor, VirtualSystemResult> threadPool(this, "Virtual-Load", false, 20);

  int priority = -1;
  int arcadeIndex = -100;
  int genreIndex = -200;

  // Add ports first so that we can move it at the right place
  if (VirtualSystemNeedRefresh(systemList, VirtualSystemType::Ports))
    threadPool.PushFeed(VirtualSystemDescriptor(VirtualSystemType::Ports, -1), ++priority);
  // Start with arcade manufacturer systems, which load slower than the others
  if (VirtualSystemNeedRefresh(systemList, VirtualSystemType::ArcadeManufacturers))
    for(const String& manufacturer : ArcadeVirtualSystems::GetVirtualArcadeSystemList())
      threadPool.PushFeed(VirtualSystemDescriptor(manufacturer, ++arcadeIndex), ++priority);
  // Add genre systems
  if (VirtualSystemNeedRefresh(systemList, VirtualSystemType::Genre))
    for(GameGenres genre : Genres::GetOrderedList())
      threadPool.PushFeed(VirtualSystemDescriptor(genre, ++genreIndex), ++priority);
  // Add favorites
  if (VirtualSystemNeedRefresh(systemList, VirtualSystemType::Favorites))
    threadPool.PushFeed(VirtualSystemDescriptor(VirtualSystemType::Favorites, -2), ++priority);
  // Add Last played
  if (VirtualSystemNeedRefresh(systemList, VirtualSystemType::LastPlayed))
    threadPool.PushFeed(VirtualSystemDescriptor(VirtualSystemType::LastPlayed, -3), ++priority);
  // Add Multiplayers
  if (VirtualSystemNeedRefresh(systemList, VirtualSystemType::Multiplayers))
    threadPool.PushFeed(VirtualSystemDescriptor(VirtualSystemType::Multiplayers, -4), ++priority);
  // Add all games
  if (VirtualSystemNeedRefresh(systemList, VirtualSystemType::AllGames))
    threadPool.PushFeed(VirtualSystemDescriptor(VirtualSystemType::AllGames, -5), ++priority);
  // Add lightgun
  if (!portableSystem)
    if (VirtualSystemNeedRefresh(systemList, VirtualSystemType::Lightgun))
      threadPool.PushFeed(VirtualSystemDescriptor(VirtualSystemType::Lightgun, -6), ++priority);
  // Add Arcade
  if (VirtualSystemNeedRefresh(systemList, VirtualSystemType::Arcade))
    threadPool.PushFeed(VirtualSystemDescriptor(VirtualSystemType::Arcade, -7), ++priority);
  // Add Tate
  if (VirtualSystemNeedRefresh(systemList, VirtualSystemType::Tate))
    threadPool.PushFeed(VirtualSystemDescriptor(VirtualSystemType::Tate, -8), ++priority);
  // Add Dial
  if (VirtualSystemNeedRefresh(systemList, VirtualSystemType::Dial))
    threadPool.PushFeed(VirtualSystemDescriptor(VirtualSystemType::Dial, -9), ++priority);
  // Add Trackball
  if (VirtualSystemNeedRefresh(systemList, VirtualSystemType::Trackball))
    threadPool.PushFeed(VirtualSystemDescriptor(VirtualSystemType::Trackball, -10), ++priority);

  if (mProgressInterface != nullptr)
    mProgressInterface->SetMaximum(priority);

  // Run !
  #ifdef SLOW_LOADING
  threadPool.Run(1, false);
  #else
  threadPool.Run(-2, false);
  #endif

  // Pop results
  int index = 0;
  std::vector<VirtualSystemResult> sortedVirtualSystem;
  for(VirtualSystemResult result {.mNewSystem=nullptr, .mIndex=0 }; threadPool.PopResult(result, index); )
    sortedVirtualSystem.push_back(result);
  std::ranges::sort(sortedVirtualSystem, [] (const VirtualSystemResult& a, const VirtualSystemResult& b) { return a.mIndex < b.mIndex; });

  for(const VirtualSystemResult& result : sortedVirtualSystem)
  {
    mAllSystems.Add(result.mNewSystem);
    if (result.mNewSystem->HasVisibleGame()) mVisibleSystems.Add(result.mNewSystem);
  }
}

VirtualSystemResult SystemManager::ThreadPoolRunJob(VirtualSystemDescriptor& virtualDescriptor)
{
  SystemData* system = nullptr;

  switch(virtualDescriptor.Type())
  {
    case VirtualSystemType::Ports: { system = CreatePortsSystem(); break; }
    case VirtualSystemType::Favorites: { system = CreateFavoriteSystem(); break; }
    case VirtualSystemType::LastPlayed: { system = CreateLastPlayedSystem(); break; }
    case VirtualSystemType::Multiplayers: { system = CreateMultiPlayerSystem(); break; }
    case VirtualSystemType::AllGames: { system = CreateAllGamesSystem(); break; }
    case VirtualSystemType::Lightgun: { system = CreateLightgunSystem(); break; }
    case VirtualSystemType::Arcade: { system = CreateArcadeSystem(); break; }
    case VirtualSystemType::Tate: { system = CreateTateSystem(); break; }
    case VirtualSystemType::Dial: { system = CreateDialSystem(); break; }
    case VirtualSystemType::Trackball: { system = CreateTrackballSystem(); break; }
    case VirtualSystemType::Genre: { system = CreateGenreSystem(virtualDescriptor.Genre()); break; }
    case VirtualSystemType::ArcadeManufacturers: { system = CreateArcadeManufacturersSystem(virtualDescriptor.ArcadeManufacturer()); break; }
    case VirtualSystemType::None:
    default: break;
  }

  // Initialize
  if (system != nullptr) InitializeSystem(system);
  else { LOG(LogFatal) << "[SystemManager] Unprocessed virtual descriptor!"; }

  return { .mNewSystem=system, .mIndex=virtualDescriptor.Index() };
}

SystemData& SystemManager::GetOrCreateSystem(const SystemDescriptor& descriptor)
{
  // Seek for existing system
  for(SystemData* system : mAllSystems)
    if (system->Descriptor().GUID() == descriptor.GUID())
      return *system;

  { LOG(LogFatal) << "Unknown system UUID " << descriptor.GUID(); }
  __builtin_unreachable();
}

bool SystemManager::ThreadPoolRunJob(SystemData*& feed)
{
  // Save changed game data back to xml
  feed->UpdateGamelistXml();

  return true;
}

void SystemManager::UpdateAllGameLists()
{
  DateTime start;

  if (mProgressInterface != nullptr)
    mProgressInterface->SetMaximum(mAllSystems.Count());
  // Create automatic thread-pool
  ThreadPool<SystemData*, bool> threadPool(this, "System-Save", false, 20);
  // Push system to process
  for(SystemData* system : mAllSystems)
  {
    if (system->IsScreenshots()) system->RemoveGamelistXml();
    else if (!system->IsVirtual()) threadPool.PushFeed(system, 0);
  }
  // Run the threadpool and automatically wait for all jobs to complete
  if (threadPool.PendingJobs() != 0)
    threadPool.Run(-2, false);

  DateTime stop;
  { LOG(LogInfo) << "[System] Gamelist update time: " << String((stop-start).TotalMilliseconds()) << "ms"; }
}

void SystemManager::DeleteAllSystems(bool updateGamelists, bool stopDataCompleter)
{
  if (stopDataCompleter)
    mDataCompleter.StopImmediately();

  if (updateGamelists && !mAllSystems.Empty())
    UpdateAllGameLists();

  // Delete virtual system first
  // since referenced item may be accesses from both regular & virtual systems during destruction
  for(int i = mAllSystems.Count(); --i >= 0; )
    if (SystemData*& system = mAllSystems(i); system->IsVirtual())
    {
      { LOG(LogTrace) << "[SystemManager] Destroying virtual system: " << system->Name(); }
      delete system;
      system = nullptr;
    }

  // Then delete regular systems
  for(SystemData* system : mAllSystems)
    if (system != nullptr && !system->IsVirtual())
    {
      { LOG(LogTrace) << "[SystemManager] Destroying regular system: " << system->Name(); }
      delete system;
    }

  mVisibleSystems.Clear();
  mAllSystems.Clear();
}

SystemData* SystemManager::VirtualArcadeManufacturerSystemByName(const String& name)
{
  for(SystemData* system : mAllSystems)
    if (system->VirtualType() == VirtualSystemType::ArcadeManufacturers)
      if (system->Name() == name)
        return system;
  return nullptr;
}

SystemData* SystemManager::VirtualGenreSystemByGenre(GameGenres genre)
{
  String shortname(sGenrePrefix); shortname.Append(Genres::GetShortName(genre));
  for(SystemData* system : mAllSystems)
    if (system->VirtualType() == VirtualSystemType::Genre)
      if (system->Name() == shortname)
        return system;
  return nullptr;
}

SystemData* SystemManager::VirtualSystemByType(VirtualSystemType type)
{
  for(SystemData* system : mAllSystems)
    if (system->VirtualType() == type)
      return system;
  return nullptr;
}

SystemData *SystemManager::SystemByName(const String &name)
{
  for(SystemData* system : mAllSystems)
    if (system->Name() == name)
      return system;
  return nullptr;
}

SystemData *SystemManager::FavoriteSystem()
{
  for(SystemData* system : mAllSystems)
    if (system->IsFavorite())
      return system;
  return nullptr;
}

int SystemManager::getVisibleSystemIndex(const String &name)
{
  for(int i = mVisibleSystems.Count(); --i >= 0; )
    if (mVisibleSystems[i]->Name() == name)
      return i;
  return -1;
}

SystemData* SystemManager::FirstNonEmptySystem()
{
  for(SystemData* system : mVisibleSystems)
    if (system->HasVisibleGame())
      return system;

  return nullptr;
}

FileData::List SystemManager::SearchTextInGames(FolderData::FastSearchContext context, const String& originaltext, int maxglobal, const SystemData* targetSystem)
{
  // Everything to lowercase cause search is not case-sensitiv
  String lowercaseText = originaltext.ToLowerCaseUTF8();

  // Fast search into metadata, collecting index and distances
  { LOG(LogDebug) << "[Search] Start searching for '" << lowercaseText << '\''; }
  StringHolderTypes::FoundTextList resultIndexes(1024, 1024);
  switch(context)
  {
    case FolderData::FastSearchContext::Name       : MetadataDescriptor::SearchInNames(lowercaseText, resultIndexes, (int)FolderData::FastSearchContext::Name); break;
    case FolderData::FastSearchContext::Licences   : MetadataDescriptor::SearchInNames(lowercaseText, resultIndexes, (int)FolderData::FastSearchContext::Licences); break;
    case FolderData::FastSearchContext::Path       : MetadataDescriptor::SearchInPath(lowercaseText, resultIndexes, (int)FolderData::FastSearchContext::Path); break;
    case FolderData::FastSearchContext::Description: MetadataDescriptor::SearchInDescription(lowercaseText, resultIndexes, (int)FolderData::FastSearchContext::Description); break;
    case FolderData::FastSearchContext::Developer  : MetadataDescriptor::SearchInDeveloper(lowercaseText, resultIndexes, (int)FolderData::FastSearchContext::Developer); break;
    case FolderData::FastSearchContext::Publisher  : MetadataDescriptor::SearchInPublisher(lowercaseText, resultIndexes, (int)FolderData::FastSearchContext::Publisher); break;
    case FolderData::FastSearchContext::All        :
    {
      MetadataDescriptor::SearchInNames(lowercaseText, resultIndexes, (int)FolderData::FastSearchContext::Name);
      MetadataDescriptor::SearchInPath(lowercaseText, resultIndexes, (int)FolderData::FastSearchContext::Path);
      MetadataDescriptor::SearchInDescription(lowercaseText, resultIndexes, (int)FolderData::FastSearchContext::Description);
      MetadataDescriptor::SearchInDeveloper(lowercaseText, resultIndexes, (int)FolderData::FastSearchContext::Developer);
      MetadataDescriptor::SearchInPublisher(lowercaseText, resultIndexes, (int)FolderData::FastSearchContext::Publisher);
      break;
    }
    default: break;
  }
  { LOG(LogDebug) << "[Search] Found " << resultIndexes.Count() << " matching file/folders"; }

  // Remove dups by index. Higher index are removed so that the lowest distances remain
  resultIndexes.Sort([](const StringHolderTypes::IndexAndDistance& a, const StringHolderTypes::IndexAndDistance& b) -> int { return a.Index - b.Index; });
  for(int i = resultIndexes.Count() - 1; --i >= 0;)
    if (resultIndexes(i).Index == resultIndexes(i + 1).Index)
    {
      resultIndexes(i + 1) = resultIndexes[resultIndexes.Count() - 1];
      resultIndexes.TruncateTo(resultIndexes.Count() - 1);
    }
  { LOG(LogDebug) << "[Search] " << resultIndexes.Count() << " remaining matching file/folders after removing duplicates"; }
  // Sort first: Distance
  resultIndexes.Sort([](const StringHolderTypes::IndexAndDistance& a, const StringHolderTypes::IndexAndDistance& b) -> int { return a.Distance - b.Distance; });

  // Build searchable system list
  Array<const SystemData*> searchableSystems(mVisibleSystems.Count());
  if (targetSystem != nullptr) searchableSystems.Add(targetSystem);
  else
    for(SystemData* system : mVisibleSystems)
      if (system->IsSearchable())
        searchableSystems.Add(system);
  { LOG(LogDebug) << "[Search] Lookup in " << searchableSystems.Count() << " systems"; }

  // Build Item series
  DateTime start;
  CreateFastSearchCache(resultIndexes, searchableSystems, context);
  { LOG(LogDebug) << "[Search] Fast lookup cache built in " << ((DateTime() - start).TotalMilliseconds()) << "ms"; }

  // Collect result
  struct IntermediateResult { int distance; FileData* game; };
  std::vector<IntermediateResult> intermediateResults;
  FileData::List results;
  FileData::TopLevelFilter filter = FileData::BuildTopLevelFilter();
  for(int i = -1; ++i < resultIndexes.Count(); )
  {
    const StringHolderTypes::IndexAndDistance& resultIndex = resultIndexes(i);
    FolderData::FastSearchItemSerie& serie = mFastSearchSeries[resultIndex.Context];
    // A valid chain never exceeds the serie item count: stopping earlier means a corrupted or cyclic chain
    int guard = serie.Count();
    for(FolderData::FastSearchItem* item = serie.Get(resultIndex.Index); item != nullptr && --guard >= 0; item = serie.Next(item))
      if (item->Game != nullptr && item->Game->IsDisplayable(filter)) intermediateResults.push_back({.distance=resultIndex.Distance, .game=(FileData*)item->Game });
    if ((int)intermediateResults.size() >= maxglobal) break;
  }
  { LOG(LogDebug) << "[Search] Final result: " << intermediateResults.size() << " games"; }

  // Final sort & copy
  std::ranges::sort(intermediateResults, [](const IntermediateResult& a, const IntermediateResult& b) { return a.distance == b.distance ? a.game->Name() < b.game->Name() : a.distance < b.distance;  });
  for(const IntermediateResult& inter : intermediateResults)
    results.Add(inter.game);

  return results;
}

void SystemManager::SystemSorting()
{
  // Sort All systems
	switch (RecalboxConf::Instance().GetSystemSorting())
  {
    case SystemSorting::Name:                                       { mAllSystems.Sort(SortingName); break; }
    case SystemSorting::ReleaseDate:                                { mAllSystems.Sort(SortingReleaseDate); break; }
    case SystemSorting::SystemTypeThenName:                         { mAllSystems.Sort(Sorting1Type2Name); break; }
    case SystemSorting::SystemTypeThenReleaseDate:                  { mAllSystems.Sort(Sorting1Type2ReleaseDate); break; }
    case SystemSorting::ManufacturerThenName:                       { mAllSystems.Sort(Sorting1Manufacturer2Name); break; }
    case SystemSorting::ManufacturerThenReleaseData:                { mAllSystems.Sort(Sorting1Manufacturer2ReleaseDate); break; }
    case SystemSorting::SystemTypeThenManufacturerThenName:         { mAllSystems.Sort(Sorting1Type2Manufacturer3Name); break; }
    case SystemSorting::SystemTypeThenManufacturerThenReleasdeDate: { mAllSystems.Sort(Sorting1Type2Manufacturer3ReleaseDate); break; }
    case SystemSorting::Default:
    default: mAllSystems = mOriginalOrderedSystems; break;
  }

  // Rebuild visible system
  List visibles = mVisibleSystems;
  mVisibleSystems.Clear();
  for(SystemData* system : mAllSystems)
    if (visibles.Contains(system))
      mVisibleSystems.Add(system);
}

void SystemManager::CreateFastSearchCache(const StringHolderTypes::FoundTextList& resultIndexes, const Array<const SystemData*>& searchableSystems, FolderData::FastSearchContext context)

{
  // Calculate searchable system hash
  unsigned int hash = 0;
  for(int i = searchableSystems.Count(); --i >= 0; )
  {
    const SystemData& system = *searchableSystems[i];
    const String key = String(system.FullName().c_str()).Append((int)context);
    hash = crc32_16bytes(key.c_str(), key.size(), hash);
  }

  // Refresh hash? if system list OR context change
  if (hash != mFastSearchCacheHash || mFastSearchSeries.empty())
  {
    mFastSearchCacheHash = hash;
    { LOG(LogDebug) << "[Search] Fast cache refresh required"; }
    mFastSearchSeries.clear();
    for(int i = (int)FolderData::FastSearchContext::All + 1; --i >= 0; )
      mFastSearchSeries.push_back(FolderData::FastSearchItemSerie(0));
  }

  // Build series of contexts not built yet, even when the hash is unchanged:
  // in All context, a new search may hit contexts never seen in previous searches
  for(int i = resultIndexes.Count(); --i >= 0; )
    if (mFastSearchSeries[resultIndexes[i].Context].Empty())
    {
      int count = 0;
      switch((FolderData::FastSearchContext)resultIndexes[i].Context)
      {
        case FolderData::FastSearchContext::Name:
        case FolderData::FastSearchContext::Licences: count = MetadataDescriptor::NameIndexCount(); break;
        case FolderData::FastSearchContext::Path: count = MetadataDescriptor::FileIndexCount(); break;
        case FolderData::FastSearchContext::Description: count = MetadataDescriptor::DescriptionIndexCount(); break;
        case FolderData::FastSearchContext::Developer: count = MetadataDescriptor::DeveloperIndexCount(); break;
        case FolderData::FastSearchContext::Publisher: count = MetadataDescriptor::PublisherIndexCount(); break;
        case FolderData::FastSearchContext::All:break;
      }
      FolderData::FastSearchItemSerie serie(count);
      for(int s = searchableSystems.Count(); --s >= 0; )
        searchableSystems[s]->BuildFastSearchSeries(serie, (FolderData::FastSearchContext)resultIndexes[i].Context);
      mFastSearchSeries[resultIndexes[i].Context] = std::move(serie);
    }
}

void SystemManager::DeleteFastSearchCache()
{
  mFastSearchSeries.clear();
  // Invalidate the hash too, otherwise the next search skips the rebuild and accesses the cleared vector
  mFastSearchCacheHash = 0;
}

void SystemManager::NotifyDeviceUnmount(const DeviceMount& mountpoint)
{
  for(SystemData* system : mAllSystems)
    for (const RootFolderData* root: system->MasterRoot().SubRoots())
      if (root->RomPath().StartWidth(mountpoint.MountPoint()))
        if (root->HasGame())
        {
          { LOG(LogWarning) << "[SystemManager] " << mountpoint.MountPoint().ToString() << " used at least in " << system->FullName(); }
          mRomFolderChangeNotificationInterface.RomPathRemoved(mountpoint);
          return;
        }
}

void SystemManager::NotifyDeviceMount(const DeviceMount& mountpoint)
{
  Path romPath;
  switch(CheckMountPoint(mountpoint.MountPoint(), romPath, mountpoint.ReadOnly()))
  {
    case RomStructure::None:
    {
      { LOG(LogWarning) << "[SystemManager] " << mountpoint.MountPoint().ToString() << " does not contain any known rom folder"; }
      mRomFolderChangeNotificationInterface.NoRomPathFound(mountpoint);
      break;
    }
    case RomStructure::Filled:
    {
      { LOG(LogInfo) << "[SystemManager] " << mountpoint.MountPoint().ToString() << " contains rom folder " << romPath.ToString(); }
      mRomFolderChangeNotificationInterface.RomPathAdded(mountpoint);
      break;
    }
    case RomStructure::Empty:
    {
      { LOG(LogInfo) << "[SystemManager] " << mountpoint.MountPoint().ToString() << " contains empty rom folder " << romPath.ToString(); }
      break;
    }
    default: break;
  }
}

bool SystemManager::HasFileWithExt(const Path& path, HashSet<String>& extensionSet)
{
  DIR* dir = opendir(path.ToChars());
  if (dir == nullptr) return false;
  bool found = false;
  while(!found)
    if (const struct dirent* entry = readdir(dir); entry != nullptr)
    {
      // File and extension in list?
      if (entry->d_type == DT_REG)
        found = (extensionSet.contains(Path(entry->d_name).Extension().LowerCase()));
      else if (entry->d_type == DT_DIR)
        if (entry->d_name[0] != '.')
          found = HasFileWithExt(path / entry->d_name, extensionSet);
    }
    else break;
  closedir(dir);

  return found;
}

SystemManager::RomStructure SystemManager::CheckMountPoint(const Path& root, Path& outputRomPath, bool readonly)
{
  static const Path pathes[] =
  {
    Path("recalbox/roms"),
    Path("roms"),
    Path(""),
  };

  { LOG(LogDebug) << "[SystemManager] Analysing mount point: " << root.ToString(); }
  RomStructure result = RomStructure::None;

  // Check known inner path
  for(const Path& romPath : pathes)
    if (Path main = root / romPath; main.Exists())
      for(const auto& iterator : mSystemNameToSystemRootPath) // Then check system path
        if (String rawPath(iterator.first); rawPath.Contains(sRootTag)) // Ignore absolute path
          if (Path systemPath(rawPath.Replace(sRootTag, main.ToString())); systemPath.Exists())
          {
            LOG(LogTrace) << "[SystemManager] " << systemPath.ToString() << " found.";
            result = RomStructure::Empty;
            HashSet<String> extensions;
            for(const String& ext : iterator.second.Split(' ')) extensions.insert(ext.ToTrim());
            if (HasFileWithExt(systemPath, extensions))
            {
              { LOG(LogInfo) << "[SystemManager] " << systemPath.ToString() << " identified as a valid rom folder"; }
              outputRomPath = main;
              if (!readonly) CompleteRomStructure(main);
              return RomStructure::Filled;
            }
            { LOG(LogInfo) << "[SystemManager] " << systemPath.ToString() << " is a valid but empty rom folder"; }
          }

  return result;
}

void SystemManager::CompleteRomStructure(const Path& target)
{
  static Path shareInit("/recalbox/share_init/roms");
  static Path shareUpgrade("/recalbox/share_upgrade/roms");

  for(const auto& it : mSystemNameToSystemRootPath)
  {
    Path targetRoms(String(it.first).Replace(sRootTag, target.ToString()));
    Path shareInitRoms(String(it.first).Replace(sRootTag, shareInit.ToString()));
    Path shareUpgradeRoms(String(it.first).Replace(sRootTag, shareUpgrade.ToString()));
    // missing path ?
    bool exists = false;
    if (exists = targetRoms.Exists(); !exists)
    {
      if (exists = targetRoms.CreatePath(); exists) { LOG(LogInfo) << "[SystemManager] External share roms folder created: " << targetRoms; }
      else { LOG(LogError) << "[SystemManager] Cannot create external share folder " << targetRoms; }
    }
    // copy helpful files
    if (exists)
    {
      // Get files to copy from from upgrade
      Path::PathList list = shareUpgradeRoms.GetDirectoryContent(true);
      for(const Path& path : list)
        if (bool dummy; path.IsDirectory())
        {
          // create subdirectories
          if (Path subfolder(target / path.MakeRelative(shareUpgrade, dummy)); !subfolder.Exists())
          {
            if (subfolder.CreatePath()) { LOG(LogInfo) << "[SystemManager] External share subfolder created: " << subfolder; }
            else { LOG(LogError) << "[SystemManager] Cannot create external share subfolder " << subfolder; }
          }
        }
        else
        {
          // Readme ?
          if (String fileName(path.Filename()); fileName.StartsWith('_') || fileName == ".p2k.cfg")
          {
            if (Path targetFile(target / path.MakeRelative(shareUpgrade, dummy)); !targetFile.Exists())
            {
              if (Files::CopyFile(path, targetFile)) { LOG(LogInfo) << "[SystemManager] File copied to " << targetFile; }
              else { LOG(LogError) << "[SystemManager] Cannot create/copy file to " << targetFile; }
            }
          }
        }
    }
  }
}

void SystemManager::InitializeMountPoints()
{
  { LOG(LogDebug) << "[SystemManager] Initialize mount points"; }

  Path romPath;
  for(const DeviceMount& root : mMountPointMonitoring.MountPoints())
    if (CheckMountPoint(root.MountPoint(), romPath, root.ReadOnly()) == RomStructure::Filled)
    {
      mMountPoints.push_back(romPath);
      { LOG(LogDebug) << "[SystemManager] Got: " << romPath.ToString() << " as a valid rom folder";  }
    }
}

bool SystemManager::CreateRomFoldersIn(const DeviceMount& device)
{
  Path shareUpgrade(sShareUpgradeRomRoot);
  { LOG(LogDebug) << "[SystemManager] Initializing USB device rom structure: " << device.MountPoint().ToString();  }
  Path::PathList romFolders = shareUpgrade.GetDirectoryContent();
  // Create share folder
  bool error = device.ReadOnly();
  if (!error)
    for(const Path& romFolder : romFolders)
      if (romFolder.IsDirectory())
      {
        Path destination = device.MountPoint() / "recalbox/roms" / romFolder.Filename();
        if (error = !destination.CreatePath(); error) break;
        // Copy
        Path::PathList templates = romFolder.GetDirectoryContent();
        for(const Path& file : templates)
          if (file.Extension() == ".txt")
            if (file.Filename().StartsWith('_'))
              Files::SaveFile(destination / file.Filename(), Files::LoadFile(file));
      }

  return !error;
}

FileData* SystemManager::LookupGameByFilePath(const String& filePath)
{
  for(const SystemData* system : mAllSystems)
  {
    FileData* result = system->MasterRoot().LookupGameByFilePath(filePath);
    if (result != nullptr) return result;
  }
  return nullptr;
}

int SystemManager::GameCount() const
{
  int result = 0;
  int favorites = 0;
  int hidden = 0;
  for(const SystemData* system : mVisibleSystems)
    if (!system->IsVirtual())
      result += system->GameCount(favorites, hidden);
  return result;
}

void SystemManager::UpdateSystemsOnGameChange(FileData* target, MetadataType changes, bool deleted)
{
  if (deleted)
  {
    List removedSystems;
    List modifiedSystems;
    if (UpdateSystemsOnGameDeletion(target, removedSystems, modifiedSystems))
      ApplySystemChanges(nullptr, &removedSystems, &modifiedSystems, false);
    return;
  }
  if (changes != MetadataType::None)
  {
    List addedSystems;
    List removedSystems;
    List modifiedSystems;
    // Single game move?
    if (target != nullptr)
    {
      if (UpdateSystemsOnSingleGameChanges(target, changes, addedSystems, removedSystems, modifiedSystems))
        ApplySystemChanges(&addedSystems, &removedSystems, &modifiedSystems, false);
      return;
    }
    // Multiple game move
    if (UpdateSystemsOnMultipleGameChanges(changes, addedSystems, removedSystems, modifiedSystems))
      ApplySystemChanges(&addedSystems, &removedSystems, &modifiedSystems, false);
    return;
  }
  { LOG(LogError) << "[SystemManager] UpdateSystem called w/o any changes!"; }
}

bool SystemManager::UpdateSystemsOnGameDeletion(FileData* target, List& removedSystems, List& modifiedSystems)
{
  bool result = false;

  for(SystemData* system : mAllSystems)
    // Only virtual system have to be modified
    if (system->IsVirtual())
      // Game has been removed from this system?
      if (system->MasterRoot().RemoveChildRecursively(target))
      {
        // Add to removed or modified system
        if (system->HasVisibleGame()) modifiedSystems.Add(system);
        else removedSystems.Add(system);
        result = true;
      }

  return result;
}

bool SystemManager::UpdateSystemsOnMultipleGameChanges(MetadataType changes, SystemManager::List& addedSystems,
                                                       SystemManager::List& removedSystems,
                                                       SystemManager::List& modifiedSystems)
{
  bool result = false;
  for(SystemData* system : mAllSystems)
    // Is this virtual system sensible to changed metadata?
    if (system->IsVirtual() && (changes & system->MetadataSensitivity()) != 0)
    {
      bool hasVisibleBefore = system->HasVisibleGame();
      // Empty system
      system->MasterRoot().DeleteVirtualSubTree();
      assert(!system->MasterRoot().HasSubRoots());
      // Re-populate
      PopulateVirtualSystem(system);
      bool hasVisibleNow = system->HasVisibleGame();

      // Store
      if (hasVisibleBefore)
      {
        if (hasVisibleNow) modifiedSystems.Add(system);
        else { removedSystems.Add(system); LogSystemRemoved(system); }
        result = true;
      }
      else
      {
        if (hasVisibleNow) { addedSystems.Add(system); LogSystemAdded(system); }
        // Else nothing, the system is still invisible
      }
    }

  return result;
}

bool SystemManager::UpdateSystemsOnSingleGameChanges(FileData* target, MetadataType changes,
                                                     SystemManager::List& addedSystems,
                                                     SystemManager::List& removedSystems,
                                                     SystemManager::List& modifiedSystems)
{
  bool result = false;
  for(SystemData* system : mAllSystems)
    // Is this virtual system sensible to changed metadata?
    if (system->IsVirtual() && (changes & system->MetadataSensitivity()) != 0)
    {
      // Game already in this system?
      bool isAlreadyIn = system->MasterRoot().LookupGame(target);
      bool shouldBeIn = system->ShouldBelongToMe(target) && !target->Metadata().Hidden();
      // Process only changed states & ignore equal states
      if (isAlreadyIn && !shouldBeIn) // Must remove
      {
        // Get unique virtual root of virtual systems
        RootFolderData& root = system->LookupOrCreateRootFolder(Path(), RootFolderData::Ownership::FolderOnly, RootFolderData::Types::Virtual);
        // Remove entry
        root.RemoveChildRecursively(target);
        LogSystemGameRemoved(system, target);
        // In what list must we add the system?
        if (system->HasGame()) modifiedSystems.Add(system);
        else { removedSystems.Add(system); LogSystemRemoved(system); }
        result = true;
      }
      else if (!isAlreadyIn && shouldBeIn) // Must add
      {
        bool hasGame = system->HasGame();
        // Create a one-file reverse doppleganger
        FileData::StringMap doppelganger; doppelganger[target->RomPath().ToString()] = target;
        // Get unique virtual root of virtual systems
        RootFolderData& root = system->LookupOrCreateRootFolder(Path(), RootFolderData::Ownership::FolderOnly, RootFolderData::Types::Virtual);
        // Add games
        system->LookupOrCreateGame(root, target->TopAncestor().RomPath(), target->RomPath(), target->Type(), doppelganger);
        LogSystemGameAdded(system, target);
        // In what list must we add the system?
        if (hasGame) modifiedSystems.Add(system);
        else { addedSystems.Add(system); LogSystemAdded(system); }
        result = true;
      }
      else if (isAlreadyIn && shouldBeIn) // Changed metadata only
      {
        // Metadata change may change the display/rendering
        // As in LastPlayed system
        modifiedSystems.Add(system);
        result = true;
      }
    }
    // Special case of hidden metadata
    else if ((changes & MetadataType::Hidden) != 0 && &target->System() == system)
    {
      // In what list must we add the system?
      if (system->HasGame()) modifiedSystems.Add(system);
      else { removedSystems.Add(system); LogSystemRemoved(system); }
      result = true;
    }

  return result;
}

int SystemManager::GetVisibleRegularSystemCount() const
{
  int count = 0;
  for(SystemData* system : mVisibleSystems)
    if (!system->IsVirtual())
      ++count;
  return count;
}

void SystemManager::SlowPopulateExecute(const SystemManager::List& listToPopulate)
{
  // Initialize & populate (if required)
  for (SystemData* system : listToPopulate)
    InitializeSystem(system);
}

void SystemManager::SlowPopulateCompleted(const SystemManager::List& listPopulated, bool autoSelectMonoSystem)
{
  bool hasVisibleGame = false;
  // Show all systems
  for (SystemData* system : listPopulated)
    if (hasVisibleGame = system->HasVisibleGame(); hasVisibleGame)
    {
      MakeSystemVisible(system);
      mSystemChangeNotifier->ShowSystem(system);
    }
  // If there is only one, select it!
  if (autoSelectMonoSystem)
    if (listPopulated.Count() == 1)
    {
      if (hasVisibleGame) mSystemChangeNotifier->SelectSystem(listPopulated.First());
      else mSystemChangeNotifier->SystemShownWithNoGames(listPopulated.First());
    }
}

bool SystemManager::ContainsUninitializedSystem(const SystemManager::List& list)
{
  // Chekc if at least one system is not initialized
  for (SystemData* system : list)
    if (!system->IsInitialized())
      return true;
  return false;
}

void
SystemManager::ApplySystemChanges(SystemManager::List* addedSystems,
                                  SystemManager::List* removedSystems, SystemManager::List* modifiedSystems,
                                  bool autoSelectMonoSystem)
{
  // Added virtual systems?
  if (addedSystems != nullptr && !addedSystems->Empty())
  {
    // Check system that must remain invisibles to the user
    for(int i = addedSystems->Count(); --i >= 0;)
      if (const SystemData* system = (*addedSystems)[i]; !system->MustBeShown())
        addedSystems->Delete(i);
    // Initialize new system
    if (ContainsUninitializedSystem(*addedSystems)) mSystemChangeNotifier->RequestSlowOperation(this, *addedSystems, autoSelectMonoSystem);
    else SlowPopulateCompleted(*addedSystems, autoSelectMonoSystem);
  }
  // Move updated system that must remain invisible to the user to the remove list
  if (modifiedSystems != nullptr)
    for(int i = modifiedSystems->Count(); --i >= 0; )
      if (SystemData* system = (*modifiedSystems)[i]; !system->MustBeShown())
      {
        removedSystems->Add(system);
        modifiedSystems->Delete(i);
      }
  // Removed virtual systems?
  if (removedSystems != nullptr && !removedSystems->Empty())
    for(SystemData* system : *removedSystems)
    {
      MakeSystemInvisible(system);
      mSystemChangeNotifier->HideSystem(system);
    }
  // Modified virtual systems?
  if (modifiedSystems != nullptr)
    for (SystemData* system: *modifiedSystems)
      mSystemChangeNotifier->UpdateSystem(system);
}

bool SystemManager::UpdatedTopLevelFilter()
{
  SystemManager::List added;
  SystemManager::List removed;
  SystemManager::List updated;

  // Check all systems
  for(SystemData* system : mAllSystems)
    if (system->HasVisibleGame())
    {
      if (!mVisibleSystems.Contains(system)) // System was invisible?
        added.Add(system);
    }
    else
    {
      if (mVisibleSystems.Contains(system)) // System was visible?
        removed.Add(system);
    }
  // Finally, any visible system not in remove list must be updated
  for(SystemData* system : mVisibleSystems)
    if (!removed.Contains(system))
      updated.Add(system);

  // Check if the filter does not remove all visible systems
  int visibleSystems = mVisibleSystems.Count();
  for(SystemData* system : removed)
    if (mVisibleSystems.Contains(system))
      --visibleSystems;

  if (visibleSystems != 0)
    ApplySystemChanges(&added, &removed, &updated, false);

  return visibleSystems != 0;
}

void SystemManager::UpdateSystemsVisibility(SystemData* system, Visibility visibility)
{
  List list({ system });
  bool visible = mVisibleSystems.Contains(system);
  if ((visibility == Visibility::Show || visibility == Visibility::ShowAndSelect) && !visible) ApplySystemChanges(&list, nullptr, nullptr, visibility == Visibility::ShowAndSelect);
  else if (visibility == Visibility::Hide && visible) ApplySystemChanges(nullptr, &list, nullptr, false);
}

void SystemManager::ConfigurationCollectionArcadeManufacturersChanged(const String::List& value)
{
  for(const std::pair<String, String>& rawIdentifier : ArcadeVirtualSystems::GetVirtualArcadeSystemListExtended())
  {
    String manufacturer(SystemManager::sArcadeManufacturerPrefix);
    manufacturer.Append(rawIdentifier.first).Replace('\\', '-');
    UpdateVirtualArcadeManufacturerSystemsVisibility(manufacturer,
                                                     std::ranges::find(value, manufacturer) != value.end() ?
                                                     SystemManager::Visibility::ShowAndSelect :
                                                     SystemManager::Visibility::Hide);
  }
}

bool SystemManager::HasVisibleTateGame() const
{
  for(const SystemData* system : mVisibleSystems)
    if (system->HasTateGame())
      return true;
  return false;
}

void SystemManager::FileSystemWatcherNotification(FileSystemWatcher& source, const Path& path, EventType event, const DateTime& time)
{
  (void)time;

  String filename = path.Filename();
  bool isGamelist = filename == SystemData::sGameListName || filename == SystemData::sUserGameListName;

  SystemData** systemProbe = mWatcherToSystem.try_get(&source);
  if (systemProbe == nullptr)
  { LOG(LogError) << "[SystemManager] Unable to retrieve source system from event on file: " << path; return; }
  SystemData* system = *systemProbe;

  if (system->IsScreenshots()) return;

  if (!isGamelist)
  {
    { LOG(LogInfo) << "[SystemManager] Rom folder << " << path << " has been altered."; }
    if (path.IsDirectory())
    {
      bool emptyBefore = mDirtyRomFolders.empty();
      mDirtyRomFolders.insert(system);
      if (emptyBefore) mSystemAlteredInterface.SystemRomFolderAltered(system);
    }
    else if (String extensions = String(' ').Append(system->Descriptor().Extension()).Append(' '); !path.Extension().empty() && extensions.Contains(path.Extension()))
    {
      bool emptyBefore = mDirtyRomFolders.empty();
      mDirtyRomFolders.insert(system);
      if (emptyBefore) mSystemAlteredInterface.SystemRomFolderAltered(system);
    }
  }
  else if ((event & (EventType::Modify | EventType::RemoveSelf | EventType::Create | EventType::MoveSelf)) != 0)
  {
    { LOG(LogInfo) << "[SystemManager] Gamelist << " << path << " has been altered."; }
    bool emptyBefore = mDirtyGamelists.empty();
    mDirtyGamelists.insert(system);
    if (emptyBefore) mSystemAlteredInterface.SystemGamelistAltered(system);
  }
}

Array<SystemData*> SystemManager::GetDirtyRomFolderSystemList() const
{
  Array<SystemData*> result;
  for(SystemData* system : mDirtyRomFolders)
    result.Add(system);
  return result;
}

Array<SystemData*> SystemManager::GetDirtyGamelistSystemList() const
{
  Array<SystemData*> result;
  for(SystemData* system : mDirtyGamelists)
    result.Add(system);
  return result;
}
