//
// Created by bkg2k on 20/03/25.
//
#pragma once

#include "guis/Gui.h"
#include "QuickJumpItemHolder.h"
#include "components/TextListComponent.h"
#include "components/TextScrollComponent.h"
#include "components/NinePatchComponent.h"
#include "components/ComponentGrid.h"
#include "themes/MenuThemeData.h"
#include "IQuickJumpTracker.h"

class GuiQuickJump : public Gui
                   , private IThemeSwitchable
                   , public ITextListComponentOverlay<QuickJumpItemHolder::TrackedItem*>
                   , public ITextListComponentInterface<QuickJumpItemHolder::TrackedItem*>
{
  public:
    explicit GuiQuickJump(WindowManager& window, QuickJumpItemHolder& trackedItems, IQuickJumpTracker& quickJumpInterface);

  private:
    //! Menu theme
    const MenuThemeData& mTheme;
    //! Configuration reference
    RecalboxConf& mConf;
    //! Tracked items reference
    QuickJumpItemHolder& mTrackedItems;
    //! Quick jump callback interface
    IQuickJumpTracker& mInterface;

    //! Main component grid
    ComponentGrid mGrid;
    //! list
    std::shared_ptr<TextListComponent<QuickJumpItemHolder::TrackedItem*>> mList;
    //! Title scroll component
    std::shared_ptr<TextScrollComponent> mTitleScroll;
    //! Title font
    NewFont* mTitleFont;

    //! List initialized ?
    bool mListInitialized;

    //! Build list items
    void BuildList(QuickJumpItemHolder::TrackedItem* forceItem);

    //§ Build items recursively from the given node
    void BuildListItems(QuickJumpItemHolder::TrackedItem& from, QuickJumpItemHolder::TrackedItem* forceItem);

    /*!
     * @brief Apply theme on the current List
     */
    void ApplyTheme();

    //! Return title height
    [[nodiscard]] float TitleHeight() const;

    //! Return List Width
    [[nodiscard]] float ListWidth() const;

    //! Return List Height
    [[nodiscard]] float ListHeight() const;

    //! Set List size & position
    void SetListSize();

    //! Resize inner components when this component is resized
    void onSizeChanged() override;

    /*
     * Overlay interface implementation
     */

    /*!
     * @brief Apply (draw) an overlay in the given item rectangle and adjust rectangle position/size
     * so that the text won't draw over the overlay if required
     * @param parentTrans Parent transform matrice
     * @param leftWidth Left overlay width
     * @param rightWidth Right overlay width
     * @param labelWidth Cell's label width in pixel
     * @param area item rectangle
     * @param data Linked data
     * @param color draw color
     */
    void OverlayApply(const Transform4x4f& parentTrans, int leftWidth, int rightWidth, int labelWidth, const Rectangle& area, QuickJumpItemHolder::TrackedItem* const& data, int index, unsigned int& color) override;

    /*!
     * @brief Get the left offset (margin to the text) if any
     * @param data Linked data
     * @return left offset
     */
    float OverlayGetLeftOffset(QuickJumpItemHolder::TrackedItem* const& data, int labelWidth) override;

    /*!
     * @brief Get the right offset (margin from text to right limit) if any
     * @param data Linked data
     * @return right offset
     */
    float OverlayGetRightOffset(QuickJumpItemHolder::TrackedItem* const& data, int labelWidth) override;

    /*
     * IThemeSwitchable implementation
     */

    /*!
     * @brief Implementation instruct the Theme switcher to use a specific system theme
     * Returning nullptr means the global theme will be used
     * @return SystemData or nullptr
     */
    [[nodiscard]] SystemData* SystemTheme() const final { return nullptr; };

    /*!
     * @brief Called when theme switch.
     * @param theme New Theme
     * @param refreshOnly True if the theme dit not change and the implementation must refresh only (i.e: apply new values)
     */
    void SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface) final;

    /*
     * Component overriding
     */

    /*!
     * @brief Proces input
     * @param event Inpuit event
     * @return True if the input has been processed
     */
    bool ProcessInput(const InputCompactEvent& event) override;

    /*!
     * @brief Called once per frame. Override to implement your own drawings.
     * Call your base::Update() to ensure animation and children are updated properly
     * @param deltaTime Elapsed time from the previous frame, in millisecond
     */
    void Update(int deltaTime) override
    {
      Gui::Update(deltaTime);
      if (!mListInitialized)
      {
        BuildList(nullptr);
        SetListSize();
        mListInitialized = true;
      }
    }

    /*!
     * @brief Collect help item
     * @param help Help object to fill
     * @return True if the Help object have been filled or modified
     */
    bool CollectHelpItems(Help& help) override;

    /*
     * ITextListComponentInterface<QuickJumpItemHolder::TrackedItem*> implementation
     */

    /*!
     * @brief Called when the cursor move to a new position
     * @param index New position index
     * @param data New position user data
     */
    void OnCursorChanged(int index, QuickJumpItemHolder::TrackedItem* data) final { (void)index; mInterface.QuickJumpTo(data->Item()); }
};
