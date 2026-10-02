#pragma once

#include <guis/Gui.h>
#include <components/ImageComponent.h>
#include <components/TextComponent.h>
#include "themes/ThemeExtras.h"
#include "IArcadeGamelistInterface.h"
#include "SlowDataInformation.h"
#include "views/PictogramCaches.h"
#include "GamelistViewType.h"
#include "guis/ItemTracker/QuickJumpItemHolder.h"
#include "guis/ItemTracker/IQuickJumpTracker.h"
#include "views/container/IGamelistContainer.h"
#include "views/container/IGamelistEvents.h"
#include <systems/SystemData.h>
#include "IMetadataChanged.h"

class SystemManager;

enum class FileChangeType
{
  Added,
  Run,
  MetadataChanged,
  Removed,
  Sorted,
  DisplayUpdated,
};

class ISimpleGameListView : public Gui
                          , protected IGamelistEvents
                          , private IThemeSwitchable
                          , private IQuickJumpTracker
                          , public IMetadataChanged
{
  public:
    enum class Change
    {
      Resort, //!< New sorting required
      Update, //!< Update lists
    };

    ISimpleGameListView(WindowManager& window, SystemManager& systemManager, SystemData& system, const IGlobalVariableResolver& resolver, PictogramCaches& flagCache);

    ~ISimpleGameListView() override = default;

    void ShowQuickJump();

    /*!
     * @brief Get Arcade interface
     * @return Arcade interface or nullptr
     */
    virtual IArcadeGamelistInterface* getArcadeInterface() { return nullptr; }

    /*!
     * @brief Refresh name & properties of the given item
     * @game Game to refresh
     */
    virtual void RefreshItem(FileData* game) = 0;

    bool ProcessInput(const InputCompactEvent& event) override;

    bool CollectHelpItems(Help& help) override;

    [[nodiscard]] SystemData& System() const { return mSystem; }

    /*!
     * @brief Get the last-use timestamp of this gamelist view, using the very same tick base as the
     * textures (TextureHolder::Tick()). Used by the memory guard to evict the least-recently-used
     * gamelist view first.
     * @return Last use timestamp
     */
    [[nodiscard]] unsigned long long LastUse() const { return mLastUse; }

    //! Access current gamelist
    virtual IGamelistContainer& Gamelist() = 0;

    /*!
     * @brief Return view name as seen in themes
     * @return View name
     */
    [[nodiscard]] virtual const char* getName() const = 0;

    /*!
     * @brief Identofy every gamelist view with an unique type
     * @return Type
     */
    [[nodiscard]] virtual GamelistViewType Type() const = 0;

    virtual void DoUpdateGameInformation(bool update) = 0;

    virtual void populateList(const FolderData& folder, FileData* target) = 0;

    virtual void refreshList() = 0;

    /*!
     * @brief Called back from view manager when a game exited and the user is back to the gamelist
     * @param game Game ran
     */
    virtual void ReturnedFromGame(FileData* game) { (void)game; }

    /*!
     * @brief Implementation may return a filelist than may be used to quick move or jump.
     * ArcadeView for exemple may choose to return parent-only items.
     * @return List of FileData
     */
    virtual FileData::List GetHighLevelFileList() = 0;

    /*!
     * @brief Must be called right after the constructor
     */
    void DoInitialize()
    {
      Initialize();
      SwitchToTheme(mSystem.Theme(), false, nullptr);
      populateList(mSystem.MasterRoot(), nullptr);
    }

    /*!
     * @brief Get available regions from the current game list
     * @return Region list (may be empty)
     */
    virtual Regions::List AvailableRegionsInGames() = 0;

    /*!
     * @brief Check if the current game (under cursor) has p2k file available
     * @return True if at least one p2k file has been found
     */
    [[nodiscard]] virtual bool HasCurrentGameP2K() const = 0;

    /*!
     * @brief Gamelist may update thos information if required
     * @param info
     */
    virtual void UpdateSlowData(const SlowDataInformation& info) = 0;

    /*!
     * @brief Refrest a list refresh
     */
    void ListRefreshRequired() { mListRefreshRequired = true; }

    //! Refresh list
    void Update(int deltatime) override
    {
      Touch(); // The view being updated means it is the current/active one: mark it as freshly used
      Gui::Update(deltatime);
      if (mListRefreshRequired)
        refreshList();
    }

  protected:
    /*!
     * @brief Called right after the constructor
     */
    virtual void Initialize() = 0;

    virtual void launch(FileData* game) = 0;

    virtual void clean() = 0;

    virtual FileData* getEmptyListItem() = 0;

    /*!
     * @brief Get raw text display name w/o any icon or decoration
     * @param game Game from wich to get the name
     * @return Raw display name
     */
    virtual String GetRawDisplayName(const FileData& game) = 0;

    // Configuration cache
    RecalboxConf& mConf;

    //! Flag texture cache
    PictogramCaches& mPictogramCaches;

    //! Global variabel resolver
    const IGlobalVariableResolver& mResolver;

    SystemData& mSystem;

    //! SystemManager instance
    SystemManager& mSystemManager;

    TextComponent mHeaderText;
    ImageComponent mHeaderImage;

    ThemeExtras mThemeExtras;

    //! Last selection in folders
    HashMap<FolderData*, FileData*> mLastPositionInFolder;

    //! Item tracker
    QuickJumpItemHolder mTrackedItems;

    /*
     * IThemeSwitchable
     */

    /*!
     * @brief Gamelist view process their own attached system
     * @return SystemData
     */
    [[nodiscard]] SystemData* SystemTheme() const override { return &mSystem; };

    /*!
     * @brief Called when theme switch.
     * @param theme New Theme
     * @param refreshOnly True if we need to refresh the current view only
     */
    void SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface) override;

    /*!
     * @brief Ack a list refresh
     */
    void ListRefreshed() { mListRefreshRequired = false; }

    //! Set the last-use timestamp to now, using the same tick base as the textures (TextureHolder::Tick())
    void Touch() { mLastUse = TextureHolder::Tick(); }

    //! Is this the favorite gamelist?
    bool IsFavoriteSystem() { return mSystem.IsFavorite(); }

    /*
     * IGamelistEvents implementation
     */

    /*!
     * @brief Called when the list moved onto a new game
     * @param game Selected game
     * @param index Game index in the list
     */
    void OnGameSelected(IGamelistContainer& caller, FileData* game, int index) final;

    /*!
     * @brief Called when a game has been validated ("Valid" button pressed, then released)
     * @param game Selected game
     * @param index Game index in the list
     */
    void OnGameValidated(IGamelistContainer& caller, FileData* game, int index) final;

    /*!
     * @brief Called when a folder has been validated ("Valid" button pressed, then released)
     * @param caller Calling gamelist implementation
     * @param folder Selected folder
     * @param index Game index in the list
     */
    void OnFolderValidated(IGamelistContainer& caller, FolderData* folder, int index) final;

    /*!
     * @brief The user just fold the given header
     * @param caller Calling gamelist implementation
     * @param header Header to fold (selected item)
     * @param index Item index
     */
    void OnFoldHeader(IGamelistContainer& /*caller*/, HeaderData* /*header*/, int /*index*/) override {}

    /*!
     * @brief The user just unfold the given header
     * @param caller Calling gamelist implementation
     * @param header Header to fold (selected item)
     * @param index Item index
     */
    void OnUnfoldHeader(IGamelistContainer& /*caller*/, HeaderData* /*header*/, int /*index*/) override {}

    /*!
     * @brief The user just fold all header
     * @param caller Calling gamelist implementation
     */
    void OnFoldAllHeader(IGamelistContainer& /*caller*/) final {}

    /*!
     * @brief The user just Unfold all header
     * @param caller Calling gamelist implementation
     */
    void OnUnfoldAllHeader(IGamelistContainer& /*caller*/) final {}

    /*!
     * @brief The user just performed a fold action on a game
     * @param caller Calling gamelist implementation
     * @param game Game on which the fold action has been performed
     * @param index Item index
     */
    void OnFoldGame(IGamelistContainer& /*caller*/, FileData* /*game*/, int /*index*/) override {}

    /*!
     * @brief The user just performed an unfold action on a game
     * @param caller Calling gamelist implementation
     * @param game Game on which the unfold action has been performed
     * @param index Item index
     */
    void OnUnfoldGame(IGamelistContainer& /*caller*/, FileData* /*game*/, int /*index*/) override {}

    /*!
     * @brief The user performed a fold all game action
     * @param caller Calling gamelist implementation
     */
    void OnFoldAllGame(IGamelistContainer& /*caller*/) override {}

    /*!
     * @brief The user performed an unfold all game action
     * @param caller Calling gamelist implementation
     */
    void OnUnfoldAllGame(IGamelistContainer& /*caller*/) override {}

  private:
    //! Is list moving vertically?
    bool mVerticalMove;
    //! List need list refresh ?
    bool mListRefreshRequired;
    //! Last-use timestamp (same tick base as the textures), used for least-recently-used eviction
    unsigned long long mLastUse;

    /*
     * IQuickJump§Tracker implementation
     */

    /*!
     * @brief Called when the user select a new item to move to
     * @param item Item to move to
     */
    void QuickJumpTo(FileData* item) override;

    /*
     * IMetadataChanged implementation
     */

    /*!
     * @brief Notify that one or more metadata have changed for the given game
     * @param game Game whose metadata has changed
     */
    void GameMetadataChanged(FileData& game) final
    {
      Gamelist().Rename(&game, GetRawDisplayName(game));
      OnGameSelected(Gamelist(), &game, Gamelist().LookupGameIndex(&game));
    }
};
