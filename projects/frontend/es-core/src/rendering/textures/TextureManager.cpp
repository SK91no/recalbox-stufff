//
// Created by thierry.imbert on 10/01/2020.
//

#include "utils/os/system/Mutex.h"
#include "recalbox/RecalboxSystem.h"
#include "../../../../data/Resources.h"
#include "TextureManager.h"
#include "utils/Sizes.h"
#include "utils/Files.h"
#include <Renderer.h>
#include <sys/sysinfo.h>

TextureManager::TextureManager()
  : StaticLifeCycleControler<TextureManager>("TextureManager")
  , mPool(this, "Texture", true)
  , mTexturesCache(128)
  , mGamelistCacheControl(nullptr)
{
  // Run the thread pool for texture loading
  mPool.Run(-1, true);

  { LOG(LogInfo) << "[TextureManager] Texture manager created."; }
}

TextureManager::~TextureManager()
{
  String::List keys;
  for (const auto& kv : mTexturesCache)
  {
    { LOG(LogTrace) << "[TextureManager] Undestroying texture Texture " << kv.first; }
    keys.push_back(kv.first);
  }
  for (const String& k : keys)
  {
    { LOG(LogTrace) << "[TextureManager] Try to destroy " << k; }
    mTexturesCache.erase(k);
  }
}

RenderingError TextureManager::RequestTextureLoading(Texture& texture, bool noDelay)
{
  if (texture.mTexture->Status() != TextureHolder::Status::Unloaded)
    return RenderingError::NoError;

  if (mPool.PendingJobs() >= mPool.WorkerCount() * 2)
    return RenderingError::NoError;

  // Set the new status to avoid re-entry issues
  texture.mTexture->StartLoading();

  // Load immediately?
  if (noDelay || texture.mTexture->MustLoadImmediately())
    return ThreadPoolRunJob(texture);

  // Delayed load
  mPool.PushFeed(texture);
  return RenderingError::NoError;
}

namespace
{
  //! Decode a TextureHolder::Tick() value (packed as sec<<24 | nsec>>8) into milliseconds.
  //! sec and nsec live in disjoint bit ranges, so the decode is exact to ~256ns.
  long long TickToMs(unsigned long long tick)
  {
    long long sec = (long long)(tick >> 24);
    long long ns  = (long long)((tick & 0xFFFFFFu) << 8);
    return sec * 1000 + ns / 1000000;
  }
}

long long TextureManager::UnloadTheOldest(TextureUnload mode)
{
  // Reactive entry point (real OOM): free the genuinely oldest GPU texture, with NO recency
  // protection - when an allocation has actually failed we must free something, even a visible
  // texture (a one-frame flicker beats a failed/blank texture).
  return EvictOldest(mode, 0);
}

long long TextureManager::EvictOldest(TextureUnload mode, long long protectMs)
{
  LOG(LogTrace) << "Requested to unload oldest texture (" << (mode == TextureUnload::Full ? "GPU+CPU" : "GPU")
                << ", protect<" << protectMs << "ms)";
  bool alsoCpu = (mode == TextureUnload::Full);

  // Critical section: Read/write maps. Held only for instant operations (scan + non-blocking
  // try-evicts): we NEVER block on a per-texture lock here, otherwise a load holding that lock
  // across slow I/O would freeze every other user of mMutexStorage (this caused the deadlock).
  Mutex::AutoLock lock(mMutexStorage);

  unsigned long long nowTick = TextureHolder::Tick();
  long long nowMs = TickToMs(nowTick);

  HashSet<TextureHolder*> busy; // Textures skipped this round because currently in use
  for(;;)
  {
    // Always target the oldest texture that actually holds a GPU texture: freeing GPU memory
    // is the goal (it is the dominant consumer, and shares system RAM on unified-memory boards).
    unsigned long long oldest = nowTick; // Initialize to current date time
    TextureHolder* oldestTexture = nullptr;
    for (TextureHolder* texture : mTextures)
      if (texture->GPUConsummed() > 0 && texture->LastUse() < oldest && !busy.contains(texture))
      {
        // Recency guard: never evict a texture Touch()ed within the last protectMs. Such a texture
        // is almost certainly still on screen; evicting it only forces a re-upload next frame, which
        // re-raises memory and gets it evicted again -> the stroboscopic flicker. When everything is
        // recent we are simply over-committed: better to overshoot the threshold than to thrash.
        if (protectMs > 0 && (nowMs - TickToMs(texture->LastUse())) < protectMs) continue;
        oldest = texture->LastUse();
        oldestTexture = texture;
      }

    if (oldestTexture == nullptr)
    {
      LOG(LogTrace) << "NO Candidate found";
      return 0; // Nothing (more) unloadable
    }

    long long freed = oldestTexture->TryEvict(alsoCpu);
    if (freed >= 0)
    {
      LOG(LogTrace) << "BEST Candidate found! " << oldestTexture->FilePath().ToString();
      return freed;
    }

    // Busy texture: skip it and look for the next oldest
    busy.insert(oldestTexture);
  }
}

