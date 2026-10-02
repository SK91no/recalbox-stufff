//
// Created by bkg2k on 31/12/2019.
//

#include <themes/MenuThemeData.h>
#include <utils/locale/LocaleHelper.h>
#include <components/TextComponent.h>
#include "GuiJukebox.h"

#include "patreon/PatronInfo.h"
#include "rendering/textures/TextureManager.h"


GuiJukebox::GuiJukebox(WindowManager& window, AudioScheduler& audioScheduler)
  : Gui(window)
  , mAudioScheduler(audioScheduler)
  , mTheme(ThemeManager::Instance().Menu())
  , mCache(ThemeManager::Instance().MenuCache())
  , mGrid(window, Vector2i(7,9))
  , mCheckBoxWidth(0)
  , mCheckBoxHeight(0)
  , mBusyAnim(window)
  , mSeekRate(0)
  , mSeekAccumulator(0)
  , mFlashFrames(0)
  , mAutoFollow(true)
  , mDownloading(false)
  , mStartSender(*this)
  , mDownloadSender(*this)
{
  // Add Grid as the main component
  addChild(&mGrid);

  bool isLowRes = Renderer::Instance().Is240p();

  // Bold-condensed font
  mBoldCondensed = isLowRes ? mTheme.SmallText().font :
                   &FontManager::Instance().FromFile(Path("/usr/share/fonts/truetype/ubuntu_condensed.ttf"), (float)mTheme.SmallText().font->Height(), true);
  // Disable font effect in low-res
  mTools.SetEnabled(!isLowRes);

  // 7 x 9
  // +-+-------------------------------------------------+-+---------------------+---------------------+---------------------+-+  0 = 2%
  // +-+-------------------------------------------------+-+---------------------+---------------------+---------------------+-+
  // | |                                                 | |                              TITLE                              | |  1 = Title height
  // +-+                                                 +-+---------------------+---------------------+---------------------+-+
  // | |                                                 | |                     |                     |                     | |
  // +-+                    LIST                         | |                     |                     |                     | |  2 = Auto
  // | |                                                 | |                     |                     |                     | |
  // +-+                                                 +-+---------------------+---------------------+---------------------+-+
  // | |                                                 | |                     |                     |                     | |
  // +-+                                                 | |                     |                     |                     | |  3 = Auto
  // | |                                                 | |                     |                     |                     | |
  // +-+                                                 +-+---------------------+---------------------+---------------------+-+
  // | |                                                 | |                     |                     |                     | |
  // +-+                                                 | |                     |                     |                     | |  4 = Auto
  // | |                                                 | |                     |                     |                     | |
  // +-+                                                 +-+---------------------+---------------------+---------------------+-+
  // | |                                                 | |              Now playing: Song name (mAction)                   | |  5 = font height
  // +-+                                                 +-+-----------------------------------------------------------------+-+
  // | |                                                 | |              mProgress / mPositionSlider                        | |  6 = font height
  // +-+                                                 +-+-----------------------------------------------------------------+-+
  // | |                                                 | |                 Song/Playlist details                           | |  7 = descPercent
  // | |                                                 | |                                                                 | |
  // +-+-------------------------------------------------+-+---------------------+---------------------+---------------------+-+  8 = 2%
  // +-+-------------------------------------------------+-+---------------------+---------------------+---------------------+-+
  //  0                        1                          2           3                     4                     5           6
  //  2%                      40%                         2%         Auto                  Auto                  Auto         2%

  /*
   * Populate the grid
   */

  // Title
  mTitle = std::make_shared<TextComponent>(window, _("JUKEBOX !"), mTheme.Title().font, mTheme.Title().color, ::Alignment::Center);
  mGrid.setEntry(mTitle, Vector2i(3, 1), false, true, Vector2i(3,1) );

  // List
  mList = std::make_shared<TextListComponent<const RemotePlaylist::TrackInfo*>>(window);
  mList->setFont(mTheme.SmallText().font);
  mList->setAlignment(HorizontalAlignment::Right);
  mList->setSelectorColor(mTheme.Text().selectorColor);
  mList->setSelectedColor(mTheme.Text().selectedColor);
  mList->setColorAt(sColorIndexNormal, mTheme.Text().color);
  mList->setColorAt(sColorIndexHalf, (mTheme.Text().color & 0xFFFFFF00) | 0x80);
  mList->setColorAt(sColorIndexRed, 0xFF000022); // Red
  mList->setColorAt(sColorIndexYellow, 0xFFFF0022); // Yellow
  mList->setColorAt(sColorIndexGreen, 0x00FF0022); // Green
  mList->setHorizontalMargin(Renderer::Instance().DisplayWidthAsFloat() * 0.95f * 0.01f);
  mList->setSelectorHeight((float)mTheme.SmallText().font->Height() * 1.5f);
  mList->setUseL1R1(true);
  mList->SetOverlayInterface(this);
  mList->SetCursorChangedCallback(this);
  mGrid.setEntry(mList, Vector2i(1, 1), true, true, Vector2i(1, 7));

  // Now playing action text
  mAction = std::make_shared<TextScrollComponent>(window, "", mTheme.SmallText().font, mTheme.Text().color);
  mGrid.setEntry(mAction, Vector2i(3, 5), false, true, Vector2i(3, 1));

  // Download progress bar
  mProgress = std::make_shared<ProgressBarComponent>(window, 100);
  mGrid.setEntry(mProgress, Vector2i(3, 6), false, true, Vector2i(3, 1));

  // Music position slider (0-100% range)
  mPositionSlider = std::make_shared<SliderComponent>(window, 0, 100, 1, "");
  mGrid.setEntry(mPositionSlider, Vector2i(3, 6), false, true, Vector2i(3, 1));

  // Details
  mMarkdown = std::make_shared<MarkdownText>(window, "", *mTheme.Text().font, mTheme.Text().color, Alignment::TopLeft, true);
  mGrid.setEntry(mMarkdown, Vector2i(3, 7), true, true, Vector2i(3, 1));

  // Album big picture
  int margin = Math::clampi(Math::roundi(mRenderer.DisplayHeightAsFloat() * 0.005f), 2, 8);
  mAlbumPicture = std::make_shared<PictureComponent>(window, ImgProps::KeepRatio, margin, margin);
  mGrid.setEntry(mAlbumPicture, Vector2i(3,2), true, true, Vector2i(3, 3));

  // Album Thumbnails & Mixtape big picture: positioned according to TATE/YOKO mode
  mAlbumThumbnails[0] = std::make_shared<PictureComponent>(window, ImgProps::KeepRatio, margin, margin);
  mAlbumThumbnails[1] = std::make_shared<PictureComponent>(window, ImgProps::KeepRatio, margin, margin);
  mAlbumThumbnails[2] = std::make_shared<PictureComponent>(window, ImgProps::KeepRatio, margin, margin);
  mMixTapePicture = std::make_shared<PictureComponent>(window, ImgProps::KeepRatio, margin, margin);
  if (Renderer::Instance().IsRotatedSide())
  {
    // TATE: Mixtape takes top 6 cells (cols 3-5, rows 2-3), thumbnails on bottom row 4
    mGrid.setEntry(mMixTapePicture, Vector2i(3, 2), true, true, Vector2i(3, 2));
    mGrid.setEntry(mAlbumThumbnails[0], Vector2i(3, 4), true, true, Vector2i(1, 1));
    mGrid.setEntry(mAlbumThumbnails[1], Vector2i(4, 4), true, true, Vector2i(1, 1));
    mGrid.setEntry(mAlbumThumbnails[2], Vector2i(5, 4), true, true, Vector2i(1, 1));
  }
  else
  {
    // YOKO: Mixtape takes left 6 cells (cols 3-4, rows 2-4), thumbnails on right column 5
    mGrid.setEntry(mMixTapePicture, Vector2i(3, 2), true, true, Vector2i(2, 3));
    mGrid.setEntry(mAlbumThumbnails[0], Vector2i(5, 2), true, true, Vector2i(1, 1));
    mGrid.setEntry(mAlbumThumbnails[1], Vector2i(5, 3), true, true, Vector2i(1, 1));
    mGrid.setEntry(mAlbumThumbnails[2], Vector2i(5, 4), true, true, Vector2i(1, 1));
  }

  // Mixtape thumbnail
  mMixTapeThumbnail = std::make_shared<PictureComponent>(window, ImgProps::KeepRatio, margin, margin);
  mGrid.setEntry(mMixTapeThumbnail, Vector2i(3,2), true, true, Vector2i(1, 1));

  // Set Window position/size
  setSize(Renderer::Instance().DisplayWidthAsFloat() * 0.8f, Renderer::Instance().DisplayHeightAsFloat() * 0.8f);
  setPosition((Renderer::Instance().DisplayWidthAsFloat() - mSize.x()) / 2, (Renderer::Instance().DisplayHeightAsFloat() - mSize.y()) / 2);
  mBusyAnim.setSize(mSize);

  // Default state: slider visible, progress hidden (must be after setSize which resets mThemeDisabled)
  mProgress->setThemeDisabled(true);

  TextureManager::Instance().RegisterHttpFilter(this);
  AudioScheduler::Instance().SetPopupAllowed(false);
}

