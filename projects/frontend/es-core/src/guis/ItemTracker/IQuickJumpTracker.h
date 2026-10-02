//
// Created by bkg2k on 20/03/25.
//
#pragma once

// Forward declaration
class FileData;

class IQuickJumpTracker
{
  public:
    //! defautl destructor
    virtual ~IQuickJumpTracker() = default;

    /*!
     * @brief Called when the uiser select a new item to move to
     * @param item Item to move to
     */
    virtual void QuickJumpTo(FileData* item) = 0;
};