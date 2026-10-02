//
// Created by bkg2k on 01/06/25.
//
#pragma once

#include <utils/String.h>
#include "ThemeContexts.h"

// Forward declarations
class SystemData;
class FileData;

class IThemeContextualVariableResolver
{
  public:
    //! Default destructor
    virtual ~IThemeContextualVariableResolver() = default;

    /*!
     * @brief Analyse the source string, looking for theme variables, then return the corresponding ThemeContexts
     * @param source
     * @return
     */
    virtual ThemeContexts AnalyseContextualVariables(const String& source) = 0;

    /*!
     * @brief Resolve contextual variables in the given string, regarding the given System & Game.
     * @param source Source string in which to replace variables
     * @param displayedSystem Displayed system context or null
     * @param displayedGame Displayed game (and real system) or null
     * @return Source string with replaced variables
     */
    virtual void ResolveContextualVariables(ThemeContexts contextSensibility, String& source, const SystemData* displayedSystem, const FileData* displayedGame) = 0;
};