//
// Created by bkg2k on 16/04/23.
//

#include "GuiDownloader.h"
#include "components/MenuComponent.h"
#include <utils/locale/LocaleHelper.h>
#include <MainRunner.h>

GuiDownloader::GuiDownloader(WindowManager& window, SystemData& system, SystemManager& systemManager)
  : Gui(window)
  , mGrid(window, Vector2i(3, 4))
  , mSystemManager(systemManager)
{
  addChild(&mGrid);

  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

  const float height = Renderer::Instance().DisplayHeightAsFloat() * (Renderer::Instance().Is480pOrLower() ? 0.7f : 0.3f);
  const float width = Renderer::Instance().DisplayWidthAsFloat() * (Renderer::Instance().Is480pOrLower() ? 0.8f : 0.6f);

  // Title
  mTitle = std::make_shared<TextComponent>(mWindow, "Downloader", menuTheme.Title().font, menuTheme.Title().color, ::Alignment::Center);
  mGrid.setEntry(mTitle, Vector2i(1, 0), false, false, Vector2i(1,1) );

  // Text
  mText = std::make_shared<TextComponent>(mWindow, "", menuTheme.SmallText().font, menuTheme.SmallText().color, ::Alignment::CenterLeft);
  mGrid.setEntry(mText, Vector2i(1, 1), false, false, Vector2i(1,1) );

  // Progress bar
  mBar = std::make_shared<ProgressBarComponent>(mWindow, 1);
  mGrid.setEntry(mBar, Vector2i(1, 2), false, true, Vector2i(1,1) );

  // ETA
  mEta = std::make_shared<TextComponent>(mWindow, _("Start downloading..."), menuTheme.SmallText().font, menuTheme.SmallText().color, ::Alignment::CenterLeft);
  mGrid.setEntry(mEta, Vector2i(1, 3), false, false, Vector2i(1,1) );

  mGrid.setColWidthPerc(0, 0.02f);
  mGrid.setColWidthPerc(1, 0.96f);
  mGrid.setColWidthPerc(2, 0.02f);

  // Set grid size
  float titlePercent = ((float)mTitle->getFont()->Height() * 1.6f) / height;
  float pbPercent = 0.08f;
  float etaPercent = ((float)mEta->getFont()->Height() * 2.0f) / height;
  float textPercent = 1.0f - (titlePercent + pbPercent + etaPercent);
  constexpr float mainColWidth = 0.96f;
  constexpr float marginWidth = (1.0f - mainColWidth) / 2;

  mGrid.setColWidthPerc(0, marginWidth, false);
  mGrid.setColWidthPerc(1, mainColWidth, false);
  mGrid.setColWidthPerc(2, marginWidth, false);
  mGrid.setRowHeightPerc(0, titlePercent, false);
  mGrid.setRowHeightPerc(1, textPercent, false);
  mGrid.setRowHeightPerc(2, pbPercent, false);
  mGrid.setRowHeightPerc(3, etaPercent, false);

  mText->setSize(width * mainColWidth, 0);

  mGrid.setSize(width, height);

  // Window
  setSize(width, height);
  setPosition((Renderer::Instance().DisplayWidthAsFloat() - width) / 2,
              (Renderer::Instance().DisplayHeightAsFloat() - height) / 2);

  // Avoid sleeping!
  mIsProcessing = true;

  // Start?
  if (DownloaderManager::HasDownloader(system))
  {
    mDownloader = mDownloadManager.CreateOrGetDownloader(system, *this);
    if (mDownloader != nullptr) mDownloader->StartDownload(String("dl-").Append(system.Name()).c_str());
    else
    {
      LOG(LogError) << "[GuiDownloader] Cannot obtain downloader for System " << system.FullName();
      mWindow.CloseAll();
    }
  }
  else
  {
    LOG(LogError) << "[GuiDownloader] System " << system.FullName() << " has no downloader!";
    mWindow.CloseAll();
  }
}

bool GuiDownloader::ProcessInput(const InputCompactEvent& event)
{
  if (event.CancelReleased())
  {
    mDownloader->MustExitAsap();
    return true;
  }
  return Component::ProcessInput(event);
}

bool GuiDownloader::CollectHelpItems(Help& help)
{
  mGrid.CollectHelpItems(help);
  help.Set(Help::Cancel(), _("CANCEL"));
  return true;
}

void GuiDownloader::UpdateProgressbar(long long int value, long long int total)
{
  mBar->setMaxValue(total);
  mBar->setCurrentValue(value);
  mGrid.onSizeChanged();
}

void GuiDownloader::UpdateMainText(const String& text)
{
  mText->setText(text);
  mGrid.onSizeChanged();
}

void GuiDownloader::UpdateETAText(const String& text)
{
  mEta->setText(text);
  mGrid.onSizeChanged();
}

void GuiDownloader::DownloadComplete(SystemData& system, bool aborted)
{
  mWindow.CloseAll();

  if (!aborted)
    mSystemManager.UpdateSystemsVisibility(&system, SystemManager::Visibility::ShowAndSelect);
}

void GuiDownloader::UpdateTitleText(const String& text)
{
  mTitle->setText(text);
  mGrid.onSizeChanged();
}


