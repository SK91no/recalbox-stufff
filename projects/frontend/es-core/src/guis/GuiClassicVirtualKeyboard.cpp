//
// Classic on-screen virtual keyboard implementation.
//
#include "GuiClassicVirtualKeyboard.h"

#include <Renderer.h>
#include <RecalboxConf.h>
#include <help/Help.h>
#include <utils/locale/LocaleHelper.h>
#include <rendering/fonts/FontManager.h>
#include <input/InputCompactEvent.h>

GuiClassicVirtualKeyboard::GuiClassicVirtualKeyboard(WindowManager& window, const String& title,
                                                     const String& initValue,
                                                     IVirtualKeyboardInterface* okCallback,
                                                     VKCharFilter filter,
                                                     const String& view)
  : Gui(window)
  , mCursor(0)
  , mOffsetInPixel(0)
  , mLayout(&VKLayouts::Get(RecalboxConf::Instance().GetSystemKbLayout()))
  , mSelRow(2)
  , mSelCol(0)
  , mDesiredCenterX(-1.0f)
  , mShift(ModState::Off)
  , mCtrl(ModState::Off)
  , mAlt(ModState::Off)
  , mAltGr(ModState::Off)
  , mShiftHoldMs(-1)
  , mCtrlHoldMs(-1)
  , mAltHoldMs(-1)
  , mAltGrHoldMs(-1)
  , mShiftLockedDuringHold(false)
  , mCtrlLockedDuringHold(false)
  , mAltLockedDuringHold(false)
  , mAltGrLockedDuringHold(false)
  , mScrollX(0)
  , mTextCursorDirection(Direction::None)
  , mTextCursorRepeatTimerMs(0)
  , mNavRepeatDRow(0)
  , mNavRepeatDCol(0)
  , mNavRepeatTimerMs(0)
  , mValidHeld(false)
  , mShown(false)
  , mValidateCallback(okCallback)
  , mFilter(filter)
  , mKeyFont(nullptr)
  , mTitleFont(nullptr)
  , mTextFont(nullptr)
  , mTitle(title)
  , mKeyboardTop(0)
  , mKeyboardHeight(0)
  , mUnitWidth(0)
  , mKeyHeight(0)
  , mKeyboardNaturalWidth(0)
  , mKeyboardLeftWhenFits(0)
  , mKeyboardOverflows(false)
  , mInnerEditor{0,0,0,0}
  , mOuterEditor{0,0,0,0}
{
  mView = view;
  // Resolve themed colors + fonts for the view this keyboard is shown over.
  mColors.LoadFromView(view);
  LoadFonts();

  // Seed the edited text + cursor position.
  mText = initValue.SplitUnicodes();
  mCursor = (int)mText.size();

  RecomputeLayout();
  ClampSelection();
  AdjustCursor();
  EnsureSelectedKeyVisible();

  SDL_StartTextInput();
}

float GuiClassicVirtualKeyboard::GetFontSize(float ratio)
{
  float minDim = Math::min(Renderer::Instance().DisplayHeightAsFloat(), Renderer::Instance().DisplayWidthAsFloat());
  return Math::roundi(ratio * minDim);
}

void GuiClassicVirtualKeyboard::LoadFonts()
{
  // On low-res screens (<= 480p) every font uses the crisp CRT bitmap font;
  // higher resolutions use the theme font face if provided, else Ubuntu Condensed.
  const bool lowRes = Renderer::Instance().Is480pOrLower();
  const Path fontPath = lowRes ? Path(FONT_PATH_CRT)
                      : (mColors.HasFont ? mColors.FontPath : Path(":/ubuntu_condensed.ttf"));

  // Key font keeps a fixed pixel size on CRT (7px in 240p, doubled in 480p).
  float keyPx = IsLowRes() ? (float)sLowResKeyFontPx
              : (lowRes ? (float)(sLowResKeyFontPx * 2) : GetFontSize(sKeyFontRatio));
  mKeyFont   = &FontManager::Instance().FromFile(fontPath, keyPx, true);
  mTitleFont = &FontManager::Instance().FromFile(fontPath, GetFontSize(sTitleFontRatio), true);
  mTextFont  = &FontManager::Instance().FromFile(fontPath, GetFontSize(sTextFontRatio), true);
}

void GuiClassicVirtualKeyboard::SwitchToTheme(const ThemeData& theme, bool, IThemeSwitchTick*)
{
  // Live theme change: re-resolve from scratch so removed properties fall back to
  // the built-in defaults, reload fonts and re-layout.
  mColors = VirtualKeyboardTheme();
  mColors.LoadFromTheme(theme, mView);
  LoadFonts();
  RecomputeLayout();
  ClampSelection();
  AdjustCursor();
  EnsureSelectedKeyVisible();
}

