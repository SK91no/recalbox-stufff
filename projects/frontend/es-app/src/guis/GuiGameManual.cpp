//
// Created by bkg2k on 06/02/2026.
//

#include "GuiGameManual.h"
#include <Renderer.h>

#include "games/FileData.h"
#include "rendering/fonts/FontManager.h"
#include "utils/Files.h"
#include "utils/SectionFile.h"

GuiGameManual::CurrentDisplay GuiGameManual::mRequestedDisplay = GuiGameManual::CurrentDisplay::ManualPdf;

void GuiGameManual::LoadTipsAndTricks()
{
  if (!mGame.Metadata().HasTipsAndTricks()) return;

  // Get prefered language
  String preferenLanguage(RecalboxConf::Instance().GetSystemLanguage());
  preferenLanguage.LowerCase();

  SectionFile tips(mGame.Metadata().TipsAndTricks());
  for (const String& section : tips.AllSections())
    mTipsAndTricks.push_back({ tips.AsString(section, "language"), tips.AsString(section, "title"), tips.AsString(section, "text").Replace("\\n", '\n').RemoveControl(), preferenLanguage });

  std::ranges::sort(mTipsAndTricks, [](const TipsAndTricks& a, const TipsAndTricks& b) { return a.Index() < b.Index(); });
}

void GuiGameManual::FillTipsAndTricks()
{
  const TipsAndTricks& tip = mTipsAndTricks[mCurrentTipsAndTricks];

  mMarkdownTitle.setText(String(' ').Append(tip.Title().ToUpperCaseUTF8()));
  mMarkdown.SetText(tip.Text());
  mMarkdownFlag.setImage(Path((_F(":/regions/{0}.svg") / tip.Language().ToLowerCase()).ToString()));

  UpdatePage(mCurrentTipsAndTricks + 1, (int)mTipsAndTricks.size());
}

void GuiGameManual::FillMap()
{
  const MetadataDescriptor::MapEntry& map = mMaps[mCurrentMap];

  String title = (_F(_("Map of {0}")) / mGame.Name())();
  if (!map.mTitle.empty()) title.Append(" - ").Append(map.mTitle);
  mTitle.setText(title);
  mImage.SetPicturePath(map.mPath);

  UpdatePage(mCurrentMap + 1, (int)mMaps.size());
}

void GuiGameManual::UpdatePage(int page, int total)
{
  mPageCounter.setText(String(page).Append('/').Append(total));
}

