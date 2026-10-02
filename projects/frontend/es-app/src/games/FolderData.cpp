//
// Created by bkg2k on 24/11/18.
//

#include "FolderData.h"
#include "utils/Log.h"
#include "systems/SystemData.h"
#include "GameNameMapManager.h"
#include "GameFilesUtils.h"
#include <utils/Files.h>

#define CastFolder(f) ((FolderData*)(f))

#define LockCopy(x) Lock(); FileData::List x(mChildren); Unlock()

FolderData::~FolderData()
{
  Lock();
  for (FileData* fd : mChildren)
    delete fd;
  mChildren.Clear();
  Unlock();
  delete &mGuardian;
}

void FolderData::AddChild(FileData* file, bool lukeImYourFather)
{
  AutoGuard locker(*this);
  assert(file->Parent() == nullptr || !lukeImYourFather);
  mChildren.Add(file);
  if (lukeImYourFather)
    file->SetParent(this);
}

void FolderData::RemoveChild(const FileData* file)
{
  AutoGuard locker(*this);
  mChildren.Remove((FileData*)file);
}

bool FolderData::RemoveChildRecursively(const FileData* item)
{
  // Recurse
  AutoGuard locker(*this);
  if (mChildren.Remove((FileData*)item)) return true;
  for(FileData* fd : mChildren)
    if (fd->IsFolder())
      if (CastFolder(fd)->RemoveChildRecursively(item)) return true;
  return false;
}

void FolderData::ClearSubChildList()
{
  AutoGuard locker(*this);
  for (int i = mChildren.Count(); --i >= 0; )
  {
    const FileData* fd = mChildren[i];
    if (fd->IsFolder())
      CastFolder(fd)->ClearSubChildList();
    else
      mChildren(i) = nullptr;
  }
}

static bool IsMatching(const String& fileWoExt, const String& extension, const String& extensionList)
{
  #define sFilesPrefix "files:"
  if (!extensionList.StartsWith(LEGACY_STRING(sFilesPrefix)))
  {
    // Seek in regular extensions
    int start = 0;
    while(start < (int)extensionList.size())
    {
      int extensionPos = extensionList.Find(extension, start);
      if (extensionPos < 0) return false;
      const char* p = extensionList.data();
      char endChar = p[extensionPos + extension.size()];
      if (endChar == ' ' || endChar == 0) return true;
      start += (int)(extensionPos + extension.size());
    }
    return false;
  }

  // Seek complete files
  constexpr int sFilesPrefixLength = sizeof(sFilesPrefix) - 1;
  String file(fileWoExt); file.Append(extension).LowerCase();
  int filePos = extensionList.Find(file);
  if (filePos < 0) return false;
  const char* p = extensionList.data();
  return ((filePos == sFilesPrefixLength) || (p[filePos - 1] == ' ')) &&
         ((filePos + file.size() == extensionList.size()) || (p[filePos + file.size()] == ' '));
}

