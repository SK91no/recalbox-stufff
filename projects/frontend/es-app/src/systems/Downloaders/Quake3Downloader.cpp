//
// Created by Pit64 on 04/02/26.
//

#include "Quake3Downloader.h"
#include "systems/SystemManager.h"
#include "utils/locale/LocaleHelper.h"
#include "utils/Files.h"
#include "utils/Zip.h"
#include "utils/hash/Crc32.h"

Quake3Downloader::Quake3Downloader(SystemData& quake3, IGuiDownloaderUpdater& updater)
    : BaseSystemDownloader(quake3, updater)
    , mSender(*this)
    , mSystem(quake3)
    , mTotalSize(0)
    , mCurrentSize(0)
    , mGames(0)
{
}

void Quake3Downloader::DownloadAndInstall()
{
    mSender.Send(Quake3DownloadingGameState::Start);
    usleep(20000); // Let display refreshing
    { LOG(LogDebug) << "[Quake3Downloader] Download files for " << mSystem.FullName(); }

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
    if (output.IsEmpty()) { mSender.Send(Quake3DownloadingGameState::WriteOnlyShare); return; }

    // Get destination filename
    Path destination("/recalbox/share/system/download.tmp");
    { LOG(LogDebug) << "[Quake3Downloader] Target path " << destination.ToString(); }

    // Source URL
    String source(sRepoURL);

    // Download
    if (mStopAsap) return;
    (void)destination.Delete();
    mTimeReference = DateTime();
    if (!mRequest.Execute(source, destination, this)) { mSender.Send(Quake3DownloadingGameState::DownloadError); return; }

    // Extract
    { LOG(LogDebug) << "[Quake3Downloader] Extracting game"; }
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
        { LOG(LogDebug) << "[Quake3Downloader] File " << destinationPath.ToString(); }
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
                { LOG(LogDebug) << "[Quake3Downloader] CRC Match"; }
            }
        }
        mCurrentSize++;
        mSender.Send(Quake3DownloadingGameState::Extracting);
        usleep(20000); // Let display refreshing
    }

    // Populate
    { LOG(LogDebug) << "[Quake3Downloader] Populate gamelist"; }
    XmlDocument gameList;
    XmlResult result = gameList.load_string(gamelist.data());
    if (result)
    {
        XmlNode games = gameList.child("gameList");
        mTotalSize = std::distance(games.children().begin(), games.children().end());
        mCurrentSize = 0;
        mSender.Send(Quake3DownloadingGameState::UpdatingMetadata);
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
                { LOG(LogError) << "[Quake3Downloader] Cannot lookup real game!"; }
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
            mSender.Send(Quake3DownloadingGameState::UpdatingMetadata);
            usleep(20000); // Let display refreshing
        }
    }
    else { LOG(LogError) << "[Quake3Downloader] Cannot load remote gamelist!"; }

    // Delete temp file
    (void)destination.Delete();
}

void Quake3Downloader::DownloadProgress(const HttpClient& http, long long currentSize, long long expectedSize)
{
    (void)http;
    // Store data and synchronize
    mTotalSize = expectedSize;
    mCurrentSize = currentSize;
    mSender.Send(Quake3DownloadingGameState::Downloading);
}

void Quake3Downloader::ReceiveSyncMessage(const Quake3DownloadingGameState& code)
{
    switch (code)
    {
        case Quake3DownloadingGameState::Start:
        {
            String title(_("DOWNLOADING GAME FOR %s"));
            mUpdater.UpdateTitleText(title.Replace("%s", mSystem.FullName()));
            mUpdater.UpdateMainText(_("Downloading Quake 3 demo game from the official site. Please wait..."));
            break;
        }
        case Quake3DownloadingGameState::Downloading:
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
        case Quake3DownloadingGameState::Extracting:
        {
            // Load size into progress bar component
            mUpdater.UpdateProgressbar(mCurrentSize, mTotalSize);

            String text = _("Extracting... found 1 game");
            mUpdater.UpdateETAText(text);
            break;
        }
        case Quake3DownloadingGameState::UpdatingMetadata:
        {
            // Load size into progress bar component
            mUpdater.UpdateProgressbar(mCurrentSize, mTotalSize);

            String text = _("Updating metadata...").Replace("%s", String(mCurrentSize));
            if (mCurrentSize == 0) text = _("Refreshing gamelist...");
            mUpdater.UpdateETAText(text);
            break;
        }
        case Quake3DownloadingGameState::WriteOnlyShare:
        {
            mUpdater.UpdateETAText(_("Can't write game to share!"));
            break;
        }
        case Quake3DownloadingGameState::DownloadError:
        {
            mUpdater.UpdateETAText(_("Error downloading game! Retry later."));
            break;
        }
    }
}

void Quake3Downloader::Completed(bool stopped)
{
    mUpdater.DownloadComplete(mSystem, stopped);
}
