#pragma once

#include "EventType.h"

class IFileSystemWatcherNotification
{
  public:
    /*!
     * @brief Instruct the callee that at least one event occured on the given file path
     * @param path File or folder
     * @param event Events occurred
     * @param time Date time of the event
     */
    virtual void FileSystemWatcherNotification(FileSystemWatcher& source, const Path& path, EventType event, const DateTime& time) = 0;
};