#include "SystemData.h"
#include <systems/SystemManager.h>
#include "audio/AudioScheduler.h"
#include "games/GameFilesUtils.h"
#include <usernotifications/NotificationManager.h>

String SystemData::sGameListName("gamelist.xml");
String SystemData::sUserGameListName("gamelist-userdata.ini");

SystemData::SystemData(SystemManager& systemManager, const SystemDescriptor& descriptor, const DynamicVisibilityType& visibility, const DynamicBelongingType& belonging, Properties properties)
  : mSystemManager(systemManager)
  , mDescriptor(descriptor)
  , mRootOfRoot(*this)
  , mProperties(properties)
  , mFixedSort(FileSorts::Sorts::FileNameAscending)
  , mArcadeDatabases(*this)
  , mSensitivity(MetadataType::Hidden)
  , mVirtualType(VirtualSystemType::None)
  , mVisibility(visibility)
  , mShouldBelongToMe(belonging)
{
}

SystemData::SystemData(SystemManager& systemManager, const SystemDescriptor& descriptor, const DynamicVisibilityType& visibility, const DynamicBelongingType& belonging, Properties properties, MetadataType sensitivity, VirtualSystemType virtualType, FileSorts::Sorts fixedSort)
  : mSystemManager(systemManager)
  , mDescriptor(descriptor)
  , mRootOfRoot(*this)
  , mProperties(properties)
  , mFixedSort(fixedSort)
  , mArcadeDatabases(*this)
  , mSensitivity(sensitivity | MetadataType::Hidden)
  , mVirtualType(virtualType)
  , mVisibility(visibility)
  , mShouldBelongToMe(belonging)
{
}

void SystemData::WatchAllGamelists()
{
  // Add all folders to the local watcher
  for (const RootFolderData* root: mRootOfRoot.SubRoots())
  {
    AddGamelistWatch(GetGamelistPath(*root));
    AddGamelistWatch(GetGamelistUserdataPath(*root, false));
  }
}

void SystemData::WatchAllRomFolders()
{
  // Add all folders to the local watcher
  for (const FolderData* subfolder: mRootOfRoot.SubRoots())
  {
    AddRomFolderWatch(subfolder->RomPath());
    // Add root rom folder
    for(const FileData* folder : subfolder->GetAllFolders())
      AddRomFolderWatch(folder->RomPath());
    // Remove media folder explicitely
    mFileWatcher.AddIgnoredPath(subfolder->RomPath() / "media");
  }
}

void SystemData::ScanStorageFolder(RootFolderData& folder, FileData::StringMap& doppelgangerWatcher, ISystemScan* scanInterface)
{
  { LOG(LogInfo) << "[Gamelist] " << folder.System().FullName() << ": Searching games/roms in " << folder.RomPath().ToString() << "..."; }

  try
  {
    String ignoreList(','); ignoreList.Append(mDescriptor.IgnoredFiles()).Append(',');
    folder.PopulateRecursiveFolder(folder, mDescriptor.Extension().ToLowerCase(), ignoreList, doppelgangerWatcher, scanInterface);
  }
  catch (std::exception& ex)
  {
    { LOG(LogError) << "[Gamelist] Reading folder \"" << folder.RomPath().ToString() << "\" has raised an error!"; }
    { LOG(LogError) << "[Gamelist] Exception: " << ex.what(); }
  }
}

Path SystemData::GetGamelistUserdataPath(const RootFolderData& root, bool forceFolderCreation)
{
  String romPath = root.RomPath().ToString();
  if (root.PreInstalled()) romPath.Replace("/share_init/", "/share/");
  Path intermediate(root.PreInstalled() ? "preinstalled" : ".");
  Path filePath = Path(romPath) / intermediate / sUserGameListName;

  if (forceFolderCreation)
    if (!filePath.Directory().Exists())
      (void)filePath.Directory().CreatePath();

  return filePath;
}

Path SystemData::GetGamelistPath(const RootFolderData& root)
{
  Path filePath = root.RomPath() / sGameListName;

  if (!filePath.Directory().Exists())
    (void)filePath.Directory().CreatePath();

  return filePath;
}

