//
// Created by bkg2k on 11/06/25.
//
#pragma once

#include "GameType.h"
#include "views/container/HeaderData.h"

class IGamelistContainer
{
  public:
    //! Default destructor
    virtual ~IGamelistContainer() = default;

    /*
     * List management
     */

    /*!
     * @brief Clear the whole list
     * @return This
     */
    virtual IGamelistContainer& ClearGames() = 0;

    /*!
     * @brief Clear the whole list and reserve memory for 'reserved' elements
     * @param reserved reserved memory
     * @return This
     */
    virtual IGamelistContainer& ClearGames(int reserved) = 0;

    /*!
     * @brief Get all items in the list, in the list order.
     * @return Array of item
     */
    virtual FileData::List GetGameArray() = 0;

    /*!
     * @brief Get games under the given header
     * @param header Header from which to get games
     * @return Gamelist
     */
    virtual FileData::List GetHeaderGames(HeaderData* header) = 0;

    /*!
     * @brief Add a new game
     * @param game Game to add
     * @param displayedName Displayed name
     * @param type Game type
     * @param colorIndex Color index to use to display game name (or whatever)
     * @return This
     */
    virtual IGamelistContainer& AddGames(FileData* game, const String& displayedName, GameType type, int colorIndex) = 0;

    /*!
     * @brief Add a new game using color index 0
     * @param game Game to add
     * @param displayedName Displayed name
     * @param type Game type
     * @return This
     */
    IGamelistContainer& AddGames(FileData* game, const String& displayedName, GameType type)
    {
      return AddGames(game, displayedName, type, 0);
    }

    /*!
     * @brief Add a new game using color index 0 & normal game type
     * @param game Game to add
     * @param displayedName Displayed name
     * @return This
     */
    IGamelistContainer& AddGames(FileData* game, const String& displayedName)
    {
      return AddGames(game, displayedName, GameType::Normal, 0);
    }

    /*!
     * @brief Add a folder
     * @param folder Folder to add
     * @param colorIndex Color index
     * @return
     */
    virtual IGamelistContainer& AddFolder(FolderData* folder, int colorIndex) = 0;

    /*!
     * @brief Add a Header
     * @param header Header to add
     * @param frontColorIndex Color index
     * @param backColorIndex Background color index
     * @return
     */
    virtual IGamelistContainer& AddHeader(HeaderData* header, int frontColorIndex, int backColorIndex) = 0;

    /*
     * State management
     */

    /*!
     * @brief Check if the list is actually scrolling or moving fast
     * @return True if the list is scrolling, false otherwise
     */
    virtual bool IsScrolling() = 0;

    /*!
     * @brief Process input
     * @param event Event to process
     * @return True if the implementation actually processed the event, false otherwise
     */
    virtual bool ProcessListInput(const InputCompactEvent& event) = 0;

    /*
     * Appearance management
     */

    /*!
     * @brief Set color at the given index (8 colors max)
     * @param index Color index
     * @param color ARGB Color
     * @return This
     */
    virtual IGamelistContainer& SetColor(int index, Colors::ColorARGB color) = 0;

    /*!
     * @brief Get color at the given index (8 colors max)
     * @param index Color index
     * @return This
     */
    virtual Colors::ColorARGB GetColor(int index) = 0;

    /*!
     * @brief Gat text (used to draw name) height in pixels
     * @return Text height
     */
    virtual int GetTextHeight() = 0;

    /*!
     * @brief Called when the list shows up on screen
     */
    virtual void OnShow() = 0;

    /*!
     * @brief Called when the list is no longer shown on screen
     */
    virtual void OnHide() = 0;

    /*!
     * @brief Try to rename list item linked to the given game
     * @param game Game to lookup
     * @param newName New item name
     * @return True if the game has been found and the item is successfully renamed, false otherwise
     */
    virtual bool Rename(FileData* game, const String& newName) = 0;

    /*!
     * @brief Try to rename list item at the given index
     * @param index Item index
     * @param newName New item name
     * @return True if the index is in range and the item is successfully renamed, false otherwise
     */
    virtual bool RenameAt(int index, const String& newName) = 0;

    /*!
     * @brief Return current selection name
     * @return Name of current selection
     */
    [[nodiscard]] virtual const String& GetSelectedName() const = 0;

