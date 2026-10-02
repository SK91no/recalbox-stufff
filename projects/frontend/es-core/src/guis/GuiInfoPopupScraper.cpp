//
// Created by bkg2k on 26/03/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "GuiInfoPopupScraper.h"
#include "scraping/ScrapeResult.h"
#include <themes/MenuThemeData.h>
#include <components/TextComponent.h>
#include <components/ProgressBarComponent.h>
#include <systems/SystemData.h>
#include <themes/ThemeManager.h>
#include <rendering/fonts/FontManager.h>

String GuiInfoPopupScraper::mTextTemplate(_("Scraping game %VALUE%/%TOTAL%\n%SYSTEM%\n%NAME%"));

GuiInfoPopupScraper::GuiInfoPopupScraper(WindowManager& window)
  : GuiInfoPopupBase(window, true, -1, PopupType::Scraper, 3, 2, 1.6f)
{
}

float GuiInfoPopupScraper::AddComponents(WindowManager& window, ComponentGrid& grid, float maxWidth, float maxHeight,
                                         int paddingX, int paddingY)
{
  (void)paddingY;
  String iconText = "\uF1e4";

  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
  float fontSizeIcon = 0.04f;
  float fontSizeText = 0.02f;
  bool fontPixelSize = false;

  if(Renderer::Instance().Is480pOrLower())
  {
    fontSizeIcon = (float)menuTheme.Text().font->RequestedHeight();
    fontSizeText = (float)menuTheme.Text().font->RequestedHeight();
    fontPixelSize = true;
  }

  mText        = std::make_shared<TextComponent>(window, mTextTemplate + '\n', &FontManager::Instance().FromDefault(fontSizeText, fontPixelSize), menuTheme.Text().color, ::Alignment::TopCenter);
  mIcon        = std::make_shared<TextComponent>(window, iconText, &FontManager::Instance().FromDefault(fontSizeIcon, fontPixelSize), menuTheme.Text().color, ::Alignment::CenterLeft);
  mProgressBar = std::make_shared<ProgressBarComponent>(window, 1);
  mImage       = std::make_shared<ImageComponent>(window);
  mNoImage     = std::make_shared<ImageComponent>(window);

  grid.setEntry(mIcon       , Vector2i(0, 0), false, false);
  grid.setEntry(mText       , Vector2i(1, 0), false, false);
  grid.setEntry(mImage      , Vector2i(2, 0), false, false);
  grid.setEntry(mNoImage    , Vector2i(2, 0), false, false);
  grid.setEntry(mProgressBar, Vector2i(0, 1), false, false, Vector2i(3, 1));

  mText->setSize(maxWidth - mIcon->getSize().y(), 0);
  float msgHeight = Math::min(maxHeight, Math::max(mText->getSize().y(), mIcon->getSize().y()));
  grid.setColWidthPerc(0, (float)(mIcon->getFont()->Height() + paddingX) / maxWidth);
  grid.setColWidthPerc(2, (msgHeight * 1.5f) / maxWidth);
  grid.setRowHeightPerc(1, 0.16f);
  mProgressBar->setSize(maxWidth, 0);
  mImage->setResize(0.f, msgHeight * 0.9f);
  mNoImage->setResize(0.f, msgHeight * 0.9f);
  mNoImage->setImage(Path(":/no_image.png"), false);

  mText->setText(_("Scraper starting..."));

  return msgHeight * 1.16f;
}

void GuiInfoPopupScraper::SetScrapedGame(const FileData& game, int value, int total)
{
  if (Initialized())
  {
    mProgressBar->setMaxValue(total);
    mProgressBar->setCurrentValue(value);
    String msg(mTextTemplate);
    msg.Replace("%VALUE%", String(value))
       .Replace("%TOTAL%", String(total))
       .Replace("%NAME%", game.Metadata().Name())
       .Replace("%SYSTEM%", game.System().FullName());
    mText->setVerticalAlignment(::VerticalAlignment::Top);
    mText->setSize(Grid().getColWidth(1), Grid().getRowHeight(0));
    mText->setText(msg);
    mProgressBar->setSize(mProgressBar->getSize().x(), Grid().getSize().y() * 0.08f);
    mImage->setResize(0.f, Grid().getRowHeight(0)* 0.9f);
    mNoImage->setResize(0.f, Grid().getRowHeight(0)* 0.9f);
    const MetadataDescriptor& meta = game.Metadata();
    mImage->SetVisible(true);
    mNoImage->SetVisible(false);
    if      (!meta.Image().IsEmpty())     mImage->setImage(meta.Image(), false);
    else
    {
      mImage->SetVisible(false);
      mNoImage->SetVisible(true);
    }
    Grid().onSizeChanged();
  }
}

void GuiInfoPopupScraper::ScrapingComplete(ScrapeResult result)
{
  if (Initialized())
  {
    String msg(_("Scraping completed!").Append("\n\n"));
    switch(result)
    {
      case ScrapeResult::Ok: msg.Append(_("All games scraped successfully.")); break;
      case ScrapeResult::QuotaReached: msg.Append(_("You reached your daily quota. Retry tomorrow.")); break;
      case ScrapeResult::DiskFull: msg.Append(_("Your disk is full Pleas make room before trying again.")); break;
      case ScrapeResult::NotScraped:
      case ScrapeResult::NotFound:
      case ScrapeResult::FatalError: msg.Append(_("Errors occurred and prevented some games to get their metadata. Retry later.")); break;
      default: break;
    }
    mText->setVerticalAlignment(::VerticalAlignment::Top);
    mText->setSize(Grid().getColWidth(1), Grid().getRowHeight(0));
    mText->setText(msg);
    Grid().onSizeChanged();
  }
}
