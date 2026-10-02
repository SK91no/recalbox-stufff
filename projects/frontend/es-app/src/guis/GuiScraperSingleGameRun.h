#pragma once

#include <scraping/scrapers/IScraperEngine.h>
#include <components/ButtonComponent.h>
#include "guis/Gui.h"
#include "components/ScraperSearchComponent.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"
#include "recalbox/RecalboxStorageWatcher.h"
#include <components/MenuComponent.h>
#include <recalbox/RecalboxSystem.h>
#include <scraping/ScraperFactory.h>
#include <usernotifications/NotificationManager.h>

template<class ContextType> class GuiScraperSingleGameRun : public Gui
                                                          , public INotifyScrapeResult
{
  public:
    class IScrapingComplete
    {
      public:
        //! Default destructor
        virtual ~IScrapingComplete() = default;

        /*!
         * @brief Notifification of a game scraping end
         * @param game Scrapped games
         * @param changedMetadata Changed metadata
         */
        virtual void ScrapingComplete(FileData& game, MetadataType changedMetadata, ContextType contextData) = 0;
    };

    explicit GuiScraperSingleGameRun(WindowManager&window, SystemManager& systemManager, FileData& game, ContextType contextData, IScrapingComplete* notifier)
      : Gui(window)
      , mSystemManager(systemManager)
      , mScraper(ScraperFactory::Instance().GetScraper(RecalboxConf::Instance().GetScraperSource(), nullptr))
      , mGame(game)
      , mNotifier(notifier)
      , mContextData(contextData)
      , mGrid(window, Vector2i(1, 7))
      , mBox(window, Path(":/frame.png"))
    {
      const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

      mBox.setImagePath(menuTheme.Background().path);
      mBox.setCenterColor(menuTheme.Background().color);
      mBox.setEdgeColor(menuTheme.Background().color);
      addChild(&mBox);
      addChild(&mGrid);

      // row 0 is a spacer

      mGameName = std::make_shared<TextComponent>(mWindow, game.RomPath().Filename().UpperCase(), menuTheme.Text().font, menuTheme.Text().color, Alignment::Center);
      mGrid.setEntry(mGameName, Vector2i(0, 1), false, true);

      // row 2 is a spacer

      mSystemName = std::make_shared<TextComponent>(mWindow, game.System().FullName().ToUpperCase(), menuTheme.SmallText().font, menuTheme.SmallText().color, Alignment::Center);
      mGrid.setEntry(mSystemName, Vector2i(0, 3), false, true);

      // row 4 is a spacer

      // ScraperSearchComponent
      mSearch = std::make_shared<ScraperSearchComponent>(window, Renderer::Instance().DisplayHeightAsInt()<=576);
      mGrid.setEntry(mSearch, Vector2i(0, 5), false);

      // buttons
      mButton = std::make_shared<ButtonComponent>(mWindow, _("CANCEL"), _("CANCEL"), [this] { Close(); });
      std::vector< std::shared_ptr<ButtonComponent> > buttons;
      buttons.push_back(mButton);
      mButtonGrid = MenuComponent::MakeButtonGrid(mWindow, buttons);

      mGrid.setEntry(mButtonGrid, Vector2i(0, 6), true, false);

      setSize(Renderer::Instance().DisplayWidthAsFloat() * 0.95f, Renderer::Instance().DisplayHeightAsFloat() * 0.747f);
      setPosition((Renderer::Instance().DisplayWidthAsFloat() - mSize.x()) / 2, (Renderer::Instance().DisplayHeightAsFloat() - mSize.y()) / 2);

      // Run!
      mScraper->RunOn(ScrapingMethod::All, game, this, RecalboxSystem::GetMinimumFreeSpaceOnSharePartition());
    }

    void onSizeChanged() override
    {
      mBox.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

      mGrid.setRowHeightPerc(0, 0.04f, false);
      mGrid.setRowHeightPerc(1, mGameName->getFont()->Height() * 1.2f / mSize.y(), false); // game name
      mGrid.setRowHeightPerc(2, 0.04f, false);
      mGrid.setRowHeightPerc(3, mSystemName->getFont()->Height() * 1.2f / mSize.y(), false); // system name
      mGrid.setRowHeightPerc(4, 0.04f, false);
      mGrid.setRowHeightPerc(6, mButtonGrid->getSize().y() * 1.8f / mSize.y(), false); // buttons
      mGrid.setSize(mSize);
    }

    bool ProcessInput(const InputCompactEvent& event) override
    {
      if (event.CancelReleased())
      {
        mScraper->StopNotifications();
        Close();
        return true;
      }

      return Component::ProcessInput(event);
    }

    bool CollectHelpItems(Help& help) override { return mGrid.CollectHelpItems(help); }

    /*
     * INotifyScrapeResult implementation
     */

    /*!
     * @brief Notify a game has been scraped
     * @param index Game index
     * @param total Total game to scrape
     * @param result Result object
     */
    void GameResult(int index, int total, FileData* result, MetadataType changedMetadata) override
    {
      (void)index;
      (void)total;

      // #TODO: move into scraper!
      switch(RecalboxConf::Instance().GetScraperNameOptions())
      {
        case ScraperNameOptions::GetFromScraper: break;
        case ScraperNameOptions::GetFromFilename:
        {
          result->Metadata().SetName(result->RomPath().FilenameWithoutExtension());
          break;
        }
        case ScraperNameOptions::GetFromFilenameUndecorated:
        {
          ScraperFactory::ExtractFileNameUndecorated(*result);
          break;
        }
      }

      // Check free space
      RecalboxStorageWatcher::CheckStorageFreeSpace(mWindow, mSystemManager.GetMountMonitor(), result->RomPath());

      // Update game data
      mSearch->SetRunning(false);
      mSearch->UpdateInfoPane(&mGame);
      if (mNotifier != nullptr) mNotifier->ScrapingComplete(mGame, changedMetadata, mContextData);
      mSystemManager.UpdateSystemsOnGameChange(result, changedMetadata, false);

      // Scripts
      NotificationManager::Instance().Notify(*result, Notification::ScrapGame);
    }

    /*!
     * @brief Scraper site quota reached. Scraping is being aborted immediately.
     */
    void ScrapingComplete(ScrapeResult reason, MetadataType changedMetadata) override
    {
      (void)reason;
      (void)changedMetadata;
      // Update UI
      mButton->setText(_("CLOSE"), _("CLOSE"));
    }

  private:
    //! SystemManager reference
    SystemManager& mSystemManager;

    //! Scraper interface
    IScraperEngine* mScraper;

    //! Target game
    FileData& mGame;

    //! Notification interface
    IScrapingComplete* mNotifier;
    //! Context data
    ContextType mContextData;

    /*
     * Components
     */

    ComponentGrid mGrid;
    NinePatchComponent mBox;

    std::shared_ptr<TextComponent> mGameName;
    std::shared_ptr<TextComponent> mSystemName;
    std::shared_ptr<ScraperSearchComponent> mSearch;
    std::shared_ptr<ButtonComponent> mButton;
    std::shared_ptr<ComponentGrid> mButtonGrid;
};
