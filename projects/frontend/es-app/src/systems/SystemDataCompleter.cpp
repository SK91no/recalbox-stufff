//
// Created by bkg2k on 02/03/23.
//

#include "SystemDataCompleter.h"
#include "utils/Zip.h"
#include "utils/7z/SevenZipInspector.h"

void SystemDataCompleter::Start()
{
  Thread::Start("Hasher");
  mSignal.Fire();
}

SystemDataCompleter::~SystemDataCompleter()
{
  Thread::Stop();
}

bool SystemDataCompleter::CheckHashFor(FileData& game)
{
  if (game.IsGame() && !game.TopAncestor().ReadOnly())
    if (game.Metadata().RomCrc32() == 0) { game.CalculateHash(); return true; }
  return false;
}

bool SystemDataCompleter::CheckSupportTypeFor(FileData& game)
{
  if (game.IsGame() && !game.TopAncestor().ReadOnly())
    if (game.Metadata().SupportType() == SupportTypes::Unknown)
    {
      Path path(game.Metadata().Rom());
      String ext = path.Extension().ToLowerCase();
      if (ext == ".zip")
      {
        Zip zip(path);
        for(int i = zip.Count(); --i >= 0; )
          if (SupportTypes type = game.System().Descriptor().LookupSupportType(zip.FileName(i).Extension()); type != SupportTypes::Unknown)
          {
            game.Metadata().SetSupportType(type);
            return true;
          }
      }
      else if (ext == ".7z")
      {
        SevenZipInspector sevenZip(path);
        for(const Path& filePath : sevenZip.AllFiles())
          if (SupportTypes type = game.System().Descriptor().LookupSupportType(filePath.Extension()); type != SupportTypes::Unknown)
          {
            game.Metadata().SetSupportType(type);
            return true;
          }
      }
      else game.Metadata().SetSupportType(game.System().Descriptor().LookupSupportType(ext));
    }
  return false;
}

void SystemDataCompleter::CheckHashFor(SystemData& system)
{
  class MissingHashes : public IParser
  {
    private:
      //! Parent reference
      SystemDataCompleter& mHasher;
      //! Hashed rom count
      int mCount;

    public:
      explicit MissingHashes(SystemDataCompleter& hasher)
        : mHasher(hasher)
        , mCount(0)
      {}

      void Parse(FileData& game) override
      {
        if (!mHasher.IsRunning()) return;
        if (CheckHashFor(game)) ++mCount;
      }

      [[nodiscard]] int HashCount() const { return mCount; }
  } missingHashes(*this);

  // Check missing hashed
  if (system.Descriptor().HasNetPlayCores())
  {
    DateTime start;
    system.MasterRoot().ParseAllItems(missingHashes, false);
    if (missingHashes.HashCount() != 0)
    { LOG(LogInfo) << "[SystemHasher] Calculated " << missingHashes.HashCount() << " missing hashes of " << system.FullName() << ". Took " << (DateTime() - start).TotalMilliseconds() << "ms."; }
  }
}

void SystemDataCompleter::CheckSupportTypeFor(SystemData& system)
{
  class MissingSupportTypes : public IParser
  {
    private:
      //! Parent reference
      SystemDataCompleter& mHasher;
      //! Hashed rom count
      int mCount;

    public:
      explicit MissingSupportTypes(SystemDataCompleter& hasher)
        : mHasher(hasher)
        , mCount(0)
      {}

      void Parse(FileData& game) override
      {
        if (!mHasher.IsRunning()) return;
        if (game.IsGame())
          if (CheckSupportTypeFor(game)) ++mCount;
      }

      [[nodiscard]] int HashCount() const { return mCount; }
  } missingSupports(*this);

  DateTime start;
  system.MasterRoot().ParseAllItems(missingSupports, false);
  if (missingSupports.HashCount() != 0)
  { LOG(LogInfo) << "[SystemHasher] Calculated " << missingSupports.HashCount() << " missing support types of " << system.FullName() << ". Took " << (DateTime() - start).TotalMilliseconds() << "ms."; }
}

void SystemDataCompleter::CompleteSystemData(SystemData& system)
{
  CheckHashFor(system);
  CheckSupportTypeFor(system);
}

void SystemDataCompleter::CompleteGameData(FileData& game)
{
  CheckHashFor(game);
  CheckSupportTypeFor(game);
}

void SystemDataCompleter::Run()
{
  // De-prioritize this thread to IDLE
  sched_param params { .sched_priority = 0 };
  if (pthread_setschedparam(pthread_self(), SCHED_IDLE, &params) == 0)
    { LOG(LogDebug) << "[SystemHasher] SystemDataCompleter thread set to IDLE priority"; }
  else
    { LOG(LogError) << "[SystemHasher] Error setting IDLE priority to SystemDataCompleter thread"; }

  // Initial delay
  Thread::Sleep(30 * 1000);

  // Loop
  while(IsRunning())
  {
    // Wait until a job is queued
    mSignal.WaitSignal();
    if (!IsRunning()) return;

    // Dequeue game
    for (;;)
    {
      mLocker.Lock();
      FileData* game = !mFileQueue.Empty() ? mFileQueue.Pop() : nullptr;
      mLocker.UnLock();
      // Check a single game
      if (game ==  nullptr) break;

      { LOG(LogDebug) << "[SystemDataCompleter] Start completing data of " << game->Name(); }
      if (!IsRunning()) return;
      CompleteGameData(*game);
    }

    // Dequeue system
    for (;;)
    {
      mLocker.Lock();
      SystemData* system = !mSystemQueue.Empty() ? mSystemQueue.Pop() : nullptr;
      mLocker.UnLock();
      // Check whole system
      if (system ==  nullptr) break;

      { LOG(LogDebug) << "[SystemDataCompleter] Start checking hash of " << system->FullName(); }
      if (!IsRunning()) return;
      CompleteSystemData(*system);
    }
  }
}

void SystemDataCompleter::Push(FileData* game)
{
  mLocker.Lock();
  mFileQueue.Push(game);
  mLocker.UnLock();
  mSignal.Fire();
}

void SystemDataCompleter::Push(SystemData* system)
{
  mLocker.Lock();
  mSystemQueue.Push(system);
  mLocker.UnLock();
  mSignal.Fire();
}

void SystemDataCompleter::StopImmediately()
{
  Thread::Stop();
}