GuiJukebox::~GuiJukebox()
{
  AudioScheduler::Instance().SetPopupAllowed(true);
  RemotePlaylist::Instance().SetDownloadInterface(nullptr);
  TextureManager::Instance().UnregisterHttpFilter(this);

  for (RemotePlaylist::TrackInfo* header : mHeaders)
    delete header;
  mHeaders.clear();
}

void GuiJukebox::Follow(bool flashIfNotFound)
{
  // Find the currently playing music in the list
  const Path& playingPath = mAudioScheduler.PlayingMusicPath();
  bool found = false;
  if (!playingPath.IsEmpty())
  {
    for (int i = mList->size(); --i >= 0; )
    {
      const RemotePlaylist::TrackInfo* ti = mList->getObjectAt(i);
      if (!ti->Name().empty() && ti->LocalPath() == playingPath)
      {
        mList->setCursorIndex(i);
        if (!mAutoFollow) { mAutoFollow = true; mWindow.UpdateHelpSystem(); }
        found = true;
        break;
      }
    }
  }
  // Make flash ?
  if (!found && flashIfNotFound) mFlashFrames = 2;
}

void GuiJukebox::OverlayApply(const Transform4x4f& parentTrans, int leftWidth, int rightWidth, int labelWidth,
                              const Rectangle& area, const RemotePlaylist::TrackInfo* const& data, int index, unsigned int& color)
{
  (void)parentTrans;
  (void)leftWidth;
  (void)rightWidth;
  (void)labelWidth;
  (void)index;
  Texture& texture = data->IsEnabled() ? mSelected : mUnselected;
  Rectangle iconArea(data->Name().empty() ? 0 : (float)mCheckBoxWidth, area.Top(), (float)mCheckBoxWidth, area.Height());
  texture.Render((int)iconArea.Left(), (int)iconArea.Top(), (int)iconArea.Width(), (int)iconArea.Height(), true, false, color);
  if (!data->LocalPath().IsEmpty() && data->LocalPath() == mAudioScheduler.PlayingMusicPath() && mPlayIcon.Valid())
  {
    iconArea.Move(iconArea.Width() + (float)mCheckBoxWidth * .6f, 0).SetWidth((float)mCheckBoxWidth);
    mPlayIcon.Render((int)iconArea.Left(), (int)iconArea.Top(), (int)iconArea.Width(), (int)iconArea.Height(), true, false, color);
  }
}

