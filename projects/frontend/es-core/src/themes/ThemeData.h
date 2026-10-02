#pragma once

#include <deque>
#include <random>
#include <utils/String.h>
#include <utils/os/fs/Path.h>
#include <RecalboxConf.h>
#include "pugixml/pugixml.hpp"
#include "ThemeElement.h"
#include "ThemeFileCache.h"
#include "ThemeExtras.h"
#include "IGlobalVariableResolver.h"
#include "utils/eval/SimpleTokenizer.h"
#include "components/IVideoComponentAction.h"
#include "ThemeOption.h"
#include "ThemeVariablesHolder.h"
#include <themes/ThemeSupport.h>
#include <themes/ThemeExtras.h>

// Forward declarations
template<typename T> class TextListComponent;
class Component;
class Sound;
class ImageComponent;
class NinePatchComponent;
class TextComponent;
class WindowManager;

class ThemeData
{
  public:
    //! Region bitflag
    enum class Regions
    {
      US, //!< USA
      EU, //!< Europe
      JP, //!< Japan
    };

    //! Compatibility
    enum class Compatibility
    {
      None  = 0,
      Hdmi  = 1,
      Crt   = 2,
      Jamma = 4,
      Tate  = 8,
    };

    //! Compatibility
    enum class Resolutions
    {
      None = 0,
      QVGA = 1,
      VGA  = 2,
      HD   = 4,
      FHD  = 8,
    };

    //! Constructor
    explicit ThemeData(ThemeFileCache& cache, const SystemData* system, IGlobalVariableResolver& globalResolver, IThemeContextualVariableResolver& contextualResolver);

    /*!
     * @brief Open the main theme file and get compatibility
     * @param root Theme root path
     * @return
     */
    static bool FetchCompatibility(const Path& root, [[out]] Compatibility& compatibility, [[out]] Resolutions& resolutions, [[out]] String& name, [[out]] String& author, [[out]] int& version, [[out]] int& recalboxVersion);

    /*!
     * @brief Load main theme
     * @param root Root folder
     */
    void LoadMain(const Path& root);

    /*!
     * @brief Load system theme
     * @param systemFolder System ftheme folder
     * @param root System root path
     */
    void LoadSystem(const String& systemFolder, const Path& root);

    /*!
     * @brief Check if the curren theme contains a menu view
     * @return True if the curren theme contains a menu view, false otherwise
     */
    [[nodiscard]] bool HasMenuView() const { return mViews.contains("menu"); }

    /*!
     * @brief Check if the curren theme contains a logo view
     * @return True if the curren theme contains a logo view, false otherwise
     */
    [[nodiscard]] bool HasLogoView() const { return mViews.contains("gamelistlogo"); }

    /*!
     * @brief Check if the curren theme contains a gris view
     * @return True if the curren theme contains a grid view, false otherwise
     */
    [[nodiscard]] bool HasCarouselView() const { return mViews.contains("gamelistcarousel"); }

    /*!
     * @brief Check if the curren theme contains a grid view
     * @return True if the curren theme contains a grid view, false otherwise
     */
    [[nodiscard]] bool HasGridView() const { return mViews.contains("gamelistgrid"); }

    void loadFile(const String& systemThemeFolder, const Path& path);

    // If expectedType is an empty string, will do no type checking.
    [[nodiscard]] const ThemeElement* Element(const String& view, const String& element, ThemeElementType expectedType, ThemeElementType expectedType2) const;

    /*!
     * @brief Build Extra component array from the curren theme
     * @param view View name
     * @param window Window manager (for components initialization)
     * @param videoInterface callback interface for video components
     * @return Extra list
     */
    ThemeExtras::List GetExtras(const String& view, WindowManager& window, IVideoComponentAction* videoInterface) const;

    /*!
     * @brief Re-apply theme properties to the current extra list, allowing to refresh components and make them
     * doing a theme refresh or a theme switch
     * @param extras Extra list
     * @param view View from which to extract theme elements
     */
    void RefreshContextualExtraProperties(ThemeExtras::List& extras, const String& view) const;

    //! Check if the current theme is valid
    [[nodiscard]] bool IsValid() const { return mValid; }

