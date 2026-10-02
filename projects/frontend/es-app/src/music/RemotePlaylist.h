//
// Created by bkg2k on 29/05/22.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <utils/os/system/Thread.h>
#include <utils/network/HttpMd5.h>
#include <vector>
#include <utils/os/system/Signal.h>
#include <utils/os/fs/Path.h>
#include <utils/cplusplus/StaticLifeCycleControler.h>
#include <utils/storage/Set.h>
#include "utils/sync/ISyncMessageReceiver.h"
#include "IRemotePlaylistNotification.h"
#include "utils/network/HttpClient.h"
#include "utils/sync/SyncMessageSender.h"
#include "PlaylistCursor.h"

//! Progress indicator micro structure
struct RemoteTrackDownloadProgress
{
  long long int mCurrentSize;
  long long int mTotalSize;
};

class RemotePlaylist : public StaticLifeCycleControler<RemotePlaylist>
                     , private Thread
                     , private ISyncMessageReceiver<void>
                     , private ISyncMessageReceiver<RemoteTrackDownloadProgress>
                     , private HttpClient::IDownload
{
  public:
    //! Requested son progress interface
    class ISongProgress
    {
      public:
        //! Default destructor
        virtual ~ISongProgress() = default;

        /*!
         * @brief Start downloading a song
         * @param songMD5 Song md5 (cache file name)
         */
        virtual void StartDownloadingSong(const String& songMD5) = 0;

        /*!
         * @brief Report the progress of downloading the requested song
         * @param songMD5 Song md5 (cache file name)
         * @param currentSize Currently downloaded bytes
         * @param totalSize Song size in byte
         */
        virtual void DownloadingSong(const String& songMD5, long long int currentSize, long long int totalSize) = 0;

        /*!
         * @brief
         * @brief A song is downloaded
         * @param songMD5 Song md5 (cache file name)
         * @param status True if the song is fully downloaded, false if any error occurred
         */
        virtual void EndDownloadingSong(const String& songMD5, bool status) = 0;
    };

    //! Little structure to hold track specifications
    class TrackInfo
    {
      public:
        /*
         * Tools
         */

        TrackInfo() : mDuration(0), mSize(0) {}

        [[nodiscard]] bool HasValidFile() const { return mPath.Size() == mSize; }

        [[nodiscard]] bool Compare(const TrackInfo& b) const
        {
          if (mMixTape != b.MixTape()) return mMixTape < b.mMixTape;
          return mName < b.mName;
        }

        /*
         * Accesors 
         */

        //! Get name
        [[nodiscard]] const String& Name() const { return mName; }
        //! Get duration
        [[nodiscard]] int Duration() const { return mDuration; }
        //! Get file size
        [[nodiscard]] int Size() const { return mSize; }
        //! Get url
        [[nodiscard]] const String& Url() const { return mUrl; }
        //! Get album media url
        [[nodiscard]] const String& AlbumPictureUrl() const { return mAlbumPictureUrl; }
        //! Get mixtape media url
        [[nodiscard]] const String& MixtapePictureUrl() const { return mMixtapePictureUrl; }
        //! Get album media url
        [[nodiscard]] const String& AlbumPictureUrlOrNoImage() const { return mAlbumPictureUrl.empty() ? sNoImage : mAlbumPictureUrl; }
        //! Get mixtape media url
        [[nodiscard]] const String& MixtapePictureUrlOrNoImage() const { return mMixtapePictureUrl.empty() ? sNoImage : mMixtapePictureUrl; }
        //! Get md5
        [[nodiscard]] const String& Md5() const { return mMd5; }
        //! Get album
        [[nodiscard]] const String& Album() const { return mAlbum; }
        //! Get mix tape
        [[nodiscard]] const String& MixTape() const { return mMixTape; }
        //! Get local path
        [[nodiscard]] const Path& LocalPath() const { return mPath; }
        //! Get artist
        [[nodiscard]] const String& Artist() const { return mArtist; }
        //! Get enabled state
        [[nodiscard]] bool IsEnabled() const { return mEnabled; }
        //! Set enabled state
        void SetEnabled(bool enabled) { mEnabled = enabled; }


        /*!
         * @brief Constructor
         * @param name Track name
         * @param duration Track duration
         * @param size File size
         * @param url Track url
         * @param albumPictureUrl Album media url
         * @param mixTapePictureUrl Mixtape media url
         * @param md5 File md5
         * @param album Album name
         * @param mixTape Mix tape name
         */
        TrackInfo(const String& name,
                  int duration,
                  int size,
                  const String& url,
                  const String& albumPictureUrl,
                  const String& mixTapePictureUrl,
                  const String& md5,
                  const String& album,
                  const String& mixTape,
                  const String& artist)
          : mName(name)
          , mDuration(duration)
          , mSize(size)
          , mUrl(url)
          , mAlbumPictureUrl(albumPictureUrl)
          , mMixtapePictureUrl(mixTapePictureUrl)
          , mMd5(md5)
          , mAlbum(album)
          , mMixTape(mixTape)
          , mPath(Path(sStorageRootPath) / md5)
          , mArtist(artist)
        {
        }

      private:
        //! No image pic
        static const String sNoImage;

        String mName;              //!< Track name
        int    mDuration;          //!< Duration in second
        int    mSize;              //!< File size
        String mUrl;               //!< Track url
        String mAlbumPictureUrl;   //!< Album Media url
        String mMixtapePictureUrl; //!< Mixtape Media url
        String mMd5;               //!< File md5
        String mAlbum;             //!< File md5
        String mMixTape;           //!< File md5
        Path   mPath;              //! Local file path
        String mArtist;            //! Artist
        bool   mEnabled = true;    //! Enabled state (not deserialized, filled from disabled list)
    };

    //! Constructor
    explicit RemotePlaylist(IRemotePlaylistNotification* callback);

    //! Destructor
    ~RemotePlaylist() override;

    /*!
     * @brief Look for the first enabled and fully downloaded track at or after the reading head,
     * scanning the whole playlist. The head is moved onto it, but not past it: call TrackAccepted()
     * once the track is actually started.
     * @return Next track structure or nullptr if nothing is cached at all
     */
    TrackInfo* GetNextTrack();

    /*!
     * @brief Confirm the track returned by GetNextTrack() is now playing.
     * Moves the reading head forward and wakes the download loop to pre-cache ahead.
     * Must be called on the main thread immediately after playback starts.
     */
    void TrackAccepted();

    /*!
     * @brief Last track returned by GetNextTrack() has been played. Remove it
     */
    void TrackConsumed();

    /*!
     * @brief Get the whole playlist
     * @return Pleylist
     */
    std::vector<TrackInfo> Playlist() { Mutex::AutoLock locker(mLocker); return mPlaylist; }

    //! Check if track list has been loaded
    bool HasTracks() { Mutex::AutoLock locker(mLocker); return !mPlaylist.empty(); }

    /*!
     * @brief Request a new song to be downloaded
     * @param songMD5
     */
    void RequestSong(const String& songMD5);

    /*!
     * @brief Set a new download interface so that download progress can be reported
     * @param interface New interface
     */
    void SetDownloadInterface(ISongProgress* interface) { Mutex::AutoLock locker(mLocker); mDownloadInterface = interface; }

    /*!
     * @brief Set a track enabled or disabled and persist the change
     * @param md5 Track MD5
     * @param enabled True to enable, false to disable
     */
    void SetTrackEnabled(const String& md5, bool enabled);

    /*!
     * @brief Check if a track is enabled (not in the disabled list)
     * @param md5 Track MD5
     * @return True if the track is enabled
     */
    bool IsTrackEnabled(const String& md5) const { return !mDisabledTracks.contains(md5); }

    /*!
     * @brief Switch between random and linear (sorted by mixtape/name) mode
     * @param randomOn True for random mode, false for linear mode
     */
    void Randomize(bool randomOn);

    //! Check if playlist is in random mode
    [[nodiscard]] bool IsRandomMode() const { return mRandomMode; }

  private:
    //! Url where to get playlist
    static constexpr const char* sRemotePlayListUrl = "https://api-patreon.recalbox.com/playlist";
    //! Storage root path - no file except music must be put there!
    static constexpr const char* sStorageRootPath = "/overlay/.music/.cache";
    //! Storage root path - no file except music must be put there!
    static constexpr const char* sStorageTimeStamp = "/overlay/.music/.cache/.timstamp";
    //! Last music Index path
    static constexpr const char* sStorageIndex = "/overlay/.music/.index";
    //! Playlist cache path
    static constexpr const char* sStorageList = "/overlay/.music/.list";
    //! Disabled tracks list path
    static constexpr const char* sDisabledListPath = "/overlay/.music/.disabled";
    //! Maximum songs to pre-cache ahead of the current play position
    static constexpr int sLookAheadCount = 10;
    //! Delay before retrying a playlist download that failed, in milliseconds
    static constexpr long long int sPlaylistRetryDelay = 30000;

    //! Download status
    enum class DownloadState
    {
      Start,    //!< Start downloading
      End,      //!< Stop downloading
    };

    //! Internal notifier when playlist is ready (at least one file downloaded)
    SyncMessageSender<void> mSender;
    //! Http progress synced messages
    SyncMessageSender<RemoteTrackDownloadProgress> mProgressSender;

    //! Http request
    HttpMd5 mRequest;

    //! Action signal
    Signal mAction;

    //! Playlist storage
    std::vector<TrackInfo> mPlaylist;
    //! Storage root
    Path mStorageRoot;
    //! Requested song
    String mSongRequest;
    //! Download interface
    ISongProgress* mDownloadInterface;
    //! Index locker (mutable so const methods can lock)
    mutable Mutex mLocker;
    //! Disabled tracks (MD5 set)
    HashSet<String> mDisabledTracks;
    //! Reading head of the playlist
    PlaylistCursor mCursor;
    //! Random mode (true = server order, false = sorted by mixtape/name)
    bool mRandomMode;

    //! Playlist state callback interface
    IRemotePlaylistNotification* mCallback;

    //! Load disabled track list from file
    void LoadDisabledList();

    //! Save disabled track list to file
    void SaveDisabledList() const;

    //! Get requested song (protected access)
    String GetRequestedSong() { Mutex::AutoLock lock(mLocker); return mSongRequest; }

    //! Clear requested song (protected access)
    void ClearRequestedSong() { Mutex::AutoLock lock(mLocker); mSongRequest.clear(); }

    //! Check if a song has been requested (protected access)
    bool IsSongRequested() { Mutex::AutoLock lock(mLocker); return !mSongRequest.empty(); }

    /*!
     * @brief Get cache size limit
     * @return Cache size limit
     */
    static long long int GetCacheLimit();

    /*!
     * @brief Update playlist
     * @result True if the playlist was successfuly fetched, false otherwise
     */
    bool UpdatePlayList();

    /*!
     * @brief Deserialize playlist into TrackInfo structures, replacing the current one
     * @param jsonString JSON Playlist
     * @return True on success, false otherwise
     */
    bool DeserializePlaylist(const String& jsonString);

    /*!
     * @brief Check files on disk, delete older/bad files and load current index
     */
    void InitializeIndex();

    /*!
     * @brief Update file index using current track index
     */
    void UpdateIndex() const;

    /*!
     * @brief Lookup a track by md5 — caller must hold mLocker
     * @param md5 md5 to lookup
     * @return Index in mPlaylist or -1
     */
    [[nodiscard]] int LookupTrackIndexLocked(const String& md5) const;

    /*!
     * @brief Lookup a track by md5 — acquires mLocker internally
     * @param md5 md5 to lookup
     * @return Index in mPlaylist or -1
     */
    [[nodiscard]] int LookupTrackIndex(const String& md5) const;

    /*!
     * @brief Check old files and delete them all
     * @return total file size
     */
    long long int SanitizeFiles();

    /*!
     * @brief Delete the least recently used cached files until the cache fits in the given budget.
     * The oldest file is elected among the cached files themselves, never by comparing them to a
     * reading of the clock: file times and wall clock do not share the same epoch.
     * @param budget Size in bytes the cache must fit in
     * @param totalSize Current total cache size (updated in place)
     * @return True if the cache fits in the budget, false if it cannot
     */
    static bool EvictDownTo(long long int budget, long long int& totalSize);

    /*!
     * @brief Download and cache files
     * @param totalSize Total file size already downloaded
     */
    void DownloadFiles(long long int totalSize);

    /*!
     * @brief Report download status
     * @param track Track being downloaded
     * @param state Download state
     * @param finalStatus Report error/success (only valid on 'end' state)
     */
    void ReportDownload(const TrackInfo& track, DownloadState state, bool finalStatus);

    /*
     * Thread implementation
     */

    //! Run loop
    void Run() override;

    /*
     * ISyncMessageReceiver<void> implementation
     */

    void ReceiveSyncMessage() final;

    /*
     * ISyncMessageReceiver<Progress> implementation
     */

    void ReceiveSyncMessage(const RemoteTrackDownloadProgress& message) final;

    /*!
     * HttpClient::IDownloadInterface
     */

    /*!
     * @brief Notify of download progress
     * @param http HTTP request
     * @param currentSize downloaded bytes
     * @param expectedSize total expected bytes
     */
    void DownloadProgress(const HttpClient& http, long long currentSize, long long expectedSize) final;
};
