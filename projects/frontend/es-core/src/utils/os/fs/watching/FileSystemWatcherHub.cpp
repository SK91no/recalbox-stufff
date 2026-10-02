//
// Created by bkg2k on 23/03/25.
//

#include "FileSystemWatcherHub.h"
#include <utils/storage/Array.h>

FileSystemWatcherHub::FileSystemWatcherHub(const String& name)
  : mSender(*this)
{
  Thread::Start(name);
}

FileSystemWatcherHub::~FileSystemWatcherHub()
{
  Mutex::AutoLock locker(mGuardian);
  mWatchersToNotifiers.clear();
}

void FileSystemWatcherHub::AddWatcher(FileSystemWatcher& watcher, IFileSystemWatcherNotification& notifier)
{
  Mutex::AutoLock locker(mGuardian);
  watcher.SetUnregisterInterface(this);
  mWatchersToNotifiers[&watcher] = &notifier;
}

void FileSystemWatcherHub::RemoveWatcher(FileSystemWatcher& watcher)
{
  Mutex::AutoLock locker(mGuardian);
  mWatchersToNotifiers.erase(&watcher);
}

void FileSystemWatcherHub::RemoveWatcher(IFileSystemWatcherNotification& notifier)
{
  Array<FileSystemWatcher*> list;
  Mutex::AutoLock locker(mGuardian);
  for(const auto& it : mWatchersToNotifiers)
    if (it.second == &notifier)
      list.Add(it.first);
  for(FileSystemWatcher* watcher : list)
    mWatchersToNotifiers.erase(watcher);
}

void FileSystemWatcherHub::Run()
{
  while(IsRunning())
  {
    Thread::Sleep(1000);
    Mutex::AutoLock locker(mGuardian);
    FileSystemEvent event;
    if (IsRunning())
      for(const auto& it : mWatchersToNotifiers)
        while(IsRunning() && it.first->GetNextEvent(event))
        {
          mQueuedMessage.push({ event, it.first, it.second });
          mSender.Send();
        }
  }
}

void FileSystemWatcherHub::ReceiveSyncMessage()
{
  Mutex::AutoLock locker(mGuardian);
  while(!mQueuedMessage.empty())
  {
    QueuedEvent e = mQueuedMessage.front();
    if (IsRunning())
      e.notifier->FileSystemWatcherNotification(*e.source, e.event.Target(), e.event.Events(), e.event.TimeStamp());
    mQueuedMessage.pop();
  }
}

