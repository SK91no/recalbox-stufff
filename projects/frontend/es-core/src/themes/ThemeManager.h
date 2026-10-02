//
// Created by bkg2k on 12/12/23.
//
#pragma once

#include "ThemeData.h"
#include "MenuThemeData.h"
#include "IThemeSwitchable.h"
#include "ThemeFileCache.h"
#include "guis/GuiSyncWaiter.h"
#include "MenuThemeDataCache.h"
#include "IThemeContextualVariableResolver.h"
#include "ThemeStringPool.h"
#include "ThemePropertyBagPool.h"
#include <utils/os/system/ThreadPool.h>

// forward declaration
class SystemData;

class ThemeManager : public StaticLifeCycleControler<ThemeManager>
                   , private IThreadPoolWorkerInterface<const SystemData*, const SystemData*>
                   , public IThemeSwitchTick
                   , public IThemeContextualVariableResolver
{
  public:
    //! Recalbox theme compatibility
    static constexpr int sRecalboxMinimumCompatibilityVersion = 0x902; // (Major << 8) + Minor
    //! Default theme folder
    static constexpr const char* sDefaultThemeFolder = "recalbox-next";
    //! Default theme file
    static constexpr const char* sRootThemeFile = "theme.xml";
    //! Default system folder
    static constexpr const char* sDefaultSystemThemeFolder = "default";

    //! List of theme w/ name
    typedef HashMap<String, Path> ThemeList;

    //! Default constructor
    explicit ThemeManager(IGlobalVariableResolver& globalResolver);

    //! Destructor
    ~ThemeManager() override;

    //! Load initial theme
    void Initialize(WindowManager* window);

    /*!
     * @brief Get a list of available theme
     * @return Theme list
     */
    static ThemeList AvailableThemes();

    /*!
     * @brief Load system theme
     * @param system System
     */
    void LoadSystemTheme(const SystemData& system);

    /*
     * Getters
     */

    //! Main theme
    [[nodiscard]] const ThemeData& Main() const { return mMain; }

    //! Menu theme
    [[nodiscard]] const MenuThemeData& Menu() const { return mMenu; }

    //! Menu cache
    [[nodiscard]] MenuThemeDataCache& MenuCache() { return mMenuCache; }

    //! System theme
    ThemeData& System(const SystemData* system) { return CreateOrGetSystem(system); }

    //! Shared deduplicating string pool backing every PropertyBag (thread-safe)
    [[nodiscard]] ThemeStringPool& ThemeStrings() { return mStringPool; }

    //! Shared deduplicating PropertyBag pool referenced by every theme element (thread-safe)
    [[nodiscard]] ThemePropertyBagPool& ThemeBags() { return mBagPool; }

    /*!
     * @brief Register a new switchable implementation
     * @param switchable Switchable implementation
     */
    void Register(IThemeSwitchable& switchable) { mSwitchables.Add(&switchable); }

    /*!
     * @brief Unregister a switchable implementation. May fail silently if the implementation is unknown
     * @param switchable Switchable implementation
     */
    void Unregister(IThemeSwitchable& switchable) { mSwitchables.Remove(&switchable); }

    /*!
     * @brief Theme or theme configuration has changed, reload main, menu & all existing system themes
     * then, apply theme in all UIs
     * @param window Window manager
     * @param force True to force theme reload
     */
    void DoThemeChange(WindowManager* window, bool force = false);

    /*!
     * @brief Check theme compatibility, extract display name and optionally switch to the theme
     * May notify the user of the compatibility lack when required to switch
     * @param window Window manager
     * @param themePath Theme path
     * @param name Displayable name
     * @param switchTheme True to switch to this theme
     * @param rawThemeVersion If not null, will be filled with the raw thgeme version (minor on first 8bits, major on next 8bits)
     * @return True iof the theme is compatible, false otherwise
     */
    bool AnalyseAndSwitch(WindowManager& window, const Path& themePath, [[out]] String& name, bool switchTheme, int* rawThemeVersion = nullptr);

    //! Get global variable resolver
    [[nodiscard]] const IGlobalVariableResolver& GlobalResolver() const { return mGlobalResolver; }

    /*!
     * @brief Give new context to extra component si that context-reactive component may refresh accordingly
     * @param displayedSystem Displayed system (never null)
     * @param gameDisplayed Displayed game (may be null)
     */
    static void ChangeThemeContext(const String& view, ThemeExtras::List& list, SystemData* displayedSystem, FileData* gameDisplayed);

    /*!
     * @brief Get default system logo from recalbox resources
     * @param system System for which to retrieve logo
     * @param path Outout logo path
     * @return
     */
    static bool GetDefaultSystemLogo(const SystemData& system, Path& path);

    //! Get current theme root path
    [[nodiscard]] const Path& CurrentThemePath() const { return mRootPath; }
    //! Get current theme name
    [[nodiscard]] const String& CurrentThemeName() const { return mName; }
    //! Get current theme author
    [[nodiscard]] const String& CurrentThemeAuthor() const { return mAuthor; }
    //! Get current theme version
    [[nodiscard]] int CurrentThemeVersion() const { return mVersion; }
    //! Get current theme recalbox version
    [[nodiscard]] int CurrentThemeRecalboxVersion() const { return mRecalboxVersion; }
    //! Get current theme compatibility
    [[nodiscard]] ThemeData::Compatibility CurrentThemeCaompatibility() const { return mCompatibility; }
    //! Get current theme resolutions
    [[nodiscard]] ThemeData::Resolutions CurrentThemeResolutions() const { return mResolutions; }

  private:
    //! Shared deduplicating string pool for all theme PropertyBag values
    ThemeStringPool mStringPool;
    //! Shared deduplicating PropertyBag pool referenced by every theme element
    ThemePropertyBagPool mBagPool;

    //! Global file cache
    ThemeFileCache mCache;

    //! Switchable implementation list
    Array<IThemeSwitchable*> mSwitchables;

    //! Main theme
    ThemeData mMain;
    //! Menu theme data
    MenuThemeData mMenu;
    //! System themes
    HashMap<const SystemData*, ThemeData*> mSystem;
    //! Hashmap protection
    Mutex mSystemLocker;

    // Theme menu data cache
    MenuThemeDataCache mMenuCache;

    //! Current theme root path
    Path mRootPath;
    //! Last theme name
    String mName;
    //! Last theme author
    String mAuthor;
    //! Theme version
    int mVersion;
    //! Recalbox version
    int mRecalboxVersion;
    //! Compatibility
    ThemeData::Compatibility mCompatibility;
    //! Resolutions
    ThemeData::Resolutions mResolutions;

    //! Wait bar
    GuiSyncWaiter* mWaiter;
    //! Wait bar reference time
    DateTime mWaitBarReference;

    //! Global variable resolver
    IGlobalVariableResolver& mGlobalResolver;

    /*!
     * @brief Create or get existing system theme
     * @param system System
     * @return ThemeData instance
     */
    ThemeData& CreateOrGetSystem(const SystemData* system);

    /*!
     * @brief Notify all IThemeSwitchable implementation that a new theme has been loaded
     * allowing them to refresh their themable elements
     * @param refreshOnly the theme did not change but options may have - Implementation must refresh element's properties
     */
    void NotifyThemeChanged(bool refreshOnly);

    /*!
     * @brief Return the currently selected theme root peth
     * If the theme is inconsistent with available themes, it is changed to match the first theme available
     */
    static Path GetThemeRootPath();

    /*!
     * @brief Load main theme & menu theme
     */
    void LoadMainTheme();

    /*
     * Variable replacement
     */

    typedef void (*ThemeContextGlobalResolver)(String& string, IGlobalVariableResolver& resolver);
    typedef void (*ThemeContextGlobalParameterizedResolver)(String& string, const String::List& parameters);
    typedef void (*ThemeContextDisplayedSystemResolver)(String& string, const SystemData& system);
    typedef void (*ThemeContextGameAndSystemResolver)(String& string, const FileData& game);

    //! Get static map to resolve theme global variables
    static const HashMap<String, ThemeContextGlobalResolver>& GetThemeContextGlobalResolverMap();
    //! Get static map to resolve theme global parameterized variables
    static const HashMap<String, ThemeContextGlobalParameterizedResolver>& GetThemeContextGlobalParameterizedResolverMap();
    //! Get static map to resolve theme displayed system variables
    static const HashMap<String, ThemeContextDisplayedSystemResolver>& GetThemeContextDisplayedSystemResolverMap();
    //! Get static map to resolve theme game & system variables
    static const HashMap<String, ThemeContextGameAndSystemResolver>& GetThemeContextGameAndSystemResolverMap();

    /*!
     * @brief Try to extract offset after the variable name and apply the offset to the current system in the
     * displayed system list, then return the new system. If the offset gets out of bounds, it wraps so that it returns
     * always a valid system
     * @param variable Variable from which to extract the offset
     * @return Valid system
     */
    static const SystemData* ApplySystemOffset(String& variable, const SystemData* system);

    /*!
     * @brief Try to extract offset after the variable name
     * @param variable Variable from which to extract the offset
     * @return True if the variable has an offset (even zero), false otherwise
     */
    static bool GetSystemOffset(String& variable);

    /*!
     * @brief Check iof the variable has parameters between (), and optionally remove parameters and ()
     * @param variable Variable to check
     * @param removeParams True to remove parameters so that caller will not have to deal w/ parameters
     * @return True if the variable is parameterized, false otherwise
     */
    static bool HasParameter(String& variable, bool removeParams);

    /*!
     * @brief Extract parameters from the given variable (and remove then along with ()) and store parameters
     * in the given list
     * @param variable Variable from which to extract parameters
     * @param parameters Array of string in which to store extracted parameters
     * @return True if parameters have been extracted, false otherwise
     */
    static bool ExtractParameters(String& variable, String::List& parameters);

    /*!
     * @brief Select a random value among those available in params list and put the result in string
     * @param string Output selected value
     * @param params Value list
     */
    static void RandomBetween(String& string, const String::List& params);

    /*!
     * @brief Interpret the first value as "min" and the second value as "max" in the given params list,
     * then select a random value between min & max (inclusive)
     * @param string Output selected value
     * @param params Value list
     */
    static void RandomRange(String& string, const String::List& params);

    /*
     * IThemeContextualVariableResolver
     */

    /*!
     * @brief Analyse the source string, looking for theme variables, then return the corresponding ThemeContexts
     * @param source
     * @return
     */
    ThemeContexts AnalyseContextualVariables(const String& source) final;

    /*!
     * @brief Resolve contextual variables in the given string, regarding the given System & Game.
     * @param source Source string in which to replace variables
     * @param displayedSystem Displayed system context or null
     * @param displayedGame Displayed game (and real system) or null
     */
    void ResolveContextualVariables(ThemeContexts contextSensibility, String& source, const SystemData* displayedSystem, const FileData* displayedGame) final;

    /*
     * IThreadPoolWorkerInterface implementation
     */

    const SystemData* ThreadPoolRunJob(const SystemData*& feed) override;

    /*
     * IThemeSwitchTick implementation
     */

    /*!
     * @brief Called by IThemeSwitchable implementation every time they update something slow
     * This is used by the theme manager to update wait bars while switching theme
     */
    void ThemeSwitchTick() override;
};