void GuiClassicVirtualKeyboard::RecomputeLayout()
{
  const float screenW = Renderer::Instance().DisplayWidthAsFloat();
  const float screenH = Renderer::Instance().DisplayHeightAsFloat();

  // Vertical layout, top to bottom: keyboard, then the edit field, then the help
  // bar (rendered underneath, see MustRenderOverHelpSystem). We keep the bottom
  // band free so the help bar stays clearly visible.
  const float topPad      = screenH * 0.03f;
  const float gap         = screenH * 0.015f;
  const float helpBarBand = screenH * sHelpBarReservedRatio;

  const float textFontH = (float)mTextFont->Height();
  const float titleH    = (float)mTitleFont->Height();
  const float pad       = textFontH * 0.25f;
  const float editorW   = screenW * 0.9f;

  const float outerEditorH = textFontH + (pad * 2.0f);
  // The title is drawn above the edit box (see RenderTextBox).
  const float editorBlockH = (titleH * 1.2f) + outerEditorH;

  // Vertical budget left for the keyboard once the edit field + help bar are
  // reserved below it.
  const float belowKeyboard = gap + editorBlockH + gap + helpBarBand;
  const float vBudget       = screenH - topPad - belowKeyboard;

  const float maxUnits = (float)mLayout->unitsPerRowX100 * 0.01f;

  // Square keys, sized to fit the screen width, but never taller than the budget.
  mKeyHeight      = Math::min(screenW / maxUnits, vBudget / (float)mLayout->rowCount);
  mUnitWidth      = mKeyHeight;
  mKeyboardTop    = topPad;
  mKeyboardHeight = mKeyHeight * (float)mLayout->rowCount;
  mKeyboardNaturalWidth = maxUnits * mUnitWidth;

  if (mKeyboardNaturalWidth <= screenW)
  {
    // It fits: center horizontally, no scroll.
    mKeyboardOverflows = false;
    mKeyboardLeftWhenFits = (screenW - mKeyboardNaturalWidth) * 0.5f;
    mScrollX = 0;
  }
  else
  {
    mKeyboardOverflows = true;
    mKeyboardLeftWhenFits = 0;
  }

  // Edit field, placed directly below the keyboard (title above the box).
  const float kbBottom = mKeyboardTop + mKeyboardHeight;
  mInnerEditor.w = editorW;
  mInnerEditor.h = textFontH;
  mInnerEditor.x = (screenW - editorW) * 0.5f;
  mOuterEditor.w = mInnerEditor.w + (pad * 2.0f);
  mOuterEditor.h = outerEditorH;
  mOuterEditor.x = mInnerEditor.x - pad;
  mOuterEditor.y = kbBottom + gap + (titleH * 1.2f);
  mInnerEditor.y = mOuterEditor.y + pad;
}

bool GuiClassicVirtualKeyboard::IsCharAllowed(unsigned int unicode) const
{
  switch (mFilter)
  {
    case VKCharFilter::All:
      return true;
    case VKCharFilter::Alphanumeric:
      return (unicode >= (unsigned int)'0' && unicode <= (unsigned int)'9')
          || (unicode >= (unsigned int)'a' && unicode <= (unsigned int)'z')
          || (unicode >= (unsigned int)'A' && unicode <= (unsigned int)'Z');
    case VKCharFilter::Numeric:
      return (unicode >= (unsigned int)'0' && unicode <= (unsigned int)'9');
    case VKCharFilter::FullNumeric:
      return (unicode >= (unsigned int)'0' && unicode <= (unsigned int)'9')
          || unicode == (unsigned int)'-'
          || unicode == (unsigned int)'.';
  }
  return true;
}

unsigned int GuiClassicVirtualKeyboard::ResolveChar(const VKKey& key) const
{
  if (key.kind != VKKeyKind::Char) return 0;
  if (AltGrActive() && key.altgr != 0) return key.altgr;
  if (ShiftActive() && key.shifted != 0) return key.shifted;
  return key.normal;
}

unsigned int GuiClassicVirtualKeyboard::GetKeyDisplayChar(const VKKey& key) const
{
  // Show the character that would be emitted with the *current* modifier state.
  return ResolveChar(key);
}

