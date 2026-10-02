//
// Created by bkg2k on 28/05/24.
//
#pragma once

#include <utils/String.h>
#include <utils/os/fs/Path.h>
#include <usernotifications/ScriptAttributes.h>

class ScriptOutputListenerInterface
{
  public:
    //! Defaulkt destructor
    virtual ~ScriptOutputListenerInterface() = default;

    /*!
     * @brief Notify start of a script
     * @param script Script path
     * @param attributes Attributes
     */
    virtual void ScriptStarts(const Path& script, ScriptAttributes attributes) = 0;

    /*!
     * @brief Receive raw string output from the currently running user script
     * @param script Script path
     * @param attributes Attributes
     * @param line Line string
     */
    virtual void ScriptOutputLine(const Path& script, ScriptAttributes attributes, const String& line) = 0;

    /*!
     * @brief Notify the script has completed
     * @param script Script path
     * @param attributes Attributes
     * @param output Complete stdout output
     * @param error True is error occured
     * @param errors Content of stderr if available and if the script was run synchronously. Empty otherwise
     */
    virtual void ScriptCompleted(const Path& script, ScriptAttributes attributes, const String& output, bool error, const String& errors) = 0;
};
