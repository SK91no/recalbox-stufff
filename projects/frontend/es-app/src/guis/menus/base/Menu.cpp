//
// Created by bkg2k on 20/08/24.
//

#include <guis/menus/base/Menu.h>
#include <themes/ThemeManager.h>
#include "components/TextScrollComponent.h"
#include "ItemSubMenu.h"
#include "ItemSwitch.h"
#include "ItemSlider.h"
#include "ItemEditable.h"
#include "ItemText.h"
#include "ItemRating.h"
#include "ItemAction.h"
#include "ItemBar.h"
#include "Upgrade.h"
#include "ItemHeader.h"
#include "MainRunner.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiInfoPopup.h"
#include "ItemSelector.h"
#include "animations/LambdaAnimation.h"

#include <patreon/PatronInfo.h>
#include <guis/menus/base/MenuColors.h>

Menu::Menu(WindowManager& window, const InheritableContext& context, const String& title, const String& footer, bool animated)
  : Gui(window)
  , mTheme(ThemeManager::Instance().Menu())
  , mConf(RecalboxConf::Instance())
  , mContext(context)
  , mCache(ThemeManager::Instance().MenuCache())
  , mGrid(window, Vector2i(1, 3))
  , mTitle(title)
  , mFooter(footer)
  , mLastSelectedItem(nullptr)
  , mTextMargin(3 + Renderer::Instance().DisplayWidthAsInt() / 320)
  , mIconMargin(3 + Renderer::Instance().DisplayWidthAsInt() / 320)
  , mInitialIndex(0)
  , mMenuInitialized(false)
  , mAnimated(animated)
  , m240p(Renderer::Instance().Is240p())
{
  addChild(&mGrid);

  std::shared_ptr<ComponentGrid> headerGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(5, 1));
  headerGrid->setColWidthPerc(0, 0.02);
  headerGrid->setColWidthPerc(4, 0.02);

  // set up title
  mTitleScroll = std::make_shared<TextScrollComponent>(mWindow);
  mTitleScroll->setAlignment(Alignment::Center);
  mTitleScroll->setText(title.ToUpperCaseUTF8());

  headerGrid->setEntry(mTitleScroll, Vector2i(1, 0), false, true, Vector2i(3, 1));
  mGrid.setEntry(headerGrid, Vector2i(0, 0), false, true);

  //  set up footer
  if (mFooter.empty()) mFooter = String("RECALBOX VERSION ").Append(Upgrade::CurrentVersion()).Append(PatronInfo::Instance().IsPatron() ? " - PATRON FLAVOR": "").UpperCase();
  mFooterScroll = std::make_shared<TextScrollComponent>(mWindow);
  mFooterScroll->setAlignment(Alignment::Center);
  mFooterScroll->setText(mFooter);
  mGrid.setEntry(mFooterScroll, Vector2i(0, 2), false, true);

  // set up list which will never change (externally, anyway)
  mList = std::make_shared<TextListComponent<ItemBase*>>(mWindow);
  mList->SetOverlayInterface(this);

  mList->setSelectorHeight(mList->EntryHeight());
  mList->setShiftSelectedTextColor(true);
  mList->setAutoAlternate(true);
  mList->setCursorChangedCallback([this](CursorState state) { if (state == CursorState::Stopped) mWindow.UpdateHelpSystem(); });
  mGrid.setEntry(mList, Vector2i(0, 1), true, true);

  mGrid.resetCursor();
  ApplyTheme();
}

void Menu::ApplyTheme()
{
  mTitleScroll->setFont(mTheme.Title().font);
  mTitleScroll->setColor(mTheme.Title().color);

  mFooterScroll->setFont(mTheme.Footer().font);
  mFooterScroll->setColor(mTheme.Footer().color);

  mList->setFont(mTheme.Text().font);
  mList->setSelectedColor(0); // If defined, it's given priority on color shift
  mList->setSelectorColor(mTheme.Text().selectorColor);
  mList->setColorAt(MenuColors::sSelectableColor, mTheme.Text().color);                                               // Text color
  mList->setColorAt(MenuColors::sSelectableSelectedColor, mTheme.Text().selectedColor);                               // selected text coplor
  mList->setColorAt(MenuColors::sUnselectableColor, MenuColors::Alpha25Percent(mTheme.Text().color));                 // Grayed color
  mList->setColorAt(MenuColors::sUnselectableSelectedColor, MenuColors::Alpha25Percent(mTheme.Text().selectedColor)); // Grayed selected color
  mList->setColorAt(MenuColors::sHeaderColor, mTheme.Section().color);                                                // Header color
  mList->setColorAt(MenuColors::sHeaderSelectedColor, mTheme.Section().selectedColor);                                // Header selected color
  mList->setSelectorHeight(0); // Reset selector height, based on new font height
  mList->setUseL1R1(true);

  //! Clear graphical element cache
  mCache.Clear();
}