GuiGameManual::GuiGameManual(WindowManager& window, FileData& game)
  : Gui(window)
  , mGame(game)
  , mMenuTheme(ThemeManager::Instance().Menu())
  , mTitle(window, "", mMenuTheme.Title().font, mMenuTheme.Title().color, Alignment::CenterLeft)
  , mPageCounter(window, "", mMenuTheme.Title().font, mMenuTheme.Title().color, Alignment::CenterRight)
  , mBackground(window, Path(":/frame.png"))
  , mGrid(window, 4, 4)
  , mMarkdownGrid(window, 2, 2)
  , mMarkdownTitle(window, "", mMenuTheme.Text().font, mMenuTheme.Text().color)
  , mMarkdownFlag(window)
  , mMarkdown(mWindow)
  , mPdf(mWindow, FontManager::Instance().FromFont(*mMenuTheme.Title().font, (float)mMenuTheme.Title().font->Height() * 2.f, true), this)
  , mImage(window, FontManager::Instance().FromFont(*mMenuTheme.Title().font, (float)mMenuTheme.Title().font->Height() * 2.f, true))
  , mMaps(game.Metadata().Maps())
  , mCurrentTipsAndTricks(0)
  , mCurrentMap(0)
  , mCurrentDisplay(CurrentDisplay::ManualPdf)
{
  addChild(&mBackground);
  addChild(&mGrid);

  const float height = Renderer::Instance().DisplayHeightAsFloat() * 0.85f;
  const float width = Renderer::Instance().DisplayWidthAsFloat() * 0.9f;

  // Prepare grid
  mGrid.setColWidthPerc(0, 0.01f, false);
  mGrid.setColWidthPerc(1, 0, false);
  mGrid.setColWidthPerc(2, 0.2f, false);
  mGrid.setColWidthPerc(3, 0.01f, false);
  mGrid.setRowHeightPerc(0, 0.01f, false);
  mGrid.setRowHeightPerc(1, ((float)mMenuTheme.Title().font->Height() * 1.0f) / height, false);
  mGrid.setRowHeightPerc(2, 0, false);
  mGrid.setRowHeightPerc(3, 0.01f, false);
  mGrid.setSize(width, height);
  mGrid.setEntry(&mTitle, 1, 1, true, true, 1, 1);
  mGrid.setEntry(&mPageCounter, 2, 1, true, true, 1, 1);
  mGrid.setEntry(&mMarkdownGrid, 1, 2, true, true, 2, 1);
  mGrid.setEntry(&mPdf, 1, 2, true, true, 2, 1);
  mGrid.setEntry(&mImage, 1, 2, true, true, 2, 1);

  // Window
  setSize(width, height);
  mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
  CenterGui();

  // Markdown grid
  mMarkdownTitle.SetBold(true).setAlignment(Alignment::Center);
  mMarkdownGrid.setRowHeightPerc(0, (float)mMarkdownTitle.getFont()->Height() / mMarkdownGrid.getHeight() * 1.5f, false);
  mMarkdownGrid.setRowHeightPerc(1, 0, false);
  mMarkdownGrid.setColWidthPerc(0, 0, false);
  mMarkdownGrid.setColWidthPerc(1, ((float)mMarkdownTitle.getFont()->Height() * (71.f / 48.f)) / mMarkdownGrid.getWidth() * 1.2f, false);
  mMarkdownGrid.setEntry(&mMarkdownTitle, 0, 0, false, true, 1, 1);
  mMarkdownGrid.setEntry(&mMarkdownFlag, 1, 0, false, true, 1, 1);
  mMarkdownGrid.setEntry(&mMarkdown, 0, 1, false, true, 2, 1);
  mMarkdownGrid.SetRowHighlight(true, 0, 1);
  mMarkdownGrid.SetColumnHighlight(false, 0, 1);

  // Security
  if (!game.Metadata().HasManual() && mMaps.empty()) Close();

  // Load tips and tricks id any
  LoadTipsAndTricks();
  mMarkdown.SetUpDownScrolling(true);

  // Load best doc
  DisplayCurrentManual(true);
}

void GuiGameManual::DisplayCurrentManual(bool force)
{
  // Adjust
  CurrentDisplay previous = mCurrentDisplay;
  mCurrentDisplay = mRequestedDisplay;
  switch (mCurrentDisplay)
  {
    case CurrentDisplay::ManualPdf:
    {
      if (!mGame.Metadata().HasManual())
      {
        mCurrentDisplay = CurrentDisplay::MapImage;
        if (mMaps.empty()) mCurrentDisplay = CurrentDisplay::TipsAndTricks;
      }
      break;
    }
    case CurrentDisplay::MapImage:
    {
      if (mMaps.empty())
      {
        mCurrentDisplay = CurrentDisplay::TipsAndTricks;
        if (!mGame.Metadata().HasTipsAndTricks()) mCurrentDisplay = CurrentDisplay::ManualPdf;
      }
      break;
    }
    case CurrentDisplay::TipsAndTricks:
    {
      if (!mGame.Metadata().HasTipsAndTricks())
      {
        mCurrentDisplay = CurrentDisplay::ManualPdf;
        if (!mGame.Metadata().HasManual() && !mMaps.empty()) mCurrentDisplay = CurrentDisplay::MapImage;
      }
      break;
    }
  }

  // Fill-in components
  if (previous != mCurrentDisplay || force)
  {
    mPdf.setThemeDisabled(true);
    mMarkdownGrid.setThemeDisabled(true);
    mImage.setThemeDisabled(true);
    switch (mCurrentDisplay)
    {
      case CurrentDisplay::ManualPdf:
      {
        mTitle.setText((_F(_("Manual of {0}")) / mGame.Name())());
        mPdf.setThemeDisabled(false);
        mPdf.SetPdfPath(mGame.Metadata().Manual());
        mPdf.LoadPage(0);
        break;
      }
      case CurrentDisplay::MapImage:
      {
        mImage.setThemeDisabled(false);
        FillMap();
        break;
      }
      case CurrentDisplay::TipsAndTricks:
      {
        mTitle.setText((_F(_("Tips & Trick of {0}")) / mGame.Name())());
        mPageCounter.setText("1/1");
        mMarkdownGrid.setThemeDisabled(false);
        FillTipsAndTricks();
        break;
      }
    }
  }

  mWindow.UpdateHelpSystem();
}

