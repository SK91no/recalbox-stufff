//
// Created by bkg2k on 11/06/25.
//
#pragma once

#include "games/FileData.h"
#include "components/TextListComponent.h"
#include "IGamelistContainer.h"
#include "IGamelistEvents.h"
#include "views/PictogramCaches.h"
#include "guis/menus/base/RatingImage.h"
#include "TypedFileData.h"

class TextGamelist : public TextListComponent<TypedFileData>
                   , public IGamelistContainer
                   , private ITextListComponentInterface<TypedFileData>
                   , private ITextListComponentOverlay<TypedFileData>
                   , private RecalboxConf::ISystemGamelistDecorationNotification
{
  public:
    /*!
     * @brief Constructor
     * @param window Window manager
     * @param system System or nullptr for non system list
     * @param eventReceiver Event receiver
     * @param pictogramCaches Icon cache
     */
    TextGamelist(WindowManager& window, SystemData* system, IGamelistEvents& eventReceiver, PictogramCaches& pictogramCaches);

    //! Destructor
    ~TextGamelist() override = default;

    //! Interface access
    IGamelistContainer& Interface() { return *this; }

    /*!
     * @brief Process input - Allow non-inherited classes to call event processing
     * @param event Event to process
     * @return True if the implementation actually processed the event, false otherwise
     */
    bool ProcessInput(const InputCompactEvent& event) final { return ProcessListInput(event); };

  protected:

    /*
     * IGamelistContainer Implementation
     */

    /*!
     * @brief Get this list as a Themable
     * @return This list as a Themable
     */
    [[nodiscard]] Themable& AsThemable() final { return *this; }

    /*!
     * @brief Get this list as a Component
     * @return This list as a Component
     */
    [[nodiscard]] Component& AsComponent() final { return *this; }

    /*
     * List management
     */

    /*!
     * @brief Clear the whole list
     * @return This
     */
    IGamelistContainer& ClearGames() final { TextListComponent<TypedFileData>::clear(); return *this; }

    /*!
     * @brief Clear the whole list and reserve memory for 'reserved' elements
     * @param reserved reserved memory
     * @return This
     */
    IGamelistContainer& ClearGames(int reserved) final { TextListComponent<TypedFileData>::clear(reserved); return *this; }

    /*!
     * @brief Get all items in the list, in the list order.
     * @return Array of item
     */
    FileData::List GetGameArray() final
    {
      FileData::List result;
      for(const TypedFileData& data : TextListComponent<TypedFileData>::getObjectsArray())
        result.Add(data.mData);
      return result;
    };

    /*!
     * @brief Get games under the given header
     * @param header Header from which to get games
     * @return Gamelist
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
      if (game->IsFolder()) return AddFolder((FolderData*)game, colorIndex);
      TextListComponent<TypedFileData>::add(displayedName, { .mData=game, .mType=type }, colorIndex);
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
      TextListComponent<TypedFileData>::add(folder->Name(), { .mData=folder, .mType=GameType::Normal }, colorIndex);
      return *this;
    }

    /*!
     * @brief Add a Header
     * @param header Header to add
     * @param frontColorIndex Color index
     * @param backColorIndex Background Color index
     * @return
     */
    IGamelistContainer& AddHeader(HeaderData* header, int frontColorIndex, int backColorIndex) final
    {
      HorizontalAlignment alignment = TextListComponent<TypedFileData>::Alignment() == HorizontalAlignment::Left ? HorizontalAlignment::Right : HorizontalAlignment::Left;
      TextListComponent<TypedFileData>::add(header->Name(true), { .mData=header, .mType=GameType::Normal }, frontColorIndex, backColorIndex, alignment);
      return *this;
    }

    /*
     * State management
     */

    /*!
     * @brief Check if the list is actually scrolling or moving fast
     * @return True if the list is scrolling, false otherwise
     */
    bool IsScrolling() final { return TextListComponent<TypedFileData>::isScrolling(); }

    /*!
     * @brief Process input
     * @param event Event to process
     * @return True if the implementation actually processed the event, false otherwise
     */
    bool ProcessListInput(const InputCompactEvent& event) final;

    /*
     * Appearance management
     */

    /*!
     * @brief Set color at the given index (8 colors max)
     * @param index Color index
     * @param color ARGB Color
     * @return
     */
    IGamelistContainer& SetColor(int index, Colors::ColorARGB color) final { TextListComponent<TypedFileData>::setColorAt(index, color); return *this; }

    /*!
     * @brief Get color at the given index (8 colors max)
     * @param index Color index
     * @return This
     */
    Colors::ColorARGB GetColor(int index) final { return TextListComponent::Color(index); }

    /*!
     * @brief Gat text (used to draw name) height in pixels
     * @return Text height
     */
    int GetTextHeight() final { return TextListComponent::getFont()->Height(); }

    /*!
     * @brief Called when the list shows up on screen
     */
    void OnShow() final {}

    /*!
     * @brief Called when the list is no longer shown on screen
     */
    void OnHide() final { TextListComponent<TypedFileData>::stopScrolling(); }

    /*!
     * @brief Try to rename list item linked to the given game
     * @param game Game to lookup
     * @param newName New item name
     * @return True if the game has been found and the item is successfully renamed, false otherwise
     */
    bool Rename(FileData* game, const String& newName) final
    {
      if (int index = LookupGameIndex(game); index >= 0) return TextListComponent::changeTextAt(index, newName);
      return false;
    }

    /*!
     * @brief Try to rename list item at the given index
     * @param index Item index
     * @param newName New item name
     * @return True if the index is in range and the item is successfully renamed, false otherwise
     */
    bool RenameAt(int index, const String& newName) final { return TextListComponent::changeTextAt(index, newName); }

    /*!
     * @brief Return current selection name
     * @return Name of current selection
     */
    [[nodiscard]] const String& GetSelectedName() const final { return TextListComponent::mEntries[TextListComponent::mCursor].name; }

    /*!
     * @brief Return name of the item at the given index
     * or empty string if the index is out of bounds
     * @param index Index from which to retrieve item name
     * @return Item name
     */
    [[nodiscard]] const String& GetNameAt(int index) const final
    {
      if (index < 0 || index > Count() - 1) { static String s; return s; }
      return TextListComponent::mEntries[index].name;
    }

    /*
     * Game management
     */

    /*!
     * @brief Check if the list has at least one game/folder
     * @return True if the list contains at least one game or folder
     */
    bool HasGame() final { return TextListComponent<TypedFileData>::Count() != 0; }

    /*!
     * @brief Get game count
     * @return Game count
     */
    int GetGameCount() final { return TextListComponent<TypedFileData>::Count(); }

    /*!
     * @brief Get currently selected game
     * @return Selected game. If the list is empty, the implementation must return a reference to an empty item
     */
    [[nodiscard]] FileData* GetSelectedGame() final { return TextListComponent<TypedFileData>::getSelected().mData; }

    /*!
     * @brief Get gam at the given position
     * @param index
     * @return Game structure. If the list is empty, the implementation must return a reference to an empty item
     */
    [[nodiscard]] FileData* GetGameAt(int index) final { return TextListComponent<TypedFileData>::getObjectAt(index).mData; }

    /*!
     * @brief Try to select the given game in the list.
     * @param game Game to select
     * @param instant True to bypass any list transition/animation
     * @return True if the game exists on the list and has been selected. False otherwise
     */
    bool SetSelectedGame(FileData* game, bool instant) final;

    /*!
     * @brief Get selected game index
     * @return Index from 0 to game count - 1
     */
    [[nodiscard]] int GetSelectedGameIndex() const final { return TextListComponent<TypedFileData>::getCursorIndex(); }

    /*!
     * @brief Try to set selection to the given index
     * @param index Index to select
     * @param instant True to bypass any list transition/animation
     * @return True if the index is in the list range and the corresponding game has been selected. False otherwise
     */
    bool SetSelectedGameIndex(int index, bool instant) final
    {
      (void)instant;
      if (index < 0 || index > Count() - 1) return false;
      TextListComponent<TypedFileData>::setCursorIndex(index);
      mEventReceiver.OnGameSelected(*this, TextListComponent<TypedFileData>::getSelected().mData, TextListComponent<TypedFileData>::getCursorIndex());
      return true;
    }

    /*!
     * @brief Try to lookup index of the given game
     * @return Index of the given game, or -1 if not found
     */
    int LookupGameIndex(const FileData* game) final { return TextListComponent<TypedFileData>::Lookup({ .mData=(FileData*)game, .mType=GameType::Normal}); }

    /*!
     * @brief Try to move to the next header if there is one
     * @return True if the move is successful, false otherwise (no header after current position)
     */
    bool MoveToNextHeader() final;

    /*!
     * @brief Try to move to the previous header if there is one
     * @return True if the move is successful, false otherwise (no header before current position)
     */
    bool MoveToPreviousHeader() final;

    /*!
     * @brief Move the current selection according to the given step, either positive or negative
     * @param step Step to move forward or backward
     * @return This
     */
    IGamelistContainer& MoveRelative(int step) final;

    /*
     * Folder stack management
     */

    /*!
     * @brief Rebuild the whole folder stack from the given game, starting from the top directory down to the game directory.
     * @param from Game from which to rebuild the stack
     * @return This
     */
    IGamelistContainer& RebuildFolderStack(FileData* from) final;

    /*!
     * @brief Get the folder stack depth
     * @return Folder stack depth
     */
    int GetFolderStackDepth() final { return mFolderStack.Count(); }

    /*!
     * @brief Get the current folder list from the top directory down to the current directory
     * @return Directory list (only directories, not fullpath)
     */
    String::List GetBreadcrumb() final;

    /*!
     * @brief Get current folder
     * @return Current folder
     */
    FolderData* GetCurrentFolder() final { return !mFolderStack.Empty() ? mFolderStack.Peek() : &mSystem->MasterRoot(); };

    /*!
     * @brief Enter the given folder. If the folder exists in the current list, it is added to the stack
     * If it doesn't exist on the current list, the stack is rebuilt from the top folder down to the give folder
     * @param folder Folder to enter
     * @return This
     */
    IGamelistContainer& EnterFolder(FolderData* folder) final;

    /*!
     * @brief Exit current folder, pop it from the stack, and return the new folder
     * @return New current folder
     */
    FolderData* ExitFolder() final;

    /*!
     * @brief Check iof we are in the root folder or not
     * @return True if we are in the root folder, false otherwise
     */
    bool IsInRootFolder() final { return mFolderStack.Empty(); }

  private:
    //! Folder prefix
    static String sFolderPrefix;
    //! Hidden prefix
    static String sHiddenPrefix;
    //! Open prefix ▼
    static String sOpenPrefix;
    //! Close prefix ▶
    static String sClosePrefix;
    //! No fold prefix
    static String sNoFoldPrefix;
    //! No-arcade prefix (X)
    static String sNoArcadePrefix;
    //! Arcade parent prefix
    static String sArcadeParentPrefix;
    //! Arcade clone prefix
    static String sArcadeClonePrefix;
    //! Arcade orphan prefix
    static String sArcadeOrphanPrefix;
    //! Arcade bios prefix
    static String sArcadeBiosPrefix;

    //! Pictogram cache reference
    PictogramCaches& mPictogramCaches;
    //! Event receiver
    IGamelistEvents& mEventReceiver;
    //! System (virtual/real) - May be null for non-system lists
    SystemData* mSystem;
    //! Folder stack
    Stack<FolderData*> mFolderStack;
    //! Individual decorations (automatically set, depending on whether the system is set or not)
    bool mIndividualDecorations;

    //! Flag width not included any margin
    int mFlagWidth;
    //! Flag margin
    int mFlagMargin;

    //! Folder prefix width
    int mFolderPrefixWidth;
    //! Hidden prefix width
    int mHiddenPrefixWidth;
    //! Open prefix ▼ width
    int mOpenPrefixWidth;
    //! Close prefix ▶ width
    int mClosePrefixWidth;
    //! No fold prefix width
    int mNoFoldPrefixWidth;
    //! No-arcade prefix (X) width
    int mNoArcadePrefixWidth;
    //! Arcade parent prefix
    int mArcadeParentPrefixWidth;
    //! Arcade clone prefix
    int mArcadeClonePrefixWidth;
    //! Arcade orphan prefix
    int mArcadeOrphanPrefixWidth;
    //! Arcade bios prefix
    int mArcadeBiosPrefixWidth;

    //! Right decoration color (top-left)
    Colors::ColorARGB mDecorationColorTopLeft;
    //! Right decoration color (top-right)
    Colors::ColorARGB mDecorationColorTopRight;
    //! Right decoration color (bottom-left)
    Colors::ColorARGB mDecorationColorBottomLeft;
    //! Right decoration color (bottom-right)
    Colors::ColorARGB mDecorationColorBottomRight;

    //! Decoration cache per system
    HashMap<const SystemData*, RecalboxConf::GamelistDecoration> mSystemDecorations;
    //! Decoration of the current system (if available)
    RecalboxConf::GamelistDecoration mSystemDecoration;

    //! Rating
    RatingImage mHeaderStars;

    /*!
     * @brief Get decoration from system or from game is system is not set, and cache it for faster access
     * @param game Game to fetch system from, if required
     * @return Decoration
     */
    RecalboxConf::GamelistDecoration Decoration(FileData* game);

    //! Get a color or half the color
    static Colors::ColorARGB HalfColor(Colors::ColorARGB c, bool half) { return half ? ((c & 0xFFFFFF00) | ((c >> 1) & 0x7F)) : c; }

    /*!
     * @brief Jump to the next/previous letter
     * If the current selection is a child, move up to the parent first
     * @param forward True to jump to the next, false to jump to the previous
     */
    void JumpToNextLetter(bool forward);

    /*!
     * @brief Move up to the parent of the item at the given index, iif it's a clone or a child game
     * @param index Start index
     * @return Parent index
     */
    int MoveToParent(int index);

    /*
     * TextComponentList overrides
     */

    void OnFontChanged() final;

    /*
     * ITextListComponentOverlay<FileData*> implementation
     */

    /*!
     * @brief Apply (draw) an overlay in the given item rectangle and adjust rectangle position/size
     * so that the text won't draw over the overlay if required
     * @param parentTrans Parent transform matrice
     * @param leftWidth Left overlay width
     * @param rightWidth Right overlay width
     * @param labelWidth Cell's label width in pixel
     * @param area item rectangle
     * @param data Linked data
     * @param index Cursor index
     * @param color draw color
     */
    void OverlayApply(const Transform4x4f& parentTrans, int leftWidth, int rightWidth, int labelWidth, const Rectangle& area, TypedFileData const& data, int index, unsigned int& color) final;

    /*!
     * @brief Get the left offset (margin to the text) if any
     * @return left offset
     */
    float OverlayGetLeftOffset(TypedFileData const& data, int labelWidth) final;

    /*!
     * @brief Get the right offset (margin from text to right limit) if any
     * @return right offset
     */
    float OverlayGetRightOffset(TypedFileData const& data, int labelWidth) final;

    /*
     * Component override
     */

    /*!
     * @brief Called once per frame
     * @param deltatime Millisecond elapsed since the last frame (caped to 1000ms)
     */
    void Update(int deltatime) final;

    /*!
     * @brief Called once par frame to render the list
     * @param parentTrans Translation to apply
     */
    void Render(const Transform4x4f& parentTrans) final;

    /*
     * ITextListComponentInterface<TypedFileData> implementation
     */

    /*!
     * @brief Called when the cursor move to a new position
     * @param index New position index
     * @param data New position user data
     */
    void OnCursorChanged(int index, TypedFileData data) final { mEventReceiver.OnGameSelected(*this, data.mData, index); }

    /*
     * RecalboxConf::ISystemGamelistDecorationNotification implementation
     */

    void ConfigurationSystemGamelistDecorationChanged(const SystemData& system, const RecalboxConf::GamelistDecoration& value) final
    {
      if (&system == mSystem) mSystemDecoration = value;
      mSystemDecorations[&system] = value;
    }
};