int FolderData::PopulateRecursiveFolder(RootFolderData& root, const String& originalFilteredExtensions, const String& ignoreList, FileData::StringMap& doppelgangerWatcher, ISystemScan* scanInterface)
{
  int count = 0;
  Lock(); const Path folderPath(RomPath()); Unlock();
  if (!folderPath.IsDirectory())
  {
    { LOG(LogWarning) << "[FolderData] Error - folder with path \"" << folderPath.ToString() << "\" is not a directory!"; }
    return 0;
  }

  // media folder?
  if (folderPath.FilenameWithoutExtension() == "media")
    return 0;

  //make sure that this isn't a symlink to a thing we already have
  if (folderPath.IsSymLink())
  {
    // if this symlink resolves to somewhere that's at the beginning of our path, it's gonna recurse
    Path canonical = folderPath.ToCanonical();
    if (folderPath.ToString().compare(0, canonical.ToString().size(), canonical.ToChars()) == 0)
    { LOG(LogWarning) << "[FolderData] Skipping infinitely recursive symlink \"" << folderPath.ToString() << "\""; return 0; }
  }

  // Subsystem override
  String filteredExtensions = originalFilteredExtensions;
  if ((folderPath / ".system.cfg").Exists())
  {
    IniFile subSystem(folderPath / ".system.cfg", "FolderData - Extension override", false, false);
    filteredExtensions = subSystem.AsString("extensions", originalFilteredExtensions);
  }

  // special system?
  bool hasFiltering = GameNameMapManager::HasFiltering(System());
  // No extension?
  bool noExtensions = filteredExtensions.empty();

  // Keep temporary object outside the loop to avoid construction/destruction and keep memory allocated AMAP
  Path::IdentifiedPathList items = folderPath.GetDirectoryIdentifiedContent();

  HashSet<Path> blacklist;
  bool containsMultiDiskFile = GameFilesUtils::ContainsMultiDiskFile(filteredExtensions);
  if (containsMultiDiskFile)
    for(const IdentifiedPath& itemPath : items)
      GameFilesUtils::ExtractUselessFiles(itemPath.GetPath(), blacklist);

  for (const IdentifiedPath& filePath : items)
  {
    // Get file
    String stem = filePath.GetPath().FilenameWithoutExtension();
    if (stem == "gamelist") continue; // Ignore gamelist.zip/xml
    if (stem.empty()) continue;

    // Force to hide ignored files
    // Force to hide ignored files
    const String fileName = filePath.GetPath().Filename();
    int p = (int)ignoreList.find(fileName);
    if (p > 0 && ignoreList[p-1] == ',')
      if (ignoreList[p + fileName.length()] == ',')
        continue;

    if (containsMultiDiskFile && blacklist.contains(filePath.GetPath())) continue;

    // and Extension
    String extension = filePath.GetPath().Extension().LowerCase();

    //fyi, folders *can* also match the extension and be added as games - this is mostly just to support higan
    //see issue #75: https://github.com/Aloshi/EmulationStation/issues/75
    bool isLaunchableGame = false;
    if (!filePath.GetPath().IsHidden())
    {
      if ((noExtensions && filePath.IsFile()) ||
          (!extension.empty() && IsMatching(stem, extension, filteredExtensions)))
      {
        if (hasFiltering)
        {
          if (GameNameMapManager::IsFiltered(System(), stem))
            continue; // MAME Bios or Machine
        }
        // Get the key for duplicate detection. MUST MATCH KEYS USED IN Gamelist.findOrCreateFile - Always fullpath
        if (scanInterface != nullptr)
          scanInterface->SystemScanProgress(root.System(), filePath.GetPath());
        FileData** doppelGanger = doppelgangerWatcher.try_get(filePath.GetPath().ToString());
        if (doppelGanger == nullptr)
        {
          FileData* newGame = new FileData(filePath.GetPath(), root);
          //newGame->Metadata().SetDirty();
          newGame->Metadata().Validate();
          AddChild(newGame, true);
          doppelgangerWatcher[filePath.GetPath().ToString()] = newGame;
          if (scanInterface != nullptr)
          {
            scanInterface->SystemScanGameAdded(root.System(), filePath.GetPath(), *newGame);
          }
          ++count;
        }
        else // Game is found again during a rescan, validate
          (*doppelGanger)->Metadata().Validate();
        isLaunchableGame = true;
      }

      //add directories that also do not match an extension as folders
      if (!isLaunchableGame && filePath.GetPath().IsDirectory())
      {
        FileData** doppelGanger = doppelgangerWatcher.try_get(filePath.GetPath().ToString());
        if (doppelGanger == nullptr)
        {
          FolderData* newFolder = new FolderData(filePath.GetPath(), root);
          newFolder->Metadata().Validate();
          int subCount = newFolder->PopulateRecursiveFolder(root, filteredExtensions, ignoreList, doppelgangerWatcher, scanInterface);

          //ignore folders that do not contain games
          if (subCount != 0)
          {
            const String key = newFolder->RomPath().ToString();
            if (!doppelgangerWatcher.contains(key))
            {
              AddChild(newFolder, true);
              doppelgangerWatcher[key] = newFolder;
              ++count;
            }
          }
          else delete newFolder;
        }
        else if (scanInterface != nullptr) // In Rescan mode only
        {
          (*doppelGanger)->Metadata().Validate();
          CastFolder(*doppelGanger)->PopulateRecursiveFolder(root, filteredExtensions, ignoreList, doppelgangerWatcher, scanInterface);
        }
      }
    }
  }

  // If a SystemScan interface is available, remove invalidated now
  if (scanInterface != nullptr)
    RemoveInvalids(scanInterface);
  return count;
}

void FolderData::ExtractUselessFiles(const Path::PathList& items, FileSet& blacklist)
{
  for (const Path& filePath : items)
  {
    const String extension = filePath.Extension();
    if      (extension == ".cue") ExtractUselessFilesFromCue(filePath, blacklist);
    else if (extension == ".ccd") ExtractUselessFilesFromCcd(filePath, blacklist);
    else if (extension == ".gdi" && filePath.Size() <= sMaxGdiFileSize) ExtractUselessFilesFromGdi(filePath, blacklist);
    else if (extension == ".m3u") ExtractUselessFilesFromM3u(filePath, blacklist);
  }
}

