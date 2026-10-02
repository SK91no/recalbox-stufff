//
// Created by bkg2k on 11/06/25.
//
#pragma once

// Forward declaration
class FileData;

class IGamelistEvents
{
  public:
    //! Default destructor
    virtual ~IGamelistEvents() = default;

    /*!
     * @brief Called when the list moved onto a new game
     * @param caller Calling gamelist implementation
     * @param game Selected game
     * @param index Game index in the list
     */
    virtual void OnGameSelected(IGamelistContainer& caller, FileData* game, int index) = 0;

    /*!
     * @brief Called when a game has been validated ("Valid" button pressed, then released)
     * @param caller Calling gamelist implementation
     * @param game Selected game
     * @param index Game index in the list
     */
    virtual void OnGameValidated(IGamelistContainer& caller, FileData* game, int index) = 0;

    /*!
     * @brief Called when a folder has been validated ("Valid" button pressed, then released)
     * @param caller Calling gamelist implementation
     * @param folder Selected folder
     * @param index Game index in the list
     */
    virtual void OnFolderValidated(IGamelistContainer& caller, FolderData* folder, int index) = 0;

    /*!
     * @brief The user just fold the given header
     * @param caller Calling gamelist implementation
     * @param header Header to fold (selected item)
     * @param index Item index
     */
    virtual void OnFoldHeader(IGamelistContainer& caller, HeaderData* header, int index) = 0;

    /*!
     * @brief The user just unfold the given header
     * @param caller Calling gamelist implementation
     * @param header Header to fold (selected item)
     * @param index Item index
     */
    virtual void OnUnfoldHeader(IGamelistContainer& caller, HeaderData* header, int index) = 0;

    /*!
     * @brief The user just fold all header
     * @param caller Calling gamelist implementation
     */
    virtual void OnFoldAllHeader(IGamelistContainer& caller) = 0;

    /*!
     * @brief The user just Unfold all header
     * @param caller Calling gamelist implementation
     */
    virtual void OnUnfoldAllHeader(IGamelistContainer& caller) = 0;

    /*!
     * @brief The user just performed a fold action on a game
     * @param caller Calling gamelist implementation
     * @param game Game on which the fold action has been performed
     * @param index Item index
     */
    virtual void OnFoldGame(IGamelistContainer& caller, FileData* game, int index) = 0;

    /*!
     * @brief The user just performed an unfold action on a game
     * @param caller Calling gamelist implementation
     * @param game Game on which the unfold action has been performed
     * @param index Item index
     */
    virtual void OnUnfoldGame(IGamelistContainer& caller, FileData* game, int index) = 0;

    /*!
     * @brief The user performed a fold all game action
     * @param caller Calling gamelist implementation
     */
    virtual void OnFoldAllGame(IGamelistContainer& caller) = 0;

    /*!
     * @brief The user performed an unfold all game action
     * @param caller Calling gamelist implementation
     */
    virtual void OnUnfoldAllGame(IGamelistContainer& caller) = 0;
};