FileData* SystemData::LookupOrCreateGame(RootFolderData& topAncestor, const Path& rootPath, const Path& path, ItemType type, FileData::StringMap& doppelgangerWatcher) const
{
  if (!path.StartWidth(rootPath))
  {
    { LOG(LogError) << "[Gamelist] File path \"" << path.ToString() << "\" is outside system path \"" <<  rootPath.ToString() << "\""; }
    return nullptr;
  }

  bool isVirtual = IsVirtual();

  int itemStart = rootPath.ItemCount();
  int itemLast = path.ItemCount() - 1;
  FolderData* treeNode = &topAncestor;
  for (int itemIndex = itemStart; itemIndex <= itemLast; ++itemIndex)
  {
    // Get the key for duplicate detection. MUST MATCH KEYS USED IN populateRecursiveFolder.populateRecursiveFolder - Always fullpath
    String key = path.UptoItem(itemIndex);
    FileData** itemFound = doppelgangerWatcher.try_get(key);
    FileData* item = itemFound != nullptr ? *itemFound : nullptr;

    // Some ScummVM folder/games may create inconsistent folders
    if (!treeNode->IsFolder()) return nullptr;

    // Last path component?
    if (itemIndex == itemLast)
    {
      if (type == ItemType::Game) // Final file
      {
        FileData* game = item;
        if (game == nullptr && !isVirtual)
        {
          // Add final game
          game = new FileData(path, topAncestor);
          doppelgangerWatcher[key] = game;
          treeNode->AddChild(game, true);
        }
        // Virtual systems use the doppleganger map in a reverse way:
        // Game to insert are already in the map
        // and are added as parent-less game in the virtual system
        else if (game != nullptr && isVirtual)
        {
          // Add existing game & remove from doppleganger
          treeNode->AddChild(game, false);
          doppelgangerWatcher.erase(key);
        }
        return game;
      }

      // Final folder (scraped obviously)
      FolderData* folder = (FolderData*) item;
      if (folder == nullptr)
      {
        // Create missing folder in both case, virtual or not
        folder = new FolderData(Path(key), topAncestor);
        doppelgangerWatcher[key] = folder;
        treeNode->AddChild(folder, true);
      }
      return folder;
    }

    // Intermediate path
    FolderData* folder = (FolderData*) item;
    if (folder == nullptr)
    {
      // Create missing folder in both case, virtual or not
      folder = new FolderData(Path(key), topAncestor);
      doppelgangerWatcher[key] = folder;
      treeNode->AddChild(folder, true);
    }
    treeNode = folder;
  }

  return nullptr;
}

bool SystemData::ValidateContent(const String& content)
{
  XmlDocument gameList;
  XmlResult result = gameList.load_string(content.data());
  if (!result) { LOG(LogError) << "[Gamelist] Validator cannot parse xml content"; return false; }
  return true;
}

void SystemData::ParseGamelistUserDataXml(RootFolderData& root, HashMap<String, String>& gameUserDataMap) const
{
  Path iniPath = GetGamelistUserdataPath(root, false);
  String content;
  if (!SecuredFile::LoadSecuredFile(false, iniPath, Path::Empty, content, String(FullName()).Append(" gamelist"), true, nullptr))
    return;

  for(int start = 0; start < content.Count(); )
  {
    // Look for key separator
    int end = content.Find('\n', start);
    if (end < 0) end = content.Count();

    // Look for key/value separator
    int equal = content.Find(':', start);
    if (equal <= start  || equal >= end) { LOG(LogError) << "[SystemData] malformated user data line " << content.SubString(start, end - start); }
    else
    {
      String key = content.SubString(start, equal - start).Trim();
      String value = content.SubString(equal + 1, end - (equal + 1)).Trim();
      gameUserDataMap[key] = value;
    }

    // Next step
    start = end + 1;
  }
}

