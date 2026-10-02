#include <RecalboxConf.h>
#include "views/gamelist/DetailedGameListView.h"
#include "animations/LambdaAnimation.h"
#include "scraping/ScraperSeamless.h"
#include "recalbox/RecalboxStorageWatcher.h"
#include "guis/GuiScraperRun.h"
#include "usernotifications/NotificationManager.h"
#include "usernotifications/Notifications.h"
#include <views/ViewController.h>

#include "views/container/CarouselGamelist.h"

DetailedGameListView::DetailedGameListView(WindowManager&window, SystemManager& systemManager, SystemData& system, const IGlobalVariableResolver& resolver, PictogramCaches& pictogramCache)
  : ISimpleGameListView(window, systemManager, system, resolver, pictogramCache)
  , mEmptyListItem(FileSorts::Sorts::FileNameAscending, _("YOUR LIST IS EMPTY. PRESS START TO CHANGE GAME FILTERS."), system)
  , mPopulatedFolder(nullptr)
  //, mList(window, &system, *this, pictogramCache)
  , mCurrentList(nullptr)
  , mElapsedTimeOnGame(0)
  , mIsScraping(false)
  , mHeaderStars(window, 0.f, "dlvh")
  , mImage(window, ImgProps::KeepRatio | ImgProps::NoCache)
  , mNoImage(window, ImgProps::KeepRatio)
  , mVideo(window, this)
  , mRegions { ImageComponent(window), ImageComponent(window), ImageComponent(window), ImageComponent(window) }
  , mLblRating(window)
  , mLblReleaseDate(window)
  , mLblDeveloper(window)
  , mLblPublisher(window)
  , mLblGenre(window)
  , mLblPlayers(window)
  , mLblLastPlayed(window)
  , mLblPlayCount(window)
  , mLblFavorite(window)
  , mFolderName(window)
  , mRating(window, 0.f, "dlv")
  , mReleaseDate(window)
  , mDeveloper(window)
  , mPublisher(window)
  , mGenre(window)
  , mPlayers(window)
  , mLastPlayed(window)
  , mPlayCount(window)
  , mFavorite(window)
  , mDecorationFlagsTemplate(window)
  , mDecorationGenreTemplate(window)
  , mDecorationPlayersTemplate(window)
  , mDescContainer(window)
  , mDescription(window)
  , mBusy(window)
  , mSettings(RecalboxConf::Instance())
  , mSort(FileSorts::Sorts::FileNameAscending)
  , mLastCursorItem(nullptr)
  , mLastCursorItemHasP2K(false)
  , mLayout(mSettings.GetSystemGamelistLayout(system))
{
}

void DetailedGameListView::Initialize()
{
  // folder components
  for (int i = sFoldersMaxGameImageCount; --i >= 0; )
  {
    auto* img = new PictureComponent(mWindow);
    addChild(img); // normalised functions required to be added first
    img->setOrigin(0.5f, 0.5f);
    img->setNormalisedSize(0.3f, 0.3f);
    img->setKeepRatio(true),
    img->setDefaultZIndex(30);
    img->setZIndex(30);
    img->setOpacity(0);
    mFolderContent.push_back(img);
  }

  SwitchLayout(mLayout);
  assert(mCurrentList != nullptr && "No gamelist container !");
  addChild(&mCurrentList->AsComponent());

  const float padding = 0.01f;

  Gamelist().AsComponent().setDefaultZIndex(20);
  Gamelist().AsComponent().setPosition(mSize.x() * (0.50f + padding), Gamelist().AsComponent().getPosition().y());
  Gamelist().AsComponent().setSize(mSize.x() * (0.50f - padding), Gamelist().AsComponent().getSize().y());


  addChild(&mFolderName);
  mFolderName.setDefaultZIndex(40);

  // image
  mImage.setOrigin(0.5f, 0.5f);
  mImage.setPosition(mSize.x() * 0.25f, Gamelist().AsComponent().getPosition().y() + (mSize.y() * 0.2125f));
  mImage.setResize(mSize.x() * (0.50f - 2 * padding), mSize.y() * 0.4f);
  mImage.setDefaultZIndex(30);
  mImage.setOpacity(0);

  // no image
  mNoImage.setOrigin(mImage.getOrigin());
  mNoImage.setPosition(mImage.getPosition());
  mNoImage.setResize(mImage.getSize());
  mNoImage.setDefaultZIndex(30);
  mNoImage.setOpacity(0);

  addChild(&mNoImage);
  addChild(&mImage);

  // video
  mVideo.setOrigin(0.5f, 0.5f);
  mVideo.setPosition(mSize.x() * 0.25f, getPosition().y() + (mSize.y() * 0.2125f));
  mVideo.setResize(mSize.x() * (0.50f - 2 * padding), mSize.y() * 0.4f);
  mVideo.setKeepRatio(true);
  mVideo.setDefaultZIndex(30);
  addChild(&mVideo);

  // Busy
  mBusy.setPosition(mImage.getPosition());
  mBusy.setSize(mImage.getSize());
  mBusy.SetText(_("UPDATING..."));

  // metadata labels + values
  addChild(&mLblRating);
  addChild(&mRating);
  addChild(&mLblReleaseDate);
  addChild(&mReleaseDate);
  addChild(&mLblDeveloper);
  addChild(&mDeveloper);
  addChild(&mLblPublisher);
  addChild(&mPublisher);
  addChild(&mLblGenre);
  addChild(&mGenre);
  addChild(&mLblPlayers);
  addChild(&mPlayers);
  addChild(&mLblLastPlayed);
  mLastPlayed.setDisplayMode(DateTimeComponent::Display::RelativeToNow);
  addChild(&mLastPlayed);
  addChild(&mLblPlayCount);
  addChild(&mPlayCount);
  addChild(&mLblFavorite);
  addChild(&mFavorite);

  for (int i = Regions::RegionPack::sMaxRegions; --i >= 0; )
  {
    addChild(&mRegions[i]); // normalised functions required to be added first
    mRegions[i].setDefaultZIndex(40);
    //mRegions[i].setThemeDisabled(true);
  }

  mDescContainer.setPosition(mSize.x() * padding, mSize.y() * 0.65f);
  mDescContainer.setSize(mSize.x() * (0.50f - 2 * padding), mSize.y() - mDescContainer.getPosition().y());
  mDescContainer.setDefaultZIndex(40);
  addChild(&mDescContainer);

  mDescription.setFont(&FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false));
  mDescription.setMultiline(true);
  mDescription.setSize(mDescContainer.getSize().x(), 0);
  mDescContainer.addChild(&mDescription);

  initMDLabels();
  initMDValues();
  OnGameSelected(Gamelist(), nullptr, -1);
}

