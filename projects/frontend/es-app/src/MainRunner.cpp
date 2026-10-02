//
// Created by bkg2k on 13/11/2019.
//
#include <utils/locale/LocaleHelper.h>
#include <utils/Log.h>
#include <utils/Files.h>
#include <audio/AudioScheduler.h>
#include <views/ViewController.h>
#include <guis/GuiMsgBoxScroll.h>
#include <guis/GuiDetectDevice.h>
#include <VideoEngine.h>
#include <bios/BiosManager.h>
#include <guis/GuiMsgBox.h>
#include <scraping/ScraperFactory.h>
#include <audio/AudioController.h>
#include <hardware/RGPProxy.h>
#include <recalbox/RecalboxSystem.h>
#include <guis/wizards/WizardAgo2.h>
#include <guis/wizards/WizardAgo3.h>
#include <guis/wizards/WizardRG353X.h>
#include "MainRunner.h"
#include "EmulationStation.h"
#include "Upgrade.h"
#include "CommandThread.h"
#include <netplay/NetPlayThread.h>
#include "DemoMode.h"
#include "RootFolders.h"
#include "web/RestApiServer.h"
#include "network/Networks.h"
#include <hardware/devices/PhysicalDevices.h>
#include "guis/GuiSearch.h"
#include "guis/menus/modaltasks/MenuModalRescanRoms.h"
#include "guis/menus/modaltasks/MenuModalRefreshGamelist.h"
#include "guis/GuiInfoPopupSeamlessScraper.h"
#include "rendering/textures/TextureManager.h"
#include "rendering/fonts/FontManager.h"
#include "guis/menus/modaltasks/MenuModalInitDevice.h"
#include "guis/menus/modaltasks/MenuModalFactoryReset.h"
#include "guis/GuiInputConfig.h"
#include <utils/network/DnsClient.h>
#include <music/RemotePlaylist.h>
#include <scraping/ScraperSeamless.h>
#include <sdl2/Sdl2Runner.h>
#include <emulators/run/GameRunner.h>
#include <sdl2/Sdl2Init.h>
#include <patreon/PatronInfo.h>
#include <guis/GuiScraperRun.h>
#include <guis/menus/MenuProvider.h>

#include "guis/GuiJukebox.h"
#include "utils/os/system/ProcessTree.h"
#include "guis/wizards/WizardWelcome.h"
#include <RendererGLES2.h>
#include <RendererGLES3.h>
#include <RendererGLCore.h>
#include <RendererVulkan.h>

MainRunner::ExitState MainRunner::sRequestedExitState = MainRunner::ExitState::Quit;
bool MainRunner::sQuitRequested = false;

MainRunner::MainRunner(const String& executablePath, int runCount, char** environment, const Options& options)
  : mOptions(options)
  , mPendingExit(PendingExit::None)
  , mRunCount(runCount)
  , mCardReaderGameToRun("")
  , mNotificationManager(environment)
  , mApplicationWindow(nullptr)
  , mExecutableFolder(Path(executablePath).Directory())
{
  Intro();
  SetLocale(executablePath);
  CheckHomeFolder();

  // Initialize SDL
  Sdl2Init::Initialize();
}

MainRunner::~MainRunner()
{
  // Finalize SDL
  Sdl2Init::Finalize();
}

