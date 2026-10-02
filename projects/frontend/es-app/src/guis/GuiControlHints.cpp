//
// Created by bkg2k on 03/05/2020.
//

#include "GuiControlHints.h"
#include "components/TextScrollComponent.h"
#include <padtokeyboard/pad/PadItems.h>
#include <padtokeyboard/pad/MappingConfiguration.h>
#include <utils/storage/HashMap.h>
#include <memory>
#include <utils/math/Misc.h>
#include <Renderer.h>
#include <utils/locale/LocaleHelper.h>
#include <components/TextComponent.h>
#include <components/ImageComponent.h>
#include <themes/MenuThemeData.h>
#include <input/InputCompactEvent.h>
#include <themes/ThemeManager.h>
#include <WindowManager.h>

#include "components/PictureComponent.h"

static const char* IconPathMap(PadItems padItems)
{
  static const HashMap<PadItems, const char*> sIconPathMap =
    {
      { PadItems::Up,                ":/help/dpad_up.svg" },
      { PadItems::Down,              ":/help/dpad_down.svg" },
      { PadItems::Left,              ":/help/dpad_left.svg" },
      { PadItems::Right,             ":/help/dpad_right.svg" },
      { PadItems::Hotkey,            ":/help/button_hotkey.svg" },
      { PadItems::Start,             ":/help/button_start.svg" },
      { PadItems::Select,            ":/help/button_select.svg" },
      { PadItems::East,                 ":/help/button_a.svg" },
      { PadItems::South,                 ":/help/button_b.svg" },
      { PadItems::North,                 ":/help/button_x.svg" },
      { PadItems::West,                 ":/help/button_y.svg" },
      { PadItems::L1,                ":/help/button_l.svg" },
      { PadItems::R1,                ":/help/button_r.svg" },
      { PadItems::L2,                ":/help/button_l2.svg" },
      { PadItems::R2,                ":/help/button_r2.svg" },
      { PadItems::L3,                ":/help/button_l3.svg" },
      { PadItems::R3,                ":/help/button_r3.svg" },
      { PadItems::J1Up,              ":/help/joystick_up_L.svg" },
      { PadItems::J1Down,            ":/help/joystick_down_L.svg" },
      { PadItems::J1Left,            ":/help/joystick_left_L.svg" },
      { PadItems::J1Right,           ":/help/joystick_right_L.svg" },
      { PadItems::J2Up,              ":/help/joystick_up_R.svg" },
      { PadItems::J2Down,            ":/help/joystick_down_R.svg" },
      { PadItems::J2Left,            ":/help/joystick_left_R.svg" },
      { PadItems::J2Right,           ":/help/joystick_right_R.svg" },
      { PadItems::HotKeyUp,          ":/help/dpad_up.svg" },
      { PadItems::HotKeyRight,       ":/help/dpad_down.svg" },
      { PadItems::HotKeyDown,        ":/help/dpad_left.svg" },
      { PadItems::HotKeyLeft,        ":/help/dpad_right.svg" },
      { PadItems::HotKeyEast,           ":/help/button_a.svg" },
      { PadItems::HotKeySouth,           ":/help/button_b.svg" },
      { PadItems::HotKeyNorth,           ":/help/button_x.svg" },
      { PadItems::HotKeyWest,           ":/help/button_y.svg" },
      { PadItems::HotKeyL1,          ":/help/button_l.svg" },
      { PadItems::HotKeyR1,          ":/help/button_r.svg" },
      { PadItems::HotKeyL2,          ":/help/button_l2.svg" },
      { PadItems::HotKeyR2,          ":/help/button_r2.svg" },
      { PadItems::HotKeyL3,          ":/help/button_l3.svg" },
      { PadItems::HotKeyR3,          ":/help/button_r3.svg" },
      { PadItems::HotKeyStart,       ":/help/button_start.svg" },
      { PadItems::HotKeyJ1Up,        ":/help/joystick_up_L.svg" },
      { PadItems::HotKeyJ1Down,      ":/help/joystick_down_L.svg" },
      { PadItems::HotKeyJ1Left,      ":/help/joystick_left_L.svg" },
      { PadItems::HotKeyJ1Right,     ":/help/joystick_right_L.svg" },
      { PadItems::HotKeyJ2Up,        ":/help/joystick_up_R.svg" },
      { PadItems::HotKeyJ2Down,      ":/help/joystick_down_R.svg" },
      { PadItems::HotKeyJ2Left,      ":/help/joystick_left_R.svg" },
      { PadItems::HotKeyJ2Right,     ":/help/joystick_right_R.svg" },
    };
  return sIconPathMap.get_or_return_default(padItems);
}

