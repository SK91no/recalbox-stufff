//
// Simplified on-screen virtual keyboard implementation.
//
#include "GuiSimplifiedVirtualKeyboard.h"

#include <Renderer.h>
#include <RecalboxConf.h>
#include <help/Help.h>
#include <utils/locale/LocaleHelper.h>
#include <rendering/fonts/FontManager.h>
#include <input/InputCompactEvent.h>

GuiSimplifiedVirtualKeyboard::GuiSimplifiedVirtualKeyboard(WindowManager& window, const String& title,
                                                           const String& initValue,
                                                           IVirtualKeyboardInterface* okCallback,
                                                           VKCharFilter filter,
                                                           const String& view)
  : Gui(window)
  , mCursor(0)
  , mOffsetInPixel(0)
  , mPage(0)
  , mSelRow(1)
  , mSelCol(0)
  , mDesiredCenterX(-1.0f)
  , mMaxRowCount(0)
  , mMaxUnitsX100(0)
  , mTextCursorDirection(Direction::None)
  , mTextCursorRepeatTimerMs(0)
  , mNavRepeatDRow(0)
  , mNavRepeatDCol(0)
  , mNavRepeatTimerMs(0)
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
  , mKeyboardLeft(0)
  , mInnerEditor{0,0,0,0}
  , mOuterEditor{0,0,0,0}
{
  mView = view;
  // Resolve themed colors + fonts for the view this keyboard is shown over.
  mColors.LoadFromView(view);
  LoadFonts();

  mText = initValue.SplitUnicodes();
  mCursor = (int)mText.size();

  BuildPages();
  ComputeSizingMetrics();

  RecomputeLayout();
  ClampSelection();
  AdjustCursor();

  SDL_StartTextInput();
}

float GuiSimplifiedVirtualKeyboard::GetFontSize(float ratio)
{
  float minDim = Math::min(Renderer::Instance().DisplayHeightAsFloat(), Renderer::Instance().DisplayWidthAsFloat());
  return Math::roundi(ratio * minDim);
}

void GuiSimplifiedVirtualKeyboard::LoadFonts()
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

void GuiSimplifiedVirtualKeyboard::SwitchToTheme(const ThemeData& theme, bool, IThemeSwitchTick*)
{
  // Live theme change: re-resolve from scratch (defaults for removed properties),
  // reload fonts and re-layout.
  mColors = VirtualKeyboardTheme();
  mColors.LoadFromTheme(theme, mView);
  LoadFonts();
  RecomputeLayout();
  ClampSelection();
  AdjustCursor();
}

// =====================================================================
//   Page building
// =====================================================================

GuiSimplifiedVirtualKeyboard::Row GuiSimplifiedVirtualKeyboard::MakeDigitRow()
{
  Row row;
  for (const char* d = "1234567890"; *d != 0; ++d)
    row.push_back({ KeyKind::Char, (unsigned int)*d, 1.0f, String() });
  return row;
}

void GuiSimplifiedVirtualKeyboard::AddFunctionRow(Page& page) const
{
  Row row;
  row.push_back({ KeyKind::Backspace, 0u,            1.5f, String("Bksp")  });
  row.push_back({ KeyKind::Space,     (unsigned)' ', 5.0f, String("Space") });
  row.push_back({ KeyKind::Enter,     0u,            1.5f, String("Enter") });
  row.push_back({ KeyKind::Left,      0u,            1.0f, String("<-")    });
  row.push_back({ KeyKind::Right,     0u,            1.0f, String("->")    });
  page.push_back(row);
}

