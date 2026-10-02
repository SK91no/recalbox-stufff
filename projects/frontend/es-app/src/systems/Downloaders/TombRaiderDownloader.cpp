//
// Created by Pit64 on 05/02/26.
//

#include "TombRaiderDownloader.h"
#include "systems/SystemManager.h"
#include "utils/locale/LocaleHelper.h"
#include "utils/Files.h"
#include "utils/Zip.h"
#include "utils/hash/Crc32.h"

TombRaiderDownloader::TombRaiderDownloader(SystemData& TombRaider, IGuiDownloaderUpdater& updater)
    : BaseSystemDownloader(TombRaider, updater)
    , mSender(*this)
    , mSystem(TombRaider)
    , mTotalSize(0)
    , mCurrentSize(0)
    , mGames(0)
{
}

void TombRaiderDownloader::DownloadAndInstall()
{
    mSender.Send(TombRaiderDownloadingGameState::Start);
    usleep(20000); // Let display refreshing
    { LOG(LogDebug) << "[TombRaiderDownloader] Download files for " << mSystem.FullName(); }

    // Seek for the right folder
    const MasterRootFolderData& roots = mSystem.MasterRoot();
    RootFolderData* targetRoot = nullptr;
    Path output;
    for (RootFolderData* root : roots.SubRoots())
        if (!root->ReadOnly() && root->RomPath().StartWidth("/recalbox/share/roms/"))
        {
            output = root->RomPath();
            targetRoot = root;
        }
    if (output.IsEmpty()) { mSender.Send(TombRaiderDownloadingGameState::WriteOnlyShare); return; }

    // Get destination filename
    Path destination("/recalbox/share/system/download.tmp");
    { LOG(LogDebug) << "[TombRaiderDownloader] Target path " << destination.ToString(); }

    // Source URL
    String source(sRepoURL);

    // Download
    if (mStopAsap) return;
    (void)destination.Delete();
    mTimeReference = DateTime();
    if (!mRequest.Execute(source, destination, this)) { mSender.Send(TombRaiderDownloadingGameState::DownloadError); return; }

    // Extract
    { LOG(LogDebug) << "[TombRaiderDownloader] Extracting game"; }
    Zip zip(destination);
    // Fill in storage
    mTotalSize = zip.Count();
    mCurrentSize = 0;
    String gamelist;
    for (int i = (int)mTotalSize; --i >= 0;)
    {
        if (mStopAsap) return;
        Path relativePath(zip.FileName(i));
        Path destinationPath = output / relativePath.FromItem(1);
        { LOG(LogDebug) << "[TombRaiderDownloader] File " << destinationPath.ToString(); }
        if (relativePath.Filename() == "gamelist.xml") gamelist = zip.Content(i);
        else
        {
            (void) destinationPath.Directory().CreatePath();
            if (!destinationPath.Exists())
            {
                String content = zip.Content(i);
                int crc32 = (int)crc32_16bytes(content.data(), content.size(), 0);
                if (targetRoot->LookupGameByCRC32(crc32) == nullptr)
                {
                    Files::SaveFile(destinationPath, zip.Content(i));
                    if (mSystem.Descriptor().Extension().Contains(relativePath.Extension().ToLowerCase())) mGames++;
                }
                else
                { LOG(LogDebug) << "[TombRaiderDownloader] CRC Match"; }
            }
        }
        mCurrentSize++;
        mSender.Send(TombRaiderDownloadingGameState::Extracting);
        usleep(20000); // Let display refreshing
    }

    // Populate
    { LOG(LogDebug) << "[TombRaiderDownloader] Populate gamelist"; }
    XmlDocument gameList;
    XmlResult result = gameList.load_string(gamelist.data());
    if (result)
    {
        XmlNode games = gameList.child("gameList");
        mTotalSize = std::distance(games.children().begin(), games.children().end());
        mCurrentSize = 0;
        mSender.Send(TombRaiderDownloadingGameState::UpdatingMetadata);
        FileData::StringMap doppleGanger;
        if (mStopAsap) return;
        targetRoot->BuildDoppelgangerMap(doppleGanger, true);
        if (mStopAsap) return;
        String ignoreList(',');
        ignoreList.Append(mSystem.Descriptor().IgnoredFiles()).Append(',');
        targetRoot->PopulateRecursiveFolder(*targetRoot, mSystem.Descriptor().Extension().ToLowerCase(), "", doppleGanger, nullptr);

        for (const XmlNode fileNode : games.children())
        {
            // Build a temporary game object...
            if (String(fileNode.name()) == "game")
            {
                Path path = output / Xml::AsString(fileNode, "path", "");
                FileData game(path, *targetRoot);
                // ...and deserialize metadata into
                game.Metadata().Deserialize(fileNode, "", output);
                // Lookup real game
                FileData* realGame = targetRoot->LookupGameByFilePath(path.ToString());
                if (realGame == nullptr)
                { LOG(LogError) << "[TombRaiderDownloader] Cannot lookup real game!"; }
                else
                {
                    // Merge data from
                    realGame->Metadata().Merge(game.Metadata());
                    // Special process for game name
                    String name(realGame->Name());
                    if (name.empty() || name == realGame->RomPath().FilenameWithoutExtension())
                        realGame->Metadata().SetName(game.Name());
                }
            }
            mCurrentSize++;
            mSender.Send(TombRaiderDownloadingGameState::UpdatingMetadata);
            usleep(20000); // Let display refreshing
        }
    }
    else { LOG(LogError) << "[TombRaiderDownloader] Cannot load remote gamelist!"; }

    // Delete temp file
    (void)destination.Delete();
}