static String IconTextMap(PadItems padItems, int pad)
{
  static const HashMap<PadItems, String> sIconTextMap =
    {
      { PadItems::Up,                _("UP") },
      { PadItems::Down,              _("DOWN") },
      { PadItems::Left,              _("LEFT") },
      { PadItems::Right,             _("RIGHT") },
      { PadItems::Hotkey,            _("HOTKEY") },
      { PadItems::Start,             "START" },
      { PadItems::Select,            "SELECT" },
      { PadItems::East,                 "A" },
      { PadItems::South,                 "B" },
      { PadItems::North,                 "X" },
      { PadItems::West,                 "Y" },
      { PadItems::L1,                "L1" },
      { PadItems::R1,                "R1" },
      { PadItems::L2,                "L2" },
      { PadItems::R2,                "R2" },
      { PadItems::L3,                "L3" },
      { PadItems::R3,                "R3" },
      { PadItems::J1Up,              _("J1 UP") },
      { PadItems::J1Down,            _("J1 DOWN") },
      { PadItems::J1Left,            _("J1 LEFT") },
      { PadItems::J1Right,           _("J1 RIGHT") },
      { PadItems::J2Up,              _("J2 UP") },
      { PadItems::J2Down,            _("J2 DOWN") },
      { PadItems::J2Left,            _("J2 LEFT") },
      { PadItems::J2Right,           _("J2 RIGHT") },
      { PadItems::HotKeyUp,          _("HOTKEY").Append('+').Append(_("UP")) },
      { PadItems::HotKeyRight,       _("HOTKEY").Append('+').Append(_("DOWN")) },
      { PadItems::HotKeyDown,        _("HOTKEY").Append('+').Append(_("LEFT")) },
      { PadItems::HotKeyLeft,        _("HOTKEY").Append('+').Append(_("RIGHT")) },
      { PadItems::HotKeyEast,           _("HOTKEY").Append('+').Append("A") },
      { PadItems::HotKeySouth,           _("HOTKEY").Append('+').Append("B") },
      { PadItems::HotKeyNorth,           _("HOTKEY").Append('+').Append("X") },
      { PadItems::HotKeyWest,           _("HOTKEY").Append('+').Append("Y") },
      { PadItems::HotKeyL1,          _("HOTKEY").Append('+').Append("L1") },
      { PadItems::HotKeyR1,          _("HOTKEY").Append('+').Append("R1") },
      { PadItems::HotKeyL2,          _("HOTKEY").Append('+').Append("L2") },
      { PadItems::HotKeyR2,          _("HOTKEY").Append('+').Append("R2") },
      { PadItems::HotKeyL3,          _("HOTKEY").Append('+').Append("L3") },
      { PadItems::HotKeyR3,          _("HOTKEY").Append('+').Append("R3") },
      { PadItems::HotKeyStart,       _("HOTKEY").Append('+').Append("START") },
      { PadItems::HotKeyJ1Up,        _("HOTKEY").Append('+').Append(_("J1 UP")) },
      { PadItems::HotKeyJ1Down,      _("HOTKEY").Append('+').Append(_("J1 DOWN")) },
      { PadItems::HotKeyJ1Left,      _("HOTKEY").Append('+').Append(_("J1 LEFT")) },
      { PadItems::HotKeyJ1Right,     _("HOTKEY").Append('+').Append(_("J1 RIGHT")) },
      { PadItems::HotKeyJ2Up,        _("HOTKEY").Append('+').Append(_("J2 UP")) },
      { PadItems::HotKeyJ2Down,      _("HOTKEY").Append('+').Append(_("J2 DOWN")) },
      { PadItems::HotKeyJ2Left,      _("HOTKEY").Append('+').Append(_("J2 LEFT")) },
      { PadItems::HotKeyJ2Right,     _("HOTKEY").Append('+').Append(_("J2 RIGHT")) },
    };

  String result(sIconTextMap.get_or_return_default(padItems));
  result.Append(" #", 2).Append(pad);
  return result;
}

