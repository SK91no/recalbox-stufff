//
// Created by bkg2k on 24/08/24.
//
#pragma once

#include <themes/ThemeManager.h>
#include <utils/locale/LocaleHelper.h>
#include "SelectorEntry.h"
#include "ISelectorChanged.h"
#include "ItemBase.h"
#include "MenuColors.h"
#include "NinePatchImage.h"
#include "components/TextListComponent.h"
#include "components/TextScrollComponent.h"
#include "utils/storage/BitArray.h"
#include <guis/menus/base/IMenuInterface.h>
#include <utils/cplusplus/IsPod.h>
#include <rendering/fonts/Font.h>
#include <rendering/textures/TextureManager.h>

//! Callback interface from the GUI to this item
template<typename T>class ISelectorGUIInterface
{
  public:
    //! default destructor
    virtual ~ISelectorGUIInterface() = default;

    /*!
     * @brief Item in the GUI changed
     * @param value Item value
     * @param selected Select state
     */
    virtual void ItemChanged(const T& value, bool selected) = 0;

    /*!
     * @brief Select all/Unselect all has been selected, select or unselect all
     * @param selected Tru to select all, false to unselect all
     */
    virtual void ItemChangedAll(bool selected) = 0;
};

template<typename T> class ItemSelectorBase : public ItemBase
                                            , ISelectorGUIInterface<T>
{
  public:
    // Entry shortcut
    typedef SelectorEntry<T> Entry;
    // Entry list shortcut
    typedef SelectorEntry<T>::List List;

    /*!
     * @brief Single selector constructor
     * @param parent Parent menu
     * @param dataProvider Menu data provider
     * @param window Window manager
     * @param label Item label
     * @param theme Menu theme reference
     * @param interface Single selector callback
     * @param identifier Item identifier
     * @param values Selector values
     * @param initialValue Initial value (will be selected automatically)
     * @param defaultValue Default selected value
     * @param help Help string
     * @param unselectable Grayed ?
     * @param helpUnselectable Help to display when the item is not selectable
     */
    ItemSelectorBase(Menu& parent, IMenuInterface& dataProvider, WindowManager& window, const String& label, const MenuThemeData& theme, ISingleSelectorChanged<T>* interface, int identifier, const List& values, const T& initialValue, const T& defaultValue, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemBase(Type::List, parent, dataProvider, label, theme, identifier, help, unselectable, helpUnselectable)
      , mWindow(window)
      , mButton(dataProvider.Cache().GetElement(MenuThemeData::IconElement::Type::Button, 0, 0))
      , mSingleInterface(interface)
      , mMultiInterface(nullptr)
      , mList(UppercaseOnce(values))
      , mInitialSelectedIndex(-1)
      , mLastSelectedIndex(0)
      , mCount(0)
      , mMulti(false)
      , mArrowWidth(0)
      , mArrowHeight(0)
      , mItemMaxWidth(0)
      , mMargin(Renderer::Instance().DisplayWidthAsInt() / 160)
      , mYMargin(Renderer::Instance().Is480pOrLower() ? 1 : 2)
      , mQuickSelectionTimer(0)
      , mTruncated(false)
    {
      SelectSingleValue(initialValue, defaultValue, true);
    }

    /*!
     * @brief Single selector constructor
     * @param parent Parent menu
     * @param dataProvider Menu data provider
     * @param window Window manager
     * @param label Item label
     * @param theme Menu theme reference
     * @param interface Single selector callback
     * @param identifier Item identifier
     * @param icon Icon type
     * @param values Selector values
     * @param initialValue Initial value (will be selected automatically)
     * @param defaultValue Default selected value
     * @param help Help string
     * @param unselectable Grayed ?
     * @param helpUnselectable Help to display when the item is not selectable
     */
    ItemSelectorBase(Menu& parent, IMenuInterface& dataProvider, WindowManager& window, const String& label, const MenuThemeData& theme, ISingleSelectorChanged<T>* interface, int identifier, MenuThemeData::MenuIcons::Type icon, const List& values, const T& initialValue, const T& defaultValue, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemBase(Type::List, parent, dataProvider, label, theme, identifier, icon, help, unselectable, helpUnselectable)
      , mWindow(window)
      , mButton(dataProvider.Cache().GetElement(MenuThemeData::IconElement::Type::Button, 0, 0))
      , mSingleInterface(interface)
      , mMultiInterface(nullptr)
      , mList(UppercaseOnce(values))
      , mInitialSelectedIndex(-1)
      , mLastSelectedIndex(0)
      , mCount(0)
      , mMulti(false)
      , mArrowWidth(0)
      , mArrowHeight(0)
      , mItemMaxWidth(0)
      , mMargin(Renderer::Instance().DisplayWidthAsInt() / 320)
      , mYMargin(Renderer::Instance().Is480pOrLower() ? 1 : 2)
      , mQuickSelectionTimer(0)
      , mTruncated(false)
    {
      SelectSingleValue(initialValue, defaultValue, true);
    }

    /*!
     * @brief Multiple selector constructor
     * @param parent Parent menu
     * @param dataProvider Menu data provider
     * @param window Window manager
     * @param label Item label
     * @param theme Menu theme reference
     * @param interface Multi selector callback
     * @param identifier Item identifier
     * @param values Selector values
     * @param help Help string
     * @param unselectable Grayed ?
     * @param helpUnselectable Help to display when the item is not selectable
     */
    ItemSelectorBase(Menu& parent, IMenuInterface& dataProvider, WindowManager& window, const String& label, const MenuThemeData& theme, IMultiSelectorChanged<T>* interface, int identifier, const List& values, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemBase(Type::Multi, parent, dataProvider, label, theme, identifier, help, unselectable, helpUnselectable)
      , mWindow(window)
      , mButton(dataProvider.Cache().GetElement(MenuThemeData::IconElement::Type::Button, 0, 0))
      , mSingleInterface(nullptr)
      , mMultiInterface(interface)
      , mList(UppercaseOnce(values))
      , mInitialSelectedIndexes(values.size())
      , mInitialSelectedIndex(0)
      , mLastSelectedIndex(0)
      , mCount(0)
      , mMulti(true)
      , mArrowWidth(0)
      , mArrowHeight(0)
      , mItemMaxWidth(0)
      , mMargin(Renderer::Instance().DisplayWidthAsInt() / 320)
      , mYMargin(Renderer::Instance().Is480pOrLower() ? 1 : 2)
      , mQuickSelectionTimer(0)
      , mTruncated(false)
    {
      SetInitialIndexes();
      UpdateCount();
    }

    /*!
     * @brief Multiple selector constructor
     * @param parent Parent menu
     * @param dataProvider Menu data provider
     * @param window Window manager
     * @param label Item label
     * @param theme Menu theme reference
     * @param interface Multi selector callback
     * @param identifier Item identifier
     * @param icon Icon type
     * @param values Selector values
     * @param help Help string
     * @param unselectable Grayed ?
     * @param helpUnselectable Help to display when the item is not selectable
     */
    ItemSelectorBase(Menu& parent, IMenuInterface& dataProvider, WindowManager& window, const String& label, const MenuThemeData& theme, IMultiSelectorChanged<T>* interface, int identifier, MenuThemeData::MenuIcons::Type icon, const List& values, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemBase(Type::Multi, parent, dataProvider, label, theme, identifier, icon, help, unselectable, helpUnselectable)
      , mWindow(window)
      , mButton(dataProvider.Cache().GetElement(MenuThemeData::IconElement::Type::Button, 0, 0))
      , mSingleInterface(nullptr)
      , mMultiInterface(interface)
      , mList(UppercaseOnce(values))
      , mInitialSelectedIndexes(values.size())
      , mInitialSelectedIndex(0)
      , mLastSelectedIndex(0)
      , mCount(0)
      , mMulti(true)
      , mArrowWidth(0)
      , mArrowHeight(0)
      , mItemMaxWidth(0)
      , mMargin(Renderer::Instance().DisplayWidthAsInt() / 320)
      , mYMargin(Renderer::Instance().Is480pOrLower() ? 1 : 2)
      , mQuickSelectionTimer(0)
      , mTruncated(false)
    {
      SetInitialIndexes();
      UpdateCount();
      // Empty multi-list are automatically set to disabled
      if (values.empty())
        SetSelectable(false);
    }

    //! Destructor
    ~ItemSelectorBase() override
    {
    }

    //! Flush waiting event immediately
    void Flush() final
    {
      // Fire immediately if a delayed timer is still active
      if (mQuickSelectionTimer > 0) Fire();
    }

    //! Lose focus
    void LoseFocus() final { Flush(); }

    /*
     * Accessors
     */

    //! Count for multi-lists. Always 1 for single lists
    [[nodiscard]] int SelectedCount() const { return mMulti ? mCount : 1; }
    //! Last selected index (or modified in multi-lists)
    [[nodiscard]] int SelectedIndex() const { return mLastSelectedIndex; }
    //! Last selected value (or modified in multi-lists)
    [[nodiscard]] T SelectedValue() const { return mList[mLastSelectedIndex].mValue; }
    //! Is the current selection, selected (multi-lists)
    [[nodiscard]] bool SelectedState() const { return mList[mLastSelectedIndex].mSelected; }

    //! Selected value list. Contains always one item in single lists
    [[nodiscard]] std::vector<T> AllSelectedValues() const
    {
      std::vector<T> list;
      for (const Entry& entry: mList) if (entry.mSelected) list.push_back(entry.mValue);
      return list;
    }

    //! Unselected value list. Contains always all items less one item in single lists
    [[nodiscard]] std::vector<T> AllUnselectedValues() const
    {
      std::vector<T> list;
      for (const Entry& entry: mList) if (!entry.mSelected) list.push_back(entry.mValue);
      return list;
    }

    /*!
     * @brief Rebuild the item using a new array of data
     * @param values New values
     * @param initialValue Initial value
     * @param defaultValue Default value
     */
    ItemSelectorBase<T>& ChangeSelectionItems(const List& values, const T& initialValue, const T& defaultValue)
    {
      mArrowHeight = 0; // Force graphic recalibration
      mList = UppercaseOnce(values);
      SelectSingleValue(initialValue, defaultValue, false);
      return *this;
    }

    /*!
     * @brief Rebuild the item using a new array of data. Previous selection is the new initial data
     * @param values New values
     * @param defaultValue Default value
     */
    ItemSelectorBase<T>& ChangeSelectionItems(const List& values, const T& defaultValue)
    {
      mArrowHeight = 0; // Force graphic recalibration
      T previousValue = mList[mLastSelectedIndex].mValue;
      mList = UppercaseOnce(values);
      SelectSingleValue(previousValue, defaultValue, false);
      return *this;
    }

    /*!
     * @brief Rebuild the item using a new array of data. Previous selection is the new initial data
     * and the first available data is the default
     * @param values New values
     */
    ItemSelectorBase<T>& ChangeSelectionItems(const List& values)
    {
      mArrowHeight = 0; // Force graphic recalibration
      T previousValue = mList[mLastSelectedIndex].mValue;
      mList = UppercaseOnce(values);
      T defaultValue = mList.front().mValue;
      SelectSingleValue(previousValue, defaultValue, false);
      return *this;
    }

    /*!
     * @brief Set selected item by index
     * @param index Index to set
     * @param callback True to call the interface callback
     */
    void SetSelectedItemIndex(int index, bool callback)
    {
      SelectSingleValue(mList[index % (int)mList.size()], mList[index % (int)mList.size()]);
      if (callback) Fire();
    }

    /*!
     * @briefSet selected item by value
     * @param value Value to set
     * @param callback True to call the interface callback
     */
    void SetSelectedItemValue(const T& value, bool callback)
    {
      SelectSingleValue(value, value, false);
      if (callback) Fire();
    }

    /*!
     * @brief Check if the initially selected item/items has/have changed since the initialization
     * @return True if something changed since the initialization
     */
    bool HasChanged() final
    {
      if (mMulti) return CheckInitialIndexes();
      return mLastSelectedIndex != mInitialSelectedIndex;
    }

  protected:
    /*
     * Item base implementation
     */

    /*!
     * @brief Notification of an input event
     * @param event Compact event
     * @return Implementation must return true if it consumed the event.
     */
    bool ProcessInput(const InputCompactEvent& event) final
    {
      if (mList.empty()) return false;
      if (event.ValidReleased()) ShowGUI();
      else if (!mMulti)
      {
        if (event.AnyLeftReleased()) MoveSelection(false);
        else if (event.AnyRightReleased()) MoveSelection(true);
      }
      return false;
    }

    /*!
     * @brief Called once per frame
     * @param elapsed elapsed time from last update call
     */
    void Update(int elapsed) final
    {
      if (mQuickSelectionTimer > 0)
        if (mQuickSelectionTimer -= elapsed; mQuickSelectionTimer <= 0)
        {
          mQuickSelectionTimer = 0;
          Fire();
        }
    }

    /*!
     * @brief Fill help list
     * @param help Help list to fill
     */
    void CollectHelpItems(Help& help) override
    {
      help.Set(Help::Valid(), _("SELECT"));
      if (!mMulti) help.Set(HelpType::LeftRight, _("CHANGE"));
    }

    //! Fire callback
    void Fire() final
    {
      if (mMulti)
      {
        if (mMultiInterface != nullptr)
        {
          std::vector<T> list;
          for (const Entry& entry: mList) if (entry.mSelected) list.push_back(entry.mValue);
          if (mMultiInterface != nullptr)
            mMultiInterface->MenuMultiChanged(*this, mIdentifier, mLastSelectedIndex, list);
        }
      }
      else if (mSingleInterface != nullptr && mLastSelectedIndex >= 0
               && mLastSelectedIndex < (int)mList.size()
               && mList[mLastSelectedIndex].mSelected)
      {
        /* Single mode: only one selected entry. Snapshot the value before the
         * callback because it can mutate mList (rebuilding entries / changing
         * mSelected), which would make a for-range loop re-fire spuriously. */
        T value = mList[mLastSelectedIndex].mValue;
        mSingleInterface->MenuSingleChanged(*this, mIdentifier, mLastSelectedIndex, value);
      }
    }

    //! Label changed - Force this item to recalibrate
    void LabelChanged() final { mArrowHeight = 0; }

    //! Theme has changed - reload
    void ThemeHasChanged() final { mArrowHeight = 0; }

    /*!
     * @brief Apply (draw) an overlay in the given item rectangle and adjust rectangle position/size
     * so that the text won't draw over the overlay if required
     * @param labelWidth Left label width
     * @param area item rectangle
     * @param color color
     * @param selected True if the current item is selected
     */
    void OverlayDraw(int labelWidth, const Rectangle& area, Colors::ColorRGBA color, bool selected) final
    {
      (void)selected;
      LoadArrow(labelWidth);
      int xRightArrow = area.Right() - mDataProvider.Margin() - mArrowWidth;
      int xText = xRightArrow - mMargin - mItemMaxWidth - 2;
      int xLeftArrow = xText - mMargin - mArrowWidth - 2;
      int yArrows = area.YCenter() - mArrowHeight / 2;
      mArrow.Render(xRightArrow, yArrows, mArrowWidth, mArrowHeight, true, false, color);
      mArrow.Render(xLeftArrow, yArrows, mArrowWidth, mArrowHeight, true, false, true, false, color);
      Rectangle subArea(xText - 2, mYMargin, mItemMaxWidth + 4, (int)area.Height() - mYMargin);
      mButton.Render(subArea, mDataProvider.Is240p() ? MenuColors::Alpha50Percent(color) : color);
      subArea.Contract(2, -mYMargin);
      if (mMulti)
      {
        NewFont& font = *mTheme.Text().font;
        // TODO: Store ellipsis text when mTruncated is computed
        if (mTruncated) font.DrawText(font.TextEllipsis(mSummary, mItemMaxWidth), subArea, color, Alignment::Center);
        else            font.DrawText(mSummary, subArea, color, Alignment::Center);
      }
      else
      {
        NewFont& font = *mTheme.Text().font;
        // TODO: Store ellipsis text when mTruncated is computed
        if (mTruncated) font.DrawText(font.TextEllipsis(mList[mLastSelectedIndex].mText, mItemMaxWidth), subArea, color, Alignment::Center);
        else            font.DrawText(mList[mLastSelectedIndex].mText, subArea, color, Alignment::Center);
      }
    }

    /*!
     * @brief Get the left offset (margin to the text) if any
     * @param labelWidth Left label width
     * @return left offset
     */
    float OverlayLeftOffset(int labelWidth) final { (void)labelWidth; return 0.f; }

    /*!
     * @brief Get the right offset (margin from text to right limit) if any
     * @param labelWidth Left label width
     * @return right offset
     */
    float OverlayRightOffset(int labelWidth) final
    {
      LoadArrow(labelWidth);
      return (float)(mItemMaxWidth + mArrowWidth * 2 + mMargin * 3 + mDataProvider.Margin());
    }

    //! Uppercase text once
    static List UppercaseOnce(const List& items)
    {
      List result(items);
      // Uppercase once
      for(SelectorEntry<T>& entry : result)
        entry.mText.UpperCaseUTF8();
      return result;
    }

    //! Selector GUI
    class Selector : public Gui
                   , public ITextListComponentOverlay<SelectorEntry<T>*>
    {
      public:
        Selector(WindowManager& window, IMenuInterface& dataprovider, ISelectorGUIInterface<T>& interface, const ItemBase& item, List& items, bool multi, int selectedIndex);

      private:
        //! Main component grid
        ComponentGrid mGrid;
        //! Main component grid
        std::shared_ptr<TextScrollComponent> mTitle;
        //! List component
        std::shared_ptr<TextListComponent<SelectorEntry<T>*>> mItemList;

        //! Unselected checkbox resource
        Texture mUnselected;
        //! Selected checkbox resource
        Texture mSelected;

        //! Data provider
        IMenuInterface& mDataProvider;
        //! Callback interface
        ISelectorGUIInterface<T>& mCallback;
        //! Theme menu reference
        const MenuThemeData& mTheme;
        //! Background image
        NinePatchImage mFrame;
        //! Button image
        NinePatchImage mButton;
        //! List reference from the selector
        List& mList;
        //! Checkbox width
        int mCheckBoxWidth;
        //! Checkbox height
        int mCheckBoxHeight;
        //! Multi mode?
        bool mMulti;
        //! Initialized ?
        bool mInitialized;

        /*!
         * @brief This little window displays over the menu option that called it
         * @return True if the GUI is an overlay
         */
        [[nodiscard]] virtual bool IsOverlay() const final { return true; }

        /*!
         * @brief Notification of an input event
         * @param event Compact event
         * @return Implementation must return true if it consumed the event.
         */
        bool ProcessInput(const InputCompactEvent& event) final
        {
          if (event.StartReleased()) { mWindow.CloseAll(); return true;}
          if (event.CancelReleased()) { Close(); }
          if (event.ValidReleased())
          {
            SelectorEntry<T>* entry = mItemList->getSelected();
            // Select all / Unselect all
            if (entry == nullptr)
            {
              int index = mItemList->getCursorIndex();
              mCallback.ItemChangedAll(index == 0);
              return true;
            }
            // Normal item
            if (mMulti) entry->mSelected = !entry->mSelected;
            else Close();
            mCallback.ItemChanged(entry->mValue, entry->mSelected);
            return true;
          }
          return Gui::ProcessInput(event);
        }

        /*!
         * @brief Lazy selector initialization
         * @param deltaTime Elapsed time from the previous frame, in millisecond
         */
        void Update(int deltaTime) final
        {
          Gui::Update(deltaTime);
          if (!mInitialized)
          {
            SetMenuSize();
            mInitialized = true;
          }
        }

        void Render(const Transform4x4f& parentTrans) final
        {
          Transform4x4f trans = parentTrans * getTransform();
          Renderer::SetMatrix(trans);

          Renderer& renderer = Renderer::Instance();
          int screenWidth = renderer.DisplayWidthAsInt();
          int screenHeight = renderer.DisplayHeightAsInt();
          renderer.Clip(Rectangle(0, (int)mGrid.getRowHeight(0), screenWidth, screenHeight));
          // Render background
          mFrame.Render(Rectangle(-2.f, -2.f, mSize.x() + 4, mSize.y() + 4), mTheme.Background().color);
          // Pop clipping
          renderer.Unclip();

          // Get clipping area
          renderer.Clip(Rectangle(0, 0, screenWidth, (int)mGrid.getRowHeight(0)));
          // Redraw part of the background
          mFrame.Render(Rectangle(-2.f, -2.f, mSize.x() + 4, mSize.y() + 4), (mTheme.Text().selectorColor & 0xFFFFFF00) | (mTheme.Background().color & 0xFF));
          // Pop clipping
          renderer.Unclip();

          // Render grid
          Gui::Render(parentTrans);
        }

        /*!
         * @brief Fill help list
         * @param help Help list to fill
         */
        bool CollectHelpItems(Help& help) override
        {
          if (mMulti)
            help.Set(Help::Valid(), _("TOGGLE"))
                .Set(Help::Cancel(), _("BACK"));
          else
            help.Set(Help::Valid(), _("SELECT"))
                .Set(Help::Cancel(), _("BACK"));
          return true;
        }

        //! Set menu size
        void SetMenuSize()
        {
          Rectangle areaMax(mDataProvider.GetMenuMaximumArea());
          Rectangle menu(Renderer::Instance().DisplayWidthAsFloat(),
                         (Renderer::Instance().DisplayHeightAsFloat() - MenuHeight()) / 2.f,
                         MenuWidth(), MenuHeight());
          // Constraint
          if (menu.Right() > areaMax.Right()) menu.Move(areaMax.Right() - menu.Right(), 0);
          if (menu.Left() < areaMax.Left()) menu.MoveLeft(areaMax.Left() - menu.Left());
          if (menu.Bottom() > areaMax.Bottom()) menu.Move(0, areaMax.Bottom() - menu.Bottom());
          if (menu.Top() < areaMax.Top()) menu.MoveTop(areaMax.Top() - menu.Top());
          menu.Round();
          // Set
          setSize(menu.Width(), menu.Height());
          setPosition(menu.Left(), menu.Top());
          mWindow.UpdateHelpSystem();
        }

        //! Get title height
        float TitleHeight() const
        {
          float padding = (Renderer::Instance().DisplayHeightAsFloat() * 0.03f);
          return mTheme.Text().font->Height() + padding;
        }

        /// Calculate menu height
        float MenuHeight() const
        {
          const float maxHeight = Renderer::Instance().DisplayHeightAsFloat() * 0.9f;
          float baseHeight = TitleHeight() + 8;
          int maxRows = (int)(maxHeight - baseHeight) / (int)mItemList->EntryHeight();
          return baseHeight + (float)(Math::min((int)mItemList->size(), maxRows)) * mItemList->EntryHeight();
        }

        //! Get fixed selector menu width
        float MenuWidth() const
        {
          int maxWidth = 0;
          for(const SelectorEntry<T>& entry : mList)
            if (int w = mTheme.SmallText().font->TextWidth(entry.mText); w > maxWidth) maxWidth = w;
          if (int w = mTheme.Text().font->TextWidth(mTitle->getValue()); w > maxWidth) maxWidth = w;
          maxWidth += mDataProvider.Margin() * 2;
          if (maxWidth > Renderer::Instance().DisplayWidthAsInt() / 2)
            maxWidth = Renderer::Instance().DisplayWidthAsInt() / 2;
          if (mMulti) maxWidth += mDataProvider.Margin() + mCheckBoxWidth;
          return (float)maxWidth;
        }

        //! Adjust sizes
        void onSizeChanged() override;

        /*
         * Overlay interface implementation
         */

        /*!
         * @brief Apply (draw) an overlay in the given item rectangle and adjust rectangle position/size
         * so that the text won't draw over the overlay if required
         * @param parentTrans Parent transform matrice
         * @param leftOverrideWidth Left overlay width
         * @param rightOverrideWidth Right overlay width
         * @param labelWidth Cell's label width in pixel
         * @param area item rectangle
         * @param data Linked data
         * @param index item index
         * @param color draw color
         */
        void OverlayApply(const Transform4x4f& parentTrans, int leftOverrideWidth, int rightOverrideWidth, int labelWidth, const Rectangle& area, SelectorEntry<T>* const& data, int index, unsigned int& color) override
        {
          (void)parentTrans;
          (void)leftOverrideWidth;
          (void)rightOverrideWidth;
          (void)labelWidth;
          (void)color;
          if (mMulti)
          {
            if (data != nullptr)
            {
              Texture& selected = (data->mSelected ? mSelected : mUnselected);
              selected.Render(mDataProvider.Margin(), (area.Height() - mCheckBoxHeight) / 2, mCheckBoxWidth, mCheckBoxHeight, true, false,
                              mItemList->getSelected() == data ? mTheme.Background().color : mTheme.Text().color);
            }
            else
              mButton.Render(Rectangle(area).Contract(mDataProvider.Margin() * 2, 1),
                             mItemList->getCursorIndex() == index ? mTheme.Background().color : mTheme.Text().color);
          }
        }

        /*!
         * @brief Get the left offset (margin to the text) if any
         * @param data Linked data
         * @param labelWidth Left label width
         * @return left offset
         */
        float OverlayGetLeftOffset(SelectorEntry<T>* const& data, int labelWidth) override
        {
          (void)data;
          (void)labelWidth;
          if (mMulti)
          {
            // Lazy resources load
            if (mCheckBoxHeight == 0)
            {
              mCheckBoxHeight = mTheme.SmallText().font->Height();
              if (!mUnselected.Valid()) mUnselected = mDataProvider.Cache().GetElement(MenuThemeData::IconElement::Type::CheckBox, 0, mCheckBoxHeight);
              if (!mSelected.Valid()) mSelected = mDataProvider.Cache().GetElement(MenuThemeData::IconElement::Type::CheckBoxFilled, 0, mCheckBoxHeight);
              mCheckBoxWidth = (mSelected.Width() * mCheckBoxHeight) / mSelected.Height();
            }
            return mDataProvider.Margin() * 2 + mCheckBoxWidth;
          }
          return mDataProvider.Margin();
        }

        /*!
         * @brief Get the right offset (margin from text to right limit) if any
         * @param data Linked data
         * @param labelWidth Left label width
         * @return right offset
         */
        float OverlayGetRightOffset(SelectorEntry<T>* const& data, int labelWidth) override
        {
          (void)data;
          (void)labelWidth;
          return mDataProvider.Margin();
        }
    };

    //! Apply change timer
    static constexpr int sQuickChangeTimer = 750;

    //! Windows manager reference
    WindowManager& mWindow;

    //! Left arrow resource
    Texture mArrow;
    //! Ninepatch empty button
    NinePatchImage mButton;

    //! Single interface
    ISingleSelectorChanged<T>* mSingleInterface;
    //! Multiple interface
    IMultiSelectorChanged<T>* mMultiInterface;

    //! Value list
    List mList;
    //! Multi-selector summary cache
    String mSummary;
    //! Initial selected indexes (multi only)
    BitArray mInitialSelectedIndexes;
    //! Initial selected index (no multi only)
    int mInitialSelectedIndex;
    //! Last changed/selected index
    int mLastSelectedIndex;
    //! Selected value count in multi mode
    int mCount;
    //! List mode
    bool mMulti;
    //! Arrow width
    int mArrowWidth;
    //! Arrow height
    int mArrowHeight;
    //! Maximum item's text length
    int mItemMaxWidth;
    //! Margin between text and arrows
    int mMargin;
    //! Y margin
    int mYMargin;
    //! Quick selection timer
    int mQuickSelectionTimer;
    //! Pössible truncated text
    bool mTruncated;

    //! Load Arrow
    void LoadArrow(int labelWidth)
    {
      if (mArrowHeight != 0) return;

      // Load arrow image
      mArrowHeight = mDataProvider.Is240p() ? mDataProvider.ItemHeight() - 4 : mDataProvider.FontHeight();
      mArrow = mDataProvider.Cache().GetElement(MenuThemeData::IconElement::Type::OptionArrow, 0, mArrowHeight);
      mArrowWidth = (mArrow.Width() * mArrowHeight) / mArrow.Height();

      // Calculate items' length
      int width = 0;
      if (!mMulti)
      {
        NewFont& font = *mTheme.Text().font;
        for (const SelectorEntry<T>& entry: mList)
          if (int newWidth = (int)font.TextWidth(entry.mText); newWidth > width)
            width = newWidth;
      }
      else
      {
        int maxChar = (mList.empty() ? 0 : (int)log10((float)mList.size())) + 1;
        width = mTheme.Text().font->TextWidth(String('0', maxChar).Append('/').Append('0', maxChar).Append(' ').Append(_("SELECTED")));
      }
      width += mDataProvider.Margin() * 2;

      // Add spacers & arrows width
      int decoWidth = mArrowWidth * 2 + mMargin * 3 + 4 /* for button drawing */;

      // Constraint ?
      mItemMaxWidth = width;
      mTruncated = CalibrateLeftAndRightParts(labelWidth, mItemMaxWidth, decoWidth);
    }

    /*!
     * @brief Ensure the given value is the only one selected in the list
     * @param initial Initial value to initialize to selected value
     * @param defaultValue Default value if the initial value is not found ihn the list.
     * @param setInitialIndex True to record Initial index
     * @
     * if the default value is neither found, LOG an error
     */
    void SelectSingleValue(const T& initial, const T& defaultValue, bool setInitialIndex)
    {
      for(Entry& entry : mList) entry.mSelected = false;
      bool found = false;
      for(int i = (int)mList.size(); --i >= 0; ) if (Entry& entry = mList[i]; entry.mValue == initial) { entry.mSelected = found = true; mLastSelectedIndex = i; if (setInitialIndex) mInitialSelectedIndex = i; break; }
      if (!found)
        for(int i = (int)mList.size(); --i >= 0; ) if (Entry& entry = mList[i]; entry.mValue == defaultValue) { entry.mSelected = found = true; mLastSelectedIndex = i; if (setInitialIndex) mInitialSelectedIndex = i; break; }
      if (!found)
      {
        LOG(LogError) << "[Menu:ItemSelector] Neither initial nor default value were found in item " << Label();
        if (!mList.empty()) mList.front().mSelected = true;
        mLastSelectedIndex = 0;
      }
    }

    void MoveSelection(bool forward)
    {
      if (mMulti) return;
      for(int i = (int)mList.size(); --i >= 0; )
        if (Entry& entry = mList[i]; entry.mSelected)
        {
          int index = ((i + (forward ? 1 : -1)) + (int)mList.size()) % (int)mList.size();
          entry.mSelected = false;
          mList[mLastSelectedIndex = index].mSelected = true;
          //ItemChanged(mList[index].mValue, true);
          mQuickSelectionTimer = sQuickChangeTimer;
          break;
        }
    }

    /*!
     * @brief Show GUI
     */
    void ShowGUI()
    {
      Selector* selector = new Selector(mWindow, mDataProvider, *this, *this, mList, mMulti, mLastSelectedIndex);
      mWindow.pushGui(selector);
    }

    /*!
     * @brief Update selected item counter
     */
    void UpdateCount()
    {
      mCount = 0;
      for(const Entry& entry : mList) if (entry.mSelected) mCount++;
      mSummary = String(mCount).Append('/').Append((int)mList.size()).Append(' ').Append(_("SELECTED"));
    }

    void SetInitialIndexes()
    {
      if (!mMulti) return;
      mInitialSelectedIndexes.UnsetAll();
      for(int i = (int)mList.size(); --i >= 0; )
        if (mList[i].mSelected)
          mInitialSelectedIndexes.Set(i);
    }

    /*!
     * @brief Compare current selected state with the initial selections
     * @return True if all selected states matches initial selected states of every single item
     * false otherwise
     */
    bool CheckInitialIndexes()
    {
      if (!mMulti) return false;
      for(int i = (int)mList.size(); --i >= 0; )
        if (const Entry& entry = mList[i]; entry.mSelected != mInitialSelectedIndexes.IsSet(i))
          return false;
      return true;
    }

    /*
     * ISelectorGUIChangeCallback
     */

    /*!
     * @brief  Callback from the GUI when a change occurred
     * @param value Value selected
     * @param selected Selected state in multimode, otherwise always true
     */
    void ItemChanged(const T& value, bool selected) final
    {
      // Select new item
      for(int i = (int)mList.size(); --i >= 0;)
      {
        Entry& entry = mList[i];
        if (!mMulti) entry.mSelected = false;
        if (entry.mValue == value)
        {
          entry.mSelected = mMulti ? selected : true;
          mLastSelectedIndex = i;
        }
      }
      // Count
      if (mMulti) UpdateCount();
      // Callback
      Fire();
    }

    /*!
     * @brief Select all/Unselect all has been selected, select or unselect all
     * @param selected Tru to select all, false to unselect all
     */
    void ItemChangedAll(bool selected) final
    {
      if (!mMulti) return;
      // Select/Unselect all
      for(Entry& entry : mList)
        entry.mSelected = selected;
      // Count
      UpdateCount();
      // Callback
      mLastSelectedIndex = -1;
      Fire();
    }
};

template<typename T> ItemSelectorBase<T>::Selector::Selector(WindowManager& window, IMenuInterface& dataprovider, ISelectorGUIInterface<T>& interface, const ItemBase& item, ItemSelectorBase::List& items, bool multi, int selectedIndex)
  : Gui(window)
  , mGrid(window, Vector2i(1, 3))
  , mDataProvider(dataprovider)
  , mCallback(interface)
  , mTheme(ThemeManager::Instance().Menu())
  , mFrame(TextureManager::Instance().Create(mTheme.Background().path, TextureHolder::Properties::None))
  , mButton(mDataProvider.Cache().GetElement(MenuThemeData::IconElement::Type::Button, 0, 0))
  , mList(items)
  , mCheckBoxWidth(0)
  , mCheckBoxHeight(0)
  , mMulti(multi)
  , mInitialized(false)
{
  addChild(&mGrid);

  // set up title
  mTitle = std::make_shared<TextScrollComponent>(mWindow);
  mTitle->setAlignment(Alignment::Center);
  mTitle->setText(String(' ').Append(item.Label()).Append(' ').ToUpperCaseUTF8());
  mTitle->setFont(mTheme.Text().font);
  mTitle->setColor(mTheme.Background().color);
  mGrid.setEntry(mTitle, Vector2i(0, 0), false, true);

  // set up list
  mItemList = std::make_shared<TextListComponent<SelectorEntry<T>*>>(mWindow);
  mItemList->SetOverlayInterface(this);
  mItemList->setUppercase(false);
  mItemList->setFont(mTheme.SmallText().font);
  mItemList->setSelectedColor(0); // If defined, it's given priority on color shift
  mItemList->setSelectorColor(mTheme.Text().selectorColor);
  mItemList->setColorAt(MenuColors::sSelectableColor, mTheme.Text().color);                                               // Text color
  mItemList->setColorAt(MenuColors::sSelectableSelectedColor, mTheme.Text().selectedColor);                               // selected text color
  mItemList->setColorAt(MenuColors::sUnselectableColor, MenuColors::Alpha25Percent(mTheme.Text().color));                 // Grayed color
  mItemList->setColorAt(MenuColors::sUnselectableSelectedColor, MenuColors::Alpha25Percent(mTheme.Text().selectedColor)); // Grayed selected color
  mItemList->setColorAt(MenuColors::sHeaderColor, mTheme.Section().color);                                                // Header color
  mItemList->setColorAt(MenuColors::sHeaderSelectedColor, mTheme.Section().selectedColor);                                // Header selected color
  mItemList->setSelectorHeight(mItemList->EntryHeight());
  mItemList->setShiftSelectedTextColor(true);
  mItemList->setAutoAlternate(true);
  mItemList->setAlignment(HorizontalAlignment::Left);
  mItemList->setCursorChangedCallback([this](CursorState state) { if (state == CursorState::Stopped) mWindow.UpdateHelpSystem(); });
  mItemList->setUseL1R1(true);
  mGrid.setEntry(mItemList, Vector2i(0, 1), true);

  // Fill selector
  if (mMulti)
  {
    mItemList->add(_("SELECT ALL"), nullptr, 0);
    mItemList->add(_("SELECT NONE"), nullptr, 0);
  }
  for(SelectorEntry<T>& entry : mList)
    mItemList->add(entry.mText, &entry, 0);
  mItemList->setCursorIndex(mMulti ? 2 : selectedIndex);

  mGrid.resetCursor();
}

template<typename T> void ItemSelectorBase<T>::Selector::onSizeChanged()
{
  // update grid row/col sizes
  mGrid.setRowHeightPerc(0, TitleHeight() / mSize.y());
  mGrid.setRowHeightPerc(1, 0);
  mGrid.setRowHeightPerc(2, 0.02f);
  mGrid.setColWidthPerc(0, 0);

  mGrid.setSize(mSize);
}

/*
 * Specialized template for POD & non-POD objects
 */

template<typename T>
requires IsNotPod<T>
class ItemSelector<T> : public ItemSelectorBase<T>
{
  // Non-POD object are already managed in the base class, only constructor must be rerouted
  public:
    /*!
     * @brief Single selector constructor
     * @param parent Parent menu
     * @param dataProvider Menu data provider
     * @param window Window manager
     * @param label Item label
     * @param theme Menu theme reference
     * @param interface Single selector callback
     * @param identifier Item identifier
     * @param values Selector values
     * @param initialValue Initial value (will be selected automatically)
     * @param defaultValue Default selected value
     * @param help Help string
     * @param unselectable Grayed ?
     * @param helpUnselectable Help to display when the item is not selectable
     */
    ItemSelector(Menu& parent, IMenuInterface& dataProvider, WindowManager& window, const String& label, const MenuThemeData& theme, ISingleSelectorChanged<T>* interface, int identifier, const ItemSelectorBase<T>::List& values, const T& initialValue, const T& defaultValue, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemSelectorBase<T>(parent, dataProvider, window, label, theme, interface, identifier, values, initialValue, defaultValue, help, unselectable,helpUnselectable)
    {}

    /*!
     * @brief Single selector constructor
     * @param parent Parent menu
     * @param dataProvider Menu data provider
     * @param window Window manager
     * @param label Item label
     * @param theme Menu theme reference
     * @param interface Single selector callback
     * @param identifier Item identifier
     * @param icon Icon type
     * @param values Selector values
     * @param initialValue Initial value (will be selected automatically)
     * @param defaultValue Default selected value
     * @param help Help string
     * @param unselectable Grayed ?
     * @param helpUnselectable Help to display when the item is not selectable
     */
    ItemSelector(Menu& parent, IMenuInterface& dataProvider, WindowManager& window, const String& label, const MenuThemeData& theme, ISingleSelectorChanged<T>* interface, int identifier, MenuThemeData::MenuIcons::Type icon, const ItemSelectorBase<T>::List& values, const T& initialValue, const T& defaultValue, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemSelectorBase<T>(parent, dataProvider, window, label, theme, interface, icon, identifier, values, initialValue, defaultValue, help, unselectable,helpUnselectable)
    {}

    /*!
     * @brief Multiple selector constructor
     * @param parent Parent menu
     * @param dataProvider Menu data provider
     * @param window Window manager
     * @param label Item label
     * @param theme Menu theme reference
     * @param interface Multi selector callback
     * @param identifier Item identifier
     * @param values Selector values
     * @param help Help string
     * @param unselectable Grayed ?
     * @param helpUnselectable Help to display when the item is not selectable
     */
    ItemSelector(Menu& parent, IMenuInterface& dataProvider, WindowManager& window, const String& label, const MenuThemeData& theme, IMultiSelectorChanged<T>* interface, int identifier, const ItemSelectorBase<T>::List& values, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemSelectorBase<T>(parent, dataProvider, window, label, theme, interface, identifier, values, help, unselectable,helpUnselectable)
    {}

    /*!
     * @brief Multiple selector constructor
     * @param parent Parent menu
     * @param dataProvider Menu data provider
     * @param window Window manager
     * @param label Item label
     * @param theme Menu theme reference
     * @param interface Multi selector callback
     * @param identifier Item identifier
     * @param icon Icon type
     * @param values Selector values
     * @param help Help string
     * @param unselectable Grayed ?
     * @param helpUnselectable Help to display when the item is not selectable
     */
    ItemSelector(Menu& parent, IMenuInterface& dataProvider, WindowManager& window, const String& label, const MenuThemeData& theme, IMultiSelectorChanged<T>* interface, int identifier, MenuThemeData::MenuIcons::Type icon, const ItemSelectorBase<T>::List& values, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemSelectorBase<T>(parent, dataProvider, window, label, theme, interface, icon, identifier, values, help, unselectable,helpUnselectable)
    {
    }
};

template<typename T>
requires IsPod<T>
class ItemSelector<T> : public ItemSelectorBase<T>
{
  public:
    /*!
     * @brief Single selector constructor
     * @param parent Parent menu
     * @param dataProvider Menu data provider
     * @param window Window manager
     * @param label Item label
     * @param theme Menu theme reference
     * @param interface Single selector callback
     * @param identifier Item identifier
     * @param values Selector values
     * @param initialValue Initial value (will be selected automatically)
     * @param defaultValue Default selected value
     * @param help Help string
     * @param unselectable Grayed ?
     * @param helpUnselectable Help to display when the item is not selectable
     */
    ItemSelector(Menu& parent, IMenuInterface& dataProvider, WindowManager& window, const String& label, const MenuThemeData& theme, ISingleSelectorChanged<T>* interface, int identifier, const ItemSelectorBase<T>::List& values, const T& initialValue, const T& defaultValue, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemSelectorBase<T>(parent, dataProvider, window, label, theme, interface, identifier, values, initialValue, defaultValue, help, unselectable,helpUnselectable)
    {}

    /*!
     * @brief Single selector constructor
     * @param parent Parent menu
     * @param dataProvider Menu data provider
     * @param window Window manager
     * @param label Item label
     * @param theme Menu theme reference
     * @param interface Single selector callback
     * @param identifier Item identifier
     * @param icon Icon type
     * @param values Selector values
     * @param initialValue Initial value (will be selected automatically)
     * @param defaultValue Default selected value
     * @param help Help string
     * @param unselectable Grayed ?
     * @param helpUnselectable Help to display when the item is not selectable
     */
    ItemSelector(Menu& parent, IMenuInterface& dataProvider, WindowManager& window, const String& label, const MenuThemeData& theme, ISingleSelectorChanged<T>* interface, int identifier, MenuThemeData::MenuIcons::Type icon, const ItemSelectorBase<T>::List& values, const T& initialValue, const T& defaultValue, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemSelectorBase<T>(parent, dataProvider, window, label, theme, interface, icon, identifier, values, initialValue, defaultValue, help, unselectable,helpUnselectable)
    {}

    /*!
     * @brief Multiple selector constructor
     * @param parent Parent menu
     * @param dataProvider Menu data provider
     * @param window Window manager
     * @param label Item label
     * @param theme Menu theme reference
     * @param interface Multi selector callback
     * @param identifier Item identifier
     * @param values Selector values
     * @param help Help string
     * @param unselectable Grayed ?
     * @param helpUnselectable Help to display when the item is not selectable
     */
    ItemSelector(Menu& parent, IMenuInterface& dataProvider, WindowManager& window, const String& label, const MenuThemeData& theme, IMultiSelectorChanged<T>* interface, int identifier, const ItemSelectorBase<T>::List& values, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemSelectorBase<T>(parent, dataProvider, window, label, theme, interface, identifier, values, help, unselectable,helpUnselectable)
    {}

    /*!
     * @brief Multiple selector constructor
     * @param parent Parent menu
     * @param dataProvider Menu data provider
     * @param window Window manager
     * @param label Item label
     * @param theme Menu theme reference
     * @param interface Multi selector callback
     * @param identifier Item identifier
     * @param icon Icon type
     * @param values Selector values
     * @param help Help string
     * @param unselectable Grayed ?
     * @param helpUnselectable Help to display when the item is not selectable
     */
    ItemSelector(Menu& parent, IMenuInterface& dataProvider, WindowManager& window, const String& label, const MenuThemeData& theme, IMultiSelectorChanged<T>* interface, int identifier, MenuThemeData::MenuIcons::Type icon, const ItemSelectorBase<T>::List& values, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemSelectorBase<T>(parent, dataProvider, window, label, theme, interface, icon, identifier, values, help, unselectable,helpUnselectable)
    {
    }

    // POD object can be managed via Array class
    [[nodiscard]] Array<T> AllSelectedValuesAsArray() const
    {
      int count = ItemSelectorBase<T>::SelectedCount();
      Array<T> list(count);
      for (const typename ItemSelectorBase<T>::Entry& entry: ItemSelectorBase<T>::mList) if (entry.mSelected) list.Add(entry.mValue);
      return list;
    }
};

