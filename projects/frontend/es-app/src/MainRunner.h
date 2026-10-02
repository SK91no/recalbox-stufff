//
// Created by bkg2k on 13/11/2019.
//
#pragma once

#include <ApplicationWindow.h>
#include <utils/cplusplus/INoCopy.h>
#include <utils/os/fs/watching/IFileSystemWatcherNotification.h>
#include <usernotifications/NotificationManager.h>
#include <guis/GuiWaitLongExecution.h>
#include <emulators/run/GameRunner.h>
#include <patreon/IPatreonNotification.h>
#include "bluetooth/BluetoothListener.h"
#include "recalbox/BootConf.h"
#include "input/IKeyboardShortcut.h"
#include "Options.h"
#include "hardware/devices/DeviceInitializationParameters.h"
#include "music/IRemotePlaylistNotification.h"
#include <btautopair/BTAutopairManager.h>
#include <bios/IBiosScanReporting.h>

#include "hardware/cardreader/CardReader.h"

class AudioScheduler;
class SystemManager;
class BiosManager;

//! Special operations
enum class HardwareTriggeredSpecialOperations
{
  Suspend,  //!< The hardware require a Suspend operation
  Resume,   //!< The hardware just woken up
  PowerOff, //!< The hardware require a Power-off
  Reset,    //!< The hardware require a reset
};

