//
// Created for feat/webmanager-sync
//
// Bridge between the (multi-threaded) REST web server and the main UI thread.
//
// The web server runs in its own thread; it cannot call RecalboxConf setters directly because
// they fire UI notifications (DoNotify) that must run on the main thread. This class queues
// configuration changes coming from the web thread and replays them on the main thread through
// the SyncMessage system, going *exclusively* through the typed RecalboxConf setters so that
// every present and future watcher is notified.
//
#pragma once

#include <vector>
#include <utils/String.h>
#include <utils/cplusplus/StaticLifeCycleControler.h>
#include <utils/sync/SyncMessageSender.h>
#include <utils/os/system/Mutex.h>

// Forward declaration
class SystemManager;

class WebManagerSync : public StaticLifeCycleControler<WebManagerSync>
                     , public ISyncMessageReceiver<void>
{
  public:
    /*!
     * @brief Constructor
     * @param systemManager System manager used to resolve system-specific keys
     */
    explicit WebManagerSync(SystemManager& systemManager);

    /*!
     * @brief Queue a global configuration change. Called from the web thread.
     * @param fullKey Full configuration key (e.g. "emulationstation.showhidden")
     * @param value Value to set, in string form
     */
    void PushConfigChange(const String& fullKey, const String& value);

    /*!
     * @brief Queue a system-specific configuration change. Called from the web thread.
     * @param systemName System short name
     * @param shortKey Short key (e.g. "sort", "ignore")
     * @param value Value to set, in string form
     */
    void PushSystemConfigChange(const String& systemName, const String& shortKey, const String& value);

    /*!
     * @brief Queue a configuration key deletion (reset to default). Called from the web thread.
     * @param fullKey Full configuration key to remove
     */
    void PushConfigDelete(const String& fullKey);

  private:
    //! A single queued configuration change
    struct Change
    {
      String Key;    //!< Full key (global change / deletion) or short key (system-specific change)
      String Value;  //!< Value to set (unused for a deletion)
      String System; //!< System short name, empty for a global change
      bool Remove;   //!< True => delete the key, false => set it to Value
    };

    //! System manager, used to resolve system-specific keys
    SystemManager& mSystemManager;
    //! Synchronized message sender (signal only; the payload lives in mPending)
    SyncMessageSender<void> mSender;
    //! Pending changes protector
    Mutex mLocker;
    //! Pending changes queue
    std::vector<Change> mPending;

    /*!
     * @brief Drain and apply all pending changes. Runs on the main thread.
     */
    void ReceiveSyncMessage() override;
};
