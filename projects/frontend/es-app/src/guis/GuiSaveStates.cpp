//
// Created by gugue_u on 04/01/2023.
//

#include <components/ButtonComponent.h>
#include <components/MenuComponent.h>
#include <guis/GuiTextEditPopupKeyboard.h>
#include <views/ViewController.h>
#include "GuiSaveStates.h"
#include <games/GameFilesUtils.h>
#include <algorithm>

#define TITLE_HEIGHT (mTitle->getFont()->Height() + Renderer::Instance().DisplayHeightAsFloat()*0.0437f )

GuiSaveStates::GuiSaveStates(WindowManager& window, SystemManager& systemManager, FileData& game, ISaveStateSlotNotifier* notifier, bool fromMenu)
  : Gui(window)
  , mSystemManager(systemManager)
  , mGrid(window, Vector2i(5, 6))
  , mMenuTheme(ThemeManager::Instance().Menu())
  , mList(nullptr)
  , mGame(game)
  , mFromMenu(fromMenu)
  , mCurrentState(Path(""))
  , mSort(Sort::Descending)
  , mInterface(notifier)
{
  mIsLibretro = EmulatorManager::GetGameEmulator(mGame).IsLibretro();
  addChild(&mGrid);

  initGridsNStuff();

  const float height = Renderer::Instance().DisplayHeightAsFloat() * 0.78f;
  const float width = Renderer::Instance().DisplayWidthAsFloat() * (Renderer::Instance().Is240p() ? 0.95f : 0.85f);
  setSize(width, height);

  setPosition((Renderer::Instance().DisplayWidthAsFloat() - mSize.x()) / 2,
              (Renderer::Instance().DisplayHeightAsFloat() - mSize.y()) / 2);

  PopulateGrid();
}

void GuiSaveStates::onSizeChanged()
{

  // update grid row/col sizes
  mGrid.setColWidthPerc(0, 0.02f);
  mGrid.setColWidthPerc(1, Renderer::Instance().Is240p() ? 0.55 : 0.4f);
  mGrid.setColWidthPerc(2, 0.02f);
  mGrid.setColWidthPerc(3, 0.f); // Auto size
  mGrid.setColWidthPerc(4, 0.02f);

  mGrid.setRowHeightPerc(0, 0.02f);
  mGrid.setRowHeightPerc(1, TITLE_HEIGHT / mSize.y());
  mGrid.setRowHeightPerc(2, TITLE_HEIGHT / mSize.y());
  mGrid.setRowHeightPerc(3, 0.04f);
  mGrid.setRowHeightPerc(4, 0.f); // Auto size
  mGrid.setRowHeightPerc(5, 0.04f);

  mGrid.setSize(mSize);
}

void GuiSaveStates::initGridsNStuff()
{
  // Title
  mTitle = std::make_shared<TextComponent>(mWindow, _("SAVE STATES"), mMenuTheme.Text().font, mMenuTheme.Text().color, ::Alignment::Center);
  mGrid.setEntry(mTitle, Vector2i(1, 1), false, true, Vector2i(3, 1));

  // Game name
  mGameName = std::make_shared<TextComponent>(mWindow, mGame.Name(), mMenuTheme.Text().font, mMenuTheme.Text().color, ::Alignment::Center);
  mGameName->setUppercase(true);
  mGrid.setEntry(mGameName, Vector2i(1, 2), false, true, Vector2i(3, 2));

  // Slot list
  mList = std::make_shared<ComponentList>(mWindow);
  mList->setCursorChangedCallback([this](CursorState) { updateInformations(); });
  mGrid.setEntry(mList, Vector2i(1, 4), true, true, Vector2i(1, 1));

  // Thumbnail
  mThumbnail = std::make_shared<ImageComponent>(mWindow);
  mGrid.setEntry(mThumbnail, Vector2i(3, 4), false, true, Vector2i(1, 1));
}

bool GuiSaveStates::ProcessInput(const class InputCompactEvent & event)
{
  if (event.CancelReleased())
  {
    Close();
    return true;
  }
  if (event.WestReleased())
  {
    launch(-1);
    return true;
  }
  else if (event.ValidReleased() && mIsLibretro && mCurrentState.GetPath().Extension() != ".auto" && mList->size() != 0)
  {
    if (mCurrentState.GetSlotNumber() < 0)
    {
      launch(-1);
      return true;
    }

    launch(mCurrentState.GetSlotNumber());

    return true;
  }
  else if (event.SelectReleased())
  {
    if(mSort == Sort::Ascending) mSort = Sort::Descending;
    else if(mSort == Sort::Descending) mSort = Sort::Ascending;
    PopulateGrid();
  }
  else if (event.NorthReleased())
  {
    String message = _("Game").Append(": ").Append(mGame.Name()).Append("\n");
    message.Append(_("You are about to delete this state, confirm ?"))
           .Append("\n\n")
           .Append(mCurrentState.GetPath().Filename());

    auto* msg = new GuiMsgBoxScroll(mWindow, _("DELETE STATE, CONFIRM?"), message,
                                   _("YES"), [this]
                                   {
                                     Delete();
                                     PopulateGrid();
                                   },
                                   _("BACK"), {}, "", nullptr, ::Alignment::CenterLeft);

    msg->SetDefaultButton(1);

    mWindow.pushGui(msg);
    return true;
  }

  return Component::ProcessInput(event);
}