void FolderData::ExtractUselessFilesFromCue(const Path& path, FileSet& list)
{
  String file = Files::LoadFile(path);
  for(const String& line : file.Split('\n'))
    if (line.Contains("FILE") && line.Contains("BINARY"))
      ExtractFileNameFromLine(line, list);
}

void FolderData::ExtractUselessFilesFromCcd(const Path& path, FileSet& list)
{
  String file = path.FilenameWithoutExtension();
  list.insert(file + ".cue");
  list.insert(file + ".bin");
  list.insert(file + ".sub");
  list.insert(file + ".img");
}

void FolderData::ExtractUselessFilesFromM3u(const Path& path, FileSet& list)
{
  String file = Files::LoadFile(path);
  for(String line : file.Split('\n'))
    list.insert(line.Trim('\r'));
}

void FolderData::ExtractUselessFilesFromGdi(const Path& path, FileSet& list)
{
  String file = Files::LoadFile(path);
  for(const String& line : file.Split('\n'))
    ExtractFileNameFromLine(line, list);
}

void FolderData::ExtractFileNameFromLine(const String& line, FileSet& list)
{
  // 1 check file name between double quotes
  String string;
  if (line.Extract('"', '"', string, true))
    if (string.Contains('.'))
    {
      list.insert(string);
      return;
    }
  
  // 2 check every words separated by space that contains dot
  for(const String& word : line.Split(' ', true))
    if (word.Contains("."))
      list.insert(word);
}

int FolderData::getAllFoldersRecursively(FileData::List& to) const
{
  int gameCount = 0;
  LockCopy(children);
  for (FileData* fd : children)
    if (fd->IsFolder())
    {
      int position = to.Count(); // TOOD: Check if the insert is necessary
      if (CastFolder(fd)->getAllFoldersRecursively(to) > 1)
        to.Insert(fd, position); // Include folders iif it contains more than one game.
    }
    else if (fd->IsGame()) ++gameCount;
  return gameCount;
}

FileData::List FolderData::GetAllFolders() const
{
  FileData::List result;
  getAllFoldersRecursively(result);
  return result;
}

void FolderData::BuildDoppelgangerMap(FileData::StringMap& doppelganger, bool includefolder) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
  {
    if (fd->IsFolder())
    {
      CastFolder(fd)->BuildDoppelgangerMap(doppelganger, includefolder);
      if (includefolder) doppelganger[fd->RomPath().ToString()] = fd;
    }
    else doppelganger[fd->RomPath().ToString()] = fd;
  }
}

bool FolderData::HasMissingHashRecursively()
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder())
    {
      if (CastFolder(fd)->HasMissingHashRecursively())
        return true;
    }
    else if (fd->IsGame())
      if (fd->Metadata().RomCrc32() == 0)
        return true;
  return false;
}

void FolderData::CalculateMissingHashRecursively()
{
  LockCopy(children);
  for (FileData* fd : children)
    if (fd->IsFolder())
      CastFolder(fd)->CalculateMissingHashRecursively();
    else if (fd->IsGame())
      if (fd->Metadata().RomCrc32() == 0)
        fd->CalculateHash();
}

int FolderData::getMissingHashRecursively(FileData::List& to) const
{
  int gameCount = 0;
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
  {
    if (fd->IsFolder())
      gameCount += CastFolder(fd)->getMissingHashRecursively(to);
    else if (fd->IsGame())
      if (fd->Metadata().RomCrc32() == 0)
      {
        to.Add(fd);
        ++gameCount;
      }
  }
  return gameCount;
}

int FolderData::getItemsRecursively(FileData::List& to, IFilter* filter, bool includefolders, bool autolock) const
{
  int gameCount = 0;
  if (autolock)
  {
    AutoGuard locker(*this);
    for (FileData* fd: mChildren)
    {
      if (fd->IsFolder())
      {
        if (CastFolder(fd)->getItemsRecursively(to, filter, includefolders, autolock) > 1)
          if (includefolders)
            to.Add(fd); // Include folders iif it contains more than one game.
      }
      else if (fd->IsGame())
        if (filter->ApplyFilter(*fd))
        {
          to.Add(fd);
          ++gameCount;
        }
    }
  }
  else
  {
    LockCopy(children);
    for (FileData* fd: children)
    {
      if (fd->IsFolder())
      {
        if (CastFolder(fd)->getItemsRecursively(to, filter, includefolders, autolock) > 1)
          if (includefolders)
            to.Add(fd); // Include folders iif it contains more than one game.
      }
      else if (fd->IsGame())
        if (filter->ApplyFilter(*fd))
        {
          to.Add(fd);
          ++gameCount;
        }
    }
  }
  filter->TearDown();
  return gameCount;
}

