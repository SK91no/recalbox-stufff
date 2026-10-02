//
// Created by xizor on 05/10/2019.
//
#pragma once

#include <components/ComponentList.h>
#include "components/ComponentGrid.h"
#include <components/NinePatchComponent.h>
#include <components/ButtonComponent.h>
#include <components/TextEditComponent.h>
#include <components/OptionListComponent.h>
#include <components/VideoComponent.h>
#include <themes/MenuThemeData.h>
#include "systems/SystemManager.h"
#include "SearchForcedOptions .h"
#include "components/PictureComponent.h"

class GuiSearch : public Gui
                , public IVirtualKeyboardInterface
                , private IGamelistEvents
{
  public:
    GuiSearch(WindowManager& window, SystemManager& systemManager, SystemData* system, SearchForcedOptions* forcedOptions = nullptr);
    
    ~GuiSearch() override;

    void updateSize();

    bool ProcessInput(const InputCompactEvent& event) override;

    bool CollectHelpItems(Help& help) override;

    void onSizeChanged() override;

    void Update(int deltaTime) override;

    void Render(const Transform4x4f& parentTrans) override;

    void PopulateGrid(const String& search);

    void populateGridMeta(int i);

    void launch(int index);

    void GoToGame();

    void initGridsNStuff();

    void ResizeGridLogo();

    void clear();

  private:
    PictogramCaches mCache;

    SystemManager& mSystemManager;

    //full grid (entire frame)
    ComponentGrid mGrid;
    const MenuThemeData& mMenuTheme;
    //grid for list & Meta
    std::shared_ptr<ComponentGrid> mGridMeta;
    //grid for logo + publisher and developer
    std::shared_ptr<ComponentGrid> mGridLogoAndMD;
    //3x3 grid to center logo
    std::shared_ptr<ComponentGrid> mGridLogo;
    std::shared_ptr<ComponentGrid> mButtonGrid;
    std::vector<std::shared_ptr<ButtonComponent> > mButtons;
    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mText;
    std::shared_ptr<TextEditComponent> mSearch;
    std::shared_ptr<TextComponent> mMDDeveloperLabel;
    std::shared_ptr<TextComponent> mMDDeveloper;
    std::shared_ptr<TextComponent> mMDPublisherLabel;
    std::shared_ptr<TextComponent> mMDPublisher;
    std::shared_ptr<TextGamelist> mList;
    std::shared_ptr<PictureComponent> mResultThumbnail;
    std::shared_ptr<PictureComponent> mResultSystemLogo;
    std::shared_ptr<VideoComponent> mResultVideo;
    std::shared_ptr<ScrollableContainer> mDescContainer;
    std::shared_ptr<TextComponent> mResultDesc;
    std::shared_ptr<OptionListComponent<FolderData::FastSearchContext>> mSearchChoices;
    SystemData* mSystemData;
    FileData::List mSearchResults;
    String  mForcedSearch;
    FolderData::FastSearchContext mForcedContext = FolderData::FastSearchContext::Name;
    bool mForcedOptions;

    //! Just-open flag
    bool mJustOpen;

    /*!
     * @brief Called when the edited text change.
     * Current text is available from the Text() method.
     */
    void VirtualKeyboardTextChange(IVirtualKeyboardBase& vk, const String& text) final;

    /*!
     * @brief Called when the edited text is validated (Enter or Start)
     * Current text is available from the Text() method.
     */
    void VirtualKeyboardValidated(IVirtualKeyboardBase& vk, const String& text) final;

    /*
     * IGamelistEvents implementation
     */

    /*!
     * @brief Called when the list moved onto a new game
     * @param caller Calling gamelist implementation
     * @param game Selected game
     * @param index Game index in the list
     */
    void OnGameSelected(IGamelistContainer& caller, FileData* game, int index) final { (void)caller; (void)game; populateGridMeta(index); }

    /*!
     * @brief Called when a game has been validated ("Valid" button pressed, then released)
     * @param caller Calling gamelist implementation
     * @param game Selected game
     * @param index Game index in the list
     */
    void OnGameValidated(IGamelistContainer& caller, FileData* game, int index) final { (void)caller; (void)game; launch(index); }

    /*!
     * @brief Called when a folder has been validated ("Valid" button pressed, then released)
     * @param caller Calling gamelist implementation
     * @param folder Selected folder
     * @param index Game index in the list
     */
    void OnFolderValidated(IGamelistContainer& caller, FolderData* folder, int index) final { (void)caller; (void)folder; (void)index; }

    /*!
     * @brief The user just fold the given header
     * @param caller Calling gamelist implementation
     * @param header Header to fold (selected item)
     * @param index Item index
     */
    void OnFoldHeader(IGamelistContainer& caller, HeaderData* header, int index) final { (void)caller; (void)header; (void)index; }

    /*!
     * @brief The user just unfold the given header
     * @param caller Calling gamelist implementation
     * @param header Header to fold (selected item)
     * @param index Item index
     */
    void OnUnfoldHeader(IGamelistContainer& caller, HeaderData* header, int index) final { (void)caller; (void)header; (void)index; }

    /*!
     * @brief The user just fold all header
     * @param caller Calling gamelist implementation
     */
    void OnFoldAllHeader(IGamelistContainer& caller) final { (void)caller; }

    /*!
     * @brief The user just Unfold all header
     * @param caller Calling gamelist implementation
     */
    void OnUnfoldAllHeader(IGamelistContainer& caller) final { (void)caller; }

    /*!
     * @brief The user just performed a fold action on a game
     * @param caller Calling gamelist implementation
     * @param game Game on which the fold action has been performed
     * @param index Item index
     */
    virtual void OnFoldGame(IGamelistContainer& caller, FileData* game, int index) final { (void)caller; (void)game; (void)index; }

    /*!
     * @brief The user just performed an unfold action on a game
     * @param caller Calling gamelist implementation
     * @param game Game on which the unfold action has been performed
     * @param index Item index
     */
    void OnUnfoldGame(IGamelistContainer& caller, FileData* game, int index) final { (void)caller; (void)game; (void)index; }

    /*!
     * @brief The user performed a fold all game action
     * @param caller Calling gamelist implementation
     */
    void OnFoldAllGame(IGamelistContainer& caller) final { (void)caller; }

    /*!
     * @brief The user performed an unfold all game action
     * @param caller Calling gamelist implementation
     */
    void OnUnfoldAllGame(IGamelistContainer& caller) final { (void)caller; }
};
