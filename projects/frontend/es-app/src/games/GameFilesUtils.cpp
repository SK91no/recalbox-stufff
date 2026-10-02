//
// Created by gugue_u on 12/05/2021.
//

#include <vector>
#include <utils/os/fs/Path.h>
#include <utils/Files.h>
#include <systems/SystemData.h>
#include "GameFilesUtils.h"
#include "FileData.h"
#include <views/ViewController.h>

HashSet<Path> GameFilesUtils::GetGameSubFiles(FileData& game)
{
  HashSet<Path> list;
  if (game.IsGame())
  {
    ExtractUselessFiles(game.RomPath(), list);
  }
  return list;
}

HashSet<Path> GameFilesUtils::GetGameSaveFiles(FileData& game)
{
  HashSet<Path> list;
  Path directory = Path("/recalbox/share/saves") / game.System().Name();

  if (game.IsGame())
  {
    for (const auto& path: directory.GetDirectoryContent())
    {
      if (path.FilenameWithoutExtension() == game.RomPath().FilenameWithoutExtension())
      {
        AddIfExist(path, list);
        // for next savestate screenshot feat
        AddIfExist(path.ChangeExtension(path.Extension() + ".png"), list);
      }
    }
    return list;
  }
  return list;
}

std::vector<SaveState> GameFilesUtils::GetGameSaveStateFiles(FileData& game)
{
  std::vector<SaveState> list;
  Path directory = Path("/recalbox/share/saves") / game.System().Name();

  if (game.IsGame())
  {
    for (const auto& path: directory.GetDirectoryContent())
    {
      if (path.FilenameWithoutExtension() == game.RomPath().FilenameWithoutExtension() && path.Extension().starts_with(".state"))
      {
        list.push_back(*new SaveState(path));
      }

      if (path.FilenameWithoutExtension() == game.RomPath().FilenameWithoutExtension() + ".state" && path.Extension() == (".auto"))
      {
        list.push_back(*new SaveState(path));
      }
    }

  }
  return list;
}

HashSet<Path> GameFilesUtils::GetGameExtraFiles(FileData& fileData)
{
  HashSet<Path> list;
  const Path path = fileData.RomPath();
  if (fileData.IsGame())
  {
    for (const auto& file: path.Directory().GetDirectoryContent())
    {
      if (file.FilenameWithoutExtension() == path.FilenameWithoutExtension())
      {
        String extension = file.Extension().ToLowerCase();

        if (extension.Contains(".ups") || extension.Contains(".bps") || extension.Contains(".ips"))
          list.insert(path);
      }
    }

    AddIfExist(fileData.P2KPath(), list);
    AddIfExist(fileData.RecalboxConfPath(), list);

    // retroarch override conf by game
    //core configs
    for (const Path& configCorePath: Path("/recalbox/share/system/.config/retroarch/config").GetDirectoryContent())
    {
      if (configCorePath.IsDirectory())
      {
        for (const Path& subPath: configCorePath.GetDirectoryContent())
        {
          if (subPath.IsFile() && subPath.FilenameWithoutExtension() == fileData.RomPath().FilenameWithoutExtension())
            AddIfExist(subPath, list);
        }
      }
    }

    //remap configs
    for (const Path& remapCorePath: Path("/recalbox/share/system/.config/retroarch/config/remaps/").GetDirectoryContent())
    {
      if (remapCorePath.IsDirectory())
      {
        for (const Path& subPath: remapCorePath.GetDirectoryContent())
        {
          if (subPath.IsFile() && subPath.FilenameWithoutExtension() == fileData.RomPath().FilenameWithoutExtension())
            AddIfExist(subPath, list);
        }
      }
    }
  }

  return list;
}


bool GameFilesUtils::HasAutoPatch(const FileData* fileData)
{
  std::vector<Path> patches;
  if (!fileData->IsGame()) return false;

  Path path = fileData->RomPath();
  for (const auto& file: path.Directory().GetDirectoryContent())
    if (file.FilenameWithoutExtension() == path.FilenameWithoutExtension())
    {
      String extension = file.Extension().ToLowerCase();

      if (extension == ".ups" || extension ==".bps" ||extension == ".ips")
        return true;
    }

  return false;
}

