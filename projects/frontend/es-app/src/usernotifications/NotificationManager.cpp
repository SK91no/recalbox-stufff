//
// Created by Bkg2k on 10/03/2020.
//

#include <utils/storage/HashMap.h>
#include <utils/Files.h>
#include <systems/SystemData.h>
#include <systems/SystemManager.h>
#include <sys/wait.h>
#include "NotificationManager.h"
#include <spawn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <poll.h>

/*
 * Members
 */
const String eol("\r\n");
const Path NotificationManager::sStatusFilePath("/tmp/es_state.inf");
HashMap<String, pid_t> NotificationManager::sPermanentScriptsPID;

NotificationManager::NotificationManager(char** environment)
  : StaticLifeCycleControler<NotificationManager>("NotificationManager"),
    mMQTTClient("recalbox-emulationstation", nullptr),
    mEnvironment(environment)
  , mProcessing(false)
{
  LoadScriptList();
  Thread::Start("EventNotifier");
}

NotificationManager::~NotificationManager()
{
  Thread::Stop();
}

const char* NotificationManager::ActionToString(Notification action)
{
  switch(action)
  {
    case Notification::None:                 return "none";
    case Notification::Start:                return "start";
    case Notification::Stop:                 return "stop";
    case Notification::Shutdown:             return "shutdown";
    case Notification::Reboot:               return "reboot";
    case Notification::Relaunch:             return "relaunch";
    case Notification::Quit:                 return "quit";
    case Notification::SystemBrowsing:       return "systembrowsing";
    case Notification::GamelistBrowsing:     return "gamelistbrowsing";
    case Notification::RunKodi:              return "runkodi";
    case Notification::RunGame:              return "rungame";
    case Notification::RunDemo:              return "rundemo";
    case Notification::EndGame:              return "endgame";
    case Notification::EndDemo:              return "enddemo";
    case Notification::Sleep:                return "sleep";
    case Notification::WakeUp:               return "wakeup";
    case Notification::ScrapStart:           return "scrapstart";
    case Notification::ScrapStop:            return "scrapstop";
    case Notification::ScrapGame:            return "scrapgame";
    case Notification::ConfigurationChanged: return "configurationchanged";
    case Notification::StartGameClip:        return "startgameclip";
    case Notification::StopGameClip:         return "stopgameclip";
    default: break;
  }
  return "error";
}

Notification NotificationManager::ActionFromString(const String& action)
{
  static HashMap<String, Notification> sStringToAction
  ({
    { "start"               , Notification::Start                },
    { "stop"                , Notification::Stop                 },
    { "shutdown"            , Notification::Shutdown             },
    { "reboot"              , Notification::Reboot               },
    { "relaunch"            , Notification::Relaunch             },
    { "quit"                , Notification::Quit                 },
    { "systembrowsing"      , Notification::SystemBrowsing       },
    { "gamelistbrowsing"    , Notification::GamelistBrowsing     },
    { "runkodi"             , Notification::RunKodi              },
    { "rungame"             , Notification::RunGame              },
    { "rundemo"             , Notification::RunDemo              },
    { "endgame"             , Notification::EndGame              },
    { "enddemo"             , Notification::EndDemo              },
    { "sleep"               , Notification::Sleep                },
    { "wakeup"              , Notification::WakeUp               },
    { "scrapstart"          , Notification::ScrapStart           },
    { "scrapstop"           , Notification::ScrapStop            },
    { "scrapgame"           , Notification::ScrapGame            },
    { "configurationchanged", Notification::ConfigurationChanged },
    { "startgameclip"       , Notification::StartGameClip        },
    { "stopgameclip"        , Notification::StopGameClip         },
  });

  if (!sStringToAction.contains(action))
    return Notification::None;

  return sStringToAction[action];
}