void DetailedGameListView::SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface)
{
  ISimpleGameListView::SwitchToTheme(theme, refreshOnly, interface);

  Gamelist().AsThemable().DoApplyThemeElement(theme, getName(), "gamelist", ThemePropertyCategory::All);
  // Set color 2/3 50% transparent of color 0/1
  Gamelist().SetColor(GameHighlightColor, (Gamelist().GetColor(GameColor) & 0xFFFFFF00) | ((Gamelist().GetColor(GameColor) & 0xFF) >> 1));
  Gamelist().SetColor(FolderHighlightColor, (Gamelist().GetColor(FolderColor) & 0xFFFFFF00) | ((Gamelist().GetColor(FolderColor) & 0xFF) >> 1));
  Gamelist().SetColor(HeaderColor, /*(mList.Color(GameColor) & 0xFFFFFF00) |*/ 0x40);
  sortChildren();

  for (int i = 0; i < (int)Regions::RegionPack::sMaxRegions; i++)
    mRegions[i].DoApplyThemeElement(theme, getName(), String("md_region").Append(i + 1).c_str(),
                                    ThemePropertyCategory::Position | ThemePropertyCategory::Size | ThemePropertyCategory::ZIndex | ThemePropertyCategory::Path);

  mImage.DoApplyThemeElement(theme, getName(), "md_image", ThemePropertyCategory::All ^ ThemePropertyCategory::Path);
  mNoImage.DoApplyThemeElement(theme, getName(), "md_image", ThemePropertyCategory::All ^ ThemePropertyCategory::Path);
  mNoImage.setImage(Path(":/no_image.png"), false);
  mNoImage.DoApplyThemeElement(theme, getName(), "default_image_path", ThemePropertyCategory::Path);
  mNoImage.setThemeDisabled(false);
  mVideo.DoApplyThemeElement(theme, getName(), "md_video", ThemePropertyCategory::All ^ ThemePropertyCategory::Path);

  mDecorationFlagsTemplate.DoApplyThemeElement(theme, getName(), "template_flag", ThemePropertyCategory::Color);
  mDecorationGenreTemplate.DoApplyThemeElement(theme, getName(), "template_genre", ThemePropertyCategory::Color);
  mDecorationPlayersTemplate.DoApplyThemeElement(theme, getName(), "template_players", ThemePropertyCategory::Color);

  BuildVideoLinks(theme);

  initMDLabels();
  Array<TextComponent*> labels = getMDLabels();
  std::vector<String> names({
                                   "md_lbl_rating",
                                   "md_lbl_releasedate",
                                   "md_lbl_developer",
                                   "md_lbl_publisher",
                                   "md_lbl_genre",
                                   "md_lbl_players",
                                   "md_lbl_lastplayed",
                                   "md_lbl_playcount"
                                 });

  names.push_back("md_lbl_favorite");

  assert((int)names.size() == labels.Count());
  for (int i = 0; i < labels.Count(); i++)
  {
    labels[i]->DoApplyThemeElement(theme, getName(), names[i], ThemePropertyCategory::All);
  }

  initMDValues();
  Array<ThemableComponent*> values = getMDValues();
  names = {
    "md_rating",
    "md_releasedate",
    "md_developer",
    "md_publisher",
    "md_genre",
    "md_players",
    "md_lastplayed",
    "md_playcount"
  };

  names.push_back("md_favorite");

  names.push_back("md_folder_name");
  values.Add(&mFolderName);

  assert((int)names.size() == values.Count());
  for (int i = 0; i < values.Count(); i++)
  {
    values[i]->DoApplyThemeElement(theme, getName(), names[i], ThemePropertyCategory::All ^ ThemePropertyCategory::Text);
  }

  //mHeaderStars.DoApplyThemeElement(theme, getName(), "md_rating", ThemePropertyCategory::All);
  mDescContainer.DoApplyThemeElementPolymorphic(theme, getName(), "md_description", ThemePropertyCategory::Position | ThemePropertyCategory::Size | ThemePropertyCategory::ZIndex);
  mDescription.setSize(mDescContainer.getSize().x(), 0);
  mDescription.DoApplyThemeElement(theme, getName(), "md_description",
                                   ThemePropertyCategory::All ^ (ThemePropertyCategory::Position | ThemePropertyCategory::Size | ThemePropertyCategory::Origin | ThemePropertyCategory::Text));
  mBusy.SetFont(mDescription.getFont());

  mLblRating.setText(_("Rating") + ": ");
  mLblReleaseDate.setText(_("Released") + ": ");
  mLblDeveloper.setText(_("Developer") + ": ");
  mLblPublisher.setText(_("Publisher") + ": ");
  mLblGenre.setText(_("Genre") + ": ");
  mLblPlayers.setText(_("Players") + ": ");
  mLblLastPlayed.setText(_("Last played") + ": ");
  mLblPlayCount.setText(_("Times played") + ": ");
  mLblFavorite.setText(_("Favorite") + ": ");

  if (theme.isFolderHandled())
  {
    mFolderName.DoApplyThemeElement(theme, getName(), "md_folder_name", ThemePropertyCategory::All);
    for (int i = 0; i < (int)mFolderContent.size(); i++)
    {
      String folderImage("md_folder_image_"); folderImage.Append(i);
      mFolderContent[i]->DoApplyThemeElement(theme, getName(), folderImage,
                                             ThemePropertyCategory::Position | ThemePropertyCategory::Size | ThemePropertyCategory::ZIndex | ThemePropertyCategory::Rotation);
    }
  }
  else
  {
    // backward compatibility
    auto size = mImage.getSize().isZero() ? mImage.getTargetSize() : mImage.getSize();
    float minSize = Math::min(size.x(), size.y());
    float left = mImage.getPosition().x() - (mImage.getOrigin().x() * minSize);
    float top = mImage.getPosition().y() - (mImage.getOrigin().y() * minSize);

    mFolderName.setPosition(left, top);
    mFolderName.setZIndex(40);
    mFolderName.setFont(&FontManager::Instance().FromDefault(FONT_SIZE_EXTRASMALL, false));
    mFolderName.setColor(0xFFFFFFFF);

    const unsigned int grid = 3; // 3 x 3
    const float relativeMarge = 0.1f;

    float labelHeight = 2.5f * (float)FontManager::Instance().FromDefault(FONT_SIZE_EXTRASMALL, false).Height();
    top += labelHeight;

    minSize = Math::min(size.x(), size.y() - labelHeight);

    const float imgSize = minSize / (grid + 2.0f * relativeMarge);

    // centering with unused space
    left += 0.5f * (minSize - grid * imgSize - 2.0f * relativeMarge);

    for (unsigned int x = 0; x < grid; x++)
    {
      for (unsigned int y = 0; y < grid; y++)
      {
        PictureComponent* img = mFolderContent[x + (y * grid)];
        img->setSize(imgSize, imgSize);
        img->setKeepRatio(true);
        img->setPosition(left + (imgSize * img->getOrigin().x()) + ((float)x * (1 + relativeMarge) * imgSize),
                         top + (imgSize * img->getOrigin().y()) + ((float)y * (1 + relativeMarge) * imgSize));
        img->setZIndex(30);
      }
    }
  }

  sortChildren();
  if (refreshOnly) mLastCursorItem = nullptr;
  DoUpdateGameInformation(false);
  if (mCurrentList->HasGame())
    ThemeManager::ChangeThemeContext(getName(), mThemeExtras.Extras(), &mSystem, mCurrentList->GetSelectedGame());
  (void)interface;
}

const char* DetailedGameListView::getName() const
{
  switch(mLayout)
  {
    case RecalboxConf::GamelistLayout::Default:
    case RecalboxConf::GamelistLayout::Legacy: break;
    case RecalboxConf::GamelistLayout::LogoList: if (mSystem.Theme().HasLogoView()) return "gamelistlogo"; break;
    case RecalboxConf::GamelistLayout::Carousel: if (mSystem.Theme().HasCarouselView()) return "gamelistcarousel"; break;
    case RecalboxConf::GamelistLayout::Grid: if (mSystem.Theme().HasGridView()) return "gamelistgrid"; break;
  }
  return "detailed";
}

void DetailedGameListView::initMDLabels()
{
  Array<TextComponent*> components = getMDLabels();

  //const unsigned int colCount = 2;
  const unsigned int rowCount = (unsigned int) (components.Count() / 2);

  Vector3f start(mSize.x() * 0.01f, mSize.y() * 0.625f, 0.0f);

  //const float colSize = (mSize.x() * 0.48f) / colCount;
  const float rowPadding = 0.01f * mSize.y();

  Vector3f pos = start + Vector3f(0, 0, 0);
  for (int i = 0; i < components.Count(); i++)
  {
    const unsigned int row = i % rowCount;
    if (row != 0)
    {
      // work from the last component
      Component* lc = components[i - 1];
      pos = lc->getPosition() + Vector3f(0, lc->getSize().y() + rowPadding, 0);
    }

    components[i]->setFont(&FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false));
    components[i]->setPosition(pos);
    components[i]->setDefaultZIndex(40);
  }
}

