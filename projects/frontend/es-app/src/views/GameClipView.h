//
// Created by gugue_U on 17/10/2020
//
#pragma once

#include <vector>
#include <systems/SystemManager.h>
#include <utils/datetime/HighResolutionTimer.h>
#include <components/VideoComponent.h>
#include <components/ScrollableContainer.h>
#include <components/TextComponent.h>
#include <components/TextScrollComponent.h>
#include <components/ImageComponent.h>
#include <components/RatingComponent.h>
#include <components/DateTimeComponent.h>
#include <components/ClippingContainer.h>
#include <themes/ThemeExtras.h>
#include <themes/IThemeSwitchable.h>
#include <RecalboxConf.h>
#include "games/GameRandomSelector.h"

class GameClipView : public Gui
                   , private IThemeSwitchable
{
    enum class State
    {
        NoGameSelected,
        EmptyPlayList,
        InitPlaying,
        Playing,
        LaunchGame,
        GoToSystem,
        Quit,
        Terminated,
    };

    enum class Direction
    {
        Next,
        Previous,
    };

  private:
    class Filter : public IFilter
    {
      private:
        FileData::TopLevelFilter mFilter;
        HashSet<String> mDuplicates;
      public:
        Filter() : mFilter(FileData::BuildTopLevelFilter()) {}
        [[nodiscard]] bool ApplyFilter(const FileData& file) override
        {
          String video(file.Metadata().VideoAsString());
          if (video.empty() || !file.IsDisplayable(mFilter)) return false;
          if (mDuplicates.contains(video)) return false;
          mDuplicates.insert(video);
          return true;
        }
        void TearDown() final { mDuplicates.clear(); }
    } mFilter;

    //! Window
    WindowManager& mWindow;

    SystemManager& mSystemManager;

    GameRandomSelector mGameRandomSelector;

    static constexpr int sMaxHistory = 10;

    int mHistoryPosition;
    std::vector<FileData*> mHistory;
    Direction mDirection;

    FileData* mGame;
    SystemData* mSystem;

    HighResolutionTimer mTimer;

    State mState;

    int mSystemIndex;

    int mVideoDuration;

    // ---- Game clip components (formerly GameClipContainer) ----

    VideoComponent mVideo;
    ImageComponent mImage;
    ImageComponent mThumbnail;
    ImageComponent mRecalboxLogo;
    ClippingContainer mClippingContainer;
    ImageComponent mClippingImage;

    TextComponent mLbGameName, mLbSystemName, mLblRating, mLblReleaseDate, mLblDeveloper, mLblPublisher, mLblGenre, mLblPlayers, mLblLastPlayed, mLblPlayCount, mLblFavorite;
    TextScrollComponent mGameName;
    TextScrollComponent mSystemName;
    RatingComponent mRating;
    TextScrollComponent mReleaseDate;
    TextScrollComponent mDeveloper;
    TextScrollComponent mPublisher;
    TextScrollComponent mGenre;
    TextScrollComponent mPlayers;
    DateTimeComponent mLastPlayed;
    TextScrollComponent mPlayCount;
    TextScrollComponent mFavorite;
    ImageComponent mFavoriteIcon;

    ScrollableContainer mDescContainer;
    TextComponent mDescription;
    TextComponent mHeaderText;
    ImageComponent mHeaderImage;
    ImageComponent mOverlay;

    ThemeExtras mThemeExtras;

    // ---- "No video available" screen (formerly GameClipNoVideoContainer) ----

    TextComponent mWarningText;
    ImageComponent mMire;

    void InsertIntoHistory(FileData* game);

    void GetGame();

    void GetNextGame();

    void GetPreviousGame();

    void StartGameClip();

    void StopGameClipView();

    void ChangeGameClip(Direction direction);

    /*
     * Game clip presentation (formerly GameClipContainer)
     */

    void initComponents();

    void initMDLabels();

    void initMDValues();

    std::vector<TextComponent*> getMDLabels();

    std::vector<ThemableComponent*> getMDValues();

    void setGameInfo(FileData* game);

    void CleanVideo();

    /*
     * IThemeSwitchable
     */

    /*!
     * @brief Gameclip view has a current game's system
     * @return SystemData
     */
    [[nodiscard]] SystemData* SystemTheme() const override { return mSystem; }

    /*!
     * @brief Called when theme switch.
     * @param theme New Theme
     * @param refreshOnly True if we need to refresh the current view only
     */
    void SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface) override;

    /*
     * GUI
     */

    void Update(int elapsed) override;

    void Render(const Transform4x4f& parentTrans) override;

    bool ProcessInput(const InputCompactEvent& event) override;

    bool CollectHelpItems(Help& help) override;

  public:

    static const char* getName() { return "gameclip"; }

    static bool IsGameClipEnabled() { return RecalboxConf::Instance().GetScreenSaverType() == RecalboxConf::Screensaver::Gameclip; }

    //! Default constructor
    explicit GameClipView(WindowManager& window, SystemManager& systemManager);

    ~GameClipView() override;

    //! Reinit the view
    void Reset();
};