float GuiJukebox::OverlayGetLeftOffset(const RemotePlaylist::TrackInfo* const& data, int labelWidth)
{
  (void)labelWidth;
  // Init boxes
  if (!mSelected.Valid())
  {
    mCheckBoxHeight = mTheme.SmallText().font->Height();
    mUnselected = mCache.GetElement(MenuThemeData::IconElement::Type::CheckBox, 0, mCheckBoxHeight);
    mSelected = mCache.GetElement(MenuThemeData::IconElement::Type::CheckBoxFilled, 0, mCheckBoxHeight);
    mCheckBoxWidth = (mSelected.Width() * mCheckBoxHeight) / mSelected.Height();
    mPlayIcon = TextureManager::Instance().Create(Path(":/play.svg"), mCheckBoxWidth, 0, TextureHolder::Properties::ImmediateLoad);
  }
  float playIndicator = !data->LocalPath().IsEmpty() && data->LocalPath() == mAudioScheduler.PlayingMusicPath() ? (float)mCheckBoxWidth * 1.6f : 0;
  return ((float)mCheckBoxWidth * 1.5f) + (data->Name().empty() ? 0 : (float)mCheckBoxWidth) + playIndicator;
}

void GuiJukebox::ConfigureHttpClient(const String& url, HttpClient& client)
{
  (void)url;
  client.SetBearer(PatronInfo::Instance().Token());
}