MainRunner::ExitState MainRunner::Run()
{
  try
  {
    // Reinstall backuped files after a factoryu reset
    MenuModalFactoryReset::ReinstallFilesAfterReboot();

    // Hardware board
    Board board(*this, mOptions);
    // Physical disk enumeration
    PhysicalDevices physicalDevices;
    // Push current Recalbox Gamepad Proxy configuration to the kernel module
    // (no-op if the module is not loaded). Done unconditionally on every boot.
    RGPProxy::Instance().Apply();
    // Wifi
    Networks wifi;

    // Set best performance/power CPU governor for battery-powered devices
    board.SetFrontendPerformanceGovernor();

    // Audio controller
    AudioController audioController;
    String originalAudioDevice = RecalboxConf::Instance().GetAudioOuput();
    String fixedAudioDevice = audioController.SetDefaultPlayback(originalAudioDevice);
    if (fixedAudioDevice != originalAudioDevice)
      RecalboxConf::Instance().SetAudioOuput(fixedAudioDevice).Save();

    // Notification Manager
    mNotificationManager.Notify(Notification::Start, String(mRunCount));

    // Shut-up joysticks :)
    SDL_JoystickEventState(SDL_DISABLE);

    SystemManager systemManager(*this, *this);
    GameRunner gameRunner(nullptr, systemManager, *this);
    InputManager inputManager(this);
    inputManager.Initialize();

    // Autorun?
    SystemData* autoRunSystem = nullptr;
    CardReader cardReader(this);

    if (mRunCount == 0) {
      { LOG(LogInfo) << "[MainRunner] First Run. BootOnGame("<< (RecalboxConf::Instance().GetBootOnGameEnabled() ? "true": "false") << "), CR.ShouldBootOnGame(" << (cardReader.ShouldBootOnGame() ? "true": "false") <<")"; }

      if (RecalboxConf::Instance().GetBootOnGameEnabled() && (!RecalboxConf::Instance().GetBootOnGameGamePath().empty() || cardReader.ShouldBootOnGame()))
      {
        String systemUUID;
        String gamePath;
        if (cardReader.ShouldBootOnGame())
        {
          systemUUID = cardReader.CurrentCardSystemUUID();
          gamePath = cardReader.CurrentCardRomPath();
          { LOG(LogInfo) << "[MainRunner] CardReader wants to boot on " << systemUUID << " -> "  << gamePath; }
        }
        else
        {
          systemUUID = RecalboxConf::Instance().GetBootOnGameSystemUUID();
          gamePath = RecalboxConf::Instance().GetBootOnGameGamePath();
        }
        systemManager.LoadSingleSystemConfigurations(systemUUID);
        FileData *game = systemManager.LookupGameByFilePath(gamePath);
        if (game == nullptr)
        {
          { LOG(LogInfo) << "[MainRunner] Will not boot on game as game = null "; }
          String message = Files::LoadFile(Path(sBootNotification)).Append("\nWarning|").Append(_("Unable to start on the game, could not find the game."));
          Files::SaveFile(Path(sBootNotification), message);
        }
        else
        {
          if (inputManager.ConfiguredControllersCount() > 0)
          {
            autoRunSystem = &(game->System());
            GameLinkedData data = GameLinkedData();
            // Start game in 240p@120Hz if the option has been set
            if (Board::Instance().CrtBoard().Has120HzSupport()
              && Board::Instance().CrtBoard().GetHorizontalFrequency() >= ICrtInterface::HorizontalFrequency::KHz31
              && CrtConf::Instance().GetSystemCRTRunDemoAndAutoIn240pOn31kHz())
              data.ConfigurableCrt().ConfigureForceResolution(CrtData::CrtMode::DoubleFreq);
            { LOG(LogInfo) << "[MainRunner] Booting on game "<< game; }
            ResolutionAdapter adapter;
            gameRunner.RunGame(adapter, *game, EmulatorManager::GetGameEmulator(*game), data);
          }
          else
          {
            { LOG(LogInfo) << "[MainRunner] Will not boot on game, no configured controllers found"; }
            String message = Files::LoadFile(Path(sBootNotification)).Append("\nWarning|").Append(_("Unable to start on the game, no controllers found."));
            Files::SaveFile(Path(sBootNotification), message);
          }
        }
      }
      else if (mConfiguration.GetKodiEnabled() && mConfiguration.GetKodiAtStartup())
      {
          gameRunner.RunKodi();
      }
    }

    // Initialize the renderer first,'cause many things depend on renderer width/height
    #ifdef USE_VULKAN
    RendererVulkan renderer(mOptions.Width(), mOptions.Height(), mOptions.Windowed(), RotationManager::GetSystemRotationOverride(mOptions));
    #elif defined(USE_OPENGL_ES31)
    RendererGLES3 renderer(mOptions.Width(), mOptions.Height(), mOptions.Windowed(), RotationManager::GetSystemRotationOverride(mOptions));
    #elif defined(USE_OPENGL_ES)
    RendererGLES2 renderer(mOptions.Width(), mOptions.Height(), mOptions.Windowed(), RotationManager::GetSystemRotationOverride(mOptions));
    #elif defined(USE_OPENGL_CORE)
    RendererGLCore renderer(mOptions.Width(), mOptions.Height(), mOptions.Windowed(), RotationManager::GetSystemRotationOverride(mOptions));
    #else
    #error "No renderer backend defined: please define USE_VULKAN, USE_OPENGL_ES31, USE_OPENGL_ES or USE_OPENGL_CORE"
    #endif
    if (!renderer.Initialized()) { LOG(LogError) << "[Renderer] Error initializing the GL renderer."; return ExitState::FatalError; }
    TextureManager textureManager;
    FontManager fontManager;

    // Theme manager
    ThemeManager themeManager(*this);
    themeManager.Initialize(nullptr);
    // Load theme from the first loaded system
    if (autoRunSystem != nullptr) themeManager.LoadSystemTheme(*autoRunSystem);

    // Initialize main Window and ViewController
    ApplicationWindow window(systemManager, mOptions, *this);
    if (!window.Initialize(mOptions.Width(), mOptions.Height(), false))
    { LOG(LogError) << "[Renderer] Window failed to initialize!"; return ExitState::FatalError; }

    // Initialize audio manager
    AudioScheduler audioManager(window);
    PlayLoadingSound(audioManager);

    // Display "loading..." screen or console mode
    window.RenderAll();

    // Store window manager
    mApplicationWindow = &window;
    gameRunner.SetWindowManager(&window);
    mBluetooth.Register(&window.OSD().GetBluetoothOSD());
    // Brightness
    if (board.HasBrightnessSupport())
      board.SetBrightness(RecalboxConf::Instance().GetBrightness());

    // Menu provider
    MenuProvider menuProvider(window, systemManager, mExecutableFolder / "data/resources/menu.xml", *this);

    // Board-related background processes
    // Initialize here so that all global object are available
    board.StartGlobalBackgroundProcesses();

    if (!TryToLoadConfiguredSystems(systemManager))
      return ExitState::FatalError;

    // Scrapers
    ScraperFactory scraperFactory;

    // External notification by file - hub always declared before watcher
    FileSystemWatcherHub fileHub("ExtNotfWatch");
    FileSystemWatcher fileNotifier;
    fileHub.AddWatcher(fileNotifier, *this);

    ExitState exitState = ExitState::Quit;
    try
    {
      // Bios (must be created before the webmanager starts)
      BiosManager biosManager;
      biosManager.LoadFromFile();
      biosManager.Scan(this, false);

      // Start webserver (owns the web manager <-> main thread configuration bridge)
      { LOG(LogDebug) << "[MainRunner] Launching Webserver"; }
      RestApiServer webManager(systemManager);

      // Patron Information
      PatronInfo patronInfo(this);
      // Remote music
      RemotePlaylist remotePlaylist(this);

      // Start update thread
      { LOG(LogDebug) << "[MainRunner] Launching Network thread"; }
      Upgrade networkThread(window, mRunCount == 0);
      // Start the socket server
      { LOG(LogDebug) << "[MainRunner] Launching Command thread"; }
      CommandThread commandThread(systemManager);
      // Start Netplay thread
      { LOG(LogDebug) << "[MainRunner] Launching Netplay thread"; }
      NetPlayThread netPlayThread(window);

      // Seamless scraper
      ScraperSeamless seamlessScraper;
      GuiInfoPopupSeamlessScraper seamlessScraperUI(window);

      // Input ok?
      InitializeUserInterface(window);

      // Update?
      CheckUpdateMessage(window);
      CheckUpdateFailed(window);
      CheckUpdateCorrupted(window);
      // Wizard
      CheckFirstTimeWizard(window);
      // Alert
      CheckAlert(window, systemManager);

      // Boot notifications (simple info popups)
      CheckBootNotification(window);

      // Enable joystick autopairing
      if(RecalboxConf::Instance().GetAutoPairOnBoot() && inputManager.ConfiguredControllersCount() == 0)
        mBTAutopairManager.StartDiscovery();

      // Main Loop!
      CreateReadyFlagFile();
      Path externalNotificationFolder = Path(sQuitNow).Directory();
      (void)externalNotificationFolder.CreatePath();
      fileNotifier.WatchFile(externalNotificationFolder, EventType::CloseWrite | EventType::Remove | EventType::Create);

      //mApplicationWindow->pushGui(new GuiInputConfig(*mApplicationWindow, &InputManager::Instance().GetDeviceConfigurationFromIndex(0), nullptr));
      //mApplicationWindow->pushGui(new GuiDetectDevice(*mApplicationWindow, false, nullptr));

      // Main SDL loop
      exitState = MainLoop(window, systemManager, mSyncMessageFactory, gameRunner);

      GuiScraperRun::Abort();
      ResetExitState();
      fileNotifier.RemoveAllWatches();
      DeleteReadyFlagFile();
      window.deleteAllGui();
    }
    catch(std::exception& ex)
    {
      { LOG(LogError) << "[MainRunner] Main thread crashed (inner)."; }
      { LOG(LogError) << "[MainRunner] Exception: " << ex.what(); }
      exitState = ExitState::Relaunch;
    }

    // Exit
    audioManager.StopAll();
    mNotificationManager.Notify(Notification::Stop, String(mRunCount));
    window.GoToQuitScreen();
    systemManager.DeleteAllSystems(DoWeHaveToUpdateGamelist(exitState), true);
    WindowManager::Finalize();
    mApplicationWindow = nullptr;

    { LOG(LogInfo) << "[MainRunner] Quit requested (outer) [" << (int)sRequestedExitState << ']'; }
    switch(exitState)
    {
      case ExitState::Quit:
      case ExitState::FatalError: mNotificationManager.Notify(Notification::Quit, exitState == ExitState::FatalError ? "fatalerror" : "quitrequested"); break;
      case ExitState::Relaunch:
      case ExitState::RelaunchNoUpdate: mNotificationManager.Notify(Notification::Relaunch); break;
      case ExitState::NormalReboot:
      case ExitState::FastReboot:
      {
        mNotificationManager.Notify(Notification::Reboot, exitState == ExitState::FastReboot ? "fast" : "normal");
        board.OnRebootOrShutdown();
        break;
      }
      case ExitState::Shutdown:
      case ExitState::FastShutdown:
      {
        mNotificationManager.Notify(Notification::Shutdown, exitState == ExitState::FastShutdown ? "fast" : "normal");
        board.OnRebootOrShutdown();
        break;
      }
    }

    // Wait for all notifications to be processed before
    // main objects are destroyed
    mNotificationManager.WaitCompletion();

    return exitState;
  }
  catch(std::exception& ex)
  {
    { LOG(LogError) << "[MainRunner] Main thread crashed (outer)."; }
    { LOG(LogError) << "[MainRunner] Exception: " << ex.what(); }
  }

  // If we get there, a severe and probably non-recoverable error occured.
  // Just quit
  return ExitState::FatalError;
}

