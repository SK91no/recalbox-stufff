//
// Created by bkg2k on 10/11/24.
//
#pragma once

#include <utils/os/system/Thread.h>
#include <guis/GuiWaitLongExecution.h>
#include <WindowManager.h>

template <typename inType, typename outType>
class IMenuModalTask : protected GuiWaitLongExecution<inType, outType>
                     , protected ILongExecution<inType, outType>
{
  public:
    //! Constructor
    explicit IMenuModalTask(WindowManager& window, const String& titleText, const inType& in)
      : GuiWaitLongExecution<inType, outType>(window, *this)
      , mIn(in)
      , mTitle(titleText)
    {
    }

    //! Start must be explicity called from whild to avoid constructor race-condition
    void StartTask() { GuiWaitLongExecution<inType, outType>::Execute(mIn, mTitle); }

    //! Execute code here (in its own thread)
    virtual outType TaskExecute(const inType& in) = 0;

    //! Get result (synchronized with main thread)
    virtual void TaskComplete(const outType& out) = 0;

  protected:
    //! Type in
    const inType mIn;

  private:
    //! Title text
    String mTitle;

    /*
     * ILongExecution<inType, outType> implementation
     */

    outType Execute(GuiWaitLongExecution<inType, outType>& from, const inType& parameter) final
    {
      (void)from;
      return TaskExecute(parameter);
    }

    void Completed(const inType& parameter, const outType& result) final
    {
      (void)parameter;
      return TaskComplete(result);
    }
};