Path GameFilesUtils::GetSubDirPriorityPatch(const FileData* fileData)
{
  if (!fileData->IsGame()) return Path("");

  Path romPath = fileData->RomPath();

  Path folder = romPath.Directory() / romPath.FilenameWithoutExtension().Append("-patches");
  Path::PathList subDirPatches = folder.GetDirectoryContent();

  for (const auto& file: subDirPatches)
  {
    String extension = file.Extension().ToLowerCase();

    if (extension == ".ups")
      return file;
  }
  for (const auto& file: subDirPatches)
  {
    String extension = file.Extension().ToLowerCase();

    if (extension == ".bps")
      return file;
  }
  for (const auto& file: subDirPatches)
  {
    String extension = file.Extension().ToLowerCase();

    if (extension == ".ips")
      return file;
  }

  if (subDirPatches.size() == 1)
    for (const auto& file: subDirPatches)
    {
      String extension = file.Extension().ToLowerCase();

      if (extension == ".ups" || extension == ".bps" || extension == ".ips")
        return file;
    }

  return Path();
}



std::vector<Path> GameFilesUtils::GetSoftPatches(const FileData* fileData)
{
  std::vector<Path> patches;
  if (!fileData->IsGame()) return patches;

  Path path = fileData->RomPath();
  Path folder = path.Directory() / path.FilenameWithoutExtension().Append("-patches");
  for (const auto& file: folder.GetDirectoryContent())
  {
    String extension = file.Extension().ToLowerCase();

    if (extension == ".ups" || extension ==".bps" ||extension == ".ips")
      patches.insert(patches.begin(), file);
  }

  for (const auto& file: path.Directory().GetDirectoryContent())
    if (file.FilenameWithoutExtension() == path.FilenameWithoutExtension())
    {
      String extension = file.Extension().ToLowerCase();

      if (extension == ".ups" || extension ==".bps" ||extension == ".ips")
        patches.insert(patches.begin(), file);
    }

  return patches;
}

HashSet<Path> GameFilesUtils::GetMediaFiles(FileData& fileData)
{
  HashSet<Path> list;
  if (fileData.Metadata().Image().Exists())
  {
    list.insert(fileData.Metadata().Image());
  }
  if (fileData.Metadata().Video().Exists())
  {
    list.insert(fileData.Metadata().Video());
  }
  if (fileData.Metadata().Box().Exists())
  {
    list.insert(fileData.Metadata().Box());
  }

  return list;
}

bool GameFilesUtils::IsMediaShared(FileData& fileData, const Path& mediaPath)
{

  for (const auto& other: fileData.System().getAllGames())
  {
    if (fileData.AreRomEqual(*other))
    {
      continue;
    }
    if (mediaPath == other->Metadata().Image())
      return true;
    if (mediaPath == other->Metadata().Box())
      return true;
    if (mediaPath == other->Metadata().Video())
      return true;
  }
  return false;
}

void GameFilesUtils::ExtractUselessFiles(const Path& path, HashSet<Path>& list)
{
  const String extension = path.Extension();
  if (extension == ".cue")
  {
    ExtractUselessFilesFromCue(path, list);
    return;
  }
  if (extension == ".ccd")
  {
    ExtractUselessFilesFromCcd(path, list);
    return;
  }
  if (extension == ".gdi" && path.Size() <= sMaxGdiFileSize)
  {
    ExtractUselessFilesFromGdi(path, list);
    return;
  }
  if (extension == ".m3u")
  {
    ExtractUselessFilesFromM3u(path, list);
    return;
  }
}

void GameFilesUtils::ExtractUselessFilesFromCue(const Path& path, HashSet<Path>& list)
{
  String file = Files::LoadFile(path);
  for (const String& line: file.Split('\n'))
    if (line.Contains("FILE") && line.Contains("BINARY"))
    {
      Path newfile = path.Directory() / ExtractFileNameFromLine(line);
      AddIfExist(newfile, list);
    }
}

void GameFilesUtils::ExtractUselessFilesFromCcd(const Path& path, HashSet<Path>& list)
{
  AddIfExist(path.ChangeExtension(".cue"), list);
  AddIfExist(path.ChangeExtension(".bin"), list);
  AddIfExist(path.ChangeExtension(".sub"), list);
  AddIfExist(path.ChangeExtension(".img"), list);
}

void GameFilesUtils::ExtractUselessFilesFromM3u(const Path& path, HashSet<Path>& list)
{
  String file = Files::LoadFile(path);
  for (String& line: file.Split('\n'))
  {
    if (line.Trim('\r').empty()) continue;

    Path newfile = path.Directory() / Path(line);
    AddIfExist(newfile, list);

    ExtractUselessFiles(newfile, list);
  }
}

