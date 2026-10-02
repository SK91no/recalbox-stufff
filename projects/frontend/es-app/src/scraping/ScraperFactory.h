//
// Created by bkg2k on 04/12/2019.
//
#pragma once

#include <utils/String.h>
#include <scraping/scrapers/IScraperEngine.h>
#include <utils/cplusplus/StaticLifeCycleControler.h>
#include <scraping/ScraperType.h>
#include <scraping/scrapers/IScraperEngineFreezer.h>

class ScraperFactory : public StaticLifeCycleControler<ScraperFactory>
{
  public:
    /*!
     * @brief Default constructor
     */
    ScraperFactory()
      : StaticLifeCycleControler("ScraperFactory")
      , mSystems(256)
      , mMethod(ScrapingMethod::AllIfNothingExists)
    {
    }

    /*!
     * @brief Default destructor
     */
    ~ScraperFactory();

    /*
     * Persistant data management
     */

    /*!
     * @brief Add a system to the persistent list
     * @param system System to add
     */
    void AddSystem(SystemData* system) { if (!mSystems.Contains(system)) mSystems.Add(system); }

    /*!
     * @brief Check if the persistent system list contains the given system
     * @param system System to check
     * @return True iof the list contains the given system, false otherwise
     */
    bool HasSystem(SystemData* system) const { return mSystems.Contains(system); }

    /*!
     * @brief Check if the persistent system list is not empty
     * @return True iof the list is not empty, false otherwise
     */
    [[nodiscard]] bool HasSystems() const { return !mSystems.Empty(); }

    /*!
     * @brief Remove a system from the persistent list
     * @param system System to remove
     */
    void RemoveSystem(SystemData* system) { mSystems.Remove(system); }

    /*!
     * @brief Get persistent system list
     * @return System list
     */
    [[nodiscard]] const Array<SystemData*>& SystemList() const { return mSystems; }

    //! Set pesistent scraping method
    void SetScrapingMethod(ScrapingMethod method) { mMethod = method; }

    //! Get persistent scraping method
    [[nodiscard]] ::ScrapingMethod ScrapingMethod() const { return mMethod; }

    /*
     * Scraper management
     */

    /*!
     * @brief Check if the given scraper need credentials
     * @param type Scraper type
     * @return True if the given scraper needs credentials, false otherwise
     */
    static bool HasCredentials(ScraperType type)
    {
      switch(type)
      {
        case ScraperType::ScreenScraper: return true;
        case ScraperType::Recalbox:
        case ScraperType::TheGameDB:
        default: break;
      }
      return false;
    }

    static String GetLogin(ScraperType type)
    {
      switch(type)
      {
        case ScraperType::ScreenScraper: return RecalboxConf::Instance().GetScreenScraperLogin();
        case ScraperType::Recalbox:
        case ScraperType::TheGameDB:
        default: break;
      }
      return String::Empty;
    }

    static String GetPassword(ScraperType type)
    {
      switch(type)
      {
        case ScraperType::ScreenScraper: return RecalboxConf::Instance().GetScreenScraperPassword();
        case ScraperType::Recalbox:
        case ScraperType::TheGameDB:
        default: break;
      }
      return String::Empty;
    }

    static void SetLogin(ScraperType type, const String& login)
    {
      switch(type)
      {
        case ScraperType::ScreenScraper: RecalboxConf::Instance().SetScreenScraperLogin(login).Save(); break;
        case ScraperType::Recalbox:
        case ScraperType::TheGameDB:
        default: break;
      }
    }

    static void SetPassword(ScraperType type, const String& password)
    {
      switch(type)
      {
        case ScraperType::ScreenScraper: RecalboxConf::Instance().SetScreenScraperPassword(password).Save(); break;
        case ScraperType::Recalbox:
        case ScraperType::TheGameDB:
        default: break;
      }
    }

    /*!
     * @brief Get scraper instance for the given type
     * @param scraper Scraper type
     * @return Scraper instance
     */
    IScraperEngine* GetScraper(ScraperType scraper, IScraperEngineFreezer* freezer);

    /*!
     * @brief Get scraper list type/name
     * @return HashMap type/name
     */
    static const HashMap<ScraperType, String>& GetScraperList();

    /*
     * Don't know what those methods are f***** doing here :)
     */

    static void ExtractFileNameUndecorated(FileData& game);

    static void ExtractRegionFromFilename(FileData& game);

  private:
    // Persistant data
    Array<SystemData*> mSystems; //!< Selected systems
    ::ScrapingMethod mMethod;      //!< Scraping method

    //! Hold scraper instances
    HashMap<ScraperType, IScraperEngine*> mScrapers;

    /*!
     * @brief Get or create new scraper instance and store it in the map
     * @param type Scraper type to get/create
     * @param freezer Freeze interface
     * @return IScraperEngine instance
     */
    IScraperEngine* Get(ScraperType type, IScraperEngineFreezer* freezer);
};