void DetailedGameListView::initMDValues()
{
  Array<TextComponent*> labels = getMDLabels();
  Array<ThemableComponent*> values = getMDValues();

  NewFont* defaultFont = &FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false);
  mRating.setSize((float)defaultFont->Height() * 5.0f, (float)defaultFont->Height());
  mReleaseDate.setFont(defaultFont);
  mDeveloper.setFont(defaultFont);
  mPublisher.setFont(defaultFont);
  mGenre.setFont(defaultFont);
  mPlayers.setFont(defaultFont);
  mLastPlayed.setFont(defaultFont);
  mPlayCount.setFont(defaultFont);
  mFavorite.setFont(defaultFont);

  float bottom = 0.0f;

  const float colSize = (mSize.x() * 0.48f) / 2;
  for (int i = 0; i < labels.Count(); i++)
  {
    const float heightDiff = (labels[i]->getSize().y() - values[i]->getSize().y()) / 2;
    values[i]->setPosition(labels[i]->getPosition() + Vector3f(labels[i]->getSize().x(), heightDiff, 0));
    values[i]->setSize(colSize - labels[i]->getSize().x(), values[i]->getSize().y());
    values[i]->setDefaultZIndex(40);

    float testBot = values[i]->getPosition().y() + values[i]->getSize().y();
    if (testBot > bottom) bottom = testBot;
  }

  mDescContainer.setPosition(mDescContainer.getPosition().x(), bottom + (mSize.y() * 0.01f));
  mDescContainer.setSize(mDescContainer.getSize().x(), mSize.y() - mDescContainer.getPosition().y());
}

bool DetailedGameListView::CollectHelpItems(Help& help)
{
  ISimpleGameListView::CollectHelpItems(help);
  FileData* item = Gamelist().GetSelectedGame();
  if (item->IsGame())
  {
    bool isOneGameOneRomMode = !IsFavoriteSystem() && RecalboxConf::Instance().GetOneGameOneRom();
    if (isOneGameOneRomMode && (m1G1R.try_get(item)) != nullptr)
      help.Set(HelpType::HkLeftRight, _("FOLD/UNFOLD"));
  }
  return true;
}

void DetailedGameListView::DoUpdateGameInformation(bool update)
{
  FileData* file = (!Gamelist().HasGame() || Gamelist().IsScrolling()) ? nullptr : Gamelist().GetSelectedGame();

  // Reset null game
  if (file == nullptr)
  {
    MoveToFadeOut(getFolderComponents());
    MoveToFadeOut(getGameComponents());
  }
  else
  {
    const bool isFolder = file->IsFolder();
    const bool isHeader = file->IsHeader();
    const bool hasImage = !file->Metadata().Image().IsEmpty();

    if (hasImage && isFolder)
    {
      setRegions(nullptr); // No region for folders
      MoveToFadeOut(getFolderComponents());
      MoveToFadeOut(getGameComponents(false));
      MoveToFadeIn(getScrapedFolderComponents());

      setScrapedFolderInfo(file);
    }
    else
    {
       if (isHeader)
       {
         setRegions(nullptr); // No region for folders
         MoveToFadeIn(getFolderComponents());
         MoveToFadeOut(getGameComponents());

         if (file != mLastCursorItem)
           SetHeaderInfo(file, Gamelist().GetHeaderGames((HeaderData*)file));
       }
       else if (isFolder)
       {
         setRegions(nullptr); // No region for folders
         MoveToFadeIn(getFolderComponents());
         MoveToFadeOut(getGameComponents());

         if (file != mLastCursorItem)
           ViewController::Instance().FetchSlowDataFor(file);
       }
       else
       {
         MoveToFadeOut(getFolderComponents());
         MoveToFadeIn(getGameComponents());

         setGameInfo(file, update);
       }
    }
  }

  // Reset p2k status (except if the last item is the same
  if (file != mLastCursorItem) mLastCursorItemHasP2K = false;
  mWindow.UpdateHelpSystem();
  // Update p2k status
  if (file != nullptr && file != mLastCursorItem)
  {
    if (file->IsGame())
      ViewController::Instance().FetchSlowDataFor(file);
    else if (file->IsFolder() || file->IsHeader()) // Kill video on multi-thumbnail folder
      mVideo.setVideo(Path::Empty, false);
  }

  // Update last procesed item
  mLastCursorItem = file;
  // Reset seamless scraping timer
  if (file != nullptr && file->IsGame()) mElapsedTimeOnGame = 0;
}

Array<ThemableComponent*> DetailedGameListView::getFolderComponents()
{
  Array<ThemableComponent*> comps;
  for (auto* img: mFolderContent)
  {
    comps.Add(img);
  }
  comps.Add(&mFolderName);
  return comps;
}

Array<ThemableComponent*> DetailedGameListView::getGameComponents(bool includeMainComponents)
{
  Array<ThemableComponent*> comps = getMDValues();
  if (includeMainComponents)
  {
    comps.Add(&mNoImage);
    comps.Add(&mImage);
    comps.Add(&mVideo);
    comps.Add(&mDescription);
  }
  for(TextComponent* component : getMDLabels())
    comps.Add(component);
  return comps;
}

Array<ThemableComponent*> DetailedGameListView::getScrapedFolderComponents()
{
  Array<ThemableComponent*> comps;
  comps.Add(&mNoImage);
  comps.Add(&mImage);
  comps.Add(&mVideo);
  comps.Add(&mDescription);
  return comps;
}

void DetailedGameListView::SetFolderInfo(FolderData* folder, int count, const FolderImagesPath& path)
{
  String gameCount(_N("%i GAME AVAILABLE", "%i GAMES AVAILABLE", count));
  gameCount.Replace("%i", String(count));
  mFolderName.setText(folder->Name() + " - " + gameCount);

  for (int i = sFoldersMaxGameImageCount; --i >= 0;)
    mFolderContent[i]->setImage(path[i], false);
}

void DetailedGameListView::SetHeaderInfo(FileData* header, const FileData::List& games)
{
  String gameCount(_N("%i GAME AVAILABLE", "%i GAMES AVAILABLE", games.Count()));
  gameCount.Replace("%i", String(games.Count()));
  mFolderName.setText(header->Name() + " - " + gameCount);

  int i = 0;
  HashSet<Path> dups;
  for (const FileData* game : games)
    if (Path p(game->AnyMedia()); !p.IsEmpty() && !dups.contains(p))
    {
      dups.insert(p);
      mFolderContent[i]->setImage(p, false);
      if (++i == sFoldersMaxGameImageCount) break;
    }

  while(i < sFoldersMaxGameImageCount)
    mFolderContent[i++]->ClearImage();
}

void DetailedGameListView::SetImageFading(FileData* game, bool videoStillRunning)
{
  if (videoStillRunning)
  {
    MoveToFadeOut(&mImage);
    MoveToFadeOut(&mNoImage);
  }
  else if (game->Metadata().Image().Exists())
  {
    mImage.setImage(game->Metadata().Image(), false);
    MoveToFadeIn(&mImage);
    MoveToFadeOut(&mNoImage);
  }
  else
  {
    MoveToFadeOut(&mImage);
    MoveToFadeIn(&mNoImage);
  }
}

void DetailedGameListView::setGameInfo(FileData* file, bool update)
{
  (void)update;
  setRegions(file);

  MetadataDescriptor& meta = file->Metadata();
  mRating.setValue(meta.RatingAsString());
  if (meta.ReleaseDateEpoc() != 0) mReleaseDate.setValue(meta.ReleaseDate().ToStringFormat("%YYYY-%MM"));
  else mReleaseDate.setValue(_("UNKNOWN"));
  mDeveloper.setValue(meta.Developer().empty() ? _("UNKNOWN") : meta.Developer());
  mPublisher.setValue(meta.Publisher().empty() ? _("UNKNOWN") : meta.Publisher());
  mGenre.setValue(meta.GenreId() != GameGenres::None ? Genres::GetFullNameStandalone(meta.GenreId()) :
                  !meta.Genre().empty() ? meta.Genre().Replace(',', ", ", 2).Replace("  ", 2, ' ') :
                  _("NONE"));
  mPlayers.setValue(meta.PlayersAsString());
  mLastPlayed.setValue(meta.LastPlayed());
  mPlayCount.setValue(meta.PlayCountAsString());
  mFavorite.setValue(meta.Favorite() ? _("YES") : _("NO"));

  mBusy.setPosition(mImage.getPosition());
  mBusy.setSize(mImage.getSize());
  mBusy.setOrigin(mImage.getOrigin());

  if (!mSettings.AsBool("system.secondminitft.enabled", false) ||
      !mSettings.AsBool("system.secondminitft.disablevideoines", false))
  {
    mVideo.setVideo(meta.Video(), AudioScheduler::Instance().CanDecodeVideoSound());
    { LOG(LogDebug) << "[GamelistView] Set video " << meta.Video().ToString() << " for " << meta.Name() << " => " << file->RomPath().ToString(); }
  }

  SetImageFading(file, mVideo.isDiplayed());

  mDescription.setText(GetDescription(*file));
  mDescContainer.reset();
}