void MainRunner::CreateReadyFlagFile()
{
  // Create a flag in  temporary directory to signal READY state
  Path ready(sReadyFile);
  Files::SaveFile(ready, "ready");
}

void MainRunner::DeleteReadyFlagFile()
{
  Path ready(sReadyFile);
  (void)ready.Delete();
}

void MainRunner::ProcessSDLEvent(ApplicationWindow& window, SDL_Event& event)
{
  for(;;)
  {
    //{ LOG(LogInfo) << "[MainRunner] Event in Loop event."; }
    InputCompactEvent compactEvent = InputManager::Instance().ManageSDLEvent(&window, event);
    // TODO: invert those lines, special events should be managed by the board in priority
    if (!ProcessSpecialInputs(compactEvent))
      if (!Board::Instance().ProcessSpecialInputs(compactEvent, this))
        if (!compactEvent.Empty())
          window.ProcessInput(compactEvent);
    // Mouse Wheel event must resend a null move
    if (event.type == SDL_MOUSEWHEEL)
      if ((int)event.wheel.which >= 0) { event.wheel.which = -1; continue; }
    break;
  }
}

MainRunner::ExitState MainRunner::MainLoop(ApplicationWindow& window, SystemManager& systemManager, SyncMessageFactory& syncMessageFactory, GameRunner& gameRunner)
{
  // Allow joystick event
  SDL_JoystickEventState(SDL_ENABLE);

  // Demo mode (real game launching)
  DemoMode demoMode(window, systemManager);

  { LOG(LogDebug) << "[MainRunner] Entering main loop"; }
  Path mustExit(sQuitNow);
  int lastTime = (int)SDL_GetTicks();
  for(;;)
  {
    // File watching
    InputManager& inputManager = InputManager::Instance();
    inputManager.WatchJoystickAddRemove(&window);
    // Sync'ed message
    syncMessageFactory.DispatchMessage();
    // Sleeping video engines
    VideoEngine::ReduceSleepingEngineMemoryFootprint();

    // Get SDL events
    Array<SDL_Event> events;
    SDL_Event sdlevent;
    while (SDL_PollEvent(&sdlevent) != 0)
      switch (sdlevent.type)
      {
        case SDL_QUIT: return ExitState::Quit;
        case SDL_TEXTINPUT:
        {
          if (inputManager.IsUsingNativeSDL()) { window.textInput(sdlevent.text.text); }
          else
          {
            String remappedText = inputManager.RemapTextInput(sdlevent.text.text);
            window.textInput(remappedText.c_str());
          }
          break;
        }
        //case SDL_JOYHATMOTION:
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        case SDL_JOYAXISMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEWHEEL: events.Add(sdlevent); break;
        default: break;
      }

    // Add recreated button events
    inputManager.GetRawEvents(events);

    // Process loop
    for(SDL_Event& event : events)
    {
       ProcessSDLEvent(window, event);
       if (window.Closed()) RequestQuit(ExitState::Quit);
    }

    if (window.isSleeping() && !GameClipView::IsGameClipEnabled() && !BouncingView::IsBouncingEnabled())
    {
      if (DemoMode::hasDemoMode())
        demoMode.runDemo();

      lastTime = (int)SDL_GetTicks();
      // Take a breath
      SDL_Delay(1);
      continue;
    }

    if (!mCardReaderGameToRun.empty())
    {
      LOG(LogInfo) << "[MainRunner] Starting game " << mCardReaderGameToRun;
      FileData *game = systemManager.LookupGameByFilePath(mCardReaderGameToRun);
      mCardReaderGameToRun = "";
      if (game)
      {
        GameLinkedData data = GameLinkedData();
        // Start game in 240p@120Hz if the option has been set
        if (Board::Instance().CrtBoard().Has120HzSupport()
          && Board::Instance().CrtBoard().GetHorizontalFrequency() >= ICrtInterface::HorizontalFrequency::KHz31
          && CrtConf::Instance().GetSystemCRTRunDemoAndAutoIn240pOn31kHz())
          data.ConfigurableCrt().ConfigureForceResolution(CrtData::CrtMode::DoubleFreq);
        ResolutionAdapter adapter;
        gameRunner.RunGame(adapter, *game, EmulatorManager::GetGameEmulator(*game), data);
        ViewController::Instance().GameExited();
      }
      else
      {
        String message = "The game associated with the card was not found on your Recalbox, the card association has been cleared.";
        PopupType icon = PopupType::Help;
        mApplicationWindow->InfoPopupAdd(new GuiInfoPopup(*mApplicationWindow, _S(message), 10, icon));
        CardReader::Instance().DeleteGameForCard();
      }
    }

    int curTime = (int)SDL_GetTicks();
    int deltaTime = curTime - lastTime;
    lastTime = curTime;
    if (deltaTime > 1000 || deltaTime < 0) // cap deltaTime at 1000
      deltaTime = 1000;

    // Proactive texture memory watch: driven once per frame, on the GL context thread (this loop
    // owns the GL context). This is the ONLY place that proactively frees texture memory now - the
    // loader worker threads no longer do it, which removes the cross-thread lock contention that
    // used to stall background image downloads and freeze the UI.
    TextureManager::Instance().WatchMemory();

    window.Update(deltaTime);
    window.RenderAll();

    // Quit Request?
    if (sQuitRequested)
    {
      { LOG(LogInfo) << "[MainRunner] Quit requested (inner) [" << (int)sRequestedExitState << ']'; }
      return sRequestedExitState;
    }

    // Quit pending?
    switch(mPendingExit)
    {
      case PendingExit::None: break;
      case PendingExit::GamelistChanged:
      case PendingExit::ThemeChanged:
      {
        String text = (mPendingExit == PendingExit::GamelistChanged) ?
          _("EmulationStation has detected external changes on a gamelist file.\nTo avoid loss of data, EmulationStation is about to relaunch and reload all files.") :
          _("EmulationStation has detected external changes on a theme file.\nTo avoid loss of data, EmulationStation is about to relaunch and reload all files.");
        GuiMsgBox* msgBox = new GuiMsgBox(window, text, _("OK"), [] { RequestQuit(ExitState::Relaunch); });
        window.pushGui(msgBox);
        break;
      }
      case PendingExit::MustExit: RequestQuit(ExitState::Quit);
      case PendingExit::WaitingExit: break;
    }
    if (mPendingExit != PendingExit::None)
      mPendingExit = PendingExit::WaitingExit; // Wait for exit
  }
}

void MainRunner::InitializeUserInterface(WindowManager& window)
{
  (void)window;
  { LOG(LogDebug) << "[MainRunner] Preparing GUI"; }
  ViewController::Instance().goToStart();
}