void GuiSimplifiedVirtualKeyboard::BuildPages()
{
  const String kbCode = RecalboxConf::Instance().GetSystemKbLayout();
  const VKLayout& layout = VKLayouts::Get(kbCode);

  // --- Collect the a-z letter rows from the chosen layout, in its arrangement ---
  std::vector<std::vector<const VKKey*>> letterRows;
  for (int r = 0; r < layout.rowCount; ++r)
  {
    const VKRow& row = layout.rows[r];
    std::vector<const VKKey*> letters;
    for (int c = 0; c < row.count; ++c)
    {
      const VKKey& k = row.keys[c];
      if (k.kind == VKKeyKind::Char && k.normal >= (unsigned int)'a' && k.normal <= (unsigned int)'z')
        letters.push_back(&k);
    }
    if (!letters.empty()) letterRows.push_back(letters);
  }

  // --- Pages 0 (lowercase) and 1 (uppercase): digits + letters + function row ---
  for (int pageCase = 0; pageCase < 2; ++pageCase)
  {
    Page page;
    page.push_back(MakeDigitRow());
    for (const std::vector<const VKKey*>& lr : letterRows)
    {
      Row row;
      for (const VKKey* k : lr)
      {
        unsigned int cp = (pageCase == 0) ? k->normal
                                          : (k->shifted != 0 ? k->shifted : k->normal);
        row.push_back({ KeyKind::Char, cp, 1.0f, String() });
      }
      page.push_back(row);
    }
    AddFunctionRow(page);
    mPages.push_back(page);
  }

  // --- Ordered symbol list: universal common symbols, then layout extras ---
  std::vector<unsigned int> symbols;
  auto contains = [](const std::vector<unsigned int>& v, unsigned int x)
  {
    for (unsigned int e : v) if (e == x) return true;
    return false;
  };

  static const unsigned int kUniversal[] = {
    (unsigned)'.', (unsigned)',', (unsigned)'?', (unsigned)'!', (unsigned)'\'',
    (unsigned)'"', (unsigned)':', (unsigned)';', (unsigned)'-',
    (unsigned)'_', (unsigned)'/', (unsigned)'\\', (unsigned)'@', (unsigned)'#',
    (unsigned)'&', (unsigned)'*', (unsigned)'+', (unsigned)'=',
    (unsigned)'(', (unsigned)')', (unsigned)'[', (unsigned)']', (unsigned)'{',
    (unsigned)'}', (unsigned)'<', (unsigned)'>', (unsigned)'%',
    (unsigned)'$', 0x20ACu /*EUR*/, 0x00A3u /*£*/, (unsigned)'|', (unsigned)'~',
    (unsigned)'^', (unsigned)'`', 0x00B0u /*°*/, 0x00A7u /*§*/,
  };
  for (unsigned int s : kUniversal) if (!contains(symbols, s)) symbols.push_back(s);

  // Append every non-letter, non-digit character offered by the layout (accents,
  // currency, layout-specific punctuation) that is not already listed.
  auto consider = [&](unsigned int cp)
  {
    if (cp == 0u || cp == (unsigned int)' ') return;
    if (cp >= (unsigned int)'a' && cp <= (unsigned int)'z') return;
    if (cp >= (unsigned int)'A' && cp <= (unsigned int)'Z') return;
    if (cp >= (unsigned int)'0' && cp <= (unsigned int)'9') return;
    if (!contains(symbols, cp)) symbols.push_back(cp);
  };
  for (int r = 0; r < layout.rowCount; ++r)
  {
    const VKRow& row = layout.rows[r];
    for (int c = 0; c < row.count; ++c)
    {
      const VKKey& k = row.keys[c];
      if (k.kind != VKKeyKind::Char) continue;
      consider(k.normal);
      consider(k.shifted);
      consider(k.altgr);
    }
  }

  AppendSymbolPages(symbols);
}

void GuiSimplifiedVirtualKeyboard::AppendSymbolPages(const std::vector<unsigned int>& symbols)
{
  const int cols = sSymbolCols;
  const int N = (int)symbols.size();
  const int capNoDigit   = 4 * cols; // 4 symbol rows, no digit row
  const int capWithDigit = 3 * cols; // digit row + 3 symbol rows

  // One page (no digit row) if everything fits; otherwise several pages, each
  // keeping a digit row on top for comfort (as requested).
  const bool withDigit = (N > capNoDigit);
  const int symbolRows  = withDigit ? 3 : 4;
  const int perPage     = withDigit ? capWithDigit : capNoDigit;
  const int pageCount   = (N <= 0) ? 1 : ((N + perPage - 1) / perPage);

  int idx = 0;
  for (int p = 0; p < pageCount; ++p)
  {
    Page page;
    if (withDigit) page.push_back(MakeDigitRow());

    int remaining = Math::min(perPage, N - idx);
    // Spread the symbols evenly across the available rows to avoid empty rows.
    for (int r = 0; r < symbolRows; ++r)
    {
      int rowsLeft = symbolRows - r;
      int take = (remaining + rowsLeft - 1) / rowsLeft; // ceil
      Row row;
      for (int i = 0; i < take && idx < N; ++i)
        row.push_back({ KeyKind::Char, symbols[idx++], 1.0f, String() });
      remaining -= (int)row.size();
      if (!row.empty()) page.push_back(row);
    }
    AddFunctionRow(page);
    mPages.push_back(page);
  }
}

