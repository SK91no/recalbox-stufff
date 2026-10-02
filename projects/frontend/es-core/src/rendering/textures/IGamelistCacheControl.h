//
// Created by bkg2k on 29/06/26.
//
#pragma once

//! Interface allowing the texture memory guard (es-core, TextureManager::WatchMemory) to ask the
//! application layer (es-app, ViewController) to free a whole gamelist view when the texture caches
//! are exhausted and the system is still under memory pressure. es-core knows nothing about
//! gamelists, so the contract is abstract and the implementation lives in ViewController.
class IGamelistCacheControl
{
  public:
    //! Default destructor
    virtual ~IGamelistCacheControl() = default;

    /*!
     * @brief Destroy the oldest still-alive gamelist view (least recently used), except the current
     * one (still displayed / reused on game exit). One view at most is freed per call.
     * @return True if a gamelist view was actually freed, false if none was evictable
     */
    virtual bool DestroyOldestGameListView() = 0;
};
