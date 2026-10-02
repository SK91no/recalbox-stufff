//
// Created by gugue_U on 17/10/2020
//

#include <recalbox/RecalboxSystem.h>
#include "GameClipView.h"
#include <views/ViewController.h>
#include "utils/locale/LocaleHelper.h"
#include "MenuFilter.h"
#include <guis/GuiInfoPopup.h>
#include <usernotifications/NotificationManager.h>
#include <audio/AudioScheduler.h>
#include <rendering/fonts/FontManager.h>
#include <themes/ThemeManager.h>
#include <Renderer.h>

GameClipView::GameClipView(WindowManager& window, SystemManager& systemManager)
  : Gui(window)
  , mWindow(window)
  , mSystemManager(systemManager)
  , mGameRandomSelector(systemManager, &mFilter)
  , mHistoryPosition(0)
  , mDirection(Direction::Next)
  , mGame(nullptr)
  , mSystem(nullptr)
  , mState(State::NoGameSelected)
  , mSystemIndex(-1)
  , mVideoDuration(0)
  , mVideo(window, nullptr)
  , mImage(window, ImgProps::KeepRatio | ImgProps::NoCache)
  , mThumbnail(window, ImgProps::KeepRatio | ImgProps::NoCache)
  , mRecalboxLogo(window)
  , mClippingContainer(window, 300, 1000)
  , mClippingImage(window)
  , mLbGameName(window)
  , mLbSystemName(window)
  , mLblRating(window)
  , mLblReleaseDate(window)
  , mLblDeveloper(window)
  , mLblPublisher(window)
  , mLblGenre(window)
  , mLblPlayers(window)
  , mLblLastPlayed(window)
  , mLblPlayCount(window)
  , mLblFavorite(window)
  , mGameName(window)
  , mSystemName(window)
  , mRating(window, 0.f, "gc")
  , mReleaseDate(window)
  , mDeveloper(window)
  , mPublisher(window)
  , mGenre(window)
  , mPlayers(window)
  , mLastPlayed(window)
  , mPlayCount(window)
  , mFavorite(window)
  , mFavoriteIcon(window)
  , mDescContainer(window)
  , mDescription(window)
  , mHeaderText(window)
  , mHeaderImage(window)
  , mOverlay(window)
  , mThemeExtras(window)
  , mWarningText(window)
  , mMire(window)
{
  setSize(Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat());

  // ---- Game clip components ----

  addChild(&mVideo);
  addChild(&mImage);
  addChild(&mThumbnail);
  addChild(&mRecalboxLogo);
  if (RecalboxConf::Instance().GetShowGameClipClippingItem())
  {
    addChild(&mClippingContainer);
    mClippingContainer.addChild(&mClippingImage);
  }
  addChild(&mOverlay);

  addChild(&mDescContainer);
  mDescContainer.addChild(&mDescription);

  // values
  addChild(&mGameName);
  addChild(&mSystemName);
  addChild(&mRating);
  addChild(&mReleaseDate);
  addChild(&mDeveloper);
  addChild(&mPublisher);
  addChild(&mGenre);
  addChild(&mPlayers);
  mLastPlayed.setDisplayMode(DateTimeComponent::Display::RelativeToNow);
  addChild(&mLastPlayed);
  addChild(&mPlayCount);

  // labels
  mLbGameName.setText(_("Game") + ": ");
  addChild(&mLbGameName);
  mLbSystemName.setText(_("System") + ": ");
  addChild(&mLbSystemName);
  mLblRating.setText(_("Rating") + ": ");
  addChild(&mLblRating);
  mLblReleaseDate.setText(_("Released") + ": ");
  addChild(&mLblReleaseDate);
  mLblDeveloper.setText(_("Developer") + ": ");
  addChild(&mLblDeveloper);
  mLblPublisher.setText(_("Publisher") + ": ");
  addChild(&mLblPublisher);
  mLblGenre.setText(_("Genre") + ": ");
  addChild(&mLblGenre);
  mLblPlayers.setText(_("Players") + ": ");
  addChild(&mLblPlayers);
  mLblLastPlayed.setText(_("Last played") + ": ");
  addChild(&mLblLastPlayed);
  mLblPlayCount.setText(_("Times played") + ": ");
  addChild(&mLblPlayCount);

  // ---- "No video available" screen ----

  mWarningText.setFont(&FontManager::Instance().FromDefault(FONT_SIZE_MEDIUM, false));
  mWarningText.setPosition(mSize.x() / 2, mSize.y() / 2);
  mWarningText.setSize(0, 0);
  mWarningText.setAlignment(::Alignment::Center);
  mWarningText.setOrigin(0.5f, 0.5f);
  mWarningText.setZIndex(50);
  mWarningText.setDefaultZIndex(50);
  mWarningText.setColor(0xFFFFFFFF);
  mWarningText.setText(_("Gameclips cannot be played. No video availabe for your selection"));
  mWarningText.setBackgroundColor(0x000000FF);

  mMire.setImage(Path(":/mire_recalbox.svg"), false);
  mMire.setOrigin(0.5f, 0.5f);
  mMire.setPosition(mSize.x() / 2, mSize.y() / 2);
  mMire.setSize(mSize.x(), mSize.y());
  mMire.setZIndex(0);
  mMire.setDefaultZIndex(0);
}

