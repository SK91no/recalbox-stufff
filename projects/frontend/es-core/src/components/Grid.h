#pragma once

#include "components/base/Component.h"
#include <utils/math/Vector2i.h>
#include <memory>

// Used to arrange a bunch of components in a spreadsheet-esque grid.
class Grid : public Component
{
  public:
    Grid(WindowManager& window, int w, int h);

    void SetGridDimensions(int w, int h);

    [[nodiscard]] int EntryCount() const
    { return (int) mCells.size(); }

    void ClearEntries() { mCells.clear(); mCursorX = mCursorY = 0; }

    bool removeEntry(Component* comp);

    void setEntry(Component* comp, int x, int y, bool canFocus, bool resize = true,
                  int w = 1, int h = 1);

    void setEntry(Component* comp, int x, int y, bool canFocus, HorizontalAlignment ha, VerticalAlignment va,
                  bool resize = true, int w = 1, int h = 1);

    void textInput(const char* text) override;

    bool ProcessInput(const InputCompactEvent& event) override;

    void Update(int deltaTime) override;

    void Render(const Transform4x4f& parentTrans) override;

    void onSizeChanged() override;

    void resetCursor();

    bool cursorValid();

    float getColWidth(int col);

    float getRowHeight(int row);

    float getColWidth(int col1, int col2);

    float getRowHeight(int row1, int row2);

    void setColWidthPerc(int col, float width,
                         bool update = true); // if update is false, will not call an onSizeChanged() which triggers a (potentially costly) repositioning + resizing of every element
    void setRowHeightPerc(int row, float height,
                          bool update = true); // if update is false, will not call an onSizeChanged() which triggers a (potentially costly) repositioning + resizing of every element

    bool moveCursor(int dirX, int dirY);

    void setCursorTo(const Component* comp);

    void setCursor(int x, int y) { mCursorX = x; mCursorY = y; }

    [[nodiscard]] int getCursorX() const { return mCursorX; }
    [[nodiscard]] int getCursorY() const { return mCursorY; }

    void setUnhandledInputCallback(const std::function<bool(const InputCompactEvent&)>& func)
    { mUnhandledInputCallback = func; }

    Component* getSelectedComponent()
    {
      GridEntry* e = getCellAt(mCursorX, mCursorY);
      if (e != nullptr) return e->component;
      return nullptr;
    }

    void onFocusLost() override;

    void onFocusGained() override;

    bool CollectHelpItems(Help& help) override;

    void SetRowHighlight(bool active, int from = 0, int to = 0)
    {
      mHighlightRows = active;
      mHighlightRowFrom = from;
      mHighlightRowTo = to;
    }

    void SetColumnHighlight(bool active, int from = 0, int to = 0)
    {
      mHighlightColumns = active;
      mHighlightColumnFrom = from;
      mHighlightColumnTo = to;
    }

    [[nodiscard]] int Columns() const { return mWidth; }

    [[nodiscard]] int Rows() const { return mHeight; }

  private:
    class GridEntry
    {
      public:
        int mX;
        int mY;
        int mWidth;
        int mHeight;
        Component* component;
        HorizontalAlignment hzAlign;
        VerticalAlignment vtAlign;
        bool canFocus;
        bool resize;

        explicit GridEntry(int x, int y, int w, int h, Component* cmp)
          : mX(x)
          , mY(y)
          , mWidth(w)
          , mHeight(h)
          , component(cmp)
          , hzAlign(HorizontalAlignment::Center)
          , vtAlign(VerticalAlignment::Center)
          , canFocus(false)
          , resize(true)
        {
        }

        explicit GridEntry(int x, int y, int w, int h, Component* cmp,
                           HorizontalAlignment ha, VerticalAlignment va)
          : mX(x)
          , mY(y)
          , mWidth(w)
          , mHeight(h)
          , component(cmp)
          , hzAlign(ha)
          , vtAlign(va)
          , canFocus(false)
          , resize(true)
        {
        }

        explicit GridEntry(int x, int y, int w, int h, Component* cmp, bool f, bool r)
          : mX(x)
          , mY(y)
          , mWidth(w)
          , mHeight(h)
          , component(cmp)
          , hzAlign(HorizontalAlignment::Center)
          , vtAlign(VerticalAlignment::Center)
          , canFocus(f)
          , resize(r)
        {
        }

        explicit GridEntry(int x, int y, int w, int h, Component* cmp,
                           HorizontalAlignment ha, VerticalAlignment va, bool f, bool r)
          : mX(x)
          , mY(y)
          , mWidth(w)
          , mHeight(h)
          , component(cmp)
          , hzAlign(ha)
          , vtAlign(va)
          , canFocus(f)
          , resize(r)
        {
        }

        explicit operator bool() const
        {
          return component != nullptr;
        }
    };

    std::vector<float> mRowHeights;
    std::vector<float> mColWidths;

    struct Vert
    {
      explicit Vert(float xi = 0, float yi = 0)
        : x(xi), y(yi)
      {};
      float x;
      float y;
    };

    std::vector<Vert> mLines;
    std::vector<unsigned int> mLineColors;

    std::vector<GridEntry> mCells;

    int mWidth;
    int mHeight;
    int mCursorX;
    int mCursorY;

    int mHighlightRowFrom;
    int mHighlightRowTo;
    int mHighlightColumnFrom;
    int mHighlightColumnTo;

    bool mHighlightRows;
    bool mHighlightColumns;

    void onCursorMoved(int xfrom, int yfrom, int xto, int yto);

    std::function<bool(const InputCompactEvent&)> mUnhandledInputCallback;

    GridEntry* getCellAt(int x, int y);

    // Update position & size
    void updateCellComponent(const GridEntry& cell);

    void updateSeparators();

    GridEntry* getSelectedEntry() { return getCellAt(mCursorX, mCursorY); }
};