void GuiClassicVirtualKeyboard::ClampSelection()
{
  if (mSelRow < 0) mSelRow = 0;
  if (mSelRow >= mLayout->rowCount) mSelRow = mLayout->rowCount - 1;
  const VKRow& row = mLayout->rows[mSelRow];
  if (mSelCol < 0) mSelCol = 0;
  if (mSelCol >= row.count) mSelCol = row.count - 1;
}

GuiClassicVirtualKeyboard::Rect GuiClassicVirtualKeyboard::KeyRectangle(int row, int col) const
{
  // Compute X position of the requested key inside its row, accounting for
  // each preceding key's variable width.
  const VKRow& r = mLayout->rows[row];
  float unitsBefore = 0.f;
  for (int i = 0; i < col && i < r.count; ++i)
    unitsBefore += (float)r.keys[i].widthx100 * 0.01f;

  const float rowUnits = [&]() {
    float s = 0.f;
    for (int i = 0; i < r.count; ++i) s += (float)r.keys[i].widthx100 * 0.01f;
    return s;
  }();
  // Row offset: center each row inside the natural keyboard width.
  const float rowNaturalW = rowUnits * mUnitWidth;
  const float rowLeftInside = (mKeyboardNaturalWidth - rowNaturalW) * 0.5f;
  const float baseLeft = mKeyboardOverflows ? 0.f : mKeyboardLeftWhenFits;

  Rect rect;
  rect.x = baseLeft + rowLeftInside + (unitsBefore * mUnitWidth) - mScrollX;
  rect.y = mKeyboardTop + ((float)row * mKeyHeight);
  rect.w = (float)r.keys[col].widthx100 * 0.01f * mUnitWidth;
  rect.h = mKeyHeight;
  return rect;
}

void GuiClassicVirtualKeyboard::EnsureSelectedKeyVisible()
{
  if (!mKeyboardOverflows) return;
  const float screenW = Renderer::Instance().DisplayWidthAsFloat();

  // We don't subtract mScrollX here: we want the natural offset.
  // Recompute as KeyRectangle does but without scroll, then aim to center.
  const VKRow& r = mLayout->rows[mSelRow];
  float unitsBefore = 0.f;
  for (int i = 0; i < mSelCol && i < r.count; ++i)
    unitsBefore += (float)r.keys[i].widthx100 * 0.01f;
  const float rowUnits = [&]() {
    float s = 0.f; for (int i = 0; i < r.count; ++i) s += (float)r.keys[i].widthx100 * 0.01f;
    return s;
  }();
  const float rowLeftInside = (mKeyboardNaturalWidth - rowUnits * mUnitWidth) * 0.5f;
  const float keyLeft  = rowLeftInside + unitsBefore * mUnitWidth;
  const float keyW     = (float)r.keys[mSelCol].widthx100 * 0.01f * mUnitWidth;
  const float keyCenter = keyLeft + keyW * 0.5f;

  // Target scroll: keep the selected key centered on screen, clamped to bounds.
  float target = keyCenter - (screenW * 0.5f);
  const float maxScroll = mKeyboardNaturalWidth - screenW;
  if (target < 0) target = 0;
  if (target > maxScroll) target = maxScroll;
  mScrollX = target;
}

void GuiClassicVirtualKeyboard::MoveGridSelection(int dRow, int dCol)
{
  if (mValidHeld) return; // user is committing the current key; don't move

  // Natural keyboard X of a key (scroll/offset independent) — used to match the
  // horizontal position across rows that have variable-width keys.
  const float baseLeft = mKeyboardOverflows ? 0.0f : mKeyboardLeftWhenFits;
  auto naturalLeft = [&](const Rect& r) { return r.x + mScrollX - baseLeft; };

  // Vertical move: wrap around (top <-> bottom), keeping the horizontal position
  // so we land on the key visually above/below. A sticky anchor (mDesiredCenterX)
  // is captured on the first vertical move and kept until a horizontal move, so
  // up/down keep returning to the same column even across wide keys (Space).
  if (dRow != 0)
  {
    if (mDesiredCenterX < 0.0f)
    {
      const Rect cur = KeyRectangle(mSelRow, mSelCol);
      mDesiredCenterX = naturalLeft(cur) + (cur.w * 0.5f);
    }
    const float targetX = mDesiredCenterX;

    mSelRow += dRow;
    if (mSelRow < 0) mSelRow = mLayout->rowCount - 1;
    else if (mSelRow >= mLayout->rowCount) mSelRow = 0;

    // Pick the key in the new row whose horizontal span contains targetX
    // (or, failing that, the one whose center is the closest).
    const VKRow& row = mLayout->rows[mSelRow];
    int best = 0;
    float bestDist = -1.0f;
    for (int i = 0; i < row.count; ++i)
    {
      const Rect r = KeyRectangle(mSelRow, i);
      const float left = naturalLeft(r);
      if (targetX >= left && targetX < left + r.w) { best = i; break; }
      float d = targetX - (left + (r.w * 0.5f));
      if (d < 0) d = -d;
      if (bestDist < 0.0f || d < bestDist) { bestDist = d; best = i; }
    }
    mSelCol = best;
  }

  // Horizontal move: wrap around (left <-> right) inside the current row. This is
  // a deliberate column change, so it drops the sticky vertical anchor.
  if (dCol != 0)
  {
    const int rowCount = mLayout->rows[mSelRow].count;
    mSelCol += dCol;
    if (mSelCol < 0) mSelCol = rowCount - 1;
    else if (mSelCol >= rowCount) mSelCol = 0;
    mDesiredCenterX = -1.0f;
  }

  EnsureSelectedKeyVisible();
}