void GuiJukebox::onSizeChanged()
{
  float factor = 1.2f;

  // Main grid
  mGrid.setSize(mSize);
  mGrid.setColWidthPerc(0, .02f, true);
  mGrid.setColWidthPerc(1, .40f, true);
  mGrid.setColWidthPerc(2, 0.02f, true);
  mGrid.setColWidthPerc(3, 0, true);
  mGrid.setColWidthPerc(4, 0, true);
  mGrid.setColWidthPerc(5, 0, true);
  mGrid.setColWidthPerc(6, .02f, true);

  // Set sample text into Markdown to force the component to calculate texte height
  // Do it after column have been set so that Mardown width is already calculated
  String text(mTools.Bold(_("Mixtape:"))); text.Append(String::CRLF);
  text.Append(mTools.Bold(_("Track count:"))); text.Append(String::CRLF);
  text.Append(mTools.Bold(_("Total duration:")));
  mMarkdown->SetText(text);

  float titlePercent = (float)mTitle->getFont()->Height() * factor / mSize.y();
  float fontRowPercent = (float)mTheme.Text().font->Height() * factor / mSize.y();
  float descPercent = (float)mMarkdown->Height() / mSize.y();

  mGrid.setRowHeightPerc(0, .02f, true);
  mGrid.setRowHeightPerc(1, titlePercent, true);
  mGrid.setRowHeightPerc(2, 0, true);
  mGrid.setRowHeightPerc(3, 0, true);
  mGrid.setRowHeightPerc(4, 0, true);
  mGrid.setRowHeightPerc(5, fontRowPercent, true);
  mGrid.setRowHeightPerc(6, fontRowPercent, true);
  mGrid.setRowHeightPerc(7, descPercent, true);
  mGrid.setRowHeightPerc(8, .02f, true);

  mBusyAnim.setSize(mSize);
}

