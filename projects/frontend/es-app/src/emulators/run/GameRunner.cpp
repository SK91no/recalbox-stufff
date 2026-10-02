//
// Created by bkg2k on 02/09/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include <audio/AudioScheduler.h>
#include <hardware/JvsCab.h>
#include <padtokeyboard/PadToKeyboardManager.h>
#include <usernotifications/NotificationManager.h>
#include <utils/cplusplus/StaticLifeCycleControler.h>
#include <input/InputMapper.h>
#include <utils/Files.h>
#include <sdl2/Sdl2Runner.h>
#include <sdl2/Sdl2Init.h>
#include <MainRunner.h>
#include <hardware/crt/CrtAdapterDetector.h>
#include "GameRunner.h"
#include "Resolutions.h"
#include "ResolutionAdapter.h"
#include "RotationManager.h"
#include <chrono>
#include <malloc.h>

#include "systems/LightGunDatabase.h"

bool GameRunner::sGameIsRunning = false;

IBoardInterface::PerformanceGovernance GameRunner::GetGovernance(const String& core)
{
  static IniFile governanceFile(Path(sGovernanceFile), "GameRunner - PerformanceGovernance", false, false);

  String governance = governanceFile.AsString(core);

  if (governance.empty()) return IBoardInterface::PerformanceGovernance::FullSpeed;
  if (governance == "powersave") return IBoardInterface::PerformanceGovernance::PowerSave;
  if (governance == "ondemand") return IBoardInterface::PerformanceGovernance::OnDemand;
  if (governance == "performance") return IBoardInterface::PerformanceGovernance::FullSpeed;
  { LOG(LogError) << "[Gamelist] Unreconized governance " << governance << " in " << sGovernanceFile; }
  return IBoardInterface::PerformanceGovernance::FullSpeed;
}

int GameRunner::Run(const String& cmd_utf8, bool debug)
{
  static String output("/recalbox/share/system/logs/es_launch_stdout.log");
  static String outerr("/recalbox/share/system/logs/es_launch_stderr.log");

  // Run command
  String cmd(cmd_utf8);
  cmd.Append(" > ").Append(output)
     .Append(" 2> ").Append(outerr);
  int exitcode = system(cmd.data());

  // Get logs
  if (debug)
  {
    Path outPath(output);
    Path errPath(outerr);

    static constexpr int sLogSizeLimit = 2 << 20; // 2Mb

    // stdout
    if (outPath.Size() > sLogSizeLimit)
    {
      long long size = outPath.Size();
      String start = Files::LoadFile(outPath, 0, sLogSizeLimit / 2);
      String stop = Files::LoadFile(outPath, size - (sLogSizeLimit / 2), sLogSizeLimit / 2);
      { LOG(LogInfo) << "[Run] Configgen Output:\n" << start << "\n...\n" << stop; }
    }
    else
    {
      String content = Files::LoadFile(outPath);
      if (!content.empty()) { LOG(LogInfo) << "[Run] Configgen Output:\n" << content; }
    }

    // stderr
    if (errPath.Size() > sLogSizeLimit)
    {
      long long size = errPath.Size();
      String start = Files::LoadFile(errPath, 0, sLogSizeLimit / 2);
      String stop = Files::LoadFile(errPath, size - (sLogSizeLimit / 2), sLogSizeLimit / 2);
      { LOG(LogInfo) << "[Run] Configgen Errors:\n" << start << "\n...\n" << stop; }
    }
    else
    {
      String content = Files::LoadFile(errPath);
      if (!content.empty()) { LOG(LogInfo) << "[Run] Configgen Errors:\n" << content; }
    }
  }

  // Return state
  return exitcode;
}