bool GuiGameManual::ProcessInput(const InputCompactEvent& event)
{
  if (event.ValidReleased())
  {
    switch (mCurrentDisplay)
    {
      case CurrentDisplay::ManualPdf: mRequestedDisplay = CurrentDisplay::MapImage; break;
      case CurrentDisplay::MapImage: mRequestedDisplay = CurrentDisplay::TipsAndTricks; break;
      case CurrentDisplay::TipsAndTricks: mRequestedDisplay = CurrentDisplay::ManualPdf; break;
    }
    DisplayCurrentManual(false);
    return true;
  }
  if (event.CancelReleased()) { Close(); return true; }

  switch (mCurrentDisplay)
  {
    case CurrentDisplay::ManualPdf: return mPdf.ProcessInput(event);
    case CurrentDisplay::MapImage:
    {
      if ((int)mMaps.size() > 1)
      {
        if (event.L1Released()) { if (--mCurrentMap < 0) mCurrentMap = (int)mMaps.size() - 1; FillMap(); return true; }
        if (event.R1Released()) { if (++mCurrentMap >= (int)mMaps.size()) mCurrentMap = 0; FillMap(); return true; }
      }
      return mImage.ProcessInput(event);
    }
    case CurrentDisplay::TipsAndTricks:
    {
      if (event.L1Released()) { if (--mCurrentTipsAndTricks < 0) mCurrentTipsAndTricks = (int)mTipsAndTricks.size() - 1; FillTipsAndTricks(); return true; }
      if (event.R1Released()) { if (++mCurrentTipsAndTricks >= (int)mTipsAndTricks.size()) mCurrentTipsAndTricks = 0; FillTipsAndTricks(); return true; }
      return mMarkdown.ProcessInput(event);
    }
  }

  return false;
}

void GuiGameManual::Render(const Transform4x4f& parentTrans)
{
  //Transform4x4f trans = parentTrans * getTransform();
  //trans.round();
  //Renderer::SetMatrix(trans);

  Gui::Render(parentTrans);
}

bool GuiGameManual::CollectHelpItems(Help& help)
{
  help.Clear()
      .Set(HelpType::A, "CLOSE");
  switch (mCurrentDisplay)
  {
    case CurrentDisplay::ManualPdf: mPdf.CollectHelpItems(help); break;
    case CurrentDisplay::MapImage: mImage.CollectHelpItems(help); break;
    case CurrentDisplay::TipsAndTricks: mMarkdown.CollectHelpItems(help); break;
  }
  switch (mCurrentDisplay)
  {
    case CurrentDisplay::ManualPdf:
      if (!mMaps.empty()) help.Set(HelpType::B, _("SWITCH TO MAPS"));
      else if (mGame.Metadata().HasTipsAndTricks()) help.Set(HelpType::B, _("SWITCH TO TIPS"));
      break;
    case CurrentDisplay::MapImage:
      if (mMaps.size() > 1) help.Set(HelpType::LR, _("PAGE -/+"));
      if (mGame.Metadata().HasTipsAndTricks()) help.Set(HelpType::B, _("SWITCH TO TIPS"));
      else if (mGame.Metadata().HasManual()) help.Set(HelpType::B, _("SWITCH TO MANUAL"));
      break;
    case CurrentDisplay::TipsAndTricks:
      help.Set(HelpType::UpDown, _("SCROLL TEXT"))
          .Set(HelpType::LR, _("PAGE -/+"));
      if (mGame.Metadata().HasManual()) help.Set(HelpType::B, _("SWITCH TO MANUAL"));
      else if (!mMaps.empty()) help.Set(HelpType::B, _("SWITCH TO MAPS"));
      break;
  }

  return true;
}
