//
// Created by bkg2k on 10/11/24.
//
#pragma once

#include <utils/os/system/Thread.h>

class IMenuBackgroundTask : protected Thread
{
  public:
    //! Constructor
    explicit IMenuBackgroundTask(const char* name)
      : mTaskName(name)
    {}

    //! Start
    void TaskStart() { Start(mTaskName); }

    //! Stop task
    void TaskStop() { Thread::Stop(); }

  protected:
    //! Tell the TaskRun the thread us still running.
    //! When the method returns false, the TaskRun must exist ASAP !
    bool Running() { return Thread::IsRunning(); }

    //! Task Implementation
    virtual void TaskRun() = 0;

  private:
    //! Task name
    String mTaskName;

    //! Run the task
    void Run() final { TaskRun(); }
};