void TombRaiderDownloader::DownloadProgress(const HttpClient& http, long long currentSize, long long expectedSize)
{
    (void)http;
    // Store data and synchronize
    mTotalSize = expectedSize;
    mCurrentSize = currentSize;
    mSender.Send(TombRaiderDownloadingGameState::Downloading);
}

void TombRaiderDownloader::ReceiveSyncMessage(const TombRaiderDownloadingGameState& code)
{
    switch (code)
    {
        case TombRaiderDownloadingGameState::Start:
        {
            String title(_("DOWNLOADING GAME FOR %s"));
            mUpdater.UpdateTitleText(title.Replace("%s", mSystem.FullName()));
            mUpdater.UpdateMainText(_("Downloading Tomb Raider demo game from the official site. Please wait..."));
            break;
        }
        case TombRaiderDownloadingGameState::Downloading:
        {
            // Load size into progress bar component
            mUpdater.UpdateProgressbar(mCurrentSize, mTotalSize);

            // Elapsed time
            if (mCurrentSize != 0 && mCurrentSize < mTotalSize)
            {
                TimeSpan elapsed = DateTime() - mTimeReference;
                TimeSpan eta((elapsed.TotalMilliseconds() * (mTotalSize - mCurrentSize)) / mCurrentSize);

                String text = _("Downloading... Estimated time: %s").Replace("%s", eta.ToTimeString());
                mUpdater.UpdateETAText(text);
            }
            break;
        }
        case TombRaiderDownloadingGameState::Extracting:
        {
            // Load size into progress bar component
            mUpdater.UpdateProgressbar(mCurrentSize, mTotalSize);

            String text = _("Extracting... found 1 game");
            mUpdater.UpdateETAText(text);
            break;
        }
        case TombRaiderDownloadingGameState::UpdatingMetadata:
        {
            // Load size into progress bar component
            mUpdater.UpdateProgressbar(mCurrentSize, mTotalSize);

            String text = _("Updating metadata...").Replace("%s", String(mCurrentSize));
            if (mCurrentSize == 0) text = _("Refreshing gamelist...");
            mUpdater.UpdateETAText(text);
            break;
        }
        case TombRaiderDownloadingGameState::WriteOnlyShare:
        {
            mUpdater.UpdateETAText(_("Can't write game to share!"));
            break;
        }
        case TombRaiderDownloadingGameState::DownloadError:
        {
            mUpdater.UpdateETAText(_("Error downloading game! Retry later."));
            break;
        }
    }
}

void TombRaiderDownloader::Completed(bool stopped)
{
    mUpdater.DownloadComplete(mSystem, stopped);
}