void MainRunner::CheckAlert(WindowManager& window, SystemManager& systemManager)
{
  int memory = Board::Instance().TotalMemory();
  int maxSystem = 20 * (memory / 256);
  int maxGames = 5000 * (memory / 256);
  if (memory != 0 && memory <= 512)
  {
    int realSystemCount = 0;
    for(const SystemData* system : systemManager.VisibleSystemList())
      if (system->HasVisibleGame())
        realSystemCount++;
    if (realSystemCount > maxSystem)
    {
      String text = _("Your system has not enough memory to handle %SYSTEMS% systems. You should not exceed %MAXSYSTEMS% consoles/computers or you may face stability issues!\n\nYou can hide preinstalled games in UI SETTINGS menu to decrease active systems")
                    .Replace("%SYSTEMS%", String(realSystemCount))
                    .Replace("%MAXSYSTEMS%", String(maxSystem));
      window.pushGui(new GuiMsgBoxScroll(window, _("WARNING! SYSTEM OVERLOAD!"), text, _("OK"), nullptr, "", nullptr, "", nullptr, ::Alignment::CenterLeft));
    }
    else if (systemManager.GameCount() > maxGames)
    {
      String text = _("Your system has not enough memory to handle %GAMES% games. You should not exceed %MAXGAMES% or you may face stability issues!")
                         .Replace("%GAMES%", String(systemManager.GameCount()))
                         .Replace("%MAXGAMES%", String(maxGames));
      window.pushGui(new GuiMsgBoxScroll(window, _("WARNING! SYSTEM OVERLOAD!"), text, _("OK"), nullptr, "", nullptr, "", nullptr, ::Alignment::CenterLeft));
    }
  }
}

void MainRunner::CheckFirstTimeWizard(WindowManager& window)
{
  if (RecalboxConf::Instance().GetFirstTimeUse())
  {
    switch (Board::Instance().GetBoardType())
    {
      case BoardType::OdroidAdvanceGo:
      {
        window.pushGui(new WizardAGO2(window));
        RecalboxConf::Instance().SetFirstTimeUse(false);
        return; // Let the OGA Wizard reset the flag
      }
      case BoardType::OdroidAdvanceGoSuper:
      {
        window.pushGui(new WizardAgo3(window));
        RecalboxConf::Instance().SetFirstTimeUse(false);
        return; // Let the OGA Wizard reset the flag
      }
      case BoardType::RG353P:
      case BoardType::RG353V:
      case BoardType::RG353M:
      case BoardType::RG503:
      {
        window.pushGui(new WizardRG353X(window));
        RecalboxConf::Instance().SetFirstTimeUse(false);
        return; // Let the RG Wizard reset the flag
      }
      case BoardType::RG351V: // todo
      case BoardType::RG351P: // todo
      case BoardType::PCx86:
      case BoardType::PCx64:
      case BoardType::UndetectedYet:
      case BoardType::Unknown:
      case BoardType::Pi0:
      case BoardType::Pi02:
      case BoardType::Pi1:
      case BoardType::Pi2:
      case BoardType::Pi3:
      case BoardType::Pi4:
      case BoardType::Pi400:
      case BoardType::Pi5:
      case BoardType::Pi500:
      case BoardType::Pi3plus:
      case BoardType::PiUnknown:
      case BoardType::AnbernicUnknown:
      case BoardType::OdroidUnknown:
      case BoardType::SteamDeckLCD:
      case BoardType::SteamDeckOLED:
      default:
      {
        if (WizardWelcome::ShouldShowWizard())
          window.pushGui(new WizardWelcome(window));
        break;
      }
    }
  }
  else
  {
    if (WizardWelcome::ShouldShowWizard())
      window.pushGui(new WizardWelcome(window));
  }
}

void MainRunner::CheckBootNotification(WindowManager& window)
{
  Path file(sBootNotification);
  if (file.Exists())
  {
    String lines = Files::LoadFile(file);
    for (const String& notif : lines.Split("\n"))
    {
      if (!notif.empty())
      {
        String message = notif;
        PopupType icon = PopupType::Help;
        if (String::List notifAndType = notif.Split("|"); notifAndType.size() > 1)
        {
          message = notifAndType[1];
          if (notifAndType[0] == "Warning")
            icon = PopupType::Warning;
        }
        window.InfoPopupAdd(new GuiInfoPopup(window, _S(message), 10, icon));
      }
    }
    (void)file.Delete();
  }
}

void MainRunner::CheckUpdateMessage(WindowManager& window)
{
  // Push a message box with the changelog if Recalbox has been updated
  Path flag(sUpgradeFileFlag);
  if (flag.Exists() && !window.DoNotDisturb())
  {
    String changelog = Files::LoadFile(Path(Upgrade::sLocalReleaseNoteFile));
    String message = "Changes :\n" + changelog;
    window.pushGui(new GuiMsgBoxScroll(window, _("THE SYSTEM IS UP TO DATE"), message, _("OK"), []{}, "", nullptr, "", nullptr, ::Alignment::CenterLeft));
    (void)flag.Delete();
  }
}

void MainRunner::CheckUpdateFailed(WindowManager& window)
{
  // Push a message if Recalbox upgrade has failed
  Path flag(sUpgradeFailedFlag);
  if (flag.Exists())
  {
    String version = Upgrade::CurrentVersion();
    String message = _("The upgrade process has failed. You are back on Recalbox %s.\nPlease retry to upgrade your Recalbox, and contact the team on https://forum.recalbox.com if the problem persists.")
                     .Replace("%s", version.c_str());
    window.pushGui(new GuiMsgBoxScroll(window, _("THE UPGRADE HAS FAILED"), message, _("OK"), []{}, "", nullptr, "", nullptr, ::Alignment::CenterLeft));
    (void)flag.Delete();
  }
}

void MainRunner::CheckUpdateCorrupted(WindowManager& window)
{
  // Push a message if Recalbox upgrade has failed
  Path flag(sUpgradeCorruptedFlag);
  if (flag.Exists())
  {
    String version = Upgrade::CurrentVersion();
    String message = _("One or more files are corrupted. You are back on Recalbox %s.\nPlease retry to upgrade your Recalbox, check your Recalbox storage (SD Card, USB Key or hard drive).\nContact the team on https://forum.recalbox.com if the problem persists.")
                     .Replace("%s", version.c_str());
    window.pushGui(new GuiMsgBoxScroll(window, _("THE UPGRADE IS CORRUPTED"), message, _("OK"), []{}, "", nullptr, "", nullptr, ::Alignment::CenterLeft));
    (void)flag.Delete();
  }
}

void MainRunner::PlayLoadingSound(AudioScheduler& audioManager)
{
  String selectedTheme = RecalboxConf::Instance().GetThemeFolder();
  Path loadingMusic = RootFolders::DataRootFolder / "system/.emulationstation/themes" / selectedTheme / "fx/loading.ogg";
  if (!loadingMusic.Exists())
    loadingMusic = RootFolders::DataRootFolder / "themes" / selectedTheme / "fx/loading.ogg";
  if (loadingMusic.Exists())
  {
    audioManager.PlayMusicFile(loadingMusic, false);
  }
}

