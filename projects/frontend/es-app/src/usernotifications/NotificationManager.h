//
// Created by Bkg2k on 10/03/2020.
//
#pragma once

#include <utils/cplusplus/StaticLifeCycleControler.h>
#include <utils/storage/Queue.h>
#include <utils/storage/MessageFactory.h>
#include <systems/SystemData.h>
#include <mqtt/MqttClient.h>
#include <utils/json/JSONBuilder.h>
#include <utils/os/system/Signal.h>
#include <utils/os/system/Thread.h>
#include "ScriptOutputListenerInterface.h"
#include "ScriptAttributes.h"
#include "Notifications.h"
#include "ScriptDescriptor.h"

class NotificationManager : public StaticLifeCycleControler<NotificationManager>
                          , private Thread
{
  private:
    static constexpr const char* sTempStdout = "/tmp/script-stdout.fifo";
    static constexpr const char* sTempStderr = "/tmp/script-stderr.fifo";

    /*!
     * @brief Script data
     */
    struct ScriptData
    {
      Path             mPath;      //!< Script path
      Notification     mFilter;    //!< Bitflag of notifications this script must reply to
      ScriptAttributes mAttribute; //!< Script attributes
    };

    //! Shortcut :)
    typedef std::vector<ScriptData> ScriptList;
    //! Shortcut 2 :)
    typedef std::vector<const ScriptData*> RefScriptList;

    /*!
     * @brief Hold a notification request
     */
    struct NotificationRequest
    {
      //! Action parameters
      String mActionParameters;
      //! System
      const SystemData* mSystemData;
      // Game
      const FileData* mFileData;
      // Action
      Notification mAction;

      /*!
       * @brief Default constructor
       */
      NotificationRequest()
        : mActionParameters(),
          mSystemData(nullptr),
          mFileData(nullptr),
          mAction(Notification::None)
      {
      }

      /*!
       * @brief Sata data
       * @param systemData System
       * @param fileData Game
       * @param action Action
       * @param actionParameters Optional action parameters
       */
      void Set(const SystemData* systemData, const FileData* fileData, Notification action, const String& actionParameters)
      {
        mActionParameters = actionParameters;
        mSystemData = systemData;
        mFileData = fileData;
        mAction = action;
      }

      NotificationRequest& operator =(const NotificationRequest& source)
      {
        if (&source != this)
        {
          mActionParameters = source.mActionParameters;
          mSystemData = source.mSystemData;
          mFileData = source.mFileData;
          mAction = source.mAction;
        }
        return *this;
      }

      /*!
       * @brief Inequality operator
       * @param compareTo Structure to compare against
       * @return True if at least one field is not equal
       */
      bool operator !=(const NotificationRequest& compareTo) const
      {
        return ((mAction != compareTo.mAction) ||
                (mSystemData != compareTo.mSystemData) ||
                (mFileData != compareTo.mFileData) ||
                (mActionParameters != compareTo.mActionParameters));
      }
    };

    //! Script folder
    static constexpr const char* sScriptPath = "/recalbox/share/userscripts";
    //! Manual Script folder
    static constexpr const char* sManualScriptPath = "/recalbox/share/userscripts/manual";

    //! MQTT Topic - event only
    static constexpr const char* sEventTopic     = "Recalbox/EmulationStation/Event";
    //! MQTT Topic - complete event w/ data in JSON form
    static constexpr const char* sEventJsonTopic = "Recalbox/EmulationStation/EventJson";

    //! Virtual devices - events
    static constexpr const char* sKeyboardEventTopic = "Recalbox/VirtualDevices/Keyboard";
    static constexpr const char* sGamePadEventTopic = "Recalbox/VirtualDevices/GamePad";
    static constexpr const char* sTouchPadEventTopic = "Recalbox/VirtualDevices/TouchPad";

    // MQTT client
    MqttClient mMQTTClient;

    //! Permanent scripts PID
    static HashMap<String, pid_t> sPermanentScriptsPID;

    //! Status file
    static const Path sStatusFilePath;

    //! All automatic scripts
    ScriptList mScriptList;

    //! Last JSON event
    JSONBuilder mLastJSONEvent;

    //! All manual scripts
    ScriptList mManualScriptList;

    //! Request provider
    MessageFactory<NotificationRequest> mRequestProvider;
    //! Request synchronized queue
    ::Queue<NotificationRequest*> mRequestQueue;
    //! Queue syncer'
    Mutex mSyncer;
    //! Thread signal
    Signal mSignal;
    //! Previous data
    NotificationRequest mPreviousRequest;

    //! Environement
    char** mEnvironment;

    //! In-process flag
    volatile bool mProcessing;

    /*!
     * @brief Convert an Action into a string
     * @param action Action to convert
     * @return Converted string
     */
    static const char* ActionToString(Notification action);

    /*!
     * @brief Convert a string into an Action
     * @param action String to convert
     * @return Converted Action (Action::None if the conversion fails)
     */
    static Notification ActionFromString(const String& action);

    /*!
     * @brief Extract notifications bitflag from file name.
     * notifications must be eclosed by [] and comma separated
     * Case insensitive
     * @param path Path to extract notifications from
     * @return Notifications bitflag
     */
    static Notification ExtractNotificationsFromPath(const Path& path);

    /*!
     * @brief Extract attributes from file name. Case insensitive
     * @param path file path
     * @return Script Attributes
     */
    static ScriptAttributes ExtractAttributesFromPath(const Path& path);

    /*!
     * @brief Load all available scripts
     */
    void LoadScriptList();

    /*!
     * @brief Load all available scripts
     */
    bool LoadManualScriptList();

    /*!
     * @brief Get a filtered list from all available list
     * @param action Action to filter
     * @return Filtered list
     */
    RefScriptList FilteredScriptList(Notification action);

    /*!
     * @brief Update EmulationStation status file
     * @param system Target System (may be null)
     * @param game Target game (may be null)
     * @param play True if the target game is going to be launched
     * @param demo True if the target game is going ot be launched as demo
     */
    void Notify(const SystemData* system, const FileData* game, Notification action, const String& actionParameters);

    /*!
     * @brief Run all script associated to the given action
     * @param action Action to filter scripts with
     * @param param Optional action parameter
     */
    void RunScripts(Notification action, const String& param);

    /*!
     * @brief Run the target using the given arguments.
     * The target is run aither natively or using python or sh regarding the target extension
     * @param target executable/scriupt to run
     * @param arguments arguments passed to the target
     * @param attributes Script attributes
     * @param interface Listening interface for manual scripts
     */
    void RunProcess(const Path& target, const String::List& arguments, ScriptAttributes attributes, ScriptOutputListenerInterface* interface);

    /*!
     * @brief Build es_state.info Common information into output string
     * @param output Output string
     * @param system System or nullptr
     * @param game Game or nullptr
     * @param action Notification
     * @param actionParameters Notification parameters or empty string
     */
    static void BuildStateCommons(String& output, const SystemData* system, const FileData* game, Notification action, const String& actionParameters);

    /*!
     * @brief Build es_state.info game information into output string
     * @param output Output string
     * @param game Game or nullptr
     * @param action Notification
     */
    static void BuildStateGame(String& output, const FileData* game, Notification action);

    /*!
     * @brief Build es_state.info system information into output string
     * @param output Output string
     * @param game System or nullptr
     * @param action Notification
     */
    static void BuildStateSystem(String& output, const SystemData* system, Notification action);

    /*!
     * @brief Build es_state.info compatibility key/values
     * @param output Output string
     */
    static void BuildStateCompatibility(String& output, Notification action);

    /*!
     * @brief Check path extension and check if the extension is valid or not
     * @param path Path to check extension
     * @return True if the path has a valid extension
     */
    static bool HasValidExtension(const Path& path);

    /*!
     * @brief Build a JSON packet from the given request
     * @param request Request to process
     * @return JSon packet
     */
    static JSONBuilder BuildJsonPacket(const NotificationRequest& request);

    /*
     * Thread implementation
     */

    //! Stop the thread
    void Break() override;

    //! Main runner
    void Run() override;

  public:
    /*!
     * @brief Default constructor
     */
    explicit NotificationManager(char* environement[]);

    //! Destructor
    ~NotificationManager() override;

    //! Wait for the queue to be empty
    void WaitCompletion();

    /*!
     * @brief Start Kodi notification
     */
    void NotifyKodi() { Notify(nullptr, nullptr, Notification::RunKodi, ""); }

    /*!
     * @brief Update EmulationStation status file with game information
     * @param game Target game
     * @param action Action to notify
     */
    void Notify(const FileData& game, Notification action) { Notify(&game.System(), &game, action, game.RomPath().ToString()); }

    /*!
     * @brief Update EmulationStation status file with system information
     * @param system Target system
     * @param action Action to notify
     */
    void Notify(const SystemData& system, Notification action) { Notify(&system, nullptr, action, system.FullName()); }

    /*!
     * @brief
     * @param action Action to notify
     * @param actionParameters Optional action parameters
     */
    void Notify(Notification action, const String& actionParameters) { Notify(nullptr, nullptr, action, actionParameters); }

    /*!
     * @brief
     * @param action Action to notify
     */
    void Notify(Notification action) { Notify(nullptr, nullptr, action, String()); }

    JSONBuilder LastJSONEvent()
    {
      Mutex::AutoLock locker(mSyncer);
      return mLastJSONEvent;
    }

    /*!
     * @brief Check if there is at least one manual script available
     * @return True if there is at least one manual script available, false otherwise
     */
    bool HasManualScript();

    /*!
     * @brief Get manual script list (file name w/o decorations)
     * @return Script list
     */
    ScriptDescriptorList GetManualScriptList();

    /*!
     * @brief Run a partucular script
     * @param index
     * @return
     */
    bool RunManualScriptAt(int index, ScriptOutputListenerInterface* interface);
};