void GameClipView::Reset()
{
  CleanVideo();
  mGameRandomSelector.Initialize();

  mHistoryPosition = 0;
  mDirection = Direction::Next;
  mGame = nullptr;
  mState = State::NoGameSelected;
  mSystemIndex = -1;
  mVideoDuration = 0;
}

GameClipView::~GameClipView()
{
  mHistory.clear();
}

void GameClipView::InsertIntoHistory(FileData* game)
{
  mHistory.insert(mHistory.begin(), game);
  if (mHistory.size() == sMaxHistory + 1)
    mHistory.erase(mHistory.begin() + sMaxHistory);
}

void GameClipView::GetGame()
{
  if (Direction::Next == mDirection)
    GetNextGame();
  else
    GetPreviousGame();
}

void GameClipView::GetNextGame()
{
  if (mHistoryPosition == 0)
    mGame = mGameRandomSelector.NextGame();
  else
  {
    // security
    if (--mHistoryPosition < 0) mHistoryPosition = 0;
    mGame = mHistory[mHistoryPosition];
  }
}

void GameClipView::GetPreviousGame()
{
  //security
  if (++mHistoryPosition >= (int) mHistory.size())
    mHistoryPosition = (int) mHistory.size() - 1;
  mGame = mHistory[mHistoryPosition];
}

void GameClipView::Update(int elapsed)
{
  // The video is rendered manually (this view drives its own state machine and does not propagate
  // Update to its children), so feed the engine heartbeat ourselves: without it the decoder freezes
  // and its texture gets purged, leaving the gameclip video black.
  mVideo.Update(elapsed);

  switch(mState)
  {
    case State::NoGameSelected:
    {
      if (!mGameRandomSelector.HasValidSystems())
      {
        mState = State::EmptyPlayList;
        UpdateHelpBar();
        mTimer.Initialize(0);
        return;
      }
      StartGameClip();
      mTimer.Initialize(0);
      mState = State::InitPlaying;
      UpdateHelpBar();
      break;
    }
    case State::EmptyPlayList:
    {
      if(mTimer.GetMilliSeconds() > 60000)
        mState = State::Quit;
      break;
    }
    case State::InitPlaying:
    {
      // when videoEngine cannot play video file
      if (!mGame->Metadata().Video().Exists() ||  mTimer.GetMilliSeconds() > 3000)
      {
        { LOG(LogDebug) << "[GameClip] Video do not start for game: " << mGame->Metadata().VideoAsString(); }
        CleanVideo();
        mState = State::NoGameSelected;
        return;
      }
      if (mVideo.isDiplayed())
      {
        NotificationManager::Instance().Notify(*mGame, Notification::StartGameClip);
        mVideoDuration = mVideo.GetVideoDurationMs();
        if (Direction::Next == mDirection && mHistoryPosition == 0)
          InsertIntoHistory(mGame);
        mState = State::Playing;
      }
      break;
    }
    case State::Playing:
    {
      if (mTimer.GetMilliSeconds() > mVideoDuration || mTimer.GetMilliSeconds() > 35000)
        ChangeGameClip(Direction::Next);
      break;
    }
    case State::LaunchGame:
    {
      ViewController::Instance().selectGamelistAndCursor(mGame);
      ViewController::Instance().Launch(mGame, GameLinkedData(), Vector3f(), true);
      mState = State::Terminated;
      break;
    }
    case State::GoToSystem:
    {
      ViewController::Instance().selectGamelistAndCursor(mGame);
      mState = State::Quit;
      break;
    }
    case State::Quit:
    {
      ViewController::Instance().BackToPreviousView();
      mState = State::Terminated;
      break;
    }
    case State::Terminated: return;
  }
}