int FolderData::getItemsRecursively(FileData::List& to, Filter includes, Filter excludes, bool includefolders) const
{
  int gameCount = 0;
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
  {
    if (fd->IsFolder())
    {
      if (CastFolder(fd)->getItemsRecursively(to, includes, excludes, includefolders) > 1)
        if (includefolders) to.Add(fd); // Include folders iif it contains more than one game.
    }
    else if (fd->IsGame())
      if (IsFiltered(fd, includes, excludes))
      {
        to.Add(fd);
        ++gameCount;
      }
  }
  return gameCount;
}

int FolderData::countItemsRecursively(IFilter* filter, bool includefolders, bool autolock) const
{
  int result = 0;
  if (autolock)
  {
    AutoGuard locker(*this);
    for (FileData* fd: mChildren)
    {
      if (fd->IsFolder())
      {
        int subCount = CastFolder(fd)->countItemsRecursively(filter, includefolders, autolock);
        if (result += subCount; subCount > 1)
          if (includefolders)
            result++; // Include folders iif it contains more than one game.
      }
      else if (fd->IsGame())
        if (filter->ApplyFilter(*fd))
          result++;
    }
  }
  else
  {
    LockCopy(children);
    for (FileData* fd: children)
    {
      if (fd->IsFolder())
      {
        int subCount = CastFolder(fd)->countItemsRecursively(filter, includefolders, autolock);
        if (result += subCount; subCount > 1)
          if (includefolders)
            result++; // Include folders iif it contains more than one game.
      }
      else if (fd->IsGame())
        if (filter->ApplyFilter(*fd))
          result++;
    }
  }
  filter->TearDown();
  return result;
}

int FolderData::countAllGamesAndFavoritesAndHidden(Filter excludes, int& favorites, int& hidden) const
{
  int result = 0;
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
  {
    if (fd->IsFolder())
    {
      int subCount = CastFolder(fd)->countAllGamesAndFavoritesAndHidden(excludes, favorites, hidden);
      result += subCount;
    }
    else if (fd->IsGame())
    {
      if (IsFiltered(fd, Filter::Normal | Filter::Favorite, excludes))
      {
        if (fd->Metadata().Favorite()) favorites++;
        result++;
      }
      else hidden++;
    }
  }
  return result;
}

int FolderData::countItemsRecursively(Filter includes, Filter excludes, bool includefolders) const
{
  int result = 0;
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
  {
    if (fd->IsFolder())
    {
      int subCount = CastFolder(fd)->countItemsRecursively(includes, excludes, includefolders);
      result += subCount;
      if (subCount > 1)
        if (includefolders)
          result++; // Include folders iif it contains more than one game.
    }
    else if (fd->IsGame())
      if (IsFiltered(fd, includes, excludes))
        result++;
  }
  return result;
}

int FolderData::countAllRecursively() const
{
  int result = 0;
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder()) result += CastFolder(fd)->countAllRecursively();
    else if (fd->IsGame()) result++;
  return result;
}

bool FolderData::HasGame() const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsGame() || (fd->IsFolder() && CastFolder(fd)->HasGame()))
      return true;
  return false;
}

bool FolderData::HasVisibleGame(FileData::TopLevelFilter filter) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if ((fd->IsGame() && fd->IsDisplayable(filter)) || (fd->IsFolder() && CastFolder(fd)->HasVisibleGame(filter)))
      return true;
  return false;
}

bool FolderData::HasScrapableGame() const
{
  if (TopAncestor().ReadOnly() || TopAncestor().PreInstalled()) return false;
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if ( (fd->IsGame()) || (fd->IsFolder() && CastFolder(fd)->HasScrapableGame()))
      return true;
  return false;
}

bool FolderData::HasTateGame() const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if ( (fd->IsGame() && (fd->Metadata().Rotation() == RotationType::Left || fd->Metadata().Rotation() == RotationType::Right)) ||
         (fd->IsFolder() && CastFolder(fd)->HasTateGame()))
      return true;
  return false;
}

bool FolderData::HasVisibleGameWithVideo(TopLevelFilter filter) const
{
  LockCopy(children);
  for (FileData* fd : children)
    if (fd->IsGame() && !fd->IsDisplayable(filter))
    {
      Path video(fd->Metadata().Video());
      if (!video.IsEmpty() && video.Exists()) return true;
    }
    else if (fd->IsFolder() && CastFolder(fd)->HasVisibleGameWithVideo(filter)) return true;
  return false;
}