GuiControlHints::GuiControlHints(WindowManager& window, const Path& romPath)
  : Gui(window),
    mGrid(window, { 0, 0}),
    mRows(0),
    mColumns(0)
{
  MappingConfiguration mapping(romPath);
  if (!mapping.Valid())
  {
    Close();
    return;
  }

  // Add grid as the main component
  addChild(&mGrid);

  // Theme
  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

  NewFont* font = mWindow.HelpBar().TextFont();

  // Keep in sync with HelpComponent to avoid rasterized svg to be stretched
  const float height = Math::round((float)font->Height() * 1.75f);

  // Some calculation to adapt row/column regarding the number of items
  float windowWidth = Renderer::Instance().DisplayWidthAsFloat() * 0.60f;
  mRows = mapping.Count();
  mColumns = 1;
  if (height * (float)mRows > Renderer::Instance().DisplayHeightAsFloat() * 0.60f)
  {
    mRows = (mRows + 1) / 2;
    mColumns = 2;
    windowWidth = Renderer::Instance().DisplayWidthAsFloat() * 0.90f;
  }
  mGrid.SetGridDimensions({ (mColumns * 5) + 2, mRows + 3 });

  mGrid.SetRowHighlight(true, 2, mRows + 1);
  mGrid.SetColumnHighlight(false, 1, mColumns * 5);

  mTitle = std::make_shared<TextComponent>(window, _("PAD TO KEYBOARD CONTROLS"), menuTheme.Title().font, menuTheme.Title().color, ::Alignment::Center);
  mGrid.setEntry(mTitle, { 1, 1 }, false, false, { mColumns * 5, 1});

  // Fill in the grid
  int currentRow = 0;
  int currentCol = 0;
  String defaultComment = _("No comment available");
  float margin = Renderer::Instance().DisplayHeightAsFloat() * 0.005f;
  for(const auto& hint : mapping.HintList())
  {
    int padNumber = hint.first.first;
    PadItems item = hint.first.second;
    const String* comment = hint.second;
    if (comment == nullptr || comment->empty())
      comment = &defaultComment;

    std::shared_ptr<TextComponent> padIndex = std::make_shared<TextComponent>(window, IconTextMap(item, padNumber), font, menuTheme.Text().color, ::Alignment::CenterRight);
    mGrid.setEntry(padIndex, { (currentCol * 5) + 1, currentRow + 2 }, false, true, { 1, 1});

    Path iconPath(IconPathMap(item));

    std::shared_ptr<PictureComponent> icon = std::make_shared<PictureComponent>(window, iconPath, ImgProps::NoCache | ImgProps::KeepRatio, margin, margin);
    //icon->setResize(0, height);
    icon->setColor(menuTheme.Text().color);
    mGrid.setEntry(icon, { (currentCol * 5) + 3, currentRow + 2}, false, true, { 1, 1});

    std::shared_ptr<TextScrollComponent> text = std::make_shared<TextScrollComponent>(window, *comment, menuTheme.Text().font, menuTheme.Text().color, ::Alignment::CenterLeft);
    mGrid.setEntry(text, { (currentCol * 5) + 5, currentRow + 2}, false, true, { 1, 1});

    if (++currentRow >= mRows)
    {
      currentRow = 0;
      currentCol++;
    }
  }

  // Set Window position/size
  float titleHeight = (float)mTitle->getFont()->Height() * 2.6f;
  setSize(windowWidth, ((float)mRows * ((float)font->Height() * 2.25f)) + titleHeight + (Renderer::Instance().DisplayHeightAsFloat() * 0.04f));
  setPosition((Renderer::Instance().DisplayWidthAsFloat() - mSize.x()) / 2, (Renderer::Instance().DisplayHeightAsFloat() - mSize.y()) / 2);
}

void GuiControlHints::onSizeChanged()
{
  // Title height
  float titlePercent = ((float)mTitle->getFont()->Height() * 2.6f) / mSize.y();

  // Set rows
  mGrid.setRowHeightPerc(0, 0.02, false);
  mGrid.setRowHeightPerc(1, titlePercent, false);
  mGrid.setRowHeightPerc(mRows + 2, 0.02, false);
  for(int i = mRows; --i >= 0; )
    mGrid.setRowHeightPerc(i + 2, 0, false);

  // icon/pad number width
  NewFont* font = mWindow.HelpBar().TextFont();
  const float iconPercent = Math::round((float)font->Height() * 2.0f) / mSize.x();

  //  Set columns
  mGrid.setColWidthPerc(0, 0.02, false);
  mGrid.setColWidthPerc((mColumns * 5) + 1, 0.02, false);
  for(int i = mColumns; --i >= 0; )
  {
    mGrid.setColWidthPerc((i * 5) + 1, iconPercent * 3, false);
    mGrid.setColWidthPerc((i * 5) + 2, 0.02, false);
    mGrid.setColWidthPerc((i * 5) + 3, iconPercent, false);
    mGrid.setColWidthPerc((i * 5) + 4, 0.02, false);
    mGrid.setColWidthPerc((i * 5) + 5, 0, false);
  }

  mGrid.setSize(mSize);
}

bool GuiControlHints::ProcessInput(const InputCompactEvent& event)
{
  if (event.CancelReleased() || event.R1Released())
  {
    Close();
    return true;
  }

  return Gui::ProcessInput(event);
}

bool GuiControlHints::CollectHelpItems(Help& help)
{
  help.Clear().Set(Help::Cancel(), _("CLOSE"));
  return true;
}