class MainRunner
  : private INoCopy
  , private IFileSystemWatcherNotification
  , public IHardwareNotifications
  , private ILongExecution<HardwareTriggeredSpecialOperations, bool>
  , private IRomFolderChangeNotification
  , private IPatreonNotification
  , private IRemotePlaylistNotification
  , public ISdl2EventNotifier
  , public ISpecialGlobalAction
  , public IBiosScanReporting
  , public IKeyboardShortcut
  , public IGlobalVariableResolver
  , public ISystemAlteredInterface
  , public CardReaderListener
{
  public:
    //! Pending Exit
    enum class PendingExit
    {
       None,            //!< No Pending exit
       GamelistChanged, //!< At least one gamelist has changed. ES must reload
       ThemeChanged,    //!< Current theme has been modified. ES must reload
       MustExit,        //!< External quit required
       WaitingExit,     //!< Special wait state after a pending exit state has been processed
    };

    //! Runner exit state
    enum class ExitState
    {
      Quit,             //!< Normal quit (usually requested by external software)
      FatalError,       //!< Initialization error or runtime fatal error
      Relaunch,         //!< Relaunch requested!
      RelaunchNoUpdate, //!< Relaunch requested! No gamelist update
      NormalReboot,     //!< Normal reboot machine requested, save everything
      FastReboot,       //!< Fast reboot machine, save nothing
      Shutdown,         //!< Relaunch machine
      FastShutdown,     //!< Relaunch machine, save nothing
    };

    //! Temporary file used as flag of readyness
    static constexpr const char* sReadyFile = "/tmp/externalnotifications/emulationstation.ready";
    //! Temporary file used as quit request
    static constexpr const char* sQuitNow = "/tmp/externalnotifications/emulationstation.quitnow";
    //! Temporary file used to stop game demo loop
    static constexpr const char* sStopDemo = "/tmp/externalnotifications/emulationstation.stopdemo";
    //! Upgrade file flag. Only available once in /tmp after a successful update
    static constexpr const char* sUpgradeFileFlag = "/overlay/.upgrade_success";
    static constexpr const char* sUpgradeFailedFlag = "/overlay/.upgrade_failed";
    static constexpr const char* sUpgradeCorruptedFlag = "/overlay/.upgrade_corrupted";
    //! Boot message file
    static constexpr const char* sBootNotification = "/overlay/.frontend_boot_notif";

  private:
    //! Power button: Threshold from short to long press, in milisecond
    static constexpr const int sPowerButtonThreshold = 500;

    //! Options
    const Options& mOptions;
    //! Requested width

    //! Pending exit
    PendingExit mPendingExit;

    //! Run count
    int mRunCount;
    String mCardReaderGameToRun;

    //! Quit request state
    static ExitState sRequestedExitState;
    //! Quit request
    static bool sQuitRequested;

    //! Inter-thread messaging system
    SyncMessageFactory mSyncMessageFactory;

    //! Recalbox configuration
    RecalboxConf mConfiguration;
    //! Crt configuration
    CrtConf mCrtConfiguration;
    //! Crt configuration
    BootConf mBootConf;

    //! Nofitication manager
    NotificationManager mNotificationManager;

    //! Window reference
    ApplicationWindow* mApplicationWindow;

    //! Bluetooth listener
    BluetoothListener mBluetooth;

    //! Known added devices
    HashSet<String> mAddedDevices;
    //! Known empty added devices
    HashSet<String> mAddedEmptyDevices;
    //! Known removed devices
    HashSet<String> mRemovedDevices;

    //! Bluetooth Autopair Manager
    BTAutopairManager mBTAutopairManager;

    //! Executable folder
    Path mExecutableFolder;

    /*!
     * @brief Process SDL2 event
     * @param window Window manager
     * @param event SDL2 event to process
     */
    void ProcessSDLEvent(ApplicationWindow& window, SDL_Event& event);

    /*!
     * @brief Reset last exit state
     */
    static void ResetExitState() { sQuitRequested = false; }

    /*!
     * @brief Display Intro
     */
    void Intro();

    /*!
     * @brief Check home folder existence
     */
    static void CheckHomeFolder();

    /*!
     * @brief Play loading jingle (loading.ogg) from theme if available
     */
    static void PlayLoadingSound(AudioScheduler& audioManager);

    /*!
     * @brief Try loading system configuration.
     * @param systemManager System manager instance
     * @param forceReloadFromDisk force reloading game list from disk
     * @return True if there is at least one system loaded
     */
    static bool TryToLoadConfiguredSystems(SystemManager& systemManager);

    /*!
     * @brief Check if Recalbox has been updated and push a display changelog popup
     * @param window Main window
     */
    static void CheckUpdateMessage(WindowManager& window);

    /*!
     * @brief Check if Recalbox upgrade has failed push an error popup
     * @param window Main window
    */
    static void CheckUpdateFailed(WindowManager& window);

    /*!
     * @brief Check if Recalbox upgrade is corrupted push an error popup
     * @param window Main window
    */
    static void CheckUpdateCorrupted(WindowManager& window);

    /*!
     * @brief Check if this is the first launch and run a wizard if required
     * @param window Main window
     */
    static void CheckFirstTimeWizard(WindowManager& window);

    /*!
     * @brief Check if there are messages to display in info popups
     * The message should be raw : "The bootloader has been updated"
     * or with the Warning icon specifier and a pipe: "Warning|An error occured"
     * @param window Main window
     */
    static void CheckBootNotification(WindowManager& window);

    /*!
     * @brief Initialize input configurations
     * @param window Main window
     */
    static void InitializeUserInterface(WindowManager& window);

    /*!
     * @brief Check if something must retain user's attention
     * @param window Main window
     * @param systemManager System Manager
     */
    static void CheckAlert(WindowManager& window, SystemManager& systemManager);

    /*!
     * @brief Send mqtt message to enable joystick auopairing
     */
    //static void EnableAutopair();

    /*!
     * @brief Main SDL event loop w/ UI update/refresh
     * @param window Main window
     * @param systemManager System Manager
     * @param syncMessageFactory Syn'ed message factory
     * @param gameRunner
     * @return Exit state
     */
    ExitState MainLoop(ApplicationWindow& window, SystemManager& systemManager, SyncMessageFactory& syncMessageFactory, GameRunner& gameRunner);

    /*!
     * @brief Process general special inputs
     * @param event Input event
     * @return bool if the input has been consummed, false otherwise
     */
    bool ProcessSpecialInputs(const InputCompactEvent& event);

    /*!
     * @brief Create ready flag file to notify all external software that
     * Emulationstation is ready
     */
    static void CreateReadyFlagFile();
    /*!
     * @brief Cleanup ready flag file
     */
    static void DeleteReadyFlagFile();

    /*!
     * @brief Tell if we have to save the gamelist, regarding the exitstate
     * @param state exit state
     * @return True if we have to update the gamelists before exiting
     */
    static bool DoWeHaveToUpdateGamelist(ExitState state);

    /*
     * IFileSystemWatcherNotification implementation
     */

    void FileSystemWatcherNotification(FileSystemWatcher& source, const Path& path, EventType event, const DateTime& time) final;

    /*
     * IHardwareEvents implementation
     */

    bool IsApplicationRunning() final { return !GameRunner::IsGameRunning(); }

    /*!
     * @brief Headphone has been pluggen in
     * @param board current board
     */
    void HeadphonePluggedIn(BoardType board) final;

    /*!
     * @brief Headphone has been unplugged
     * @param board current board
     */
    void HeadphoneUnplugged(BoardType board) final;

    /*!
     * @brief Decrease volume
     * @param board current board
     */
    void VolumeDecrease(BoardType board, float percent) final;

    /*!
     * @brief Increase volume
     * @param board current board
     */
    void VolumeIncrease(BoardType board, float percent) final;

    /*!
     * @brief Suspend will occur in a short delay
     * @param board current board
     * @param delayInMs delay in ms before suspend will occurs
     */
    void PowerButtonPressed(BoardType board, int delayInMs) final;



    void ResetButtonPressed(BoardType board) final;
    /*!
     * @brief We have been resumed from suspend mode
     * @param board current board
     */
    void Resume(BoardType board) final;

    /*!
     * @brief Decrease brightness
     * @param board current board
     */
    void BrightnessDecrease(BoardType board, float percent) final;

    /*!
     * @brief Increase brightness
     * @param board current board
     */
    void BrightnessIncrease(BoardType board, float percent) final;

    /*!
     * @brief Undervoltage alert
     * @param board current board
     */
    void UnderVoltage(BoardType board) final;

    /*!
     * @brief HighTemperature detected
     * @param board current board
     */
    void TemperatureAlert(BoardType board) final;

    /*
     * ILongExecution implementation
     */

    /*!
     * @brief Dummy execution of sleeps, allowing UI to draw special operation wait-windows
     * @param from Source window
     * @param parameter Operation
     * @return Not used
     */
    bool Execute(GuiWaitLongExecution<HardwareTriggeredSpecialOperations, bool>& from,
                 const HardwareTriggeredSpecialOperations& parameter) override;

    /*!
     * @brief Called when special wait-window close so that we can execute the required operation
     * @param parameter Operation required
     * @param result Not used
     */
    void Completed(const HardwareTriggeredSpecialOperations& parameter, const bool& result) override;

    /*
     * IPatreonNotification implementation
     */

    /*!
     * @brief Notify of the current user status
     * @param result Authentication result (user status)
     * @param level Patron level
     * @param patreonName Patreon name
     */
    void PatreonState(PatronAuthenticationResult result, int level, const String& patreonName) final;

    /*!
     * @brief Notify of the remote playlist status
     */
    void PlaylistReady() override;

    /*
     * IBiosScanReporting implementation
     */

    /*!
     * @brief Report a new Bios has been scanned
     * @param bios Newly scanned bios
     */
    void ScanProgress(const Bios& bios) override { (void)bios; }

    /*!
     * @brief Report the bios scan is complete
     */
    void ScanComplete() override;

    /*
     * ISystemAlterted implementation
     */

    /*!
     * @brief Instruct the callee that a system gamelist has been altered
     * @param system First system whose gamelist has been altered
     */
    void SystemGamelistAltered(SystemData* system) final;

    /*!
     * @brief Instruct the callee that a system rom folder has been altered
     * @param system First system whose roms have been altered
     */
    void SystemRomFolderAltered(SystemData* system) final;

  public:
    /*!
     * @brief Constructor
     * @param executablePath current executable path
     * @param runCount Number of time the MainRunner has been run
     * @param environment Application environment
     * @param options Command line options
     */
    MainRunner(const String& executablePath, int runCount, char** environment, const Options& options);

    //! Destructor
    ~MainRunner() override;

    /*!
     * @brief Run the game!
     */
    ExitState Run();

    /*!
     * @brief Request the application to quit using a particular exitstate
     * @param requestedState Requested Exit State
     * @param forceReloadFromDisk Force reload gamelist from disk
     */
    static void RequestQuit(ExitState requestedState);

    /*!
     * @brief Set the system locale
     * @param executablePath Path to current executable
     */
    static void SetLocale(const String& executablePath);

    /*!
     * @brief Set debug log state
     * @param debug True to set debug logs on
     * @param trace True to set trace logs on. Trace takes precedence over debug
     */
    static void SetDebugLogs(bool debug, bool trace);

    /*
     * RomFolderChangeNotification implementaton
     */

    /*!
     * @brief Notify a new rompath has been added
     * @param root rompath added
     */
    void RomPathAdded(const DeviceMount& root) override;

    /*!
     * @brief Notify an existing reompath has been removed
     * @param root rompath removed
     */
    void RomPathRemoved(const DeviceMount& root) override;

    /*!
     * @brief Notify a device has been added with no valid rom path found
     * @param deviceRoot Device mount point
     */
    void NoRomPathFound(const DeviceMount& deviceRoot) override;

    /*
     * ISdl2EventNotifier implementation
     */

    /*!
     * @brief Received a raw SDL2 event
     * @param event SDL2 event
     */
    void Sdl2EventReceived(const SDL_Event& event) final;

    /*
     * ISpecialGlobalAction implementation
     */

    /*!
     * @brief Disable OSD
     * @param imagePath OSD image
     * @param x X coordinate in the range of 0.0 (left) ... 1.0 (right)
     * @param y Y coordinate in the range of 0.0 (up) ... 1.0 (bottom)
     * @param width Width in the range of 0.0 (invisible) ... 1.0 (full screen width)
     * @param height Height in the range of 0.0 (invisible) ... 1.0 (full screen height)
     * @param autoCenter if true, x/y are ignored and the image is screen centered
     */
    void EnableOSDImage(const Path& imagePath, float x, float y, float width, float height, float alpha, bool autoCenter) final
    {
      if (mApplicationWindow != nullptr) mApplicationWindow->EnableOSDImage(imagePath, x, y, width, height, alpha, autoCenter);
    }

    /*!
     * @brief Disable OSD
     */
    void DisableOSDImage() final
    {
      if (mApplicationWindow != nullptr) mApplicationWindow->DisableOSDImage();
    }

    /*
     * IKeyboardShortcut implementation
     */

    /*!
     * @brief Called when a shortcut has been pressed and released
     * @param shortcut Shortcut type
     */
    void ShortcutTriggered(IKeyboardShortcut::Shortcut shortcut) override;

    /*
     * IExtertnalVariable resolver implementation
     */

    /*!
     * @brief Resolve variables in the given string
     * @param string String in which to resolve variables
     */
    void ResolveVariableIn(String& string) final;

    //! Has a CRT adapter conencted?
    [[nodiscard]] bool HasCrt() const final { return Board::Instance().CrtBoard().GetCrtAdapter() != CrtAdapterType::None && Renderer::Instance().Is480pOrLower(); }

    //! Has a CRT adapter active?
    [[nodiscard]] bool HasCrtAttached() const final { return Board::Instance().CrtBoard().IsCrtAdapterAttached(); }

    //! Has any CRT hat installed (regardless of resolution / screen state)?
    [[nodiscard]] bool HasCrtHatInstalled() const final { return Board::Instance().CrtBoard().GetCrtAdapter() != CrtAdapterType::None; }

    //! Interlaced is usable: board capable AND configured screen has a 15 kHz mode.
    [[nodiscard]] bool HasCrtInterlaced() const final {
      if (!Board::Instance().CrtBoard().HasInterlacedSupport()) return false;
      auto freq = Board::Instance().CrtBoard().GetHorizontalFrequency();
      return freq == ICrtInterface::HorizontalFrequency::KHz15
          || freq == ICrtInterface::HorizontalFrequency::KHzMulti1525
          || freq == ICrtInterface::HorizontalFrequency::KHzMulti1531
          || freq == ICrtInterface::HorizontalFrequency::KHzTriFreq
          || freq == ICrtInterface::HorizontalFrequency::Auto;
    }

    //! Has an RGB Dual adapter
    [[nodiscard]] bool HasRRGBD() const final { return Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::RGBDual; }

    //! Has an RGB Dual 2 adapter
    [[nodiscard]] bool HasRRGBD2() const final { return Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::RGBDual2; }

    //! Has a CRT adapter active?
    [[nodiscard]] bool HasJamma() const final { return Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::RGBJamma; }

    //! Is it a PC ?
    [[nodiscard]] bool IsPC() const final { return Board::Instance().IsPC(); }
    //! Is it a Pi ?
    [[nodiscard]] bool IsPi() const final { return Board::Instance().IsPi(); }
    //! Is it an odroid board ?
    [[nodiscard]] bool IsOdroid() const final { return Board::Instance().IsOdroid(); }
    //! Is it an anbernic board
    [[nodiscard]] bool IsAnbernic() const final { return Board::Instance().IsAnbernic(); }
    //! Has a JVS adapter active?
    [[nodiscard]] bool HasJVS() const final { return Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::JVS; }

    //! Is in TATE mode?
    [[nodiscard]] bool IsTate() const final { return Renderer::Instance().IsRotatedSide(); }
    //! Is in right TATE mode?
    [[nodiscard]] bool IsTateRight() const final { return Renderer::Instance().Rotation() == RotationType::Right; }
    //! Is in left TATE mode?
    [[nodiscard]] bool IsTateLeft() const final { return Renderer::Instance().Rotation() == RotationType::Left; }

    //! Is resolution QVGA or less?
    [[nodiscard]] bool IsQVGA() const final { return Renderer::Instance().IsQVGA(); }

    //! Is Resolution between QVGA (excluded) & VGA (included)?
    [[nodiscard]] bool IsVGA() const final { return Renderer::Instance().IsVGA(); }

    //! Is Resolution between VGA (excluded) & HD (included)?
    [[nodiscard]] bool IsHD() const final { return Renderer::Instance().IsHD(); }

    //! Is Resolution higher than HD?
    [[nodiscard]] bool IsFHD() const final { return Renderer::Instance().IsFHD(); }

    //! Is Resolution higher than HD?
    [[nodiscard]] bool IsBartopModeOrHigher() const final { return RecalboxConf::Instance().GetMenuType() >= RecalboxConf::Menu::Bartop; }

    //! Menu should be avoided ?
    [[nodiscard]] bool IsNoMenuMode() const final { return RecalboxConf::Instance().GetMenuType() >= RecalboxConf::Menu::None; };

    //! Is HDMI?
    [[nodiscard]] bool HasHDMI() const final { return !(HasCrt() || HasJamma()); }

    [[nodiscard]] bool HasCardReaderAvailable() const final { return CardReader::Instance().IsAvailable(); }

    [[nodiscard]] bool HasCardReaderPlugged() const final { return CardReader::Instance().IsCardPlugged(); }

    void CardPlugged(String romPath, String systemUUID) override;
    void CardUnplugged() override;
};