bool FolderData::HasFilteredItemsRecursively(IFilter* filter, bool autolock) const
{
  bool result = false;
  if (autolock)
  {
    AutoGuard locker(*this);
    for (FileData* fd: mChildren)
      if (fd->IsFolder())
      {
        if (CastFolder(fd)->HasFilteredItemsRecursively(filter, autolock))
        { result = false; break; }
      }
      else if (fd->IsGame())
        if (filter->ApplyFilter(*fd))
        { result = true; break; }
  }
  else
  {
    LockCopy(children);
    for (FileData* fd: children)
      if (fd->IsFolder())
      {
        if (CastFolder(fd)->HasFilteredItemsRecursively(filter, autolock))
        { result = true; break; }
      }
      else if (fd->IsGame())
        if (filter->ApplyFilter(*fd))
        { result = true; break; }
  }
  filter->TearDown();
  return result;
}

int FolderData::getItems(FileData::List& to, Filter includes, Filter excludes, bool includefolders) const
{
  int gameCount = 0;
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
  {
    if (fd->IsFolder())
    {
      FolderData* folder = CastFolder(fd);
      // Seek for isolated file
      FileData* isolatedFile = nullptr;
      folder->Lock();
      while ((folder->ChildrenCount() == 1) && folder->FirstChild()->IsFolder())
      {
        FolderData* subFolder = CastFolder(folder->FirstChild());
        folder->Unlock();
        folder = subFolder;
        folder->Lock();
      }
      if (folder->ChildrenCount() == 1)
      {
        FileData* item = folder->FirstChild();
        if (item->IsGame())
          if (IsFiltered(item, includes, excludes))
            isolatedFile = item;
      }
      folder->Unlock();
      if (isolatedFile != nullptr) to.Add(isolatedFile);
      else if (includefolders)
        if (folder->countItems(includes, excludes, includefolders ) > 0) // Only add if it contains at leas one game
          to.Add(fd);
    }
    else if(IsFiltered(fd, includes, excludes))
    {
      to.Add(fd);
      gameCount++;
    }
  }
  return gameCount;
}

int FolderData::countItems(Filter includes, Filter excludes, bool includefolders) const
{
  int result = 0;
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
  {
    if (fd->IsFolder())
    {
      FolderData* folder = CastFolder(fd);
      // Seek for isolated file
      FileData* isolatedFile = nullptr;
      folder->Lock();
      while((folder->ChildrenCount() == 1) && folder->FirstChild()->IsFolder())
      {
        FolderData* nextFolder = CastFolder(folder->FirstChild());
        folder->Unlock();
        folder = nextFolder;
        folder->Lock();
      }
      if (folder->ChildrenCount() == 1)
      {
        FileData* item = folder->FirstChild();
        if (item->IsGame())
        {
          if(IsFiltered(fd, includes, excludes))
            isolatedFile = item;
        }
      }
      folder->Unlock();
      if (isolatedFile != nullptr) result++;
      else
        if (includefolders)
          if (folder->countItems(includes, excludes, includefolders) > 0) // Only add if it contains at leas one game
            result++;
    }
    else if (fd->IsGame())
    {
      if (IsFiltered(fd, includes, excludes))
        result++;
    }
  }
  return result;
}

void FolderData::ParseAllItems(IParser& parser, bool autolock)
{
  if (autolock)
  {
    AutoGuard locker(*this);
    for (FileData* fd : mChildren)
    {
      parser.Parse(*fd);
      if (fd->IsFolder()) CastFolder(fd)->ParseAllItems(parser, autolock);
    }
    return;
  }
  LockCopy(children);
  for (FileData* fd : children)
  {
    parser.Parse(*fd);
    if (fd->IsFolder()) CastFolder(fd)->ParseAllItems(parser, autolock);
  }
}

