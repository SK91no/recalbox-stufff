//
// Created by Bkg2k on 10/01/2020.
//
#pragma once

#include "utils/os/system/IThreadPoolWorkerInterface.h"
#include "utils/os/system/ThreadPool.h"
#include "utils/storage/HashMap.h"
#include "TextureHolder.h"
#include "Texture.h"
#include "utils/cplusplus/StaticLifeCycleControler.h"
#include "utils/storage/Set.h"
#include "ITextureCacheControl.h"
#include "IGamelistCacheControl.h"

class TextureManager : public StaticLifeCycleControler<TextureManager>
                     , public IThreadPoolWorkerInterface<Texture, RenderingError>
                     , public ITextureCacheControl
                     , public IHttpConfiguration
{
  private:
    //! Thread pool
    ThreadPool<Texture, RenderingError> mPool;

    //! ALL Textures
    HashSet<TextureHolder*> mTextures;

    //! Path to Texture
    HashMap<std::string, Texture> mTexturesCache;

    //! Storage protection
    Mutex mMutexStorage;

    /*!
     * @brief Read the kernel's "available memory" estimate (free + reclaimable caches) in bytes
     * @return Available memory in bytes, or -1 if /proc/meminfo cannot be read
     */
    static long long ReadMemAvailable();

    //! Http filtering/configuration
    HashSet<IHttpConfiguration*> mHttpConfigurationFilter;

    //! Http filter protection - dedicated leaf mutex.
    //! MUST NOT be mMutexStorage: ConfigureHttpClient() is called from deep inside a texture load
    //! (IImage::LoadResource), while tex.mMutex and IImage::mLocker are already held. Reusing
    //! mMutexStorage there would invert the lock hierarchy and deadlock against FreeAllCPUResources.
    Mutex mHttpMutex;

    //! Optional gamelist eviction callback (ViewController). Touched only from the GL/main thread
    //! (WatchMemory and (un)registration), so no dedicated lock is required.
    IGamelistCacheControl* mGamelistCacheControl;

    /*!
     * @brief Unload the oldest GPU-resident texture to free memory (reactive OOM path, no recency
     * protection - frees even a visible texture because an allocation has actually failed)
     * @param mode GpuOnly = keep the CPU cache, Full = drop the CPU cache too
     * @return Amount of memory freed in bytes (0 if no more unloadable texture)
     */
    [[nodiscard]] long long UnloadTheOldest(TextureUnload mode) final;

    /*!
     * @brief Unload the oldest GPU-resident texture, skipping any texture Touch()ed within the last
     * protectMs milliseconds (those are almost certainly still on screen - evicting them causes the
     * stroboscopic flicker). Used by the proactive per-frame WatchMemory tiers.
     * @param mode GpuOnly = keep the CPU cache, Full = drop the CPU cache too
     * @param protectMs Do not evict textures used more recently than this (0 = no protection)
     * @return Amount of memory freed in bytes (0 if no unprotected unloadable texture)
     */
    [[nodiscard]] long long EvictOldest(TextureUnload mode, long long protectMs);

    /*
     * IThreadPoolWorkerInterface implementation
     */

    /*!
     * @brief The main runner. Implement here the task to process a feed object
     * @param feed Feed object to process
     * @return Result Object
     */
    RenderingError ThreadPoolRunJob(Texture& feed) final;

    /*!
     * @brief Select next texture to load
     * @param list List into which to select the next item
     * @return Next item index
     */
    int ThreadPoolNextFeedIndex(const std::vector<IndexedFeed<Texture>>& list) final;

    /*!
     * @brief Called asap by the main thread when a job complete, regarding the tick duration
     * @param completed Currently completed jobs count
     * @param total Total jobs
     */
    void ThreadPoolTick(int completed, int total) override { (void)completed; (void)total; };

    /*
     * Children management
     */

    /*!
     * @brief Called by TextureHolder constructor
     * @param newTexture texture being created
     */
    void TextureHolderCreated(TextureHolder* newTexture);

    /*!
     * @brief Called from TextureHolder destructor
     * @param textureDestroyed texture being destroyed
     */
    void TextureHolderDestroyed(TextureHolder* textureDestroyed);

    /*
     * IHttpProvider
     */

    /*!
     * @brief Configure the given http client
     */
    void ConfigureHttpClient(const String& url, HttpClient& client) final;

    friend class TextureHolder;

  public:
    /*!
     * @brief Default constructor - only ONE instance allowed
     */
    TextureManager();

    /*!
     * @brief Desfault destructor
     */
    ~TextureManager() override;

    /*!
     * @brief Create a new texture
     * @param path File to load
     * @param targetWidth target width (for sizables images only)
     * @param targetHeight target height (for sizables images only)
     * @param properties Texture properties
     * @return Texture
     */
    Texture Create(const Path& path, int targetWidth, int targetHeight, TextureHolder::Properties properties = TextureHolder::Properties::None);

    /*!
     * @brief Create a new texture
     * @param path File to load
     * @param properties Texture properties
     * @return Texture
     */
    Texture Create(const Path& path, TextureHolder::Properties properties = TextureHolder::Properties::None)
    {
      return Create(path, 0, 0, properties);
    }

    /*!
     * @brief Request texture loading, immediate or delayed
     * @param texture Texture to load
     * @param noDelay Load immediately, even if the texture has no immediateLoad property
     */
    RenderingError RequestTextureLoading(Texture& texture, bool noDelay = false);

    /*!:
     * @brief Check if a texture resource exists, either embedded or real file
     * @param path path to resource
     * @return True if the resource is available
     */
    static bool Exists(const Path& path);

    /*
     * Memory global management
     */

    /*!
     * @brief Free all possible CPU caches (non-blocking: busy textures are skipped)
     */
    void FreeAllCPUResources();

    /*!
     * @brief Drop only the redundant CPU caches (those of textures already backed by a GPU copy).
     * Non-blocking and work-in-progress safe: busy and not-yet-uploaded textures are skipped.
     * @return Number of CPU caches actually dropped this call (0 = nothing left to flush)
     */
    int DropRedundantCPUCaches();

    /*!
     * @brief Free all possible GL texture
     */
    void FreeAllGPUResources();

    /*!
     * @brief Free both CPU caches and GL textures
     */
    void FreeAllResources();

    /*
     * Statistics
     */

    /*!
     * @brief Return total CPU consumed by texture data
     * @return Total CPU memory in byte
     */
    long long TotalCPUMemory();

    /*!
     * @brief Return total GPU consumed by texture data
     * @return Total GPU memory in byte
     */
    long long TotalGPUMemory();

    /*!
     * @brief Watch system memory and unload textures in tiers when free RAM gets low.
     * On unified-memory boards (Pi...), GPU textures share the system RAM, so GPU textures
     * are freed first while the CPU cache is kept for fast re-upload (see TextureUnload).
     */
    void WatchMemory();

    /*!
     * @brief For debug purpose only
     */
    void ShowStatistics();

    /*!
     * @brief Register a configurator for the given url part
     * @param httpConfigure Http configuration interface
     */
    void RegisterHttpFilter(IHttpConfiguration* httpConfigure) { Mutex::AutoLock locker(mHttpMutex); mHttpConfigurationFilter.insert(httpConfigure); }

    /*!
     * @brief Unregister a previously assigned filter
     * @param httpConfigure Http configuration interface
     */
    void UnregisterHttpFilter(IHttpConfiguration* httpConfigure) { Mutex::AutoLock locker(mHttpMutex); mHttpConfigurationFilter.erase(httpConfigure); }

    /*!
     * @brief Register the gamelist eviction callback (ViewController), used by WatchMemory as a last
     * resort to free a whole gamelist view once the texture caches are exhausted.
     * @param control Gamelist cache control interface
     */
    void RegisterGamelistCacheControl(IGamelistCacheControl* control) { mGamelistCacheControl = control; }

    /*!
     * @brief Unregister the gamelist eviction callback if it matches the registered one.
     * @param control Gamelist cache control interface to remove
     */
    void UnregisterGamelistCacheControl(IGamelistCacheControl* control) { if (mGamelistCacheControl == control) mGamelistCacheControl = nullptr; }
};

