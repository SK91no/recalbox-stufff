//
// Created by bkg2k on 23/12/25.
//
#pragma once

#include "RecalboxConf.h"
#include "IGamelistContainer.h"
#include "IGamelistEvents.h"
#include "views/PictogramCaches.h"
#include "AbstractGamelist.h"
//#include "rendering/textures/TextureManager.h"

struct LogoEntry
{
  Texture mFront;
  Texture mBack;
  int mTextWidth;
  int mChildIndex;            //!< Global index in child list
  signed   char mChildLength; //!< Child count
  signed   char mChildPos;    //!< Current Child position
  signed   char mColorIndex;
  signed   char mBackgroundColorIndex;
  unsigned char mFrontOpacity;
  unsigned char mBackOpacity;
  signed   char mHeaderIndex;

  /*!
   * @brief Children placeholder constructor
   * @param parent Copy parent data
   * @param fistChildIndex First child index in children list
   */
  LogoEntry(const LogoEntry& parent, int fistChildIndex)
    : mBack(parent.mBack)
    , mTextWidth(-1)
    , mChildIndex(fistChildIndex)
    , mChildLength(2) // First child + parent
    , mChildPos(0)
    , mColorIndex(parent.mColorIndex)
    , mBackgroundColorIndex(parent.mBackgroundColorIndex)
    , mFrontOpacity(0)
    , mBackOpacity(0)
    , mHeaderIndex(-1)
  {}

  /*!
   * @brief Folder constructor
   * @param icon Folder icon
   * @param headerIndex Header index
   * @param colorIndex Folder name color
   * @param backColorIndex Background color (drawn if != 0)
   */
  LogoEntry(const Texture& icon, int headerIndex, int colorIndex, int backColorIndex)
    : mFront(icon)
    , mBack()
    , mTextWidth(-1)
    , mChildIndex(0)
    , mChildLength(1)
    , mChildPos(0)
    , mColorIndex((signed char)colorIndex)
    , mBackgroundColorIndex((signed char)backColorIndex)
    , mFrontOpacity(0)
    , mBackOpacity(0)
    , mHeaderIndex((signed char)headerIndex)
  {}

  /*!
   * @brief Regular game/parent constructor
   * @param front Front texture (folder / logo)
   * @param back Background texture
   * @param headerIndex Header index
   * @param colorIndex Main color index (text color)
   * @param backColorIndex Background color (drawn if != 0)
   */
  LogoEntry(const Texture& front, const Texture& back, int headerIndex, int colorIndex, int backColorIndex)
    : mFront(front)
    , mBack(back)
    , mTextWidth(-1)
    , mChildIndex(0)
    , mChildLength(1)
    , mChildPos(0)
    , mColorIndex((signed char)colorIndex)
    , mBackgroundColorIndex((signed char)backColorIndex)
    , mFrontOpacity(0)
    , mBackOpacity(0)
    , mHeaderIndex((signed char)headerIndex)
  {}
} __attribute__((packed));

