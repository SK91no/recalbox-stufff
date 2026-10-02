//
// Created by bkg2k on 26/03/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <guis/GuiInfoPopupBase.h>
#include "scraping/scrapers/IScraperEngineStage.h"
#include "scraping/INotifySeamlessScrapeResult.h"
#include <components/ComponentGrid.h>
#include <WindowManager.h>

// Forward declaration
class TextComponent;
class ProgressBarComponent;
class ImageComponent;

class GuiInfoPopupSeamlessScraper : public StaticLifeCycleControler<GuiInfoPopupSeamlessScraper>
                                  , private IScraperEngineStage
                                  , private INotifySeamlessScrapeResult
                                  , public GuiInfoPopupBase
{
  public:
    /*!
     * @brief Constructor
     * @param window Window manager
     */
    explicit GuiInfoPopupSeamlessScraper(WindowManager& window);

    //! Add game to scrape
    void AddGames(FileData::List& games);

  protected:
    float AddComponents(WindowManager& window, ComponentGrid& grid, float maxWidth, float maxHeight, int paddingX, int paddingY) override;

  private:
    //! Text template
    static String mTextTemplate;
    //! Text template CRT
    static String mTextTemplateCRT;

    //! Message component
    std::shared_ptr<TextComponent> mText;
    //! Icon component
    std::shared_ptr<TextComponent> mIcon;
    //! Image component
    std::shared_ptr<ImageComponent> mImage;
    //! No Image
    std::shared_ptr<ImageComponent> mNoImage;

    //! Current count
    int mCount;

    /*!
     * @brief Refresh game informations
     * @param game FileData
     */
    void SetScrapeResult(const FileData& game);

    /*!
     * @brief Scraping completed
     */
    void SetScrapeEnd();

    /*
     * IScraperEngineStage implementation
     */

    /*!
     * @brief Report scraping stage completion
     * @param game Target game
     * @param stage Last stage completed
     */
    void ScrapingStageCompleted(FileData* game, Stage stage, MetadataType changes) final;

    /*
     * Global seamless interface
     */

    /*!
     * @brief Notify a game has been scraped
     * @param count Number of queued or running scrapes
     * 0 means no more scrape running and no more scape queued
     */
    void ScrapeCountYetToGo(int count) final;
};



