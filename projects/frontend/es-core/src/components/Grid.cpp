#include "components/Grid.h"
#include "Renderer.h"
#include <utils/locale/LocaleHelper.h>
#include <input/InputCompactEvent.h>
#include <help/Help.h>

Grid::Grid(WindowManager& window, int w, int h)
  : Component(window)
  , mWidth(0)
  , mHeight(0)
  , mCursorX(0)
  , mCursorY(0)
  , mHighlightRowFrom(0)
  , mHighlightRowTo(0)
  , mHighlightColumnFrom(0)
  , mHighlightColumnTo(0)
  , mHighlightRows(false)
  , mHighlightColumns(false)
  , mUnhandledInputCallback(nullptr)
{
  SetGridDimensions(w, h);
}

void Grid::SetGridDimensions(int w, int h)
{
  if (w == 0 || h == 0) return;

  mWidth = w;
  mHeight = h;

  mCells.clear();
  mCells.reserve(w * h);

  mColWidths.resize(w, 0.0f);
  mRowHeights.resize(h, 0.0f);
}

float Grid::getColWidth(int col)
{
  if (mColWidths[col] != 0)
    return mColWidths[col] * mSize.x();

  // calculate automatic width
  float freeWidthPerc = 1.0f;
  int between = 0;
  for (int x = mWidth; --x >= 0;)
  {
    freeWidthPerc -= mColWidths[x]; // if it's 0 it won't do anything
    if (mColWidths[x] == 0)
      between++;
  }

  return (freeWidthPerc * mSize.x()) / (float) between;
}

float Grid::getRowHeight(int row)
{
  if (mRowHeights[row] != 0)
    return mRowHeights[row] * mSize.y();

  // calculate automatic height
  float freeHeightPerc = 1.0f;
  int between = 0;
  for (int y = mHeight; --y >= 0;)
  {
    freeHeightPerc -= mRowHeights[y]; // if it's 0 it won't do anything
    if (mRowHeights[y] == 0) between++;
  }

  return (freeHeightPerc * mSize.y()) / (float) between;
}

float Grid::getColWidth(int col1, int col2)
{
  float result = 0;
  for (++col2; --col2 >= col1;)
    result += getColWidth(col2);
  return result;
}

float Grid::getRowHeight(int row1, int row2)
{
  float result = 0;
  for (++row2; --row2 >= row1;)
    result += getRowHeight(row2);
  return result;
}

void Grid::setColWidthPerc(int col, float width, bool update)
{
  assert(width >= 0 && width <= 1);
  assert(col >= 0 && col < mWidth);
  mColWidths[col] = width;

  if (update)
    onSizeChanged();
}

void Grid::setRowHeightPerc(int row, float height, bool update)
{
  assert(height >= 0 && height <= 1);
  assert(row >= 0 && row < mHeight);
  mRowHeights[row] = height;

  if (update)
    onSizeChanged();
}

void Grid::setEntry(Component* comp, int x, int y, bool canFocus, bool resize,
                    int w, int h)
{
  assert(x >= 0 && x < mWidth && y >= 0 && y < mHeight);
  assert(comp != nullptr);
  assert(comp->getParent() == nullptr);

  GridEntry entry(x, y, w, h, comp, canFocus, resize);
  mCells.push_back(entry);

  addChild(comp);

  if (!cursorValid() && canFocus)
  {
    int oldx = mCursorX;
    int oldy = mCursorY;
    mCursorX = x;
    mCursorY = y;
    onCursorMoved(oldx, oldy, mCursorX, mCursorY);
  }

  updateCellComponent(mCells.back());
  updateSeparators();
}

void Grid::setEntry(Component* comp, int x, int y, bool canFocus, HorizontalAlignment ha, VerticalAlignment va,
                    bool resize, int w, int h)
{
  assert(x >= 0 && x < mWidth && y >= 0 && y < mHeight);
  assert(comp != nullptr);
  assert(comp->getParent() == nullptr);

  GridEntry entry(x, y, w, h, comp, ha, va, canFocus, resize);
  mCells.push_back(entry);

  addChild(comp);

  if (!cursorValid() && canFocus)
  {
    int oldx = mCursorX;
    int oldy = mCursorY;
    mCursorX = x;
    mCursorY = y;
    onCursorMoved(oldx, oldy, mCursorX, mCursorY);
  }

  updateCellComponent(mCells.back());
  updateSeparators();
}

bool Grid::removeEntry(Component* comp)
{
  for(int i = (int)mCells.size(); --i >= 0;)
    if (mCells[i].component == comp)
    {
      removeChild(mCells[i].component);
      mCells.erase(mCells.begin() + i);
      return true;
    }

  return false;
}

