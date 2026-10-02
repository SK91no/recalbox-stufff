#pragma once

#include <utils/String.h>
#include <vector>
#include <memory>
#include "guis/Gui.h"
#include "components/ImageComponent.h"
#include <rendering/fonts/FontManager.h>
#include <RecalboxConf.h>

enum class CursorState
{
  Stopped,
  Scrolling,
};

enum class LoopType
{
  Always,
  PauseAtEnd,
  NeverLoop,
};

struct ScrollTier
{
  int length; // how long we stay on this level before going to the next
  int scrollDelay; // how long between scrolls
};

struct ScrollTierList
{
  const int count;
  const ScrollTier* tiers;
};

// default scroll tiers
const ScrollTier QUICK_SCROLL_TIERS[] = {{.length=500,  .scrollDelay=500},
                                         {.length=5000, .scrollDelay=114},
                                         {.length=0,    .scrollDelay=8}};
const ScrollTierList LIST_SCROLL_STYLE_QUICK = {
  .count=3,
  .tiers=QUICK_SCROLL_TIERS
};

const ScrollTier SLOW_SCROLL_TIERS[] = {{.length=500, .scrollDelay=500},
                                        {.length=0,   .scrollDelay=100}};
const ScrollTierList LIST_SCROLL_STYLE_SLOW = {
  .count=2,
  .tiers=SLOW_SCROLL_TIERS
};


/*
    Warn: onCursorChanged was call many times during the scoll process
    it is done in the final function scroll()
    It mustn't be done in stopScrolling, clear, or listInput
    Else, display is updated to much times and ressources (image loading)
    slow down the whole process
 */