String GameRunner::CreateCommandLine(ResolutionAdapter& resolutionAdapter, const FileData& game, const EmulatorData& emulator, const String& core, const GameLinkedData& data, const InputMapper& mapper, bool debug, bool demo)
{
  String command = game.System().Descriptor().Command();
  String video_backend = "default";
  Path path(game.RomPath());
  const String basename = path.FilenameWithoutExtension();
  bool isLighGunGame = lightGunDB.IsLightGunGame(game);
  String controlersConfig = InputManager::Instance().GetMappedDeviceListConfiguration(mapper, isLighGunGame);
  { LOG(LogInfo) << "[Run] Controllers config : " << controlersConfig; }

  EmulatorDescriptor emulator_descriptor = game.System().Descriptor().EmulatorTree().Named(emulator.Emulator());
  // Search for lowest priority core/emulator
  for(int coreIndex = emulator_descriptor.CoreCount(); --coreIndex >= 0; )
    if (emulator_descriptor.Core(coreIndex).Name() == core)
      video_backend = emulator_descriptor.Core(coreIndex).VideoBackend();

  command.Replace("%ROM%", path.MakeEscaped())
         .Replace("%CONTROLLERSCONFIG%", InputManager::Instance().GetMappedDeviceListConfiguration(mapper, isLighGunGame))
         .Replace("%SYSTEM%", game.System().Name())
         .Replace("%BASENAME%", basename)
         .Replace("%ROM_RAW%", path.ToString())
         .Replace("%EMULATOR%", emulator.Emulator())
         .Replace("%RATIO%", game.Metadata().Ratio().empty() ? "auto" : game.Metadata().Ratio())
         .Replace("%NETPLAY%", NetplayOption(game, data.NetPlay()))
         .Replace("%CRT%", BuildCRTOptions(game, emulator, data.Crt(), RotationManager::ShouldRotateGame(game), demo))
         .Replace("%VIDEO_BACKEND%", video_backend);

  if (controllerDB.IsDialGame(game))
    command.Append(" -spinner");
  if (controllerDB.IsTrackballGame(game))
    command.Append(" -trackball");

  if (debug) command.Append(" -verbose");

  command.Append(" -rotation ").Append((int)RotationManager::ShouldRotateGame(game));
  if(RotationManager::ShouldRotateGameControls(game)) command.Append(" -rotatecontrols ");
  if(RotationManager::IsVerticalGame(game)) command.Append(" -verticalgame ");

  // Forced resolution
  Resolutions::SimpleResolution targetResolution { .Width=0, .Height=0 };
  String userVideoMode = RecalboxConf::Instance().GetSystemVideoMode(game.System());
  // Force default 640x480 on Supermodel Raspberry Pi 5
  if (userVideoMode == "default" && emulator.Emulator() == "supermodel" && Board::Instance().IsPi())
  {
    { LOG(LogDebug) << "[Run] Setting default resolution for supermodel on pi5 640x480" ; }
    userVideoMode = "640x480";
  }
  if (resolutionAdapter.AdjustResolution(0, userVideoMode, targetResolution))
    command.Append(" -resolution ").Append(targetResolution.ToString());

  // Allow to load save state slot on game launching
  if (!data.SaveState().SlotNumber().empty())
    command.Append(" -entryslot ").Append(data.SaveState().SlotNumber());

  // launch without patch
  if (data.Patch().DisabledSofpatching())
    command.Append(" -disabledsoftpatching");
  else if (data.Patch().HasPatchPath()
           && data.Patch().PatchPath().Directory() != game.RomPath().Directory())
  {
    // launch with a selected path in xxxxx-patches directory
    const String patchPathEscaped = data.Patch().PatchPath().MakeEscaped();
    if (data.Patch().PatchPath().Extension().ToLowerCaseUTF8() == ".ips")
      command.Append(" -ips ").Append(patchPathEscaped);
    else if (data.Patch().PatchPath().Extension().ToLowerCaseUTF8() == ".bps")
      command.Append(" -bps ").Append(patchPathEscaped);
    else if (data.Patch().PatchPath().Extension().ToLowerCaseUTF8() == ".ups")
      command.Append(" -ups ").Append(patchPathEscaped);
  }
  if(data.SuperGameBoy().ShouldEnable(game.System()))
    command.Append(" -sgb ")
           .Replace("%CORE%", data.SuperGameBoy().Core(game, core));
  else
    command.Replace("%CORE%", core);

  if(data.JammaJVS().ShouldConfigureArcadePanelConfiggen()){
    command.Append(" -jammalayoutp1 ").Append(data.JammaJVS().JammaControlType(game, emulator, 1));
    command.Append(" -jammalayoutp2 ").Append(data.JammaJVS().JammaControlType(game, emulator, 2));
  }

  command.Append(" -systemtype ").Append(SystemDescriptor::ConvertSystemTypeToString(game.System().Descriptor().Type()));

  return command;
}