void GameClipView::Render(const Transform4x4f& parentTrans)
{
  if (mState == State::EmptyPlayList)
  {
    mMire.Render(parentTrans);
    mWarningText.Render(parentTrans);
    return;
  }

  if (!mVideo.isDiplayed())
  {
    mVideo.Render(parentTrans);
    return;
  }

  mFavorite.setValue(mGame->Metadata().FavoriteAsString());
  if(mGame->Metadata().Favorite())
  {
    addChild(&mFavoriteIcon);
  }
  else
  {
    removeChild(&mFavoriteIcon);
  }

  Vector2f videoCenter = mVideo.getCenter();
  mClippingImage.setPosition(videoCenter.x(), videoCenter.y());

  renderChildren(parentTrans);
}

bool GameClipView::ProcessInput(const InputCompactEvent& event)
{
  if (event.CancelReleased())
  {
    mState = State::Quit;
    StopGameClipView();
  }

  if (State::EmptyPlayList == mState || State::Playing != mState || mTimer.GetMilliSeconds() < 1000) return true;

  // GO TO GAME LIST -  SELECT
  if (event.SelectReleased())
  {
    mState = State::GoToSystem;
    StopGameClipView();
    return true;
  }

  // RUN GAME - START
  if (event.StartReleased())
  {
    StopGameClipView();
    mState = State::LaunchGame;
    return true;
  }

  // NEXT GAMECLIP  - RIGHT
  if (event.RightReleased())
  {
    ChangeGameClip(Direction::Next);
    return true;
  }
  // PREVIOUS GAMECLIP - LEFT
  if (event.LeftReleased())
  {
    ChangeGameClip(Direction::Previous);
    return true;
  }

  // TOGGLE FAVORITES - West (SNES Y)
  if (event.WestReleased() && MenuFilter::ShouldEnableFeature(MenuFilter::Favorites) && RecalboxConf::Instance().GetEnableEditFavorites())
  {
    if (mGame->IsGame())
    {
      ViewController::Instance().ToggleFavorite(mGame);
      UpdateHelpBar();
    }
    return true;
  }
  return true;
}

void GameClipView::StartGameClip()
{
  GetGame();
  ThemeManager::ChangeThemeContext("gameclip", mThemeExtras.Extras(), &mGame->System(), mGame);
  setGameInfo(mGame);
}

void GameClipView::StopGameClipView()
{
  NotificationManager::Instance().Notify(Notification::StopGameClip);
  CleanVideo();
}

void GameClipView::ChangeGameClip(Direction direction)
{
  if (direction == Direction::Previous && mHistoryPosition >= (int) mHistory.size() - 1)
    return;

  mDirection = direction;
  mState = State::NoGameSelected;
  CleanVideo();
}

bool GameClipView::CollectHelpItems(Help& help)
{
  switch(mState)
  {
    case State::NoGameSelected :
    case State::GoToSystem:
    case State::LaunchGame:
    case State::Terminated :
    case State::Quit : break;
    case State::EmptyPlayList:
      help.Set(Help::Cancel(), _("QUIT"));
      return true;
    case State::InitPlaying:
    case State::Playing:
      if(RecalboxConf::Instance().GetShowGameClipHelpItems())
      {
        help.Set(Help::Cancel(), _("QUIT"))
            .Set(HelpType::Start, _("LAUNCH"));
        if (RecalboxConf::Instance().GetEnableEditFavorites())
          help.Set(HelpType::Y, mGame->Metadata().Favorite() ? _("Remove from favorite") : _("Favorite"));
        help.Set(HelpType::LeftRight, _("CHANGE"))
            .Set(HelpType::Select, _("GO TO GAME"));
      }
      return true;
  }
  return true;
}

