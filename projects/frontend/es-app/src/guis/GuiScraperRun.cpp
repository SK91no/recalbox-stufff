#include <systems/SystemManager.h>
#include <scraping/ScraperFactory.h>
#include <MainRunner.h>
#include <recalbox/RecalboxSystem.h>
#include "guis/GuiScraperRun.h"

#include "components/ScraperSearchComponent.h"
#include "components/MenuComponent.h" // for makeButtonGrid
#include "guis/GuiMsgBox.h"
#include "MenuMessages.h"
#include "recalbox/RecalboxStorageWatcher.h"

GuiScraperRun* GuiScraperRun::sInstance = nullptr;

void GuiScraperRun::CreateOrShow(WindowManager& window, SystemManager& systemManager, const SystemManager::List& systems, ScrapingMethod method, IScraperEngineFreezer* freezer, bool lowResolution)
{
  if (IsRunning())
    Show(window);
  else
    window.pushGui(sInstance = new GuiScraperRun(window, systemManager, systems, method, freezer, lowResolution));
}

void GuiScraperRun::CreateOrShow(WindowManager& window, SystemManager& systemManager, FileData::List& games, ScrapingMethod method, IScraperEngineFreezer* freezer, bool lowResolution)
{
  if (IsRunning())
    Show(window);
  else
    window.pushGui(sInstance = new GuiScraperRun(window, systemManager, games, method, freezer, lowResolution));
}

void GuiScraperRun::Show(WindowManager& window)
{
  if (sInstance != nullptr)
  {
    window.RemoveGui(sInstance);
    window.pushGui(sInstance);
    window.InfoPopupRemove(&sInstance->mPopup);
  }
}

void GuiScraperRun::Hide(WindowManager& window)
{
  if (sInstance != nullptr)
  {
    window.RemoveGui(sInstance);
    if(sInstance->mLowResolution)
      window.InfoPopupAdd(new GuiInfoPopup(window, _("Scrap running in background.\nReturn to the scrap menu to get the progression."), 8, PopupType::Scraper));
    else
      window.InfoPopupAdd(&sInstance->mPopup, true);
    window.CloseAll();
  }
}

void GuiScraperRun::Abort()
{
  if (sInstance != nullptr)
  {
    sInstance->TerminateScraping();
    TerminateGui();
  }
}

void GuiScraperRun::TerminateGui()
{
  if (sInstance != nullptr)
    sInstance->Close();
  sInstance = nullptr;
}

