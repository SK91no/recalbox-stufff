#pragma once

#include <views/GameClipView.h>
#include "views/gamelist/ISimpleGameListView.h"
#include "views/SystemView.h"
#include "views/CardReaderConsoleView.h"
#include "SplashView.h"
#include "views/crt/CrtCalibrationView.h"
#include "guis/menus/IFastMenuListCallback.h"
#include "ISaveStateSlotNotifier.h"
#include "ISoftPatchingNotifier.h"
#include "views/gamelist/DetailedGameListView.h"
#include "guis/GuiWaitLongExecution.h"
#include "ViewTypes.h"
#include <emulators/run/GameLinkedData.h>

#include <rendering/textures/IGamelistCacheControl.h>
#include <views/BouncingView.h>

#include "utils/storage/MessageFactory.h"

class SystemData;

struct DelayedSystemOperationData
{
  SystemManager::List mSystemList;
  ISlowSystemOperation* mSlowIMethodInterface;
  bool autoSelectMonoSystem;
};

// Used to smoothly transition the camera between multiple views (e.g. from system to system, from gamelist to gamelist).
class ViewController : public StaticLifeCycleControler<ViewController>
                     , public Gui
                     , public ISystemChangeNotifier
                     , public IFastMenuListCallback
                     , public ISaveStateSlotNotifier
                     , public ISoftPatchingNotifier
                     , private Thread
                     , public ISyncMessageReceiver<SlowDataInformation*>
                     , public ILongExecution<DelayedSystemOperationData, bool>
                     , private RecalboxConf::IArcadeViewEnhancedNotification
                     , public IGamelistCacheControl
{
  public:
    //! Flags used in launch method to check what option is already selected
    enum class LaunchCheckFlags
    {
      None             = 0x00, //!< Nothing
      Bios             = 0x01, //!< Bios checked
      CrtResolution    = 0x02, //!< Crt Resolution selected
      Frequency        = 0x04, //!< Frequency selected
      Extension        = 0x08, //!< Extension
      SoftPatching     = 0x10, //!< Soft patching selected
      SuperGameboy     = 0x20, //!< Supergameboy selected
      SaveState        = 0x40, //!< Savestate selected
      CrtSignal        = 0x80, //!< Crt Signal selected
    };

    /*!
     * @brief Constructor
     * @param window Window manager
     * @param systemManager System manager
     */
  	ViewController(WindowManager& window, SystemManager& systemManager, const IGlobalVariableResolver& resolver);
    //! Destructor
    ~ViewController() override;

    /*!
     * @brief Wake up the system if it is in a sleeping state
     */
    void WakeUp() { mWindow.DoWake(); }

    /*!
     * @brief Check bios and call LaunchAnimated
     * @param game game to launch
     * @param netplay optional netplay data
     * @param centerCameraOn optional camera target point
     * @param forceGoToGame Force gamelist & game display after the game end. This is only usefull when a game is not run from its gamelist
     */
    void Launch(FileData* game, const GameLinkedData& netplay, const Vector3f& centerCameraOn, bool forceGoToGame);

    bool ForceGamelistViewRecreation(SystemData* system);
    void InvalidateGamelist(const SystemData* system);
    void InvalidateAllGamelistsExcept(const SystemData* systemExclude);

    //! Destroy every built gamelist view except the current one (kept alive, reused on game exit)
    void DestroyGameListViews();

    /*
     * IGamelistCacheControl implementation
     */

    /*!
     * @brief Destroy the oldest still-alive gamelist view (least recently used), except the current
     * one (still displayed / reused on game exit). At most one view is freed per call. Called by the
     * texture memory guard (TextureManager::WatchMemory) when texture caches are exhausted.
     * @return True if a gamelist view was actually freed, false if none was evictable
     */
    bool DestroyOldestGameListView() override;

    // Navigation.
    SystemData* goToNextGameList();
    void goToPrevGameList();
    void goToGameList(SystemData* system);
    void goToSystemView(SystemData* system);
    void goToGameConsoleView();
    void goToGameClipView();
    void goToBouncingView();
    void goToCrtView(CrtCalibrationView::CalibrationType screenType);
    void selectGamelistAndCursor(FileData* file);
    void goToStart();
    void goToQuitScreen();


    /*!
     * @brief Force the gamelist associated to the given system to refresh
     * If the gamelist doesn't exist, no refresh is performed
     * @param data Target system
     */
    void ForceGamelistRefresh(SystemData& data);

    /*!
     * @brief Set the previous vue back - Only once!
     */
    void BackToPreviousView();

    [[nodiscard]] bool IsInVirtualSystem() const { return isViewing(ViewType::GameList) && mCurrentSystem != nullptr && mCurrentSystem->IsVirtual(); }

    [[nodiscard]] bool isViewing(ViewType viewing) const { return mCurrentViewType == viewing; }

    bool CollectHelpItems(Help& help) override;

    ISimpleGameListView* GetOrCreateGamelistView(SystemData* system, bool reorder = true);

    SystemView& getSystemListView() { return mSystemListView; }

    [[nodiscard]] Gui& CurrentUi() const { return *mCurrentView; }

    void ToggleFavorite(FileData* game, bool forceStatus = false, bool forcedStatus = false);

    /*!
     * Window manager told us that a GUI has popup up over the current view
     */
    void FirstGuiOpened();

    /*!
     * Window manager told us that the last GUI closed and make our current view fully visible
     */
    void LastGuiClosed();

    //! Get pictogram cache
    PictogramCaches& GetPictomgramCache() { return mFlagCaches; }

    /*!
     * @brief Get the progress interface
     * @return Progress interface
     */
    IProgressInterface& GetProgressInterface() { return mSplashView; }

    bool CheckFilters();

    [[nodiscard]] ViewType CurrentView() const { return mCurrentViewType; }

    //! Get current system
    [[nodiscard]] SystemData* CurrentSystem() const { assert(mCurrentViewType == ViewType::GameList || mCurrentViewType == ViewType::SystemList); return mCurrentSystem; }

    /*!
     * @brief Remove the given game from every single view in which it could show
     * @param game Game to remove
     */
    void RemoveGame(FileData& game);

    /*
     * Gui implementation
     */

    [[nodiscard]] bool DoNotDisturb() const override
    {
      switch(mCurrentViewType)
      {
        case ViewType::SplashScreen: return true;
        case ViewType::CardReaderConsoleView: return true;
        case ViewType::SystemList: return mSystemListView.DoNotDisturb();
        case ViewType::GameList: return mCurrentView->DoNotDisturb();
        case ViewType::GameClip: return false;
        case ViewType::Bouncing: return false;
        case ViewType::CrtCalibration: return true;
        case ViewType::None:
        default: break;
      }
      return false;
    }

    /*
     * ISystemChangeNotifier
     */

    //! System must show
    void ShowSystem(SystemData* system) override;

    //! System must hide
    void HideSystem(SystemData* system) override;

    //! System must be updated (games have been updated inside)
    void UpdateSystem(SystemData* system) override;

    //! System should be selected
    void SelectSystem(SystemData* system) override;

    //! Request threaded operations
    void RequestSlowOperation(ISlowSystemOperation* interface, ISlowSystemOperation::List systems, bool autoSelectMonoSystem) override;

    //! User notification
    void SystemShownWithNoGames(SystemData* system) override;

    /*
     * Component override
     */

    bool ProcessInput(const InputCompactEvent& event) override;
    void Update(int deltaTime) override;
    void Render(const Transform4x4f& parentTrans) override;

    void FetchSlowDataFor(FileData* data);

    /*
     * ILongExecution implementation
     */

    bool Execute(GuiWaitLongExecution<DelayedSystemOperationData, bool>& from,
                 const DelayedSystemOperationData& parameter) override;

    void Completed(const DelayedSystemOperationData& parameter, const bool& result) override;

    void GameExited();
    bool CanPlayMusic();

  private:
    //! Animation move
    enum Move
    {
      None,
      Left,
      Right,
    };

    //! Fast menu types
    enum class FastMenuType
    {
      Frequencies,        //!< Frequency choice
      FrequenciesMulti60, //!< Frequency choice w/ multiple 60hz
      CrtResolution,      //!< CRT resolution choice
      CrtSignal,          //!< CRT signal choice
      SuperGameboy,       //!< Supergameboy choice
    };

    //! Flag texture cache
    PictogramCaches mFlagCaches;

    //! Game linked data internal instance
    GameLinkedData mGameLinkedData;
    //! Game to launch
    FileData* mGameToLaunch;
    //! Camera target for launch
    Vector3f mLaunchCameraTarget;
    //! Check flags
    LaunchCheckFlags mCheckFlags;

    //! Global variable resolver
    const IGlobalVariableResolver& mResolver;
    //! SystemManager instance
    SystemManager& mSystemManager;

    //! Current view reference
    Gui* mCurrentView;
    // Current system for views dealing with systems (System list/Game list)
    SystemData* mCurrentSystem;

    HashMap<SystemData*, ISimpleGameListView*> mGameListViews;
    SystemView mSystemListView;
    SplashView mSplashView;
    CardReaderConsoleView mCRConsoleView;
    GameClipView mGameClipView;
    BouncingView mBouncingView;
    CrtCalibrationView mCrtView;
    HashMap<SystemData*, bool> mInvalidGameList;

    ViewType mCurrentViewType;  //!< Current view type
    ViewType mPreviousViewType; //!< Previous view type


    Transform4x4f mCamera;
    float mFadeOpacity;
    bool mLockInput;

    //! Last launch time
    DateTime mLastGameLaunched;

    //! Keep choice of frequency (megadrive multi-60)
    int mFrequencyLastChoiceMulti60;
    //! Keep choice of frequency (all systems, except megadrive)
    int mFrequencyLastChoice;
    //! Keep choice of CRT resolution
    int mResolutionLastChoice;
    //! Keep choice of CRT signal
    int mSignalLastChoice;
    //! Keep choice of Supergameboy launch
    int mSuperGameboyLastChoice;
    //! Keep choice of Soft patching
    bool mSoftPatchingPreviouslyUsed;

    //! Message provider
    MessageFactory<SlowDataInformation> mMessageFactory;
    //! Item information synchronizer
    SyncMessageSender<SlowDataInformation*> mSender;
    //! Next item to fetch slow data from
    FileData* mNextItem;
    //! Locker
    Mutex mLocker;
    //! Fetch info thread signal
    Signal mSignal;

    /*!
     * @brief Change the current view and store the previous
     * @param newViewMode New view mode
     * @param targetSystem Target system for view requiring a system
     */
    void ChangeView(ViewType newViewMode, SystemData* targetSystem);

    /*!
     * @brief Check bios and call LaunchAnimated
     */
    void LaunchCheck();

    /*!
     * @brief Run animation and call LaunchActually
     * @param emulator Emulator data
     */
    void LaunchAnimated(const EmulatorData& emulator);

    /*!
     * @brief Actually run the game :)
     * @param emulator Emulator data
     */
    void LaunchActually(const EmulatorData& emulator);

    /*!
     * @brief Check BIOS before running a game
     * @param emulatorData Emulator data froo the target emulator
     * @return True if we're waiting for a user interaction, false otherwise
     */
    bool CheckBios(const EmulatorData& emulatorData);

    /*!
     * @brief Check if the user is required to choose a refresh rate before running the selected game
     * @return True if we're waiting for a user interaction, false otherwise
     */
    bool CheckRefreshRate();

    /*!
     * @brief Check if the user is required to choose a crt signal type
     * @return True if we're waiting for a user interaction, false otherwise
     */
    bool CheckCRTSignalType();

    /*!
     * @brief Check if the user is required to choose a CRT Resolution before running the selected game
     * @param emulator Emulator data from the target emulator
     * @return True if we're waiting for a user interaction, false otherwise
     */
    bool CheckCRTResolution(const EmulatorData& emulator);

    /*!
     * @brief Check if the user is required to select a patch before running the selected game
     * @param emulator Emulator data from the target emulator
     * @return True if we're waiting for a user interaction, false otherwise
     */
    bool CheckSoftPatching(const EmulatorData& emulator);

    /*!
     * @brief Check if the user is required to select a gb/sgb mode before running the selected game
     * @return True if we're waiting for a user interaction, false otherwise
     */
    bool CheckSuperGameboy();

    /*!
     * @brief Check if the user is required to select a save state before running the selected game
     * @param emulator Emulator data from the target emulator
     * @return True if we're waiting for a user interaction, false otherwise
     */
    bool CheckSaveStates(const EmulatorData& emulator);

    /*!
     * @brief Check if selected file extension is supported by the emulator
     * @param emulatorData Emulator data from the target emulator
     * @return True if we're waiting for a user interaction, false otherwise
     */
    bool CheckExtensions(const EmulatorData& emulatorData);

    /*!
     * @brief Reset game filters
     */
    static void ResetFilters();

    void playViewTransition();

    /*!
     * @brief Reorder gamelist view according to current visible systems and next move
     * Replace camera if required
     * @param target Target system
     * @param rightMove Next move
     * @return X position of the target system
     */
    float ReorderGamelistViewBeforeMoving(SystemData* target, Move rightMove);

    /*
     * IFastMenuLineCallback implementation
     */

    /*!
     * @brief Main callback
     * @param menuIndex Menu identifier to identify multiple menu in a single callback
     * @param itemIndex Item index, starting from
     */
    void FastMenuLineSelected(int menuIndex, int itemIndex) override;

    /*
     * ISaveStateSlotNotifier implementation
     */

    /*!
     * @brief Notifie a slot has been selected
     * @param slot Slot number or -1
     */
    void SaveStateSlotSelected(int slot) override;

    /*
     * ISoftPatchingNotifier implementation
     */

    bool PreviousLaunchWasUsingSoftpathing() final { return mSoftPatchingPreviouslyUsed; }

    /*!
     * @brief Notify the soft patching is disabled
     */
    void SoftPathingDisabled() override;

    /*!
     * @brief Notify a path has been selected
     * @param path Selected patch's path
     */
    void SoftPatchingSelected(const Path& path) override;

    /*
     * Thread implementation
     */

    //! Break the system info fetching thread
    void Break() override { mSignal.Fire(); }

    //! Implementation of system fetching info
    void Run() override;

    /*
     * ISyncMessageReceiver<const FileData*> implementation
     */

    //! Receive enf-of-fetch info for the very last system the user is on
    void ReceiveSyncMessage(SlowDataInformation* const& data) final;

    /*
     * RecalboxConf::IArcadeViewEnhancedNotification
     */

    void ConfigurationArcadeViewEnhancedChanged(const bool& value) final;

};

DEFINE_BITFLAG_ENUM(ViewController::LaunchCheckFlags, int)