void Menu::SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface)
{
  (void)theme;
  (void)refreshOnly;
  (void)interface;
  ApplyTheme();
  SetMenuSize(true);
}

Menu::~Menu()
{
  for(ItemBase* item : mItems)
    delete(item);
}

void Menu::Reboot()
{
  MainRunner::RequestQuit(MainRunner::ExitState::NormalReboot);
}

void Menu::RequestReboot()
{
  mWindow.pushGui(new GuiMsgBox(mWindow, _("THE SYSTEM WILL NOW REBOOT"), _("OK"), Reboot, _("LATER"), std::bind(Menu::RebootPending, &mWindow)));
}

void Menu::RebootPending(WindowManager* window)
{
  static bool pending = false;
  if (!pending)
  {
    window->InfoPopupAdd(new GuiInfoPopup(*window, _("A reboot is required to apply pending changes."), 10000, PopupType::Reboot));
    pending = true;
  }
}

void Menu::Relaunch()
{
  MainRunner::RequestQuit(MainRunner::ExitState::Relaunch);
}

void Menu::RequestRelaunch()
{
  mWindow.pushGui(new GuiMsgBox(mWindow, _("EmulationStation must relaunch to apply your changes."), _("OK"), Relaunch, _("LATER"), std::bind(Menu::RebootPending, &mWindow)));
}

bool Menu::ProcessInput(const InputCompactEvent& event)
{
  // Close?
  if (event.CancelReleased()) { Close(); return true; }
  // Close all?
  if (event.StartReleased()) { mWindow.CloseAll(); return true; }

  // Get selected item
  if (mList->isEmpty()) return false;
  const ItemBase& item = *mList->getSelected();
  // Help
  if (event.WestPressed())
  {
    if (item.IsSelectable() && item.HasHelpText())
    {
      mWindow.InfoPopupAddRegular(mList->getSelected()->HelpText(), RecalboxConf::Instance().GetPopupHelp(),
                                  PopupType::Help, false);
      return true;
    }
    if (item.HasHelpUnselectableText())
    {
      mWindow.InfoPopupAddRegular(mList->getSelected()->HelpUnselectableText(), RecalboxConf::Instance().GetPopupHelp(),
                                  PopupType::Help, false);
      return true;
    }
  }
  // Then to the currently selected menu entry
  if (mList->getSelected()->IsSelectable())
    if (mList->getSelected()->ProcessInput(event)) return true;

  // Give event to parent
  return Gui::ProcessInput(event);
}

bool Menu::IsDisplayable(int index) const
{
  const ItemBase& first = *mItems[index];
  // Normal item
  if (!first.IsHeader())
    return first.IsDisplayable();
  // Header
  while(++index < mItems.Count())
  {
    const ItemBase& next = *mItems[index];
    if (next.IsHeader()) break;
    if (next.IsDisplayable()) return true;
  }
  return false;
}

void Menu::BuildMenu()
{
  mLastSelectedItem = nullptr;
  // Clear all
  for(ItemBase* item : mItems) delete item;
  mItems.Clear();
  mList->clear();

  // Rebuild
  bool opened = true;
  BuildMenuItems();
  for(int i = 0; i < mItems.Count(); ++i)
    if (IsDisplayable(i))
    {
      if (ItemBase* item = mItems[i]; item->ItemType() == ItemBase::Type::Header)
      {
        opened = item->AsHeader()->IsOpened();
        mList->add(String::Empty, item, item->IsSelectable() ? MenuColors::sSelectableColor : MenuColors::sHeaderColor, -1/*MenuColors::sHeaderBackgroundColor*/, HorizontalAlignment::Right);
      }
      else if (opened)
        mList->add(item->Label(), item, item->IsSelectable() ? MenuColors::sSelectableColor : MenuColors::sUnselectableColor, HorizontalAlignment::Left);
    }

  // Initial index
  if (mInitialIndex < 0) mInitialIndex = 0;
  if (mInitialIndex >= mList->size()) mInitialIndex = mList->size() - 1;
  if (mInitialIndex >= 0)
    mList->setCursorIndex(mInitialIndex);
}


void Menu::ItemSelectabilityChanged(ItemBase& itemChanged)
{
  (void)itemChanged;
  for(int i = mList->size(); --i >= 0; )
  {
    ItemBase* item = mList->getObjectAt(i);
    mList->getSelectedEntryAt(i).colorId = item->IsSelectable() ? MenuColors::sSelectableColor : MenuColors::sUnselectableColor;
  }
}

float Menu::TitleHeight() const
{
  float padding = (Renderer::Instance().DisplayHeightAsFloat() * 0.0537f);
  return (float)mTheme.Title().font->RequestedHeight() + padding;
}