GuiScraperRun::GuiScraperRun(WindowManager& window, SystemManager& systemManager, bool lowResolution)
  :	Gui(window)
  , mSystemManager(systemManager)
  , mScraper(nullptr)
  , mResult(ScrapeResult::NotScraped)
  , mTimeReference(0)
  , mGrid(window, Vector2i(1, 8))
  , mLowResolution(lowResolution)
  , mPopup(window)
{
  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

  addChild(&mGrid);

  mIsProcessing = true;
  // set up grid
  mTitle = std::make_shared<TextComponent>(mWindow, _("SCRAPING IN PROGRESS"), lowResolution ? menuTheme.Text().font : menuTheme.Title().font, menuTheme.Title().color, ::Alignment::Center);
  mGrid.setEntry(mTitle, Vector2i(0, 0), false, true);

  mSystem = std::make_shared<TextComponent>(mWindow, _("SYSTEM"), menuTheme.Text().font, menuTheme.Text().color, ::Alignment::Center);
  mGrid.setEntry(mSystem, Vector2i(0, 1), false, true);

  mSubtitle = std::make_shared<TextComponent>(mWindow, "", menuTheme.Footer().font, menuTheme.Footer().color, ::Alignment::Center);
  mGrid.setEntry(mSubtitle, Vector2i(0, 2), false, true);

  mSearchComp = std::make_shared<ScraperSearchComponent>(mWindow,lowResolution);
  mGrid.setEntry(mSearchComp, Vector2i(0, 3), false, true);

  // Progress bar
  mProgressGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(3, 3));
  mBar = std::make_shared<ProgressBarComponent>(mWindow, 1);
  mProgressGrid->setEntry(mBar, Vector2i(1, 1), false, true);
  mGrid.setEntry(mProgressGrid, Vector2i(0,4), false, true);
  mProgressGrid->setColWidthPerc(0, 0.02f);
  mProgressGrid->setColWidthPerc(1, 0.96f);
  mProgressGrid->setColWidthPerc(2, 0.02f);
  mProgressGrid->setRowHeightPerc(0, 0.2f);
  mProgressGrid->setRowHeightPerc(1, 0.6f);
  mProgressGrid->setRowHeightPerc(2, 0.2f);

  mTiming = std::make_shared<TextComponent>(mWindow, "", menuTheme.Footer().font, menuTheme.Text().color, ::Alignment::Center);
  mGrid.setEntry(mTiming, Vector2i(0, 5), false, true);


  mDatabaseMessage = std::make_shared<TextComponent>(mWindow, "", menuTheme.Footer().font, menuTheme.Footer().color, ::Alignment::Center);
  if(!lowResolution)
    mGrid.setEntry(mDatabaseMessage, Vector2i(0, 6), false, true);

  std::vector<std::shared_ptr<ButtonComponent>> buttons
    {
      mButton = std::make_shared<ButtonComponent>(mWindow, _("STOP"), _("stop (progress saved)"), std::bind( &GuiScraperRun::TerminateScraping, this)),
      mRunInBgButton = std::make_shared<ButtonComponent>(mWindow, _("RUN IN BACKGROUND"), _("RUN IN BACKGROUND"), [this] { GuiScraperRun::Hide(mWindow); })
    };
  mButtonGrid = MenuComponent::MakeButtonGrid(mWindow, buttons);
  mGrid.setEntry(mButtonGrid, Vector2i(0, 7), true, false);

  setSize(Renderer::Instance().DisplayWidthAsFloat() * 0.95f, Renderer::Instance().DisplayHeightAsFloat() * (lowResolution ? 0.78f : 0.849f));
  setPosition((Renderer::Instance().DisplayWidthAsFloat() - mSize.x()) / 2, (Renderer::Instance().DisplayHeightAsFloat() - mSize.y()) / 2);

	// Final report component
  mFinalReport = std::make_shared<TextComponent>(mWindow, _("subtitle text"), menuTheme.Footer().font, menuTheme.Text().color, ::Alignment::Center);
  mFinalReport->setMultiline(true);

  // Scripts
  NotificationManager::Instance().Notify(Notification::ScrapStart);
}

GuiScraperRun::GuiScraperRun(WindowManager& window, SystemManager& systemManager, const SystemManager::List& systems, ScrapingMethod method, IScraperEngineFreezer* freezer, bool lowResolution)
  : GuiScraperRun(window, systemManager, lowResolution)
{
  // Create scraper and run!
	mScraper = ScraperFactory::Instance().GetScraper(RecalboxConf::Instance().GetScraperSource(), freezer);
	mScraper->RunOn(method, systems, this, RecalboxSystem::GetMinimumFreeSpaceOnSharePartition());
}

GuiScraperRun::GuiScraperRun(WindowManager& window, SystemManager& systemManager, FileData::List& games, ScrapingMethod method, IScraperEngineFreezer* freezer, bool lowResolution)
  : GuiScraperRun(window, systemManager, lowResolution)
{
  // Create scraper and run!
  mScraper = ScraperFactory::Instance().GetScraper(RecalboxConf::Instance().GetScraperSource(), freezer);
  mScraper->RunOn(method, games, this, RecalboxSystem::GetMinimumFreeSpaceOnSharePartition());
}

