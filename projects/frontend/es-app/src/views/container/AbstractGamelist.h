//
// Created by bkg2k on 25/12/25.
//
#pragma once

#include <guis/Gui.h>

#include "GameType.h"
#include "GamelistNameArena.h"
#include "IGamelistContainer.h"
#include "utils/math/Misc.h"

template <typename EntryData>
class AbstractGamelist : public Gui
                       , public IGamelistContainer
{
  public:
    struct ScrollingTime
    {
      int mMoveDelay; //!< Time between 2 moves, in milliseconds
      int mNext;      //!< Total time until next step
    };

    //! Callback interface class
    class CallbackInterface
    {
      public:
        //! Default destructor
        virtual ~CallbackInterface() = default;

        /*!
         * @brief Called when the index just changed
         * @param newIndex New index
         * @param data Selected game
         */
        virtual void BaseListIndexChanged(int newIndex, FileData* data) = 0;

        /*!
         * @brief Callback when a game is renamed to let implementation recalculate name-related values
         * @param newName New name
         * @param game Associated game
         * @param data Associated data
         */
        virtual void BaseListNameChangedAt(const String& newName, FileData* game, EntryData& data) = 0;
    };

    //! Destructor
    ~AbstractGamelist() override = default;

    //! Constructor
    explicit AbstractGamelist(WindowManager& window, SystemData& system, CallbackInterface* interface, const Array<ScrollingTime>& scrollingTimes, bool avoidChildren)
      : Gui(window)
      , mIndex(0)
      , mTargetIndex(0)
      , mDelta(0)
      , mTimeAccumulator(0)
      , mStepTimeAccumulator(0)
      , mTimerIndex(0)
      , mMinimumStep(0x10000 >> 8)
      , mAvoidChildren(avoidChildren)
      , mSystem((system))
      , mCallbackInterface(interface)
      , mScrollings(scrollingTimes)
      , mFolderStack(8)
    {}

    //! System accessor
    [[nodiscard]] SystemData& System() const { return mSystem; }

    /*!
     * @brief Get this list as a Component
     * @return This list as a Component
     */
    [[nodiscard]] Component& AsComponent() final { return *this; }

    /*!
     * @brief Add a new item
     * @param name Entry name
     * @param data Working/display data
     * @param game User data
     * @param type Game type
     */
    void Add(const String& name, const EntryData& data, FileData* game, GameType type) { mEntries.push_back({ mNames.Intern(name), data, game, type}); }

    /*!
     * @brief Add a new item reusing a name index already interned in the arena (no re-hashing)
     * @param nameIndex Name index returned by a previous InternName()/entry
     * @param data Working/display data
     * @param game User data
     * @param type Game type
     */
    void AddIndexed(int nameIndex, const EntryData& data, FileData* game, GameType type) { mEntries.push_back({ nameIndex, data, game, type}); }

    //! Clear the whole list
    void Clear() { mEntries.clear(); mNames.Reset(0); }
    //! Clear the whole list and prepare the list to receive "reserved" entries without reallocating
    void Clear(int reserved) { mEntries.clear(); mEntries.reserve(reserved); mNames.Reset(reserved); }

    //! Get list entry count
    [[nodiscard]] int Count() const { return (int)mEntries.size(); }

    /*!
     * @brief Set new index
     * @param newIndex new Index
     * @param instant True to set the new index instantly w/o animation
     */
    void SetIndex(int newIndex, bool instant)
    {
      if ((unsigned int)newIndex < (unsigned int)mEntries.size())
      {
        while(mAvoidChildren && mEntries[newIndex].IsChild())
          if (--newIndex < 0)
            newIndex = (int)mEntries.size() - 1;
        mTargetIndex = newIndex << 16;
        if (instant) mIndex = mTargetIndex;
      }
    }

    //! Get real index clamped to image list size
    [[nodiscard]] int Index() const
    {
      int realIndex = (mIndex + 32768) >> 16;
      int entryCount = (int) mEntries.size();
      if (!mEntries.empty())
      {
        realIndex %= entryCount;
        if (realIndex < 0) realIndex += entryCount;
      }
      return realIndex;
    }

    //! Get real target clamped to image list size
    [[nodiscard]] int Target() const
    {
      int realTarget = (mTargetIndex + 32768) >> 16;
      int entryCount = (int) mEntries.size();
      if (!mEntries.empty())
      {
        realTarget %= entryCount;
        if (realTarget < 0) realTarget += entryCount;
      }
      return realTarget;
    }

    //! Get index offset
    [[nodiscard]] int Offset() const { return ((mIndex + 32768) & 0xFFFF) - 32768; }

    //! Get game associated to the selected entry
    [[nodiscard]] FileData* GetSelectedGame() override { return mEntries[Index()].mGame; }
    //! Get game associated to the entry at the given index
    [[nodiscard]] FileData* GetGameAt(int index) final { int i = ({ int ir = (index % Count()); ir += (ir < 0 ? Count() : 0); ir; }); return mEntries[i].mGame; }

    //! Get game type associated to the selected entry
    [[nodiscard]] GameType GetSelectedGameType() const { return mEntries[Index()].mType; }
    //! Get user data associated to the entry at th given index
    [[nodiscard]] GameType GetGameTypeAt(int index) const { int i = ({ int ir = (index % Count()); ir += (ir < 0 ? Count() : 0); ir; }); return mEntries[i].mType; }

    //! Get game type associated to the selected entry
    [[nodiscard]] const String& GetSelectedName() const final { mNameCache = mNames.Get(mEntries[Index()].mNameIndex); return mNameCache; }
    //! Get user data associated to the entry at th given index
    [[nodiscard]] const String& GetNameAt(int index) const final { int i = ({ int ir = (index % Count()); ir += (ir < 0 ? Count() : 0); ir; }); mNameCache = mNames.Get(mEntries[i].mNameIndex); return mNameCache; }

    //! Get game type associated to the selected entry
    [[nodiscard]] bool IsSelectedChild() const { return mEntries[Index()].IsChild(); }
    //! Get user data associated to the entry at th given index
    [[nodiscard]] bool IsChildAt(int index) const { int i = ({ int ir = (index % Count()); ir += (ir < 0 ? Count() : 0); ir; }); return mEntries[i].IsChild(); }

  protected:
    //! Entry structure
    //! Field order is memory-layout-sensitive: mGame (pointer, 8-aligned), then the 4-byte name
    //! index, then the variable-sized mData, and finally the 1-byte mType last to minimize padding.
    //! The display name is not stored here: it lives once in the list name arena and is referenced
    //! by mNameIndex. Do not reorder without re-checking sizeof().
    struct Entry
    {
      FileData* mGame;      //!< Associated data
      int       mNameIndex; //!< Display name index into the list name arena (see GamelistNameArena)
      EntryData mData;      //!< Working/Display data
      GameType  mType;      //!< Game type

      Entry(int nameIndex, const EntryData& data, FileData* game, GameType type)
        : mGame(game), mNameIndex(nameIndex), mData(data), mType(type)
      {}

      [[nodiscard]] bool IsChild() const { return mType == GameType::ChildGame || mType == GameType::ArcadeClone; }
      [[nodiscard]] bool IsFolded() const { return mType == GameType::ParentGameFolded || mType == GameType::ArcadeParentFolded; }
      [[nodiscard]] bool IsUnfolded() const { return mType == GameType::ParentGameUnfolded || mType == GameType::ArcadeParentUnfolded; }
    };

    //! Get game associated to the target
    [[nodiscard]] FileData* GetTargetGame() { return mEntries[Target()].mGame; }

    /*!
     * @brief Instruct the base class of the item height in pixel so that it can adjust the minimum step value to
     * match the real size of one plein pixel when moving.
     * @param heightInPixels Item height in pixel
     */
    void SetItemHeightInPixel(int heightInPixels) { mMinimumStep = Math::roundi((float)0x10000 / (float)heightInPixels); }

    //! System accessor
    SystemData& System() { return mSystem; }

    //! Internal real scrolling
    [[nodiscard]] bool IsReallyScrolling() const { return mIndex != mTargetIndex; }

    /*!
     * @brief Start new scrolling in the list. 0 stop the scrolling
     * @param delta Signed delta to apply to current position
     */
    void StartScrolling(int delta)
    {
      if (mEntries.empty()) return;

      if (delta > (int)mEntries.size()) delta %= (int)mEntries.size();
      mDelta = delta;
      mTimeAccumulator = mScrollings(mTimerIndex).mMoveDelay;
      DoScrolling(0);
    }

    /*!
     * @brief Must be called periodically to manage the scrolling
     */
    void DoScrolling(int deltaTime)
    {
      if (mDelta != 0)
      {
        const ScrollingTime& scrollingTime = mScrollings(mTimerIndex);
        // Move delay ?
        if (mTimeAccumulator += deltaTime; mTimeAccumulator >= scrollingTime.mMoveDelay)
        {
          mTimeAccumulator -= scrollingTime.mMoveDelay;
          mTargetIndex += (mDelta << 16);
          while (mAvoidChildren && mEntries[TargetIndex()].IsChild())
            mTargetIndex += (mDelta << 16);
        }
        // Global delay ?
        if (mStepTimeAccumulator += deltaTime; mStepTimeAccumulator >= scrollingTime.mNext)
        {
          mStepTimeAccumulator -= scrollingTime.mNext;
          if (mTimerIndex < mScrollings.Count() - 1) ++mTimerIndex;
        }
      }
      else mTimeAccumulator = mTimerIndex = 0;
      Move();
    }

    //! Intern a display name into the list arena and return its index (deduplicated)
    int InternName(const String& name) { return mNames.Intern(name); }
    //! Get the display name of the given entry as a String copy (reconstructed from the arena)
    [[nodiscard]] String EntryName(const Entry& entry) const { return mNames.Get(entry.mNameIndex); }

    //! Get entry data structure associated to the selected entry
    EntryData& GetSelectedEntryData() { return mEntries[Index()].mData; }
    //! Get entry data structure associated to the selected entry
    EntryData& GetEntryDataAt(int index) { int i = ({ int ir = (index % Count()); ir += (ir < 0 ? Count() : 0); ir; }); return mEntries[i].mData; }

    /*!
     * @brief Rename current game
     * @param newName
     */
    /*virtual void Rename(const String& newName)
    {
      int i = Index();
      mEntries[i].mNameIndex = mNames.Intern(newName);
      mCallbackInterface->BaseListNameChangedAt(newName, mEntries[i].mGame, mEntries[i].mData);
    }*/

    /*!
     * @brief Try to rename list item at the given index
     * @param index Item index
     * @param newName New item name
     * @return True if the index is in range and the item is successfully renamed, false otherwise
     */
    bool RenameAt(int index, const String& newName) final
    {
      if ((unsigned int)index < (unsigned int)Count())
      {
        mEntries[index].mNameIndex = mNames.Intern(newName);
        mCallbackInterface->BaseListNameChangedAt(newName, mEntries[index].mGame, mEntries[index].mData);
        return true;
      }
      return false;
    }
    /*!
     * @brief Try to rename list item linked to the given game
     * @param game Game to lookup
     * @param newName New item name
     * @return True if the game has been found and the item is successfully renamed, false otherwise
     */
    bool Rename(FileData* game, const String& newName) final
    {
      if (int p = LookupGame(game); p >= 0) return RenameAt(p, newName);
      return false;
    }

    //! Get raw entries
    const std::vector<Entry>& GetRawEntries() { return mEntries; }
    //! Get raw entry at the given position
    Entry& GetRawEntryAt(int index)
    {
      int i = ({ int ir = (index % Count()); ir += (ir < 0 ? Count() : 0); ir; });
      return mEntries[i];
    }
    //! Get entry data structure associated to the selected entry
    Entry& GetRawEntry() { return mEntries[Index()]; }

    /*!
     * @brief Lookup game in the list and return index
     * @param game Game too lookup
     * @return Game index or -1 if the given game is not found
     */
    int LookupGame(const FileData* game)
    {
      for(int i = AbstractGamelist::Count(); --i >= 0; )
        if (AbstractGamelist::GetGameAt(i) == game)
          return i;
      return -1;
    }

    /*!
     * @brief Rebuild the whole folder stack from the given game, starting from the top directory down to the game directory.
     * @param from Game from which to rebuild the stack
     * @return This
     */
    IGamelistContainer& RebuildFolderStack(FileData* from) final
    {
      FolderData* folder = from->Parent();
      mFolderStack.Clear();
      while((folder != nullptr) && !folder->IsTopMostRoot())
      {
        mFolderStack.Push(folder);
        folder = folder->Parent();
      }
      mFolderStack.Reverse();
      return *this;
    }

    /*!
     * @brief Get the folder stack depth
     * @return Folder stack depth
     */
    int GetFolderStackDepth() final { return mFolderStack.Count(); }

    /*!
     * @brief Get the current folder list from the top directory down to the current directory
     * @return Directory list (only directories, not fullpath)
     */
    String::List GetBreadcrumb() final
    {
      String::List result;
      for(FolderData* folder : mFolderStack)
        result.push_back(folder->Metadata().RomFileOnly().Filename());
      return result;
    }

    /*!
     * @brief Get current folder
     * @return Current folder
     */
    FolderData* GetCurrentFolder() final { return !mFolderStack.Empty() ? mFolderStack.Peek() : &mSystem.MasterRoot(); };

    /*!
     * @brief Enter the given folder. If the folder exists in the current list, it is added to the stack
     * If it doesn't exist on the current list, the stack is rebuilt from the top folder down to the give folder
     * @param folder Folder to enter
     * @return This
     */
    IGamelistContainer& EnterFolder(FolderData* folder) final
    {
      if (LookupGameIndex(folder) < 0) RebuildFolderStack(folder);
      mFolderStack.Push(folder);
      return *this;
    }

    /*!
     * @brief Exit current folder, pop it from the stack, and return the new folder
     * @return New current folder
     */
    FolderData* ExitFolder() override
    {
      if (!mFolderStack.Empty()) mFolderStack.PopAndDelete();
      return !mFolderStack.Empty() ? mFolderStack.Peek() : &mSystem.MasterRoot();
    }

    /*!
     * @brief Check iof we are in the root folder or not
     * @return True if we are in the root folder, false otherwise
     */
    bool IsInRootFolder() final { return mFolderStack.Empty(); }

  private:
    //! Master index 16:16 (int:frac)
    int mIndex;
    //! Target index 16:16 (int:frac)
    int mTargetIndex;
    //! Delta (move speed)
    int mDelta;

    //! Move time accumulator
    int mTimeAccumulator;
    //! Global time accumulator
    int mStepTimeAccumulator;
    //! Timer index
    int mTimerIndex;
    //! Minimum step
    int mMinimumStep;

    //! Avoid Children
    bool mAvoidChildren;

    //! System
    SystemData& mSystem;
    //! Callback interface
    CallbackInterface* mCallbackInterface;

    //! Entries
    std::vector<Entry> mEntries;

    //! Deduplicating arena holding all display names (referenced by Entry::mNameIndex)
    GamelistNameArena mNames;
    //! Scratch buffer backing the const String& name accessors (single-threaded UI usage)
    mutable String mNameCache;

    //! Scrolling times
    Array<ScrollingTime> mScrollings;

    //! Folder stack
    Stack<FolderData*> mFolderStack;

    //! Get target index clamped to image list size
    [[nodiscard]] int TargetIndex() const
    {
      int realIndex = (mTargetIndex + 32768) >> 16;
      int entryCount = (int) mEntries.size();
      if (!mEntries.empty())
      {
        realIndex %= entryCount;
        if (realIndex < 0) realIndex += entryCount;
      }
      return realIndex;
    }

    /*!
     * @brief Make the lit move !
     */
    void Move()
    {
      if (!mEntries.empty())
      {
        if (mIndex != mTargetIndex)
        {
          int oldSlot = Index();
          int move = (mTargetIndex - mIndex) >> 2;
          if      (mTargetIndex - mIndex < 0) move = Math::min(move, -mMinimumStep);
          else if (mTargetIndex - mIndex > 0) move = Math::max(move,  mMinimumStep);
          mIndex += move;
          if (Math::absi(mTargetIndex - mIndex) < mMinimumStep) mIndex = mTargetIndex;
          if (int newSlot = Index(); newSlot != oldSlot)
          {
            if (mCallbackInterface != nullptr)
              mCallbackInterface->BaseListIndexChanged(newSlot, mEntries[newSlot].mGame);
          }
        }
        else
        {
          if (mIndex < 0)
          {
            mIndex %= (int) mEntries.size() << 16;
            if (mIndex < 0) mIndex += (int) mEntries.size() << 16;
            mTargetIndex = mIndex;
          }
          else if (mIndex > (int) mEntries.size() << 16)
          {
            mIndex %= (int) mEntries.size() << 16;
            mTargetIndex = mIndex;
          }
        }
      }
    }
};