bool FolderData::IsFiltered(FileData* fd, FileData::Filter includes, FileData::Filter excludes)
{
  Filter currentIncludes = Filter::None;
  if (fd->Metadata().Favorite())                         currentIncludes |= Filter::Favorite;
  if (currentIncludes == 0)                              currentIncludes = Filter::Normal;

  Filter currentExcludes = Filter::None;
  if (fd->Metadata().Hidden())                           currentExcludes |= Filter::Hidden;
  if (fd->Metadata().NoGame())                           currentExcludes |= Filter::NoGame;
  if (!fd->Metadata().LatestVersion())                   currentExcludes |= Filter::NotLatest;
  if (fd->TopAncestor().PreInstalled())                  currentExcludes |= Filter::PreInstalled;
  if (!fd->System().IncludeAdultGames() &&
      fd->Metadata().Adult())                            currentExcludes |= Filter::Adult;
  if (fd->Metadata().GenreId() == GameGenres::Board ||
      fd->Metadata().GenreId() == GameGenres::Trivia ||
      fd->Metadata().GenreId() == GameGenres::Casino)    currentExcludes |= Filter::Board;
  if (!RotationUtils::IsTate(fd->Metadata().Rotation())) currentExcludes |= Filter::Yoko;
  if (fd->Metadata().PlayerMax() < 3)                    currentExcludes |= Filter::OneAndTwoPlayers;
  if (RotationUtils::IsTate(fd->Metadata().Rotation()))  currentExcludes |= Filter::Tate;

  return ((currentIncludes & includes) != 0 && (currentExcludes & excludes) == 0);
}

bool FolderData::LookupGame(const FileData* game) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
  {
    if (fd == game) return true;
    if (fd->IsFolder())
      if (CastFolder(fd)->LookupGame(game))
        return true;
  }
  return false;
}

FileData* FolderData::LookupGame(const String& item, SearchAttributes attributes, const String& path) const
{
  // Recursively look for the game in subfolders too
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
  {
    if (fd->IsFolder())
    {
      FolderData* folder = CastFolder(fd);
      FileData* result = folder->LookupGame(item, attributes, path);
      if (result != nullptr)
        return result;
    }
    else
    {
      Path filePath(fd->RomPath());
      String filename = path.empty() ? filePath.ToString() : path + '/' + filePath.Filename();

      if ((attributes & SearchAttributes::ByHash) != 0 && fd->Metadata().RomCrc32AsString() == item) return fd;
      if ((attributes & SearchAttributes::ByNameWithExt) != 0 && strcasecmp(filename.c_str(), item.c_str()) == 0) return fd;
      if ((attributes & SearchAttributes::ByName) != 0)
        if (strcasecmp((path.empty() ? filePath.FilenameWithoutExtension() : path + '/' + filePath.FilenameWithoutExtension()).c_str(), item.c_str()) == 0)
          return fd;
    }
  }
  return nullptr;
}

FileData* FolderData::LookupGameByFilePath(const String& filepath) const
{
  // Recursively look for the game in subfolders too
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder())
    {
      FolderData* folder = CastFolder(fd);
      FileData* result = folder->LookupGameByFilePath(filepath);
      if (result != nullptr)
        return result;
    }
    else
      if (filepath == fd->RomPath().ToString())
        return fd;

  return nullptr;
}

FileData* FolderData::LookupGameByCRC32(int crc32) const
{
  if (crc32 == 0) return nullptr;
  // Recursively look for the game in subfolders too
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder())
    {
      FolderData* folder = CastFolder(fd);
      FileData* result = folder->LookupGameByCRC32(crc32);
      if (result != nullptr)
        return result;
    }
    else
      if (crc32 == fd->Metadata().RomCrc32())
        return fd;

  return nullptr;
}

void FolderData::Sort(FileData::List& items, FileSorts::Comparer comparer, bool ascending)
{
  if (items.Count() > 1)
  {
    if (ascending)
      QuickSortAscending(items, 0, items.Count() - 1, comparer);
    else
      QuickSortDescending(items, 0, items.Count() - 1, comparer);
  }
}

void FolderData::QuickSortAscending(FileData::List& items, int low, int high, FileSorts::Comparer comparer)
{
  int Low = low;
  int High = high;
  const FileData& pivot = *items[(Low + High) >> 1];
  do
  {
    while((*comparer)(*items[Low] , pivot) < 0) Low++;
    while((*comparer)(*items[High], pivot) > 0) High--;
    if (Low <= High)
    {
      FileData* Tmp = items[Low]; items(Low) = items[High]; items(High) = Tmp;
      Low++; High--;
    }
  }while(Low <= High);
  if (High > low) QuickSortAscending(items, low, High, comparer);
  if (Low < high) QuickSortAscending(items, Low, high, comparer);
}

void FolderData::QuickSortDescending(FileData::List& items, int low, int high, FileSorts::Comparer comparer)
{
  int Low = low;
  int High = high;
  const FileData& pivot = *items[(Low + High) >> 1];
  do
  {
    while((*comparer)(*items[Low] , pivot) > 0) Low++;
    while((*comparer)(*items[High], pivot) < 0) High--;
    if (Low <= High)
    {
      FileData* Tmp = items[Low]; items(Low) = items[High]; items(High) = Tmp;
      Low++; High--;
    }
  }while(Low <= High);
  if (High > low) QuickSortDescending(items, low, High, comparer);
  if (Low < high) QuickSortDescending(items, Low, high, comparer);
}