ScriptAttributes NotificationManager::ExtractAttributesFromPath(const Path& path)
{
  const String scriptName = path.FilenameWithoutExtension().LowerCase();
  // Get attributes
  ScriptAttributes result = ScriptAttributes::None;
  if (scriptName.Contains("(sync)")) result |= ScriptAttributes::Synced;
  if (scriptName.Contains("(progress)")) result |= ScriptAttributes::ReportProgress | ScriptAttributes::Synced;
  if (scriptName.Contains("(permanent)")) result = ScriptAttributes::Permanent;

  return result;
}

Notification NotificationManager::ExtractNotificationsFromPath(const Path& path)
{
  // Extract events between [ and ] in filename
  const String scriptName = path.FilenameWithoutExtension().LowerCase();
  int start = scriptName.Find('[');
  int stop = scriptName.Find(']');

  if (((start | stop) < 0) || (stop - start <= 1)) return (Notification)-1;

  Notification result = Notification::None;
  // Split events
  String::List events = scriptName.SubString(start + 1, stop - start - 1).Split(',');
  // Extract notifications
  for(const String& event : events)
    result |= ActionFromString(event);
  return result;
}

void NotificationManager::LoadScriptList()
{
  // Load event scripts
  Path scriptsFolder(sScriptPath);
  Path::PathList scripts = scriptsFolder.GetDirectoryContent();
  mScriptList.clear();

  for(const Path& path : scripts)
    if (path.IsFile() && !path.IsHidden())
      if (HasValidExtension(path))
      {
        ScriptAttributes attributes = ExtractAttributesFromPath(path);
        if (hasFlag(attributes, ScriptAttributes::Permanent))
        {
          RunProcess(path, {}, attributes, nullptr);
          { LOG(LogDebug) << "[Script] Run permanent UserScript: " << path.ToString(); }
        }
        else
        {
          mScriptList.push_back({ .mPath=path, .mFilter=ExtractNotificationsFromPath(path), .mAttribute=attributes });
          { LOG(LogDebug) << "[Script] Scan UserScript: " << path.ToString(); }
        }
      }
}

bool NotificationManager::LoadManualScriptList()
{
  // Load manual scripts
  Path manualScriptsFolder(sManualScriptPath);
  Path::PathList scripts = manualScriptsFolder.GetDirectoryContent();
  mManualScriptList.clear();

  for(const Path& path : scripts)
    if (path.IsFile() && !path.IsHidden())
      if (HasValidExtension(path))
      {
        mManualScriptList.push_back({ .mPath=path, .mFilter=Notification::None, .mAttribute=ExtractAttributesFromPath(path) });
        { LOG(LogDebug) << "[Script] Manual UserScript: " << path.ToString(); }
      }
  return !mManualScriptList.empty();
}

NotificationManager::RefScriptList NotificationManager::FilteredScriptList(Notification action)
{
  RefScriptList result;

  for(const ScriptData& script : mScriptList)
    if ((script.mFilter & action) != 0)
      result.push_back(&script);

  return result;
}

void NotificationManager::RunScripts(Notification action, const String& param)
{
  RefScriptList scripts = FilteredScriptList(action);
  if (scripts.empty()) return; // Nothing to launch

  // Build parameter
  String::List args;
  args.push_back("-action");
  args.push_back(ActionToString(action));
  args.push_back("-statefile");
  args.push_back(sStatusFilePath.ToString());
  if (!param.empty())
  {
    args.push_back("-param");
    args.push_back(param);
  }

  for(const ScriptData* script : scripts)
  {
    // Run!
    RunProcess(script->mPath, args, script->mAttribute, nullptr);
  }
}