void GameFilesUtils::ExtractUselessFilesFromGdi(const Path& path, HashSet<Path>& list)
{
  String file = Files::LoadFile(path);
  for (const String& line: file.Split('\n'))
  {
    AddIfExist(Path(ExtractFileNameFromLine(line)), list);
  }
}

String GameFilesUtils::ExtractFileNameFromLine(const String& line)
{
  // 1 check file name between double quotes
  if (String string; line.Extract('"', '"', string, true))
    if (string.Contains('.'))
      return string;

  // 2 check every words separated by space that contains dot
  for (const String& word: line.Split(' ', true))
    if (word.Contains('.'))
      return word;

  return "";
}

void GameFilesUtils::AddIfExist(const Path& path, HashSet<Path>& list)
{
  if (path.Exists())
    list.insert(path);
}

void GameFilesUtils::DeleteAllFiles(FileData& fileData)
{
  HashSet<Path> files;
  HashSet<Path> mediaFiles = GetMediaFiles(fileData);
  files.insert(fileData.RomPath());

  for (const auto& path: GetGameExtraFiles(fileData))
  {
    files.insert(path);
  }

  for (const auto& path: GetGameSaveFiles(fileData))
  {
    files.insert(path);
  }

  for (const auto& path: GetGameSubFiles(fileData))
  {
    files.insert(path);
  }

  DeleteSelectedFiles(fileData, files, mediaFiles);
}

void GameFilesUtils::DeleteSelectedFiles(FileData& fileData, const HashSet<Path>& paths, const HashSet<Path>& mediaPaths)
{
  SystemData& systemData = fileData.System();
  SystemData::PausedWatcher pause(systemData);

  { LOG(LogDebug) << "[DELETE] Begin delete of \"" << fileData.Name() << "\" deletion"; }
  if (paths.empty() && mediaPaths.empty())
  {
    { LOG(LogDebug) << "[DELETE] no file to delete for game " << fileData.Name(); }
    return;
  }

  bool mainFileDeleted = false;
  Path gamePath = fileData.RomPath();
  Path root = fileData.TopAncestor().RomPath();
  for (const auto& path : paths)
  {
    if (path == gamePath.ToString())
      mainFileDeleted = true;
    (void)Path(path).Delete();
    { LOG(LogDebug) << "[DELETE] Game file" << path << " has been deleted"; }
  }

  bool mediaIsDirty = false;
  for (const auto& mediaPath: mediaPaths)
  {
    Path path = Path(mediaPath);
    if (path == fileData.Metadata().Image())
    {
      fileData.Metadata().SetImagePath(Path::Empty);
      mediaIsDirty = true;
    }
    if (path == fileData.Metadata().Video().ToString())
    {
      fileData.Metadata().SetVideoPath(Path::Empty);
      mediaIsDirty = true;
    }
    if (path == fileData.Metadata().Box().ToString())
    {
      fileData.Metadata().SetBoxPath(Path::Empty);
      mediaIsDirty = true;
    }

    if (path.Exists() && !IsMediaShared(fileData, path))
    {
      (void)Path(mediaPath).Delete();
      { LOG(LogDebug) << "[DELETE] Game media file" << mediaPath << " has been deleted"; }
    }
    else
    { LOG(LogDebug) << "[DELETE] Game media file not exist or is shared" << mediaPath; }
  }

  // remmove fileData
  if (mainFileDeleted)
  {
    RootFolderData::DeleteChild(&fileData);
    //ViewController::Instance().RemoveGame(fileData);
    systemData.Manager().UpdateSystemsOnGameChange(&fileData, MetadataType::None, true);
    DeleteFoldersRecIfEmpty(fileData.Parent());
  }
  else if (mediaIsDirty)
  {
    //clean gamelist metadata
    fileData.Metadata().SetDirty();
  }

  systemData.UpdateGamelistXml();
}

void GameFilesUtils::DeleteFoldersRecIfEmpty(FolderData* folderData)
{
  if (folderData->IsRoot() || folderData->HasChildren())
  {
    { LOG(LogDebug) << "[DELETE] Directory " << folderData->RomPath().ToString() << " folder is not empty or root, it cannot be deleted"; }
    return;
  }

  FolderData* parent = folderData->Parent();
  Path currentFolder = folderData->RomPath();
  (void)currentFolder.Delete();
  RootFolderData::DeleteChild(folderData);
  { LOG(LogDebug) << "[DELETE] Directory " << currentFolder.ToString() << " is now empty and have been deleted"; }

  DeleteFoldersRecIfEmpty(parent);
}