bool FolderData::Contains(const FileData* item, bool recurse) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
  {
    if ((fd->IsFolder()) && recurse)
      if (Contains(fd, true)) return true;
    if (fd == item) return true;
  }
  return false;
}

FileData::List FolderData::GetFilteredItemsRecursively(IFilter* filter, bool includefolders, bool autolock) const
{
  FileData::List result;
  result.Reserve(countItemsRecursively(Filter::All, System().Excludes(), includefolders)); // Allocate once
  getItemsRecursively(result, filter, includefolders, autolock);

  return result;
}

FileData::List FolderData::GetAllItemsRecursively(bool includefolders, Filter excludes) const
{
  FileData::List result;
  result.Reserve(countItemsRecursively(Filter::All, excludes, includefolders)); // Allocate once
  getItemsRecursively(result, Filter::All, excludes, includefolders);

  return result;
}

FileData::List FolderData::GetAllItems(bool includefolders, Filter excludes) const
{
  FileData::List result;
  result.Reserve(countItems(Filter::All, excludes, includefolders)); // Allocate once
  getItems(result, Filter::All, excludes ,includefolders);

  return result;
}

bool FolderData::IsDirty() const
{
  if (TopAncestor().HasDeletedChildren()) return true;
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
  {
    if (fd->Metadata().IsDirty()) return true;
    if (fd->IsFolder() && CastFolder(fd)->IsDirty()) return true;
  }
  return false;
}

int FolderData::RemoveInvalids(ISystemScan* interface)
{
  int removed = 0;
  for (int i = mChildren.Count(); --i >= 0;)
  {
    FileData* fd = mChildren[i];
    if (fd->IsGame() && !fd->Metadata().IsValidated())
    {
      if (!fd->RomPath().Exists())
      {
        if (interface != nullptr)
          interface->SystemScanGameRemoved(System(), *fd);
        RootFolderData::DeleteChild(fd);
        removed++;
      }
    }
    else if (fd->IsFolder()) removed += CastFolder(fd)->RemoveInvalids(interface);
  }
  if (IsFolderOnly())
    if (!Metadata().IsValidated())
      RootFolderData::DeleteChild(this);
  return removed;
}

void FolderData::InvalidateAll()
{
  Metadata().Invalidate();
  for (FileData* fd : mChildren)
  {
    if (fd->IsGame()) fd->Metadata().Invalidate();
    if (fd->IsFolder()) CastFolder(fd)->InvalidateAll();
  }
}

void FolderData::ValidateAll()
{
  Metadata().Validate();
  for (FileData* fd : mChildren)
  {
    if (fd->IsGame()) fd->Metadata().Validate();
    if (fd->IsFolder()) CastFolder(fd)->ValidateAll();
  }
}

int FolderData::GetFoldersRecursivelyTo(FileData::List& to) const
{
  if (IsTopMostRoot())
  {
    int count = 0;
    for (const RootFolderData* root : ((MasterRootFolderData*) this)->SubRoots())
      count += root->getAllFoldersRecursively(to);
    return count;
  }

  return getAllFoldersRecursively(to);
}

int FolderData::GetItemsRecursivelyTo(FileData::List& to, FileData::Filter includes, FileData::Filter excludes, bool includefolders) const
{
  if (IsTopMostRoot())
  {
    int count = 0;
    for (const RootFolderData* root : ((MasterRootFolderData*) this)->SubRoots())
      count += root->getItemsRecursively(to, includes, excludes, includefolders);
    return count;
  }

  return getItemsRecursively(to, includes, excludes, includefolders);
}

int FolderData::GetItemsTo(FileData::List& to, FileData::Filter includes, FileData::Filter excludes, bool includefolders) const
{
  if (IsTopMostRoot())
  {
    int count = 0;
    for (const RootFolderData* root : ((MasterRootFolderData*) this)->SubRoots())
      count += root->getItems(to, includes, excludes, includefolders);
    return count;
  }

  return getItems(to, includes, excludes, includefolders);
}

void FolderData::LookupGamesFromPath(const StringHolderTypes::IndexAndDistance& index, FileData::List& games) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder()) CastFolder(fd)->LookupGamesFromPath(index, games);
    else if (fd->Metadata().IsMatchingFileIndex(index.Index))
      games.Add(fd);
}

