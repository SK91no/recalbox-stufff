#include "FileSystemWatcher.h"
#include "utils/Log.h"
#include "IFileSystemWatcherNotification.h"

#include <utils/String.h>
#include <sys/epoll.h>
#include <zconf.h>
#include <cstring>

FileSystemWatcher::FileSystemWatcher()
  : mUnregisterInterface(nullptr)
  , mEventBuffer()
  , mEpollEvents{}
  , mInotifyEpollEvent()
  , mInotifyFd(0)
  , mEpollFd(0)
{
  mInotifyFd = inotify_init1(IN_NONBLOCK);
  if (mInotifyFd == -1)
  { LOG(LogError) << "[FileWatcher] Can't initialize inotify ! " << strerror(errno) << "."; }

  mEpollFd = epoll_create1(0);
  if (mEpollFd == -1)
  { LOG(LogError) << "[FileWatcher] Can't initialize epoll ! " << strerror(errno) << "."; }

  mInotifyEpollEvent.events = EPOLLIN | EPOLLET;
  mInotifyEpollEvent.data.fd = mInotifyFd;
  if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mInotifyFd, &mInotifyEpollEvent) == -1)
  { LOG(LogError) << "[FileWatcher] Can't add inotify filedescriptor to epoll ! " << strerror(errno) << "."; }
}

FileSystemWatcher::~FileSystemWatcher()
{
  // Unregister first
  if (mUnregisterInterface != nullptr)
    mUnregisterInterface->UnregisterFileWatcher(this);

  Pause();

  epoll_ctl(mEpollFd, EPOLL_CTL_DEL, mInotifyFd, nullptr);

  if (close(mInotifyFd) != 0)
  { LOG(LogError) << "[FileWatcher] Error closing notify fd."; }

  if (close(mEpollFd) != 0)
  { LOG(LogError) << "[FileWatcher] Error closing poll fd."; }
}

void FileSystemWatcher::WatchDirectoryRecursively(const Path& path, EventType events)
{
  if (path.Exists())
  {
    WatchFile(path, events);
    if (path.IsDirectory())
    {
      Path::PathList list = path.GetDirectoryContent();
      for (const Path& p : list)
        if (p.IsDirectory())
          WatchDirectoryRecursively(p, events);
    }
  }
  else { LOG(LogError) << "[FileWatcher] Can´t watch Path! Path does not exist. Path: " + path.ToString(); }
}

void FileSystemWatcher::WatchFile(const Path& file, EventType events)
{
  if (file.Exists())
  {
    Mutex::AutoLock locker(mGuardian);
    if (!mPathToHandle.contains(file))
    {
      int wd = inotify_add_watch(mInotifyFd, file.ToChars(), (int) events);

      if (wd == -1)
      {
        int Error = errno;
        if (Error == 28) { LOG(LogError) << "[FileWatcher] Failed to watch! " << strerror(Error) << ". Please increase number of watches in \"/proc/sys/fs/inotify/max_user_watches\"."; }
        { LOG(LogError) << "[FileWatcher] Failed to watch! " << strerror(Error) << ". Path: " << file.ToString(); }
      }
      mHandleToPath.insert(wd, file);
      mPathToHandle.insert(file, wd);
      if (!mPathToEvents.contains(file)) // On resume, mPathtoEvents is already filled, do not modify
        mPathToEvents.insert(file, events);
    }
  }
  else { LOG(LogError) << "[FileWatcher] Can´t watch Path! Path does not exist. Path: " << file.ToString(); }
}

void FileSystemWatcher::RemoveWatchStartingWith(const Path& path)
{
  HashSet<Path> removed;
  {
    Mutex::AutoLock locker(mGuardian);
    for (const auto& it: mPathToHandle)
      if (it.first.StartWidth(path))
        removed.insert(it.first);
  }
  for(Path& p : removed)
    RemoveWatch(p);
}