bool MainRunner::TryToLoadConfiguredSystems(SystemManager& systemManager)
{
  bool portable = BootConf::Instance().AsString("case") == "GPiV1:1" || Board::IsHandheldSystem();

  if (!systemManager.LoadSystemConfigurations(portable, Board::Instance().CrtBoard().IsCrtAdapterAttached()))
  {
    { LOG(LogError) << "[MainRunner] Error while parsing systems configuration file!"; }
    { LOG(LogError) << "[MainRunner] IT LOOKS LIKE YOUR SYSTEMS CONFIGURATION FILE HAS NOT BEEN SET UP OR IS INVALID. YOU'LL NEED TO DO THIS BY HAND, UNFORTUNATELY.\n\n"
                       "VISIT EMULATIONSTATION.ORG FOR MORE INFORMATION."; }
    return false;
  }

  if (systemManager.VisibleSystemList().Empty())
  {
    { LOG(LogError) << "[MainRunner] No systems found! Does at least one system have a game present? (check that extensions match!)\n(Also, make sure you've updated your es_systems.cfg for XML!)"; }
    { LOG(LogError) << "[MainRunner]  WE CAN'T FIND ANY SYSTEMS!\n"
                       "CHECK THAT YOUR PATHS ARE CORRECT IN THE SYSTEMS CONFIGURATION FILE, AND "
                       "YOUR GAME DIRECTORY HAS AT LEAST ONE GAME WITH THE CORRECT EXTENSION.\n"
                       "\n"
                       "VISIT RECALBOX.FR FOR MORE INFORMATION."; }
    return false;
  }

  return true;
}

void onExit()
{
  //::Log::Close();
}

void Sdl2Log(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
  (void)userdata;
  const char* cat = "Unknown";
  switch(category)
  {
    case SDL_LOG_CATEGORY_APPLICATION: cat = "Application"; break;
    case SDL_LOG_CATEGORY_ERROR: cat = "Error"; break;
    case SDL_LOG_CATEGORY_ASSERT: cat = "Assert"; break;
    case SDL_LOG_CATEGORY_SYSTEM: cat = "System"; break;
    case SDL_LOG_CATEGORY_AUDIO: cat = "Audio"; break;
    case SDL_LOG_CATEGORY_VIDEO: cat = "Video"; break;
    case SDL_LOG_CATEGORY_RENDER: cat = "Render"; break;
    case SDL_LOG_CATEGORY_INPUT: cat = "Input"; break;
    case SDL_LOG_CATEGORY_TEST: cat = "Test"; break;
    default: break;
  }
  const char*  subType = "Unknown";
  switch(priority)
  {
    case SDL_LOG_PRIORITY_VERBOSE: subType = "Verbose"; break;
    case SDL_LOG_PRIORITY_DEBUG: subType = "Debug"; break;
    case SDL_LOG_PRIORITY_INFO: subType = "Info"; break;
    case SDL_LOG_PRIORITY_WARN: subType = "Warning"; break;
    case SDL_LOG_PRIORITY_ERROR: subType = "Error"; break;
    case SDL_LOG_PRIORITY_CRITICAL: subType = "Critical"; break;
    case SDL_NUM_LOG_PRIORITIES:
    default: break;
  }
  { LOG(LogDebug) << "[SDL2] (" << cat << ':' << subType << ") " << message; }
}

void MainRunner::SetDebugLogs(bool debug, bool trace)
{
  if (trace)
  {
    ::Log::SetAllMinimumReportingLevel(LogLevel::LogTrace);
    SDL_LogSetOutputFunction(Sdl2Log, nullptr);
    SDL_LogSetAllPriority(SDL_LogPriority::SDL_LOG_PRIORITY_VERBOSE);
  }
  else if (debug)
  {
    ::Log::SetAllMinimumReportingLevel(LogLevel::LogDebug);
    SDL_LogSetOutputFunction(Sdl2Log, nullptr);
    SDL_LogSetAllPriority(SDL_LogPriority::SDL_LOG_PRIORITY_VERBOSE);
  }
  else
  {
    ::Log::SetAllReportingLevel(LogLevel::LogInfo);
    SDL_LogSetOutputFunction(nullptr, nullptr);
    SDL_LogSetAllPriority(SDL_LogPriority::SDL_LOG_PRIORITY_ERROR);
  }
}

void MainRunner::Intro()
{
  if (atexit(&onExit) != 0) // Always close the log on exit
    { LOG(LogError) << "[MainRunner] Error setting exit function!"; }

  SetDebugLogs(mOptions.Debug() || mConfiguration.GetDebugLogs(), mOptions.Trace());

  { LOG(LogInfo) << "[MainRunner] EmulationStation - v" << PROGRAM_VERSION_STRING << ", built " << PROGRAM_BUILT_STRING; }
}

void MainRunner::CheckHomeFolder()
{
  //make sure the config directory exists
  Path home = RootFolders::DataRootFolder;
  Path configDir = home / "system/.emulationstation";
  if (!configDir.Exists())
  {
    { LOG(LogError) << "[MainRunner] Creating config directory \"" << configDir.ToString() << "\"\n"; }
    if (!configDir.CreatePath()) { LOG(LogError) << "[MainRunner] Config directory could not be created!\n"; }
  }
}

void MainRunner::SetLocale(const String& executablePath)
{
  Path path(executablePath);
  path = path.Directory(); // Get executable folder
  if (path.IsEmpty() || !path.Exists()) { LOG(LogError) << "[Locale] Error getting executable path (received: " << executablePath << ')'; }

  // Get locale from configuration
  String localeName = RecalboxConf::Instance().GetSystemLanguage();

  // Set locale
  if (!Internationalizer::InitializeLocale(localeName, { path / "locale/lang", Path("/usr/share/locale") }, "emulationstation2"))
  { LOG(LogWarning) << "[Locale] No locale found. Default text used."; }
}

void MainRunner::RequestQuit(MainRunner::ExitState requestedState)
{
  sQuitRequested = true;
  sRequestedExitState = requestedState;
}

bool MainRunner::DoWeHaveToUpdateGamelist(MainRunner::ExitState state)
{
  switch(state)
  {
    case ExitState::Quit:
    case ExitState::NormalReboot:
    case ExitState::Shutdown:
    case ExitState::Relaunch: return true;
    case ExitState::RelaunchNoUpdate:
    case ExitState::FatalError:
    case ExitState::FastReboot:
    case ExitState::FastShutdown: break;
  }
  return false;
}

void MainRunner::FileSystemWatcherNotification(FileSystemWatcher& source, const Path& path, EventType event, const DateTime& time)
{
  (void)source;
  (void)time;

  if (path == sQuitNow)
    event = event | EventType::None;

  if (mPendingExit == PendingExit::None)
  {
    if (((event & EventType::Create) != 0) && (path == sQuitNow))
      mPendingExit = PendingExit::MustExit;
  }
}

void MainRunner::HeadphonePluggedIn(BoardType board)
{
  (void)board;
  { LOG(LogInfo) << "[Audio] Headphones plugged!"; }
  Board::Instance().HeadphonePlugged();
}

void MainRunner::HeadphoneUnplugged(BoardType board)
{
  (void)board;
  { LOG(LogInfo) << "[Audio] Headphones unplugged!"; }
  Board::Instance().HeadphoneUnplugged();
}

void MainRunner::ResetButtonPressed(BoardType board)
{
  (void)board;
  if (IsApplicationRunning())
  {
    // The application is running and is on screen.
    // Display little window to notify the user we are going to reset
    { LOG(LogDebug) << "[MainRunner] Reset Button Pressed : reseting"; }
    mApplicationWindow->pushGui((new GuiWaitLongExecution<HardwareTriggeredSpecialOperations, bool>(*mApplicationWindow, *this))
                                      ->Execute(HardwareTriggeredSpecialOperations::Reset, _("Restarting.")));
  } else {
    // Something is running (game, demo, kodi)
    Files::SaveFile(Path(sStopDemo), String());
    { LOG(LogDebug) << "[MainRunner] Reset Button Pressed in game : exiting subprocesses"; }
    ProcessTree::TerminateAll(1000);
  }
}

