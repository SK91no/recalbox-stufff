//
// Created by bkg2k on 26/03/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "GuiInfoPopupSeamlessScraper.h"
#include "scraping/ScrapeResult.h"
#include "hardware/Board.h"
#include <themes/MenuThemeData.h>
#include <components/TextComponent.h>
#include <components/ProgressBarComponent.h>
#include <systems/SystemData.h>
#include <themes/ThemeManager.h>
#include <scraping/ScraperSeamless.h>
#include <rendering/fonts/FontManager.h>

String GuiInfoPopupSeamlessScraper::mTextTemplate("Scraping game... %COUNT% yet to go.\n%SYSTEM%\n%NAME%");
String GuiInfoPopupSeamlessScraper::mTextTemplateCRT("Scraping game... %COUNT% yet to go.");

GuiInfoPopupSeamlessScraper::GuiInfoPopupSeamlessScraper(WindowManager& window)
  : StaticLifeCycleControler("SeamlessUI")
  , GuiInfoPopupBase(window, true, -1, PopupType::Scraper, 3, 1, Renderer::Instance().Is480pOrLower() ? 1.f : 1.6f)
  , mCount(0)
{
}

float GuiInfoPopupSeamlessScraper::AddComponents(WindowManager& window, ComponentGrid& grid, float maxWidth, float maxHeight,
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

  String msg(_("Starting background scraper..."));
  bool crt = Board::Instance().CrtBoard().IsCrtAdapterAttached();
  if (!crt) msg.Append(String::CRLF).Append(String::CRLF);
  mText        = std::make_shared<TextComponent>(window, msg, &FontManager::Instance().FromDefault(fontSizeText, fontPixelSize), menuTheme.Text().color, ::Alignment::TopCenter);
  mIcon        = std::make_shared<TextComponent>(window, iconText, &FontManager::Instance().FromDefault(fontSizeIcon, fontPixelSize), menuTheme.Text().color, ::Alignment::CenterLeft);
  mImage       = std::make_shared<ImageComponent>(window);
  mNoImage     = std::make_shared<ImageComponent>(window);

  grid.setEntry(mIcon       , Vector2i(0, 0), false, true);
  grid.setEntry(mText       , Vector2i(1, 0), false, true);
  if (!crt)
  {
    grid.setEntry(mImage, Vector2i(2, 0), false, true);
    grid.setEntry(mNoImage, Vector2i(2, 0), false, true);
  }

  mText->setSize(maxWidth - mIcon->getSize().y(), 0);
  float msgHeight = Math::min(maxHeight, Math::max(mText->getSize().y(), mIcon->getSize().y()));
  grid.setColWidthPerc(0, (float)(mIcon->getFont()->Height() + paddingX) / maxWidth);
  grid.setColWidthPerc(2, crt ? (msgHeight * 1.5f) / maxWidth : 0.02f);
  mImage->setResize(0.f, msgHeight * 0.9f);
  mNoImage->setResize(0.f, msgHeight * 0.9f);
  mNoImage->setImage(Path(":/no_image.png"), false);

  return msgHeight * 1.16f;
}

void GuiInfoPopupSeamlessScraper::SetScrapeResult(const FileData& game)
{
  if (Initialized())
  {
    String msg(Board::Instance().CrtBoard().IsCrtAdapterAttached() ? _S(mTextTemplateCRT) : _S(mTextTemplate));
    msg.Replace("%COUNT%", String(mCount))
       .Replace("%NAME%", game.Metadata().Name())
       .Replace("%SYSTEM%", game.System().FullName());
    mText->setVerticalAlignment(::VerticalAlignment::Top);
    mText->setSize(Grid().getColWidth(1), Grid().getRowHeight(0));
    mText->setText(msg);
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

void GuiInfoPopupSeamlessScraper::SetScrapeEnd()
{
  if (Initialized())
  {
    String msg(_("Scraping completed!"));
    msg.Append(String::CRLF).Append(String::CRLF);
    mText->setVerticalAlignment(::VerticalAlignment::Top);
    mText->setSize(Grid().getColWidth(1), Grid().getRowHeight(0));
    mText->setText(msg);
    Grid().onSizeChanged();
  }
}

void GuiInfoPopupSeamlessScraper::ScrapingStageCompleted(FileData* game, IScraperEngineStage::Stage stage, MetadataType changes)
{
  (void)stage;
  (void)changes;
  SetScrapeResult(*game);
}

void GuiInfoPopupSeamlessScraper::AddGames(FileData::List& games)
{
  Restart();
  // Pops up or restart the UI
  mWindow.InfoPopupAdd(this, true);
  // Call the scraper
  ScraperSeamless::Instance().Push(games, this, this);
}

void GuiInfoPopupSeamlessScraper::ScrapeCountYetToGo(int count)
{
  mCount = count;
  if (count == 0)
  {
    SetScrapeEnd();
    Stop(5000);
  }
}