void DetailedGameListView::setScrapedFolderInfo(FileData* file)
{
  SetImageFading(file, false);
  mVideo.setVideo(Path::Empty, false);
  mDescription.setText(GetDescription(*file));
  mDescContainer.reset();
}

void DetailedGameListView::launch(FileData* game)
{
  mVideo.setVideo(Path::Empty, false);

  Vector3f target(Renderer::Instance().DisplayWidthAsFloat() / 2.0f, Renderer::Instance().DisplayHeightAsFloat() / 2.0f, 0);
  if (mImage.hasImage())
    target.Set(mImage.getCenter().x(), mImage.getCenter().y(), 0);

  ViewController::Instance().Launch(game, GameLinkedData(), target, false);
}

// element order needs to follow the one in onThemeChanged
Array<TextComponent*> DetailedGameListView::getMDLabels()
{
  Array<TextComponent*> ret;
  ret.Add(&mLblRating);
  ret.Add(&mLblReleaseDate);
  ret.Add(&mLblDeveloper);
  ret.Add(&mLblPublisher);
  ret.Add(&mLblGenre);
  ret.Add(&mLblPlayers);
  ret.Add(&mLblLastPlayed);
  ret.Add(&mLblPlayCount);
  ret.Add(&mLblFavorite);
  return ret;
}

// element order needs to follow the one in onThemeChanged
Array<ThemableComponent*> DetailedGameListView::getMDValues()
{
  Array<ThemableComponent*> ret;
  ret.Add(&mRating);
  ret.Add(&mReleaseDate);
  ret.Add(&mDeveloper);
  ret.Add(&mPublisher);
  ret.Add(&mGenre);
  ret.Add(&mPlayers);
  ret.Add(&mLastPlayed);
  ret.Add(&mPlayCount);
  ret.Add(&mFavorite);
  return ret;
}

void DetailedGameListView::Update(int elapsed)
{
  ISimpleGameListView::Update(elapsed);

  mBusy.Enable(mIsScraping);
  mBusy.Update(elapsed);

  // Process fade-in list
  for(Component* component : mFadeInList)
    if (component->getOpacity() != 255)
      component->setOpacity(Math::clampi(component->getOpacity() + elapsed, 0, 255));

  // Process fade-out list
  for(Component* component : mFadeOutList)
    if (component->getOpacity() != 0)
      component->setOpacity(Math::clampi(component->getOpacity() - elapsed, 0, 255));

  // Cancel video
  if (Gamelist().IsScrolling()) // #TODO: check usefullness w/ instanciated video engine
    mVideo.setVideo(Path::Empty, false);

  if (!mSystem.IsScreenshots() && !GuiScraperRun::IsRunning())
  {
    // Need busy animation?
    ScraperSeamless& scraper = ScraperSeamless::Instance();
    FileData* game = Gamelist().GetSelectedGame();
    mIsScraping = false;
    if (game != nullptr)
      if (game->IsGame())
      {
        // Currently scraping?
        mIsScraping = (scraper.HowLong(game) > sMaxScrapingTimeBeforeBusyAnim);
        // Or start scraping?
        if (mElapsedTimeOnGame >= 0) // Valid timer?
          if (mElapsedTimeOnGame += elapsed; mElapsedTimeOnGame > sMaxHoveringTimeBeforeScraping) // Enough time on game?
          {
            // Shutdown timer for the current game
            mElapsedTimeOnGame = -1;
            // Push game into the seamless scraper
            scraper.Push(game, this);
          }
      }
  }
}

void DetailedGameListView::Render(const Transform4x4f& parentTrans)
{
  Transform4x4f trans = parentTrans * getTransform();

  renderChildren(trans);

  Renderer::SetMatrix(trans);
  //Renderer::DrawRectangle(mBusy.getPosition().x(), mBusy.getPosition().y(), mBusy.getSize().x(), mBusy.getSize().y(), 0x00000080);
  mBusy.Render(trans);
}

DetailedGameListView::~DetailedGameListView()
{
  for(int i = (int)mFolderContent.size(); --i >= 0; )
    delete mFolderContent[i];
  mFolderContent.clear();
  Free1G1R();
}

void DetailedGameListView::setRegions(FileData* file)
{
  if (file != nullptr)
  {
    for (int r = Regions::RegionPack::sMaxRegions; --r >= 0;)
      if (Regions::GameRegions region = file->Metadata().Region().Regions[r]; region != Regions::GameRegions::Unknown)
      {
        int flagHeight = Gamelist().GetTextHeight();
        Texture flagTexture = mPictogramCaches.GetFlag(region, flagHeight);
        //mRegions[r].setImage(flagTexture); #TODO: Change !
      }
      else mRegions[r].setImage(Path::Empty, false);
  }
  else for (int r = Regions::RegionPack::sMaxRegions; --r >= 0;)
      mRegions[r].setImage(Path::Empty, false);
}

void DetailedGameListView::ScrapingStageCompleted(FileData* game, Stage stage, MetadataType changes)
{
  // Got result, from the seamless scraper, update game data!
  if (game == Gamelist().GetSelectedGame())
    switch(stage)
    {
      case Stage::Text:
      {
        DoUpdateGameInformation(false);
        // Game name
        if ((changes & MetadataType::Name) != 0)
          Gamelist().RenameAt(Gamelist().GetSelectedGameIndex(), GetRawDisplayName(*game));
        break;
      }
      case Stage::Images:
      {
        if ((changes & (MetadataType::Image | MetadataType::Box)) != 0)
          DoUpdateGameInformation(true);
        break;
      }
      case Stage::Video:
      {
        if ((changes & MetadataType::Video) != 0)
          DoUpdateGameInformation(false);
        break;
      }
      case Stage::Extra: break; // Nothing to do with extra data for now
      case Stage::Completed: RecalboxStorageWatcher::CheckStorageFreeSpace(mWindow, mSystemManager.GetMountMonitor(), game->RomPath()); break;
      default: break;
    }
  else
    if (stage == Stage::Text)
      if ((changes & MetadataType::Name) != 0)
        Gamelist().Rename(game, GetRawDisplayName(*game));

  { LOG(LogDebug) << "[Scraper] Scraper stage: " << (int)stage; }
}

void DetailedGameListView::SwitchLayout(RecalboxConf::GamelistLayout layout)
{
  FileData* selectedGame = mCurrentList != nullptr ? mCurrentList->GetSelectedGame() : nullptr;
  if (mCurrentList != nullptr) removeChild(&mCurrentList->AsComponent());
  delete mCurrentList;
  mCurrentList = nullptr;

  switch(layout)
  {
    // Default is resolved before reaching this point; text list as a safety net
    case RecalboxConf::GamelistLayout::Default:
    case RecalboxConf::GamelistLayout::Legacy: mCurrentList = new TextGamelist(mWindow, &mSystem, *this, mPictogramCaches); break;
    case RecalboxConf::GamelistLayout::LogoList: mCurrentList = new LogoGamelist(mWindow, mSystem, *this, mPictogramCaches); break;
    case RecalboxConf::GamelistLayout::Carousel: mCurrentList = new CarouselGamelist(mWindow, mSystem, *this, mPictogramCaches); break;
    case RecalboxConf::GamelistLayout::Grid: break;
  }

  if (mCurrentList != nullptr)
  {
    addChild(&mCurrentList->AsComponent());
    Gamelist().AsComponent().setDefaultZIndex(20);
    if (mPopulatedFolder != nullptr)
    {
      populateList(*mPopulatedFolder, selectedGame);
      Gamelist().SetSelectedGame(selectedGame, true);
    }
    SwitchToTheme(mSystem.Theme(), true, nullptr);
  }
  else { LOG(LogFatal) << "[DetailedGamelistView] No gamelist from layout !"; }
}