void MainRunner::UnderVoltage(BoardType board)
{
  (void)board;
  { LOG(LogInfo) << "[MainRunner] Undervoltage popup."; }
  String message = _("An undervoltage has been detected, the system may slow down.\n");
  String suffix;
  switch(board)
  {
    case BoardType::Pi500: suffix = " 500."; break;
    case BoardType::Pi400: suffix = " 400."; break;
    case BoardType::Pi5: suffix = " 5."; break;
    case BoardType::Pi4: suffix = " 4."; break;
    case BoardType::UndetectedYet:
    case BoardType::Unknown:
    case BoardType::Pi0:
    case BoardType::Pi02:
    case BoardType::Pi1:
    case BoardType::Pi2:
    case BoardType::Pi3:
    case BoardType::Pi3plus:
    case BoardType::PiUnknown:
    case BoardType::OdroidAdvanceGo:
    case BoardType::OdroidAdvanceGoSuper:
    case BoardType::OdroidUnknown:
    case BoardType::RG351V:
    case BoardType::RG351P:
    case BoardType::RG353P:
    case BoardType::RG353V:
    case BoardType::RG353M:
    case BoardType::RG503:
    case BoardType::AnbernicUnknown:
    case BoardType::PCx86:
    case BoardType::PCx64:
    case BoardType::SteamDeckLCD:
    case BoardType::SteamDeckOLED:
    default: suffix = "."; break;

  }
  if(Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::RGBJamma)
    message.Append(_("We recommend adjusting your JAMMA cabinet power supply to increase the voltage to between 5.05V and 5.2V"));
  else if (BoardTypeUtil::IsRaspberryPi(Board::Instance().GetBoardType()))
    message.Append(_("We recommend that you purchase an official USB-C power supply designed for your Raspberry Pi").Append(suffix));

  mApplicationWindow->InfoPopupAdd(new GuiInfoPopup(*mApplicationWindow, message, 15, PopupType::Warning));
}

void MainRunner::TemperatureAlert(BoardType board)
{
  (void)board;
  { LOG(LogInfo) << "[MainRunner] Temperature popup."; }
  String message = _("The temperature of your system is high.\nThe system may slow down. Try cooling your Raspberry Pi with a fan or disable overclock if it's enabled.");
  mApplicationWindow->InfoPopupAdd(new GuiInfoPopup(*mApplicationWindow, message, 15, PopupType::Warning));
}

void MainRunner::PowerButtonPressed(BoardType board, int milliseconds)
{
  (void)board;
  if (IsApplicationRunning())
  {
    // The application is running and is on screen.
    // Display little window to notify the user
    if (milliseconds < sPowerButtonThreshold)
    {
      // Only if supported. Otherwise does nothing
      { LOG(LogDebug) << "[MainRunner] Short Power Button Press: standby"; }
      if (Board::Instance().HasSuspendResume())
        mApplicationWindow->pushGui((new GuiWaitLongExecution<HardwareTriggeredSpecialOperations, bool>(*mApplicationWindow, *this))
                                      ->Execute(HardwareTriggeredSpecialOperations::Suspend, _("Entering standby...")));
    }
    else
    {
      { LOG(LogDebug) << "[MainRunner] Long Power Button Press: shutting down"; }
      mApplicationWindow->pushGui((new GuiWaitLongExecution<HardwareTriggeredSpecialOperations, bool>(*mApplicationWindow, *this))
                                      ->Execute(HardwareTriggeredSpecialOperations::PowerOff, _("Powering off.")));
    }
    return;
  }

  // The application is not Running, execute orders immediately
  if (milliseconds < sPowerButtonThreshold)
  {
    // Priority on Suspend. If not available, get out of the game.
    if (Board::Instance().HasSuspendResume())
    {
      { LOG(LogDebug) << "[MainRunner] Power Button short pressed while running game: suspending"; }
      Board::Instance().Suspend();
    }
    else
    {
      { LOG(LogDebug) << "[MainRunner] Power Button short pressed while running game: exiting game"; }
      ProcessTree::TerminateAll(1000);
    }
  }
  else
  {
    { LOG(LogDebug) << "[MainRunner] Power Button long pressed while running game: shutting down"; }
    Files::SaveFile(Path(sQuitNow), String());
    Files::SaveFile(Path(sStopDemo), String());
    // Gracefuly qui emulators and all the call chain
    ProcessTree::TerminateAll(1000);
    // Quit
    RequestQuit(ExitState::Shutdown);
  }
}

void MainRunner::Resume(BoardType board)
{
  (void)board;
  // so... Waking up :)
  if (mApplicationWindow != nullptr && IsApplicationRunning())
    mApplicationWindow->pushGui((new GuiWaitLongExecution<HardwareTriggeredSpecialOperations, bool>(*mApplicationWindow, *this))
                                ->Execute(HardwareTriggeredSpecialOperations::Resume, _("Waking up!")));
  Board::Instance().PostResumeActions();
}

bool MainRunner::Execute(GuiWaitLongExecution<HardwareTriggeredSpecialOperations, bool>& from,
                         const HardwareTriggeredSpecialOperations& parameter)
{
  (void)from;
  switch(parameter)
  {
    case HardwareTriggeredSpecialOperations::PowerOff:
    case HardwareTriggeredSpecialOperations::Reset:
    {
      mApplicationWindow->DoWake();
      Thread::Sleep(1000); // Just sleep one second
      break;
    }
    case HardwareTriggeredSpecialOperations::Suspend:
    case HardwareTriggeredSpecialOperations::Resume:
    default: Thread::Sleep(1000); // Just sleep one second
  }
  return false; // unused
}

void MainRunner::Completed(const HardwareTriggeredSpecialOperations& parameter, const bool& result)
{
  (void)result;
  switch(parameter)
  {
    case HardwareTriggeredSpecialOperations::Suspend:
    {
      // Here is a little trick to erase the window from the screen before suspending
      // To show the user we're soon suspending the hardware, just display the last screen in half luminosity
      mApplicationWindow->Update(20);
      mApplicationWindow->RenderAll(true);

      // This method won't return until wake up
      Board::Instance().Suspend();
      break;
    }
    case HardwareTriggeredSpecialOperations::PowerOff:
    {
      // Bye bye :)
      RequestQuit(ExitState::Shutdown);
      break;
    }
    case HardwareTriggeredSpecialOperations::Reset:
    {
      RequestQuit(ExitState::NormalReboot);
      break;
    }
    case HardwareTriggeredSpecialOperations::Resume:
    {
      // Set audio output since headphone may have been plugged/unplugged
      String output = RecalboxConf::Instance().GetAudioOuput();
      AudioScheduler::Instance().Deactivate();
      AudioController::Instance().Refresh();
      AudioController::Instance().SetDefaultPlayback(output);
      AudioScheduler::Instance().Reactivate();
    }
    default: break;
  }
}

void MainRunner::VolumeDecrease(BoardType board, float percent)
{
  (void)board;

  int value = RecalboxConf::Instance().GetAudioVolume() - (int)(100 * percent);
  value = Math::clampi(value, 0, 100);
  value = (value / 10) * 10;
  AudioController::Instance().SetVolume(value);
  RecalboxConf::Instance().SetAudioVolume(value);
  RecalboxConf::Instance().Save();
}