void GameClipView::initComponents()
{
  // init all labels and values with theme default values
  // and disabled them for default view "fullscreen"
  initMDValues();
  initMDLabels();

  mDescription.setSize(mDescContainer.getSize().x(), 0);
  mDescContainer.setThemeDisabled(true);
  mDescription.setPosition(0, 0);

  mHeaderText.setDefaultZIndex(50);
  mHeaderText.setThemeDisabled(true);
  mHeaderImage.setDefaultZIndex(50);
  mHeaderImage.setThemeDisabled(true);

  mThumbnail.setDefaultZIndex(50);
  mThumbnail.setThemeDisabled(true);

  // init visible components for default view
  const float padding = 0.01f;
  //const float verticalPadding = mSize.y() * padding;
  const float horizontalPadding = mSize.x() * padding;

  mVideo.setOrigin(0.5, 0.5);
  mVideo.setPosition(mSize.x() * 0.5f, mSize.y() * 0.5f);
  mVideo.setResize(mSize.x(), mSize.y());
  mVideo.setKeepRatio(true);
  mVideo.setDefaultZIndex(30);
  mVideo.setThemeDisabled(false);

  if (RecalboxConf::Instance().GetShowGameClipClippingItem())
  {
    mClippingContainer.setSize(mSize);
    mClippingContainer.setDefaultZIndex(50);
    mClippingContainer.setZIndex(50);
    mClippingContainer.setThemeDisabled(false);

    mClippingImage.setImage(Path(":/press_start.svg"), false);
    mClippingImage.setResize(mSize.x() * 0.12f, mSize.y() * 0.12f);
    mClippingImage.setKeepRatio(true);
    mClippingImage.setOrigin(0.5, 0.5);
    mClippingImage.setThemeDisabled(false);
  }

  mRecalboxLogo.setImage(Path(":/Recalbox_logo_White.svg"), false);
  mRecalboxLogo.setOrigin(1, 0);
  mRecalboxLogo.setPosition(mSize.x() - horizontalPadding, 0 + horizontalPadding);
  mRecalboxLogo.setResize(mSize.x() * 0.09f, mSize.x() * 0.09f);
  mRecalboxLogo.setKeepRatio(true);
  mRecalboxLogo.setZIndex(50);
  mRecalboxLogo.setDefaultZIndex(50);
  mRecalboxLogo.setThemeDisabled(false);

  mOverlay.setImage(Path(":/white.png"), false);
  mOverlay.setOrigin(0, 1);
  mOverlay.setPosition(0, mSize.y());
  mOverlay.setSize(mSize.x(), mSize.y() * 0.13f);
  mOverlay.setColor(0x9F9F9FCC);
  mOverlay.setZIndex(40);
  mOverlay.setDefaultZIndex(0);
  mOverlay.setThemeDisabled(false);

  mImage.setOrigin(0, 1);
  mImage.setPosition(mSize.x() * 0.025f, mSize.y() * 0.92f);
  mImage.setResize(mSize.x() * 0.2f, mSize.y() * 0.4f);
  mImage.setKeepRatio(true);
  mImage.setZIndex(50);
  mImage.setDefaultZIndex(40);
  mImage.setThemeDisabled(false);

  mGameName.setFont(&FontManager::Instance().FromDefault(FONT_SIZE_MEDIUM, false));
  mGameName.setOrigin(0, 1);
  mGameName.setPosition(mSize.x() * 0.03f, mSize.y());
  mGameName.setSize(mSize.x() * 0.94f, mSize.y() * 0.13f);
  mGameName.setZIndex(50);
  mGameName.setDefaultZIndex(50);
  mGameName.setAlignment(::Alignment::TopRight);
  mGameName.setColor(0xFFFFFFFF);
  mGameName.setThemeDisabled(false);

  mFavoriteIcon.setImage(Path(":/heart_filled.svg"), false);
  mFavoriteIcon.setOrigin(0, 1);
  mFavoriteIcon.setPosition(mSize.x() * 0.975f, mSize.y() * 0.91f);
  mFavoriteIcon.setResize(mSize.x() * 0.018f, mSize.x() * 0.018f);
  mFavoriteIcon.setKeepRatio(true);
  mFavoriteIcon.setZIndex(50);
  mFavoriteIcon.setDefaultZIndex(50);
  mFavoriteIcon.setThemeDisabled(false);

  mSystemName.setOrigin(0, 1);
  mSystemName.setPosition(mSize.x() * 0.03f, mSize.y());
  mSystemName.setSize(mSize.x() * 0.94f, mSize.y() * 0.13f);
  mSystemName.setZIndex(50);
  mSystemName.setDefaultZIndex(50);
  mSystemName.setAlignment(::Alignment::CenterRight);
  mSystemName.setFont(&FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false));
  mSystemName.setColor(0xFFFFFFFF);
  mSystemName.setThemeDisabled(false);

  mReleaseDate.setOrigin(0, 1);
  mReleaseDate.setPosition(mSize.x() * 0.03f, mSize.y());
  mReleaseDate.setSize(mSize.x() * 0.94f, mSize.y() * 0.13f);
  mReleaseDate.setZIndex(50);
  mReleaseDate.setDefaultZIndex(50);
  mReleaseDate.setAlignment(::Alignment::BottomRight);
  mReleaseDate.setFont(&FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false));
  mReleaseDate.setColor(0xFFFFFFFF);
  mReleaseDate.setThemeDisabled(false);
}

