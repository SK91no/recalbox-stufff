//
// Created by bkg2k on 23/12/25.
//
#pragma once

#include "RecalboxConf.h"
#include "IGamelistContainer.h"
#include "IGamelistEvents.h"
#include "views/PictogramCaches.h"
#include "AbstractGamelist.h"

struct CarouselEntry
{
  signed char mColorIndex;           //!< Item color index (0..sColorCount-1)
  signed char mBackgroundColorIndex; //!< Item background color index (0..sColorCount-1)
  char        mChildren;             //!< Child count
};

class CarouselGamelist : private RecalboxConf::ISystemGamelistDecorationNotification
                       , public AbstractGamelist<CarouselEntry>
                       , public AbstractGamelist<CarouselEntry>::CallbackInterface
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
    CarouselGamelist(WindowManager& window, SystemData& system, IGamelistEvents& eventReceiver, PictogramCaches& pictogramCaches);

    //! Destructor
    ~CarouselGamelist() override = default;

    //! Interface access
    IGamelistContainer& Interface() { return *this; }

    /*!
     * @brief Clear the whole list
     * @return This
     */
    IGamelistContainer& ClearGames() final { AbstractGamelist::Clear(); mParentToChild.clear(); mLastParent = nullptr; return *this; }

    /*!
     * @brief Clear the whole list and reserve memory for 'reserved' elements
     * @param reserved reserved memory
     * @return This
     */
    IGamelistContainer& ClearGames(int reserved) final { AbstractGamelist::Clear(reserved); mParentToChild.clear(); mLastParent = nullptr; return *this; }

    /*!
     * @brief Get all items in the list, in the list order.
     * @return Array of item
     */
    FileData::List GetGameArray() final
    {
      FileData::List list;
      for(int i = -1, l = AbstractGamelist::Count(); ++i <= l; )
        list.Add(AbstractGamelist::GetGameAt(i));
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
     * @param displayedName Displayed name
     * @param type Game type
     * @param colorIndex Color index to use to display game name (or whatever)
     * @return This
     */
    IGamelistContainer& AddGames(FileData* game, const String& displayedName, GameType type, int colorIndex) final
    {
      switch(type)
      {
        case GameType::Normal:
        case GameType::ParentGameFolded:
        case GameType::ParentGameUnfolded:
        case GameType::ArcadeParentFolded:
        case GameType::ArcadeParentUnfolded: mLastParent = game; [[fallthrough]];
        case GameType::ArcadeParentAlone:
        case GameType::ArcadeOrphan:
        case GameType::ArcadeBios:
        case GameType::ArcadeIllegal: AbstractGamelist::Add(displayedName, { .mColorIndex=(signed char)colorIndex, .mBackgroundColorIndex=0, .mChildren=0}, game, type); break;
        case GameType::ChildGame:
        case GameType::ArcadeClone: mParentToChild[mLastParent].Add(game);
      }
      return *this;
    }

    /*!
     * @brief Add a folder
     * @param folder Folder to add
     * @param colorIndex Color index
     * @return
     */
    IGamelistContainer& AddFolder(FolderData* folder, int colorIndex) final
    {
      AbstractGamelist::Add(folder->Name(), { .mColorIndex=(signed char)colorIndex, .mBackgroundColorIndex=0, .mChildren=0}, folder, GameType::Normal);
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
      AbstractGamelist::Add(header->Name(), { .mColorIndex=(signed char)frontColorIndex, .mBackgroundColorIndex=(signed char)backColorIndex, .mChildren=0}, header,GameType::Normal);
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
      if (int p = LookupGame(game); p >= 0) { AbstractGamelist::SetIndex(p, instant); return true; }
      return false;
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
      if ((unsigned int)index < (unsigned int)AbstractGamelist::Count()) { AbstractGamelist::SetIndex(index, instant); return true; }
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
     * Themable
     */

    /*!
     * @brief Force implementations to return a theme element type
     * @return Element type
     */
    [[nodiscard]] ThemeElementType GetThemeElementType() const final { return ThemeElementType::CarouselList; }

    /*!
     * @brief Called when a theme element is applied on the current component.
     * @param element Element instance
     * @param properties Properties to update
     */
    void OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties) final;

  private:
    //! Max color count
    static constexpr int sColorCount = 8;

    //! Callback event
    IGamelistEvents& mEvents;
    //! Picto cache
    PictogramCaches& mCaches;

    //! Last game added (Only useful in list building)
    FileData* mLastParent;
    //! Parent to children
    HashMap<FileData*, Array<FileData*>> mParentToChild;

    //! Colors
    Colors::ColorARGB mColors[sColorCount];
    //! Font
    NewFont* mFont;

    //! Decorations
    RecalboxConf::GamelistDecoration mDecorations;

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
    void BaseListNameChangedAt(const String& newName, FileData* game, CarouselEntry& data) final { (void)newName; (void)game; (void)data; }
};

