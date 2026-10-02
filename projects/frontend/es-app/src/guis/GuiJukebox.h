//
// Created by bkg2k on 31/12/2019.
//
#pragma once

#include <guis/Gui.h>
#include <bios/Bios.h>
#include <bios/BiosList.h>
#include <bios/IBiosScanReporting.h>
#include <components/TextListComponent.h>
#include <components/BusyComponent.h>
#include <components/ButtonComponent.h>
#include <components/ScrollableContainer.h>
#include <systems/SystemManager.h>

#include "components/MarkdownText.h"
#include "components/TextScrollComponent.h"
#include "components/PictureComponent.h"
#include "components/ProgressBarComponent.h"
#include "components/SliderComponent.h"
#include "music/RemotePlaylist.h"
#include "utils/markdown/MarkdownTools.h"
#include "utils/sync/SyncMessageSender.h"


class GuiJukebox : public Gui
                   , ITextListComponentOverlay<const RemotePlaylist::TrackInfo*>
                   , ITextListComponentInterface<const RemotePlaylist::TrackInfo*>
                   , IHttpConfiguration
                   , RemotePlaylist::ISongProgress
                   , private ISyncMessageReceiver<bool>
                   , private ISyncMessageReceiver<void>
{
  private:
    //! Text color index
    static constexpr signed char sColorIndexNormal = 0;
    //! Half Text color index
    static constexpr signed char sColorIndexHalf = 1;
    //! Transparent Red
    static constexpr signed char sColorIndexRed = 2;
    //! Transparent Yellow
    static constexpr signed char sColorIndexYellow = 3;
    //! Transparent Green
    static constexpr signed char sColorIndexGreen = 4;

    //! Thumbnails
    static constexpr int sPictureGridCount = 3;

    //! Play indicator icon
    Texture mPlayIcon;

    //! Audio scheduler
    AudioScheduler& mAudioScheduler;

    //! Menu data
    const MenuThemeData& mTheme;
    //! Icon cache
    MenuThemeDataCache& mCache;
    //! Ubuntu condensed
    NewFont* mBoldCondensed;

    //! Global component grid for easy positioning
    ComponentGrid mGrid;

    //! Playlist
    std::vector<RemotePlaylist::TrackInfo> mPlayList;
    //! Playlist headers
    std::vector<RemotePlaylist::TrackInfo*> mHeaders;
    //! Unique media
    HashSet<String> mUniqueMedia;

    //! Text title
    std::shared_ptr<TextComponent> mTitle;
    //! Play list
    std::shared_ptr<TextListComponent<const RemotePlaylist::TrackInfo*>> mList;
    //! Details markdown
    std::shared_ptr<MarkdownText> mMarkdown;
    //! Big image
    std::shared_ptr<PictureComponent> mAlbumPicture;
    //! 9 thumbnails
    std::shared_ptr<PictureComponent> mAlbumThumbnails[sPictureGridCount];
    //! Big mixtape image
    std::shared_ptr<PictureComponent> mMixTapePicture;
    //! Mixtape thumbnail
    std::shared_ptr<PictureComponent> mMixTapeThumbnail;

    //! Now playing action text
    std::shared_ptr<TextScrollComponent> mAction;
    //! Download progress bar
    std::shared_ptr<ProgressBarComponent> mProgress;
    //! Music position slider
    std::shared_ptr<SliderComponent> mPositionSlider;

    //! Detail: Image
    std::shared_ptr<PictureComponent> mDetailStatusImage;

    //! Unselected checkbox resource
    Texture mUnselected;
    //! Selected checkbox resource
    Texture mSelected;

    //! Markdown tool
    MarkdownTools mTools;

    //! Checkbox width
    int mCheckBoxWidth;
    //! Checkbox height
    int mCheckBoxHeight;

    //! Busy animation while scanning
    BusyComponent mBusyAnim;

    //! Seek repeat rate (seconds per tick when holding left/right)
    float mSeekRate;
    //! Seek repeat accumulator
    int mSeekAccumulator;
    //! Flash counter for "music not found" feedback
    int mFlashFrames;
    //! Auto-follow mode: cursor follows playing music
    bool mAutoFollow;

    //! MD5 of the song being downloaded
    String mDownloadingMd5;
    //! Local path of the song being downloaded (captured at request time)
    Path mDownloadingPath;
    //! Name of the song being downloaded
    String mDownloadingName;
    //! True when in download mode
    bool mDownloading;
    //! Sync sender: background thread signals download start to main thread
    SyncMessageSender<void> mStartSender;
    //! Sync sender: background thread posts download result (true=success) to main thread
    SyncMessageSender<bool> mDownloadSender;

    //! Switch UI to download mode
    void SwitchToDownloadMode(const String& songName);

    //! Switch UI back to playing mode
    void SwitchToPlayingMode();

    //! Called on main thread when a download starts (via SyncMessageSender)
    void ReceiveSyncMessage() final;
    //! Called on main thread when a download completes (via SyncMessageSender)
    void ReceiveSyncMessage(bool success) final;

    /*!
     * @brief Called when sizing window
     */
    void onSizeChanged() override;

    /*!
     * @brief Override rendering to draw custom background
     * @param parentTrans parent transformation
     */
    void Render(const Transform4x4f& parentTrans) override;

    /*!
     * @brief Update
     * @param deltaTime Elapsed time since last update
     */
    void Update(int deltaTime) override;

    /*!
     * @brief Process input events
     * @param event Input event
     * @return True if the method consumed the event
     */
    bool ProcessInput(const InputCompactEvent& event) override;

    /*!
     * @brief Set help prompts
     * @param help Help structure to fill in
     * @return True
     */
    bool CollectHelpItems(Help& help) override;

    //! Fill the song list
    void FillPlayList();

    /*!
     * @brief Display track details. If track us a header (mixtape), display header details instead
     * @param track Track to display
     */
    void FillSongDetails(const RemotePlaylist::TrackInfo& track);

    /*!
     * @brief Format duration in minute'seconds"
     * @param duration Duration in seconds
     * @return Formated duration: min'sec"
     */
    static String FormatDuration(int duration);

  public:
    /*!
     * @brief Constructor
     * @param window Main window
     * @param audioScheduler Audio Scheduler
     */
    explicit GuiJukebox(WindowManager& window, AudioScheduler& audioScheduler);

    //! Destructor
    ~GuiJukebox() override;

    /*!
     * @brief Try to follow the current playing track in the track list.
     * @param flashIfNotFound If true and the track is not found, make the track list flashing shortly
     */
    void Follow(bool flashIfNotFound);

    /*
     * ITextListComponentOverlay<const RemotePlaylist::TrackInfo*> implementation
     */

    /*!
     * @brief Apply (draw) an overlay in the given item rectangle and adjust rectangle position/size
     * so that the text won't draw over the overlay if required
     * @param parentTrans Parent transform matrice
     * @param leftWidth Left overlay width
     * @param rightWidth Right overlay width
     * @param labelWidth Cell's label width in pixel
     * @param area item rectangle
     * @param data Linked data
     * @param color draw color
     */
    void OverlayApply(const Transform4x4f& parentTrans, int leftWidth, int rightWidth, int labelWidth, const Rectangle& area, const RemotePlaylist::TrackInfo* const& data, int index, unsigned int& color) final;

    /*!
     * @brief Get the left offset (margin to the text) if any
     * @param data Linked data
     * @param labelWidth Cell's label width in pixel
     * @return left offset
     */
    float OverlayGetLeftOffset(const RemotePlaylist::TrackInfo* const& data, int labelWidth) final;

    /*!
     * @brief Get the right offset (margin from text to right limit) if any
     * @param data Linked data
     * @param labelWidth Cell's label width in pixel
     * @return right offset
     */
    float OverlayGetRightOffset(const RemotePlaylist::TrackInfo* const& data, int labelWidth) final { (void)data; (void)labelWidth; return 0; };

    /*
     * ITextListComponentInterface<const RemotePlaylist::TrackInfo*> implementation
     */

    /*!
     * @brief Called when the cursor move to a new position
     * @param index New position index
     * @param data New position user data
     */
    void OnCursorChanged(int index, const RemotePlaylist::TrackInfo* data) final { (void)index; FillSongDetails(*data); }

    /*
     * IHttpConfiguration
     */

    /*!
     * @brief Configure the given http client
     * @param url Url
     * @param client Client to configure
     */
    void ConfigureHttpClient(const String& url, HttpClient& client) final;

    /*
     * RemotePlaylist::ISongProgress implementation
     */

    void StartDownloadingSong(const String& songMD5) final;
    void DownloadingSong(const String& songMD5, long long int currentSize, long long int totalSize) final;
    void EndDownloadingSong(const String& songMD5, bool status) final;
};
