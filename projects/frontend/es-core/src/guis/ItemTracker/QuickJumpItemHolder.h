//
// Created by bkg2k on 20/03/25.
//
#pragma once

#include <vector>
#include <games/FileData.h>
#include <rendering/fonts/Font.h>

class QuickJumpItemHolder
{
  public:
    //! Deepness
    enum class ItemDeepness
    {
      Level0, //!< Root members
      Level1, //!< Level one
      Level2, //!< Level two
    };

    //! Single tracked item
    struct TrackedItem
    {
      public:
        //! List of ourselves
        typedef std::vector<TrackedItem> List;
        //! Item Constructor
        explicit TrackedItem(FileData* item, const String& displayAs, ItemDeepness deepness)
          : mDisplay(displayAs)
          , mItem(item)
          , mDeepness(deepness)
          , mInitialItem(false)
          , mFolded(true)
        {}

        //! Set initial opened item
        void SetInitial() { mInitialItem = true; }
        //! Set folded/unfolded status
        void SetFolded(bool folded) { mFolded = folded; }
        //! Toggle folded/unfolded status
        void ToggleFolded() { mFolded = !mFolded; }

        /*
         * Accessors
         */

        //! Get children
        List& Children() { return mChildren; }
        [[nodiscard]] const List& Children() const { return mChildren; }
        //! Get item
        [[nodiscard]] FileData* Item() const { return mItem; }
        //! Get display
        [[nodiscard]] const String& Display() const { return mDisplay; }
        //! Get Deepness
        [[nodiscard]] ItemDeepness Deepness() const { return mDeepness; }
        //! Get Initial item flag
        [[nodiscard]] bool IsInitialItem() const { return mInitialItem; }
        //! Get folded status
        [[nodiscard]] bool IsFolded() const { return mFolded; }

      private:
        List mChildren;     //!< Children
        String mDisplay;    //!< Raw display
        FileData* mItem;    //!< Tracked item
        ItemDeepness mDeepness; //!< Item deepness
        bool mInitialItem;  //!< This item has been flagged as initial item
        bool mFolded;       //!< Folded/Unfolded status
    };

    //! Constructor
    QuickJumpItemHolder()
      : mRoot(nullptr, String::Empty, ItemDeepness::Level0)
      , mSelectionToTrack(nullptr)
      , mFirstInitial(0)
      , mSecondInitial(0)
      , mHasHeader(false)
    {}

    //! Get root
    TrackedItem& GetRoot() { return mRoot; }

    //! Start adding session
    void Initialize(FileData* selectedItem)
    {
      mRoot.Children().clear();
      mSelectionToTrack = selectedItem;
      mFirstInitial = mSecondInitial = 0;
      mHasHeader = false;
    }

    void Finalize()
    {
      Sanitize(mRoot);
      UnfoldSelectionTree(mRoot);
    }

    /*!
     * @brief Add candidate to the list. Only first candidate with first & second initials are added to the tree
     * @param displayAs Display name n/o icon or decoration
     * @param item Item to check
     */
    void AddCandidate(const String& displayAs, FileData* item)
    {
      assert(!displayAs.empty() && "displayedAs cannot be empty !");
      // If the new file is a header, then it is a 1st level item
      if (item->IsHeader())
      {
        mHasHeader = true; // next item must be inserted from level 1
        mFirstInitial = mSecondInitial = 0;
        Add(&mRoot, item, displayAs, ItemDeepness::Level0);
      }
      else
      {
        int position = 0;
        String::Unicode c = String::UnicodeUppercase(displayAs.ReadUTF8(position));
        if (c != mFirstInitial)
        {
          mFirstInitial = c; mSecondInitial = 0;
          Add(mHasHeader ? &mRoot.Children().back() : &mRoot, item,
              String().AssignUTF8(c).AppendUTF8(0x2026),
              mHasHeader ? ItemDeepness::Level1 : ItemDeepness::Level0);
        }
        if (c = displayAs.ReadUTF8(position); c != mSecondInitial)
        {
          mSecondInitial = c;
          Add(mHasHeader ? &mRoot.Children().back().Children().back() : &mRoot.Children().back(), item,
              String().AssignUTF8(mFirstInitial).AppendUTF8(c).AppendUTF8(0x2026),
              mHasHeader ? ItemDeepness::Level2 : ItemDeepness::Level1);
        }
      }
    }

    /*!
     * @brief Get largest text size, regarding the given font
     * @param font Font from which to calculate text size
     */
    int GetLargestTextWidth(NewFont& font) const { return LargestTextWidth(font, mRoot); }

    //! Get total item count
    [[nodiscard]] int TotalItemCount() const { return ItemCount(mRoot); }

  private:
    //! Single root
    TrackedItem mRoot;
    //! Current selection from list
    FileData* mSelectionToTrack;
    //! Last initial
    String::Unicode mFirstInitial;
    //! Last second level initial
    String::Unicode mSecondInitial;
    //! Header has been inserted?
    bool mHasHeader;

    /*!
     * @brief Recursively delete 2nd level lists w/ only one child
     * @param parent root structure
     */
    void Sanitize(TrackedItem& root)
    {
      bool isInitial = false;
      for(TrackedItem& item : root.Children()) { isInitial |= item.IsInitialItem(); Sanitize(item); }
      if (root.Item() != nullptr)
        if (!root.Item()->IsHeader())        // If parent is a game/folder
          if (root.Children().size() == 1)   // and has only one 2nd level child
          {
            root.Children().clear();         // then delete unique child
            if (isInitial) root.SetInitial();// ...and report initial state to the parent if required
          }
    }

    /*!
     * @brief Get largest text size recursively
     * @param font Font from which to calculate text size
     * @param root root node
     * @return Largest text size
     */
    int LargestTextWidth(NewFont& font, const TrackedItem& root) const
    {
      int largest = root.Item() == nullptr ? 0 : font.TextWidth(root.Display());
      for(const TrackedItem& item : root.Children())
        largest = Math::max(largest, LargestTextWidth(font, item));
      return largest;
    }

    /*!
     * @brief Locate intitial item and unfold all immediate parents
     * @param root Root node
     * @return True if the initial value has been located and unfolded
     */
    bool UnfoldSelectionTree(TrackedItem& root)
    {
      for(TrackedItem& item : root.Children())
        if (item.IsInitialItem()) { if (!item.Children().empty()) item.SetFolded(false); return true; } // Selection has children? unfold it
        else if (UnfoldSelectionTree(item)) { item.SetFolded(false); return true; } // Unfold direct ancestors
      return false;
    }

    /*!
     * @brief Get item count recursively
     * @param root Node from which to count items
     * @return Total item count
     */
    [[nodiscard]] int ItemCount(const TrackedItem& root) const
    {
      int count = (int)root.Children().size();
      for(const TrackedItem& item : root.Children())
        count += ItemCount(item);
      return count;
    }

    /*!
     * @brief Add an item in the Quiwk jump tree
     * @param parent Parent in which to add new child
     * @param item associated FileData
     * @param display Display name
     * @param deepness Deepness
     */
    void Add(TrackedItem* parent, FileData* item, const String& display, ItemDeepness deepness)
    {
      // Insert only if a valid parent has been set, otherwise both initials match the previous item
      // and we are juste ignoring the item
      if (parent != nullptr) parent->Children().push_back(TrackedItem(item, display.ToUpperCaseUTF8(), deepness));
      // If we encountered the initial item, set the latest item to initial value
      if (item == mSelectionToTrack)
      {
        parent = &mRoot.Children().back();
        while(!parent->Children().empty()) parent = &parent->Children().back();
        parent->SetInitial();
      }
    }
};