//
// Created by Pit64 on 11/11/25
//

#include "MenuModalUserScript.h"
#include <utils/locale/LocaleHelper.h>
#include <usernotifications/NotificationManager.h>

MenuModalUserScript::MenuModalUserScript(WindowManager& window, int scriptIndex)
  : IMenuModalTask<int, bool>(window, _("Running script..."), scriptIndex)
  , mWindowRef(window)
  , mScriptResult(false)
{
}

String MenuModalUserScript::ExtractScriptName(const Path& script, ScriptAttributes attributes)
{
  String name = script.FilenameWithoutDecorations();
  if ((attributes & (ScriptAttributes::Synced | ScriptAttributes::ReportProgress)) != 0) name.Append(' ');
  if (hasFlag(attributes, ScriptAttributes::Synced)) name.AppendUTF8(0xF1AF);
  if (hasFlag(attributes, ScriptAttributes::ReportProgress)) name.AppendUTF8(0xF1AE);
  return name;
}

void MenuModalUserScript::ScriptStarts(const Path& script, ScriptAttributes attributes)
{
  if ((attributes & ScriptAttributes::Synced) == 0)
  {
    // Asynchronous script - just show a message
    mWindowRef.displayMessage((_F(_("Script {0} started successfully!")) / ExtractScriptName(script, attributes)).ToString());
  }
  else
  {
    // Synchronous script - update the modal dialog (thread-safe)
    SetText((_F(_("Running {0}...")) / ExtractScriptName(script, attributes)).ToString());
    // Force progress bar to show even if script only outputs text
    if (hasFlag(attributes, ScriptAttributes::ReportProgress))
      SetProgress(0);
  }
}

void MenuModalUserScript::ScriptOutputLine(const Path& script, ScriptAttributes attributes, const String& line)
{
  (void)script;
  // Only update progress for synchronous scripts (thread-safe)
  if ((attributes & ScriptAttributes::Synced) != 0)
  {
    int percent = 0;
    if (line.StartsWith('%') && line.TryAsInt(1, 0, percent))
      SetProgress(percent);
    else
      SetSecondaryText(line);
  }
}

void MenuModalUserScript::ScriptCompleted(const Path& script, ScriptAttributes attributes, const String& output, bool error, const String& errors)
{
  // Store result data (protected by mutex)
  {
    Mutex::AutoLock lock(mResultGuardian);
    mResultData.mPath = script;
    mResultData.mAttributes = attributes;
    mResultData.mStdOut = output;
    mResultData.mStdErr = errors;
    mResultData.mError = error;
    mScriptResult = !error;
  }
}

bool MenuModalUserScript::TaskExecute(const int& scriptIndex)
{
  NotificationManager::Instance().RunManualScriptAt(scriptIndex, this);
  return mScriptResult;
}

void MenuModalUserScript::TaskComplete(const bool& result)
{
  (void)result;

  // Display final message (called from main thread)
  Mutex::AutoLock lock(mResultGuardian);

  if (hasFlag(mResultData.mAttributes, ScriptAttributes::Synced))
  {
    String title = _("Script execution complete");
    if (mResultData.mError)
    {
      String text = (_F(_("Script {0} has failed!")) / ExtractScriptName(mResultData.mPath, mResultData.mAttributes)).ToString();
      if (!mResultData.mStdErr.empty())
        text.Append('\n').Append(_("With the following Error output:")).Append('\n').Append('\n').Append(mResultData.mStdErr);
      mWindowRef.displayScrollMessage(title, text);
    }
    else
    {
      String text = (_F(_("Script {0} executed successfully!")) / ExtractScriptName(mResultData.mPath, mResultData.mAttributes)).ToString();
      if (!hasFlag(mResultData.mAttributes, ScriptAttributes::ReportProgress) && !mResultData.mStdOut.empty())
        text.Append('\n').Append(_("With the following output:")).Append('\n').Append('\n').Append(mResultData.mStdOut);
      mWindowRef.displayScrollMessage(title, text);
    }
  }
}