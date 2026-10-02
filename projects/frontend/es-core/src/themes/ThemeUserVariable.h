//
// Created by bkg2k on 27/06/25.
//
#pragma once

#include "ThemeContexts.h"
#include "utils/String.h"
#include "IThemeContextualVariableResolver.h"

class ThemeUserVariable
{
  public:
    /*!
     * @brief Constructor
     * @param name Variable name
     * @param value Variable value
     * @param resolver Value sensitivity
     */
    ThemeUserVariable(const String& name, const String& value, IThemeContextualVariableResolver& resolver)
      : mName(name)
      , mValue(value)
      , mSensitivity(resolver.AnalyseContextualVariables(value))
    {}

    //! Get name
    [[nodiscard]] const String& Name() const { return mName; }

    //! Get value
    [[nodiscard]] const String& Value() const { return mValue; }

    // Allow variable older to access private data
    friend class ThemeVariablesHolder;

  private:
    //! Variable name
    String mName;
    //! Variable Value
    String mValue;
    //! Context sensitivity (if value contains unresolved variables)
    ThemeContexts mSensitivity;
};