void MainRunner::VolumeIncrease(BoardType board, float percent)
{
  (void)board;

  int value = RecalboxConf::Instance().GetAudioVolume() + (int)(100 * percent);
  value = Math::clampi(value, 0, 100);
  value = (value / 10) * 10;
  AudioController::Instance().SetVolume(value);
  RecalboxConf::Instance().SetAudioVolume(value);
  RecalboxConf::Instance().Save();
}

void MainRunner::BrightnessDecrease(BoardType board, float percent)
{
  (void)percent;
  (void)board;
  int value = RecalboxConf::Instance().GetBrightness() - 1;
  value = Math::clampi(value, 0, 8);
  Board::Instance().SetBrightness(value);
  RecalboxConf::Instance().SetBrightness(value);
  RecalboxConf::Instance().Save();
}

void MainRunner::BrightnessIncrease(BoardType board, float percent)
{
  (void)percent;
  (void)board;
  int value = RecalboxConf::Instance().GetBrightness() + 1;
  value = Math::clampi(value, 0, 8);
  Board::Instance().SetBrightness(value);
  RecalboxConf::Instance().SetBrightness(value);
  RecalboxConf::Instance().Save();
}

void MainRunner::RomPathAdded(const DeviceMount& device)
{
  String text = _("The device %NAME% containing roms has been plugged in! EmulationStation must relaunch to load new games.")
                     .Replace("%NAME%", device.Name());
  if (device.ReadOnly())
    text.Append(_("\nWARNING: You device may not have been properly unplugged and has consistency errors. As a result, it's been mounted as read-only. You should plug your device in a Window PC and use the repair tool."));
  GuiMsgBox* msgBox = new GuiMsgBox(*mApplicationWindow, text, _("OK"), [] { RequestQuit(ExitState::Relaunch); }, _("LATER"), []{});
  mApplicationWindow->pushGui(msgBox);
}

void MainRunner::RomPathRemoved(const DeviceMount& device)
{
  (void)device;
  String text = _("A device containing roms has been unplugged! EmulationStation must relaunch to remove unavailable games.");
  GuiMsgBox* msgBox = new GuiMsgBox(*mApplicationWindow, text, _("OK"), [] { RequestQuit(ExitState::Relaunch); }, _("LATER"), []{});
  mApplicationWindow->pushGui(msgBox);
}

void MainRunner::NoRomPathFound(const DeviceMount& device)
{
  auto initializeRoms = [this, device]
  {
    MenuModalInitDevice::CreateDeviceInitializer(*mApplicationWindow, device, USBInitializationAction::OnlyRomFolders);
  };

  auto moveShareFolder = [this, device]
  {
    MenuModalInitDevice::CreateDeviceInitializer(*mApplicationWindow, device, USBInitializationAction::CompleteShare);
  };

  String text = _("The USB device %NAME% with no rom folder and no share folder has been plugged in! Would you like to initialize this device?");
  text.Append('\n')
      .Append(_("• Choose '%INIT%' to create only all the rom folders")).Append('\n')
      .Append(_("• Choose '%MOVE%' to copy all the current share to the new device, automatically switch to this device, and reboot")).Append('\n')
      .Append(_("• Or just chose '%CANCEL%' to do nothing with this new device"))
      .Replace("%NAME%", device.Name())
      .Replace("%INIT%", _("INITIALIZE"))
      .Replace("%MOVE%", _("MOVE SHARE"))
      .Replace("%CANCEL%", _("CANCEL"));
  GuiMsgBox* msgBox = new GuiMsgBox(*mApplicationWindow, text, _("INITIALIZE"), initializeRoms,
                                                               _("MOVE SHARE"), moveShareFolder,
                                                               _("CANCEL"), nullptr, ::Alignment::CenterLeft);
  mApplicationWindow->pushGui(msgBox);
}

void MainRunner::PlaylistReady()
{
  // A patron track just landed: PlayNextMusic keeps a remote track playing and replaces anything else,
  // so the default musics never have to be waited out
  if (!ViewController::Available() || !AudioScheduler::Available()) return;
  if (ViewController::Instance().CanPlayMusic())
    AudioScheduler::Instance().PlayNextMusic();
}

void MainRunner::PatreonState(PatronAuthenticationResult result, int level, const String& name)
{
  String message;
  switch(result)
  {
    case PatronAuthenticationResult::Patron:
    {
      message = _("Welcome back %NAME%!\nPatron level %LEVEL%\nYou are now connected to your recalbox patron account, and all exclusives features are available!")
                .Replace("%NAME%", name)
                .Replace("%LEVEL%", String(level));
      break;
    }
    case PatronAuthenticationResult::FormerPatron:
    {
      message = _("Hello %NAME%, your private key is linked to a Patreon account which is no longer a Recalbox Patron.\nWe still hope to see you back soon as a Recalbox Patron!\nDelete your private key to suppress this message.")
                .Replace("%NAME%", name);
      break;
    }
    case PatronAuthenticationResult::Invalid:
    {
      message = _("Your private key does not allow to retrieve your Patreon information. Go to recalbox.com/patreon to generate a new valid key!");
      break;
    }
    case PatronAuthenticationResult::NetworkError:
    {
      message = _("Sorry we're not able to retrieve your Patron level because no network is available!");
      break;
    }
    case PatronAuthenticationResult::HttpError:
    case PatronAuthenticationResult::ApiError:
    {
      message = _("We're not able to retrieve your Patron level! Sorry for the inconvenience, we're already working on a fix!");
      break;
    }
    case PatronAuthenticationResult::NoPatron:
    case PatronAuthenticationResult::Unknown:
    default: break;
  }

  if (!message.empty())
    mApplicationWindow->InfoPopupAdd(new GuiInfoPopup(*mApplicationWindow, message, 15, PopupType::Help));
}

void MainRunner::Sdl2EventReceived(const SDL_Event& event)
{
  ProcessSpecialInputs(InputManager::Instance().ManageSDLEvent(mApplicationWindow, event));
}

bool MainRunner::ProcessSpecialInputs(const InputCompactEvent& event)
{
  // setting this high quality code will avoid the mainrunner to manage
  // power buttons on those specific boards.
  // todo: must be cleaned after 8.1
  const BoardType board = Board::Instance().GetBoardType();
  if (board == BoardType::OdroidAdvanceGo || board == BoardType::OdroidAdvanceGoSuper || board == BoardType::Pi400 ||
      board == BoardType::Pi500 || board == BoardType::RG353P || board == BoardType::RG353V || board == BoardType::RG353M ||
      board == BoardType::RG503 || board == BoardType::RG351V || board == BoardType::RG351P ||
      board == BoardType::SteamDeckLCD || board == BoardType::SteamDeckOLED)
    return false;
  const InputEvent& raw = event.RawEvent();
  static DateTime lastPress;
  if (raw.Type() == InputEvent::EventType::Key)
  {
    if (raw.Value() == 1)
    {
      // KEYDOWN
      switch(raw.Id())
      {
      case SDLK_POWER:
      {
        lastPress = DateTime();
        if (Case::CurrentCase().HasShutdownOnOffButton())
        {
          PowerButtonPressed(board, sPowerButtonThreshold);
          return true;
        }
        return false;
      }
      case SDLK_SLEEP: ResetButtonPressed(board); return true;
      default: break;
      }
    }
    else
    {
      if (raw.Id() == SDLK_POWER)
      {
        PowerButtonPressed(board, (DateTime() - lastPress).TotalMilliseconds() );
        return true;
      }
    }
  }
  return false;
}