    /*!
     * @brief Return name of the item at the given index
     * or empty string if the index is out of bounds
     * @param index Index from which to retrieve item name
     * @return Item name
     */
    [[nodiscard]] virtual const String& GetNameAt(int index) const = 0;

    /*
     * Game management
     */

    /*!
     * @brief Check if the list has at least one game/folder
     * @return True if the list contains at least one game or folder
     */
    virtual bool HasGame() = 0;

    /*!
     * @brief Get game count
     * @return Game count
     */
    virtual int GetGameCount() = 0;

    /*!
     * @brief Get currently selected game
     * @return Selected game. If the list is empty, the implementation must return a reference to an empty item
     */
    [[nodiscard]] virtual FileData* GetSelectedGame() = 0;

    /*!
     * @brief Get gam at the given position
     * @param index
     * @return Game structure. If the list is empty, the implementation must return a reference to an empty item
     */
    [[nodiscard]] virtual FileData* GetGameAt(int index) = 0;

    /*!
     * @brief Try to select the given game in the list.
     * @param game Game to select
     * @param instant True to bypass any list transition/animation
     * @return True if the game exists on the list and has been selected. False otherwise
     */
    virtual bool SetSelectedGame(FileData* game, bool instant) = 0;

    /*!
     * @brief Get selected game index
     * @return Index from 0 to game count - 1
     */
    [[nodiscard]] virtual int GetSelectedGameIndex() const = 0;

    /*!
     * @brief Try to set selection to the given index
     * @param index index to set
     * @param instant True to bypass any list transition/animation
     * @return True if the index is in the list range and the corresponding game has been selected. False otherwise
     */
    [[nodiscard]] virtual bool SetSelectedGameIndex(int index, bool instant) = 0;

    /*!
     * @brief Try to lookup index of the given game
     * @return Index of the given game, or -1 if not found
     */
    virtual int LookupGameIndex(const FileData* game) = 0;

    /*!
     * @brief Try to move to the next header if there is one
     * @return True if the move is successful, false otherwise (no header after current position)
     */
    virtual bool MoveToNextHeader() = 0;

    /*!
     * @brief Try to move to the previous header if there is one
     * @return True if the move is successful, false otherwise (no header before current position)
     */
    virtual bool MoveToPreviousHeader() = 0;

    /*!
     * @brief Move the current selection according to the given step, either positive or negative
     * @param step Step to move forward or backward
     * @return This
     */
    virtual IGamelistContainer& MoveRelative(int step) = 0;

    /*
     * Folder stack management
     */

    /*!
     * @brief Rebuild the whole folder stack from the given game, starting from the top directory down to the game directory.
     * @param from Game from which to rebuild the stack
     * @return This
     */
    virtual IGamelistContainer& RebuildFolderStack(FileData* from) = 0;

    /*!
     * @brief Get the folder stack depth
     * @return Folder stack depth
     */
    virtual int GetFolderStackDepth() = 0;

    /*!
     * @brief Get the current folder list from the top directory down to the current directory
     * @return Directory list (only directories, not fullpath)
     */
    virtual String::List GetBreadcrumb() = 0;

    /*!
     * @brief Get current folder
     * @return Current folder
     */
    virtual FolderData* GetCurrentFolder() = 0;

    /*!
     * @brief Enter the given folder. If the folder exists in the current list, it is added to the stack
     * If it doesn't exist on the current list, the stack is rebuilt from the top folder down to the give folder
     * @param folder Folder to enter
     * @return This
     */
    virtual IGamelistContainer& EnterFolder(FolderData* folder) = 0;

    /*!
     * @brief Exit current folder, pop it from the stack, and return the new folder
     * @return New current folder
     */
    virtual FolderData* ExitFolder() = 0;

    /*!
     * @brief Check iof we are in the root folder or not
     * @return True if we are in the root folder, false otherwise
     */
    virtual bool IsInRootFolder() = 0;

    /*!
     * @brief Get this list as a Themable
     * @return This list as a Themable
     */
    virtual Themable& AsThemable() = 0;

    /*!
     * @brief Get this list as a Component
     * @return This list as a Component
     */
    virtual Component& AsComponent() = 0;
};