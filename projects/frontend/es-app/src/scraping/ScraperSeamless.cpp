//
// Created by bkg2k on 17/03/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "ScraperSeamless.h"
#include <systems/SystemManager.h>
#include <patreon/PatronInfo.h>

// TODO
// - [ ] menu pour wiper les videos si plus de place
// - [ ] enlever le loading... quand l'image est chargée

ScraperSeamless::ScraperSeamless()
  : StaticLifeCycleControler<ScraperSeamless>("SeamlessScrape")
  , mConfiguration(RecalboxConf::Instance())
  , mEngines
    {
      ScreenScraperSingleEngine(this, &mEndPoints, this), ScreenScraperSingleEngine(this, &mEndPoints, this),
      ScreenScraperSingleEngine(this, &mEndPoints, this), ScreenScraperSingleEngine(this, &mEndPoints, this),
      ScreenScraperSingleEngine(this, &mEndPoints, this), ScreenScraperSingleEngine(this, &mEndPoints, this),
      ScreenScraperSingleEngine(this, &mEndPoints, this), ScreenScraperSingleEngine(this, &mEndPoints, this)
    }
  , mBusyEngines(0)
  , mSender(*this)
  , mPool(this, "SeamlessScrape", true)
{
  // Run the thread pool for seamless scraping loading
  mPool.Run(sScrapingEngineCount, true);

  { LOG(LogInfo) << "[SeamlessScraping] SeamlessScraper started"; }
}

FileData* ScraperSeamless::ThreadPoolRunJob(FileData*& feed)
{
  int engineIndex = AllocateEngine();
  if (engineIndex < 0)
  {
    { LOG(LogDebug) << "[SeamlessScraping] Error allocating engine! Scrape aborted!"; }
    return feed;
  }

  { LOG(LogDebug) << "[SeamlessScraping] Start scraping of " << feed->RomPath().ToString() << " using engine " << engineIndex; }

  // Get engine
  ScreenScraperSingleEngine& engine = mEngines[engineIndex];
  // Scrape!
  MetadataType dummy = MetadataType::None;
  engine.Scrape(ScrapingMethod::CompleteAndKeepExisting, *feed, dummy, mProtectedSet);
  // Free engine so that another scrape can start now
  FreeEngine(engineIndex);

  { LOG(LogDebug) << "[SeamlessScraping] Stop scraping of " << feed->RomPath().ToString(); }
  // Pop & remove game whatever the scraper returned
  mSender.Send({ feed, Stage::Completed, MetadataType::None, ScrapeSeamlessMessage::Types::GlobalCount });

  return feed;
}

void ScraperSeamless::ReceiveSyncMessage(const ScrapeSeamlessMessage& message)
{
  if (message.mType == ScrapeSeamlessMessage::Types::GameProgress)
  {
    // Status message
    FileData* game = message.mGame;
    IScraperEngineStage::Stage stage = message.mStage;
    IScraperEngineStage* interface = GetInterface(game);
    MetadataType changes = message.mChanges;
    if (game != nullptr && interface != nullptr)
      interface->ScrapingStageCompleted(game, stage, changes);
  }
  else if (message.mType == ScrapeSeamlessMessage::Types::GlobalCount)
  {
    FileData* game = message.mGame;
    INotifySeamlessScrapeResult* interface = GetYetToGoInterface(game);
    RemoveGame(game);
    // Count message
    if (interface != nullptr)
    {
      mRunningScrapesLoker.Lock();
      int count = (int) mRunningScrapes.size();
      mRunningScrapesLoker.UnLock();
      interface->ScrapeCountYetToGo(count);
    }
  }
  else { LOG(LogError) << "[ScraperSeamless] Unprocessed message type: " << (int)message.mType; }
}

void ScraperSeamless::ScrapingStageCompleted(FileData* game, Stage stage, MetadataType changes)
{
  // Send result to main thread
  mSender.Send({ game, stage, changes, ScrapeSeamlessMessage::Types::GameProgress });
}

void ScraperSeamless::Push(FileData::List& games, IScraperEngineStage* interface, INotifySeamlessScrapeResult* yetToGoInterface)
{
  // Scrape
  unsigned int ticks = SDL_GetTicks();
  Mutex::AutoLock locker(mRunningScrapesLoker);
  for(FileData* game : games)
    if (!mRunningScrapes.contains(game))
    {
      mRunningScrapes.insert(game, { ticks, interface, yetToGoInterface });
      mPool.PushFeed(game);
    }
  // Reset "initial" count
  if (yetToGoInterface != nullptr)
    yetToGoInterface->ScrapeCountYetToGo((int)mRunningScrapes.size());
}

void ScraperSeamless::Push(FileData* game, IScraperEngineStage* interface)
{
  if (PatronInfo::Instance().IsPatron() && !game->TopAncestor().ReadOnly() && RecalboxConf::Instance().GetScraperAuto())
  {
    // Need to run scrape again?
    long long ts = (long long)game->Metadata().TimeStamp();
    if (ts == 0 || (DateTime() - DateTime(ts)).TotalDays() >= 30)
    {
      // Scrape
      Mutex::AutoLock locker(mRunningScrapesLoker);
      if (!mRunningScrapes.contains(game))
      {
        mRunningScrapes.insert(game, { SDL_GetTicks(), interface, nullptr });
        mPool.PushFeed(game);
      }
    }
  }
}

IScraperEngineStage* ScraperSeamless::GetInterface(FileData* game)
{
  Mutex::AutoLock locker(mRunningScrapesLoker);
  Information* result = mRunningScrapes.try_get(game);
  if (result != nullptr) return result->Interface;
  return nullptr;
}

INotifySeamlessScrapeResult* ScraperSeamless::GetYetToGoInterface(FileData* game)
{
  Mutex::AutoLock locker(mRunningScrapesLoker);
  Information* result = mRunningScrapes.try_get(game);
  if (result != nullptr) return result->YetToGoInterface;
  return nullptr;
}

void ScraperSeamless::RemoveGame(FileData* game)
{
  Mutex::AutoLock locker(mRunningScrapesLoker);
  Information* result = mRunningScrapes.try_get(game);
  if (result != nullptr) mRunningScrapes.erase(game);
}