bool GameRunner::RunGame(ResolutionAdapter& resolutionAdapter, FileData& game, const EmulatorData& emulator, const GameLinkedData& data)
{
  // Automatic running flag management
  GameRunning IAmRunning;

  { LOG(LogInfo) << "[Run] Launching game"; }

  bool debug = RecalboxConf::Instance().GetDebugLogs();
  InputMapper& mapper = InputManager::Instance().Mapper();
  OrderedDevices controllers = InputManager::Instance().GetMappedDeviceList(mapper);
  const String& core = data.NetPlay().NetplayMode() == NetPlayData::Mode::Client ? data.NetPlay().CoreName() : emulator.Core();

  String command = CreateCommandLine(resolutionAdapter, game, emulator, core, data, mapper, debug, false);

  SubSystemPrepareForRun();

  Path path(game.RomPath());
  int exitCode = -1;
  {
    Sdl2Runner sdl2Runner;
    sdl2Runner.Register(SDL_KEYDOWN, &mSdl2Callback);
    sdl2Runner.Register(SDL_KEYUP, &mSdl2Callback);

    PadToKeyboardManager padToKeyboard(controllers, path, sdl2Runner);
    padToKeyboard.StartMapping();
    if (padToKeyboard.IsValid() ||
        game.System().Descriptor().Type() == SystemDescriptor::SystemType::Computer) command.Append(" -nodefaultkeymap");
    NotificationManager::Instance().Notify(game, Notification::RunGame);

    { LOG(LogInfo) << "[Run] Command: " << command; }

    Board::Instance().SetPerformanceGovernance(GetGovernance(core));
    Board::Instance().StartInGameBackgroundProcesses(sdl2Runner);
    (void)fputs("==============================================\n", stdout);

    JvsCab::ArmOutputs(path.FilenameWithoutExtension(), core);

    auto start = std::chrono::steady_clock::now();

    game.System().PauseFileWatcher();
    // Start game thread
    ThreadRunner gameRunner(sdl2Runner, command, debug);
    // Start sdl2 loop
    sdl2Runner.Run();
    exitCode = gameRunner.ExitCode();
    game.System().ResumeFileWatcher();

    auto end = std::chrono::steady_clock::now();
    long gameDuration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

    (void)fputs("==============================================\n", stdout);
    JvsCab::DisarmOutputs();
    Board::Instance().StopInGameBackgroundProcesses(sdl2Runner);
    Board::Instance().SetFrontendPerformanceGovernor();
    NotificationManager::Instance().Notify(game, Notification::EndGame);
    padToKeyboard.StopMapping();

    if (exitCode != 0) { LOG(LogWarning) << "[Run] Non-Zero exit code " << exitCode << " !"; }
    else
    {
      LOG(LogInfo) << "[Run] No error running " << path.ToString();
      // Update time played
      game.Metadata().SetTotalPlayTime(game.Metadata().TotalPlayTime() + (int) gameDuration);
    }
  }

  SubSystemRestore();

  // Update number of times the game has been launched
  game.Metadata().IncPlayCount();

  // Update last played time
  game.Metadata().SetLastPlayedNow();

  // Ensure no font has been created during GL context shutdown
  if (FontManager::Available())
    FontManager::Instance().ClearFontCaches();

  return exitCode == 0;
}

void GameRunner::SubSystemPrepareForRun()
{
  if(mWindowManager != nullptr)
  {
    AudioScheduler::Instance().Deactivate();
    WindowManager::Finalize();
    // Hand back as much RAM as possible to the (potentially memory-hungry) emulator.
    if (TextureManager::IsInstantiated())
      TextureManager::Instance().FreeAllCPUResources();
    // Also drop every non-current gamelist view to free their data/components.
    if (ViewController::IsInstantiated())
      ViewController::Instance().DestroyGameListViews();
    // Instruct memory manager to release all freed memory to the system
    malloc_trim(0);
  }
}

void GameRunner::SubSystemRestore()
{
  if(mWindowManager != nullptr)
  {
    // Reinit
    Sdl2Init::Finalize();
    Sdl2Init::Initialize();
    mWindowManager->ReInitialize();
    mWindowManager->normalizeNextUpdate();
    AudioScheduler::Instance().Reactivate();
    InputManager::Instance().Refresh(mWindowManager, false);
  }
}