void GuiClassicVirtualKeyboard::StartGridRepeat(int dRow, int dCol)
{
  mNavRepeatDRow = dRow;
  mNavRepeatDCol = dCol;
  mNavRepeatTimerMs = sGridRepeatStartMs;
  MoveGridSelection(dRow, dCol); // first move occurs immediately
}

void GuiClassicVirtualKeyboard::StopGridRepeat()
{
  mNavRepeatDRow = 0;
  mNavRepeatDCol = 0;
}

void GuiClassicVirtualKeyboard::PressSelectedKey()
{
  const VKKey& key = mLayout->rows[mSelRow].keys[mSelCol];
  switch (key.kind)
  {
    case VKKeyKind::Char:
    {
      unsigned int c = ResolveChar(key);
      AddCharacter(c);
      ConsumeOnceModifiers();
      break;
    }
    case VKKeyKind::Space:     AddCharacter((unsigned int)' '); ConsumeOnceModifiers(); break;
    case VKKeyKind::Backspace: Backspace(); break;
    case VKKeyKind::Enter:     ValidateEditor(); break;
    case VKKeyKind::Left:
      if (mCursor > 0) { mCursor--; AdjustCursor(); ResetCursorBlink(); }
      break;
    case VKKeyKind::Right:
      if (mCursor < (int)mText.size()) { mCursor++; AdjustCursor(); ResetCursorBlink(); }
      break;
    // Modifiers are handled by the press/release logic, not here.
    case VKKeyKind::Shift:
    case VKKeyKind::Ctrl:
    case VKKeyKind::Alt:
    case VKKeyKind::AltGr:
      break;
  }
}

void GuiClassicVirtualKeyboard::AddCharacter(unsigned int unicode)
{
  if (unicode == 0) return;
  if (!IsCharAllowed(unicode)) return;

  if (mCursor < (int)mText.size())
    mText.insert(mText.begin() + mCursor, unicode);
  else
    mText.push_back(unicode);

  mCursor++;
  AdjustCursor();
  ResetCursorBlink();

  if (mValidateCallback != nullptr)
    mValidateCallback->VirtualKeyboardTextChange(*this, String::JoinUnicodes(mText));
}

void GuiClassicVirtualKeyboard::Backspace()
{
  if (mCursor > 0)
  {
    mCursor--;
    mText.erase(mText.begin() + mCursor);
    AdjustCursor(-((int)mInnerEditor.w / 4));
    ResetCursorBlink();
    if (mValidateCallback != nullptr)
      mValidateCallback->VirtualKeyboardTextChange(*this, String::JoinUnicodes(mText));
  }
}

void GuiClassicVirtualKeyboard::ValidateEditor()
{
  if (mValidateCallback != nullptr)
    mValidateCallback->VirtualKeyboardValidated(*this, String::JoinUnicodes(mText));
  Close();
  if (mValidateCallback != nullptr)
    mValidateCallback->VirtualKeyboardHide(*this);
}

void GuiClassicVirtualKeyboard::ConsumeOnceModifiers()
{
  if (mShift == ModState::Once) mShift = ModState::Off;
  if (mCtrl  == ModState::Once) mCtrl  = ModState::Off;
  if (mAlt   == ModState::Once) mAlt   = ModState::Off;
  if (mAltGr == ModState::Once) mAltGr = ModState::Off;
}

void GuiClassicVirtualKeyboard::ModifierShortTap(ModState& state)
{
  // Cycle: Locked → Off, Once → Off, Off → Once
  if (state == ModState::Locked) state = ModState::Off;
  else if (state == ModState::Once) state = ModState::Off;
  else state = ModState::Once;
}