String DetailedGameListView::GetRawDisplayName(const FileData& game)
{
  return mConf.GetDisplayByFileName() ? game.Metadata().RomFileOnly().FilenameWithoutExtension() : game.Name();
}

void DetailedGameListView::populateList(const FolderData& folder, FileData* target)
{
  mPopulatedFolder = &folder;
  Gamelist().ClearGames();
  mHeaderText.setText(mSystem.FullName());

  // Default filter
  FileData::Filter includesFilter = FileData::Filter::Normal | FileData::Filter::Favorite;
  // Favorites only?
  if (RecalboxConf::Instance().GetFavoritesOnly()) includesFilter = FileData::Filter::Favorite;

  // Get items
  bool flatfolders = mSystem.IsAlwaysFlat() || (RecalboxConf::Instance().GetSystemFlatFolders(mSystem));
  mGamelist.Clear();
  if (flatfolders) folder.GetItemsRecursivelyTo(mGamelist, includesFilter, mSystem.Excludes(), false);
  else folder.GetItemsTo(mGamelist, includesFilter, mSystem.Excludes(), true);

  // Check emptyness
  if (mGamelist.Empty())
  {
    if (!Gamelist().IsInRootFolder())
    {
      FolderData* current = Gamelist().GetCurrentFolder();
      populateList(*Gamelist().ExitFolder(), target);
      Gamelist().SetSelectedGame(current, true);
      return;
    }
    // Otherwise, insert en emergency empty item
    mGamelist.Add(&mEmptyListItem); // Insert "EMPTY SYSTEM" item
  }

  // Clean-up favorite child flags
  for(FileData* game : mGamelist)
    if (game->IsGame())
      game->Metadata().SetHasFavoriteChild(false);

  // Sort
  FileSorts::SortSets set = mSystem.IsVirtual() ? FileSorts::SortSets::MultiSystem :
                            mSystem.Descriptor().IsArcade() ? FileSorts::SortSets::Arcade :
                            FileSorts::SortSets::SingleSystem;
  FileSorts::Sorts sort = mSystem.IsSelfSorted() ? mSystem.FixedSort() :
                          FileSorts::Clamp(RecalboxConf::Instance().GetSystemSort(mSystem), set);
  FileSorts& sorts = FileSorts::Instance();
  FolderData::Sort(mGamelist, sorts.ComparerFromSort(sort, mSystem.IsLastPlayed() || mSystem.IsFavorite()), sorts.IsAscending(sort));
  SetInternalSort(sort);

  bool isOneGameOneRomMode = !IsFavoriteSystem() && RecalboxConf::Instance().GetOneGameOneRom();
  // compute one game on rom structs parent and children and filter only parent items
  if (isOneGameOneRomMode)
    GeneratePreferredOneGameOneRoms(mGamelist);

  BuildList(target);
}

void DetailedGameListView::BuildList(FileData* target)
{
  bool isOneGameOneRomMode = !IsFavoriteSystem() && RecalboxConf::Instance().GetOneGameOneRom();
  if (isOneGameOneRomMode && target != nullptr)
  {
    mLastCursorItem = target;
  }

  // Reorder favorites
  bool hasTopFavorites = false;
  if (!mSystem.IsFavorite() && !mSystem.IsLastPlayed() && RecalboxConf::Instance().GetFavoritesFirst())
  {
    // Has any favorite?
    for(int t = mGamelist.Count() - 1, i = (mGamelist.Count() + 1) / 2; --i >= 0; )
      if (mGamelist[i]->Metadata().AnyFavorite() || mGamelist[t - i]->Metadata().AnyFavorite()) { hasTopFavorites = true; break; }
    // If we have favorite + favorite first + descending order, move favorites on top
    if (hasTopFavorites && !FileSorts::Instance().IsAscending(mSort))
    {
      int favoriteCount = 0;
      int favoriteBase = 0;
      for(int i = mGamelist.Count(); --i >= 0; )
        if (mGamelist[i]->Metadata().AnyFavorite())
        {
          for(int j = i + 1; --j >= 0; )
            if (mGamelist[j]->Metadata().AnyFavorite()) { favoriteCount++; favoriteBase = j; }
            else break;
          break;
        }
      for(int i = favoriteCount; --i >= 0; ) mGamelist.Swap(i, favoriteBase + i);
    }
  }

  // Region filtering?
  Regions::GameRegions currentRegion = Regions::Clamp(RecalboxConf::Instance().GetSystemRegionFilter(mSystem));
  bool activeRegionFiltering = false;
  if (currentRegion != Regions::GameRegions::Unknown)
  {
    Regions::List availableRegion = AvailableRegionsInGames(mGamelist);
    // Check if our region is in the available ones
    for(Regions::GameRegions region : availableRegion)
    {
      activeRegionFiltering = (region == currentRegion);
      if (activeRegionFiltering) break;
    }
  }

  // Add to list
  Gamelist().ClearGames(mGamelist.Count());
  FileData* previous = nullptr;
  HeaderData* lastHeader = nullptr;
  for (FileData* item : mGamelist)
  {
    String name = item->Name();
    // Region filtering?
    int colorIndexOffset = BaseColor;
    if (activeRegionFiltering)
      if (!Regions::IsIn4Regions(item->Metadata().Region().Pack, currentRegion))
        colorIndexOffset = HighlightColor;
    // Header?
    if (item->IsGame())
      if (HeaderData* header = NeedHeader(previous, item, hasTopFavorites); header != nullptr)
      {
        Gamelist().AddHeader(header, GameColor, HeaderColor);
        lastHeader = header;
      }
    // Store
    if (lastHeader == nullptr || !lastHeader->IsFolded())
    {
      OneGameOneRom** oGoR = nullptr;
      if (isOneGameOneRomMode && (oGoR = m1G1R.try_get(item)) != nullptr)
      {
        OneGameOneRom& hierarchy = **oGoR;

        // usefull for go to game from search or gameclip
        if (mLastCursorItem != nullptr && hierarchy.mChildren.Contains(mLastCursorItem))
          hierarchy.mFolded = false;

        Gamelist().AddGames(item, GetRawDisplayName(*item), hierarchy.mFolded ? GameType::ParentGameFolded : GameType::ParentGameUnfolded,
                            colorIndexOffset + (item->IsFolder() ? FolderColor : GameColor));

        if (!hierarchy.mFolded)
          for (FileData* child: hierarchy.mChildren)
            Gamelist().AddGames(child, GetRawDisplayName(*child), GameType::ChildGame, colorIndexOffset + (item->IsFolder() ? FolderColor : GameColor));
      }
      else
        Gamelist().AddGames(item, GetRawDisplayName(*item), GameType::Normal, colorIndexOffset + (item->IsFolder() ? FolderColor : GameColor));
    }

    if (item->IsGame())
      previous = item;
  }

  if (target != nullptr)
    Gamelist().SetSelectedGame(target, true);
  else
    (void)Gamelist().SetSelectedGameIndex(0, true);
}