bool
GameRunner::DemoRunGame(ResolutionAdapter& resolutionAdapter, const FileData& game, const EmulatorData& emulator, int duration, int infoscreenduration)
{
  // Automatic running flag management
  GameRunning IAmRunning;

  { LOG(LogInfo) << "[Run] Launching game demo..."; }

  InputMapper& mapper = InputManager::Instance().Mapper();
  bool debug = RecalboxConf::Instance().GetDebugLogs();

  String command = CreateCommandLine(resolutionAdapter, game, emulator, emulator.Core(), GameLinkedData(), mapper, debug, true);

  // Add demo stuff
  command.Append(" -demo 1")
         .Append(" -demoduration ").Append(duration)
         .Append(" -demoinfoduration ").Append(infoscreenduration);

  int exitCode = -1;
  {
    Sdl2Runner sdl2Runner;
    sdl2Runner.Register(SDL_KEYDOWN, &mSdl2Callback);

    NotificationManager::Instance().Notify(game, Notification::RunDemo);
    Board::Instance().SetPerformanceGovernance(IBoardInterface::PerformanceGovernance::FullSpeed);
    Board::Instance().StartInGameBackgroundProcesses(sdl2Runner);

    { LOG(LogInfo) << "[Run] Demo command: " << command; }
    ThreadRunner gameRunner(sdl2Runner, command, debug);
    sdl2Runner.Run();
    exitCode = gameRunner.ExitCode();
    Board::Instance().SetPerformanceGovernance(IBoardInterface::PerformanceGovernance::PowerSave);
    NotificationManager::Instance().Notify(game, Notification::EndDemo);
    { LOG(LogInfo) << "[Run] Demo exit code :	" << exitCode; }

    // Ensure no font has been created during GL context shutdown
    if (FontManager::Available())
      FontManager::Instance().ClearFontCaches();

    // Configgen returns an exitcode 0x33 when the user interact with any pad/mouse
    if (exitCode == 0x33)
    {
      { LOG(LogInfo) << "[Run] Exiting demo upon user request"; }
      return true;
    }

    if (exitCode != 0)
    { LOG(LogWarning) << "[Run] ...launch terminated with nonzero exit code " << exitCode << "!"; }

    return false;
  }
}

String GameRunner::NetplayOption(const FileData& game, const NetPlayData& netplay)
{
  switch(netplay.NetplayMode())
  {
    case NetPlayData::Mode::None: break;
    case NetPlayData::Mode::Client:
    {
      String netplayLine("-netplay client -netplay_port ");
      netplayLine.Append(netplay.Port()).Append(" -netplay_ip ").Append(netplay.Ip());
      if (!netplay.PlayerPassword().empty()) netplayLine.Append(" -netplay_playerpassword ").Append('"').Append(netplay.PlayerPassword()).Append('"');
      if (!netplay.ViewerPassword().empty()) netplayLine.Append(" -netplay_viewerpassword ").Append('"').Append(netplay.ViewerPassword()).Append('"');
      if (netplay.IsViewerOnly()) netplayLine.Append(" -netplay_vieweronly");
      if (game.Metadata().RomCrc32() != 0) netplayLine.Append(" -hash ").Append(game.Metadata().RomCrc32AsString());
      return netplayLine;
    }
    case NetPlayData::Mode::Server:
    {
      String netplayLine("-netplay host -netplay_port ");
      netplayLine.Append(netplay.Port());
      if (!netplay.PlayerPassword().empty()) netplayLine.Append(" -netplay_playerpassword ").Append('"').Append(netplay.PlayerPassword()).Append('"');
      if (!netplay.ViewerPassword().empty()) netplayLine.Append(" -netplay_viewerpassword ").Append('"').Append(netplay.ViewerPassword()).Append('"');
      if (game.Metadata().RomCrc32() != 0) netplayLine.Append(" -hash ").Append(game.Metadata().RomCrc32AsString());
      return netplayLine;
    }
  }

  return "";
}