void GuiScraperRun::onSizeChanged()
{
	mGrid.setRowHeightPerc(0, (float)mTitle->getFont()->Height() * 1.6f / mSize.y(), false);
	mGrid.setRowHeightPerc(1, ((float)mSystem->getFont()->Height() + 2.0f) / mSize.y(), false);
	mGrid.setRowHeightPerc(2, (float)mSubtitle->getFont()->Height() * 1.2f / mSize.y(), false);
  mGrid.setRowHeightPerc(4, 0.04f, false);
  mGrid.setRowHeightPerc(5, (float)mTiming->getFont()->Height() / mSize.y(), false);
  mGrid.setRowHeightPerc(6, (float)mDatabaseMessage->getFont()->Height() / mSize.y(), false);
  mGrid.setRowHeightPerc(7, mButtonGrid->getSize().y() * 1.2f  / mSize.y(), false);
	mGrid.setSize(mSize);
}

void GuiScraperRun::TerminateScraping()
{
  mScraper->Abort(true);
  TerminateGui();
  mSystemManager.UpdateAllGameLists();
  mWindow.CloseAll();
  /*switch(mResult)
  {
    case ScrapeResult::Ok: MainRunner::RequestQuit(MainRunner::ExitState::Relaunch, true); break;
    case ScrapeResult::NotScraped:
    case ScrapeResult::NotFound:
    case ScrapeResult::QuotaReached:
    case ScrapeResult::DiskFull:
    case ScrapeResult::FatalError:
    default: break;
  }*/
  ViewController::Instance().InvalidateAllGamelistsExcept(nullptr);
	mIsProcessing = false;
}

void GuiScraperRun::GameResult(int index, int total, FileData* result, MetadataType changedMetadata)
{
  (void)changedMetadata; // #TODO: see later if it's worth the try to refresh systems game per game
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

  // No more than 1 refresh per 20ms, otherwise the windows is overwhelmed by refresh message
  // and the whole scrapper seems to freeze
  int now = (int)SDL_GetTicks();
  if (now - mTimeReference < 20) return;
  mTimeReference = now;

  // Update popup
  mPopup.SetScrapedGame(*result, index, total);

  // update title
  mSystem->setText(result->System().FullName().ToUpperCaseUTF8());

  // update subtitle
  mSubtitle->setText(result->RomPath().Filename().ToUpperCase());

  mBar->setMaxValue(total);
  mBar->setCurrentValue(index);

  // Update game data
  mSearchComp->UpdateInfoPane(result);

  // Update timings
  TimeSpan elapsed = DateTime() - mStart;

  String status = (_F(_("GAME {0} OF {1}")) / index / total).ToString().Append(" - ");
  if(!mLowResolution)
    status.Append(_("ELAPSED TIME: "))
          .Append(elapsed.ToStringFormat("%H:%mm:%ss"))
          .Append(" - ");
  status.Append(_("ESTIMATED TIME: "));

  if ((mScraper->ScrapesProcessed() > 10 || mScraper->ScrapesTotal() <= 10) && elapsed.TotalSeconds() > 10)
  {
    long long millisecondPerGame = elapsed.TotalMilliseconds() / mScraper->ScrapesProcessed();
    TimeSpan estimated(millisecondPerGame * mScraper->ScrapesStillPending());
    if (mScraper->ScrapesProcessed() < mScraper->ScrapesTotal())
      status.Append(estimated.ToStringFormat("%H:%mm:%ss"));
    else
      status.Append(_("COMPLETE!"));
  }
  else status.Append("---");

  if (mScraper->ScrapesProcessed() >= mScraper->ScrapesTotal() && mLowResolution)
    status = "";
  mTiming->setText(status);

  // Check free space
  RecalboxStorageWatcher::CheckStorageFreeSpace(mWindow, mSystemManager.GetMountMonitor(), result->RomPath());

  // Update database message
  mDatabaseMessage->setText(mScraper->ScraperDatabaseMessage());

  // Scripts
  NotificationManager::Instance().Notify(*result, Notification::ScrapGame);
}