void SystemData::ParseGamelistXml(RootFolderData& root, FileData::StringMap& doppelgangerWatcher)
{
  try
  {
    HashMap<String, String> gameUserDataMap;
    ParseGamelistUserDataXml(root, gameUserDataMap);

    Path xmlPath = GetGamelistPath(root);
    String xml;
    if (!SecuredFile::LoadSecuredFile(false, xmlPath, Path::Empty, xml, String(FullName()).Append(" gamelist"), true, this))
      return;

    XmlDocument gameList;
    XmlResult result = gameList.load_string(xml.data());
    if (!result)
    {
      { LOG(LogError) << "[Gamelist] Cannot parse " << xmlPath.ToString() << " file!"; }
      return;
    }

    String ignoreList(','); ignoreList.Append(mDescriptor.IgnoredFiles()).Append(',');

    const Path relativeTo(root.RomPath());
    XmlNode games = gameList.child("gameList");
    HashSet<Path> blacklist;

    if (games != nullptr)
    {
      // build game subfiles blacklist
      String extensions = mDescriptor.Extension();
      if (GameFilesUtils::ContainsMultiDiskFile(extensions))
        for (const XmlNode fileNode: games.children())
        {
          Path path = relativeTo / Xml::AsString(fileNode, "path", "");
          GameFilesUtils::ExtractUselessFiles(path, blacklist);
        }

      for (const XmlNode fileNode: games.children())
      {
        ItemType type = ItemType::Game;
        String name = fileNode.name();
        if (name == "folder") type = ItemType::Folder;
        else if (name != "game") continue; // Unknown node

        String stringRelativePath(Xml::AsString(fileNode, "path", ""));
        if (stringRelativePath.StartsWith("./", 2)) stringRelativePath.Delete(0, 2);
        Path relativePath(stringRelativePath);
        Path path = relativeTo / relativePath;

        // Force to hide ignored files
        const String fileName = path.Filename();
        int p = ignoreList.Find(fileName);
        if (p > 0 && ignoreList[p-1] == ',')
          if (ignoreList[p + fileName.length()] == ',')
            continue;

        if (blacklist.contains(path))
          continue;

        FileData* file = LookupOrCreateGame(root, relativeTo, path, type, doppelgangerWatcher);
        if (file == nullptr)
        {
          { LOG(LogError) << "[Gamelist] Error finding/creating FileData for \"" << path.ToString() << "\", skipping."; }
          continue;
        }

        // load the metadata
        String* userData = gameUserDataMap.try_get(relativePath.ToString());
        file->Metadata().Deserialize(fileNode, userData != nullptr ? *userData : String::Empty, relativeTo);
      }
    }
  }
  catch (std::exception& ex)
  {
    { LOG(LogError) << "[Gamelist] System \"" << Name() << "\" has raised an error while reading its gamelist.xml!"; }
    { LOG(LogError) << "[Gamelist] Exception: " << ex.what(); }
  }
}

void SystemData::RemoveGamelistXml()
{
  for(const RootFolderData* root : mRootOfRoot.SubRoots())
    if ((!root->ReadOnly() || root->PreInstalled()) && root->IsDirty())
    {
      // Delete xml
      (void)GetGamelistPath(*root).Delete();
      // Delete user data
      (void)GetGamelistUserdataPath(*root, false).Delete();
    }
}

