//
// Created by bkg2k on 20/03/25.
//

#include "GuiQuickJump.h"
#include "utils/locale/LocaleHelper.h"
#include "themes/ThemeManager.h"

GuiQuickJump::GuiQuickJump(WindowManager& window, QuickJumpItemHolder& trackedItems, IQuickJumpTracker& quickJumpInterface)
  : Gui(window)
  , mTheme(ThemeManager::Instance().Menu())
  , mConf(RecalboxConf::Instance())
  , mTrackedItems(trackedItems)
  , mInterface(quickJumpInterface)
  , mGrid(window, Vector2i(1, 2))
  , mTitleFont(&FontManager::Instance().FromDefault(FONT_SIZE_MEDIUM, false))
  , mListInitialized(false)
{
  addChild(&mGrid);

  // set up title
  mTitleScroll = std::make_shared<TextScrollComponent>(mWindow);
  mTitleScroll->setAlignment(::Alignment::Center);
  mTitleScroll->setText(_("QUICK JUMP"));

  mGrid.setEntry(mTitleScroll, Vector2i(0, 0), false, true);

  // set up list which will never change (externally, anyway)
  mList = std::make_shared<TextListComponent<QuickJumpItemHolder::TrackedItem*>>(mWindow);
  mList->SetOverlayInterface(this);
  mList->SetCursorChangedCallback(this);
  mList->setUseL1R1(true);

  mList->setSelectorHeight(mList->EntryHeight());
  mList->setShiftSelectedTextColor(true);
  mList->setAutoAlternate(true);
  mList->setCursorChangedCallback([this](CursorState state) { if (state == CursorState::Stopped) mWindow.UpdateHelpSystem(); });
  mGrid.setEntry(mList, Vector2i(0, 1), true, true);

  mGrid.resetCursor();
  ApplyTheme();
}

void GuiQuickJump::ApplyTheme()
{
  mTitleScroll->setFont(mTitleFont);
  mTitleScroll->setColor(0xFFFFFFFF);

  mList->setFont(mTheme.SmallText().font);
  mList->setSelectedColor(0); // If defined, it's given priority on color shift
  mList->setSelectorColor(0xFFFFFFC0);
  mList->setColorAt(0, 0xFFFFFFFF); // Text color
  mList->setColorAt(1, 0x000000FF); // selected text coplor
  mList->setSelectorHeight(0); // Reset selector height, based on new font height
}

void GuiQuickJump::SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface)
{
  (void)theme;
  (void)refreshOnly;
  (void)interface;
  ApplyTheme();
  SetListSize();
}

void GuiQuickJump::OverlayApply(const Transform4x4f& parentTrans, int leftWidth, int rightWidth, int labelWidth, const Rectangle& area, QuickJumpItemHolder::TrackedItem* const& data, int index, unsigned int& color)
{
  (void)parentTrans;
  (void)leftWidth;
  (void)rightWidth;
  (void)area;
  (void)labelWidth;
  (void)data;
  (void)color;
  (void)index;
}

float GuiQuickJump::OverlayGetLeftOffset(QuickJumpItemHolder::TrackedItem* const& data, int labelWidth)
{
  (void)labelWidth;
  switch(data->Deepness())
  {
    case QuickJumpItemHolder::ItemDeepness::Level0: break;
    case QuickJumpItemHolder::ItemDeepness::Level1: return (float)mTheme.SmallText().font->Height();
    case QuickJumpItemHolder::ItemDeepness::Level2: return (float)mTheme.SmallText().font->Height() * 2.f;
  }
  return 0;
}

float GuiQuickJump::OverlayGetRightOffset(QuickJumpItemHolder::TrackedItem* const& data, int labelWidth) { (void)data, (void)labelWidth; return 0; }

float GuiQuickJump::TitleHeight() const
{
  float padding = (Renderer::Instance().DisplayHeightAsFloat() * 0.03f);
  return (float)mTitleFont->Height() + padding;
}

float GuiQuickJump::ListWidth() const
{
  int textWidth = mTrackedItems.GetLargestTextWidth(*mTheme.SmallText().font);
  int titleWidth = mTitleFont->TextWidth(mTitleScroll->getValue());
  textWidth = Math::max(textWidth, titleWidth);
  return (float)Math::min(textWidth, Renderer::Instance().DisplayWidthAsInt() / 3);
}

