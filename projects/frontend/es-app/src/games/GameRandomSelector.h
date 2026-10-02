//
// Created by bkg2k on 27/01/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include "FileData.h"
#include <systems/SystemManager.h>
#include <systems/SystemData.h>
#include <utils/storage/Array.h>
#include <utils/storage/HashMap.h>
#include <random>

class GameRandomSelector
{
  public:
    //! Constructor
    explicit GameRandomSelector(SystemManager& systemManager, IFilter* filter);

    /*!
     * @brief Get next game
     * @return Next game
     */
    FileData* NextGame();

    //! Check if there is at least one non-empty system
    bool HasValidSystems() { return !mSystemArray.Empty(); }

    //! Initialize systemes
    void Initialize() { InitializeSystems(); }

  private:
    class DefaultFilter : public IFilter
    {
      private:
        FileData::TopLevelFilter mTopFilter;
      public:
        DefaultFilter() : mTopFilter(FileData::BuildTopLevelFilter()) {}
        [[nodiscard]] bool ApplyFilter(const FileData& file) override
        {
          return file.IsDisplayable(mTopFilter);
        }
    };

    class SystemContent
    {
      public:
        //! Defgault constructor
        SystemContent()
          : mIndex(0)
        {
        }

        /*!
         * @brief Build game list from the given system using the given filter if not null
         * @param system System to get game from
         * @param filter Filter to apply of not null
         */
        SystemContent(const SystemData* system, IFilter& filter)
          : mIndex(0)
        {
          // Load games
          mGames = system->MasterRoot().GetFilteredItemsRecursively(&filter, false, true);
          // Randomize list
          Randomize();
          { LOG(LogDebug) << "[GameRandomizer] System " << system->FullName() << " loaded " << mGames.Count() << " games."; }
        }

        void Randomize()
        {
          // Fisher-Yates: draw j in [0, i] for a uniform permutation
          for(int i = mGames.Count(); --i >= 1;)
            mGames.Swap(i, std::uniform_int_distribution<int>(0, i)(sRandomGenerator));
        }

        /*!
         * @brief Count games from the given system using the given filter if not null
         * @param system System to get game from
         * @param filter Filter to apply of not null
         */
        static int GetSystemGameCount(const SystemData* system, IFilter& filter)
        {
          int count = system->MasterRoot().CountFilteredItemsRecursively(&filter, false, true);
          { LOG(LogDebug) << "[GameRandomizer] System " << system->FullName() << ": " << count; }
          return count;
        }

        /*!
         * @brief Pick next game
         * @return
         */
        FileData* NextGame()
        {
          if (mGames.Empty()) return nullptr; // Nothing to pick
          if (--mIndex < 0)
          {
            Randomize();
            mIndex = mGames.Count() - 1;
          }
          FileData* game = mGames[mIndex];
          //{ LOG(LogDebug) << "[GameRandomizer] Game selected: " << game->Name() << " (" << game->RomPath().ToString() << ") in " << game->System().FullName(); }
          return game;
        }

        //! Get current index
        [[nodiscard]] int Current() const { return mIndex; }
        //! Get total
        [[nodiscard]] int Total() const { return mGames.Count(); }

      private:
        //! Map system => gamelist (lazy init)
        Array<FileData*> mGames;
        //! Current index
        int mIndex;
    };

    //! System manager instance
    SystemManager& mSystemManager;

    // Optionnal game filter
    IFilter* mFilter;

    //! Default filter (used when mFilter is null), and the resolved active filter shared by
    //! count (InitializeSystems) and load (EnsureSystemIsInitialized) to keep them consistent
    DefaultFilter mDefaultFilter;
    IFilter* mActiveFilter;

    //! Pre-randomized array of system
    //! System may appear one or mote times regarding the "weihgt"
    Array<const SystemData*> mSystemArray;

    //! Map system => gamelist (lazy init)
    HashMap<const SystemData*, SystemContent> mGamesMap;

    //! Random device to seed random generator
    static std::random_device sRandomDevice;
    //! Random generator
    static std::mt19937 sRandomGenerator;

    //! Current system index
    int mSystemIndex;

    //! Shuffle the weighted system array (uniform Fisher-Yates)
    void ShuffleSystems();

    /*!
     * @brief Initialize System list
     */
    void InitializeSystems();

    /*!
     * @brief Ensure the map is filled with game data and randomizer initialized, regarding the given system
     * @param system System to load games from
     */
    void EnsureSystemIsInitialized(const SystemData* system);
};