void GameClipView::initMDLabels()
{
  std::vector<TextComponent*> labels = getMDLabels();

  for (auto& textLabel : labels)
  {
    textLabel->setFont(&FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false));
    textLabel->setThemeDisabled(true);
    textLabel->setDefaultZIndex(50);
  }
}

void GameClipView::initMDValues()
{
  std::vector<TextComponent*> labels = getMDLabels();
  std::vector<ThemableComponent*> values = getMDValues();

  NewFont* defaultFont = &FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false);
  mRating.setSize((float)defaultFont->Height() * 5.0f, (float)defaultFont->Height());
  mDeveloper.setFont(defaultFont);
  mPublisher.setFont(defaultFont);
  mGenre.setFont(defaultFont);
  mPlayers.setFont(defaultFont);
  mLastPlayed.setFont(defaultFont);
  mPlayCount.setFont(defaultFont);
  mFavorite.setFont(defaultFont);
  mDescription.setFont(defaultFont);

  for (auto & value : values)
  {
    value->setDefaultZIndex(50);
    value->setThemeDisabled(true);
  }
}

void GameClipView::SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface)
{
  (void)refreshOnly;
  initComponents();
  if (theme.IsValid())
    if (String themeOption = RecalboxConf::Instance().GetThemeGameClipView(theme.RawName()); !themeOption.empty() && themeOption != ThemeData::getNoTheme())
    {
      mOverlay.DoApplyThemeElement(theme, GameClipView::getName(), "background", ThemePropertyCategory::All);
      mHeaderImage.DoApplyThemeElement(theme, GameClipView::getName(), "logo", ThemePropertyCategory::All);
      mHeaderText.DoApplyThemeElement(theme, GameClipView::getName(), "logoText", ThemePropertyCategory::All);

      // Remove old theme extras
      removeChild(&mThemeExtras);

      mThemeExtras.AssignExtras(theme, GameClipView::getName(), theme.GetExtras(GameClipView::getName(), mWindow, nullptr), false);

      // Add new theme extras
      addChild(&mThemeExtras);

      if (mHeaderImage.hasImage())
      {
        removeChild(&mHeaderText);
        addChild(&mHeaderImage);
      }
      else
      {
        addChild(&mHeaderText);
        removeChild(&mHeaderImage);
      }

      mImage.DoApplyThemeElement(theme, GameClipView::getName(), "md_image",
                                 ThemePropertyCategory::Position | ThemePropertyCategory::Size | ThemePropertyCategory::ZIndex |
                                 ThemePropertyCategory::Rotation);

      mThumbnail.DoApplyThemeElement(theme, GameClipView::getName(), "md_thumbnail",
                                     ThemePropertyCategory::Position | ThemePropertyCategory::Size | ThemePropertyCategory::ZIndex |
                                     ThemePropertyCategory::Rotation);

      mFavoriteIcon.DoApplyThemeElement(theme, GameClipView::getName(), "favoriteIcon",
                                        ThemePropertyCategory::All);

      mRecalboxLogo.DoApplyThemeElement(theme, GameClipView::getName(), "recalboxLogo", ThemePropertyCategory::All);

      mClippingImage.DoApplyThemeElement(mGame == nullptr ? ThemeManager::Instance().Main() : mGame->System().Theme(), GameClipView::getName(), "clippingImage",
                                         ThemePropertyCategory::Path | ThemePropertyCategory::Size);

      std::vector<TextComponent*> labels = getMDLabels();
      std::vector<String> names({
                                       "md_lbl_gameName",
                                       "md_lbl_systemName",
                                       "md_lbl_rating",
                                       "md_lbl_releasedate",
                                       "md_lbl_developer",
                                       "md_lbl_publisher",
                                       "md_lbl_genre",
                                       "md_lbl_players",
                                       "md_lbl_lastplayed",
                                       "md_lbl_playcount"
                                     });

      names.emplace_back("md_lbl_favorite");

      assert(names.size() == labels.size());
      for (unsigned int i = 0; i < (unsigned int) labels.size(); i++)
      {
        labels[i]->DoApplyThemeElement(theme, GameClipView::getName(), names[i], ThemePropertyCategory::All);
      }

      std::vector<ThemableComponent*> values = getMDValues();
      names = {
        "md_gameName",
        "md_systemName",
        "md_rating",
        "md_releasedate",
        "md_developer",
        "md_publisher",
        "md_genre",
        "md_players",
        "md_lastplayed",
        "md_playcount",
      };

      names.emplace_back("md_favorite");

      assert(names.size() == values.size());
      for (unsigned int i = 0; i < (unsigned int) values.size(); i++)
      {
        values[i]->DoApplyThemeElement(theme, GameClipView::getName(), names[i], ThemePropertyCategory::All ^ ThemePropertyCategory::Text);
      }

      mSystemName.DoApplyThemeElement(theme, GameClipView::getName(), "md_systemName",
                                      ThemePropertyCategory::All);
      mDescContainer.DoApplyThemeElement(theme, GameClipView::getName(), "md_description",
                                         ThemePropertyCategory::Position | ThemePropertyCategory::Size | ThemePropertyCategory::ZIndex);
      mDescription.setSize(mDescContainer.getSize().x(), 0.0);
      mDescription.DoApplyThemeElement(theme, GameClipView::getName(), "md_description", ThemePropertyCategory::All ^
                                                                                         (ThemePropertyCategory::Position |
                                                                                          ThemePropertyCategory::Size |
                                                                                          ThemePropertyCategory::Origin |
                                                                                          ThemePropertyCategory::Text));

      mVideo.DoApplyThemeElement(theme, GameClipView::getName(), "md_video",
                                 ThemePropertyCategory::Position | ThemePropertyCategory::Size | ThemePropertyCategory::ZIndex |
                                 ThemePropertyCategory::Rotation);

    }
  sortChildren();
  if (interface != nullptr) interface->ThemeSwitchTick();
}

