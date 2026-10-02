#pragma once

#include <components/base/Component.h>
#include <components/TextComponent.h>
#include <components/IList.h>
#include <themes/ThemeExtras.h>
#include <themes/IThemeSwitchable.h>
#include "IProgressInterface.h"
#include "themes/IGlobalVariableResolver.h"
#include <utils/os/system/Thread.h>
#include <utils/os/system/Signal.h>
#include <utils/sync/SyncMessageSender.h>

class SystemManager;
class SystemData;
class AnimatedImageComponent;

enum class CarouselType : unsigned int
{
	Horizontal    = 0,
	Vertical      = 1,
	VerticalWheel = 2
};

struct SystemViewData
{
	std::shared_ptr<ImageComponent> logo;
	std::shared_ptr<TextComponent> logotext;
  std::shared_ptr<ThemeExtras> backgroundExtras;
  int textWidth = -1; // text-list mode: cached rendered text width
};

struct SystemViewCarousel
{
	CarouselType type;
	Vector2f pos;
	Vector2f size;
	Vector2f origin;
  NewFont* fontlarge;
  NewFont* fontmedium;
  unsigned int fontcolor;
	float logoScale;
	float logoRotation;
	Vector2f logoRotationOrigin;
  ::Alignment logoAlignment;
	unsigned int color;
	int maxLogoCount; // number of logos shown on the carousel
	Vector2f logoSize;
	bool textOnly;        // render system names as a text list instead of logo images
	bool forceUppercase;  // text-list mode: display system names in uppercase
	unsigned int selectorColor; // text-list mode: selector bar color
	unsigned int selectedColor; // text-list mode: selected entry text color
	unsigned int primaryColor;   // text-list mode: even unselected entry text color
	unsigned int secondaryColor; // text-list mode: odd unselected entry text color
	float selectorHeight;       // text-list mode: selector bar height (0 = auto, = row height)
	float selectorOffsetX;      // text-list mode: horizontal offset applied to selector bar
	float selectorOffsetY;      // text-list mode: vertical offset applied to selector bar
	float textOffsetX;          // text-list mode: horizontal offset applied to text (positive = right)
	float lineSpacing;          // text-list mode: row height = font height * lineSpacing
	float horizontalMargin;     // text-list mode: left/right text inset
	float zIndex;
};

struct SystemGameCount
{
  int VisibleGames;
  int Favorites;
  int Hidden;
};