void SystemData::UpdateGamelistXml()
{
  // We do this by reading the XML again, adding changes and then writing it back,
  // because there might be information missing in our systemdata which would then miss in the new XML.
  // We have the complete information for every game though, so we can simply remove a game
  // we already have in the system from the XML, and then add it back from its GameData information...
  for(const RootFolderData* root : mRootOfRoot.SubRoots())
  {
    //{ LOG(LogDebug) << "[UpdateGamelistXml] System " << Name() << " - Root " << root->RomPath() << " - RO: " << (root->ReadOnly() ? 'T' : 'F') << " - PI: " << (root->PreInstalled() ? 'T' : 'F') << " - Dirty: " << (root->IsDirty() ? 'T' : 'F')<< " - DC: " << (root->HasDeletedChildren() ? 'T' : 'F'); }
    if (!(root->ReadOnly() || root->PreInstalled()))
    {
      Path xmlPath = GetGamelistPath(*root);
      Path userDataPath = GetGamelistUserdataPath(*root, true);
      //{ LOG(LogDebug) << "[UpdateGamelistXml] System " << Name() << " - XmlPath " << xmlPath; }

      if (root->IsDirty() || root->HasDeletedChildren() || !xmlPath.Exists())
        try
        {
          //{ LOG(LogDebug) << "[UpdateGamelistXml] System " << Name() << " - XmlPath exists: " << xmlPath.Exists(); }
          String userDataContent;

          /*
           * Get all folder & games in a flat storage
           */
          FileData::List fileList = root->GetAllItemsRecursively(true, FileData::Filter::None);
          FileData::List folderList = root->GetAllFolders();
          // Nothing to process?
          if (fileList.Empty() && !root->HasDeletedChildren())
            continue;

          /*
           * Create gamelist
           */
          XmlDocument document;
          XmlNode gameList = document.append_child("gameList");

          /*
           * Serialize folder and game nodes
           */
          Path rootPath = root->RomPath();
          bool dummy;
          for (const FileData* folder: folderList)
          {
            String userData;
            folder->Metadata().Serialize(gameList, userData, folder->RomPath(), rootPath);
            if (!userData.empty()) userDataContent.Append(folder->RomPath().MakeRelative(rootPath, dummy).ToString()).Append(':').Append(userData).Append('\n');
          }
          for (FileData* file: fileList)
          {
            Path path(file->RomPath());
            if (root->GetDeletedChildren().contains(file))
              continue;
            String userData;
            file->Metadata().Serialize(gameList, userData, path, rootPath);
            file->Metadata().UnsetDirty();
            if (!userData.empty()) userDataContent.Append(path.MakeRelative(rootPath, dummy).ToString()).Append(':').Append(userData).Append('\n');
          }

          /*
           * Custom thread-safe writer
           */
          struct XmlWriter : public pugi::xml_writer
          {
            String mOutput;

            void write(const void* data, size_t size) override
            { mOutput.Append((const char*) data, (int) size); }
          } Writer;

          /*
           * Write the list.
           * At this point, we're sure at least one node has been updated (or added and updated).
           */
          (void) xmlPath.Directory().CreatePath();
          document.save(Writer);

          // Pause file watching before writing to being fooled by our own writes
          mFileWatcher.Pause();

          // Save
          if (!root->PreInstalled()) (void)SecuredFile::SaveSecuredFile(xmlPath, Writer.mOutput, String(FullName()).Append(" gamelist"), true, this);
          if (!userDataContent.empty()) (void)SecuredFile::SaveSecuredFile(userDataPath, userDataContent, String(FullName()).Append(" gamelist user data"), true, nullptr);

          // Re-enable file watching
          mFileWatcher.Resume();
        }
        catch (std::exception& e)
        {
          { LOG(LogError) << "[Gamelist] Something went wrong while saving " << FullName() << " : " << e.what(); }
        }
    }
  }
}

bool SystemData::IsAutoScrapable() const
{
  return (mProperties & Properties::GameInPng) != 0;
}

bool SystemData::IsFavorite() const
{
  return (mProperties & Properties::Favorite) != 0;
}

bool SystemData::IsPorts() const
{
  return (mProperties & Properties::Ports) != 0;
}

bool SystemData::IsMainPort() const
{
  return (mVirtualType == VirtualSystemType::Ports);
}

bool SystemData::IsScreenshots() const
{
    return (mProperties & Properties::ScreenShots) != 0;
}

bool SystemData::IsLastPlayed() const
{
  return (mSensitivity & MetadataType::LastPlayed) != 0;
}

bool SystemData::IsVirtual() const
{
  return (mProperties & Properties::Virtual) != 0;
}

bool SystemData::IsSelfSorted() const
{
  return (mProperties & Properties::FixedSort) != 0;
}

bool SystemData::IsAlwaysFlat() const
{
  return (mProperties & Properties::AlwaysFlat) != 0;
}

bool SystemData::IsSearchable() const
{
  return (mProperties & Properties::Searchable) != 0;
}

void SystemData::BuildDoppelgangerMap(FileData::StringMap& doppelganger, bool includefolder) const
{
  for(const RootFolderData* root : mRootOfRoot.SubRoots())
    root->BuildDoppelgangerMap(doppelganger, includefolder);
}