// element order need to follow the one in onThemeChanged
std::vector<TextComponent*> GameClipView::getMDLabels()
{
  std::vector<TextComponent*> ret;
  ret.push_back(&mLbGameName);
  ret.push_back(&mLbSystemName);
  ret.push_back(&mLblRating);
  ret.push_back(&mLblReleaseDate);
  ret.push_back(&mLblDeveloper);
  ret.push_back(&mLblPublisher);
  ret.push_back(&mLblGenre);
  ret.push_back(&mLblPlayers);
  ret.push_back(&mLblLastPlayed);
  ret.push_back(&mLblPlayCount);
  ret.push_back(&mLblFavorite);
  return ret;
}

// element order need to follow the one in onThemeChanged
std::vector<ThemableComponent*> GameClipView::getMDValues()
{
  std::vector<ThemableComponent*> ret;
  ret.push_back(&mGameName);
  ret.push_back(&mSystemName);
  ret.push_back(&mRating);
  ret.push_back(&mReleaseDate);
  ret.push_back(&mDeveloper);
  ret.push_back(&mPublisher);
  ret.push_back(&mGenre);
  ret.push_back(&mPlayers);
  ret.push_back(&mLastPlayed);
  ret.push_back(&mPlayCount);
  ret.push_back(&mFavorite);
  return ret;
}

void GameClipView::setGameInfo(FileData* game)
{
  mGame = game;
  mSystem = &mGame->System();

  SwitchToTheme(mSystem->Theme(), true, nullptr);

  mSystemName.setValue(mGame->System().FullName());
  mGameName.setValue(mGame->Metadata().Name());
  mRating.setValue(mGame->Metadata().RatingAsString());
  mReleaseDate.setValue(mGame->Metadata().ReleaseDateEpoc() != 0 ? mGame->Metadata().ReleaseDate().ToStringFormat("%YYYY") : _("UNKNOWN"));
  mDeveloper.setValue(mGame->Metadata().Developer());
  mPublisher.setValue(mGame->Metadata().Publisher());
  mGenre.setValue(mGame->Metadata().Genre());
  mPlayers.setValue(mGame->Metadata().PlayersAsString());
  mLastPlayed.setValue(mGame->Metadata().LastPlayedAsString());
  mPlayCount.setValue(mGame->Metadata().PlayCountAsString());
  mVideo.setVideo(mGame->Metadata().Video(), 0, 1, AudioScheduler::Instance().CanDecodeVideoSound());

  mImage.setImage(mGame->Metadata().Image(), false);
  mThumbnail.setImage(mGame->Metadata().Image(), false);
  mDescription.setText(mGame->Metadata().Description());
  mDescContainer.reset();

  mLblFavorite.setText(_("Favorite") + ": ");
  addChild(&mLblFavorite);
  addChild(&mFavorite);
}

void GameClipView::CleanVideo()
{
  mVideo.setVideo(Path::Empty, false);
}