bool GuiJukebox::ProcessInput(const InputCompactEvent& event)
{
  if (event.CancelReleased())
  {
    Close();
    return true;
  }

  if (event.WestReleased())
  {
    const RemotePlaylist::TrackInfo* track = mList->getSelected();
    if (track != nullptr)
    {
      if (!track->Name().empty())
      {
        // Single track toggle
        bool newEnabled = !track->IsEnabled();
        RemotePlaylist::Instance().SetTrackEnabled(track->Md5(), newEnabled);
        const_cast<RemotePlaylist::TrackInfo*>(track)->SetEnabled(newEnabled);
      }
      else
      {
        // Header: toggle all tracks in this mixtape based on header's displayed state
        bool newEnabled = !track->IsEnabled();
        const String& mixTape = track->MixTape();
        for (RemotePlaylist::TrackInfo& ti : mPlayList)
          if (ti.MixTape() == mixTape)
          {
            RemotePlaylist::Instance().SetTrackEnabled(ti.Md5(), newEnabled);
            ti.SetEnabled(newEnabled);
          }
        const_cast<RemotePlaylist::TrackInfo*>(track)->SetEnabled(newEnabled);
      }
    }
    return true;
  }

  if (event.StartReleased())
  {
    Follow(true);
    return true;
  }

  if (event.NorthReleased())
  {
    // Toggle random/linear mode
    bool newMode = !RemotePlaylist::Instance().IsRandomMode();
    RemotePlaylist::Instance().Randomize(newMode);
    // Rebuild the list to reflect the new order
    mList->clear();
    for (auto* h : mHeaders) delete h;
    mHeaders.clear();
    mUniqueMedia.clear();
    FillPlayList();
    // Refresh help bar to show new mode label
    mWindow.UpdateHelpSystem();
    return true;
  }

  // Up/Down exits auto-follow mode
  if (event.AnyUpPressed() || event.AnyDownPressed())
    if (mAutoFollow) { mAutoFollow = false; mWindow.UpdateHelpSystem(); }

  if (event.ValidReleased())
  {
    const RemotePlaylist::TrackInfo* track = mList->getSelected();
    if (track != nullptr && !track->Name().empty())
    {
      mDownloadingMd5 = track->Md5();
      mDownloadingPath = track->LocalPath();
      mDownloadingName = track->Name();
      RemotePlaylist::Instance().SetDownloadInterface(this);
      RemotePlaylist::Instance().RequestSong(track->Md5());
    }
    return true;
  }

  // Left/Right: seek music position (10s on press, 1s/tick when held)
  if (event.AnyLeft())
  {
    if (event.AnyLeftPressed())
    {
      mAudioScheduler.SeekMusic(mAudioScheduler.PlayingMusicPosition() - 10);
      mSeekRate = -1;
      mSeekAccumulator = -500;
    }
    else mSeekRate = 0;
    return true;
  }
  if (event.AnyRight())
  {
    if (event.AnyRightPressed())
    {
      mAudioScheduler.SeekMusic(mAudioScheduler.PlayingMusicPosition() + 10);
      mSeekRate = 1;
      mSeekAccumulator = -500;
    }
    else mSeekRate = 0;
    return true;
  }

  return Gui::ProcessInput(event);
}

void GuiJukebox::Update(int deltaTime)
{
  Component::Update(deltaTime);

  // Seek repeat
  if (mSeekRate != 0)
  {
    mSeekAccumulator += deltaTime;
    while (mSeekAccumulator >= 100)
    {
      mAudioScheduler.SeekMusic(mAudioScheduler.PlayingMusicPosition() + mSeekRate);
      mSeekAccumulator -= 100;
    }
  }

  if (mPlayList.empty() && RemotePlaylist::Instance().HasTracks())
  {
    // Fill playlist
    FillPlayList();
  }


  // Auto-follow: keep cursor on the playing music
  if (mAutoFollow && !mPlayList.empty())
    Follow(false);

  // Update now-playing info (only when not downloading)
  if (!mDownloading)
  {
    const String& musicName = mAudioScheduler.PlayingMusicName();
    if (!musicName.empty())
    {
      // Lookup real track name from playlist (PlayingMusicName returns the filename which is an MD5 for remote tracks)
      const String* displayName = &musicName;
      for (const RemotePlaylist::TrackInfo& ti : mPlayList)
        if (ti.Md5() == musicName) { displayName = &ti.Name(); break; }
      String nowPlaying(_("Now playing"));
      nowPlaying.Append(": ").Append(*displayName);
      if (mAction->getValue() != nowPlaying)
        mAction->setText(nowPlaying);

      float totalTime = mAudioScheduler.PlayingMusicTotalTime();
      if (totalTime > 0)
        mPositionSlider->setSlider(mAudioScheduler.PlayingMusicPosition() * 100.0f / totalTime);
      else
        mPositionSlider->setSlider(0);
    }
    else
    {
      if (!mAction->getValue().empty())
        mAction->setText(String::Empty);
      mPositionSlider->setSlider(0);
    }
  }
}

void GuiJukebox::Render(const Transform4x4f& parentTrans)
{
  Gui::Render(parentTrans);

  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);
  unsigned int bgColor = (mFlashFrames > 0) ? 0xFFFFFF44 : 0x00000011;
  if (mFlashFrames > 0) --mFlashFrames;
  Renderer::DrawRectangle(mGrid.getPosition().x() + mGrid.getColWidth(0), mGrid.getPosition().y() + mGrid.getRowHeight(0),
                          mGrid.getColWidth(1), mGrid.getRowHeight(1, 7), bgColor);
  if (mPlayList.empty())
    mBusyAnim.Render(trans);
}

