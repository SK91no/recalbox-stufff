//
// Created by Pit64 on 26/03/25.
//
#pragma once

#include "systems/BaseSystemDownloader.h"
#include "utils/network/HttpClient.h"
#include <utils/sync/SyncMessageSender.h>
#include "utils/os/system/Thread.h"

enum class ThemeHospitalDownloadingGameState
{
  Start,             //!< Init
  // Actions
  Downloading,       //!< Downloading games
  Extracting,        //!< Extracting
  UpdatingMetadata,  //!< Update metadata
  // Errors
  WriteOnlyShare,    //!< Share is write only!
  DownloadError,     //!< Error downloading file(s)
};

class ThemeHospitalDownloader : public BaseSystemDownloader
                              , private ISyncMessageReceiver<ThemeHospitalDownloadingGameState>
                              , private HttpClient::IDownload
{
  public:
    /*!
     * @brief Constructor
     * @param updater UI update interface
     */
    ThemeHospitalDownloader(SystemData& themehospital, IGuiDownloaderUpdater& updater);

    /*
     * Http::IDownload implementation
     */

    /*!
     * @brief Notify of download progress
     * @param http HTTP request
     * @param currentSize downloaded bytes
     * @param expectedSize total expected bytes
     */
    void DownloadProgress(const HttpClient& http, long long currentSize, long long expectedSize) override;

  private:
    //! Game fetching URL
    static constexpr const char* sRepoURL = "https://gitlab.com/recalbox/packages/game-providers/themehospital/-/archive/main/themehospital-main.zip";

    //! Http request object
    HttpClient mRequest;

    //! Sync messager
    SyncMessageSender<ThemeHospitalDownloadingGameState> mSender;

    //! Time reference
    DateTime mTimeReference;

    //! Theme Hospital system reference
    SystemData& mSystem;

    //! File length
    long long mTotalSize;
    //! Downloaded length
    long long mCurrentSize;

    //! Extracted games
    int mGames;

    /*!
     * @brief Receive synchronous code
     */
    void ReceiveSyncMessage(const ThemeHospitalDownloadingGameState& code) override;

    /*
     * ISystemDownloader implementation
     */

    //! Start downloading & installing games
    void DownloadAndInstall() override;

    /*!
     * @brief Called once when the process is complete
     * @param stopped true if the process has been stopped by calling MustExitAsap
     */
    void Completed(bool stopped) override;
};
