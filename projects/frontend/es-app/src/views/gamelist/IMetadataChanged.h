//
// Created by bkg2k on 20/02/2026.
//
#pragma once

class FileData;

class IMetadataChanged
{
  public:
    //! Default destructor
    virtual ~IMetadataChanged() = default;

    /*!
     * @brief Notify that one or more metadata have changed for the given game
     * @param game Game whose metadata has changed
     */
    virtual void GameMetadataChanged(FileData& game) = 0;
};