RenderingError TextureManager::ThreadPoolRunJob(Texture& feed)
{
  #ifdef BETA
  if (Log::ReportingLevel(LogType::Generic) >= LogLevel::LogDebug)
    ShowStatistics();
  #endif

  //if (mTextures.size() > 10)
  //  UnloadTheOldest(false);

  // NOTE: proactive memory watching is NOT done here anymore. WatchMemory() is driven once per frame
  // by the main loop (MainRunner::MainLoop) on the GL context thread - the only thread allowed to
  // create/destroy GL textures. The per-job OOM handling below (OutOfMemory / OutOfGPUMemory) remains
  // as the hard last-resort safety net when an actual allocation fails mid-load.

  // Actually load the texture content and try to bind it
  // either from the thread pool, or from direct calls
  // Check CPU/GPU OOM and process unloading of oldest texture until
  // enough space has been freed
  if (feed.mTexture->Status() < TextureHolder::Status::Loading)
    for(;;)
    {
      // Load is ok?
      switch(feed.mTexture->AsIUnloadable().Load())
      {
        case RenderingError::NoError: return RenderingError::NoError;
        case RenderingError::OutOfMemory:
        {
          FreeAllCPUResources();
          break;
        }
        case RenderingError::OutOfGPUMemory:
        {
          // Try to free some memory - keep CPU caches for fast re-upload
          if (!UnloadTheOldest(TextureUnload::GpuOnly))
            return RenderingError::OutOfGPUMemory; // Nothing more to free
          break;
        }
        case RenderingError::NoResource: return RenderingError::NoResource;
      }

      #ifdef BETA
      if (Log::ReportingLevel(LogType::Generic) >= LogLevel::LogDebug)
        ShowStatistics();
      #endif
    }

  // Already loaded or in loading process
  return RenderingError::NoError;
}

Texture TextureManager::Create(const Path& path, int targetWidth, int targetHeight, TextureHolder::Properties properties)
{
  // Free texture have empty path
  bool noCache = path.IsEmpty() ||
    ((properties & (TextureHolder::Properties::NoCache | TextureHolder::Properties::Mutable | TextureHolder::Properties::Free)) != 0);

  // Cached texture?
  if (!noCache)
  { // Protect storage
    Mutex::AutoLock lock(mMutexStorage);
    Texture* texture = mTexturesCache.try_get(path.ToString());
    if (texture != nullptr) return *texture;
  }

  // Create texture
  // Note: the holder registers itself into mTextures from its constructor (TextureHolder::TextureHolder)
  Texture newTexture(new TextureHolder(path, targetWidth, targetHeight, properties, *this));

  // Add to map?
  if (!noCache)
  {
    { // Protect storage
      Mutex::AutoLock lock(mMutexStorage);
      mTexturesCache.insert(path.ToString(), newTexture);
    }
  }

  return newTexture;
}


void TextureManager::FreeAllCPUResources()
{
  Mutex::AutoLock lock(mMutexStorage);
  int skipped = 0;
  for (TextureHolder* texture : mTextures)
    if (!texture->TryFreeCPUCaches(false)) // Non-blocking: never wait on a busy texture
      skipped++;
  if (skipped != 0) { LOG(LogTrace) << "[TextureManager] FreeAllCPUResources skipped " << skipped << " busy texture(s)"; }
}