class SystemView : public IList<SystemViewData, SystemData*>
                 , public Thread
                 , public ISyncMessageReceiver<SystemGameCount>
                 , public IThemeSwitchable
{
  public:
    SystemView(WindowManager& window, SystemManager& systemManager, const IGlobalVariableResolver& resolver);

    //! Destructor
    ~SystemView() override { Thread::Stop(); }

    void onShow() override { mShowing = true; }
    void onHide() override { mShowing = false; stopScrolling(); }

    void goToSystem(SystemData* system, bool animate);

    bool ProcessInput(const InputCompactEvent& event) override;
    void Update(int deltaTime) override;
    void Render(const Transform4x4f& parentTrans) override;

    //void onThemeChanged(const ThemeData& theme);

    bool CollectHelpItems(Help& help) override;
    void populate();

    /*!
     * @brief Set progress interface used ot notify first system loading
     * @param interface Progress interface
     */
    void SetProgressInterface(IProgressInterface* interface) { mProgressInterface = interface; }

    /*!
     * @brief update view with only visible games systems
     */
    void manageSystemsList();

    /*!
    * @brief Add a system to system view
    * @param System data
    */
    void addSystem(SystemData* system);

    /*!
     * @brief remove a system from system view
     * @param System data
     */
    void removeSystem(SystemData* system);

    /*!
     * @brief Lookup a system by name in the system list
     * @param name System name to look for
     * @return True if the system exists in the systemlist, false otherwise
     */
    SystemData* LookupSystemByName(const String& name);

    SystemData* Prev();
    void RemoveCurrentSystem();
    void Sort();
    void onCursorChanged(const CursorState& state) override;

  private:
    //! Global variable resolver
    const IGlobalVariableResolver& mResolver;
    //! SystemManager instance
    SystemManager& mSystemManager;

    SystemViewCarousel mCarousel;
    TextComponent mSystemInfo;

    //! IProgressInterface to notify loading screen we're loading systems (and it takes time...)
    IProgressInterface* mProgressInterface;

    //! System information synchronizer
    SyncMessageSender<SystemGameCount> mSender;
    //! System from witch to extract data (game count, favorites, hidden)
    const SystemData* mSystemFromWitchToExtractData;
    //! System locker
    Mutex mSystemLocker;
    //! Fetch info thread signal
    Signal mSystemSignal;

    // unit is list index
    SystemData* mCurrentSystem;
    float mCamOffset;
    float mExtrasCamOffset;
    float mExtrasFadeOpacity;
    bool mViewNeedsReload;
    bool mShowing;
    bool mLaunchKodi;

    // Marquee state for text-list mode
    enum class MarqueeSteps { LeftPause, ScrollToRight, RightPause, RollOver };
    static constexpr int sMarqueeSpeed1 = 80;
    static constexpr int sMarqueeSpeed2 = 160;
    static constexpr int sMarqueePause  = 1000;
    int mMarqueeOffset = 0;
    int mMarqueeTime   = 0;
    MarqueeSteps mMarqueeStep = MarqueeSteps::LeftPause;

    // Scrollbar fade state for text-list mode (mirrors TextListComponent behavior)
    static constexpr int sBarFixedTime = 200;
    static constexpr int sBarFadeTime  = 600;
    int  mBarTimer  = -1;
    bool mShowBar   = false;

    // Faint alternating row tints drawn behind unselected text-list entries
    static constexpr unsigned int sRowTintEven = 0x00000008;
    static constexpr unsigned int sRowTintOdd  = 0xFFFFFF08;

    //! Wrap an arbitrary row index into the valid [0, mEntries.size()) range
    int wrapEntryIndex(int index) const;
    //! System display name for the given entry, honoring the text-list uppercase option
    String entryDisplayName(int index) const;
    //! Rendered width of an entry's display name (lazily cached in its SystemViewData)
    int entryTextWidth(int index, const String& name);
    //! Width available for text-list names (box width minus both horizontal insets)
    float textListAreaWidth() const;

    /*!
     * @brief Assign a system logo to the given system entry
     * @param system System from which to retrieve logo
     * @param theme Theme to apply
     * @param entry System entry to fill in
     */
    void AssignSystemLogo(const SystemData& system, const ThemeData& theme, Entry& entry);

    /*!
     * @brief Check and apply alignment to the given component regarding carousel type
     * @param alignment Alignment to apply
     * @param component Component to align
     */
    static void AlignComponent(CarouselType carouselType, ::Alignment alignment, Component* component);

    /*!
     * @brief Set next system from witch to extract game information
     * @param system
     */
    void SetNextSystem(const SystemData* system);

	//! Process extra inputs
	bool InputExtra(const InputCompactEvent& event);

    //! Update extra data
    void UpdateExtra(int elapsed);

    /*
     * Thread implementation
     */

    //! Break the system info fetching thread
    void Break() override { mSystemSignal.Fire(); }

    //! Implementation of system fetching info
    void Run() override;

    /*
     * ISyncMessageReceiver<SystemGameCount> implementation
     */

    //! Receive enf-of-fetch info for the very last system the user is on
    void ReceiveSyncMessage(const SystemGameCount& data) override;

    /*
     * Legacy
     */

    void RefreshViewElements(const ThemeData& theme);
    void getDefaultElements();
    void getCarouselFromTheme(const ThemeElement* elem);

    void renderCarousel(const Transform4x4f& parentTrans);
    void renderTextList(const Transform4x4f& parentTrans);
    void renderExtras(const Transform4x4f& parentTrans, float lower, float upper);
    void renderInfoBar(const Transform4x4f& trans);
    void renderFade(const Transform4x4f& trans);

    /*
     * IThemeSwitchable
     */

    /*!
     * @brief System view process all system themes by itself
     * @return nullptr
     */
    [[nodiscard]] SystemData* SystemTheme() const override { return nullptr; };

    /*!
     * @brief Called when theme switch.
     * @param theme New Theme
     * @param refreshOnly True if we need to refresh the current view only
     */
    void SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface) override;
};
