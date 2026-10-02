//
// Created by bkg2k on 27/01/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "GameRandomSelector.h"
#include <systems/SystemManager.h>
#include <cmath>

std::random_device GameRandomSelector::sRandomDevice;
std::mt19937 GameRandomSelector::sRandomGenerator(sRandomDevice());

GameRandomSelector::GameRandomSelector(SystemManager& systemManager, IFilter* filter)
  : mSystemManager(systemManager)
  , mFilter(filter)
  , mActiveFilter(nullptr)
  , mSystemIndex(0)
{
}

void GameRandomSelector::InitializeSystems()
{
  // Fill system array, weighting each system by 1 + 0.64*ln(games)^2
  // 1 game -> 1x, 5 -> 3x, 50 -> 11x, 5000 -> 48x, 250000 -> 100x
  mSystemArray.Clear();
  // Single filter instance shared by count (here) and load (EnsureSystemIsInitialized)
  mDefaultFilter = DefaultFilter();
  mActiveFilter = mFilter != nullptr ? mFilter : &mDefaultFilter;
  for(const SystemData* system : mSystemManager.VisibleSystemList())
  {
    // Demo system?
    if (!RecalboxConf::Instance().isInList("global.demo.systemlist", system->Name()) &&
        !RecalboxConf::Instance().GetSystemDemoInclude(*system)) continue;
    // Has game?
    int gameCount = SystemContent::GetSystemGameCount(system, *mActiveFilter);
    if (gameCount <= 0) continue;

    // Get system weight (convex log stretch, see comment above)
    double l = log((double)gameCount);
    int weightedGameCount = 1 + (int)ceil(0.64 * l * l);
    { LOG(LogDebug) << "[GameRandomizer] System " << system->FullName() << " added with a weight of " << weightedGameCount << " for " << gameCount << " games."; }
    // ... and copy the system as many times as it's weighted
    for(; --weightedGameCount >= 0; ) mSystemArray.Add(system);
  }

  ShuffleSystems();
}

void GameRandomSelector::ShuffleSystems()
{
  // Fisher-Yates: draw j in [0, i] for a uniform permutation
  for(int i = mSystemArray.Count(); --i >= 1;)
    mSystemArray.Swap(i, std::uniform_int_distribution<int>(0, i)(sRandomGenerator));
}

void GameRandomSelector::EnsureSystemIsInitialized(const SystemData* system)
{
  // Not initialized?
  if (!mGamesMap.contains(system))
    mGamesMap[system] = SystemContent(system, *mActiveFilter);
}

FileData* GameRandomSelector::NextGame()
{
  // Lazy systems initialization
  if (mSystemArray.Empty()) InitializeSystems();
  if (mSystemArray.Empty()) return nullptr; // No eligible system
  // Move system index, reshuffle on each full cycle for more variety
  if (--mSystemIndex < 0)
  {
    ShuffleSystems();
    mSystemIndex = mSystemArray.Count() - 1;
  }
  // Get system
  const SystemData* system = mSystemArray[mSystemIndex];
  // Lazy game initialization initialize
  EnsureSystemIsInitialized(system);
  // Get next games
  FileData* game = mGamesMap[system].NextGame();
  if (game == nullptr) return nullptr; // Selected system has no game

  { LOG(LogDebug) << "[GameRandomizer] Selected System " << system->FullName() << " (" << mSystemIndex << '/' << mSystemArray.Count() << ") - Game " << game->RomPath() << " (" << mGamesMap[system].Current() << '/' << mGamesMap[system].Total() << ')'; }
  return game;
}