    /*!
     * @brief Get the ThemeOption structure matching the given name
     * @param name Name of theme option (subset)
     * @return ThemeOption pointer or nullptr of the given name does not exists
     */
    [[nodiscard]] ThemeOption* GetSubset(const String& name) const { return mSubSets.try_get(name); }

    [[nodiscard]] Array<ThemeOption*> GetSubsets() const;

    //! Get subset list
    [[nodiscard]] String::List GetSubsetList() const;

    static const char* getNoTheme() { return "0 - DEFAULT"; }

    [[nodiscard]] RecalboxConf::Transition getThemeTransition() const;

    [[nodiscard]] bool hasDefaultTransition() const { return mDefaultTrans; }

    [[nodiscard]] bool isFolderHandled() const;

    static constexpr int MINIMUM_THEME_FORMAT_VERSION = 4;
    static constexpr int CURRENT_THEME_FORMAT_VERSION = 5;

    /*!
     * @brief Cleanup current ThemeData
     */
    void Reset();

    /*!
     * @brief Extract compatibility flags or set default hdmi flag
     */
    static Compatibility ExtractCompatibility(const String& comp);

    /*!
     * @brief Extract resolutions flags or set default hdmi flag
     */
    static Resolutions ExtractResolutions(const String& res);

    /*!
     * @brief Get theme raw name (filename less extension)
     * @return
     */
    [[nodiscard]] const String& RawName() const { return mThemeName; }

    /*!
     * @brief Convert compatibility bitflag into a human-readable text
     * @param display Display bitflag
     * @return Stringized bitflag
     */
    static String GetDisplayList(Compatibility display);

    /*!
     * @brief Convert resolution bitflag into a human-readable text
     * @param display Display bitflag
     * @return Stringized bitflag
     */
    static String GetResolutionList(Resolutions resolutions);

    //! Get variable holder
    [[nodiscard]] ThemeVariablesHolder& VariableHolder() { return mVariables; }

  private:
    //! View content
    class ThemeView
    {
      public:
        //! Raw element array
        std::vector<ThemeElement> mElementArray;
        //! Element name to element index
        HashMap<String, int> mElements;
    };

    //! Cache reference
    ThemeFileCache& mCache;
    //! System reference
    const SystemData* mSystem;

    //! View's properties
    HashMap<String, ThemeView> mViews;
    //! Recorded subsets
    HashMap<String, ThemeOption> mSubSets;

    //! Variable holder
    ThemeVariablesHolder mVariables;
    //! Variable resolver
    IThemeContextualVariableResolver& mResolver;

    //! Static list of element attributes not to process as element properties
    static HashSet<String> mNoProcessAttributes;

    std::deque<Path> mIncludePathStack;
    float mVersion;

    // System foilder (empty for main theme)
    String mSystemThemeFolder;
    // Theme name (theme folder name)
    String mThemeName;

    String mRandomPath;

    //! User langage
    String mLangageCode;
    //! User country
    String mCountryCode;
    //! User langage as integer
    int mLangageCodeInteger;
    //! User language/region as integer
    int mLanguageCountryCodeInteger;
    //! User region as integer
    int mRegionCodeInteger;

    //! External global variable resolver
    IGlobalVariableResolver& mGlobalResolver;
    //! External game-related variable resolver
    IGlobalVariableResolver* mGameResolver;

    //! Compatibility
    Compatibility mCompatiblity;

    //! Theme is loaded and valid?
    bool mValid;

    //! Theme has defaulttransition?
    mutable bool mDefaultTrans;

    void Clear(const Path& root)
    {
      mIncludePathStack.clear();
      mIncludePathStack.push_back(root);
      mViews.clear();
      mSubSets.clear();
    }

    static constexpr const char* sRandomMethod = "$random(";

    unsigned int getHexColor(const char* str, const pugi::xml_node& node);

    void parseFeatures(const pugi::xml_node& themeRoot);

    void parseVariables(const pugi::xml_node& themeRoot);

    void parseIncludes(const pugi::xml_node& themeRoot, bool mainTheme);

    void parseViews(const pugi::xml_node& themeRoot);

    void parseView(const pugi::xml_node& viewNode, ThemeView& view, bool forcedExtra);

    void parseElement(const pugi::xml_node& elementNode, const ThemePropertyNameBits& typeList, ThemeElement& element);