class LogoGamelist : private RecalboxConf::ISystemGamelistDecorationNotification
                    , public AbstractGamelist<LogoEntry>
                    , public AbstractGamelist<LogoEntry>::CallbackInterface
                    , public Themable
{
  public:
    /*!
     * @brief Constructor
     * @param window Window manager
     * @param system System or nullptr for non system list
     * @param eventReceiver Event receiver
     * @param pictogramCaches Icon cache
     */
    LogoGamelist(WindowManager& window, SystemData& system, IGamelistEvents& eventReceiver, PictogramCaches& pictogramCaches);

    //! Destructor
    ~LogoGamelist() override = default;

    //! Interface access
    IGamelistContainer& Interface() { return *this; }

    /*
     * IGamelistContainer
     */

    /*!
     * @brief Clear the whole list
     * @return This
     */
    IGamelistContainer& ClearGames() final { return ClearGames(128); }

    /*!
     * @brief Clear the whole list and reserve memory for 'reserved' elements
     * @param reserved reserved memory
     * @return This
     */
    IGamelistContainer& ClearGames(int reserved) final
    {
      AbstractGamelist::Clear(reserved);
      mChildren.clear();
      mHeaders.Clear();
      mLastHeaderIndex = -1;
      return *this;
    }

    /*!
     * @brief Get all items in the list, in the list order.
     * @return Array of item
     */
    FileData::List GetGameArray() final
    {
      FileData::List list;
      for(int i = -1, l = AbstractGamelist::Count(); ++i <= l; )
        list.Add(AbstractGamelist::GetSelectedGame());
      return list;
    }

    /*!
     * @brief Get games under the given header
     * @param header Header from which to get games
     * @return Game list
     */
    FileData::List GetHeaderGames(HeaderData* header) final;

    /*!
     * @brief Add a new game
     * @param game Game to add
     * @param displayName Displayed name
     * @param type Game type
     * @param colorIndex Color index to use to display game name (or whatever)
     * @return This
     */
    IGamelistContainer& AddGames(FileData* game, const String& displayName, GameType type, int colorIndex) final;

    /*!
     * @brief Add a folder
     * @param folder Folder to add
     * @param colorIndex Color index
     * @return
     */
    IGamelistContainer& AddFolder(FolderData* folder, int colorIndex) final
    {
      // Store the raw name: uppercasing is applied at render time (see CasedName)
      AbstractGamelist::Add(folder->Name(), { mFolder, -1, colorIndex, -1 }, folder, GameType::Normal);
      return *this;
    }

    /*!
     * @brief Add a Header
     * @param header Header to add
     * @param frontColorIndex Color index
     * @param backColorIndex Background color index
     * @return
     */
    IGamelistContainer& AddHeader(HeaderData* header, int frontColorIndex, int backColorIndex) final
    {
      // Store the raw name: uppercasing is applied at render time (see CasedName)
      AbstractGamelist::Add(header->Name(), { mFolder, -1, frontColorIndex, backColorIndex }, header, GameType::Normal);
      mLastHeaderIndex = mHeaders.Count();
      mHeaders.Add(header);
      return *this;
    }

    /*!
     * @brief Check if the list is actually scrolling or moving fast
     * @return True if the list is scrolling, false otherwise
     */
    bool IsScrolling() final { return false; }

    /*!
     * @brief Process input
     * @param event Event to process
     * @return True if the implementation actually processed the event, false otherwise
     */
    bool ProcessListInput(const InputCompactEvent& event) final;

    /*!
     * @brief Set color at the given index (8 colors max)
     * @param index Color index
     * @param color ARGB Color
     * @return This
     */
    IGamelistContainer& SetColor(int index, Colors::ColorARGB color) final { if ((unsigned int)index < sColorCount) mColors[index] = color; return *this; }

    /*!
     * @brief Get color at the given index (8 colors max)
     * @param index Color index
     * @return This
     */
    Colors::ColorARGB GetColor(int index) final { return mColors[index % sColorCount]; }

    /*!
     * @brief Gat text (used to draw name) height in pixels
     * @return Text height
     */
    int GetTextHeight() final { return mFont->Height(); }

    /*!
     * @brief Called when the list shows up on screen
     */
    void OnShow() final { }

    /*!
     * @brief Called when the list is no longer shown on screen
     */
    void OnHide() final { }

    /*!
     * @brief Check if the list has at least one game/folder
     * @return True if the list contains at least one game or folder
     */
    bool HasGame() final { return AbstractGamelist::Count() != 0; }

    /*!
     * @brief Get game count
     * @return Game count
     */
    int GetGameCount() final { return AbstractGamelist::Count(); }

    /*!
     * @brief Try to select the given game in the list.
     * @param game Game to select
     * @param instant True to bypass any list transition/animation
     * @return True if the game exists on the list and has been selected. False otherwise
     */
    bool SetSelectedGame(FileData* game, bool instant) final
    {
      if (int p = LookupGame(game); p >= 0)
      {
        AbstractGamelist::SetIndex(p, instant);
        int index = Index();
        mEvents.OnGameSelected(*this, game, index);

        AbstractGamelist::Entry& entry = AbstractGamelist::GetRawEntryAt(index);
        mExtendDown = entry.IsUnfolded();

        return true;
      }
      if (int p = LookupParentGame(game); p >= 0)
      {
        p = MoveToChild(p);
        AbstractGamelist::SetIndex(p, instant);
        int index = Index();
        mEvents.OnGameSelected(*this, game, index);

        AbstractGamelist::Entry& entry = AbstractGamelist::GetRawEntryAt(index);
        mExtendDown = entry.IsUnfolded();

        return true;
      }
      return false;
    }

    /*!
     * @brief Lookup parent game from the given child the list and return index
     * @param child Child game too lookup
     * @return Game index or -1 if the given game is not found
     */
    int LookupParentGame(const FileData* child)
    {
      for(int i = (int)mChildren.size(); --i >= 0; )
        if (mChildren[i].mGame == child)
        {
          for (int p = AbstractGamelist::Count(); --p >= 0; )
            if (const LogoEntry& parent = AbstractGamelist::GetEntryDataAt(p); parent.mChildPos <= i)
              if (i < parent.mChildPos + parent.mChildLength)
                return p;
        }
      return -1;
    }

    //! Get game associated to the selected entry
    [[nodiscard]] FileData* GetSelectedGame() final
    {
      int i = MoveToChild(Index());
      return AbstractGamelist::GetRawEntryAt(i).mGame;
    }

    /*!
     * @brief Get selected game index
     * @return Index from 0 to game count - 1
     */
    [[nodiscard]] int GetSelectedGameIndex() const final { return AbstractGamelist::Index(); }

    /*!
     * @brief Try to set selection to the given index
     * @param index index to set
     * @param instant True to bypass any list transition/animation
     * @return True if the index is in the list range and the corresponding game has been selected. False otherwise
     */
    bool SetSelectedGameIndex(int index, bool instant) final
    {
      if ((unsigned int)index < (unsigned int)AbstractGamelist::Count())
      {
        if (AbstractGamelist::GetRawEntryAt(index).IsChild() && index > 0) --index;
        AbstractGamelist::SetIndex(index, instant);
        mEvents.OnGameSelected(*this, GetSelectedGame(), index);

        AbstractGamelist::Entry& entry = AbstractGamelist::GetRawEntryAt(index);
        mExtendDown = entry.IsUnfolded();

        return true;
      }
      return false;
    }

    /*!
     * @brief Try to lookup index of the given game
     * @return Index of the given game, or -1 if not found
     */
    int LookupGameIndex(const FileData* game) final { return LookupGame(game); }

    /*!
     * @brief Try to move to the next header if there is one
     * @return True if the move is successful, false otherwise (no header after current position)
     */
    bool MoveToNextHeader() final
    {
      for(int i = AbstractGamelist::Index(), l = AbstractGamelist::Count(); ++i < l; )
        if (AbstractGamelist::GetGameAt(i)->IsHeader()) { AbstractGamelist::SetIndex(i, false); return true; }
      for(int i = 0, l = AbstractGamelist::Index(); ++i < l; )
        if (AbstractGamelist::GetGameAt(i)->IsHeader()) { AbstractGamelist::SetIndex(i, false); return true; }
      return false;
    }

    /*!
     * @brief Try to move to the previous header if there is one
     * @return True if the move is successful, false otherwise (no header before current position)
     */
    bool MoveToPreviousHeader() final
    {
      for(int i = AbstractGamelist::Index(); --i >= 0; )
        if (AbstractGamelist::GetGameAt(i)->IsHeader()) { AbstractGamelist::SetIndex(i, false); return true; }
      for(int i = AbstractGamelist::Count(), l = AbstractGamelist::Index(); --i > l; )
        if (AbstractGamelist::GetGameAt(i)->IsHeader()) { AbstractGamelist::SetIndex(i, false); return true; }
      return false;
    }

    /*!
     * @brief Move the current selection according to the given step, either positive or negative
     * @param step Step to move forward or backward
     * @return This
     */
    IGamelistContainer& MoveRelative(int step) final { AbstractGamelist::SetIndex(AbstractGamelist::Index() + step, false); return *this; }

    /*
     * Component overrides
     */

    /*!
     * @brief Update components
     * @param deltaTime delta ms from the previous frame
     */
    void Update(int deltaTime) final;

    /*!
     * @brief Called once per frame, after Update
     * Implement your own Render() to draw your own components or over-drawings
     * First, execute: 	Transform4x4f trans = (parentTrans * getTransform()).round();
     *                  Renderer::setMatrix(trans);
     * Then draw your components
     * Finally, call your base.Render(trans) to draw animations and childrens
     * @param parentTrans Transformation
     */
    void Render(const Transform4x4f& parentTrans) final;

    /*
     * Themable
     */

    /*!
     * @brief Force implementations to return a theme element type
     * @return Element type
     */
    [[nodiscard]] ThemeElementType GetThemeElementType() const final { return ThemeElementType::LogoList; }

    /*!
     * @brief Implementations may return a second compatible element type
     * @return Element type
     */
    [[nodiscard]] ThemeElementType GetThemeElementType2() const final { return ThemeElementType::TextList; };

    /*!
     * @brief Called when a theme element is applied on the current component.
     * @param element Element instance
     * @param properties Properties to update
     */
    void OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties) final;

  protected:

    /*!
     * @brief Get primary media path or empty path
     * @param game Game providing media
     * @return Primary media path
     */
    virtual Path GetPrimaryMediaPath(const FileData* game);

    /*!
     * @brief Get secondary media path or empty path
     * @param game Game providing media
     * @return Secondary media path
     */
    virtual Path GetSecondaryMediaPath(const FileData* game);

  private:
    //! Child micro structure
    struct LogoChild
    {
      int mNameIndex;  //!< Display name index into the list name arena
      FileData* mGame; //!< Linked game
    };

    enum class MarqueeSteps
    {
      LeftPause,     //!< Start with a smart pause
      ScrollToRight, //!< Scroll until the last char is visible
      RightPause,    //!< Smart pause again to let the user read the end
      RollOver,      //!< Scroll until text #2 left part is on the start position and then restart
    };

    //! Max color count
    static constexpr int sColorCount = 8;
    //! Vertical divider (number of icon vertically)
    static constexpr int sVerticalDivider = 4;
    //! Time the scroll ar stay without fading
    static constexpr int sBarFixedTime = 200;
    //! Scrollbar fading time
    static constexpr int sBarFadeTime = 600;
    //! Normal scrolling speed
    static constexpr int sMarqueeSpeed1 = 80; // In pixel per seconds
    //! Accelerated scrolling speed
    static constexpr int sMarqueeSpeed2 = 160; // In pixel per seconds
    //! Pause time
    static constexpr int sMarqueePause = 1000; // In milliseconds

    //! Clone template text (arcade only)
    String mCloneTemplate;
    //! Related template text (non arcade only)
    String mRelatedTemplate;
    //! Parent template text (both arcade & non-arcade)
    String mParentTemplate;


    //! Callback event
    IGamelistEvents& mEvents;
    //! Picto cache
    PictogramCaches& mCaches;

    //! Children
    std::vector<LogoChild> mChildren;

    //! Headers are kept here as they may appear as non-items
    Array<HeaderData*> mHeaders;
    //! Last header index
    int mLastHeaderIndex;

    //! Colors
    Colors::ColorARGB mColors[sColorCount];
    //! Font
    NewFont* mFont;
    //! Font
    NewFont* mFolderFont;
    //! Default font effect
    FontEffect mDefaultFontEffect;

    //! Folder texture
    Texture mFolder;

    //! Scroll bar visible ?
    bool mShowScrollbar;
    //! Scroll bar fade timer
    int mScrollbarTimer;
    //! Scrollbar width
    const int mScrollbarWidth;

    //! Cached folder name maximum width
    int mFolderNameMaxWidth;
    //! Cached game name maximum width
    int mGameNameMaxWidth;
    //! Cached child name maximum width
    int mChildNameMaxWidth;

    //! Uppercasing
    bool mUppercase;

    //! Text scrolling offset
    int mMarqueeOffset;
    //! Text scrolling timer
    int mMarqueeTime;
    //! Text scrolling step
    MarqueeSteps mMarqueeStep;

    //! Alignment
    HorizontalAlignment mAlignment;

    //! Margin X
    int mHorizontalMargin;
    //! Margin Y
    int mVerticalMargin;
    //! Item height
    int mSelectorHeight;
    //! Selector color
    unsigned int mSelectorColor;
    //! Selected text color
    unsigned int mSelectedColor;
    //! Sidebar color
    unsigned int mBarColor;

    //! Right decoration color (top-left)
    Colors::ColorARGB mDecorationColorTopLeft;
    //! Right decoration color (top-right)
    Colors::ColorARGB mDecorationColorTopRight;
    //! Right decoration color (bottom-left)
    Colors::ColorARGB mDecorationColorBottomLeft;
    //! Right decoration color (bottom-right)
    Colors::ColorARGB mDecorationColorBottomRight;

    //! Picto width not including any margin
    int mPictoWidth;
    //! Picto width not including any margin
    int mPictoHeight;
    //! Picto horizontal margin
    int mPictoHMargin;
    //! Picto vertical margin
    int mPictoVMargin;

    //! Text offset from left
    int mTextOffset;
    //! Maximum text width
    int mTextMaxWidth;

    //! Time counter
    int mTimeCounter;

    //! Selection rectangle extension in pixels (downward)
    int mExtensionDown;
    //! True if the selection is currently extending downward
    bool mExtendDown;

    //! Decorations
    RecalboxConf::GamelistDecoration mDecorations;

    /*!
     * @brief Get the display-ready name of an entry, reconstructed from the arena and uppercased
     * if the theme requested it. Uppercasing is done here (at render time) rather than at build time
     * so the arena stores raw, deduplicated names.
     * @param entry Entry to get the name from
     * @return Display-ready name copy
     */
    [[nodiscard]] String CasedName(const AbstractGamelist::Entry& entry) const
    { String name = EntryName(entry); return mUppercase ? name.ToUpperCaseUTF8() : name; }

    //! Calculate pictogram size
    void GeneratePictoSize();

    /*!
     * @brief Reset scrolling stuff
     */
    void ResetMarquee();

    /*!
     * @brief Process name scrolling
     * @param elapsed Elapsed time since last call, in milliseconds
     */
    void ProcessMarquee(int elapsed);

    //! Get a color or half the color
    static Colors::ColorRGBA HalfColor(Colors::ColorRGBA c, int fade, bool half) { return half ? ((c & 0xFFFFFF00) | (((c * fade) >> 9) & 0x7F)) : ((c & 0xFFFFFF00) | (((c * fade) >> 8) & 0xFF)); }

    static Colors::ColorRGBA ShiftAlpha(Colors::ColorRGBA c, int opacity, int shift) { return (c & 0xFFFFFF00) | (((c & 0xFF) * opacity) >> (8 + shift)); }

    /*!
     * @brief Update scrollbar color, when not defined in theme
     */
    void UpdateScrollbarColor()
    {
      unsigned char lr = mSelectorColor >> 24;
      unsigned char lg = (mSelectorColor >> 16) & 0xFF;
      unsigned char lb = (mSelectorColor >> 8) & 0xFF;
      unsigned char la = mSelectedColor & 0xFF;
      unsigned char rr = mSelectedColor >> 24;
      unsigned char rg = (mSelectedColor >> 16) & 0xFF;
      unsigned char rb = (mSelectedColor >> 8) & 0xFF;
      unsigned char ra = mSelectedColor & 0xFF;
      mBarColor = (((lr + rr) >> 1) << 24) | (((lg + rg) >> 1) << 16) | (((lb + rb) >> 1) << 8) | ((la + ra) >> 1);
    }

    /*!
     * @brief Move child one step forward or backward
     * @param entry Child entry to move
     * @param forward True to move forward, false ot move backward
     * @param index Child entry index
     */
    void MoveChild(AbstractGamelist::Entry& entry, bool forward, int index);

    /*!
     * @brief Move child one step forward or backward
     * @param entry Child entry to move
     * @param child Child to lookup and set
     */
    void MoveChild(AbstractGamelist::Entry& entry, const FileData* child);

    /*!
     * @brief Draw item background if required
     * @param globalArea Global component drawing area
     * @param from From this lower position ( < 0 )
     * @param to To this upper position ( > 0)
     * @param yStart Upper Y position
     * @param mainIndex Central index ( == 0 )
     */
    void RenderItemBackground(const Rectangle& globalArea, int from, int to, int yStart, int mainIndex);

    /*!
     * @brief Render folder item
     * @param area Area to draw in
     * @param name Folder name
     * @param entry LogoEntry data
     * @param colorShift Alpha shifting
     */
    void RenderFolder(const Rectangle& area, const String& name, const LogoEntry& entry, int colorShift);

    /*!
     * @brief Render header item
     * @param area Area to draw in
     * @param header Header data
     * @param entry LogoEntry data
     * @param colorShift Alpha shifting
     */
    void RenderHeader(const Rectangle& area, HeaderData* header, const LogoEntry& entry, int colorShift);

    /*!
     * @brief Render rotated system logo
     * @param area Area to draw in
     * @param game Game to get system
     * @param fading Color fading
     * @param trans Parent transform
     * @param extend if True, the logo can extend over the upper item
     * @param drawActually True if the system logo isq actually drawn. False to make calculation only
     */
    void RenderSystemLogo(Rectangle& area, const FileData& game, int fading, const Transform4x4f& trans, bool extend, bool drawActually);

    /*!
     * @brief
     * @param area Area to draw in
     * @param entry Entry data
     * @param colorShift Alpha shifting
     */
    void RenderGameBackground(const Rectangle& area, AbstractGamelist::Entry& entry, int colorShift) const;

    /*!
     * @brief
     * @param area Area to draw in
     * @param entry Entry data
     * @param colorShift Alpha shifting
     * @param index Relative index
     */
    void RenderGameForeground(const Rectangle& area, AbstractGamelist::Entry& entry, int colorShift, int index);

    /*!
     * @brief Render scrollbar
     * @param area Global area
     * @param index Current index
     */
    void RenderScrollbar(const Rectangle& area, int index);

    /*!
     * @brief Render gamelist decorations
     * @param area Area to draw in
     * @param game Game which to draw data
     * @param fading current opacity
     * @param selected Is current item selected ?
     */
    void RenderDecorations(const Rectangle& area, const FileData& game, int fading, bool selected) const;

    /*!
     * @brief Render gamelist decorations in a single row (children)
     * @param area Area to draw in
     * @param game Game which to draw data
     * @param fading current opacity
     * @param selected Is current item selected ?
     */
    void RenderLinearDecorations(const Rectangle& area, const FileData& game, int fading, bool selected) const;

    /*!
     * @brief Render favorite/hidden/other metadata icon
     * @param area Area to draw in
     * @param game Game which to draw data
     * @param fading current opacity
     * @param selected Is current item selected ?
     */
    void RenderMetadataIcons(const Rectangle& area, const FileData& game, int fading, bool selected) const;

    /*!
     * @brief Move up to the parent of the item at the given index, iif it's a clone or a child game
     * @param index Start index
     * @return Parent index
     */
    [[nodiscard]] int MoveToParent(int index) const;

    /*!
     * @brief Move down to the child if any. Otherwise, stay on parent
     * @param index Start index
     * @return Child index
     */
    [[nodiscard]] int MoveToChild(int index);

    /*!
     * @brief Jump to the next/previous letter
     * If the current selection is a child, move up to the parent first
     * @param forward True to jump to the next, false to jump to the previous
     */
    void JumpToNextLetter(bool forward);

    /*!
     * @brief Get this list as a Themable
     * @return This list as a Themable
     */
    [[nodiscard]] Themable& AsThemable() final { return *this; }

    /*
     * RecalboxConf::ISystemGamelistDecorationNotification
     */

    /*!
     * @brief
     * @param system Target system
     * @param value New decorations
     */
    void ConfigurationSystemGamelistDecorationChanged(const SystemData& system, const RecalboxConf::GamelistDecoration& value) final { if (&System() == &system) mDecorations = value; }

    /*
     * AbstractGamelist<CarouselEntry>::CallbackInterface
     */

    /*!
     * @brief Called when the selection changes
     * @param newIndex New selected index
     * @param data Selected game
     */
    void BaseListIndexChanged(int newIndex, FileData* data) final;

    /*!
     * @brief Callback when a game is renamed to let implementation recalculate name-related values
     * @param newName New name
     * @param game Associated game
     * @param data Associated data
     */
    void BaseListNameChangedAt(const String& newName, FileData* game, LogoEntry& data) final
    {
      (void)newName;
      (void)game;
      data.mTextWidth = -1;
    }
};