bool GuiJukebox::CollectHelpItems(Help& help)
{
  //mGrid.CollectHelpItems(help);
  help.Set(HelpType::UpDown, _("SELECT"))
      .Set(HelpType::LeftRight, _("SEEK"))
      .Set(HelpType::X, RemotePlaylist::Instance().IsRandomMode() ? _("SWITCH TO LINEAR PLAY") : _("SWITCH TO RANDOM PLAY"));
  if (!mAutoFollow)
    help.Set(HelpType::Start, _("FOLLOW"));
  help.Set(HelpType::Y, _("ENABLE/DISABLE"))
      .Set(Help::Valid(), _("PLAY"))
      .Set(Help::Cancel(), _("CLOSE"));
  return true;
}

void GuiJukebox::FillPlayList()
{
  // Get all available tracks
  mPlayList = RemotePlaylist::Instance().Playlist();

  // Linear mode: sorted by mixtape/name with headers
  std::ranges::sort(mPlayList, [](const RemotePlaylist::TrackInfo& a, const RemotePlaylist::TrackInfo& b) { return a.Compare(b); });

  String lastMixTape;
  int index = 0;
  for (const RemotePlaylist::TrackInfo& ti : mPlayList)
  {
    if (ti.MixTape() != lastMixTape)
    {
      // Calculate total duration, song count and unique media list
      int duration = 0;
      int count = 0;
      bool allEnabled = ti.IsEnabled();
      HashSet<String> media;
      for (int i = index + 1; i < (int)mPlayList.size() && mPlayList[i].MixTape() == ti.MixTape(); ++i, ++count) { duration += mPlayList[i].Duration(); media.insert(mPlayList[i].AlbumPictureUrlOrNoImage()); mUniqueMedia.insert(ti.AlbumPictureUrlOrNoImage()); if (!mPlayList[i].IsEnabled()) allEnabled = false; }
      mUniqueMedia.insert(ti.MixtapePictureUrlOrNoImage());
      String mediaList;
      for (const String& m : media) { if (!mediaList.empty()) mediaList.Append('|'); mediaList.Append(m); }
      // Insert header
      mHeaders.push_back(new RemotePlaylist::TrackInfo(String::Empty, duration, count, String::Empty, mediaList, ti.MixtapePictureUrl(), String::Empty, String::Empty, ti.MixTape(), String::Empty));
      mHeaders.back()->SetEnabled(allEnabled);
      mList->add((_F("{0} ({1})") / ti.MixTape() / FormatDuration(duration)).ToString(), mHeaders[mHeaders.size() - 1], sColorIndexNormal, sColorIndexRed, HorizontalAlignment::Left);
      lastMixTape = ti.MixTape();
    }
    mList->add((_F("{0} ({1})") / ti.Name() / FormatDuration(ti.Duration())).ToString(), &ti, sColorIndexHalf, -1, HorizontalAlignment::Left);
    ++index;
  }

  if (RemotePlaylist::Instance().HasTracks())
    FillSongDetails(*mList->getObjectAt(0));
}