int TextureManager::DropRedundantCPUCaches()
{
  // Emergency CPU relief that is safe to run from ANY thread and does not destroy work in progress:
  // only the CPU caches of textures already backed by a GPU copy are dropped (they keep displaying,
  // they merely lose the fast re-upload cache). Busy and not-yet-uploaded textures are skipped.
  Mutex::AutoLock lock(mMutexStorage);
  int dropped = 0, skipped = 0;
  for (TextureHolder* texture : mTextures)
    if (!texture->TryFreeCPUCaches(true)) skipped++; else dropped++;
  LOG(LogTrace) << "[TextureManager] DropRedundantCPUCaches: processed " << dropped << ", skipped " << skipped << " busy";
  return dropped;
}

void TextureManager::FreeAllGPUResources()
{
  Mutex::AutoLock lock(mMutexStorage);
  for (TextureHolder* texture : mTextures)
    texture->AsIUnloadable().Unload();
}

void TextureManager::FreeAllResources()
{
  FreeAllCPUResources();
  FreeAllGPUResources();
}

bool TextureManager::Exists(const Path& path)
{
  // Internal resource?
  if (path.ToChars()[0] == ':')
  {
    // Embedded
    const auto& it = res2hMap.find(path.ToString());
    return (it != res2hMap.end());
  }
  if (path.StartWidth("http://") || path.StartWidth("https://")) return true;
  return path.Exists();
}

void TextureManager::ShowStatistics()
{
  long long estimatedGPUMemory = 0;
  long long estimatedCPUMemory = 0;
  int countLoaded = 0;
  int countLoading = 0;
  int countUnloaded = 0;
  int countSVG = 0;
  int countInvalid = 0;
  { // Critical section: Read/write maps
    Mutex::AutoLock lock(mMutexStorage);

    for (auto& texture : mTexturesCache)
      if (!texture.second.Valid())
        countInvalid++;

    for (TextureHolder* texture : mTextures)
    {
      countLoaded   += texture->Status() == TextureHolder::Status::Loaded   ? 1 : 0;
      countUnloaded += texture->Status() == TextureHolder::Status::Unloaded ? 1 : 0;
      countLoading  += texture->Status() == TextureHolder::Status::Loading || texture->Status() == TextureHolder::Status::Queued  ? 1 : 0;
      countSVG      += texture->IsSVG() ? 1 : 0;
      estimatedCPUMemory += texture->CPUConsummed();
      estimatedGPUMemory += texture->GPUConsummed();
    }
  }
  LOG(LogInfo) << "[Texture Statistics] Total: " << mTextures.size()
               << " Cached/Invalids: " << mTexturesCache.size() << '/' << countInvalid
               << " Loaded/Loading/Unloaded: " << countLoaded << '/' << countLoading << '/' << countUnloaded
               << " SVG: " << countSVG
               << " CPU: " << Sizes(estimatedCPUMemory).ToHumanSize()
               << " GPU: " << Sizes(estimatedGPUMemory).ToHumanSize();
}

void TextureManager::TextureHolderCreated(TextureHolder* newTexture)
{
  // Critical section: Read/write maps
  Mutex::AutoLock lock(mMutexStorage);
  mTextures.insert(newTexture);
}

void TextureManager::TextureHolderDestroyed(TextureHolder* textureDestroyed)
{
  // Critical section: Read/write maps
  Mutex::AutoLock lock(mMutexStorage);
  if (!textureDestroyed->IsNotCached())
    if (!mTexturesCache.erase(textureDestroyed->FilePath().ToString()))
      { LOG(LogTrace) << "Texture path " << textureDestroyed->FilePath() << " not found in cache !"; }
  mTextures.erase(textureDestroyed);
}

long long TextureManager::TotalCPUMemory()
{
  long long result = 0;
  // Critical section: Read/write maps
  Mutex::AutoLock lock(mMutexStorage);
  for (TextureHolder* texture : mTextures)
    result += texture->CPUConsummed();
  return result;
}

