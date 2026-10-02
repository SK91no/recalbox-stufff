#pragma once

#include <utils/os/fs/Path.h>
#include <utils/datetime/DateTime.h>
#include "EventType.h"

class FileSystemEvent
{
  public:
    //! Empty constructor
    FileSystemEvent()
      : mHandle(0),
        mMask(EventType::None)
    {
    }

    /*!
     * @brief Constructor
     * @param wd Handle
     * @param mask Event mask
     * @param path Path
     * @param eventTime Timestamp
     */
    FileSystemEvent(int wd, EventType mask, const Path& path, const DateTime& eventTime)
      : mHandle(wd),
        mMask(mask),
        mPath(path),
        mEventTime(eventTime)
    {
    }

    /*
     * Accessors
     */

    //! Get handle
    [[nodiscard]] int Handle() const { return mHandle; }
    //! Get events received
    [[nodiscard]] EventType Events() const { return mMask; }
    //! Get file/folder path
    [[nodiscard]] const Path& Target() const { return mPath; }
    //! Get timstamp
    [[nodiscard]] const DateTime& TimeStamp() const { return mEventTime; }

  private:
    int mHandle;
    EventType mMask;
    Path mPath;
    DateTime mEventTime;
};