float GuiQuickJump::ListHeight() const
{
  const float maxHeight = Renderer::Instance().DisplayHeightAsFloat() / 2;
  /*float baseHeight = TitleHeight() + 2;
  int maxrows = (int)(maxHeight - baseHeight) / (int)mList->EntryHeight();
  return baseHeight + (float)(Math::max(mList->size(), maxrows)) * mList->EntryHeight();*/
  return maxHeight;
}

void GuiQuickJump::SetListSize()
{
  setSize(ListWidth(), ListHeight());
  setPosition((Renderer::Instance().DisplayWidthAsFloat() - getWidth()) / 2.f, (Renderer::Instance().DisplayHeightAsFloat() - getHeight()) / 2.f);

  mWindow.UpdateHelpSystem();
}

void GuiQuickJump::onSizeChanged()
{
  // update grid row/col sizes
  mGrid.setRowHeightPerc(0, TitleHeight() / mSize.y());

  mGrid.setSize(mSize);
}

void GuiQuickJump::BuildList(QuickJumpItemHolder::TrackedItem* forceItem)
{
  mList->clear(mTrackedItems.TotalItemCount());
  BuildListItems(mTrackedItems.GetRoot(), forceItem);
}

void GuiQuickJump::BuildListItems(QuickJumpItemHolder::TrackedItem& from, QuickJumpItemHolder::TrackedItem* forceItem)
{
  for(QuickJumpItemHolder::TrackedItem& item : from.Children())
  {
    // Add current item
    String display(item.Children().empty() ? "• " : (item.IsFolded() ? "▶ " : "▼ "));
    display.Append(item.Display());
    mList->add(display, &item, 0, HorizontalAlignment::Left);
    if ((item.IsInitialItem() && forceItem == nullptr) ||
        &item == forceItem) mList->setCursorIndex(mList->size() - 1);

    // Add children
    if (!item.Children().empty() && !item.IsFolded())
      BuildListItems(item, forceItem);
  }
}

bool GuiQuickJump::ProcessInput(const InputCompactEvent& event)
{
  // Close with Cancel
  if (event.CancelReleased()) { Close(); return true; }

  // Fold/Unfold with Validate
  if (event.ValidReleased())
    if (QuickJumpItemHolder::TrackedItem* item = mList->getSelected(); !item->Children().empty())
    {
      item->ToggleFolded();
      BuildList(item);
      return true;
    }

  // Fold with Left
  if (event.LeftReleased())
  {
    QuickJumpItemHolder::TrackedItem* item = mList->getSelected();
    // This item is the parent, fold it
    if (!item->Children().empty() && !item->IsFolded())
    {
      item->SetFolded(true);
      BuildList(item);
      return true;
    }
    // This item is a child, try to fold the parent
    if (QuickJumpItemHolder::ItemDeepness originalDeepness = item->Deepness(); originalDeepness > QuickJumpItemHolder::ItemDeepness::Level0)
    {
      for(int i = mList->getCursorIndex() - 1; i > 0; --i)
        if (item = mList->getObjectAt(i); item->Deepness() != originalDeepness)
        {
          item->SetFolded(true);
          BuildList(item);
          return true;
        }
    }
  }

  // Unfold with Right
  if (event.RightReleased())
    if (QuickJumpItemHolder::TrackedItem* item = mList->getSelected(); !item->Children().empty() && item->IsFolded())
    {
      item->SetFolded(false);
      BuildList(item);
      return true;
    }

  // First/last with West/North (SNES Y/X)
  if (event.North())
  {
    mList->setCursorIndex(0);
    return true;
  }
  if (event.West())
  {
    mList->setCursorIndex(mList->size() - 1);
    return true;
  }

  return Component::ProcessInput(event);
}

bool GuiQuickJump::CollectHelpItems(Help& help)
{
  help.Set(Help::Cancel(), _("BACK"))
      .Set(HelpType::LeftRight, _("FOLD/UNFOLD"))
      .Set(HelpType::LR, _("FAST MOVE"))
      .Set(HelpType::Y, _("BOTTOM"))
      .Set(HelpType::X, _("TOP"));

  if (!mList->isEmpty())
    if (!mList->getSelected()->Children().empty())
      help.Set(Help::Valid(), _("UNFOLD"));

  return true;
}