    /*!
     * @brief Parse an element's property
     * @param node source node
     * @param elementName Element name (parent node)
     * @param propertyName Property name (curren tnode)
     * @param value Property value
     * @param element Element in which to store property
     */
    void parseProperty(const pugi::xml_node& node, const String& elementName, ThemePropertyName propertyName, String& value, ThemeElement& element);

    [[nodiscard]] bool IsMatchingRegionOldTag(const pugi::xml_node& node) const;

    bool parseSubset(const pugi::xml_node& node);

    /*!
     * @brief Replace variables in path
     * @param refPath Reference path (for relative path)
     * @param systemThemeFolder System folder name
     * @param path Path to replace variables in
     * @param randomPath Random path for spécial random path feature
     */
    void resolveSystemVariable(const Path& refPath, const String& systemThemeFolder, [[out]] String& path, String& randomPath);

    static void PickRandomPath(String& value, String& randomPath);

    bool Condition(const pugi::xml_node& node);

    /*!
     * @brief Evaluate expression or subexpression
     * @param tokenizer Tokenizer to use
     * @param deepLevel Deep level ( >0 = subexpression level )
     * @return Evaluated expression or subexpression
     */
    [[nodiscard]] bool EvaluateExpression(SimpleTokenizer& tokenizer, int deepLevel) const;

    /*!
     * @brief Evaluate a single identifier in conditions
     * @param tokenizer Tokenizer to get identifier from. Must be located on an identifier
     * @return Evaluated identifier or false of th identifier is unknown
     */
    [[nodiscard]] bool EvaluateIdentifier(const SimpleTokenizer& tokenizer) const;

    /*!
     * @brief Build a file list from the current include stack for logging only
     * @param node source node
     * @return Log string
     */
    String FileList(const pugi::xml_node& node);

    /*!
     * @brief Build a file list from the current include stack for logging only
     * @return Log string
     */
    String FileList();

    /*!
     * @brief Browse the whole theme, looking for all subset & subset values
     * @param root Root theme file
     */
    void CrawlThemeSubSets(const Path& root);

    /*!
     * @brief Crawl new node subset
     * @param root root node
     */
    void CrawlSubsets(const pugi::xml_node& root);

    /*!
     * @brief Crawl includes, looking for subsets
     * @param root Root node
     */
    void CrawlIncludes(const pugi::xml_node& root);

    /*!
     * @brief Crawl includes, looking for region subsets
     * @param doc Document main node
     */
    //void CrawlRegions(const pugi::xml_document& doc);

    /*!
     * @brief Check if the selected option exists in thje given sibset.
     * If not, it is adjusted to the first option (default)
     * @param selected Selected option
     * @param subset Subset name
     * @return True if the option selected option has been adjusted, false otherwise
     */
    bool CheckThemeOption(String& selected, const String& subset) const;

    /*!
     * @brief Try to find language and region code in the name, after a final '.' ir available
     * @param name Original name - May be trimmed from localized info when returning
     * @return Language code, language + region code, or 0 if no information is available
     */
    static int ExtractLocalizedCode(String& name) ;

    /*!
     * @brief Extract item type from the given c string
     * @param visibility any combinaison of OR'ed "game", "folder", "header", comma separated
     * @return ThemeItemType
     */
    static ThemeItemType ExtractLimitedVisibility(const String& visibility);

    /*!
     * @brief Extract compatibility flags or set default hdmi flag
     */
    static Compatibility ExtractCompatibility(const pugi::xml_node& node);

    /*!
     * @brief Extract resolutions flags or set default hdmi flag
     */
    static Resolutions ExtractResolutions(const pugi::xml_node& node);


    bool IsMatchingLocaleOrRegionOrNeutral([[in, out]] String& name) const;

    bool IsPropertyMatchingLocaleOrRegionOrNeutral(ThemeElement& element, [[in, out]] String& name, [[out]] ThemePropertyName*& property, [[out]] bool& matchLocalization) const;
};

DEFINE_BITFLAG_ENUM(ThemeData::Compatibility, int)
DEFINE_BITFLAG_ENUM(ThemeData::Resolutions, int)
DEFINE_BITFLAG_ENUM(ThemeData::Regions, int)