void FolderData::LookupGamesFromName(const StringHolderTypes::IndexAndDistance& index, FileData::List& games) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder()) CastFolder(fd)->LookupGamesFromName(index, games);
    else if (fd->Metadata().IsMatchingNameIndex(index.Index))
      games.Add(fd);
}

void FolderData::LookupGamesFromDescription(const StringHolderTypes::IndexAndDistance& index, FileData::List& games) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder()) CastFolder(fd)->LookupGamesFromDescription(index, games);
    else if (fd->Metadata().IsMatchingDescriptionIndex(index.Index))
      games.Add(fd);
}

void FolderData::LookupGamesFromDeveloper(const StringHolderTypes::IndexAndDistance& index, FileData::List& games) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder()) CastFolder(fd)->LookupGamesFromDeveloper(index, games);
    else if (fd->Metadata().IsMatchingDeveloperIndex(index.Index))
      games.Add(fd);
}

void
FolderData::LookupGamesFromPublisher(const StringHolderTypes::IndexAndDistance& index, FileData::List& games) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder()) CastFolder(fd)->LookupGamesFromPublisher(index, games);
    else if (fd->Metadata().IsMatchingPublisherIndex(index.Index))
      games.Add(fd);
}

void FolderData::LookupGamesFromAll(const StringHolderTypes::IndexAndDistance& index, FileData::List& games) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder()) CastFolder(fd)->LookupGamesFromAll(index, games);
    else switch ((FastSearchContext) index.Context)
    {
      case FastSearchContext::Path: { if (fd->Metadata().IsMatchingFileIndex(index.Index)) games.Add(fd); break; }
      case FastSearchContext::Name: { if (fd->Metadata().IsMatchingNameIndex(index.Index)) games.Add(fd); break; }
      case FastSearchContext::Description: { if (fd->Metadata().IsMatchingDescriptionIndex(index.Index)) games.Add(fd); break; }
      case FastSearchContext::Developer: { if (fd->Metadata().IsMatchingDeveloperIndex(index.Index)) games.Add(fd); break; }
      case FastSearchContext::Publisher: { if (fd->Metadata().IsMatchingPublisherIndex(index.Index)) games.Add(fd); break; }
      case FastSearchContext::All:
      case FastSearchContext::Licences :
      default: break;
    }
}

void FolderData::BuildFastSearchSeriesPath(FolderData::FastSearchItemSerie& into) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder()) CastFolder(fd)->BuildFastSearchSeriesPath(into);
    else into.Set(fd, fd->Metadata().FileIndex());
}

void FolderData::BuildFastSearchSeriesName(FolderData::FastSearchItemSerie& into) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder()) CastFolder(fd)->BuildFastSearchSeriesName(into);
    else  into.Set(fd, !fd->Metadata().Aliases().empty() ? fd->Metadata().AliasesIndex() : fd->Metadata().NameIndex());
}

void FolderData::BuildFastSearchSeriesLicences(FolderData::FastSearchItemSerie& into) const
{
  AutoGuard locker(*this);
  for(const FileData* fd : mChildren)
    if (fd->IsFolder()) CastFolder(fd)->BuildFastSearchSeriesLicences(into);
    else into.Set(fd, fd->Metadata().LicencesIndex());
}

void FolderData::BuildFastSearchSeriesDescription(FolderData::FastSearchItemSerie& into) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder()) CastFolder(fd)->BuildFastSearchSeriesDescription(into);
    else into.Set(fd, fd->Metadata().DescriptionIndex());
}

void FolderData::BuildFastSearchSeriesDeveloper(FolderData::FastSearchItemSerie& into) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder()) CastFolder(fd)->BuildFastSearchSeriesDeveloper(into);
    else into.Set(fd, fd->Metadata().DeveloperIndex());
}

void FolderData::BuildFastSearchSeriesPublisher(FolderData::FastSearchItemSerie& into) const
{
  AutoGuard locker(*this);
  for (FileData* fd : mChildren)
    if (fd->IsFolder()) CastFolder(fd)->BuildFastSearchSeriesPublisher(into);
    else into.Set(fd, fd->Metadata().PublisherIndex());
}

std::random_device FolderData::sRandomDevice;
std::mt19937 FolderData::sRandomGenerator(sRandomDevice());

void FolderData::Shuffle(FileData::List& list)
{
  // Initialize randomize
  std::uniform_int_distribution<int> randomizer(0, list.Count() - 1);
  for(int i = list.Count(); --i >= 0;)
  {
    int j = randomizer(sRandomGenerator);
    list.Swap(i, j);
  }
}
