/*! File watching, inspired from the Inotofy library from Erik Zenker */

#pragma once

#include <utils/storage/HashMap.h>
#include <queue>
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <vector>

#include "FileSystemEvent.h"
#include "utils/storage/Set.h"
#include "utils/os/system/Mutex.h"
#include "IUnregisterFileWatcher.h"

/*!
 * @eventMask
 *
 * IN_ACCESS         File was accessed (read) (*).
 * IN_ATTRIB         Metadata changed—for example, permissions,
 *                   timestamps, extended attributes, link count
 *                   (since Linux 2.6.25), UID, or GID. (*).
 * IN_CLOSE_WRITE    File opened for writing was closed (*).
 * IN_CLOSE_NOWRITE  File not opened for writing was closed (*).
 * IN_CREATE         File/directory created in watched directory(*).
 * IN_DELETE         File/directory deleted from watched directory(*).
 * IN_DELETE_SELF    Watched file/directory was itself deleted.
 * IN_MODIFY         File was modified (*).
 * IN_MOVE_SELF      Watched file/directory was itself moved.
 * IN_MOVED_FROM     Generated for the directory containing the old
 *                   filename when a file is renamed (*).
 * IN_MOVED_TO       Generated for the directory containing the new
 *                   filename when a file is renamed (*).
 * IN_OPEN           File was opened (*).
 * IN_ALL_EVENTS     macro is defined as a bit mask of all of the above
 *                   events
 * IN_MOVE           IN_MOVED_FROM|IN_MOVED_TO
 * IN_CLOSE          IN_CLOSE_WRITE | IN_CLOSE_NOWRITE
 *
 * See inotify manpage for more event details
 *
 */

class FileSystemWatcher
{
  public:
    /*!
     * @brief Default constructor
     */
    FileSystemWatcher();

    /*!
     * @brief Default destructor
     */
    ~FileSystemWatcher();

    /*!
     * @brief Set Unregister interface
     */
     void SetUnregisterInterface(IUnregisterFileWatcher* unregisterInterface) { mUnregisterInterface = unregisterInterface; }

    /*!
     * @brief Recursively add every single file from the given path to the watch list
     * @param path Path to watch
     * @param events Events to watch
     */
    void WatchDirectoryRecursively(const Path& path, EventType events);

    /*!
     * @brief Add a single file to the watch list
     * @param file File to watch
     * @param events Events to watch
     */
    void WatchFile(const Path& file, EventType events);

    /*!
     * @brief Add an ignored file or folder
     * @param path Path to ignore
     */
    void AddIgnoredPath(const Path& path);

    /*!
     * @brief Remove an ignored file or folder
     * @param path Ignored Path
     */
    void RemoveIgnoredPath(const Path& path);

    /*!
     * @brief Clear the list of ignored files
     */
    void RemoveAllIgnoredPath();

    /*!
     * @brief Remove watched path
     * @param path Path to remove
     */
    bool RemoveWatch(const Path& path);

    /*!
     * @brief Remove all watched path
     */
    void RemoveAllWatches();

    /*!
     * @brief Remove watched path that starts with the given path
     * @param path Path to remove
     */
    void RemoveWatchStartingWith(const Path& path);

    //! Pause all events until Resume is called
    void Pause();

    //! Resume previously paused events
    void Resume();

    /*!
     * @brief Get next event if any
     * @param fsevent Event structure filled in with the next event if available
     * @return True if the fsevent structure has been filled with a valid event, false otherwise
     */
    bool GetNextEvent(FileSystemEvent& fsevent);

  private:
    //! Max events
    static constexpr int MAX_EVENTS = 256;
    //! Max polled event at once
    static constexpr int MAX_EPOLL_EVENTS = 1;
    //! Event structure size
    static constexpr int EVENT_SIZE = (int) (sizeof(inotify_event));

    /*!
     * @brief Return a handle from a path, or -1 if the handle does not exist
     * @param path Path
     * @return Handle
     */
    [[nodiscard]] int PathToHandle(const Path& path) const { int* found = mPathToHandle.try_get(path); return found != nullptr ? *found : -1; }

    /*!
     * @brief Return a path from an handle
     * @param wd Handle
     * @return Path
     */
    [[nodiscard]] Path HandleToPath(int wd) const { Path* found = mHandleToPath.try_get(wd); return found != nullptr ? *found : Path::Empty; }

    /*!
     * @brief Read available events if any and return read size
     * @return Read size
     */
    int ReadEventsIntoBuffer();

    /*!
     * @brief Convert raw events to event structures
     * @param length
     * @param events
     */
    void ReadEventsFromBuffer(int length, std::queue<FileSystemEvent>& to);

    //! Guardian
    Mutex mGuardian;

    //! Unregister interface
    IUnregisterFileWatcher* mUnregisterInterface;

    unsigned char mEventBuffer[MAX_EVENTS * (EVENT_SIZE + 16)];
    std::queue<FileSystemEvent> mEventQueue; //!< Recorded events
    HashMap<int, Path> mHandleToPath;        //!< Handle to path map
    HashMap<Path, int> mPathToHandle;        //!< Path to handle map
    HashMap<Path, EventType> mPathToEvents;  //!< Path to event map
    HashSet<Path> mIgnoredPath;              //!< Ignored file or folders

    //! Polling bloc (1 = read event one by one)
    epoll_event mEpollEvents[MAX_EPOLL_EVENTS];
    epoll_event mInotifyEpollEvent;

    int mInotifyFd;             //!< inotify file descriptor
    int mEpollFd;               //!< Polleing file descriptor
};