void SystemData::UpdateLastPlayedGame(FileData& updated)
{
  // Update game
  updated.Metadata().SetLastPlayedNow();

  // Build the doppelganger map
  FileData::StringMap map;
  BuildDoppelgangerMap(map, false);
  // If the game is already here, exit
  Path path(updated.RomPath());
  if (map.contains(path.ToString())) return;

  // Prepare a one game map
  map.clear();
  map[path.ToString()] = &updated;
  // Add the virtual game
  RootFolderData* root = GetRootFolder(RootFolderData::Types::Virtual);
  if (root != nullptr)
    LookupOrCreateGame(*root, updated.TopAncestor().RomPath(), path, ItemType::Game, map);
}

RootFolderData* SystemData::GetRootFolder(RootFolderData::Types type)
{
  for(RootFolderData* rootFolder : mRootOfRoot.SubRoots())
    if (rootFolder->RootType() == type)
      return rootFolder;
  return nullptr;
}

RootFolderData* SystemData::GetRootFolder(const Path& root)
{
  for(RootFolderData* rootFolder : mRootOfRoot.SubRoots())
    if (rootFolder->RomPath() == root)
      return rootFolder;
  return nullptr;
}

RootFolderData& SystemData::CreateRootFolder(const Path& startpath, RootFolderData::Ownership childownership, RootFolderData::Types type)
{
  RootFolderData* newRoot = new RootFolderData(mRootOfRoot, childownership, type, startpath, *this);
  mRootOfRoot.AddSubRoot(newRoot);
  return *newRoot;
}

RootFolderData& SystemData::LookupOrCreateRootFolder(const Path& startpath, RootFolderData::Ownership childownership,
                                                    RootFolderData::Types type)
{
  RootFolderData* lookup = GetRootFolder(startpath);
  if (lookup != nullptr) return *lookup;

  return CreateRootFolder(startpath, childownership, type);
}

bool SystemData::HasGame() const
{
  for(const RootFolderData* root : mRootOfRoot.SubRoots())
    if (root->HasGame())
      return true;
  return false;
}

int SystemData::GameCount(int& favorites, int& hidden) const
{
  int result = 0;
  favorites = hidden = 0;
  FileData::Filter excludes = Excludes();
  for(const RootFolderData* root : mRootOfRoot.SubRoots())
    result += root->CountAllGamesAndFavoritesAndHidden(excludes, favorites, hidden);
  return result;
}

FileData::List SystemData::getFavorites() const
{
  FileData::Filter filter = FileData::Filter::Favorite;
  FileData::Filter excludes = Excludes();
  FileData::List result;
  for(const RootFolderData* root : mRootOfRoot.SubRoots())
    root->GetItemsRecursivelyTo(result, filter, excludes, false);
  return result;
}

FileData::List SystemData::getAllGames() const
{
  FileData::List result;
  for(const RootFolderData* root : mRootOfRoot.SubRoots())
    root->GetItemsRecursivelyTo(result, FileData::Filter::All, FileData::Filter::None, false);
  return result;
}

bool SystemData::HasVisibleGame() const
{
  FileData::TopLevelFilter filter = FileData::BuildTopLevelFilter();
  for(const RootFolderData* root : mRootOfRoot.SubRoots())
    if (root->HasVisibleGame(filter)) return true;
  return false;
}

bool SystemData::HasScrapableGame() const
{
  for(const RootFolderData* root : mRootOfRoot.SubRoots())
    if (root->HasScrapableGame()) return true;

  return false;
}

bool SystemData::HasTateGame() const
{
  for(const RootFolderData* root : mRootOfRoot.SubRoots())
    if (root->HasTateGame()) return true;

  return false;
}

Path::PathList SystemData::WritableGamelists()
{
  Path::PathList result;
  for(const RootFolderData* root : mRootOfRoot.SubRoots())
    if (root->Normal())
      result.push_back(GetGamelistPath(*root));
  return result;
}

FileData::List SystemData::getTopGamesAndFolders() const
{
  FileData::List result;
  FileData::Filter excludes = Excludes();
  for(const RootFolderData* root : mRootOfRoot.SubRoots())
    root->GetItemsTo(result, FileData::Filter::All, excludes, true);
  return result;
}

bool SystemData::IncludeAdultGames() const
{
  return !RecalboxConf::Instance().GetSystemFilterAdult(*this);
}