void GuiScraperRun::ScrapingComplete(ScrapeResult reason, MetadataType changedMetadata)
{
  mGrid.removeEntry(mSearchComp);
  String finalReport;
  bool fatalError = false;
  switch(mResult = reason)
  {
    case ScrapeResult::Ok:
    case ScrapeResult::NotScraped:
    case ScrapeResult::NotFound:
    {
      finalReport = _(MENUMESSAGE_SCRAPER_FINAL_POPUP);
      finalReport.Replace("{PROCESSED}", String(mScraper->ScrapesTotal()))
                 .Replace("{SUCCESS}", String(mScraper->ScrapesSuccessful()))
                 .Replace("{NOTFOUND}", String(mScraper->ScrapesNotFound()))
                 .Replace("{ERRORS}", String(mScraper->ScrapesErrors()))
                 .Replace("{TEXTINFO}", String(mScraper->StatsTextInfo()))
                 .Replace("{IMAGES}", String(mScraper->StatsImages()))
                 .Replace("{VIDEOS}", String(mScraper->StatsVideos()));
      long long size = mScraper->StatsMediaSize();
      finalReport = finalReport.Replace("{MEDIASIZE}", Sizes(size).ToHumanSize()).ToUpperCaseUTF8();
      break;
    }
    case ScrapeResult::FatalError:
    {
      finalReport = _(MENUMESSAGE_SCRAPER_FINAL_FATAL);
      fatalError = true;
      break;
    }
    case ScrapeResult::QuotaReached:
    {
      finalReport = _(MENUMESSAGE_SCRAPER_FINAL_QUOTA);
      fatalError = true;
      break;
    }
    case ScrapeResult::DiskFull:
    {
      finalReport = _(MENUMESSAGE_SCRAPER_FINAL_DISKFULL);
      fatalError = true;
      break;
    }
  }
  mFinalReport->setText(finalReport);
  mGrid.setEntry(mFinalReport, Vector2i(0, 3), false, true);

  // Update popup
  mPopup.ScrapingComplete(reason);

  // Save all gamelists
  {
    GuiSyncWaiter waiter(mWindow, _("Saving gamelists..."));
    waiter.Show();
    mSystemManager.UpdateAllGameLists();
    waiter.Hide();
  }

  // Refresh systems
  switch(reason)
  {
    case ScrapeResult::Ok:
    case ScrapeResult::NotScraped:
    case ScrapeResult::NotFound:
    case ScrapeResult::QuotaReached:
    case ScrapeResult::DiskFull:
    {
      GuiSyncWaiter waiter(mWindow, _("Refreshing systems..."));
      waiter.Show();
      mSystemManager.UpdateSystemsOnGameChange(nullptr, changedMetadata, false);
      waiter.Hide();
      break;
    }
    case ScrapeResult::FatalError:
    default: break;
  }


  // Update button?
  mButton->setText(_("CLOSE"), _("CLOSE"));
  mSearchComp->SetRunning(false);
  mGrid.removeEntry(mButtonGrid);
  mButtonGrid->removeEntry(mButton);
  mButtonGrid = MenuComponent::MakeButtonGrid(mWindow,  std::vector<std::shared_ptr<ButtonComponent>>
  {
    mButton
  });
  mGrid.setEntry(mButtonGrid, Vector2i(0, 7), true, false);

  // Scripts
  NotificationManager::Instance().Notify(Notification::ScrapStop, String(mScraper->ScrapesSuccessful()));

  // Hiden?
  if ((mWindow.InfoPopupIsShown(&mPopup) || mLowResolution) && !fatalError)
  {
    String text = mLowResolution ? _("Your scraping session completed!") : _("Your scraping session completed. Press OK to show the results.");
    GuiMsgBox* msgBox = new GuiMsgBox(mWindow, text, _("OK"), [this] { if (mLowResolution) TerminateGui(); else Show(mWindow); mButtonGrid->resetCursor(); });
    mWindow.pushGui(msgBox);
  }
}