float Menu::FooterHeight() const
{
  return (float)mTheme.Footer().font->Height() * 2.f;
}

Rectangle Menu::GetMenuMaximumArea() const
{
  float screenWidth = Renderer::Instance().DisplayWidthAsFloat();
  float screenHeight = Renderer::Instance().DisplayHeightAsFloat();
  float maxWidth = screenWidth * 0.9f;
  float maxHeight = screenHeight * ThemeManager::Instance().Menu().Height();
  return Rectangle((screenWidth - maxWidth) / 2, (screenHeight - maxHeight) / 2, maxWidth, maxHeight);
}

float Menu::MenuWidth() const
{
  //return Math::min(Renderer::Instance().DisplayHeightAsFloat() * 1.2f,
  //                 Renderer::Instance().DisplayWidthAsFloat() * 0.85f);
  return Renderer::Instance().DisplayWidthAsFloat() * 0.85f;
}

float Menu::MenuHeight() const
{
  float menuTheme = ThemeManager::Instance().Menu().Height();
  const float maxHeight = Renderer::Instance().DisplayHeightAsFloat() * menuTheme;
  float baseHeight = TitleHeight() + FooterHeight() + 2;
  int maxrows = (int)(maxHeight - baseHeight) / (int)mList->EntryHeight();
  return baseHeight + ((float)(Math::min(mList->size(), maxrows)) * mList->EntryHeight());
}

void Menu::SetMenuSize(bool doNotAnimate)
{
  bool doAnimate = mAnimated && !doNotAnimate;
  setSize(MenuWidth(), MenuHeight());
  setPosition((Renderer::Instance().DisplayWidthAsFloat() - getWidth()) / 2.f,
              doAnimate ? Renderer::Instance().DisplayHeightAsFloat()
                        : (Renderer::Instance().DisplayHeightAsFloat() - getHeight()) / 2.f);

  if (mAnimated && !doNotAnimate)
  {
    // Animation
    auto fadeFunc = [this](float t)
    {
      setOpacity((int) lerp<float>(0, 255, t));
      setPosition(getPosition().x(), lerp<float>(Renderer::Instance().DisplayHeightAsFloat(), (Renderer::Instance().DisplayHeightAsFloat() - mSize.y()) / 2, t));
    };

    setOpacity(0);
    setAnimation(new LambdaAnimation(fadeFunc, 200), 0);
  }

  mWindow.UpdateHelpSystem();
}

void Menu::onSizeChanged()
{
  // update grid row/col sizes
  mGrid.setRowHeightPerc(0, TitleHeight() / mSize.y());
  mGrid.setRowHeightPerc(2, FooterHeight() / mSize.y());

  mGrid.setSize(mSize);
}

float Menu::OverlayGetLeftOffset(ItemBase* const& data, int labelWidth)
{
  int w = 0;
  if (data->HasValidIcon())
    w = Math::roundi(mList->EntryHeight() * .8f) + mIconMargin;
  return (float)w + data->OverlayLeftOffset(labelWidth + (mTextMargin * 2)) + (float)mTextMargin;
}

void Menu::OverlayApply(const Transform4x4f& parentTrans, int leftOverrideWidth, int rightOverrideWidth, int labelWidth, const Rectangle& area, ItemBase* const& data, int index, unsigned int& color)
{
  (void)parentTrans;
  (void)leftOverrideWidth;
  (void)rightOverrideWidth;
  (void)color;
  (void)index;

  bool selected = data == mList->getSelected();
  Colors::ColorRGBA iconColor = selected ? mTheme.Text().selectedColor : mTheme.Text().color;
  iconColor = data->IsSelectable() ? iconColor : MenuColors::Alpha25Percent(iconColor);
  int offset = 0;
  Rectangle inner(area);
  if (data->HasValidIcon())
  {
    int h = (int)(mList->EntryHeight() * 0.8f);
    int x = (int)inner.Left() + mIconMargin;
    int y = (int)(inner.Top() + ((inner.Height() - (float)h) / 2.f));
    Texture texture = data->IconPath().IsEmpty() ? mCache.GetIcon(data->Icon(), 0, h) : mCache.GetFromPath(data->IconPath(), 0, h);
    // Draw
    texture.Render(x, y, h, h, true, false, iconColor);
    offset = mIconMargin;
    inner.MoveLeft((float)(h + offset));
  }
  data->OverlayDraw(labelWidth + (mTextMargin * 2) + offset, inner, iconColor, selected);
}

ItemHeader* Menu::AddHeader(const String& label)
{
  ItemHeader* item = new ItemHeader(*this, *this, mList->IsUppercase() ? label.ToUpperCaseUTF8() : label, mTheme, this);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}