FileData::Filter SystemData::Excludes() const
{
  RecalboxConf& conf = RecalboxConf::Instance();

  // Default excludes filter
  FileData::Filter excludesFilter = FileData::Filter::None;
  // ignore hidden?
  if (!conf.GetShowHidden())
    excludesFilter |= FileData::Filter::Hidden;
  // ignore non Latest version
  if (conf.GetShowOnlyLatestVersion())
    excludesFilter |= FileData::Filter::NotLatest;
  // ignore pre installed games ?
  if (conf.GetGlobalHidePreinstalled())
    excludesFilter |= FileData::Filter::PreInstalled;
  // ignore adult games ?
  if(!IncludeAdultGames())
    excludesFilter |= FileData::Filter::Adult;
  // ignore no games ?
  if(conf.GetHideNoGames())
    excludesFilter |= FileData::Filter::NoGame;
  // ignore board games ?
  if(conf.GetHideBoardGames())
    excludesFilter |= FileData::Filter::Board;
  // ignore yoko games ?
  if(conf.GetShowOnlyTateGames())
    excludesFilter |= FileData::Filter::Yoko;
  // ignore less than 3 players games
  if(conf.GetShowOnly3PlusPlayers())
    excludesFilter |= FileData::Filter::OneAndTwoPlayers;
  // ignore tate games ?
  if(conf.GetShowOnlyYokoGames())
    excludesFilter |= FileData::Filter::Tate;
  return excludesFilter;
}

void SystemData::LookupGames(FolderData::FastSearchContext context, const StringHolderTypes::IndexAndDistance& index, FileData::List& games) const
{
  for(const RootFolderData* root : mRootOfRoot.SubRoots())
    switch(context)
    {
      case FolderData::FastSearchContext::Path: root->LookupGamesFromPath(index, games); break;
      case FolderData::FastSearchContext::Name: root->LookupGamesFromName(index, games); break;
      case FolderData::FastSearchContext::Description: root->LookupGamesFromDescription(index, games); break;
      case FolderData::FastSearchContext::Developer: root->LookupGamesFromDeveloper(index, games); break;
      case FolderData::FastSearchContext::Publisher: root->LookupGamesFromPublisher(index, games); break;
      case FolderData::FastSearchContext::All: root->LookupGamesFromAll(index, games); break;
      case FolderData::FastSearchContext::Licences:
      default: break;
    }
}

void SystemData::BuildFastSearchSeries(FolderData::FastSearchItemSerie& into, FolderData::FastSearchContext context) const
{
  for(const RootFolderData* root : mRootOfRoot.SubRoots())
    switch(context)
    {
      case FolderData::FastSearchContext::Name: root->BuildFastSearchSeriesName(into); break;
      case FolderData::FastSearchContext::Licences: root->BuildFastSearchSeriesLicences(into); break;
      case FolderData::FastSearchContext::Path: root->BuildFastSearchSeriesPath(into); break;
      case FolderData::FastSearchContext::Description: root->BuildFastSearchSeriesDescription(into); break;
      case FolderData::FastSearchContext::Developer: root->BuildFastSearchSeriesDeveloper(into); break;
      case FolderData::FastSearchContext::Publisher: root->BuildFastSearchSeriesPublisher(into); break;
      case FolderData::FastSearchContext::All:
      default: break;
    }
}

void SystemData::RemoveArcadeReference(const FileData& game)
{
  mArcadeDatabases.RemoveGame(game);
}

bool SystemData::Rotatable() const
{
  return mDescriptor.Name() == SystemManager::sTateSystemShortName;
}

void SystemData::AddRomFolderWatch(const Path& path)
{
  if (path.Exists())
    mFileWatcher.WatchFile(path, EventType::Remove |  // Rom or dir has been removed or moved to another fs
                                 EventType::Create |  // Rom or dir has been created
                                 EventType::Move |  // Rom inside dir, or dir has been moved
                                 EventType::MoveSelf |  // dir has been moved
                                 EventType::RemoveSelf); // dir has been deleted
}

void SystemData::AddGamelistWatch(const Path& path)
{
  if (path.Exists())
    mFileWatcher.WatchFile(path, EventType::RemoveSelf |  // Gamelist has been deleted
                                 EventType::Create |  // Gamelist has been created (copied/moved)
                                 EventType::MoveSelf |  // Gamelist has been moved
                                 EventType::Modify); // Gamelist has been altered
}
