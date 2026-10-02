//
// Created by gugue_u on 12/05/2021.
//


#include "FileData.h"
#include "SaveState.h"
#include <utils/storage/Set.h>
#include <views/gamelist/ISimpleGameListView.h>

class GameFilesUtils
{
  public:
    static HashSet<Path> GetGameSubFiles(FileData& game);
    static HashSet<Path> GetGameExtraFiles(FileData& fileData);
    static HashSet<Path> GetGameSaveFiles(FileData& game);
    static HashSet<Path> GetMediaFiles(FileData& fileData);
    static bool HasAutoPatch(const FileData* fileData);
    static Path GetSubDirPriorityPatch(const FileData* fileData);
    static std::vector<Path> GetSoftPatches(const FileData* fileData);
    static std::vector<SaveState> GetGameSaveStateFiles(FileData& game);

    static bool ContainsMultiDiskFile(const String& extensions)
    {
      return extensions.Contains(".m3u") || extensions.Contains(".cue") ||
             extensions.Contains(".ccd") || extensions.Contains(".gdi");
    }

    static void ExtractUselessFiles(const Path& path, HashSet<Path>& list);
    static void ExtractUselessFilesFromCue(const Path& path, HashSet<Path>& list);
    static void ExtractUselessFilesFromCcd(const Path& path, HashSet<Path>& list);
    static void ExtractUselessFilesFromM3u(const Path& path, HashSet<Path>& list);
    static void ExtractUselessFilesFromGdi(const Path& path, HashSet<Path>& list);
    static String ExtractFileNameFromLine(const String& line);
    static void AddIfExist(const Path& path, HashSet<Path>& list);
    static constexpr int sMaxGdiFileSize = (10 << 10); // 10 Kb

    static void DeleteSelectedFiles(FileData& fileData, const HashSet<Path>&, const HashSet<Path>&);
    static void DeleteAllFiles(FileData& fileData);

    static bool IsMediaShared(FileData& system, const Path& mediaPath);

    static void DeleteFoldersRecIfEmpty(FolderData* folderData);
};


