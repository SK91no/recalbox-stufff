#pragma once

#include <views/gamelist/ISimpleGameListView.h>
#include "components/RatingComponent.h"
#include "components/DateTimeComponent.h"
#include "components/VideoComponent.h"
#include "components/VerticalScrollableContainer.h"
#include "components/TextScrollComponent.h"
#include "views/container/TextGamelist.h"
#include "components/PictureComponent.h"
#include <views/container/HeaderData.h>
#include <components/TextListComponent.h>
#include <games/EmptyData.h>
#include <scraping/scrapers/IScraperEngineStage.h>

#include <utility>

#include "RecalboxConf.h"
#include "views/container/LogoGamelist.h"

struct OneGameOneRom
{
  FileData* mParent;
  FileData::List mChildren;
  bool mFolded;   //!< True if the parent is folded

  OneGameOneRom(FileData* parent, FileData::List&& children) :
    mParent(parent),
    mChildren(std::move(children)),
    mFolded(true)
  {
    mParent->Metadata().SetHasFavoriteChild(HasFavorite());
  }

  bool HasFavorite()
  {
    if (mParent->Metadata().Favorite())
      return true;

    for (FileData* child : mChildren)
      if (child->Metadata().Favorite())
        return true;

    return false;
  }

};

class DetailedGameListView : public ISimpleGameListView
                           , private IScraperEngineStage
                           , private IVideoComponentAction
                           , private RecalboxConf::ISystemSortNotification
                           , private RecalboxConf::ISystemRegionFilterNotification
                           , private RecalboxConf::IRomPreferredRegionNotification
                           , private RecalboxConf::IRomSecondPreferredRegionNotification
                           , private RecalboxConf::IRomPreferredFallbackNotification
                           , private RecalboxConf::IOneGameOneRomNotification
                           , private RecalboxConf::ISystemFlatFoldersNotification
                           , private RecalboxConf::IShowOnlyLatestVersionNotification
                           , private RecalboxConf::IFavoritesOnlyNotification
                           , private RecalboxConf::IFavoritesFirstNotification
                           , private RecalboxConf::IShowHiddenNotification
                           , private RecalboxConf::IHideBoardGamesNotification
                           , private RecalboxConf::IFilterAdultGamesNotification
                           , private RecalboxConf::IGlobalHidePreinstalledNotification
                           , private RecalboxConf::IShowOnly3PlusPlayersNotification
                           , private RecalboxConf::IShowOnlyYokoGamesNotification
                           , private RecalboxConf::IShowOnlyTateGamesNotification
                           , private RecalboxConf::IHideNoGamesNotification
                           , private RecalboxConf::IDisplayByFileNameNotification
                           , private RecalboxConf::ISystemGamelistLayoutNotification
                           , private RecalboxConf::IGlobalGamelistLayoutNotification
{
  public:
    DetailedGameListView(WindowManager& window, SystemManager& systemManager, SystemData& system, const IGlobalVariableResolver& resolver, PictogramCaches& pictogramCache);

    ~DetailedGameListView() override;

    /*!
     * @brief Must be called right after the constructor
     */
    void Initialize() override;

    bool CollectHelpItems(Help& help) override;

    void SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface) override;

    [[nodiscard]] const char* getName() const override;

    [[nodiscard]] GamelistViewType Type() const override { return GamelistViewType::Detailed; };

    void DoUpdateGameInformation(bool update) final;

    //[[nodiscard]] int Count() const override { return Gamelist().Count(); };
    //[[nodiscard]] bool IsEmpty() const override { return mList.IsEmpty(); };

    /*!
     * @brief Rebuild the current gamelist according to the give folder
     * @param folder Game folder
     * @param target
     */
    void populateList(const FolderData& folder, FileData* target) override;

    /*!
     * @brief Refresh the UI list w/ what have been built in populateList
     * @param target Try to set the curser to the given target after refresh
     */
    void BuildList(FileData* target);

    void refreshList() override
    {
      // Get data & indexes
      FileData* currentItem = Gamelist().GetSelectedGame();
      int index = Gamelist().GetSelectedGameIndex();
      // Refresh list
      populateList(*mPopulatedFolder, nullptr);

      // Try to set the old data. If it does not exist anymore, ajust index
      if (!Gamelist().SetSelectedGame(currentItem, true))
        (void)Gamelist().SetSelectedGameIndex(index, true);
      // Ack list refresh
      ListRefreshed();
    }

    FileData::List GetHighLevelFileList() override { return Gamelist().GetGameArray(); }

    /*!
     * @brief Get available regions from the current game list
     * @return Region list (may be empty)
     */
    Regions::List AvailableRegionsInGames() override;

  protected:
    //! List's color index, integer compatible
    enum ColorIndexes
    {
      BaseColor = 0,
      GameColor = 0,
      FolderColor = 1,
      HighlightColor = 2,
      GameHighlightColor = 2,
      FolderHighlightColor = 3,
      HeaderColor = 4
    };

    //! Empty item
    HeaderData mEmptyListItem;

    //! Current folder
    const FolderData *mPopulatedFolder;

    //! Game list
    //LogoGamelist mList;
    //! Gamelist interface
    IGamelistContainer* mCurrentList;

    //! Gamelist
    [[nodiscard]] IGamelistContainer& Gamelist() final { return *mCurrentList; }

    void launch(FileData* game) override;
    void clean() override { mVideo.setVideo(Path::Empty, false); }

    /*!
     * @brief Switch to the given layout
     * @param layout Layout to switch to
     */
    void SwitchLayout(RecalboxConf::GamelistLayout layout);

    /*!
     * @brief Get raw text display name w/o any icon or decoration
     * @return Raw display name
     */
    String GetRawDisplayName(const FileData& game) override;

    /*!
     * @brief Get description of the given game
     * @param game Game
     * @return Description
     */
    virtual String GetDescription(FileData& game) { return game.Metadata().Description(); }

    /*
     * Component override
     */

    void Update(int deltatime) override;

    void Render(const Transform4x4f& parentTrans) override;

    /*!
     * @brief Process extended actions
     * @param event Event to process
     * @return true if the event has been processed, false otherwise
     */
    bool ProcessInput(const InputCompactEvent& event) override;

    /*!
     * @brief Called when the list goes out of screen or is no longer visible
     */
    void onHide() override { Gamelist().OnHide(); }

    /*!
     * @brief Check if a header is required between previous and next item
     * @param previous Previous filedata
     * @param next Next fildata
     * @param hasTopFavorites instruct the method that the list has at least one favorite on top or bottom
     * @return No null header pointer if a header is required, nullptr otherwise
     */
    HeaderData* NeedHeader(FileData* previous, FileData* next, bool hasTopFavorites);

    /*!
     * @brief Set internal sort cache used in header generation
     * Inherited class must use it once, before generating header using NeedHeader
     * @param sort New sort
     */
    void SetInternalSort(FileSorts::Sorts sort) { mSort = sort; }

  private:
    //! Hovering time in ms before starting a seamless scrape
    static constexpr int sMaxHoveringTimeBeforeScraping = 200; // 200ms
    //! Elapsed time since starting seamless scraping before displaying busy animation
    static constexpr int sMaxScrapingTimeBeforeBusyAnim = 200; // 200ms

    //! Elapsed time on the selected game (hovering time)
    int mElapsedTimeOnGame;
    //! Scraping state (for visual feedback)
    bool mIsScraping;

    void initMDLabels();
    void initMDValues();

    RatingComponent mHeaderStars;

    ImageComponent mImage;
    ImageComponent mNoImage;
    VideoComponent mVideo;
    std::vector<PictureComponent *> mFolderContent;
    ImageComponent mRegions[Regions::RegionPack::sMaxRegions];

    TextComponent mLblRating, mLblReleaseDate, mLblDeveloper, mLblPublisher, mLblGenre, mLblPlayers, mLblLastPlayed, mLblPlayCount, mLblFavorite;
    TextComponent mFolderName;

    RatingComponent mRating;
    TextScrollComponent mReleaseDate;
    TextScrollComponent mDeveloper;
    TextScrollComponent mPublisher;
    TextScrollComponent mGenre;
    TextComponent mPlayers;
    DateTimeComponent mLastPlayed;
    TextComponent mPlayCount;
    TextComponent mFavorite;

    ImageComponent mDecorationFlagsTemplate;
    ImageComponent mDecorationGenreTemplate;
    ImageComponent mDecorationPlayersTemplate;

    Array<TextComponent*> getMDLabels();
    Array<ThemableComponent*> getMDValues();

    VerticalScrollableContainer mDescContainer;
    TextComponent mDescription;

    BusyComponent mBusy;

    RecalboxConf& mSettings;

    //! Components linked by video component
    Array<Component*> mVideoLinks;

    //! Array of fade-in components (from transparent to opaque)
    Array<Component*> mFadeInList;
    //! Array of fade-out components (from opaque to transparent)
    Array<Component*> mFadeOutList;

    //! HeaderData map
    HashMap<String, HeaderData*> mHeaderMap;
    //! Game list
    FileData::List mGamelist;
    //! One Game One rom links
    HashMap<FileData*, OneGameOneRom*> m1G1R;

    //! Current sort
    FileSorts::Sorts mSort;

    //! Last processed cursor item
    FileData* mLastCursorItem;
    //! Current game P2K status
    bool mLastCursorItemHasP2K;

    //! Current layout
    RecalboxConf::GamelistLayout mLayout;

    //! Free 1G1R hashmap
    void Free1G1R()
    {
      for(auto& it : m1G1R) delete it.second;
      m1G1R.clear();
    }

    /*!
     * @brief Move a component into the fade-in list, removing it from the fade-out list if it was in.
     * @param component Component to move to the fade-in list
     */
    void MoveToFadeIn(Component* component)
    {
      mFadeOutList.Remove(component);
      if (!mFadeInList.Contains(component))
        mFadeInList.Add(component);
    }
    /*!
     * @brief Move a component into the fade-out list, removing it from the fade-in list if it was in.
     * @param component Component to move to the fade-out list
     */
    void MoveToFadeOut(Component* component)
    {
      mFadeInList.Remove(component);
      if (!mFadeOutList.Contains(component))
        mFadeOutList.Add(component);
    }

    /*!
     * @brief Move a component into the fade-in list, removing it from the fade-out list if it was in.
     * @param components Component to move to the fade-in list
     */
    void MoveToFadeIn(const Array<Component*>& components)
    {
      for(Component* component : components)
      {
        mFadeOutList.Remove(component);
        if (!mFadeInList.Contains(component))
          mFadeInList.Add(component);
      }
    }
    /*!
     * @brief Move a component into the fade-out list, removing it from the fade-in list if it was in.
     * @param components Component to move to the fade-out list
     */
    void MoveToFadeOut(const Array<Component*>& components)
    {
      for(Component* component : components)
      {
        mFadeInList.Remove(component);
        if (!mFadeOutList.Contains(component))
          mFadeOutList.Add(component);
      }
    }

    /*!
     * @brief Move a component into the fade-in list, removing it from the fade-out list if it was in.
     * @param components Component to move to the fade-in list
     */
    void MoveToFadeIn(const Array<ThemableComponent*>& components)
    {
      for(Component* component : components)
      {
        mFadeOutList.Remove(component);
        if (!mFadeInList.Contains(component))
          mFadeInList.Add(component);
      }
    }
    /*!
     * @brief Move a component into the fade-out list, removing it from the fade-in list if it was in.
     * @param components Component to move to the fade-out list
     */
    void MoveToFadeOut(const Array<ThemableComponent*>& components)
    {
      for(Component* component : components)
      {
        mFadeInList.Remove(component);
        if (!mFadeOutList.Contains(component))
          mFadeOutList.Add(component);
      }
    }

    /*!
     * @brief Move a component into the fade-in list, removing it from the fade-out list if it was in.
     * Set opacity to 255 instantly
     * @param component Component to move to the fade-in list
     */
    void MoveToFadeInMax(Component* component)
    {
      mFadeOutList.Remove(component);
      if (!mFadeInList.Contains(component))
        mFadeInList.Add(component);
      component->setOpacity(255);
    }
    /*!
     * @brief Move a component into the fade-out list, removing it from the fade-in list if it was in.
     * Set opacity to 0 instantly
     * @param component Component to move to the fade-out list
     */
    void MoveToFadeOutMax(Component* component)
    {
      mFadeInList.Remove(component);
      if (!mFadeOutList.Contains(component))
        mFadeOutList.Add(component);
      component->setOpacity(255);
    }

    FileData* getEmptyListItem() override { return &mEmptyListItem; }

    Array<ThemableComponent*> getFolderComponents();
    Array<ThemableComponent*> getGameComponents(bool includeMainComponents = true);
    Array<ThemableComponent*> getScrapedFolderComponents();
    void setGameInfo(FileData* file, bool update);
    void setRegions(FileData* file);
    void setScrapedFolderInfo(FileData* file);

    /*!
     * @brief Refresh the list if the given system is this system
     * @param system System to check against this
     */
    void RefreshRequiredIfSelf(const SystemData& system) { if (&mSystem == &system) ListRefreshRequired(); }

    /*!
     * @brief Build component list linked by the video component
     * @param theme Current theme
     */
    void BuildVideoLinks(const ThemeData& theme);

    /*!
     * @brief Gamelist may update those information if required
     * @param info
     */
    void UpdateSlowData(const SlowDataInformation& info) override;

    /*!
     * @brief Build a list of 1G1R structure in the natural sort order and remove child from the given list
     * @param items Game list
     */
    void GeneratePreferredOneGameOneRoms(FileData::List& items);

    /*!
     * @brief Display folder info
     * @param folder Folder
     * @param count Game count in folder
     * @param path Path of folder images
     */
    void SetFolderInfo(FolderData* folder, int count, const FolderImagesPath& path);

    void SetHeaderInfo(FileData* header, const FileData::List& games);

    /*!
     * @brief Check if the game has P2k
     * @return
     */
    [[nodiscard]] bool HasCurrentGameP2K() const override { return mLastCursorItemHasP2K; };

    /*!
     * @brief Refresh name & properties of the given item
     */
    void RefreshItem(FileData* game) override;

    /*!
     * @brief Set fading (or not) between NoImage & Image, regarding game data
     * @param game Game data
     * @param videoStillRunning True if the game video is still running
     */
    void SetImageFading(FileData* game, bool videoStillRunning);

    /*!
     * @brief Create or get a named header
     * @param name Header name
     * @return HeaderData instance
     */
    HeaderData* GetHeader(FileSorts::Sorts sort, const String& name);

    /*!
     * @brief Create or get a named header
     * @param name Header name
     * @param data Integer data
     * @return HeaderData instance
     */
    HeaderData* GetHeader(FileSorts::Sorts sort, const String& name, int data);

    /*!
     * @brief Create or get a named header
     * @param name Header name
     * @param data Float data
     * @return HeaderData instance
     */
    HeaderData* GetHeader(FileSorts::Sorts sort, const String& name, float data);

    /*!
     * @brief Move to next/previous sort
     * @param next True to move to next sort, false for previous
     */
    void ChangeSort(bool next);

    /*!
     * @brief Get available regions from the given listt
     * @return Region list (may be empty)
     */
    static Regions::List AvailableRegionsInGames(FileData::List& list);

    /*
     * IGamelistEvents override
     */

    /*!
     * @brief The user just fold the given header
     * @param caller Calling gamelist implementation
     * @param header Header to fold (selected item)
     * @param index Item index
     */
    void OnFoldHeader(IGamelistContainer& caller, HeaderData* header, int index) override
    {
      (void)caller;
      (void)index;
      header->ToggleFolded();
      ListRefreshRequired();
    }

    /*!
     * @brief The user just unfold the given header
     * @param caller Calling gamelist implementation
     * @param header Header to fold (selected item)
     * @param index Item index
     */
    void OnUnfoldHeader(IGamelistContainer& caller, HeaderData* header, int index) override
    {
      (void)caller;
      (void)index;
      header->ToggleFolded();
      ListRefreshRequired();
    }

    /*!
     * @brief The user just performed a fold action on a game
     * @param caller Calling gamelist implementation
     * @param game Game on which the fold action has been performed
     * @param index Item index
     */
    void OnFoldGame(IGamelistContainer& caller, FileData* game, int index) override;

    /*!
     * @brief The user just performed an unfold action on a game
     * @param caller Calling gamelist implementation
     * @param game Game on which the unfold action has been performed
     * @param index Item index
     */
    void OnUnfoldGame(IGamelistContainer& caller, FileData* game, int index) override;

    /*!
     * @brief The user performed a fold all game action
     * @param caller Calling gamelist implementation
     */
    void OnFoldAllGame(IGamelistContainer& caller) override;

    /*!
     * @brief The user performed an unfold all game action
     * @param caller Calling gamelist implementation
     */
    void OnUnfoldAllGame(IGamelistContainer& caller) override;

    /*
     * IScrapeEngineStage implementation
     */

    /*!
     * @brief Report scraping stage completion
     * @param game Target game
     * @param stage Last stage completed
     * @param changes Metadata that have been actually changed
     */
    void ScrapingStageCompleted(FileData* game, Stage stage, MetadataType changes) override;

    /*
     * IVideoComponentAction
     */

    /*!
     * @brief Called when a video component require action
     * @param source Source video component
     * @param action Required action
     */
    void VideoComponentRequireAction(const VideoComponent* source, Action action) final;

    /*!
     * @brief Get a simplified text of elapsed time in minutes, hours, days, month or years
     * @param span Time span
     * @return String representation
     */
    static String GetSimplifiedElapsedTime(const TimeSpan& span);

    /*
     * ISimpleGameListView overrides
     */

    /*!
     * @brief Called back from view manager when a game exited and the user is back to the gamelist
     * @param game Game ran
     */
    void ReturnedFromGame(FileData* game) override;

    /*
     * RecalboxConf::ISystemSortNotification implementation
     */

    void ConfigurationSystemSortChanged(const SystemData& system, const FileSorts::Sorts& value) final { (void)value; RefreshRequiredIfSelf(system); }


    /*
     * RecalboxConf::SystemRegionFilterNotify implementation
     */

    void ConfigurationSystemRegionFilterChanged(const SystemData& system, const Regions::GameRegions& state) final { (void)state; RefreshRequiredIfSelf(system); }

    /*
     * RecalboxConf::RomPreferredRegion implementation
     */
    void ConfigurationRomPreferredRegionChanged(const Regions::GameRegions& state) final { (void)state; ListRefreshRequired(); }

    /*
     * RecalboxConf::RomSecondPreferredRegion implementation
     */
    void ConfigurationRomSecondPreferredRegionChanged(const Regions::GameRegions& state) final { (void)state; ListRefreshRequired(); }

    /*
     * RecalboxConf::RomPreferredFallback implementation
     */
    void ConfigurationRomPreferredFallbackChanged(const Regions::OrderedMasterRegions& state) final { (void)state; ListRefreshRequired(); }

    /*
     * RecalboxConf::OneGameOneRom implementation
     */
    void ConfigurationOneGameOneRomChanged(const bool& state) final { (void)state; ListRefreshRequired(); }

    /*
     * RecalboxConf::SystemFlatFoldersNotify implementation
     */

    void ConfigurationSystemFlatFoldersChanged(const SystemData& system, const bool& state) final { (void)state; RefreshRequiredIfSelf(system); }

    /*
     * RecalboxConf::ShowOnlyLatestVersionNotidy implementation
     */

    void ConfigurationShowOnlyLatestVersionChanged(const bool& state) final { (void)state; ListRefreshRequired(); }

    /*
     * RecalboxConf::FavoritesOnlyNotidy implementation
     */

    void ConfigurationFavoritesOnlyChanged(const bool& state) final { (void)state; ListRefreshRequired(); }

    /*
     * RecalboxConf::FavoritesFirstConfigurationNotify
     */

    void ConfigurationFavoritesFirstChanged(const bool& value) final { (void)value; ListRefreshRequired(); }

    /*
     * RecalboxConf::ShowHiddenConfigurationNotify
     */

    void ConfigurationShowHiddenChanged(const bool& value) final { (void)value; ListRefreshRequired(); }

    /*
     * RecalboxConf::HideBoardGamesConfigurationNotify
     */

    void ConfigurationHideBoardGamesChanged(const bool& value) final { (void)value; ListRefreshRequired(); }

    /*
     * RecalboxConf::FilterAdultGamesConfigurationNotify
     */

    void ConfigurationFilterAdultGamesChanged(const bool& value) final { (void)value; ListRefreshRequired(); }

    /*
     * RecalboxConf::GlobalHidePreinstalledConfigurationNotify
     */

    void ConfigurationGlobalHidePreinstalledChanged(const bool& value) final { (void)value; ListRefreshRequired(); }

    /*
     * RecalboxConf::ShowOnly3PlusPlayersConfigurationNotify
     */

    void ConfigurationShowOnly3PlusPlayersChanged(const bool& value) final { (void)value; ListRefreshRequired(); }

    /*
     * RecalboxConf::ShowOnly3PlusPlayersConfigurationNotify
     */

    void ConfigurationShowOnlyYokoGamesChanged(const bool& value) final { (void)value; ListRefreshRequired(); }

    /*
     * RecalboxConf::ShowOnly3PlusPlayersConfigurationNotify
     */

    void ConfigurationShowOnlyTateGamesChanged(const bool& value) final { (void)value; ListRefreshRequired(); }

    /*
     * RecalboxConf::ShowOnly3PlusPlayersConfigurationNotify
     */

    void ConfigurationHideNoGamesChanged(const bool& value) final { (void)value; ListRefreshRequired(); }

    /*
     * RecalboxConf::DisplayByFileNameNotification
     */

    void ConfigurationDisplayByFileNameChanged(const bool& state) final { (void)state; ListRefreshRequired(); }

    /*
     * RecalboxConf::GamelistContainerTypeNotification
     */

    void ConfigurationSystemGamelistLayoutChanged(const SystemData& system, const RecalboxConf::GamelistLayout& layout) final;

    /*
     * RecalboxConf::GlobalGamelistLayoutNotification
     */

    void ConfigurationGlobalGamelistLayoutChanged(const RecalboxConf::GamelistLayout& layout) final;
};