long long TextureManager::TotalGPUMemory()
{
  long long result = 0;
  // Critical section: Read/write maps
  Mutex::AutoLock lock(mMutexStorage);
  for (TextureHolder* texture : mTextures)
    result += texture->GPUConsummed();
  return result;
}

#include <sys/resource.h>

long long TextureManager::ReadMemAvailable()
{
  // Kernel estimate of memory truly available for new allocations (free + reclaimable page cache,
  // slab, etc.). Far more meaningful than si.freeram, which excludes the page cache.
  for (const String& line : Files::LoadAllFileLines(Path("/proc/meminfo")))
    if (line.rfind("MemAvailable:", 0) == 0)
    {
      long long kb = 0;
      for (char c : line) if (c >= '0' && c <= '9') kb = kb * 10 + (c - '0');
      return kb << 10; // kB -> bytes
    }
  return -1; // Not available
}

void TextureManager::WatchMemory()
{
  // Called once per frame from the main loop (MainRunner::MainLoop), on the GL context thread - and
  // ONLY from there. Workers no longer call it: this removes the off-context-GL hazard and the
  // cross-thread contention on mMutexStorage that used to stall the download pipeline, and lets us
  // freely evict GPU textures here. Eviction still uses non-blocking try-locks (TryEvict /
  // TryFreeCPUCaches) because a worker holds each texture's own lock while loading/downloading it.
  if (!Renderer::IsContextThread()) return; // Safety net: never touch GL off the context thread

  struct sysinfo si {};
  if (sysinfo(&si) != 0)
  {
    { LOG(LogError) << "[TextureManager] Cannot get RAM values !"; }
    return;
  }

  // Values are expressed in multiples of si.mem_unit (bytes)
  long long total = (long long)si.totalram * (long long)si.mem_unit;

  // Use the kernel's "available" estimate rather than the strictly-free RAM: si.freeram excludes
  // the page cache, which is large on these boards and instantly reclaimable. Using it would fire
  // false emergencies that can never clear (the cache refills at once). Fall back to si.freeram
  // when /proc/meminfo cannot be read.
  long long available = ReadMemAvailable();
  if (available < 0) available = (long long)si.freeram * (long long)si.mem_unit;

  // Available RAM safety margins. On unified-memory boards (Pi...), GPU textures live in this same
  // RAM, so freeing GPU textures (not CPU caches) is what actually relieves the pressure.
  bool small = (total >> 30) < 1; // System with less than 1Go
  long long level1Warning   = small ? (320 << 20) : (1 << 29);   // 320Mb / 512Mb : free oldest GPU texture, keep CPU cache
  long long level2High      = small ? (224 << 20) : (384 << 20); // 224Mb / 384Mb : free oldest texture entirely (GPU + CPU)
  long long level3Emergency = small ? (128 << 20) : (1 << 28);   // 128Mb / 256Mb : flush all CPU caches + free GPU down to level2

  { LOG(LogTrace) << "[TextureManager] Available/Used/Total RAM values : " << Sizes(available).ToHumanSize() << '/' << Sizes(total - available).ToHumanSize() << '/' << Sizes(total).ToHumanSize(); }

  // Recency protection: never proactively evict a texture used within the last second - it is almost
  // certainly still on screen, and evicting it would only cause it to be re-uploaded next frame
  // (the stroboscopic flicker). The reactive OOM path (UnloadTheOldest, protect=0) ignores this.
  static constexpr long long kRecentProtectMs = 1000;

  // Hysteresis: each tier is a Schmitt trigger with a +/-16 MB band (32 MB total) around its
  // threshold. A tier ARMS only once available drops below level-16MB, and stays armed (keeps
  // evicting every frame) until available climbs back above level+16MB. Without this, a tier would
  // re-fire frame after frame right at its boundary -> oscillation around the threshold.
  static constexpr long long H = 16 << 20; // 16 MB half-band
  static bool sWarnArmed = false, sHighArmed = false, sEmergArmed = false;
  sWarnArmed  = sWarnArmed  ? (available < level1Warning   + H) : (available < level1Warning   - H);
  sHighArmed  = sHighArmed  ? (available < level2High      + H) : (available < level2High      - H);
  sEmergArmed = sEmergArmed ? (available < level3Emergency + H) : (available < level3Emergency - H);

  if (sEmergArmed)
  {
    // Emergency: drop the redundant CPU caches (those already backed by a GPU copy - preserves work
    // in progress), then unload the oldest non-recent GPU textures until enough has been freed.
    //{ LOG(LogWarning) << "[TextureManager] Emergency memory level reached - dropping redundant CPU caches and oldest GPU textures"; }
    int cpuDropped = DropRedundantCPUCaches();

    // Drive the loop by the textures' own GPU accounting, NOT by re-reading the OS counters: freed
    // memory is not reflected promptly/reliably there (deferred GL free, glibc arena retention),
    // which would otherwise unload EVERY texture and freeze the UI.
    // Bounded batch: a transient dip must not evict the whole visible set in a single frame (that
    // thrashes: evict -> re-upload next frame -> evict again). We free a few of the oldest GPU
    // textures per frame; since this runs every frame, real pressure clears over a few frames.
    static constexpr int sMaxEvictionsPerFrame = 8;
    long long toReclaim = (level2High + H) - available; // hysteresis target: climb back above the high band
    long long reclaimed = 0;
    int unloaded = 0;
    while (toReclaim > 0 && unloaded < sMaxEvictionsPerFrame)
    {
      long long freed = EvictOldest(TextureUnload::GpuOnly, kRecentProtectMs);
      if (freed <= 0) break; // Nothing (non-recent) left to unload
      toReclaim -= freed;
      reclaimed += freed;
      unloaded++;
    }
    if (unloaded != 0)
      LOG(LogTrace) << "[TextureManager] Emergency GPU eviction: unloaded " << unloaded
                    << " texture(s), reclaimed " << Sizes(reclaimed).ToHumanSize()
                    << (toReclaim > 0 ? " (target not fully reached)" : "");

    // Once there is no more CPU cache to flush (DropRedundantCPUCaches found nothing this frame), we
    // have squeezed everything we can out of the textures. Escalate to the next, deeper level of RAM
    // relief: tear down the least-recently-used gamelist view - ONE per call, so that sustained
    // pressure peels them off one frame at a time instead of nuking the whole set at once. Done last
    // and never on the current view (handled by the callback), so a transient dip stays cheap.
    if (cpuDropped == 0 && mGamelistCacheControl != nullptr)
      if (mGamelistCacheControl->DestroyOldestGameListView())
        { LOG(LogWarning) << "[TextureManager] Emergency: freed the oldest gamelist view to reclaim RAM"; }
  }
  else if (sHighArmed)
    (void)EvictOldest(TextureUnload::Full, kRecentProtectMs);    // Free oldest non-recent texture entirely (GPU + CPU)
  else if (sWarnArmed)
    (void)EvictOldest(TextureUnload::GpuOnly, kRecentProtectMs); // Free oldest non-recent GPU texture, keep CPU cache
}

int TextureManager::ThreadPoolNextFeedIndex(const std::vector<IndexedFeed<Texture>>& list)
{
  int selectedIndex = 0;
  unsigned long long int mostRecentUse = 0;
  for(int i = (int)list.size(); --i >= 0;)
    if (unsigned long long int lastUse = list[i].Feed.mTexture->LastUse(); lastUse > mostRecentUse)
    {
      mostRecentUse = lastUse;
      selectedIndex = i;
    }
  return selectedIndex;
}

void TextureManager::ConfigureHttpClient(const String& url, HttpClient& client)
{
  // Dedicated leaf mutex - NOT mMutexStorage (see mHttpMutex declaration): this runs while
  // tex.mMutex and IImage::mLocker are held, so taking mMutexStorage here would deadlock.
  Mutex::AutoLock locker(mHttpMutex);
  for(IHttpConfiguration* configurator : mHttpConfigurationFilter)
    if (configurator != nullptr)
      configurator->ConfigureHttpClient(url, client);
}