JSONBuilder NotificationManager::BuildJsonPacket(const NotificationManager::NotificationRequest& request)
{
  String emulator;
  String core;
  JSONBuilder builder;

  builder.Open();

  // Action
  builder.Field("Action", ActionToString(request.mAction))
         .Field("Parameter", request.mAction == Notification::RunKodi ? "Kodi" : request.mActionParameters)
         .Field("Version", LEGACY_STRING("2.0"));
  // Kodi
  if (request.mAction == Notification::RunKodi)
  {
    builder.OpenObject("System")
           .Field("System", "kodi")
           .Field("SystemId", "kodi")
           .CloseObject();
  }
  // System
  if (request.mSystemData != nullptr)
  {
    builder.OpenObject("System")
           .Field("System", request.mSystemData->FullName())
           .Field("SystemId", request.mSystemData->Name());
    if (!request.mSystemData->IsVirtual())
      if (EmulatorManager::GetDefaultEmulator(*request.mSystemData, emulator, core))
        builder.OpenObject("DefaultEmulator")
               .Field("Emulator", emulator)
               .Field("Core", core)
               .CloseObject();
    builder.CloseObject();
  }
  // Game
  if (request.mFileData != nullptr)
  {
    builder.OpenObject("Game")
           .Field("Game", request.mFileData->Name())
           .Field("GamePath", request.mFileData->RomPath().ToString())
           .Field("IsFolder", request.mFileData->IsFolder())
           .Field("ImagePath", request.mFileData->Metadata().Image().ToString())
           .Field("BoxPath", request.mFileData->Metadata().Box().ToString())
           .Field("VideoPath", request.mFileData->Metadata().Video().ToString())
           .Field("Developer", request.mFileData->Metadata().Developer())
           .Field("Publisher", request.mFileData->Metadata().Publisher())
           .Field("Players", request.mFileData->Metadata().PlayersAsString())
           .Field("Region", request.mFileData->Metadata().RegionAsString())
           .Field("Genre", request.mFileData->Metadata().Genre().Replace(',', ", ", 2).Replace(" ", 2, ' '))
           .Field("GenreId", request.mFileData->Metadata().GenreIdAsString())
           .Field("Favorite", request.mFileData->Metadata().Favorite())
           .Field("Hidden", request.mFileData->Metadata().Hidden())
           .Field("Adult", request.mFileData->Metadata().Adult());
    if (EmulatorManager::GetGameEmulator(*request.mFileData, emulator, core))
      builder.OpenObject("SelectedEmulator")
             .Field("Emulator", emulator)
             .Field("Core", core)
             .CloseObject();
    builder.CloseObject();
  }
  builder.Close();

  return builder;
}

void NotificationManager::BuildStateCommons(String& output, const SystemData* system, const FileData* game, Notification action, const String& actionParameters)
{
  // Build status file
  output.Append("Action=").Append(ActionToString(action)).Append(eol)
        .Append("ActionData=").Append(actionParameters).Append(eol);

  // System
  if (system != nullptr)
    output.Append("System=").Append(system->FullName()).Append(eol)
          .Append("SystemId=").Append(system->Name()).Append(eol);
  else if (action == Notification::RunKodi)
    output.Append("System=kodi").Append(eol)
          .Append("SystemId=kodi").Append(eol);
  else
    output.Append("System=").Append(eol)
          .Append("SystemId=").Append(eol);

  // Permanent game infos
  if (game != nullptr)
    output.Append("Game=").Append(game->Name()).Append(eol)
          .Append("GamePath=").Append(game->RomPath().ToString()).Append(eol)
          .Append("ImagePath=").Append(game->Metadata().Image().ToString()).Append(eol);
  else
    output.Append("Game=").Append(eol)
          .Append("GamePath=").Append(eol)
          .Append("ImagePath=").Append(eol);
}

void NotificationManager::BuildStateGame(String& output, const FileData* game, Notification action)
{
  if (game == nullptr) return;

  output.Append("IsFolder=").Append(game->IsFolder() ? "1" : "0").Append(eol)
        .Append("BoxPath=").Append(game->Metadata().Box().ToString()).Append(eol)
        .Append("VideoPath=").Append(game->Metadata().Video().ToString()).Append(eol)
        .Append("Developer=").Append(game->Metadata().Developer()).Append(eol)
        .Append("Publisher=").Append(game->Metadata().Publisher()).Append(eol)
        .Append("Players=").Append(game->Metadata().PlayersAsString()).Append(eol)
        .Append("Region=").Append(game->Metadata().RegionAsString()).Append(eol)
        .Append("Genre=").Append(game->Metadata().Genre()).Append(eol)
        .Append("GenreId=").Append(game->Metadata().GenreIdAsString()).Append(eol)
        .Append("Favorite=").Append(game->Metadata().Favorite() ? "1" : "0").Append(eol)
        .Append("Hidden=").Append(game->Metadata().Hidden() ? "1" : "0").Append(eol)
        .Append("Adult=").Append(game->Metadata().Adult() ? "1" : "0").Append(eol);

  if (action != Notification::ScrapGame)
  {
    String emulator;
    String core;
    if (EmulatorManager::GetGameEmulator(*game, emulator, core))
      output.Append("Emulator=").Append(emulator).Append(eol).Append("Core=").Append(core).Append(eol);
  }
}