template<typename EntryData, typename UserData>
class IList : public Gui
            , public RecalboxConf::IListFastMoveEnabledNotification
{
  public:
    struct Entry
    {
      String name;
      UserData object;
      EntryData data;
    };

  protected:
    int mCursor;

    int mScrollTier;
    int mScrollVelocity;

    int mScrollTierAccumulator;
    int mScrollCursorAccumulator;

    bool mFastScrollingEnable;
    unsigned char mTitleOverlayOpacity;
    unsigned int mTitleOverlayColor;
    ImageComponent mGradient;
    NewFont* mTitleOverlayFont;

    const ScrollTierList& mTierList;
    const LoopType mLoopType;

    std::vector<Entry> mEntries;

  public:
    IList(WindowManager& window, const ScrollTierList& tierList, LoopType loopType)
      : Gui(window)
      , mCursor(0)
      , mScrollTier(0)
      , mScrollVelocity(0)
      , mScrollTierAccumulator(0)
      , mScrollCursorAccumulator(0)
      , mFastScrollingEnable(RecalboxConf::Instance().GetListFastMoveEnabled())
      , mTitleOverlayOpacity(0x00)
      , mTitleOverlayColor(0xFFFFFF00)
      , mGradient(window)
      , mTierList(tierList), mLoopType(loopType)
    {
      mGradient.setResize(Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat());
      mGradient.setImage(Path(":/scroll_gradient.png"), false);
      mTitleOverlayFont = &FontManager::Instance().FromDefault(FONT_SIZE_LARGE, false);
    }

    IList(WindowManager& window, const ScrollTierList& tierList)
      : IList(window, tierList, LoopType::PauseAtEnd)
    {
    }

    explicit IList(WindowManager& window)
      : IList(window, LIST_SCROLL_STYLE_QUICK, LoopType::PauseAtEnd)
    {
    }

    [[nodiscard]] bool isScrolling() const
    { return (mScrollVelocity != 0 && mScrollTier > 0); }

    int getScrollingVelocity()
    {
      return mScrollVelocity;
    }

    // see onCursorChanged warn
    void stopScrolling()
    {
      listInput(0);
    }

    // see onCursorChanged warn
    void clear(int size)
    {
      mEntries.clear();
      mEntries.reserve(size);
      mCursor = 0;
      listInput(0);
    }

    // see onCursorChanged warn
    void clear()
    {
      mEntries.clear();
      mCursor = 0;
      listInput(0);
    }

    std::vector<UserData> getObjects()
    {
      std::vector<UserData> objects;
      for (auto it = mEntries.begin(); it != mEntries.end(); it++)
      {
        objects.push_back((*it).object);
      }
      return objects;
    }

    Array<UserData> getObjectsArray()
    {
      Array<UserData> objects;
      for (auto it = mEntries.begin(); it != mEntries.end(); it++)
        objects.Add((*it).object);
      return objects;
    }

    [[nodiscard]] int Count() const { return (int) mEntries.size(); }

    [[nodiscard]] bool IsEmpty() const { return mEntries.size() == 0; }

    UserData& getObjectAt(int atIndex)
    {
      return mEntries[atIndex].object;
    }

    const String& getSelectedName()
    {
      assert(size() > 0);
      return mEntries[mCursor].name;
    }

    const UserData& getSelected() const
    {
      assert(size() > 0);
      return mEntries[mCursor].object;
    }

    const  EntryData& getSelectedEntry() const
    {
      assert(size() > 0);
      return mEntries[mCursor].data;
    }

    EntryData& getSelectedEntryAt(int index)
    {
      assert(size() > 0);
      return mEntries[index].data;
    }

    void setCursorIndex(int index)
    {
      if (index >= 0 && index < (int)mEntries.size())
      {
        mCursor = index;
        onCursorChanged(CursorState::Stopped);
      }
    }

    int getCursorIndex() const
    {
      return mCursor;
    }

    // returns true if successful (select is in our list), false if not
    bool setCursor(const UserData& obj, unsigned long offset = 0)
    {
      for (auto it = mEntries.begin() + offset; it != mEntries.end(); it++)
      {
        if ((*it).object == obj)
        {
          mCursor = it - mEntries.begin();
          onCursorChanged(CursorState::Stopped);
          return true;
        }
      }

      return false;
    }

    bool changeCursorName(int cursor, const String& name)
    {
      if ((unsigned int) cursor >= mEntries.size())
      {
        return false;
      }

      auto& entry = mEntries[cursor];
      entry.name = name;
      entry.data.textWidth = -1;

      return true;
    }

    // entry management
    void add(const Entry& e)
    {
      mEntries.push_back(e);
    }

    // entry management
    void insert(int index, const Entry& e)
    {
      mEntries.insert(mEntries.begin() + index, e);
    }

    [[nodiscard]] int size() const { return mEntries.size(); }

    [[nodiscard]] bool isEmpty() const { return mEntries.empty(); }

    [[nodiscard]] bool IsAtStart() const { return mCursor == 0; }

    [[nodiscard]] bool IsAtEnd() const { return mCursor == (int)(mEntries.size() - 1); }

    [[nodiscard]] int getCursor() const { return mCursor; }

  protected:

    // see onCursorChanged warn
    bool listInput(int velocity, int tier = 0)
    {
      // a velocity of 0 = stop scrolling
      mScrollVelocity = velocity;
      mScrollTier = tier;
      mScrollTierAccumulator = 0;
      mScrollCursorAccumulator = 0;

      int prevCursor = mCursor;
      scroll(mScrollVelocity);
      return (prevCursor != mCursor);
    }

    void listUpdate(int deltaTime)
    {
      // update the title overlay opacity
      const int dir = (mScrollTier >= mTierList.count - 1) ? 1
                                                           : -1; // fade in if scroll tier is >= 1, otherwise fade out
      int op = mTitleOverlayOpacity + (deltaTime * dir); // we just do a 1-to-1 time -> opacity, no scaling
      if (op >= 255)
        mTitleOverlayOpacity = 255;
      else if (op <= 0)
        mTitleOverlayOpacity = 0;
      else
        mTitleOverlayOpacity = (unsigned char) op;

      if (!mFastScrollingEnable) mTitleOverlayOpacity = 0;

      if (mScrollVelocity == 0 || size() < 2)
        return;

      mScrollCursorAccumulator += deltaTime;
      mScrollTierAccumulator += deltaTime;

      // we delay scrolling until after scroll tier has updated so isScrolling() returns accurately during onCursorChanged callbacks
      // we don't just do scroll tier first because it would not catch the scrollDelay == tier length case
      int scrollCount = 0;
      while (mScrollCursorAccumulator >= mTierList.tiers[mScrollTier].scrollDelay)
      {
        mScrollCursorAccumulator -= mTierList.tiers[mScrollTier].scrollDelay;
        scrollCount++;
      }

      // are we ready to go even FASTER?
      if (mFastScrollingEnable || mTierList.count < 3)
        while (mScrollTier < mTierList.count - 1 && mScrollTierAccumulator >= mTierList.tiers[mScrollTier].length)
        {
          mScrollTierAccumulator -= mTierList.tiers[mScrollTier].length;
          mScrollTier++;
        }

      // actually perform the scrolling
      for (int i = 0; i < scrollCount; i++)
        scroll(mScrollVelocity);
    }

    String extractFirstTwoAscii7Characters(const String& name)
    {
      String result;
      int position = 0;
      for(String::Unicode c; (c = name.ReadUTF8(position)) != 0; )
        if (c < 128)
          if (result.Append((char)c).Count() >= 2)
            break;
      return result.UpperCase();
    }

    void listRenderTitleOverlay(const Transform4x4f& trans)
    {
      (void) trans;

      if (size() == 0 || mTitleOverlayFont == nullptr || mTitleOverlayOpacity == 0)
        return;

      // we don't bother caching this because it's only two letters and will change pretty much every frame if we're scrolling
      const String text = extractFirstTwoAscii7Characters(getSelectedName());

      Vector2f off = mTitleOverlayFont->TextSize(text);
      off[0] = (Renderer::Instance().DisplayWidthAsFloat() - off.x()) * 0.5f;
      off[1] = (Renderer::Instance().DisplayHeightAsFloat() - off.y()) * 0.5f;

      Transform4x4f identTrans = Transform4x4f::Identity();

      mGradient.setOpacity(mTitleOverlayOpacity);
      mGradient.Render(identTrans);

      mTitleOverlayFont->DrawText(text, (int)off.x(), (int)off.y(), 0xFFFFFF00 | mTitleOverlayOpacity, ::Alignment::Center); // relies on mGradient's render for Renderer::setMatrix()
    }

    void scroll(int amt)
    {
      if (mScrollVelocity == 0 || size() < 2)
      {
        onCursorChanged(CursorState::Stopped);
        return;
      }

      int cursor = mCursor + amt;
      int absAmt = amt < 0 ? -amt : amt;

      // stop at the end if we've been holding down the button for a long time or
      // we're scrolling faster than one item at a time (e.g. page up/down)
      // otherwise, loop around
      if ((mLoopType == LoopType::PauseAtEnd && (mScrollTier > 0 || absAmt > 1)) || mLoopType == LoopType::NeverLoop)
      {
        if (cursor < 0)
        {
          cursor = 0;
          mScrollVelocity = 0;
          mScrollTier = 0;
        }
        else if (cursor >= size())
        {
          cursor = size() - 1;
          mScrollVelocity = 0;
          mScrollTier = 0;
        }
      }
      else
      {
        while (cursor < 0)
        {
          cursor += size();
        }
        while (cursor >= size())
        {
          cursor -= size();
        }
      }

      if (cursor != mCursor)
      {
        onScroll(absAmt);
        mCursor = cursor;
        onCursorChanged((mScrollTier > 0) ? CursorState::Scrolling : CursorState::Stopped);
      }

    }

    virtual void onCursorChanged(const CursorState& state) { (void) state; }

    virtual void onScroll(int amt) { (void) amt; }

  private:

    /*
     * RecalboxConf::IListFastMoveEnabledNotification implementation
     */

    void ConfigurationListFastMoveEnabledChanged(const bool& value) final { mFastScrollingEnable = value; }
};