void Grid::updateCellComponent(const GridEntry& cell)
{
  // size
  Vector2f size(0, 0);
  for (int x = cell.mX; x < cell.mX + cell.mWidth; x++) size[0] += getColWidth(x);
  for (int y = cell.mY; y < cell.mY + cell.mHeight; y++) size[1] += getRowHeight(y);

  if (cell.resize)
    cell.component->setSize(size);

  // position
  // find top left corner
  Vector3f pos(0, 0, 0);
  for (int x = 0; x < cell.mX; x++) pos[0] += getColWidth(x);
  for (int y = 0; y < cell.mY; y++) pos[1] += getRowHeight(y);

  // center component
  switch(cell.hzAlign)
  {
    case HorizontalAlignment::Left:break;
    case HorizontalAlignment::Center: pos[0] = pos.x() + ((size.x() - cell.component->getSize().x()) / 2); break;
    case HorizontalAlignment::Right: pos[0] = pos.x() + (size.x() - cell.component->getSize().x()); break;
  }
  switch(cell.vtAlign)
  {
    case VerticalAlignment::Top:break;
    case VerticalAlignment::Center: pos[1] = pos.y() + ((size.y() - cell.component->getSize().y()) / 2); break;
    case VerticalAlignment::Bottom: pos[1] = pos.y() + (size.y() - cell.component->getSize().y()); break;
  }

  cell.component->setPosition(pos);
}

void Grid::updateSeparators()
{
  mLines.clear();

  Vector2f pos(0, 0);
  Vector2f size(0, 0);
  for (auto& mCell: mCells)
  {
    //if (mCell.border == 0) continue;

    // find component position + size
    for (int x = 0; x < mCell.mX; x++) pos[0] += getColWidth(x);
    for (int y = 0; y < mCell.mY; y++)  pos[1] += getRowHeight(y);
    for (int x = mCell.mX; x < mCell.mX + mCell.mWidth; x++)  size[0] += getColWidth(x);
    for (int y = mCell.mY; y < mCell.mY + mCell.mHeight; y++) size[1] += getRowHeight(y);

    /*if (hasFlag(mCell.border, Borders::Top))
    {
      mLines.push_back(Vert(pos.x(), pos.y()));
      mLines.push_back(Vert(pos.x() + size.x(), pos.y()));
    }
    if (hasFlag(mCell.border, Borders::Bottom))
    {
      mLines.push_back(Vert(pos.x(), pos.y() + size.y()));
      mLines.push_back(Vert(pos.x() + size.x(), mLines.back().y));
    }
    if (hasFlag(mCell.border, Borders::Left))
    {
      mLines.push_back(Vert(pos.x(), pos.y()));
      mLines.push_back(Vert(pos.x(), pos.y() + size.y()));
    }
    if (hasFlag(mCell.border, Borders::Right))
    {
      mLines.push_back(Vert(pos.x() + size.x(), pos.y()));
      mLines.push_back(Vert(mLines.back().x, pos.y() + size.y()));
    }*/
  }

  mLineColors.reserve(mLines.size());
  Renderer::BuildGLColorArray((ColorByte*) mLineColors.data(), 0xC6C7C6FF, (int)mLines.size());
}

void Grid::onSizeChanged()
{
  for (auto& mCell: mCells)
    updateCellComponent(mCell);

  updateSeparators();
}

Grid::GridEntry* Grid::getCellAt(int x, int y)
{
  assert(x >= 0 && x < mWidth && y >= 0 && y < mHeight);

  for (auto& mCell: mCells)
  {
    int xmin = mCell.mX;
    int xmax = xmin + mCell.mWidth;
    int ymin = mCell.mY;
    int ymax = ymin + mCell.mHeight;

    if (x >= xmin && y >= ymin && x < xmax && y < ymax)
      return &mCell;
  }

  return nullptr;
}

bool Grid::ProcessInput(const InputCompactEvent& event)
{
  GridEntry* cursorEntry = getSelectedEntry();

  if ((cursorEntry != nullptr) && cursorEntry->component->ProcessInput(event))
    return true;

  if (event.AnyDownPressed()) return moveCursor(0, 1);
  if (event.AnyUpPressed()) return moveCursor(0, -1);
  if (event.AnyLeftPressed()) return moveCursor(-1, 0);
  if (event.AnyRightPressed()) return moveCursor(1, 0);
  if (mUnhandledInputCallback) return mUnhandledInputCallback(event);

  return false;
}

void Grid::resetCursor()
{
  if (mCells.empty())
    return;

  for (auto& mCell: mCells)
  {
    if (mCell.canFocus)
    {
      int oldx = mCursorX;
      int oldy = mCursorY;
      mCursorX = mCell.mX;
      mCursorY = mCell.mY;
      onCursorMoved(oldx, oldy, mCursorX, mCursorY);
      break;
    }
  }
}