Regions::List DetailedGameListView::AvailableRegionsInGames()
{
  bool regionIndexes[256];
  memset(regionIndexes, 0, sizeof(regionIndexes));
  // Run through all games
  for(int i = Gamelist().GetGameCount(); --i >= 0; )
  {
    const FileData& fd = *Gamelist().GetGameAt(i);
    unsigned int fourRegions = fd.Metadata().Region().Pack;
    // Set the 4 indexes corresponding to all 4 regions (Unknown regions will all point to index 0)
    regionIndexes[(fourRegions >>  0) & 0xFF] = true;
    regionIndexes[(fourRegions >>  8) & 0xFF] = true;
    regionIndexes[(fourRegions >> 16) & 0xFF] = true;
    regionIndexes[(fourRegions >> 24) & 0xFF] = true;
  }
  // Rebuild final list
  Regions::List list;
  for(int i = 0; i < (int)sizeof(regionIndexes); ++i )
    if (regionIndexes[i])
      list.push_back((Regions::GameRegions)i);
  // Only unknown region?
  if (list.size() == 1 && regionIndexes[0])
    list.clear();
  return list;
}

Regions::List DetailedGameListView::AvailableRegionsInGames(FileData::List& fdList)
{
  bool regionIndexes[256];
  memset(regionIndexes, 0, sizeof(regionIndexes));
  // Run through all games
  for(const FileData* fd : fdList)
  {
    unsigned int fourRegions = fd->Metadata().Region().Pack;
    // Set the 4 indexes corresponding to all 4 regions (Unknown regions will all point to index 0)
    regionIndexes[(fourRegions >>  0) & 0xFF] = true;
    regionIndexes[(fourRegions >>  8) & 0xFF] = true;
    regionIndexes[(fourRegions >> 16) & 0xFF] = true;
    regionIndexes[(fourRegions >> 24) & 0xFF] = true;
  }
  // Rebuild final list
  Regions::List list;
  for(int i = (int)sizeof(regionIndexes); --i >= 0; )
    if (regionIndexes[i])
      list.push_back((Regions::GameRegions)i);
  // Only unknown region?
  if (list.size() == 1 && regionIndexes[0])
    list.clear();
  return list;
}

void DetailedGameListView::RefreshItem(FileData* game)
{
  if (game == nullptr || !game->IsGame()) { LOG(LogError) << "[DetailedGameListView] Trying to refresh null or empty item"; return; }

  if (!Gamelist().Rename(game, GetRawDisplayName(*game)))
  { LOG(LogError) << "[DetailedGameListView] Trying to refresh a not found item"; return; }
  if (Gamelist().GetSelectedGame() == game) DoUpdateGameInformation(true);
}

void DetailedGameListView::UpdateSlowData(const SlowDataInformation& info)
{
  if (info.mItem == Gamelist().GetSelectedGame())
  {
    // Game? update p2k status
    if (info.mItem->IsGame())
    {
      mLastCursorItemHasP2K = info.mHasP2k;
      mWindow.UpdateHelpSystem();
    }
    else if (info.mItem->IsFolder())
      SetFolderInfo((FolderData*)info.mItem, info.mCount, info.mPathList);
  }
}

void DetailedGameListView::VideoComponentRequireAction(const VideoComponent* source, IVideoComponentAction::Action action)
{
  if (source == &mVideo)
    switch(action)
    {
      case Action::FadeIn:
      {
        MoveToFadeIn(mVideoLinks);
        SetImageFading(mLastCursorItem, false);
        break;
      }
      case Action::FadeOut:
      {
        MoveToFadeOut(mVideoLinks);
        break;
      }
      default: break;
    }
}

void DetailedGameListView::BuildVideoLinks(const ThemeData& theme)
{
  mVideoLinks.Clear();
  const ThemeElement* elem = theme.Element(getName(), "md_video", ThemeElementType::Video, ThemeElementType::None);
  if (elem != nullptr)
    if (elem->HasProperty(ThemePropertyName::Link))
      for(String& link : elem->AsString(ThemePropertyName::Link).Split(','))
      {
        link.Trim();
        if (link == "md_image") { mVideoLinks.Add(&mImage); mVideoLinks.Add(&mNoImage); }
        else if (Component* comp = mThemeExtras.Lookup(link); comp != nullptr) mVideoLinks.Add(comp);
      }
}

HeaderData* DetailedGameListView::NeedHeader(FileData* previous, FileData* next, bool hasTopFavorites)
{
  // Favorites
  if (hasTopFavorites)
  {
    if (next->Metadata().AnyFavorite())
    {
      if (previous == nullptr || (!previous->Metadata().AnyFavorite() && next->Metadata().AnyFavorite()))
        return GetHeader(mSort, _("Favorites"));
      return nullptr;
    }
    // Leaving favorite zone: reset previous so sort-specific branches emit a fresh header for the first non-favorite
    if (previous != nullptr && previous->Metadata().AnyFavorite() && !next->Metadata().AnyFavorite())
      previous = nullptr;
  }

  // Normal processing
  switch(mSort)
  {
    case FileSorts::Sorts::FileNameAscending:
    case FileSorts::Sorts::FileNameDescending:
    {
      if (hasTopFavorites)
      {
        // Leavinf favorite area
        if ((previous != nullptr && previous->Metadata().AnyFavorite() && !next->Metadata().AnyFavorite()) ||
            // List has favorites but not at top
            (previous == nullptr))
          return GetHeader(mSort, _("In alphabetical order"));
      }
      break;
    }
    case FileSorts::Sorts::SystemAscending:
    case FileSorts::Sorts::SystemDescending:
    {
      if (previous == nullptr || &previous->System() != &next->System())
        if (previous == nullptr || previous->System().SortingName() != next->System().SortingName())
        return GetHeader(mSort, next->System().SortingFullName());
      break;
    }
    case FileSorts::Sorts::RatingAscending:
    case FileSorts::Sorts::RatingDescending:
    {
      int previousRating = previous == nullptr ? -1 : Math::clampi(Math::roundi(previous->Metadata().Rating() * 10.f), 0, 10);
      int nextRating = Math::clampi(Math::roundi(next->Metadata().Rating() * 10.f), 0, 10);
      if (previousRating != nextRating)
        return GetHeader(mSort, nextRating != 0 ? (_F(_("RATED {0} / 10")) / nextRating)() : _("NOT RATED"), next->Metadata().Rating());
      break;
    }
    case FileSorts::Sorts::TotalPlayTimeAscending:
    case FileSorts::Sorts::TotalPlayTimeDescending:
    {
      int previousRange = -1;
      int nextRange = 0;
      // Get previous range
      if (previous != nullptr)
      {
        if (previous->Metadata().TotalPlayTime() == 0) previousRange = 0;
        else if (TimeSpan span(previous->Metadata().TotalPlayTime(), 0); span.TotalMinutes() < 30) previousRange = 1;
        else { int hours = (int)span.TotalHours(); previousRange = hours == 0 ? 2 : hours + 2; }
      }
      // Get next range
      if (next->Metadata().TotalPlayTime() == 0) nextRange = 0;
      else if (TimeSpan span(next->Metadata().TotalPlayTime(), 0); span.TotalMinutes() < 30) nextRange = 1;
      else { int hours = (int)span.TotalHours(); nextRange = hours == 0 ? 2 : hours + 2; }

      // Get header
      if (previousRange != nextRange)
      {
        String text;
        if (nextRange == 0) text = _("Never played");
        else if (nextRange == 1) text = _("Just a few minutes");
        else if (nextRange == 2) text = _("Less than an hour");
        else text = (_F(_("{0} hours")) / (nextRange - 2)).ToString();
        return GetHeader(mSort, text);
      }
      break;
    }
    case FileSorts::Sorts::PlayCountAscending:
    case FileSorts::Sorts::PlayCountDescending:
    {
      static int sPow10[7] = { 1, 10, 100, 1000, 10000, 100000, 1000000 };
      int previousLog = previous == nullptr ? -1 : (int)log10((float)previous->Metadata().PlayCount() + .01f);
      previousLog = Math::clampi(previousLog, 0, 6);
      int previousValue = previous == nullptr ? -1 : previous->Metadata().PlayCount() / sPow10[previousLog];
      int nextLog = (int)log10((float)next->Metadata().PlayCount() + .01f);
      nextLog = Math::clampi(nextLog, 0, 6);
      int nextValue = next->Metadata().PlayCount() / sPow10[nextLog];
      if (previousLog != nextLog || previousValue != nextValue)
      {
        String formatText = (nextLog == 0 && nextValue == 0) ? "Never played"
                          : (nextLog == 0) ? "{0} times"
                          : (nextLog <= 5) ? "More than {0} times"
                          : "irrepressible monomaniac";
        String text = (_F(_S(formatText)) / (nextValue * sPow10[nextLog]))();
        return GetHeader(mSort, text);
      }
      break;
    }
    case FileSorts::Sorts::LastPlayedAscending:
    case FileSorts::Sorts::LastPlayedDescending:
    {
      DateTime now;
      String previousTime = previous != nullptr ? GetSimplifiedElapsedTime(now - previous->Metadata().LastPlayed()) : String::Empty;
      String nextTime = GetSimplifiedElapsedTime(now - next->Metadata().LastPlayed());
      if (previousTime != nextTime)
        return GetHeader(mSort, nextTime);
      break;
    }
    case FileSorts::Sorts::PlayersAscending:
    case FileSorts::Sorts::PlayersDescending:
    {
      if (previous == nullptr || previous->Metadata().PlayerRange() != next->Metadata().PlayerRange())
      {
        bool monoPlayer = next->Metadata().PlayerMin() == next->Metadata().PlayerMax() &&
                          next->Metadata().PlayerMin() == 1;
        return GetHeader(mSort, monoPlayer ? _("One player") : (_F(_("{0} Players")) / next->Metadata().PlayersAsString())(),
                         (next->Metadata().PlayerMax() << 16) | next->Metadata().PlayerMin());
      }
      break;
    }
    case FileSorts::Sorts::DeveloperAscending:
    case FileSorts::Sorts::DeveloperDescending:
    {
      String prevString = previous != nullptr ? previous->Metadata().Developer().Trim().ToLowerCase() : "\t";
      String nextString = next->Metadata().Developer().Trim().ToLowerCase();
      if (prevString != nextString)
      {
        String developer(next->Metadata().Developer());
        if (developer.empty()) developer = _("Unknown developer");
        return GetHeader(mSort, developer);
      }
      break;
    }
    case FileSorts::Sorts::PublisherAscending:
    case FileSorts::Sorts::PublisherDescending:
    {
      String prevString = previous != nullptr ? previous->Metadata().Publisher().Trim().ToLowerCase() : "\t";
      String nextString = next->Metadata().Publisher().Trim().ToLowerCase();
      if (prevString != nextString)
      {
        String publisher(next->Metadata().Publisher());
        if (publisher.empty()) publisher = _("Unknown publisher");
        return GetHeader(mSort, publisher);
      }
      break;
    }
    case FileSorts::Sorts::GenreAscending:
    case FileSorts::Sorts::GenreDescending:
    {
      if (previous == nullptr || previous->Metadata().GenreId() != next->Metadata().GenreId())
        return GetHeader(mSort, Genres::GetFullNameStandalone(next->Metadata().GenreId()), (int)next->Metadata().GenreId());
      break;
    }
    case FileSorts::Sorts::ReleaseDateAscending:
    case FileSorts::Sorts::ReleaseDateDescending:
    {
      int year = next->Metadata().ReleaseDate().Year();
      if (previous == nullptr || previous->Metadata().ReleaseDate().Year() != year)
        return GetHeader(mSort, year <= 1970 ? _("Release date unknown") : (_F(_("Year {0}")) / year)());
      break;
    }
    case FileSorts::Sorts::RegionAscending:
    case FileSorts::Sorts::RegionDescending:
    {
      Regions::RegionPack regions = next->Metadata().Region();
      if (previous == nullptr || previous->Metadata().Region() != regions)
      {
        String header;
        for(int i = Regions::RegionPack::sMaxRegions; --i >= 0; )
          if (Regions::GameRegions region = regions.Regions[i]; region != Regions::GameRegions::Unknown)
          {
            if (!header.empty()) header.Append(", ", 2);
            header.Append(Regions::RegionFullName(region));
          }
        if (header.empty()) header = _("NO REGION");
        return GetHeader(mSort, header, regions.Pack);
      }
      break;
    }
  }
  return nullptr;
}