bool GuiSaveStates::CollectHelpItems(Help& help)
{
  help.Clear();
  String sort = mSort == Sort::Descending ? "ASC" : "DESC";

  help.Set(HelpType::UpDown, _("CHOOSE"))
      .Set(Help::Cancel(), _("BACK"))
      .Set(HelpType::Select, _("CHANGE ORDER") + " " + sort);

  if (!mList->isEmpty())
  {
    if (mIsLibretro && mCurrentState.GetPath().Extension() != ".auto")
      help.Set(Help::Valid(), _("LAUNCH GAME FROM STATE"));
    help.Set(HelpType::X, _("DELETE STATE SLOT"));
  }
  return true;
}

void GuiSaveStates::Update(int deltaTime)
{
  Component::Update(deltaTime);
}

void GuiSaveStates::Render(const Transform4x4f& parentTrans)
{
  Gui::Render(parentTrans);

  Transform4x4f trans = parentTrans * getTransform();
  renderChildren(trans);

  Renderer::SetMatrix(trans);
  Renderer::DrawRectangle(mGrid.getPosition().x() + mGrid.getColWidth(0),
                          mGrid.getPosition().y() + mGrid.getRowHeight(0, 3),
                          mGrid.getColWidth(1), mGrid.getRowHeight(4), 0x00000018);

}

void GuiSaveStates::PopulateGrid()
{
  mSaveStates = GameFilesUtils::GetGameSaveStateFiles(mGame);
  std::sort(mSaveStates.begin(), mSaveStates.end(), mSort == Sort::Ascending ? GuiSaveStates::asc : GuiSaveStates::desc);

  if (mList) mList->clear();

  ComponentListRow row;
  std::shared_ptr<TextScrollComponent> ed = std::make_shared<TextScrollComponent>(mWindow, _("none"), mMenuTheme.Text().font, mMenuTheme.Text().color, ::Alignment::CenterLeft);
  ed->setUppercase(true);

  row.addElement(ed, true);
  mList->addRow(row, false, true);

  for (auto& state : mSaveStates)
  {
    String text = _("SLOT");
    if (state.GetIsAuto()) text.Append(' ').Append(_("AUTO"));
    else text.Append('#').Append(state.GetSlotNumber()).Append(" - ").Append(state.GetDateTime().ToStringFormat("%YYYY/%MM/%dd %HH:%mm:%ss"));
    ed = std::make_shared<TextScrollComponent>(mWindow, text, mMenuTheme.Text().font, mMenuTheme.Text().color, ::Alignment::CenterLeft);
    ed->setUppercase(true);

    row.Clear();
    row.addElement(ed, true);
    mList->addRow(row, false, true);
  }
  if (mList->size() > 1) mList->setCursorIndex(1);

  updateInformations();
}

//called when changing cursor in mList to populate MD
void GuiSaveStates::updateInformations()
{
  mThumbnail->setImage(Path::Empty, false);
  if (mList->isEmpty()) return;

  int index = mList->getCursorIndex();
  if (index == 0)
  {
    mCurrentState = SaveState(Path::Empty);
    return;
  }

  mCurrentState = mSaveStates[index - 1];
  mThumbnail->setImage(mCurrentState.GetThrumbnail(), false);
  mThumbnail->setResize(mGrid.getColWidth(3), mGrid.getRowHeight(4));
  mThumbnail->setKeepRatio(true);
  mThumbnail->setOrigin(0.5f, 0.5f);
  mThumbnail->setPosition(mGrid.getPosition().x() + mGrid.getColWidth(0, 2) + (mGrid.getColWidth(3) / 2.f),
                          mGrid.getPosition().y() + mGrid.getRowHeight(0, 3) + (mGrid.getRowHeight(4) / 2.f));

  UpdateHelpBar();
}

void GuiSaveStates::launch(int slot)
{
  if (mFromMenu)
  {
    Vector3f target(Renderer::Instance().DisplayWidthAsFloat() / 2.0f,
                      Renderer::Instance().DisplayHeightAsFloat() / 2.0f, 0);
    GameLinkedData gameLinkedData = GameLinkedData();
    gameLinkedData.ConfigurableSaveState().SetSlotNumber(slot);
    ViewController::Instance().Launch(&mGame, gameLinkedData, target, !ViewController::Instance().IsInVirtualSystem());
  }
  else
  {
    if (mInterface != nullptr)
      mInterface->SaveStateSlotSelected(slot);
    Close();
  }
}

void GuiSaveStates::Delete()
{
  (void)mCurrentState.GetPath().Delete();
  (void)mCurrentState.GetThrumbnail().Delete();
  UpdateHelpBar();
  { LOG(LogDebug) << "[SAVESTATE] " << mCurrentState.GetPath().Filename() << " slot has been deleted"; }
}

//non compatibl libretro core pi4
//libretro-atari800
//libretro-easyrpg
//libretro-gw
//libretro-lowresnx
//libretro-lutro
//libretro-mame2010
//libretro-minivmac
//libretro-px68k
//libretro-retro8
//libretro-virtualjaguar