void GuiClassicVirtualKeyboard::TickModifier(ModState& state, int& holdMs, bool& lockedDuringHold,
                                             bool pressed, int deltaTime)
{
  if (pressed)
  {
    if (holdMs < 0) // just pressed
    {
      holdMs = 0;
      lockedDuringHold = false;
    }
    else
    {
      holdMs += deltaTime;
      if (!lockedDuringHold && holdMs >= sModifierLockHoldMs)
      {
        state = ModState::Locked;
        lockedDuringHold = true;
      }
    }
  }
  else
  {
    if (holdMs >= 0) // just released
    {
      if (!lockedDuringHold) ModifierShortTap(state);
      holdMs = -1;
      lockedDuringHold = false;
    }
  }
}

void GuiClassicVirtualKeyboard::AdjustCursor(int cursorextend)
{
  unsigned int unicode = (mCursor < (int)mText.size()) ? mText[mCursor] : 32;
  float cursorWidth = (float)mTextFont->CharWidth(unicode);

  float cursorLeft = 0;
  for (int i = 0; i < mCursor; i++)
    cursorLeft += (float)mTextFont->CharWidth(mText[i]);
  float cursorRight = cursorLeft + cursorWidth;

  if (cursorextend < 0) cursorLeft  += (float)cursorextend;
  if (cursorextend > 0) cursorRight += (float)cursorextend;

  if (cursorRight > mOffsetInPixel + mInnerEditor.w)
    mOffsetInPixel = cursorRight - mInnerEditor.w;
  else if (cursorLeft < mOffsetInPixel)
    mOffsetInPixel = cursorLeft;
  if (mOffsetInPixel < 0) mOffsetInPixel = 0;
}

void GuiClassicVirtualKeyboard::ResetCursorBlink()
{
  mCursorBlinkReference = DateTime();
}

void GuiClassicVirtualKeyboard::textInput(const char* text)
{
  for (unsigned int unicode : String(text).SplitUnicodes())
    AddCharacter(unicode);
  AdjustCursor();
}