ItemSubMenu* Menu::AddSubMenu(const String& label, int id, ISubMenuSelected* interface, const String& help, bool unselectable, const String& helpUnselectable)
{
  ItemSubMenu* item = new ItemSubMenu(*this, *this, label, mTheme, interface, id, help, unselectable, helpUnselectable);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}

ItemSubMenu* Menu::AddSubMenu(const String& label, MenuThemeData::MenuIcons::Type icon, int id, ISubMenuSelected* interface,
                     const String& help, bool unselectable, const String& helpUnselectable)
{
  ItemSubMenu* item = new ItemSubMenu(*this, *this, label, mTheme, interface, id, icon, help, unselectable, helpUnselectable);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}

ItemSwitch* Menu::AddSwitch(const String& label, bool value, int id, ISwitchChanged* interface, const String& help, bool unselectable, const String& helpUnselectable)
{
  ItemSwitch* item = new ItemSwitch(*this, *this, label, mTheme, interface, id, value, help, unselectable, helpUnselectable);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}

ItemSwitch* Menu::AddSwitch(const MenuThemeData::MenuIcons::Type icon, const String& label, bool value, int id, ISwitchChanged* interface,
                    const String& help, bool unselectable, const String& helpUnselectable)
{
  ItemSwitch* item = new ItemSwitch(*this, *this, label, mTheme, interface, id, icon, value, help, unselectable, helpUnselectable);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}

ItemSwitch* Menu::AddSwitch(const Path& icon, const String& label, bool value, int id, ISwitchChanged* interface,
                    const String& help, bool unselectable, const String& helpUnselectable)
{
  ItemSwitch* item = new ItemSwitch(*this, *this, label, mTheme, interface, id, icon, value, help, unselectable, helpUnselectable);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}

ItemSlider* Menu::AddSlider(const String& label, float min, float max, float inc, float defaultValue, float value, const String& suffix, int id,
                    ISliderChanged* interface, const String& help, bool unselectable, const String& helpUnselectable)
{
  ItemSlider* item = new ItemSlider(*this, *this, label, mTheme, interface, id, min, max, inc, value, defaultValue, suffix, help, unselectable, helpUnselectable);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}

ItemText* Menu::AddText(const String& label, const String& value, const String& help)
{
  ItemText* item = new ItemText(*this, *this, label, mTheme, 0, value, 0, help);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}

ItemText* Menu::AddText(const String& label, const String& value, unsigned int color, const String& help)
{
  ItemText* item = new ItemText(*this, *this, label, mTheme, 0, value, color, help);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}

ItemEditable* Menu::AddEditable(const String& edittitle, const String& label, const String& value, int id,
                                IEditableChanged* interface, const String& help, bool masked, bool unselectable,
                                const String& helpUnselectable, VKCharFilter filter)
{
  ItemEditable* item = new ItemEditable(*this, mWindow, *this, edittitle, label, mTheme, interface, id, value, masked, help, unselectable, helpUnselectable, filter);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}


ItemEditable* Menu::AddEditable(const String& label, const String& value, int id, IEditableChanged* interface, const String& help,
                                bool masked, bool unselectable, const String& helpUnselectable, VKCharFilter filter)
{
  ItemEditable* item = new ItemEditable(*this, mWindow, *this, String::Empty, label, mTheme, interface, id, value, masked, help, unselectable, helpUnselectable, filter);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}

ItemAction* Menu::AddAction(const String& label, const String& buttonWord, int id, bool positive, IActionTriggered* interface, const String& help, bool unselectable, const String& helpUnselectable)
{
  ItemAction* item = new ItemAction(*this, *this, label, buttonWord, mTheme, interface, id, positive, help, unselectable, helpUnselectable);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}

ItemAction* Menu::AddAction(const MenuThemeData::MenuIcons::Type icon, const String& label, const String& buttonWord, int id, bool positive, IActionTriggered* interface, const String& help, bool unselectable, const String& helpUnselectable)
{
  ItemAction* item = new ItemAction(*this, *this, icon, label, buttonWord, mTheme, interface, id, positive, help, unselectable, helpUnselectable);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}

ItemRating* Menu::AddRating(const String& label, float value, int id, IRatingChanged* interface, const String& help, bool unselectable, const String& helpUnselectable)
{
  ItemRating* item = new ItemRating(*this, *this, label, mTheme, interface, id, value, help, unselectable, helpUnselectable);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}

ItemBar* Menu::AddBar(const String& label, const String& text, float ratio, const String& help, bool unselectable,
                           const String& helpUnselectable)
{
  ItemBar* item = new ItemBar(*this, *this, label, mTheme, 0, text, ratio, 0, help, unselectable, helpUnselectable);
  item->MergeContext(mContext);
  mItems.Add(item);
  return item;
}