bool GameRunner::RunKodi()
{
  { LOG(LogInfo) << "[System] Attempting to launch kodi..."; }
  InputMapper& mapper = InputManager::Instance().Mapper();
  SubSystemPrepareForRun();

  String command = "configgen -system kodi -rom '' " + InputManager::Instance().GetMappedDeviceListConfiguration(mapper, false);
  // Forced resolution
  Resolutions::SimpleResolution targetResolution { .Width=0, .Height=0 };
  const ICrtInterface& crtBoard = Board::Instance().CrtBoard();
  String kodiVideoMode = RecalboxConf::Instance().GetKodiVideoMode();
  ResolutionAdapter().AdjustResolution(0, kodiVideoMode, targetResolution, false);

  if (crtBoard.IsCrtAdapterAttached())
  {
    const bool is15Khz = crtBoard.GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHz15;
    if(is15Khz){
      if(kodiVideoMode == "default" && crtBoard.HasInterlacedSupport()){
        if(crtBoard.MustForce50Hz()){
          command.Append(" -resolution ").Append("768x576i");
        }else {
          command.Append(" -resolution ").Append("640x480i");
        }
      } else {
        // Custom resolution for CRT, we must add the resolution type
        command.Append(" -resolution ").Append(targetResolution.ToString());
        command.Append(targetResolution.Height > 288 ? "i":"p");
      }
    } else {
      command.Append(" -resolution ").Append("640x480p");
    }
  } else {
    { LOG(LogInfo) << "[Run] Kodi resolution: " << targetResolution.ToString(); }
    command.Append(" -resolution ").Append(targetResolution.ToString());
  }

  NotificationManager::Instance().NotifyKodi();

  bool debug = RecalboxConf::Instance().GetDebugLogs();
  int exitCode = -1;
  {
    Sdl2Runner sdl2Runner;
    sdl2Runner.Register(SDL_KEYDOWN, &mSdl2Callback);
    Board::Instance().StartInGameBackgroundProcesses(sdl2Runner);

    { LOG(LogInfo) << "[Run] Launching Kodi with command: " << command; }

    ThreadRunner gameRunner(sdl2Runner, command, debug);

    sdl2Runner.Run();
    exitCode = gameRunner.ExitCode();

    if (WIFEXITED(exitCode))
    {
      exitCode = WEXITSTATUS(exitCode);
    }
  }

  SubSystemRestore();

  // handle end of kodi
  switch (exitCode)
  {
    case 10: { MainRunner::RequestQuit(MainRunner::ExitState::NormalReboot); return true; }
    case 11: { MainRunner::RequestQuit(MainRunner::ExitState::Shutdown);     return true; }
    default: break;
  }

  // Ensure no font has been created during GL context shutdown
  if (FontManager::Available())
    FontManager::Instance().ClearFontCaches();

  return exitCode == 0;
}

String GameRunner::BuildCRTOptions(const FileData& game, const EmulatorData& emulator, const CrtData& data, const RotationType rotation, const bool demo)
{
  (void)demo;
  (void)rotation;
  String result;

  const ICrtInterface& crtBoard = Board::Instance().CrtBoard();
  if (crtBoard.IsCrtAdapterAttached())
  {
    if (data.GameIsHD(game, emulator))
      result.Append(" -hdgame ");
    result.Append(" -crtadaptor ").Append(crtBoard.ShortName());
    result.Append(" -crtsuperrez ").Append(CrtConf::Instance().GetSystemCRTSuperrez());
    result.Append(" -crtresolutiontype ").Append(CrtData::CrtModeToString(data.GetCrtMode(game, emulator)));
    result.Append(" -crtscreentype ").Append(ICrtInterface::HorizontalFrequencyToString(crtBoard.GetHorizontalFrequency()));
    result.Append(" -crtsignaltype ").Append(crtBoard.MustForceComposite() ? CrtData::CrtSignalToString(CrtData::CrtSignal::Composite):  CrtData::CrtSignalToString(data.GetCrtSignal()));
    result.Append(" -crtscanlines ").Append(CrtConf::CrtScanlinesFromEnum(data.Scanlines(game.System())));
    result.Append(" -crtvideostandard ");
    // Force pal if switch 50hz
    if (crtBoard.MustForce50Hz())
    {
      result.Append("pal");
    } else
    {
      switch (data.VideoStandard())
      {
        case CrtData::CrtVideoStandard::PAL:   result.Append("pal");  break;
        case CrtData::CrtVideoStandard::NTSC:  result.Append("ntsc"); break;
        case CrtData::CrtVideoStandard::AUTO:
        default:                               result.Append("auto"); break;
      }
    }

    result.Append(" -crtregion ");
    // Force eu if switch 50hz
    if (crtBoard.MustForce50Hz())
    {
      result.Append("eu");
    }
    else
    {
      switch (data.Region())
      {
        case CrtData::CrtRegion::EU:
          result.Append("eu");
          break;
        case CrtData::CrtRegion::US:
          result.Append("us");
          break;
        case CrtData::CrtRegion::JP:
          result.Append("jp");
          break;
        case CrtData::CrtRegion::AUTO:
        default:
          result.Append("auto");
          break;
      }
    }
  }

  return result;
}

void GameRunner::ThreadRunner::Run()
{
  mExitCode = WEXITSTATUS(GameRunner::Run(mCommand, mDebug));
  mRunner.Stop();
}
