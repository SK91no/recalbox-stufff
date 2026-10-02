//
// Created by bkg2k on 17/03/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <games/FileData.h>
#include <utils/os/system/IThreadPoolWorkerInterface.h>
#include <utils/os/system/ThreadPool.h>
#include <utils/cplusplus/StaticLifeCycleControler.h>
#include <scraping/scrapers/screenscraper/ScreenScraperSingleEngine.h>
#include <scraping/scrapers/recalbox/RecalboxEndPoints.h>
#include <utils/sync/SyncMessageSender.h>
#include <RecalboxConf.h>
#include <SDL_timer.h>
#include <scraping/ScraperSeamlessMessage.h>
#include "INotifySeamlessScrapeResult.h"

// Forward declaration
class SystemManager;

/*!
 * @brief Seamless scraping class
 */
class ScraperSeamless : public IThreadPoolWorkerInterface<FileData*, FileData*>
                      , public StaticLifeCycleControler<ScraperSeamless>
                      , public IConfiguration
                      , public ISyncMessageReceiver<ScrapeSeamlessMessage>
                      , public IScraperEngineStage
{
  public:
    //! Scraping "engines" count
    static constexpr int sScrapingEngineCount = 8;

    /*!
     * @brief Constructor
     */
    ScraperSeamless();

    //! Destructor
    ~ScraperSeamless() override = default;

    /*!
     * @brief Push a new candidate to scraping
     * @param game Game to check/scrape
     * @param interface Callback interface
     */
    void Push(FileData* game, IScraperEngineStage* interface);

    /*!
     * @brief Push game list
     * @param games game list
     * @param interface Callback interface
     * @param yetToGoInterface Global count interface
     */
    void Push(FileData::List& game, IScraperEngineStage* interface, INotifySeamlessScrapeResult* yetToGoInterface);

    //! Check if the given game is currently scraping
    bool IsScraping(FileData* game)
    {
      Mutex::AutoLock locker(mRunningScrapesLoker);
      return mRunningScrapes.contains(game);
    }

    /*!
     * @brief Get how long the given game is scraping
     * @param game Game to check
     * @return Elapsed time in ms, or -1 if the given game is not currently scraping
     */
    int HowLong(FileData* game)
    {
      Mutex::AutoLock locker(mRunningScrapesLoker);
      Information* result = mRunningScrapes.try_get(game);
      return result != nullptr ? (int)(SDL_GetTicks() - (*result).StartTick) : -1;
    }

  private:
    //! Storage class
    struct Information
    {
      unsigned long long StartTick;                  //!< Timestamp of scrape start
      IScraperEngineStage* Interface;                //!< Game callback interface
      INotifySeamlessScrapeResult* YetToGoInterface; //!< Global interface (scrape count only)
    };

    //! Recalbox configuration
    RecalboxConf& mConfiguration;

    //! Endpoint provider
    RecalboxEndPoints mEndPoints;
    //! Protected media set
    ProtectedSet mProtectedSet;

    //! Scraping "engines"
    ScreenScraperSingleEngine mEngines[sScrapingEngineCount];
    //! Busy Engines bitflag
    int mBusyEngines;
    //! Busy engines protection
    Mutex mBusyEngineLocker;

    //! Main thread synchronizer
    SyncMessageSender<ScrapeSeamlessMessage> mSender;

    //! Current scraping set
    HashMap<FileData*, Information> mRunningScrapes;
    //! Set protector
    Mutex mRunningScrapesLoker;
    //! Thread pool
    ThreadPool<FileData*, FileData*> mPool;

    /*!
     * @brief Get progress interface for the given game
     * @param game Game to retrieve interface from
     * @result Progress interface for the given game
     */
    IScraperEngineStage* GetInterface(FileData* game);

    /*!
     * @brief Get global count interface for the given game
     * @param game Game to retrieve interface from
     * @result Progress interface for the given game
     */
    INotifySeamlessScrapeResult* GetYetToGoInterface(FileData* game);

    /*!
     * @brief Remove the given game from internal list
     */
    void RemoveGame(FileData* game);

    /*!
     * @brief Get a free engine index
     * @return Engine index
     */
    int AllocateEngine()
    {
      Mutex::AutoLock locker(mBusyEngineLocker);
      for(int i = sScrapingEngineCount; --i >= 0;)
        if ((mBusyEngines & (1 << i)) == 0)
        {
          mBusyEngines |= 1 << i;
          return i;
        }
      return -1;
    }

    /*!
     * @brief Free engine by index
     * @param index Indix of engine to free
     */
    void FreeEngine(int index)
    {
      Mutex::AutoLock locker(mBusyEngineLocker);
      mBusyEngines &= ~(1 << index);
    }

    /*
     * IThreadPoolWorkerInterface implementation
     */

    /*!
     * @brief The main runner. Implement here the task to process a feed object
     * @param feed Feed object to process
     * @return Result Object
     */
    FileData* ThreadPoolRunJob(FileData*& feed) override;

    /*
     * ScreenScraperApis::IConfiguration implementation
     */

    //! Empty reset
    void ResetConfiguration() override {}

    //! Get screenscraper login
    [[nodiscard]] String GetLogin() const override { return String::Empty; }

    //! Get screenscraper password
    [[nodiscard]] String GetPassword() const override { return String::Empty; }

    //! Get recalbox token
    [[nodiscard]] String GetBearer() const override { return mConfiguration.GetRecalboxPrivateKey().ToTrim(); }

    //! Get favorite language
    [[nodiscard]] Languages GetFavoriteLanguage() const override { return LanguagesTools::GetScrapingLanguage(); };

    //! Get favorite region
    [[nodiscard]] Regions::GameRegions GetFavoriteRegion() const override { return mConfiguration.GetScreenScraperRegion(); }

    //! Get main image type
    [[nodiscard]] ScreenScraperEnums::ScreenScraperImageType GetImageType() const override { return mConfiguration.GetScreenScraperMainMedia(); }

    //! Get thumbnail image typ
    [[nodiscard]] ScreenScraperEnums::ScreenScraperBoxType GetBoxType() const override { return mConfiguration.GetScreenScraperBox(); }

    //! Check if video are required
    [[nodiscard]] ScreenScraperEnums::ScreenScraperVideoType GetVideo() const override
    {
      switch(mConfiguration.GetScreenScraperVideo())
      {
        case ScreenScraperEnums::ScreenScraperVideoType::Raw:
        case ScreenScraperEnums::ScreenScraperVideoType::Optimized: return ScreenScraperEnums::ScreenScraperVideoType::Optimized;
        case ScreenScraperEnums::ScreenScraperVideoType::None:
        default: break;
      }
      return ScreenScraperEnums::ScreenScraperVideoType::None;
    }

    //! Check if wheel are required
    [[nodiscard]] ScreenScraperEnums::ScreenScraperLogoType GetLogo() const override { return mConfiguration.GetScreenScraperLogo(); }

    //! Check if manual are required
    [[nodiscard]] bool GetWantManual() const override { return mConfiguration.GetScreenScraperWantManual(); }

    //! Check if maps are required
    [[nodiscard]] bool GetWantMaps() const override { return mConfiguration.GetScreenScraperWantMaps(); }

    //! Check if tips & tricks are required
    [[nodiscard]] bool GetWantTips() const override { return mConfiguration.GetScreenScraperWantTips(); }

    //! Check if p2k are required
    [[nodiscard]] bool GetWantP2K() const override { return mConfiguration.GetScreenScraperWantP2K(); }

    /*
     * ISyncMessageReceiver implementation
     */

    /*!
     * @brief Receive message
     * @param message Scraper message
     */
    void ReceiveSyncMessage(const ScrapeSeamlessMessage& message) override;

    /*
     * IScrapeEngineStage implementation
     */

    /*!
     * @brief Rezport scraping stage completion
     * @param game Target game
     * @param stage Last stage completed
     */
    void ScrapingStageCompleted(FileData* game, Stage stage, MetadataType changes) override;
};