bool GuiClassicVirtualKeyboard::ProcessInput(const InputCompactEvent& event)
{
  // ----- Physical keyboard handling (same shortcuts as the wheel VK) -----
  if (event.IsKeyboard())
  {
    bool released = event.RawEvent().Value() == 0;
    switch (event.RawEvent().Id())
    {
      case SDLK_BACKSPACE: if (released) Backspace(); break;
      case SDLK_DELETE:
        if (released && mCursor < (int)mText.size())
        {
          mText.erase(mText.begin() + mCursor);
          AdjustCursor((int)mInnerEditor.w / 4);
          ResetCursorBlink();
          if (mValidateCallback != nullptr)
            mValidateCallback->VirtualKeyboardTextChange(*this, String::JoinUnicodes(mText));
        }
        break;
      case SDLK_ESCAPE:
      case SDLK_RETURN:
      case SDLK_RETURN2: if (released) ValidateEditor(); break;
      case SDLK_LEFT:
        if (!released && mCursor > 0) { mCursor--; AdjustCursor(); ResetCursorBlink(); }
        break;
      case SDLK_RIGHT:
        if (!released && mCursor < (int)mText.size()) { mCursor++; AdjustCursor(); ResetCursorBlink(); }
        break;
      default: break;
    }
    return true;
  }

  // ----- Modifier press/release ticking (gamepad analogue: handled via labels)
  // Modifiers are activated by pressing Valid on a modifier key. We use the
  // Valid press/release timing while the selection is on that modifier key.

  // Grid navigation (D-pad) with auto-repeat. Locked while Valid is held.
  if (!mValidHeld)
  {
    if (event.UpPressed())    { StartGridRepeat(-1, 0); return true; }
    if (event.DownPressed())  { StartGridRepeat( 1, 0); return true; }
    if (event.LeftPressed())  { StartGridRepeat( 0,-1); return true; }
    if (event.RightPressed()) { StartGridRepeat( 0, 1); return true; }
  }
  // Stop the auto-repeat when the held direction is released.
  if (event.UpReleased()    && mNavRepeatDRow < 0) { StopGridRepeat(); return true; }
  if (event.DownReleased()  && mNavRepeatDRow > 0) { StopGridRepeat(); return true; }
  if (event.LeftReleased()  && mNavRepeatDCol < 0) { StopGridRepeat(); return true; }
  if (event.RightReleased() && mNavRepeatDCol > 0) { StopGridRepeat(); return true; }

  // Validate (Cancel button) / Start → close with current text
  if (event.CancelReleased() || event.StartReleased())
  {
    ValidateEditor();
    return true;
  }

  // X = Delete char at cursor, Y = Backspace
  if (event.NorthPressed())
  {
    if (mCursor < (int)mText.size())
    {
      mText.erase(mText.begin() + mCursor);
      AdjustCursor((int)mInnerEditor.w / 4);
      ResetCursorBlink();
      if (mValidateCallback != nullptr)
        mValidateCallback->VirtualKeyboardTextChange(*this, String::JoinUnicodes(mText));
    }
    return true;
  }
  if (event.WestPressed()) { Backspace(); return true; }

  // L1/R1 = lock/unlock Shift / AltGr for quick access to shifted chars & symbols.
  if (event.L1Pressed()) { mShift = (mShift == ModState::Locked) ? ModState::Off : ModState::Locked; return true; }
  if (event.R1Pressed()) { mAltGr = (mAltGr == ModState::Locked) ? ModState::Off : ModState::Locked; return true; }

  // L2/R2 → move text cursor (with auto-repeat in Update)
  if (event.L2Pressed())       { mTextCursorDirection = Direction::Left;  mTextCursorRepeatTimerMs = sCursorRepeatStartMs;
                                 if (mCursor > 0) { mCursor--; AdjustCursor(); ResetCursorBlink(); } return true; }
  if (event.L2Released())      { if (mTextCursorDirection == Direction::Left)  mTextCursorDirection = Direction::None; return true; }
  if (event.R2Pressed())       { mTextCursorDirection = Direction::Right; mTextCursorRepeatTimerMs = sCursorRepeatStartMs;
                                 if (mCursor < (int)mText.size()) { mCursor++; AdjustCursor(); ResetCursorBlink(); } return true; }
  if (event.R2Released())      { if (mTextCursorDirection == Direction::Right) mTextCursorDirection = Direction::None; return true; }

  // Valid (A): press/hold/release the selected key. While held, no nav happens.
  if (event.ValidPressed())
  {
    mValidHeld = true;
    // For modifier keys, start the hold timer immediately.
    const VKKey& key = mLayout->rows[mSelRow].keys[mSelCol];
    switch (key.kind)
    {
      case VKKeyKind::Shift: mShiftHoldMs = 0; mShiftLockedDuringHold = false; break;
      case VKKeyKind::Ctrl : mCtrlHoldMs  = 0; mCtrlLockedDuringHold  = false; break;
      case VKKeyKind::Alt  : mAltHoldMs   = 0; mAltLockedDuringHold   = false; break;
      case VKKeyKind::AltGr: mAltGrHoldMs = 0; mAltGrLockedDuringHold = false; break;
      case VKKeyKind::Char:
      case VKKeyKind::Backspace:
      case VKKeyKind::Enter:
      case VKKeyKind::Space:
      case VKKeyKind::Left:
      case VKKeyKind::Right:
      default: break;
    }
    return true;
  }
  if (event.ValidReleased())
  {
    mValidHeld = false;
    const VKKey& key = mLayout->rows[mSelRow].keys[mSelCol];
    switch (key.kind)
    {
      case VKKeyKind::Shift:
        TickModifier(mShift, mShiftHoldMs, mShiftLockedDuringHold, false, 0);
        break;
      case VKKeyKind::Ctrl:
        TickModifier(mCtrl, mCtrlHoldMs, mCtrlLockedDuringHold, false, 0);
        break;
      case VKKeyKind::Alt:
        TickModifier(mAlt, mAltHoldMs, mAltLockedDuringHold, false, 0);
        break;
      case VKKeyKind::AltGr:
        TickModifier(mAltGr, mAltGrHoldMs, mAltGrLockedDuringHold, false, 0);
        break;
      case VKKeyKind::Char:
      case VKKeyKind::Backspace:
      case VKKeyKind::Enter:
      case VKKeyKind::Space:
      case VKKeyKind::Left:
      case VKKeyKind::Right:
      default:
        PressSelectedKey();
        break;
    }
    return true;
  }

  return true;
}