void MainRunner::ScanComplete()
{
  BiosManager& manager = BiosManager::Instance();
  WindowManager& window = *mApplicationWindow;
  if (manager.Moved() && !manager.MoveError())
    window.displayMessage(_("With regard to the new BIOS folder structure, some of your bios files have been moved automatically to their new path.").Append("\n\n").Append(_("This move is applied only once. No additional operation required.")));
  else if (manager.Moved() && manager.MoveError())
    window.displayMessage(_("With regard to the new BIOS folder structure, some of your bios files have been moved automatically to their new path.").Append("\n\n").Append(_("However, some files failed to move. You should run the BIOS Checker and move some files manually.")));
  else if (manager.MoveError())
    window.displayMessage(_("With regard to the new BIOS folder structure, some of your bios files have been moved automatically to their new path.").Append("\n\n").Append(_("However, all files failed to move. You should:\n- either run the BIOS Checker and move the required files manually.\n- or if your bios files are on a read-only device or remote share, change it to read-write, reboot your recalbox, wait until all files are moved, then protect it again.")));
}

void MainRunner::ShortcutTriggered(IKeyboardShortcut::Shortcut shortcut)
{
  switch(shortcut)
  {
    case Shortcut::Quit:
    {
      SDL_Event quit;
      quit.type = SDL_QUIT;
      SDL_PushEvent(&quit);
      break;
    }
    case Shortcut::Refresh:
    {
      TextureManager::Instance().FreeAllResources();
      ThemeManager::Instance().DoThemeChange(mApplicationWindow, true);
      break;
    }
    case Shortcut::Search:
    {
      //mApplicationWindow->pushGui(new GuiSearch(mApplicationWindow, mApplicationWindow.));
      break;
    }
    case Shortcut::MenuTemplate:
    {
      #if defined(BETA) || defined(DEBUG)
      Path path("/recalbox/share/system/.emulationstation/menu.xml");
      if (path.Exists())
      {
        mApplicationWindow->pushGui(new GuiMsgBox(*mApplicationWindow, "Overwrite existing file? You may loose your previous modifications !", "NO!", nullptr, "YES, OVERWRITE", [this, path] { (void)path.Delete(); ShortcutTriggered(Shortcut::MenuTemplate); } ));
        return;
      }
      String content = Files::LoadFile(Path(":/menu.xml"));
      Files::SaveFile(path, content);
      mApplicationWindow->displayMessage("Menu file saved to " + path.ToString());
      break;
      #endif
    }
    case Shortcut::MenuReload:
    {
      #if defined(BETA) || defined(DEBUG)
      if (mApplicationWindow->HasGui()) mApplicationWindow->displayMessage("Please close all GUIs before reloading menus");
      else
      {
        MenuProvider::Instance().Reload();
        mApplicationWindow->displayMessage("Menu reloaded");
      }
      #endif
    }
  }
}

void MainRunner::ResolveVariableIn(String& string)
{
  Renderer& renderer = Renderer::Instance();
  int height = renderer.RealDisplayHeightAsInt();
  String resolution = height > 920 ? "fhd" :
                      height > 576 ? "hd" :
                      height > 288 ? "vga" : "qvga";
  string.Replace("$crt", HasCrt() ? "yes" : "no")
        .Replace("$jamma", HasJamma() ? "yes" : "no")
        .Replace("$tate", IsTate() ? "yes" : "no")
        .Replace("$tateright", IsTateRight() ? "yes" : "no")
        .Replace("$tateleft", IsTateLeft() ? "yes" : "no")
        .Replace("$overscan", HasCrt() && !HasJamma() ? "yes" : "no")
        .Replace("$resolution", resolution);
}

void MainRunner::SystemGamelistAltered(SystemData* system)
{
  String text = (_F(_("A gamelist file has been altered manually or by an application external to Recalbox.\n"
                      "\n"
                      "In order not to lose any data, this file will be reloaded as soon as you click on the 'update' button.\n"
                      "If several files have been modified when you click on 'update', all the systems concerned will be updated. No reboot is required.")) / system->FullName())();
  mApplicationWindow->pushGui(new GuiMsgBoxScroll(*mApplicationWindow, _("GAMELIST MODIFIED!"), text,
                                                  _("UPDATE"), [this, system]{ MenuModalRefreshGamelist::CreateRefreshGamelistTask(*mApplicationWindow, system->Manager(), system->Manager().GetDirtyGamelistSystemList()); },
                                                  String::Empty, nullptr,
                                                  String::Empty, nullptr, ::Alignment::TopLeft));
}

void MainRunner::SystemRomFolderAltered(SystemData* system)
{
  String text = (_F(_("Changes have been detected in a roms directory on system {0}.\n"
                      "\n"
                      "Wait for your file operations to finish, then click on the 'update' button to update your roms in Recalbox.\n"
                      "No restart is required, and if you've added roms, you'll be able to scrape them at the end of the update.\n"
                      "\n"
                      "If you add roms to several systems, they will all be updated when you click on the 'update' button.")) / system->FullName())();
  mApplicationWindow->pushGui(new GuiMsgBoxScroll(*mApplicationWindow, _("ROM FOLDERS MODIFIED!"), text,
                                                  _("UPDATE"), [this, system]{ MenuModalRescanRoms::CreateRescanRomsTask(*mApplicationWindow, system->Manager(), system->Manager().GetDirtyRomFolderSystemList()); },
                                                  _("LATER"), nullptr,
                                                  String::Empty, nullptr, ::Alignment::TopLeft));
}

void MainRunner::CardPlugged(String romPath, String systemUUID)
{
  if (IsApplicationRunning())
  {
    if (!romPath.empty() && !systemUUID.empty())
    {
      LOG(LogDebug) << "[MainRunner] CardPlugged game " << romPath;
      if (rand() % 40 == 0 && ViewController::Instance().CurrentView() != ViewType::CardReaderConsoleView)
      {
        String message = "The card could not be read.\nThis problem can probably be fixed by blowing on the card's contacts!";
        PopupType icon = PopupType::Help;
        mApplicationWindow->InfoPopupAdd(new GuiInfoPopup(*mApplicationWindow, _S(message), 10, icon));
      }
      else
        mCardReaderGameToRun = romPath;
    }
    else
    {
      String message = "A card has been detected but is not yet associated with a game. Use START menu on a game to associate.";
      PopupType icon = PopupType::Help;
      mApplicationWindow->InfoPopupAdd(new GuiInfoPopup(*mApplicationWindow, _S(message), 10, icon));
    }
  }
  else
  {
    // Exit game and start game
    {
      LOG(LogDebug) << "[MainRunner] CardReader plugged during game";
      ProcessTree::TerminateAll(2000);
      mCardReaderGameToRun = romPath;
    }
  }
}

void MainRunner::CardUnplugged()
{
  if (IsApplicationRunning())
    { LOG(LogDebug) << "[MainRunner] CardReader unplugged during frontend"; }
  else
  {
    { LOG(LogDebug) << "[MainRunner] CardReader unplugged during game"; }
    ProcessTree::TerminateAll(2000);
  }
}