HeaderData* DetailedGameListView::GetHeader(FileSorts::Sorts sort, const String& name)
{
  HeaderData** header = mHeaderMap.try_get(name);
  if (header != nullptr) return *header;

  return mHeaderMap[name] = new HeaderData(sort, name, mSystem);
}

HeaderData* DetailedGameListView::GetHeader(FileSorts::Sorts sort, const String& name, int data)
{
  HeaderData** header = mHeaderMap.try_get(name);
  if (header != nullptr) return *header;

  return mHeaderMap[name] = new HeaderData(sort, name, mSystem, data, (float)data);
}

HeaderData* DetailedGameListView::GetHeader(FileSorts::Sorts sort, const String& name, float data)
{
  HeaderData** header = mHeaderMap.try_get(name);
  if (header != nullptr) return *header;

  return mHeaderMap[name] = new HeaderData(sort, name, mSystem, (int)data, data);
}

bool DetailedGameListView::ProcessInput(const InputCompactEvent& event)
{
  // Change sort anywhere
  if (event.AnyHotkeyCombination())
  {
    if (event.HotkeyL2Released()) { ChangeSort(false); return true; }
    if (event.HotkeyR2Released()) { ChangeSort(true); return true; }
  }

  return ISimpleGameListView::ProcessInput(event);
}

void DetailedGameListView::ChangeSort(bool next)
{
  FileSorts::SortSets set = mSystem.IsVirtual() ? FileSorts::SortSets::MultiSystem :
                            mSystem.Descriptor().IsArcade() ? FileSorts::SortSets::Arcade :
                            FileSorts::SortSets::SingleSystem;
  const FileSorts::SortList sorts = FileSorts::AvailableSorts(set);

  // Move sort
  int index = sorts.IndexOf(mSort);
  if (index < 0) index = 0;
  index += next ? 1 : -1;
  if (index < 0) index += sorts.Count();
  if (index >= sorts.Count()) index -= sorts.Count();
  SetInternalSort(sorts[index]);

  // Save new sort
  RecalboxConf::Instance().SetSystemSort(mSystem, mSort).Save();

  // Refresh
  FileData* item = Gamelist().GetSelectedGame();
  populateList(*mPopulatedFolder, nullptr);
  Gamelist().SetSelectedGame(item, true);

  // Notify
  String message = (_F(_("Game are now sorted\nby {0}")) / FileSorts::Instance().Name(mSort))();
  mWindow.InfoPopupAddRegular(message, 10, PopupType::Recalbox, false);
}

String DetailedGameListView::GetSimplifiedElapsedTime(const TimeSpan& span)
{
  if (span.TotalDays() > (365 * 20)) return _("Never played");
  if (span.TotalDays() > 365) return (_F(_("{0} Years ago...")) / (span.TotalDays() / 365))();
  if (span.TotalDays() >= 31) return (_F(_("{0} Month ago...")) / (int)((float)span.TotalDays() / 30.5f))();
  if (span.TotalDays() > 0) return (_F(_("{0} Days ago...")) / span.TotalDays())();
  if (span.TotalHours() > 0) return (_F(_("{0} Hours ago...")) / span.TotalHours())();
  return _("A short while ago");
}