bool Grid::moveCursor(int dirX, int dirY)
{
  assert(dirX || dirY);

  GridEntry* current = getSelectedEntry();
  if (current == nullptr) return false;
  int xCenter = (int)getColWidth(0, current->mX) + (current->mWidth / 2);
  int yCenter = (int)getRowHeight(0, current->mY) + (current->mHeight / 2);

  int max = INT32_MAX;

  GridEntry* target = nullptr;
  for (GridEntry& e : mCells)
    if (&e != current)
    {
      int xCurrent = (int)getColWidth(0, e.mX) + (current->mWidth / 2);
      int yCurrent = (int)getRowHeight(0, e.mY) + (current->mHeight / 2);

      int xDelta = xCurrent - xCenter;
      int yDelta = yCurrent - yCenter;
      if      (xDelta > 0 && dirX < 0) xDelta -= (int)mSize.x();
      else if (xDelta < 0 && dirX > 0) xDelta += (int)mSize.x();
      if      (yDelta > 0 && dirY < 0) yDelta -= (int)mSize.y();
      else if (yDelta < 0 && dirY > 0) yDelta += (int)mSize.y();

      if (int c = Math::diagonali(xDelta, yDelta); c < max) { max = c; target = &e; }
    }
  if (target != nullptr)
  {
    int oldx = mCursorX;
    int oldy = mCursorY;
    mCursorX = target->mX;
    mCursorY = target->mY;
    onCursorMoved(oldx, oldy, mCursorX, mCursorY);
    return true;
  }

  return false;
}

void Grid::onFocusLost()
{
  GridEntry* cursorEntry = getSelectedEntry();
  if (cursorEntry != nullptr)
    cursorEntry->component->onFocusLost();
}

void Grid::onFocusGained()
{
  GridEntry* cursorEntry = getSelectedEntry();
  if (cursorEntry != nullptr)
    cursorEntry->component->onFocusGained();
}

bool Grid::cursorValid()
{
  GridEntry* e = getSelectedEntry();
  return (e != nullptr && e->canFocus);
}

void Grid::Update(int deltaTime)
{
  for (auto& mCell: mCells)
      mCell.component->Update(deltaTime);
}

void Grid::Render(const Transform4x4f& parentTrans)
{
  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  // Render rows
  if (mHighlightRows)
  {
    float x = /*mPosition.x() +*/ getColWidth(0, mHighlightColumnFrom - 1);
    float w = getColWidth(mHighlightColumnFrom, mHighlightColumnTo);
    float y = /*mPosition.y() +*/ getRowHeight(0, mHighlightRowFrom - 1);
    for (int i = mHighlightRowFrom - 1; ++i <= mHighlightRowTo;)
    {
      float h = getRowHeight(i);
      Renderer::DrawRectangle(x, y, w, h, (i & 1) != 0 ? 0xFFFFFF18 : 0x00000018);
      y += h;
    }
  }

  if (mHighlightColumns)
  {
    float y = mPosition.y() + getRowHeight(0, mHighlightRowFrom - 1);
    float h = getRowHeight(mHighlightRowFrom, mHighlightRowTo);
    float x = mPosition.x() + getColWidth(0, mHighlightColumnFrom - 1);
    for (int i = mHighlightColumnFrom - 1; ++i < mHighlightColumnTo;)
    {
      float w = getColWidth(i);
      Renderer::DrawRectangle(x, y, w, h, (i & 1) != 0 ? 0xFFFFFF18 : 0x00000018);
      x += w;
    }
  }

  renderChildren(trans);

  // draw cell separators
  /*if (!mLines.empty())
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(2, GL_FLOAT, 0, &mLines[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, mLineColors.data());

    glDrawArrays(GL_LINES, 0, mLines.size());

    glDisable(GL_BLEND);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
  }*/
}

void Grid::textInput(const char* text)
{
  GridEntry* selectedEntry = getSelectedEntry();
  if (selectedEntry != nullptr && selectedEntry->canFocus)
    selectedEntry->component->textInput(text);
}

void Grid::onCursorMoved(int xfrom, int yfrom, int xto, int yto)
{
  GridEntry* cell = getCellAt(xfrom, yfrom);
  if (cell != nullptr) cell->component->onFocusLost();

  cell = getCellAt(xto, yto);
  if (cell != nullptr) cell->component->onFocusGained();

  UpdateHelpBar();
}

void Grid::setCursorTo(const Component* comp)
{
  for (auto& mCell: mCells)
  {
    if (mCell.component == comp)
    {
      int oldx = mCursorX;
      int oldy = mCursorY;
      mCursorX = mCell.mX;
      mCursorY = mCell.mY;
      onCursorMoved(oldx, oldy, mCursorX, mCursorY);
      return;
    }
  }

  // component not found!!
  assert(false);
}

bool Grid::CollectHelpItems(Help& help)
{
  GridEntry* e = getSelectedEntry();
  bool result = false;
  if (e != nullptr) result |= e->component->CollectHelpItems(help);

  bool canScrollVert = mHeight > 1;
  bool canScrollHoriz = mWidth > 1;
  if (help.IsSet(HelpType::AllDirections)) canScrollHoriz = canScrollVert = false;
  if (help.IsSet(HelpType::LeftRight)) canScrollHoriz = false;
  if (help.IsSet(HelpType::UpDown)) canScrollVert = false;

  if (canScrollHoriz && canScrollVert) help.Set(HelpType::AllDirections, _("CHOOSE"));
  else if (canScrollHoriz) help.Set(HelpType::LeftRight, _("CHOOSE"));
  else if (canScrollVert) help.Set(HelpType::UpDown, _("CHOOSE"));

  return canScrollHoriz || canScrollVert || result;
}

