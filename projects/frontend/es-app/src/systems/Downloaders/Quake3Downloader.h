//
// Created by Pit64 on 04/02/26.
//
#pragma once

#include "systems/BaseSystemDownloader.h"
#include "utils/network/HttpClient.h"
#include <utils/sync/SyncMessageSender.h>
#include "utils/os/system/Thread.h"

enum class Quake3DownloadingGameState
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

class Quake3Downloader : public BaseSystemDownloader
                       , private ISyncMessageReceiver<Quake3DownloadingGameState>
                       , private HttpClient::IDownload
{
    public:
      /*!
       * @brief Constructor
       * @param updater UI update interface
       **/
    Quake3Downloader(SystemData& quake3, IGuiDownloaderUpdater& updater);

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
      static constexpr const char* sRepoURL = "https://gitlab.com/recalbox/packages/game-providers/quake3/-/archive/demo/quake3-demo.zip";

      //! Http request object
      HttpClient mRequest;

      //! Sync messager
      SyncMessageSender<Quake3DownloadingGameState> mSender;

      //! Time reference
      DateTime mTimeReference;

      //! Quake 3 system reference
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
      void ReceiveSyncMessage(const Quake3DownloadingGameState& code) override;

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