void GuiClassicVirtualKeyboard::Update(int deltaTime)
{
  // Tick the hold timer of the modifier the user is currently pressing (Valid held + modifier selected).
  if (mValidHeld)
  {
    const VKKey& key = mLayout->rows[mSelRow].keys[mSelCol];
    switch (key.kind)
    {
      case VKKeyKind::Shift:
        TickModifier(mShift, mShiftHoldMs, mShiftLockedDuringHold, true, deltaTime);
        break;
      case VKKeyKind::Ctrl:
        TickModifier(mCtrl, mCtrlHoldMs, mCtrlLockedDuringHold, true, deltaTime);
        break;
      case VKKeyKind::Alt:
        TickModifier(mAlt, mAltHoldMs, mAltLockedDuringHold, true, deltaTime);
        break;
      case VKKeyKind::AltGr:
        TickModifier(mAltGr, mAltGrHoldMs, mAltGrLockedDuringHold, true, deltaTime);
        break;
      case VKKeyKind::Char:
      case VKKeyKind::Backspace:
      case VKKeyKind::Enter:
      case VKKeyKind::Space:
      case VKKeyKind::Left:
      case VKKeyKind::Right:
      default: break;
    }
  }

  // Auto-repeat for text cursor while L1/R1 is held.
  if (mTextCursorDirection != Direction::None)
  {
    mTextCursorRepeatTimerMs -= deltaTime;
    while (mTextCursorRepeatTimerMs <= 0)
    {
      mTextCursorRepeatTimerMs += sCursorRepeatTickMs;
      if (mTextCursorDirection == Direction::Left  && mCursor > 0) { mCursor--; AdjustCursor(); ResetCursorBlink(); }
      else if (mTextCursorDirection == Direction::Right && mCursor < (int)mText.size()) { mCursor++; AdjustCursor(); ResetCursorBlink(); }
      else break;
    }
  }

  // Auto-repeat for grid selection while a D-pad direction is held.
  if (mNavRepeatDRow != 0 || mNavRepeatDCol != 0)
  {
    mNavRepeatTimerMs -= deltaTime;
    while (mNavRepeatTimerMs <= 0)
    {
      mNavRepeatTimerMs += sGridRepeatTickMs;
      MoveGridSelection(mNavRepeatDRow, mNavRepeatDCol);
    }
  }
}

unsigned int GuiClassicVirtualKeyboard::KeyBackgroundColor(const VKKey& key, ModState modState, bool selected) const
{
  (void)key;
  if (selected) return mColors.KeySelectedColor;
  switch (modState)
  {
    case ModState::Once:   return mColors.KeyModifierColor;
    case ModState::Locked: return mColors.KeyModifierLockedColor;
    case ModState::Off:    default: break;
  }
  return mColors.KeyColor;
}

void GuiClassicVirtualKeyboard::RenderKey(const VKKey& key, const Rect& rect, bool selected)
{
  ModState modState = ModState::Off;
  switch (key.kind)
  {
    case VKKeyKind::Shift: modState = mShift; break;
    case VKKeyKind::Ctrl:  modState = mCtrl;  break;
    case VKKeyKind::Alt:   modState = mAlt;   break;
    case VKKeyKind::AltGr: modState = mAltGr; break;
    case VKKeyKind::Char:
    case VKKeyKind::Backspace:
    case VKKeyKind::Enter:
    case VKKeyKind::Space:
    case VKKeyKind::Left:
    case VKKeyKind::Right:
    default: break;
  }

  // Background (inset a few pixels for visual separation)
  const float inset = mKeyHeight * 0.06f;
  unsigned int bg = KeyBackgroundColor(key, modState, selected);
  Renderer::DrawRectangle(rect.x + inset, rect.y + inset, rect.w - 2 * inset, rect.h - 2 * inset, bg);

  // Draw the glyph/label centered. On low-res the CRT pixel font must be snapped
  // to integer pixels on a shared baseline, otherwise fractional sampling drops
  // pixels from the glyphs. Higher resolutions keep smooth box-centering.
  const bool isChar = (key.kind == VKKeyKind::Char || key.kind == VKKeyKind::Space);
  unsigned int c = (key.kind == VKKeyKind::Space) ? (unsigned int)' '
                 : (isChar ? GetKeyDisplayChar(key) : 0u);
  unsigned int color = (isChar && !IsCharAllowed(c)) ? mColors.KeyDisabledColor : mColors.KeyTextColor;
  String lbl(key.label != nullptr ? key.label : "");

  if (Renderer::Instance().Is480pOrLower())
  {
    const int top = Math::roundi(rect.y + (rect.h - (float)mKeyFont->Height()) * 0.5f);
    const int baseline = top + mKeyFont->TopToBaseline();
    if (isChar)
    {
      const int x = Math::roundi(rect.x + (rect.w - (float)mKeyFont->CharWidth(c)) * 0.5f);
      mKeyFont->DrawCharBaseline(c, x, baseline, color);
    }
    else
    {
      const int x = Math::roundi(rect.x + (rect.w - (float)mKeyFont->TextWidth(lbl)) * 0.5f);
      mKeyFont->DrawText(lbl, x, top, color, ::Alignment::TopLeft);
    }
  }
  else
  {
    const int cx = (int)(rect.x + rect.w * 0.5f);
    const int cy = (int)(rect.y + rect.h * 0.5f);
    if (isChar) mKeyFont->DrawChar(c, cx, cy, color, ::Alignment::Center);
    else        mKeyFont->DrawText(lbl, cx, cy, color, ::Alignment::Center);
  }
}