bool FileSystemWatcher::RemoveWatch(const Path& path)
{
  {
    Mutex::AutoLock locker(mGuardian);
    if (int handle = PathToHandle(path); handle >= 0)
    {
      inotify_rm_watch(mInotifyFd, handle);
      mHandleToPath.erase(handle);
      mPathToHandle.erase(path);
      mPathToEvents.erase(path);
      return true;
    }
  }
  { LOG(LogWarning) << "[FileWatcher] Can´t unwatch Path! Path was not watched. Path: " << path.ToString(); }
  return false;
}

bool FileSystemWatcher::GetNextEvent(FileSystemEvent& fsevent)
{
  Mutex::AutoLock locker(mGuardian);
  int length = ReadEventsIntoBuffer();
  if (length != 0)
    ReadEventsFromBuffer(length, mEventQueue);

  if (!mEventQueue.empty())
  {
    fsevent = mEventQueue.front();
    mEventQueue.pop();
    return true;
  }
  return false;
}

int FileSystemWatcher::ReadEventsIntoBuffer()
{
  ssize_t length = 0;
  int timeout = 0;
  int nFdsReady = epoll_wait(mEpollFd, mEpollEvents, MAX_EPOLL_EVENTS, timeout);

  if (nFdsReady == -1) return 0;

  for (int n = nFdsReady; --n >= 0; )
  {
    length = read(mEpollEvents[n].data.fd, mEventBuffer, sizeof(mEventBuffer));
    if (length == -1 && errno == EINTR) break;
  }

  return (int)length;
}

void FileSystemWatcher::ReadEventsFromBuffer(int length, std::queue<FileSystemEvent>& to)
{
  int i = 0;
  while (i < length)
  {
    inotify_event* event = ((struct inotify_event*) &mEventBuffer[i]);

    if ((event->mask & IN_IGNORED) != 0u)
    {
      i += (int)(EVENT_SIZE + event->len);
      Mutex::AutoLock locker(mGuardian);
      mPathToHandle.erase(HandleToPath(event->wd));
      mHandleToPath.erase(event->wd);
      continue;
    }

    Path path = HandleToPath(event->wd) / String(event->len != 0 ? event->name : "");
    if (!path.IsEmpty() && !mIgnoredPath.contains(path))
    {
      if (path.IsDirectory())
      {
        // If the event occurs on a new subdirectory, watch it !
        Mutex::AutoLock locker(mGuardian);
        // Lookup parent event
        EventType* types = mPathToEvents.try_get(path.Directory());
        if (types != nullptr) WatchFile(path, *types);
        event->mask |= IN_ISDIR;
      }
      to.push(FileSystemEvent(event->wd, (EventType)event->mask, path, DateTime()));
    }
    i += (int)(EVENT_SIZE + event->len);
  }
}

void FileSystemWatcher::Pause()
{
  Mutex::AutoLock locker(mGuardian);
  for(const auto& it : mHandleToPath)
    inotify_rm_watch(mInotifyFd, it.first);
  mPathToHandle.clear();
  mHandleToPath.clear();
}

void FileSystemWatcher::Resume()
{
  Mutex::AutoLock locker(mGuardian);
  for(const auto& it : mPathToEvents)
    WatchFile(it.first, it.second);
}

void FileSystemWatcher::RemoveAllWatches()
{
  Mutex::AutoLock locker(mGuardian);
  Pause();
  mPathToEvents.clear();
}

void FileSystemWatcher::AddIgnoredPath(const Path& path)
{
  Mutex::AutoLock locker(mGuardian);
  mIgnoredPath.insert(path);
  RemoveWatchStartingWith(path);
}

void FileSystemWatcher::RemoveIgnoredPath(const Path& path)
{
  Mutex::AutoLock locker(mGuardian);
  mIgnoredPath.erase(path);
}

void FileSystemWatcher::RemoveAllIgnoredPath()
{
  Mutex::AutoLock locker(mGuardian);
  mIgnoredPath.clear();
}