void NotificationManager::BuildStateSystem(String& output, const SystemData* system, Notification action)
{
  if (system == nullptr) return;

  if (!system->IsVirtual() && action != Notification::ScrapGame)
  {
    String emulator;
    String core;
    if (EmulatorManager::GetDefaultEmulator(*system, emulator, core))
      output.Append("DefaultEmulator=").Append(emulator).Append(eol).Append("DefaultCore=").Append(core).Append(eol);
  }
}

void NotificationManager::BuildStateCompatibility(String& output, Notification action)
{
  // Mimic old behavior of "State"
  output.Append("State=");
  switch(action)
  {
    case Notification::RunKodi:
    case Notification::RunGame: output.Append("playing"); break;
    case Notification::RunDemo: output.Append("demo"); break;
    case Notification::None:
    case Notification::Start:
    case Notification::Stop:
    case Notification::Shutdown:
    case Notification::Reboot:
    case Notification::Relaunch:
    case Notification::Quit:
    case Notification::SystemBrowsing:
    case Notification::GamelistBrowsing:
    case Notification::EndGame:
    case Notification::EndDemo:
    case Notification::Sleep:
    case Notification::WakeUp:
    case Notification::ScrapStart:
    case Notification::ScrapStop:
    case Notification::ScrapGame:
    case Notification::ConfigurationChanged:
    case Notification::StartGameClip:
    case Notification::StopGameClip:
    default: output.Append("selected"); break;
  }
}

void NotificationManager::Break()
{
  mSignal.Fire();
}

void NotificationManager::Run()
{
  NotificationRequest* request = nullptr;
  while(IsRunning())
  {
    mSignal.WaitSignal();
    while(IsRunning())
    {
      // Get request
      { Mutex::AutoLock locker(mSyncer); request = !mRequestQueue.Empty() ? mRequestQueue.Pop() : nullptr; }
      if (request == nullptr) break;

      { Mutex::AutoLock locker(mSyncer); mProcessing = true; }

      // Process
      if (*request != mPreviousRequest)
      {
        // Build all
        String output("Version=2.0");
        output.Append(eol);
        BuildStateCommons(output, request->mSystemData, request->mFileData, request->mAction,
                          request->mActionParameters);
        BuildStateGame(output, request->mFileData, request->mAction);
        BuildStateSystem(output, request->mSystemData, request->mAction);
        BuildStateCompatibility(output, request->mAction);
        // Save
        Files::SaveFile(Path(sStatusFilePath), output);
        // MQTT notification
        mMQTTClient.Send(sEventTopic, ActionToString(request->mAction));

        // Build json event
        JSONBuilder json = BuildJsonPacket(*request);
        // MQTT notification
        mMQTTClient.Send(sEventJsonTopic, json);
        //! Store
        mSyncer.Lock();
        mLastJSONEvent = json;
        mSyncer.UnLock();

        // Run scripts
        const String notificationParameter = (request->mFileData != nullptr)
                                             ? request->mFileData->RomPath().ToString()
                                             : ((request->mSystemData != nullptr) ? request->mSystemData->Name()
                                                                                        : request->mActionParameters);
        RunScripts(request->mAction, notificationParameter);

        mPreviousRequest = *request;
      }

      // Recycle
      mRequestProvider.Recycle(request);
    }
    // End processing
    { Mutex::AutoLock locker(mSyncer); mProcessing = false; }
  }
}

