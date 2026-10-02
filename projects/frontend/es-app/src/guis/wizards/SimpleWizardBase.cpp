//
// Created by bkg2k on 11/11/2020.
//

#include "SimpleWizardBase.h"
#include <themes/MenuThemeData.h>
#include <utils/locale/LocaleHelper.h>
#include <components/TextComponent.h>
#include <components/ImageComponent.h>
#include <help/Help.h>
#include <themes/ThemeManager.h>

SimpleWizardBase::SimpleWizardBase(WindowManager& window, const String& title, int pageCount)
  : Gui(window)
  , mGrid(window, { 5, 3 })
  , mPageCount(pageCount)
  , mCurrentPage(-1)
{
  // Add grid as the main component
  addChild(&mGrid);

  // Get theme
  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

  // 7 x 15
  // +-+-------------------------++--------------------------------------+-+
  // | |                                 TITLE                           | |  0 = Text size%
  // +-+-------------------------++--------------------------------------+-+
  // | |                         ||                                      | |
  // | |                         ||                                      | |
  // | |                         ||                                      | |
  // | |        IMAGE            ||                TEXT                  | |
  // | |                         ||                                      | |
  // | |                         ||                                      | |
  // | |                         ||                                      | |
  // | |                         ||                                      | |
  // | |                         ||                                      | |  1 = remaining size
  // | |                         ||                                      | |
  // | |                         ||                                      | |
  // | |                         ||                                      | |
  // | |                         ||                                      | |
  // | |                         ||                                      | |
  // | |                         ||                                      | |
  // +-+-------------------------++--------------------------------------+-+
  // +-+-------------------------++--------------------------------------+-+  2 = 2% margin
  //  0           1               2                  3                    4
  //  2%          27%             2%                67%                   2%

  // Title
  mTitle = std::make_shared<TextComponent>(window, _S(title), menuTheme.Title().font, menuTheme.Title().color, ::Alignment::Center);
  mGrid.setEntry(mTitle, Vector2i(1, 0), false, true, Vector2i(3,1) );

  // Image
  mImage = std::make_shared<ImageComponent>(window);
  mGrid.setEntry(mImage, Vector2i(1, 1), false, true, Vector2i(1,1) );

  // Text
  mText = std::make_shared<TextComponent>(window, "", menuTheme.Text().font, menuTheme.Text().color, ::Alignment::Center);
  mText->setMultiline(true);
  mGrid.setEntry(mText, Vector2i(3, 1), false, true, Vector2i(1,1) );

  // Set Window position/size
  setSize(Renderer::Instance().DisplayWidthAsFloat() * 0.9f, Renderer::Instance().DisplayHeightAsFloat() * 0.8f);
  setPosition((Renderer::Instance().DisplayWidthAsFloat() - mSize.x()) / 2, (Renderer::Instance().DisplayHeightAsFloat() - mSize.y()) / 2);

  mGrid.setSize(mSize);

  // Sizes
  mGrid.setColWidthPerc(0, 0.02f, false);
  mGrid.setColWidthPerc(1, 0.27f, false);
  mGrid.setColWidthPerc(2, 0.02f, false);
  mGrid.setColWidthPerc(3, 0.67f, false);
  mGrid.setColWidthPerc(4, 0.02f, true);
  mGrid.setRowHeightPerc(0, mTitle->getFont()->Height() * 2.6f / mSize.y(), false);
  mGrid.setRowHeightPerc(1, 0.0f, false);
  mGrid.setRowHeightPerc(2, 0.04f, true);
  mGrid.SetColumnHighlight(true, 1, 1);

  // Image size
  mImage->setResize(0/*mGrid.getColWidth(1)*/, mGrid.getRowHeight(1));
  mImage->setKeepRatio(true);
}

bool SimpleWizardBase::ProcessInput(const InputCompactEvent& event)
{
  switch(OnKeyReceived(mCurrentPage, event))
  {
    case Move::Backward: { SetPage(mCurrentPage - 1); return true; }
    case Move::Foreward: { SetPage(mCurrentPage + 1); return true; }
    case Move::Close: { Close(); return true; }
    case Move::None:
    default: break;
  }
  return Component::ProcessInput(event);
}

void SimpleWizardBase::Update(int deltaTime)
{
  Component::Update(deltaTime);

  if (mCurrentPage < 0)
    SetPage(0);
}

void SimpleWizardBase::SetPage(int page)
{
  mCurrentPage = Math::clampi(page, 0, mPageCount - 1);

  mImage->setImage(OnImageRequired(mCurrentPage), false);
  mText->setText(OnTextRequired(mCurrentPage));

  mGrid.onSizeChanged();
  UpdateHelpBar();
}

bool SimpleWizardBase::CollectHelpItems(Help& help)
{
  help.Clear();
  if (mCurrentPage > 0) help.Set(Help::Cancel(), _("BACK"));

  OnHelpRequired(mCurrentPage, help);

  return true;
}

void SimpleWizardBase::UpdatePage()
{
  SetPage(mCurrentPage);
}