void GuiClassicVirtualKeyboard::RenderKeyboard()
{
  for (int r = 0; r < mLayout->rowCount; ++r)
  {
    const VKRow& row = mLayout->rows[r];
    for (int c = 0; c < row.count; ++c)
    {
      Rect rect = KeyRectangle(r, c);
      // Off-screen culling (only relevant when scrolling)
      if (rect.x + rect.w < 0) continue;
      if (rect.x > Renderer::Instance().DisplayWidthAsFloat()) continue;
      RenderKey(row.keys[c], rect, (r == mSelRow) && (c == mSelCol));
    }
  }
}

void GuiClassicVirtualKeyboard::RenderEditedString()
{
  float baseline = (float)mTextFont->TopToBaseline();
  float offsetX = mInnerEditor.x - mOffsetInPixel;
  float offsetY = mInnerEditor.y + baseline;

  int cursorLight = ((int)(DateTime() - mCursorBlinkReference).TotalMilliseconds() % 400) - 200;
  if (cursorLight < 0) cursorLight = -cursorLight;
  unsigned int cursorColor = (cursorLight << 24) | (cursorLight << 16) | (cursorLight << 8) | 0xFFu;

  for (int i = 0; i < (int)mText.size(); i++)
  {
    unsigned int unicode = mText[i];
    float width = (float)mTextFont->CharWidth(unicode);
    if (i == mCursor)
      Renderer::DrawRectangle(offsetX, mInnerEditor.y, width, mInnerEditor.h, cursorColor);
    mTextFont->DrawCharBaseline(unicode, (int)offsetX, (int)offsetY, mColors.EditTextColor);
    offsetX += width;
  }
  if (mCursor >= (int)mText.size())
  {
    Renderer::DrawRectangle(offsetX, mInnerEditor.y, (float)mTextFont->CharWidth(32), mInnerEditor.h, cursorColor);
  }
}

void GuiClassicVirtualKeyboard::RenderTextBox()
{
  Renderer::DrawRectangle(mOuterEditor.x, mOuterEditor.y, mOuterEditor.w, mOuterEditor.h, 0x00000080u);

  mTitleFont->DrawText(mTitle,
                       (int)mInnerEditor.x,
                       (int)mOuterEditor.y - (int)((float)mTitleFont->Height() * 1.2f),
                       mColors.TitleColor, ::Alignment::TopLeft);

  Renderer::Instance().Clip(::Rectangle(mInnerEditor.x, mInnerEditor.y, mInnerEditor.w, mInnerEditor.h));
  RenderEditedString();
  Renderer::Instance().Unclip();
}

void GuiClassicVirtualKeyboard::Render(const Transform4x4f& parentTrans)
{
  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  // Dim background
  Renderer::DrawRectangle(0.f, 0.f,
                          Renderer::Instance().DisplayWidthAsFloat(),
                          Renderer::Instance().DisplayHeightAsFloat(),
                          0x00000080u);

  // Recompute the layout if the screen changed (cheap; mostly a no-op).
  if (mKeyboardHeight <= 0) RecomputeLayout();

  RenderKeyboard();
  RenderTextBox();

  if (!mShown)
  {
    if (mValidateCallback != nullptr)
      mValidateCallback->VirtualKeyboardShow(*this);
    mShown = true;
  }
}

bool GuiClassicVirtualKeyboard::CollectHelpItems(Help& help)
{
  help.Clear()
      .Set(Help::Valid(), _("PRESS KEY"))
      .Set(Help::Cancel(), _("CLOSE"))
      .Set(HelpType::X, _("DELETE"))
      .Set(HelpType::Y, _("BACKSPACE"))
      .Set(HelpType::LR, _("LOCK SHIFT/ALTGR"))
      .Set(HelpType::L2R2, _("MOVE CURSOR"))
      .Set(HelpType::AllDirections, _("MOVE SELECTION"));
  return true;
}