void NotificationManager::Notify(const SystemData* system, const FileData* game, Notification action, const String& actionParameters)
{
  // Build new parameter bag
  NotificationRequest* request = mRequestProvider.Obtain();
  request->Set(system, game, action, actionParameters);

  // Push new param bag
  {
    Mutex::AutoLock locker(mSyncer);
    mRequestQueue.Push(request);
    mSignal.Fire();
  }
}

void NotificationManager::RunProcess(const Path& target, const String::List& arguments, ScriptAttributes attributes, ScriptOutputListenerInterface* interface)
{
  if (interface != nullptr)
    interface->ScriptStarts(target, attributes);

  // final argument array
  std::vector<const char*> args;

  String command;

  // Extract extension
  String ext = target.Extension().LowerCase();
  if      (ext == ".sh")  { command = "/bin/sh";          args.push_back(command.data()); }
  else if (ext == ".ash") { command = "/bin/ash";         args.push_back(command.data()); }
  else if (ext == ".py")  { command = "/usr/bin/python";  args.push_back(command.data()); args.push_back("-u"); }
  else if (ext == ".py3") { command = "/usr/bin/python3"; args.push_back(command.data()); args.push_back("-u"); }
  else { command = target.ToString(); }

  args.push_back(target.ToChars());
  for (const String& argument : arguments) args.push_back(argument.c_str());

  { LOG(LogDebug) << "[Script] Run UserScript: " << args; }

  // Push final null
  args.push_back(nullptr);

  if (sPermanentScriptsPID.contains(target.ToString()))
  {
    // Still running?
    if (waitpid(sPermanentScriptsPID[target.ToString()], nullptr, WNOHANG) == 0)
      return;
    // Not running, remove pid
    sPermanentScriptsPID.erase(target.ToString());
  }

  pid_t pid = 0;
  posix_spawnattr_t spawnAttributes;
  posix_spawnattr_init(&spawnAttributes);

  // Init file action to redirect stdout/stderr
  posix_spawn_file_actions_t spawnFileActions;
  posix_spawn_file_actions_init(&spawnFileActions);
  int pipeStdOut[2];
  int pipeStdErr[2];
  if (hasFlag(attributes, ScriptAttributes::ReportProgress))
  {
    pipe(pipeStdOut);
    pipe(pipeStdErr);

    // Close copied read handles
    posix_spawn_file_actions_addclose(&spawnFileActions, pipeStdOut[0]);
    posix_spawn_file_actions_addclose(&spawnFileActions, pipeStdErr[0]);
    // Duplicate copied write handle
    posix_spawn_file_actions_adddup2(&spawnFileActions, pipeStdOut[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&spawnFileActions, pipeStdErr[1], STDERR_FILENO);
    // Close copied write handle
    posix_spawn_file_actions_addclose(&spawnFileActions, pipeStdOut[1]);
    posix_spawn_file_actions_addclose(&spawnFileActions, pipeStdErr[1]);
  }
  // SPAWN !
  int status = posix_spawn(&pid, command.data(), &spawnFileActions, nullptr, (char**)args.data(), mEnvironment);

  if (status != 0) // Error
  {
    { LOG(LogError) << "[Script] Error running " << target.ToString() << " (spawn failed)"; }
    if (interface != nullptr)
      interface->ScriptCompleted(target, attributes, "", true, "Failed to run !");
    return;
  }

  // Wait for child?
  bool error = false;
  String outputContent;
  String errorContent;
  int outputIndex = 0;
  if (hasFlag(attributes, ScriptAttributes::Synced))
  {
    if (hasFlag(attributes, ScriptAttributes::ReportProgress))
    {
      // Close write handle on parent size
      close(pipeStdOut[1]);
      close(pipeStdErr[1]);

      // Open pipes
      struct pollfd polls[2] { { .fd=pipeStdOut[0], .events=POLLIN, .revents=0 }, { .fd=pipeStdErr[0], .events=POLLIN, .revents=0 } };
      for (;;)
      {
        // Read error & output
        while(poll(&polls[0], 2, 20) > 0)
        {
          bool hasRead = false;
          if ((polls[1].revents & POLLIN) != 0)
          {
            if (char c = 0; read(pipeStdErr[0], &c, 1) == 1)
            { errorContent.Append(c); hasRead = true; }
          }
          if ((polls[0].revents & POLLIN) != 0)
          {
            if (char c = 0; read(pipeStdOut[0], &c, 1) == 1)
            {
              if (c == '\n')
              {
                String line(outputContent.data() + outputIndex, outputContent.Count() - outputIndex);
                //printf("%s\n", line.data());
                if (interface != nullptr)
                  interface->ScriptOutputLine(target, attributes, line);
                outputIndex = outputContent.Count() + 1;
              }
              outputContent.Append(c);
              hasRead = true;
            }
          }
          if (!hasRead) break; // Error received
        }

        // Check if the script is still running
        int err = waitpid(pid, &status, WNOHANG);
        if (err < 0)
        {
          { LOG(LogError) << "[Script] Error waiting for " << target.ToString() << " to complete. (waitpid failed)"; }
          error = true;
          break;
        }
        if (err == pid)
          break;
      }

      // Close pipes
      close(pipeStdOut[0]);
      close(pipeStdErr[0]);
    }
    else if (waitpid(pid, &status, 0) != pid)
      { LOG(LogError) << "[Script] Error waiting for " << target.ToString() << " to complete. (waitpid failed)"; }

    // Check status
    if (WEXITSTATUS(status) != 0) // Error
    {
      { LOG(LogError) << "[Script] Running script " << target.ToString() << " returned an error"; }
      if (!errorContent.empty()) errorContent.Append('\n');
      errorContent.Append("Exited with code ").Append(WEXITSTATUS(status));
      error = true;
    }
  }

  if (interface != nullptr)
    interface->ScriptCompleted(target, attributes, outputContent, !errorContent.empty() || error, errorContent);

  // Permanent?
  if (hasFlag(attributes, ScriptAttributes::Permanent))
    sPermanentScriptsPID.insert(target.ToString(), pid);

  posix_spawnattr_destroy(&spawnAttributes);
  posix_spawn_file_actions_destroy(&spawnFileActions);
}

bool NotificationManager::HasValidExtension(const Path& path)
{
  String ext = path.Extension().LowerCase();
  return (ext.empty()) ||
         (ext == ".sh" ) ||
         (ext == ".ash") ||
         (ext == ".py" ) ||
         (ext == ".py3");
}

void NotificationManager::WaitCompletion()
{
  for(;;)
  {
    mSyncer.Lock();
    bool havePendings = !mRequestQueue.Empty();
    mSyncer.UnLock();
    if (!havePendings && !mProcessing) break;
    Thread::Sleep(100);
  }
}

ScriptDescriptorList NotificationManager::GetManualScriptList()
{
  LoadManualScriptList();

  ScriptDescriptorList result;
  int index = 0;
  for(const ScriptData& data : mManualScriptList)
    result.push_back({ .mPath=data.mPath, .mIndex=index++, .mAttribute=data.mAttribute});
  return result;
}

bool NotificationManager::RunManualScriptAt(int index, ScriptOutputListenerInterface* interface)
{
  if ((unsigned int)index < (unsigned int)mManualScriptList.size())
  {
    { LOG(LogError) << "[Script] Running manual script " << mManualScriptList[index].mPath; }
    RunProcess(mManualScriptList[index].mPath, {}, mManualScriptList[index].mAttribute, interface);
    return true;
  }

  { LOG(LogError) << "[Script] Error running " << mManualScriptList[index].mPath << " (index out of range)"; }
  if (interface != nullptr)
    interface->ScriptCompleted(Path::Empty, ScriptAttributes::None, "", true, "Script index out of range !");
  return false;
}

bool NotificationManager::HasManualScript()
{
  if (!mManualScriptList.empty()) return true;
  return LoadManualScriptList();
}