void DetailedGameListView::ReturnedFromGame(FileData* game)
{
  (void)game;
  switch(mSort)
  {
    case FileSorts::Sorts::PlayCountAscending:
    case FileSorts::Sorts::PlayCountDescending:
    case FileSorts::Sorts::TotalPlayTimeAscending:
    case FileSorts::Sorts::TotalPlayTimeDescending:
    case FileSorts::Sorts::LastPlayedAscending:
    case FileSorts::Sorts::LastPlayedDescending:
    {
      // Refresh to reflect time played/last played
      FileData* item = Gamelist().GetSelectedGame();
      populateList(*mPopulatedFolder, nullptr);
      Gamelist().SetSelectedGame(item, true);
      break;
    }
    case FileSorts::Sorts::FileNameAscending:
    case FileSorts::Sorts::FileNameDescending:
    case FileSorts::Sorts::SystemAscending:
    case FileSorts::Sorts::SystemDescending:
    case FileSorts::Sorts::RatingAscending:
    case FileSorts::Sorts::RatingDescending:
    case FileSorts::Sorts::PlayersAscending:
    case FileSorts::Sorts::PlayersDescending:
    case FileSorts::Sorts::DeveloperAscending:
    case FileSorts::Sorts::DeveloperDescending:
    case FileSorts::Sorts::PublisherAscending:
    case FileSorts::Sorts::PublisherDescending:
    case FileSorts::Sorts::GenreAscending:
    case FileSorts::Sorts::GenreDescending:
    case FileSorts::Sorts::ReleaseDateAscending:
    case FileSorts::Sorts::ReleaseDateDescending:
    case FileSorts::Sorts::RegionAscending:
    case FileSorts::Sorts::RegionDescending:
    {
      // Force full game refresh
      OnGameSelected(Gamelist(), game, Gamelist().LookupGameIndex(game));
      break;
    }
    default: break;
  }
}

void DetailedGameListView::ConfigurationSystemGamelistLayoutChanged(const SystemData& system, const RecalboxConf::GamelistLayout& layout)
{
  if (&system == &mSystem)
  {
    mLayout = layout;
    SwitchLayout(layout);
  }
}

void DetailedGameListView::ConfigurationGlobalGamelistLayoutChanged(const RecalboxConf::GamelistLayout& layout)
{
  (void)layout;
  // Only systems having no layout of their own follow the global one
  if (mSettings.IsDefinedSystemGamelistLayout(mSystem)) return;
  RecalboxConf::GamelistLayout inherited = mSettings.GetSystemGamelistLayout(mSystem);
  if (inherited == mLayout) return;
  mLayout = inherited;
  SwitchLayout(inherited);
}

void DetailedGameListView::GeneratePreferredOneGameOneRoms(FileData::List& items)
{
  // Free previous records
  Free1G1R();

  // Prepare sorting
  FileSorts& sorts = FileSorts::Instance();
  FileSorts::Sorts sort = FileSorts::Sorts::RegionAscending;

  // Set of uniques aliases
  HashSet<StringHolderTypes::Index32> aliases;
  // List of game per unique alias
  HashMap<StringHolderTypes::Index32, FileData::List> aliasToGames;

  // Record aliases and games per alias
  for (FileData* item: items)
  {
    StringHolderTypes::Index32 alias = item->Metadata().AliasesIndex();
    if (alias == 0) alias = item->Metadata().NameIndex() | (int)0x80000000;
    aliasToGames[alias].Add(item);
    aliases.insert(alias);
  }

  // Get prefered regions
  Regions::GameRegions firstPreferredRegion = Regions::Clamp(RecalboxConf::Instance().GetRomPreferredRegion());
  Regions::GameRegions secondPreferredRegion = Regions::Clamp(RecalboxConf::Instance().GetRomSecondPreferredRegion());
  long long int masterRegions = 0;
  int masterRegionCount = Regions::OrderedMasterRegionsTo8Regions(RecalboxConf::Instance().GetRomPreferredFallback(), masterRegions);
  Regions::GameRegions region1 = (Regions::GameRegions)((masterRegions >>  0) & 0xFF);
  Regions::GameRegions region2 = (Regions::GameRegions)((masterRegions >>  8) & 0xFF);
  Regions::GameRegions region3 = (Regions::GameRegions)((masterRegions >> 16) & 0xFF);
  Regions::GameRegions region4 = (Regions::GameRegions)((masterRegions >> 24) & 0xFF);
  Regions::GameRegions region5 = (Regions::GameRegions)((masterRegions >> 32) & 0xFF);

  // Run through all names/aliases
  for (StringHolderTypes::Index32 alias : aliases)
  {
    FileData::List* games = aliasToGames.try_get(alias);
    if (games == nullptr) { LOG(LogError) << "[DetailledGameList] Abnormal missing alias in 1G1R list generation"; continue; }
    // Ignore unique games
    if (games->Count() == 1) continue;

    int priority = 255;
    FileData* parent = nullptr;
    for (FileData* game : *games)
    {
      const MetadataDescriptor& meta = game->Metadata();
      int regions = meta.Region().Pack;
      if (priority >  0 && Regions::IsIn4Regions(regions, firstPreferredRegion) && (meta.LatestVersion() || meta.IsFirstOrUniqueSupport())   ) { priority =  0; parent = game; continue; }
      if (priority >  1 && Regions::IsIn4Regions(regions, firstPreferredRegion) && meta.LatestVersion()                                      ) { priority =  1; parent = game; continue; }
      if (priority >  2 && Regions::IsIn4Regions(regions, firstPreferredRegion)                                                              ) { priority =  2; parent = game; continue; }
      if (secondPreferredRegion != Regions::GameRegions::Unknown)
      {
        if (priority >  3 && Regions::IsIn4Regions(regions, secondPreferredRegion) && (meta.LatestVersion() || meta.IsFirstOrUniqueSupport())) { priority =  3; parent = game; continue; }
        if (priority >  4 && Regions::IsIn4Regions(regions, secondPreferredRegion) && meta.LatestVersion()                                   ) { priority =  4; parent = game; continue; }
        if (priority >  5 && Regions::IsIn4Regions(regions, secondPreferredRegion)                                                           ) { priority =  5; parent = game; continue; }
      }
      if (priority >  6 && Regions::IsIn4Regions(regions, region1)                                                                           ) { priority =  6; parent = game; continue; }
      if (priority >  7 && Regions::IsIn4Regions(regions, region2)                                                                           ) { priority =  7; parent = game; continue; }
      if (priority >  8 && Regions::IsIn4Regions(regions, region3)                                                                           ) { priority =  8; parent = game; continue; }
      if (priority >  9 && Regions::IsIn4Regions(regions, region4)                                                                           ) { priority =  9; parent = game; continue; }
      if (priority > 10 && masterRegionCount > 4 && Regions::IsIn4Regions(regions, region5)                                                  ) { priority = 10; parent = game; continue; }
    }
    if (parent == nullptr) parent = (*games)[0];

    // Remove parent from child list
    games->Remove(parent);
    // Remove children from the main list
    for(FileData* game : *games) items.Remove(game);
    // Sort remaining children
    FolderData::Sort(*games, sorts.ComparerFromSort(sort, mSystem.IsLastPlayed() || mSystem.IsFavorite()), sorts.IsAscending(sort));
    // Add the new 1G1R in the natural sort order
    m1G1R.insert(parent, new OneGameOneRom(parent, std::move(*games)));
  }
}

void DetailedGameListView::OnFoldGame(IGamelistContainer& caller, FileData* game, int index)
{
  (void)caller;
  (void)index;
  for (auto& it : m1G1R)
    if (it.second->mChildren.Contains(game) || it.second->mParent == game)
    {
      it.second->mFolded = true;
      BuildList(it.second->mParent);
      break;
    }
}

void DetailedGameListView::OnUnfoldGame(IGamelistContainer& caller, FileData* game, int index)
{
  (void)caller;
  (void)index;
  for (auto& it : m1G1R)
    if (it.second->mParent == game)
    {
      it.second->mFolded = false;
      BuildList(it.second->mParent);
      break;
    }
}

void DetailedGameListView::OnFoldAllGame(IGamelistContainer& caller)
{
  (void)caller;
  FileData* game = Gamelist().GetSelectedGame();
  for (auto& it : m1G1R)
  {
    it.second->mFolded = true;
    if (it.second->mChildren.Contains(game))
      game = it.second->mParent;
  }
  BuildList(game);
}

void DetailedGameListView::OnUnfoldAllGame(IGamelistContainer& caller)
{
  (void)caller;
  FileData* game = Gamelist().GetSelectedGame();
  for (auto& it : m1G1R)
    it.second->mFolded = false;
  BuildList(game);
}
