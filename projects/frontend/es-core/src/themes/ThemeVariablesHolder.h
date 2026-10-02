//
// Created by bkg2k on 27/06/25.
//
#pragma once

#include "ThemeUserVariable.h"
#include "utils/storage/HashMap.h"

// forward declaration
class FileData;

class ThemeVariablesHolder
{
  public:
    /*!
     * @brief Constructor
     * @param resolver Contextual variable reolver
     */
    explicit ThemeVariablesHolder(IThemeContextualVariableResolver& resolver, const SystemData* system)
      : mSystem(system)
      , mGame(nullptr)
      , mResolver(resolver)
    {}

    //! Clear all variables
    void Clear() { mVariables.clear(); }

    /*!
     * @brief Add a new variable
     * @param name Variable name
     * @param value Variable value
     */
    void Add(const String& name, const String& value);

    /*!
     * @brief Resolve everything possible, i.e. all user variabless, global variables and displayed system variables
     * game variables and displayed system with offset are not resolved
     */
    void ResolveUserVariables();

    /*!
     * @brief Resolve all user variables in the given string
     * @param value String in which to resolve user variables
     */
    void ResolveUserVariables(String& value) const;

    /*!
     * @brief Resolve everything than can be resolved in the given string
     * @param value Value in which to resolve variable
     * @paral game optional game for context resolution
     */
    void Resolve(String& value, FileData* game) const { mResolver.ResolveContextualVariables(ThemeContexts::All, value, mSystem, game); }

    /*!
     * @brief Resolve everything than can be resolved in the given string. This method is used mostly
     * in ThemeData while reading theme
     * @param value Value in which to resolve variable
     * @return Resolved string
     */
    String Resolve(const char* value) const
    {
      String s(value);
      Resolve(s, nullptr);
      ResolveUserVariables(s);
      return s;
    }

    /*!
     * @brief Set game context for variable resolution
     * @param game
     */
    void SetGameContext(FileData* game) { mGame = game; }

    //! Get game context
    [[nodiscard]] const FileData* GetGameContext() const { return mGame; }

    /*!
     * @brief Resolve contextual variables using system & game context
     * @param string String in which to resolve variables
     * @param contextToResolve Variable type to resolve
     */
    void Resolve(String& string, ThemeContexts contextToResolve) { mResolver.ResolveContextualVariables(contextToResolve, string, mSystem, mGame); }

  private:
    //! System reference (may be null)
    const SystemData* mSystem;
    //! Game context (may be null)
    const FileData* mGame;
    //! Contextual variable Resolver
    IThemeContextualVariableResolver& mResolver;
    //! Variable storage
    HashMap<String, ThemeUserVariable> mVariables;

    /*!
     * @brief Validate variabla name
     * @param name Name to validate
     * @return True if the name is valid, false otherwise
     */
    static bool ValidateName(const String& name);
};
