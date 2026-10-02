//
// Created by bkg2k on 08/05/25.
//
#pragma once

class FileSystemWatcher;

class IUnregisterFileWatcher
{
  public:
    //! Defauylt destructor
    virtual ~IUnregisterFileWatcher() = default;

    //! Unregister
    virtual void UnregisterFileWatcher(FileSystemWatcher* fileWatcherToUnregister) = 0;
};