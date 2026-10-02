//
// Created by Pit64 on 05/02/26.
//
#pragma once

#include "systems/BaseSystemDownloader.h"
#include "utils/network/HttpClient.h"
#include <utils/sync/SyncMessageSender.h>
#include "utils/os/system/Thread.h"

enum class TombRaiderDownloadingGameState
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

class TombRaiderDownloader : public BaseSystemDownloader
                       , private ISyncMessageReceiver<TombRaiderDownloadingGameState>
                       , private HttpClient::IDownload
{
    public:
      /*!
       * @brief Constructor
       * @param updater UI update interface
       **/
    TombRaiderDownloader(SystemData& TombRaider, IGuiDownloaderUpdater& updater);

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
      static constexpr const char* sRepoURL = "https://gitlab.com/recalbox/packages/game-providers/tombraider/-/archive/master/tombraider-master.zip";

      //! Http request object
      HttpClient mRequest;

      //! Sync messager
      SyncMessageSender<TombRaiderDownloadingGameState> mSender;

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
      void ReceiveSyncMessage(const TombRaiderDownloadingGameState& code) override;

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
