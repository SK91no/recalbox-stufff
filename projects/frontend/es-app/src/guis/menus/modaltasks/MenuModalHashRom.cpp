//
// Created by bkg2k on 12/11/24.
//

#include "MenuModalHashRom.h"
#include "systems/SystemManager.h"
#include "WindowManager.h"

MenuModalHashRom::MenuModalHashRom(WindowManager& window, SystemManager& systemManager)
  : IMenuModalTask<bool, bool>(window, _("Preparing Games..."), false)
  , mSystemManager(systemManager)
  , mThreadPool(this, "Crc32", false)
  , mTotalGames(0)
  , mRemainingGames(0)
  , mPreviousProgressPercent(0)
{
}

bool MenuModalHashRom::TaskExecute(const bool& parameter)
{
  (void)parameter;

  // Just sleep 2s to make the long operation bar readable :)
  Thread::Sleep(2000);

  // Collect non-hashed roms...
  for(SystemData* system : mSystemManager.AllSystems())
    if (system->Descriptor().HasNetPlayCores())
    {
      FileData::List list;
      system->MasterRoot().getMissingHashRecursively(list);
      // Inject into thread pool
      for(FileData* game : list)
        mThreadPool.PushFeed(game);
    }

  mTotalGames = mThreadPool.PendingJobs();
  mRemainingGames = mTotalGames;

  // Run!
  if (mRemainingGames != 0) mThreadPool.Run(-4, true);
  mWaiter.WaitSignal();

  return true;
}

FileData* MenuModalHashRom::ThreadPoolRunJob(FileData*& feed)
{
  feed->CalculateHash();
  {
    Mutex::AutoLock locker(mSyncher);
    // Update
    if (--mRemainingGames <= 0)
      mWaiter.Fire();
    else
    {
      int percent = 100 - (mRemainingGames * 100) / mTotalGames;
      if (percent != mPreviousProgressPercent)
      {
        mPreviousProgressPercent = percent;
        SetText(_("Preparing Games...").Append(percent).Append('%'));
      }
    }
  }
  return feed;
}

