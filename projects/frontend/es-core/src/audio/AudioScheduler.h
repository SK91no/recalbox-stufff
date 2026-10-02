#pragma once

#include <guis/GuiInfoPopupBase.h>
#include <random>

#include "IMusicCalback.h"
#include "Sound.h"
#include "Music.h"

#include "utils/sync/SyncMessageSender.h"

class AudioScheduler : private ISyncMessageReceiver<void>
                     , public StaticLifeCycleControler<AudioScheduler>
                     , private RecalboxConf::IAudioModeNotification
{
  public:
    typedef unsigned long long AudioHandle;

    //! Music playing is permitted?
    [[nodiscard]] bool CanPlayMusic() const;

    //! Video sound is permitted?
    [[nodiscard]] bool CanDecodeVideoSound() const;

    /*!
     * @brief Set the authorization to create new music popups
     * @param allowed True to allow popup to show
     */
    void SetPopupAllowed(bool allowed) { mPopupAllowed = allowed; }

  private:
    //! Music source folder
    static constexpr const char* sMusicFolder = "/recalbox/share/music/";

    //! Music source
    enum class MusicSource
    {
        None,        //!< Not selected yet
        ThemeSystem, //!< Theme music for the current system
        Theme,       //!< Theme musics
        User,        //!< User musics
        RemoteTrack, //!< Remote track
    };

    //! Max tries to find a different music to play
    static constexpr int sMaxTries = 8;

    //! Audio mode
    AudioMode mCurrentAudioMode;

    //! Callback Interface
    IMusicCallback* mJukeboxInterface;

    //! Path to Sound (sounds are small, hot and few: caching them is worthwhile)
    HashMap<AudioHandle, Sound*> mSoundMap;

    //! Window to attach popups to
    WindowManager& mWindow;

    //! Currently playing music - owned: loaded on demand, freed when replaced or ended (no cache)
    Music* mMusicPlaying;

    //! Last popup text
    String mCurrentMusicTitle;
    //! Current music source
    MusicSource mCurrentMusicSource;

    //! Reserved SDL Event
    SyncMessageSender<void> mSender;

    //! Random device to seed random generator
    std::random_device mRandomDevice;
    //! Random generator
    std::mt19937 mRandomGenerator;
    //! Random repartition (system)
    std::uniform_int_distribution<int> mSystemRandomizer;

    //! Theme music folder
    Path mThemeMusicFolder;
    //! Theme music
    Path mThemeMusic;
    //! Theme
    ThemeData* mTheme;

    //! Popup allowed ?
    bool mPopupAllowed;

    /*!
     * @brief Synchronous event receiver
     */
    void ReceiveSyncMessage() final;

    /*!
     * @brief Initialize SDL audio
     */
    void Initialize();

    /*!
     * @brief Finalize SDL audio and free sound & music objects
     */
    void Finalize();

    /*!
     * @brief List music file from the given folder
     * @param path Path to folder
     * @return Music list, may be empty
     */
    static std::vector<Path> ListMusicInFolder(const Path& path);

    /*!
     * @brief Fetch a random music from the given folder
     * @param from source folder
     * @return Path to music
     */
    Path FetchRandomMusic(const Path& from, const Path& previousPath);

    /*
     * Recalbox::IAudioModeNotification implementation
     */

    void ConfigurationAudioModeChanged(const AudioMode& audioMode) final;

  public:
    /*!
     * @brief Constructor - act as a singleton (Multiple instance is not possible)
     * That is, the application can control the lifecycle of this object
     */
    explicit AudioScheduler(WindowManager& window);

    /*!
     * @brief Destructor
     */
    ~AudioScheduler() override;

    /*!
     * @brief Set jukebox interface
     * @param interface Interface
     */
    void SetJukeboxInterface(IMusicCallback* interface) { mJukeboxInterface = interface; }

    /*!
     * @brief Send a signal to the AudioManager from another thread to instruct
     * current music has ended
     */
    void SignalEndOfMusic() { mSender.Send(); }

    /*!
     * @brief Load sound effect from the givenpath
     * @param path Sound file path
     * @return Non zero handle if the file has been successfully loaded
     */
    AudioHandle LoadSound(const Path& path);

    /*!
     * @brief Play sound
     * @param handle Sound handle to play
     * @return True if the handle is valid and the sound is playing
     */
    bool PlaySound(AudioHandle handle);

    /*!
     * @brief Load and play a music file directly. The previously playing music (if any) is freed.
     * Music is never cached: only one track plays at a time, so it is loaded on demand and released
     * as soon as it is replaced or finishes.
     * @param path Music file path
     * @param loop True to play the music continuously
     * @return True if the file has been loaded and is playing
     */
    bool PlayMusicFile(const Path& path, bool loop);

    /*!
     * @brief Load and play a remote track, properly updating internal state
     * so that end-of-music handling (TrackConsumed, etc.) works correctly
     * @param path Local path of the cached track file
     * @return True if the music is playing
     */
    bool PlayRemoteTrack(const Path& path);

    //! Pause music
    static void PauseMusic();

    //! Resume music
    static void ResumeMusic();

    /*!
     * @brief Stop all sounds and musics
     */
    void StopAll();

    /*!
     * @brief Set new theme to fetch music from
     * @param theme Theme to fetch music from
     * @return This
     */
    AudioScheduler& SetTheme(ThemeData& theme) { mTheme = &theme; return *this; }

    /*!
     * @brief Play a new random music from theme/user music folder
     */
    void PlayNextMusic();

    /*!
     * @brief Clear cached musics/sounds
     */
    void ClearCaches();

    /*!
     * @brief Deactivate the sound system to allow external application to run
     */
    void Deactivate()
    {
      Finalize();
    }

    /*!
     * @brief Check if a music is currently playing
     * @return True if a music is currently playing
     */
    bool IsPlaying();

    /*!
     * @brief Get current music total time in second
     * @return Time in seconds
     */
    [[nodiscard]] float PlayingMusicTotalTime() const;

    /*!
     * @brief Get current music position
     * @return Position in second
     */
    [[nodiscard]] float PlayingMusicPosition() const;

    /*!
     * @brief Seek the current music to the given position
     * @param position Position in seconds (clamped to [0, duration])
     */
    void SeekMusic(float position);

    /*!
     * @brief Get current music filename
     * @return Music file name or empty string
     */
    [[nodiscard]] const String PlayingMusicName() const;

    /*!
     * @brief Get current music filepath
     * @return Music file name or empty string
     */
    [[nodiscard]] const Path& PlayingMusicPath() const;

    /*!
     * @brief Reactivate the sound system if it has been previously deactivated.
     */
    void Reactivate();

    //! Pause music to avoid sound conflict w/ video
    static void PauseMusicIfNecessary();

    //! Resume music that have been paused to avoid sound conflict w/ video
    static void ResumeMusicIfNecessary();
};