void GuiJukebox::FillSongDetails(const RemotePlaylist::TrackInfo& track)
{
  mAlbumPicture->setThemeDisabled(true);
  for (int i = sPictureGridCount; --i >= 0; ) mAlbumThumbnails[i]->setThemeDisabled(true);
  mMixTapePicture->setThemeDisabled(true);
  mMixTapeThumbnail->setThemeDisabled(true);

  if (track.Name().empty())
  {
    // Header
    String text(mTools.Bold(_("Mixtape:"))); text.Append(' ').Append(track.MixTape()).Append(String::CRLF);
    text.Append(mTools.Bold(_("Track count:"))); text.Append(' ').Append(track.Size()).Append(String::CRLF);
    text.Append(mTools.Bold(_("Total duration:"))); text.Append(' ').Append(FormatDuration(track.Duration()));
    mMarkdown->SetText(text);

    int position = 0;
    for (const String& path : track.AlbumPictureUrlOrNoImage().Split('|'))
    {
      mAlbumThumbnails[position]->setImage(Path(path), false);
      mAlbumThumbnails[position]->setColor(path.StartsWith(':') ? mTheme.Text().color : 0xFFFFFFFF);
      mAlbumThumbnails[position]->setThemeDisabled(false);
      if (++position >= sPictureGridCount) break;
    }
    mMixTapePicture->setImage(Path(track.MixtapePictureUrlOrNoImage()), false);
    mMixTapePicture->setColor(track.MixtapePictureUrlOrNoImage().StartsWith(':') ? mTheme.Text().color : 0xFFFFFFFF);
    mMixTapePicture->setThemeDisabled(false);
    return;
  }
  // Header
  String text(mTools.Bold(_("Author:"))); text.Append(' ').Append(track.Artist()).Append(String::CRLF);
  text.Append(mTools.Bold(_("Album:"))); text.Append(' ').Append(track.Album()).Append(String::CRLF);
  text.Append(mTools.Bold(_("Duration:"))); text.Append(' ').Append(FormatDuration(track.Duration()));
  mMarkdown->SetText(text);
  mAlbumPicture->setImage(Path(track.AlbumPictureUrlOrNoImage()), false);
  mAlbumPicture->setColor(track.AlbumPictureUrlOrNoImage().StartsWith(':') ? mTheme.Text().color : 0xFFFFFFFF);
  mAlbumPicture->setThemeDisabled(false);
  mMixTapeThumbnail->setImage(Path(track.MixtapePictureUrlOrNoImage()), false);
  mMixTapeThumbnail->setColor(track.MixtapePictureUrlOrNoImage().StartsWith(':') ? mTheme.Text().color : 0xFFFFFFFF);
  mMixTapeThumbnail->setThemeDisabled(false);
}

String GuiJukebox::FormatDuration(int duration)
{
  String timing(duration / 60);
  timing.Append(':');
  int seconds = duration % 60;
  if (seconds <= 9) timing.Append('0');
  timing.Append(seconds);
  return timing;
}

void GuiJukebox::SwitchToDownloadMode(const String& songName)
{
  mDownloading = true;
  mPositionSlider->setThemeDisabled(true);
  mProgress->setThemeDisabled(false);
  mProgress->setCurrentValue(0);
  String text(_("Downloading"));
  text.Append(": ").Append(songName);
  mAction->setText(text);
}

void GuiJukebox::SwitchToPlayingMode()
{
  mDownloading = false;
  mProgress->setThemeDisabled(true);
  mPositionSlider->setThemeDisabled(false);
}

void GuiJukebox::StartDownloadingSong(const String& songMD5)
{
  (void)songMD5;
  // Called from background thread: marshal to main thread via SyncMessageSender
  mStartSender.Send();
}

void GuiJukebox::DownloadingSong(const String& songMD5, long long int currentSize, long long int totalSize)
{
  (void)songMD5;
  // Called on main thread via RemotePlaylist::mProgressSender
  mProgress->setMaxValue(totalSize);
  mProgress->setCurrentValue(currentSize);
}

void GuiJukebox::EndDownloadingSong(const String& songMD5, bool status)
{
  (void)songMD5;
  // Called from background thread: marshal result to main thread via SyncMessageSender
  mDownloadSender.Send(status);
}

void GuiJukebox::ReceiveSyncMessage()
{
  // Main thread: download has started
  SwitchToDownloadMode(mDownloadingName);
}

void GuiJukebox::ReceiveSyncMessage(bool success)
{
  // Main thread: download complete
  if (success)
    mAudioScheduler.PlayRemoteTrack(mDownloadingPath);
  else
    mWindow.InfoPopupAddRegular((_F(_("Error downloading {0}. Retry in a few while !")) / mDownloadingName).ToString(), 10, PopupType::Music, false);
  SwitchToPlayingMode();
}