void GuiSimplifiedVirtualKeyboard::ComputeSizingMetrics()
{
  mMaxRowCount = 0;
  mMaxUnitsX100 = 0;
  for (const Page& p : mPages)
  {
    if ((int)p.size() > mMaxRowCount) mMaxRowCount = (int)p.size();
    for (const Row& row : p)
    {
      float u = 0.0f;
      for (const Key& k : row) u += k.width;
      if (u * 100.0f > mMaxUnitsX100) mMaxUnitsX100 = u * 100.0f;
    }
  }
  if (mMaxRowCount <= 0) mMaxRowCount = 1;
  if (mMaxUnitsX100 <= 0) mMaxUnitsX100 = 100.0f;
}

String GuiSimplifiedVirtualKeyboard::PageLabel(int page) const
{
  if (page == 0) return String("abc");
  if (page == 1) return String("ABC");
  return String("#+=");
}

// =====================================================================
//   Layout
// =====================================================================

void GuiSimplifiedVirtualKeyboard::RecomputeLayout()
{
  const float screenW = Renderer::Instance().DisplayWidthAsFloat();
  const float screenH = Renderer::Instance().DisplayHeightAsFloat();

  // Vertical layout, top to bottom: keyboard, edit field, then the reserved
  // help-bar band at the very bottom.
  const float topPad      = screenH * 0.03f;
  const float gap         = screenH * 0.015f;
  const float helpBarBand = screenH * sHelpBarReservedRatio;

  const float textFontH = (float)mTextFont->Height();
  const float titleH    = (float)mTitleFont->Height();
  const float pad       = textFontH * 0.25f;
  const float editorW   = screenW * 0.9f;

  const float outerEditorH = textFontH + (pad * 2.0f);
  const float editorBlockH = (titleH * 1.2f) + outerEditorH;
  const float belowKeyboard = gap + editorBlockH + gap + helpBarBand;
  const float vBudget = screenH - topPad - belowKeyboard;

  const float maxUnits = mMaxUnitsX100 * 0.01f;

  // Square keys, sized to fit the width, never taller than the vertical budget.
  mKeyHeight = Math::min(screenW / maxUnits, vBudget / (float)mMaxRowCount);
  mUnitWidth = mKeyHeight;
  mKeyboardTop = topPad;
  mKeyboardHeight = mKeyHeight * (float)mMaxRowCount;
  mKeyboardNaturalWidth = maxUnits * mUnitWidth;
  mKeyboardLeft = (screenW - mKeyboardNaturalWidth) * 0.5f;

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

GuiSimplifiedVirtualKeyboard::Rect GuiSimplifiedVirtualKeyboard::KeyRectangle(int row, int col) const
{
  const Row& r = CurrentPage()[row];
  float unitsBefore = 0.0f;
  for (int i = 0; i < col && i < (int)r.size(); ++i)
    unitsBefore += r[i].width;
  float rowUnits = 0.0f;
  for (const Key& k : r) rowUnits += k.width;

  const float rowNaturalW = rowUnits * mUnitWidth;
  const float rowLeftInside = (mKeyboardNaturalWidth - rowNaturalW) * 0.5f;

  Rect rect;
  rect.x = mKeyboardLeft + rowLeftInside + (unitsBefore * mUnitWidth);
  rect.y = mKeyboardTop + ((float)row * mKeyHeight);
  rect.w = r[col].width * mUnitWidth;
  rect.h = mKeyHeight;
  return rect;
}

void GuiSimplifiedVirtualKeyboard::ClampSelection()
{
  const Page& page = CurrentPage();
  if (mSelRow < 0) mSelRow = 0;
  if (mSelRow >= (int)page.size()) mSelRow = (int)page.size() - 1;
  if (mSelCol < 0) mSelCol = 0;
  if (mSelCol >= (int)page[mSelRow].size()) mSelCol = (int)page[mSelRow].size() - 1;
}

void GuiSimplifiedVirtualKeyboard::MoveGridSelection(int dRow, int dCol)
{
  const Page& page = CurrentPage();
  const int rowCount = (int)page.size();

  // Vertical move: wrap around, keeping the horizontal position (sticky anchor).
  if (dRow != 0)
  {
    if (mDesiredCenterX < 0.0f)
    {
      const Rect cur = KeyRectangle(mSelRow, mSelCol);
      mDesiredCenterX = cur.x + (cur.w * 0.5f);
    }
    const float targetX = mDesiredCenterX;

    mSelRow += dRow;
    if (mSelRow < 0) mSelRow = rowCount - 1;
    else if (mSelRow >= rowCount) mSelRow = 0;

    const int n = (int)page[mSelRow].size();
    int best = 0;
    float bestDist = -1.0f;
    for (int i = 0; i < n; ++i)
    {
      const Rect r = KeyRectangle(mSelRow, i);
      if (targetX >= r.x && targetX < r.x + r.w) { best = i; break; }
      float d = targetX - (r.x + (r.w * 0.5f));
      if (d < 0) d = -d;
      if (bestDist < 0.0f || d < bestDist) { bestDist = d; best = i; }
    }
    mSelCol = best;
  }

  // Horizontal move: wrap inside the current row, drop the sticky anchor.
  if (dCol != 0)
  {
    const int n = (int)page[mSelRow].size();
    mSelCol += dCol;
    if (mSelCol < 0) mSelCol = n - 1;
    else if (mSelCol >= n) mSelCol = 0;
    mDesiredCenterX = -1.0f;
  }
}

void GuiSimplifiedVirtualKeyboard::StartGridRepeat(int dRow, int dCol)
{
  mNavRepeatDRow = dRow;
  mNavRepeatDCol = dCol;
  mNavRepeatTimerMs = sGridRepeatStartMs;
  MoveGridSelection(dRow, dCol);
}

void GuiSimplifiedVirtualKeyboard::StopGridRepeat()
{
  mNavRepeatDRow = 0;
  mNavRepeatDCol = 0;
}

void GuiSimplifiedVirtualKeyboard::ChangePage(int delta)
{
  const int n = (int)mPages.size();
  if (n <= 0) return;
  mPage += delta;
  while (mPage < 0) mPage += n;
  while (mPage >= n) mPage -= n;
  mDesiredCenterX = -1.0f;
  ClampSelection();
}

// =====================================================================
//   Editing
// =====================================================================

bool GuiSimplifiedVirtualKeyboard::IsCharAllowed(unsigned int unicode) const
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

void GuiSimplifiedVirtualKeyboard::PressSelectedKey()
{
  const Key& key = CurrentPage()[mSelRow][mSelCol];
  switch (key.kind)
  {
    case KeyKind::Char:      AddCharacter(key.code); break;
    case KeyKind::Space:     AddCharacter((unsigned int)' '); break;
    case KeyKind::Backspace: Backspace(); break;
    case KeyKind::Enter:     ValidateEditor(); break;
    case KeyKind::Left:      MoveTextCursor(-1); break;
    case KeyKind::Right:     MoveTextCursor(1); break;
  }
}

void GuiSimplifiedVirtualKeyboard::AddCharacter(unsigned int unicode)
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

void GuiSimplifiedVirtualKeyboard::Backspace()
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

void GuiSimplifiedVirtualKeyboard::DeleteAtCursor()
{
  if (mCursor < (int)mText.size())
  {
    mText.erase(mText.begin() + mCursor);
    AdjustCursor((int)mInnerEditor.w / 4);
    ResetCursorBlink();
    if (mValidateCallback != nullptr)
      mValidateCallback->VirtualKeyboardTextChange(*this, String::JoinUnicodes(mText));
  }
}

void GuiSimplifiedVirtualKeyboard::ValidateEditor()
{
  if (mValidateCallback != nullptr)
    mValidateCallback->VirtualKeyboardValidated(*this, String::JoinUnicodes(mText));
  Close();
  if (mValidateCallback != nullptr)
    mValidateCallback->VirtualKeyboardHide(*this);
}

void GuiSimplifiedVirtualKeyboard::MoveTextCursor(int delta)
{
  int nc = mCursor + delta;
  if (nc < 0) nc = 0;
  if (nc > (int)mText.size()) nc = (int)mText.size();
  if (nc != mCursor)
  {
    mCursor = nc;
    AdjustCursor();
    ResetCursorBlink();
  }
}

void GuiSimplifiedVirtualKeyboard::AdjustCursor(int cursorextend)
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

void GuiSimplifiedVirtualKeyboard::ResetCursorBlink()
{
  mCursorBlinkReference = DateTime();
}

// =====================================================================
//   Input
// =====================================================================

void GuiSimplifiedVirtualKeyboard::textInput(const char* text)
{
  for (unsigned int unicode : String(text).SplitUnicodes())
    AddCharacter(unicode);
  AdjustCursor();
}

bool GuiSimplifiedVirtualKeyboard::ProcessInput(const InputCompactEvent& event)
{
  // ----- Physical keyboard -----
  if (event.IsKeyboard())
  {
    bool released = event.RawEvent().Value() == 0;
    switch (event.RawEvent().Id())
    {
      case SDLK_BACKSPACE: if (released) Backspace(); break;
      case SDLK_DELETE:    if (released) DeleteAtCursor(); break;
      case SDLK_ESCAPE:
      case SDLK_RETURN:
      case SDLK_RETURN2:   if (released) ValidateEditor(); break;
      case SDLK_LEFT:      if (!released) MoveTextCursor(-1); break;
      case SDLK_RIGHT:     if (!released) MoveTextCursor(1); break;
      default: break;
    }
    return true;
  }

  // ----- Grid navigation (D-pad) with auto-repeat -----
  if (event.UpPressed())    { StartGridRepeat(-1, 0); return true; }
  if (event.DownPressed())  { StartGridRepeat( 1, 0); return true; }
  if (event.LeftPressed())  { StartGridRepeat( 0,-1); return true; }
  if (event.RightPressed()) { StartGridRepeat( 0, 1); return true; }
  if (event.UpReleased()    && mNavRepeatDRow < 0) { StopGridRepeat(); return true; }
  if (event.DownReleased()  && mNavRepeatDRow > 0) { StopGridRepeat(); return true; }
  if (event.LeftReleased()  && mNavRepeatDCol < 0) { StopGridRepeat(); return true; }
  if (event.RightReleased() && mNavRepeatDCol > 0) { StopGridRepeat(); return true; }

  // ----- Page switching (L1/R1), wrapping like the wheel's charsets -----
  if (event.L1Pressed()) { ChangePage(-1); return true; }
  if (event.R1Pressed()) { ChangePage(+1); return true; }

  // ----- Text cursor (L2/R2) with auto-repeat -----
  if (event.L2Pressed())  { mTextCursorDirection = Direction::Left;  mTextCursorRepeatTimerMs = sCursorRepeatStartMs; MoveTextCursor(-1); return true; }
  if (event.L2Released()) { if (mTextCursorDirection == Direction::Left)  mTextCursorDirection = Direction::None; return true; }
  if (event.R2Pressed())  { mTextCursorDirection = Direction::Right; mTextCursorRepeatTimerMs = sCursorRepeatStartMs; MoveTextCursor(1); return true; }
  if (event.R2Released()) { if (mTextCursorDirection == Direction::Right) mTextCursorDirection = Direction::None; return true; }

  // ----- Validate (Cancel / Start) -----
  if (event.CancelReleased() || event.StartReleased()) { ValidateEditor(); return true; }

  // ----- Edit shortcuts -----
  if (event.NorthPressed()) { DeleteAtCursor(); return true; }
  if (event.WestPressed()) { Backspace(); return true; }

  // ----- Press the selected key -----
  if (event.ValidReleased()) { PressSelectedKey(); return true; }

  return true;
}

void GuiSimplifiedVirtualKeyboard::Update(int deltaTime)
{
  // Auto-repeat for text cursor while L2/R2 is held.
  if (mTextCursorDirection != Direction::None)
  {
    mTextCursorRepeatTimerMs -= deltaTime;
    while (mTextCursorRepeatTimerMs <= 0)
    {
      mTextCursorRepeatTimerMs += sCursorRepeatTickMs;
      if (mTextCursorDirection == Direction::Left  && mCursor > 0) MoveTextCursor(-1);
      else if (mTextCursorDirection == Direction::Right && mCursor < (int)mText.size()) MoveTextCursor(1);
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

// =====================================================================
//   Rendering
// =====================================================================

void GuiSimplifiedVirtualKeyboard::RenderKey(const Key& key, const Rect& rect, bool selected)
{
  const float inset = mKeyHeight * 0.06f;
  unsigned int bg = selected ? mColors.KeySelectedColor : mColors.KeyColor;
  Renderer::DrawRectangle(rect.x + inset, rect.y + inset, rect.w - 2 * inset, rect.h - 2 * inset, bg);

  // Draw the glyph/label centered. On low-res the CRT pixel font must be snapped
  // to integer pixels on a shared baseline, otherwise fractional sampling drops
  // pixels from the glyphs. Higher resolutions keep smooth box-centering.
  const bool isChar = (key.kind == KeyKind::Char || key.kind == KeyKind::Space);
  unsigned int c = (key.kind == KeyKind::Space) ? (unsigned int)' ' : (isChar ? key.code : 0u);
  unsigned int color = (isChar && !IsCharAllowed(c)) ? mColors.KeyDisabledColor : mColors.KeyTextColor;

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
      const int x = Math::roundi(rect.x + (rect.w - (float)mKeyFont->TextWidth(key.label)) * 0.5f);
      mKeyFont->DrawText(key.label, x, top, color, ::Alignment::TopLeft);
    }
  }
  else
  {
    const int cx = (int)(rect.x + rect.w * 0.5f);
    const int cy = (int)(rect.y + rect.h * 0.5f);
    if (isChar) mKeyFont->DrawChar(c, cx, cy, color, ::Alignment::Center);
    else        mKeyFont->DrawText(key.label, cx, cy, color, ::Alignment::Center);
  }
}

void GuiSimplifiedVirtualKeyboard::RenderKeyboard()
{
  const Page& page = CurrentPage();
  for (int r = 0; r < (int)page.size(); ++r)
  {
    const Row& row = page[r];
    for (int c = 0; c < (int)row.size(); ++c)
      RenderKey(row[c], KeyRectangle(r, c), (r == mSelRow) && (c == mSelCol));
  }
}

void GuiSimplifiedVirtualKeyboard::RenderEditedString()
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
    Renderer::DrawRectangle(offsetX, mInnerEditor.y, (float)mTextFont->CharWidth(32), mInnerEditor.h, cursorColor);
}

void GuiSimplifiedVirtualKeyboard::RenderTextBox()
{
  Renderer::DrawRectangle(mOuterEditor.x, mOuterEditor.y, mOuterEditor.w, mOuterEditor.h, 0x00000080u);

  const int titleY = (int)mOuterEditor.y - (int)((float)mTitleFont->Height() * 1.2f);
  mTitleFont->DrawText(mTitle, (int)mInnerEditor.x, titleY, mColors.TitleColor, ::Alignment::TopLeft);

  // Page indicator (e.g. "abc 1/3"), right-aligned above the edit box.
  String pageInfo = PageLabel(mPage);
  pageInfo.Append(" ").Append(mPage + 1).Append("/").Append((int)mPages.size());
  mTitleFont->DrawText(pageInfo, (int)(mInnerEditor.x + mInnerEditor.w), titleY, mColors.TitleColor, ::Alignment::TopRight);

  Renderer::Instance().Clip(::Rectangle(mInnerEditor.x, mInnerEditor.y, mInnerEditor.w, mInnerEditor.h));
  RenderEditedString();
  Renderer::Instance().Unclip();
}

void GuiSimplifiedVirtualKeyboard::Render(const Transform4x4f& parentTrans)
{
  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  Renderer::DrawRectangle(0.f, 0.f,
                          Renderer::Instance().DisplayWidthAsFloat(),
                          Renderer::Instance().DisplayHeightAsFloat(),
                          0x00000080u);

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

bool GuiSimplifiedVirtualKeyboard::CollectHelpItems(Help& help)
{
  help.Clear()
      .Set(Help::Valid(), _("PRESS KEY"))
      .Set(Help::Cancel(), _("CLOSE"))
      .Set(HelpType::X, _("DELETE"))
      .Set(HelpType::Y, _("BACKSPACE"))
      .Set(HelpType::LR, _("CHANGE PAGE"))
      .Set(HelpType::L2R2, _("MOVE CURSOR"))
      .Set(HelpType::AllDirections, _("MOVE SELECTION"));
  return true;
}
