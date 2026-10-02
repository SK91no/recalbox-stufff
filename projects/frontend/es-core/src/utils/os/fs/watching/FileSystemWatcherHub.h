//
// Created by bkg2k on 23/03/25.
//
#pragma once

#include <utils/storage/HashMap.h>
#include "FileSystemWatcher.h"
#include "IFileSystemWatcherNotification.h"
#include "utils/os/system/Thread.h"
#include "utils/os/system/Mutex.h"
#include "utils/sync/SyncMessageSender.h"
#include "IUnregisterFileWatcher.h"

class FileSystemWatcherHub : private Thread
                           , private ISyncMessageReceiver<void>
                           , public IUnregisterFileWatcher
{
  public:
    FileSystemWatcherHub(const String& name);

    ~FileSystemWatcherHub() override;

    void AddWatcher(FileSystemWatcher& watcher, IFileSystemWatcherNotification& notifier);

    void RemoveWatcher(FileSystemWatcher& watcher);

    void RemoveWatcher(IFileSystemWatcherNotification& notifier);

  private:
    //! Little container for event and notifier
    struct QueuedEvent
    {
      FileSystemEvent event;
      FileSystemWatcher* source;
      IFileSystemWatcherNotification* notifier;
    };
    //! Message sender
    SyncMessageSender<void> mSender;
    //! FileWatcher map
    HashMap<FileSystemWatcher*, IFileSystemWatcherNotification*> mWatchersToNotifiers;
    //! Message queue
    std::queue<QueuedEvent> mQueuedMessage;
    //! Map guardian
    Mutex mGuardian;


    /*
     * Thread implementation
     */

    void Run() final;

    /*
     * ISyncMessageReceiver<void>
     */

    void ReceiveSyncMessage() final;

    /*
     * IUnregisterFileWatcher
     */

    void UnregisterFileWatcher(FileSystemWatcher* fileWatcherToUnregister) final { RemoveWatcher(*fileWatcherToUnregister); }
};
