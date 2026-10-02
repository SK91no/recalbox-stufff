//
// Created by Pit64 on 11/11/25
//
#pragma once

#include "IMenuModelTask.h"
#include <usernotifications/ScriptOutputListenerInterface.h>
#include <utils/os/fs/Path.h>

//! Forward declarations
class WindowManager;

/*!
 * @brief Modal task to execute user scripts with visual feedback
 *
 * This class implements IMenuModalTask and ScriptOutputListenerInterface to provide
 * visual feedback when executing user scripts from the menu. It handles:
 * - Display of a nice modal window with background frame
 * - Progress bars for scripts with ReportProgress attribute
 * - Progress updates via percentage or text
 * - Display of success/error messages at completion
 * - Automatic cleanup via IMenuModalTask framework
 */
class MenuModalUserScript : private IMenuModalTask<int, bool>
                          , private ScriptOutputListenerInterface
{
  public:
    /*!
     * @brief Factory method to create and start a user script execution
     * @param window Window manager reference for displaying UI elements
     * @param scriptIndex Index of the script to run
     */
    static void CreateUserScriptRunner(WindowManager& window, int scriptIndex)
    {
      // Create new instance
      IMenuModalTask* task = new MenuModalUserScript(window, scriptIndex);
      window.pushGui(task);
      task->StartTask();
    }

  private:
    //! Constructor
    explicit MenuModalUserScript(WindowManager& window, int scriptIndex);

    //! Window manager reference (for async scripts that don't show modal)
    WindowManager& mWindowRef;

    //! Script execution result
    bool mScriptResult;

    //! Result data for final message
    struct ResultData
    {
      Path mPath;
      ScriptAttributes mAttributes;
      String mStdOut;
      String mStdErr;
      bool mError;
    };
    ResultData mResultData;
    Mutex mResultGuardian;

    /*!
     * @brief Extract and format the script name with icons
     * @param script Script path
     * @param attributes Script attributes
     * @return Formatted script name with icons
     */
    static String ExtractScriptName(const Path& script, ScriptAttributes attributes);

    /*
     * IMenuModalTask implementation
     */

    /*!
     * @brief Execute the script (called in worker thread)
     * @param scriptIndex Index of the script to run
     * @return True if the script executed successfully
     */
    bool TaskExecute(const int& scriptIndex) final;

    /*!
     * @brief Called when script execution completes (called in main thread)
     * @param result Result state
     */
    void TaskComplete(const bool& result) final;

    /*
     * ScriptOutputListenerInterface implementation (called from script thread)
     */
    void ScriptStarts(const Path& script, ScriptAttributes attributes) override;
    void ScriptOutputLine(const Path& script, ScriptAttributes attributes, const String& line) override;
    void ScriptCompleted(const Path& script, ScriptAttributes attributes, const String& output, bool error, const String& errors) override;
};