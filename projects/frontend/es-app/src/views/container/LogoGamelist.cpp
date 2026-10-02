//
// Created by bkg2k on 23/12/25.
//

#include "LogoGamelist.h"

#include "MetadataIcons.h"
#include "rendering/fonts/FontManager.h"
#include "systems/SystemManager.h"
#include <rendering/textures/TextureManager.h>

LogoGamelist::LogoGamelist(WindowManager& window, SystemData& system, IGamelistEvents& eventReceiver,
                           PictogramCaches& pictogramCaches)
  : AbstractGamelist(window, system, this, { { .mMoveDelay=500, .mNext=5000}, { .mMoveDelay=100, .mNext=INT32_MAX} }, true)
  , Themable(this)
  , mEvents(eventReceiver)
  , mCaches(pictogramCaches)
  , mLastHeaderIndex(-1)
  , mColors { 0 }
  , mFont(nullptr)
  , mFolderFont(nullptr)
  , mShowScrollbar(false)
  , mScrollbarTimer(0)
  , mScrollbarWidth(Renderer::Instance().DisplayWidthAsInt() > 1024 ? 6 : (Renderer::Instance().DisplayWidthAsInt() > 480 ? 4 : 2))
  , mFolderNameMaxWidth(0)
  , mGameNameMaxWidth(0)
  , mChildNameMaxWidth(0)
  , mUppercase(false)
  , mMarqueeOffset(0)
  , mMarqueeTime(0)
  , mMarqueeStep(MarqueeSteps::LeftPause)
  , mAlignment(HorizontalAlignment::Center)
  , mHorizontalMargin(0)
  , mVerticalMargin(0)
  , mSelectorHeight(20)
  , mSelectorColor(0xFF0000FF)
  , mSelectedColor(0xFFFFFFFF)
  , mBarColor(0x0000FFFF)
  , mDecorationColorTopLeft(0xFFFFFFFF)
  , mDecorationColorTopRight(0xFFFFFFFF)
  , mDecorationColorBottomLeft(0xFFFFFFFF)
  , mDecorationColorBottomRight(0xFFFFFFFF)
  , mPictoWidth(0)
  , mPictoHeight(0)
  , mPictoHMargin(Renderer::Instance().Is480pOrLower() ? 1 : 2)
  , mPictoVMargin(0)
  , mTextOffset(0)
  , mTextMaxWidth(0)
  , mTimeCounter(0)
  , mExtensionDown(0)
  , mExtendDown(false)
, mDecorations(RecalboxConf::Instance().GetSystemGamelistDecoration(system))
{
  // Default "theming"
  const MenuThemeData& menu = ThemeManager::Instance().Menu();
  mFont = menu.Text().font;
  memset(mColors, 0, sizeof(mColors));
  SetColor(0, menu.Text().color);
  SetColor(1, menu.Text().color);
  GeneratePictoSize();
  //! Texts
  mCloneTemplate = _("Clone {0}/{1}");
  mRelatedTemplate =_("Related {0}/{1}");
  mParentTemplate = _("Parent");
}

FileData::List LogoGamelist::GetHeaderGames(HeaderData* header)
{
  FileData::List result;
  bool found = false;
  for(const auto& entry : AbstractGamelist::GetRawEntries())
  {
    if (found && entry.mGame->IsGame())
      result.Add(entry.mGame);
    if (entry.mGame->IsHeader())
    {
      if (found) break; // found another header, just break;
      if (entry.mGame == header) found = true; // found the requested header
    }
  }
  return result;
}

IGamelistContainer& LogoGamelist::AddGames(FileData* game, const String& displayName, GameType type, int colorIndex)
{
  // Rerouting
  if (game->IsFolder()) return AddFolder((FolderData*)game, colorIndex);

  // Extract textures
  Path frontPath(GetPrimaryMediaPath(game));
  Path backPath(GetSecondaryMediaPath(game));
  Texture front; if (!frontPath.IsEmpty()) front = TextureManager::Instance().Create(frontPath,  mSelectorHeight, (int)mSize.y(), TextureHolder::Properties::None);
  Texture back;  if (!backPath.IsEmpty())  back  = TextureManager::Instance().Create(backPath ,  mSelectorHeight, (int)mSize.y(), TextureHolder::Properties::None);

  // Parent/Child creation
  if ((type == GameType::ChildGame || type == GameType::ArcadeClone) && Count() != 0)
  {
    // First child ? Create the placeholder (reuse the parent's already-interned name index)
    if (AbstractGamelist::Entry& entry = AbstractGamelist::GetRawEntryAt(AbstractGamelist::Count() - 1); !entry.IsChild())
      AbstractGamelist::AddIndexed(entry.mNameIndex, { entry.mData, (int)mChildren.size() - 1 }, entry.mGame, type);
    else // Else just increment child count
      entry.mData.mChildLength++;
    // Store children (raw name, deduplicated in the arena; uppercasing happens at render)
    mChildren.push_back({ .mNameIndex=InternName(displayName), .mGame=game });
  }
  else
  {
    AbstractGamelist::Add(displayName, { front, back, mLastHeaderIndex, colorIndex, -1 }, game, type);
    // Store children (raw name, deduplicated in the arena; uppercasing happens at render)
    if (type == GameType::ArcadeParentUnfolded || type == GameType::ParentGameUnfolded)
      mChildren.push_back({ .mNameIndex=InternName(displayName), .mGame=game });
  }
  return *this;
}

void LogoGamelist::GeneratePictoSize()
{
  // Pictograms
  mPictoHeight = (mSelectorHeight - (sVerticalDivider + 1) * 2) / sVerticalDivider;
  mPictoVMargin = (mSelectorHeight - mPictoHeight * sVerticalDivider) / (sVerticalDivider + 1);
  mPictoWidth = Math::roundi((float)mPictoHeight * (71.f / 48.f));

  // Text
  mTextOffset = mSelectorHeight / 2;
  mTextMaxWidth = Math::roundi(mSize.x()) - (mHorizontalMargin * 2) - mTextOffset - (2 * (mPictoHMargin + mPictoWidth));

  // Folder texture
  mFolder = TextureManager::Instance().Create(Path(":/folder.svg"), mSelectorHeight, (mSelectorHeight * 4) / 3);
}

void LogoGamelist::ResetMarquee()
{
  mMarqueeOffset = 0;
  mMarqueeTime = 0;
  mMarqueeStep = MarqueeSteps::LeftPause;
}

void LogoGamelist::ProcessMarquee(int elapsed)
{
  if (!IsReallyScrolling() && Count() > 0)
  {
    // Get text width
    AbstractGamelist::Entry& entry = AbstractGamelist::GetRawEntry();
    int textWidth = entry.mData.mTextWidth;
    if (textWidth < 0)
    {
      String name = CasedName(entry);
      textWidth = entry.mData.mTextWidth = (entry.mGame->IsFolder() ? mFolderFont : mFont)->TextWidth(name);
    }

    int width = entry.mGame->IsFolder() ? mFolderNameMaxWidth : (entry.IsChild() ? mChildNameMaxWidth : mGameNameMaxWidth);
    if (textWidth > width)
    {
      switch (mMarqueeStep)
      {
        case MarqueeSteps::LeftPause:
        {
          mMarqueeOffset = 0;
          if (mMarqueeTime > sMarqueePause) { mMarqueeTime = 0; mMarqueeStep = MarqueeSteps::ScrollToRight; }
          break;
        }
        case MarqueeSteps::ScrollToRight:
        {
          mMarqueeOffset = (mMarqueeTime * sMarqueeSpeed1) / 1000;
          if (mMarqueeOffset >= textWidth - width) { mMarqueeTime = 0; mMarqueeOffset = textWidth - width; mMarqueeStep = MarqueeSteps::RightPause;}
          break;
        }
        case MarqueeSteps::RightPause:
        {
          mMarqueeOffset = textWidth - width;
          if (mMarqueeTime > sMarqueePause) { mMarqueeTime = 0; mMarqueeStep = MarqueeSteps::RollOver; }
          break;
        }
        case MarqueeSteps::RollOver:
        {
          mMarqueeOffset = textWidth - width + ((mMarqueeTime * sMarqueeSpeed2) / 1000);
          if (mMarqueeOffset >= textWidth + ((int)mSize.x() / 4)) { mMarqueeTime = 0; mMarqueeOffset = 0; mMarqueeStep = MarqueeSteps::LeftPause;}
          break;
        }
        default: break;
      }
      mMarqueeTime += elapsed;
    }
  }
}

void LogoGamelist::MoveChild(AbstractGamelist::Entry& entry, bool forward, int index)
{
  if (!entry.IsChild()) return;

  // Move
  LogoEntry& logo = entry.mData;
  if (logo.mChildLength < 2) return;
  if (forward) { if (++logo.mChildPos >= logo.mChildLength) logo.mChildPos = 0; }
  else         { if (--logo.mChildPos < 0 ) logo.mChildPos = (signed char)(logo.mChildLength - 1); }

  // Reset data with child data
  logo.mTextWidth = -1;
  LogoChild& child = mChildren[logo.mChildIndex + logo.mChildPos];
  entry.mGame = child.mGame;
  entry.mNameIndex = child.mNameIndex;

  mEvents.OnGameSelected(*this, entry.mGame, index);
}

void LogoGamelist::MoveChild(AbstractGamelist::Entry& entry, const FileData* child)
{
  if (!entry.IsChild()) return;

  // Move
  LogoEntry& logo = entry.mData;
  for (signed char i = logo.mChildLength, p = logo.mChildPos; --i >= 0;)
    if (LogoChild& c = mChildren[p + i]; c.mGame == child)
    {
      // Reset data with child data
      logo.mTextWidth = -1;
      entry.mGame = c.mGame;
      entry.mNameIndex = c.mNameIndex;
      entry.mData.mChildPos =  i;
      break;
    }
}

void LogoGamelist::RenderItemBackground(const Rectangle& globalArea, int from, int to, int yStart, int mainIndex)
{
  //! We're descending from the upper relative index down to the lower relative index
  Rectangle itemArea(globalArea.Left(), (float)yStart, globalArea.Width(), (float)mSelectorHeight);
  itemArea.Contract((float)mHorizontalMargin, (float)mVerticalMargin);
  for (int i = to; --i >= from; yStart -= mSelectorHeight)
  {
    LogoEntry& entry = GetEntryDataAt(mainIndex + i);
    // Draw background
    if ((unsigned int)entry.mBackgroundColorIndex < sColorCount)
      Renderer::DrawRectangle(itemArea, mColors[entry.mBackgroundColorIndex]);
    itemArea.Move(0, (float)-mSelectorHeight);
  }
}

void LogoGamelist::RenderFolder(const Rectangle& area, const String& name, const LogoEntry& entry, int colorShift)
{
  // Render folder icon & name
  Rectangle folder(area);
  float itemIconWidth = area.Height() * 4.f / 3.f;
  mFolder.Render((int)folder.Left(), (int)folder.Top(), (int)itemIconWidth, (int)area.Height(), true, false, (mColors[entry.mColorIndex] & 0xFFFFFF00) | (entry.mFrontOpacity >> colorShift));
  folder.MoveLeft(itemIconWidth + (float)mHorizontalMargin);
  mFolderFont->DrawText(name, folder, 0, 0, ShiftAlpha(mColors[entry.mColorIndex], entry.mFrontOpacity, colorShift), Alignment::CenterLeft, mDefaultFontEffect);
  mFolderNameMaxWidth = Math::roundi(folder.Width());
}

void LogoGamelist::RenderHeader(const Rectangle& area, HeaderData* header, const LogoEntry& entry, int colorShift)
{
  Rectangle inner(area);
  switch(header->Sort())
  {
    case FileSorts::Sorts::RatingAscending:
    case FileSorts::Sorts::RatingDescending:
    {

      break;
    }
    case FileSorts::Sorts::PlayersAscending:
    case FileSorts::Sorts::PlayersDescending:
    {
      inner.MoveRight((float)-(mSelectorHeight + mHorizontalMargin));
      int playerMinMax = header->Int();
      int playerMax = playerMinMax >> 16;
      int playerMin = playerMinMax & 0xFFFF;
      int h = Math::roundi((float)mSelectorHeight * (48.f / 71.f));
      Texture playersTexture = mCaches.GetPlayers(playerMin, playerMax, h);
      int y = (int)(area.Top() + ((area.Height() - (float) h) / 2.f));
      // Draw
      playersTexture.Render((int)inner.Right(), y, mSelectorHeight, h, true, 0xFFFFFF00 | (0xFF >> colorShift));
      break;
    }
    case FileSorts::Sorts::GenreAscending:
    case FileSorts::Sorts::GenreDescending:
    {
      if (GameGenres genre = (GameGenres)(header->Int()); genre != GameGenres::None)
      {
        inner.MoveRight((float)-(mSelectorHeight + mHorizontalMargin));
        int h = Math::roundi((float)mSelectorHeight * (48.f / 71.f));
        int y = (int)(area.Top() + ((area.Height() - (float) h) / 2.f));
        Texture genreTexture = mCaches.GetGenre(genre, h);
        // Draw
        genreTexture.Render((int)inner.Right(), y, mSelectorHeight, h, true, 0xFFFFFF00 | (0xFF >> colorShift));
      }
      break;
    }
    case FileSorts::Sorts::RegionAscending:
    case FileSorts::Sorts::RegionDescending:
    {
      break;
    }
    case FileSorts::Sorts::FileNameAscending:
    case FileSorts::Sorts::FileNameDescending:
    case FileSorts::Sorts::SystemAscending:
    case FileSorts::Sorts::SystemDescending:
    case FileSorts::Sorts::PlayCountAscending:
    case FileSorts::Sorts::PlayCountDescending:
    case FileSorts::Sorts::TotalPlayTimeAscending:
    case FileSorts::Sorts::TotalPlayTimeDescending:
    case FileSorts::Sorts::LastPlayedAscending:
    case FileSorts::Sorts::LastPlayedDescending:
    case FileSorts::Sorts::DeveloperAscending:
    case FileSorts::Sorts::DeveloperDescending:
    case FileSorts::Sorts::PublisherAscending:
    case FileSorts::Sorts::PublisherDescending:
    case FileSorts::Sorts::ReleaseDateAscending:
    case FileSorts::Sorts::ReleaseDateDescending:
    default: break;
  }
  inner.Contract(mHorizontalMargin, 0);
  mFolderFont->DrawText(header->Name(), inner, 0, 0, (mColors[entry.mColorIndex] & 0xFFFFFF00) | (entry.mFrontOpacity >> colorShift), Alignment::CenterLeft, mDefaultFontEffect);
}

void LogoGamelist::RenderSystemLogo(Rectangle& area, const FileData& game, int fading, const Transform4x4f& trans, bool extend, bool drawActually)
{
  if (drawActually)
  {
    Rectangle systemLogo(area.Left(), area.Top(), area.Height(), area.Height() / 2.f);
    if (extend) systemLogo.MoveRight((float)mSelectorHeight);
    systemLogo.Round();
    Transform4x4f t(Transform4x4f::Identity());
    t.translate(systemLogo.Left(), systemLogo.Top()).rotateZ(-Math::PiF / 2);
    Renderer::SetMatrix(trans * t);
    SystemData& system = game.System().Descriptor().IsPort() ? System().Manager().PortsSystem() : game.System();
    mCaches.GetSystemLogo(system, (int)systemLogo.Height()).Render((int)-systemLogo.Width() / (extend ? 2 : 1), 0, (int)systemLogo.Width(), (int)systemLogo.Height(), true, false, 0xFFFFFF00 | fading);
    Renderer::SetMatrix(trans);
  }
  area.MoveLeft((float)(mSelectorHeight >> 1));
}

void LogoGamelist::RenderGameBackground(const Rectangle& area, AbstractGamelist::Entry& entry, int colorShift) const
{
  LogoEntry& logoEntry = entry.mData;
  if (logoEntry.mBackOpacity < 255) logoEntry.mBackOpacity = Math::clampi(logoEntry.mBackOpacity + 20, 0, 255);
  int backOpacity = logoEntry.mBackOpacity;
  Rectangle image(area);
  if (entry.IsChild())
  {
    image.MoveTop(-area.Width()).MoveBottom(area.Width()).Move(0, -area.Height() / 2.f);//.Move(0, -itemArea.Height());
    Renderer::Instance().Clip(Rectangle(area).MoveTop((float)(-2 * mVerticalMargin)));
  }
  else
  {
    image.MoveTop(-area.Width()).MoveBottom(area.Width()).Move(0, area.Height() / 2.f);
    Renderer::Instance().Clip(area);
  }
  logoEntry.mBack.Render((int)image.Left(), (int)image.Top(), (int)image.Width(), (int)image.Height(), true, false, 0xFFFFFF00 | (backOpacity >> (2 + colorShift)));
  Renderer::Instance().Unclip();
}

void LogoGamelist::RenderGameForeground(const Rectangle& itemArea, AbstractGamelist::Entry& entry, int shiftColor, int index)
{
  LogoEntry& logoEntry = entry.mData;
  Rectangle inner(itemArea);
  if (entry.IsChild())
  {
    bool selected = index == 1;
    String name = CasedName(entry);
    Colors::ColorRGBA color = ShiftAlpha(mColors[logoEntry.mColorIndex], logoEntry.mFrontOpacity, shiftColor + (selected ? 0 : 1));
    // Display position
    String position(logoEntry.mChildPos == 0 ? mParentTemplate : (_F(entry.mGame->System().IsArcade() ? mCloneTemplate : mRelatedTemplate) / logoEntry.mChildPos / (logoEntry.mChildLength - 1)).ToString());
    mFont->DrawText(position, (int)inner.Left() + mHorizontalMargin, (int)inner.Top() + mPictoVMargin, color, Alignment::CenterLeft, mDefaultFontEffect);

    // Draw arrows
    inner.Contract((float)mHorizontalMargin, 0);
    Texture leftArrow = mCaches.GetArrow(PictogramCaches::Arrow::Left, mPictoHeight);
    Texture rightArrow = mCaches.GetArrow(PictogramCaches::Arrow::Right, mPictoHeight);
    int w = leftArrow.Width();
    int h = leftArrow.Height();
    int hOffset = ((int)inner.Height() - h) / 2;
    int animate = selected ? (mTimeCounter >> 8) & 0x3 : 0;
    leftArrow.Render((int)inner.Left() + 3 -animate, (int)inner.Top() + hOffset, w, h, true, false, color);
    rightArrow.Render((int)inner.Right() - w - 3 + animate, (int)inner.Top() + hOffset, w, h, true, false, color);

    // Draw title
    inner.Contract((float)leftArrow.Size().x() + (float)mHorizontalMargin + 4, 0);
    int offset = selected ? mMarqueeOffset : 0;
    mFont->DrawText(name, inner, (float)-offset, 0, color, Alignment::CenterLeft, mDefaultFontEffect);
    if (offset != 0)
      mFont->DrawText(name, inner, ((float)entry.mData.mTextWidth + mSize.x() / 4) - (float)offset, 0, color, Alignment::CenterLeft, mDefaultFontEffect);
    mChildNameMaxWidth = Math::roundi(inner.Width());

    // Render overlay items
    RenderLinearDecorations(itemArea, *entry.mGame, logoEntry.mFrontOpacity >> shiftColor, index == 1);
  }
  else
  {
    // Render logo
    if (logoEntry.mFront.Valid()) logoEntry.mFront.Render((int)inner.Left() + mTextOffset, (int)inner.Top(), (int)inner.Width() - (mTextOffset * 2), (int)inner.Height(), true, false, 0xFFFFFF00 | (logoEntry.mFrontOpacity >> shiftColor));
    else
    {
      // Render scrollable text
      String name = CasedName(entry);
      inner.Contract((float)mHorizontalMargin, 0);
      int offset = index == 0 ? mMarqueeOffset : 0;
      Colors::ColorRGBA color = ShiftAlpha(mColors[logoEntry.mColorIndex], logoEntry.mFrontOpacity, shiftColor);
      mFont->DrawText(name, inner, (float)-offset, 0, color, Alignment::CenterLeft, mDefaultFontEffect);
      if (offset != 0)
        mFont->DrawText(name, inner, ((float)entry.mData.mTextWidth + mSize.x() / 4) - (float)offset, 0, color, Alignment::CenterLeft, mDefaultFontEffect);
      mGameNameMaxWidth = Math::roundi(inner.Width());
    }
    // Render overlay items
    RenderDecorations(itemArea, *entry.mGame, logoEntry.mFrontOpacity >> shiftColor, index == 0);
  }
}

void LogoGamelist::RenderScrollbar(const Rectangle& area, int index)
{
  // Draw bar
  if (HasGame() && (mShowScrollbar || mScrollbarTimer > 0))
  {
    int verticalSize = (int)area.Height();
    int screenCount = (verticalSize + mSelectorHeight - 1) / mSelectorHeight;
    if (Count() > screenCount)
    {
      int barHeight = (int) ((area.Height() * (float)screenCount) / (float)Count());

      Renderer& renderer = Renderer::Instance();
      if ((barHeight < renderer.DisplayHeightAsInt() / 40) && (verticalSize >= renderer.DisplayHeightAsInt() / 20))
        barHeight = renderer.DisplayHeightAsInt() / 40;
      int pixelTotal = Count() * mSelectorHeight;
      int pixelPosition = (int)(((float)index + ((float)Offset() / 65535.f)) * (float)mSelectorHeight);
      int yOffset = ((pixelPosition * (verticalSize - barHeight)) / pixelTotal);

      int alpha = mShowScrollbar || mScrollbarTimer > sBarFadeTime ? 0xFF : (255 * mScrollbarTimer) / sBarFadeTime;
      Renderer::DrawRectangle((int) mSize.x() - 2 - mScrollbarWidth, 2 + yOffset, mScrollbarWidth, barHeight, (mBarColor & 0xFFFFFF00) | alpha);
    }
  }
}

void LogoGamelist::RenderLinearDecorations(const Rectangle& area, const FileData& game, int fading, bool selected) const
{
  if (mDecorations == 0) return;
  // Regions
  int x = (int)area.Right() + mPictoHMargin;
  int y = (int)area.Bottom() - mPictoVMargin - mPictoHeight;
  Colors::ColorRGBA tl = HalfColor(mDecorationColorTopLeft, fading, !selected);
  Colors::ColorRGBA tr = HalfColor(mDecorationColorTopRight, fading, !selected);
  Colors::ColorRGBA br = HalfColor(mDecorationColorBottomRight, fading, !selected);
  Colors::ColorRGBA bl = HalfColor(mDecorationColorBottomLeft, fading, !selected);
  if (hasFlag(mDecorations, RecalboxConf::GamelistDecoration::Regions))
  {
    for (int r = Regions::RegionPack::sMaxRegions; --r >= 0;)
      if (Regions::GameRegions region = game.Metadata().Region().Regions[r]; region != Regions::GameRegions::Unknown)
      {
        // Draw
        x -= mPictoWidth + mPictoHMargin;
        mCaches.GetFlag(region, mPictoHeight).Render(x, y, mPictoWidth, mPictoHeight, false,tl, tr, br, bl);
      }
  }

  // Genre
  if (hasFlag(mDecorations, RecalboxConf::GamelistDecoration::Genre))
    if (game.Metadata().GenreId() != GameGenres::None)
    {
      x -= mPictoWidth + mPictoHMargin;
      mCaches.GetGenre(game.Metadata().GenreId(), mPictoHeight).Render(x, y, mPictoWidth, mPictoHeight, false, tl, tr, br, bl);
    }

  // Players
  if (hasFlag(mDecorations, RecalboxConf::GamelistDecoration::Players))
  {
    x -= mPictoWidth + mPictoHMargin;
    mCaches.GetPlayers(game.Metadata().PlayerMin(), game.Metadata().PlayerMax(), mPictoHeight).Render(x, y, mPictoWidth, mPictoHeight, false, tl, tr, br, bl);
  }

  // Support type
  if (hasFlag(mDecorations, RecalboxConf::GamelistDecoration::SupportType))
    if (game.Metadata().SupportType() != SupportTypes::Unknown)
    {
      Texture typeTexture = mCaches.GetSupportType(game.Metadata().SupportType(), mPictoHeight);
      int w = typeTexture.Size().x();
      x -= w + mPictoHMargin;
      typeTexture.Render(x, y, w, mPictoHeight, false, tl, tr, br, bl);
    }

  // Metadata
  if (System().IsVirtual() && (game.Metadata().Favorite() || game.Metadata().Hidden() || game.Metadata().Adult()))
  {
    if (game.Metadata().Favorite())
      if (Texture t = mCaches.GetMetadataIconType(MetadataIcons::Favorite, mPictoHeight); t.MakeReady())
      {
        x -= t.Size().x() + mPictoHMargin;
        t.Render(x, y, mPictoHeight, mPictoHeight, true, false, 0xFFC00000 | fading);
      }
    if (game.Metadata().Hidden())
      if (Texture t = mCaches.GetMetadataIconType(MetadataIcons::Hidden, mPictoHeight); t.MakeReady())
      {
        x -= t.Size().x() + mPictoHMargin;
        t.Render(x, y, mPictoHeight, mPictoHeight, true, false, 0x80808000 | fading);
      }
    if (game.Metadata().Adult())
      if (Texture t = mCaches.GetMetadataIconType(MetadataIcons::Adult, mPictoHeight); t.MakeReady())
      {
        x -= t.Size().x() + mPictoHMargin;
        t.Render(x, y, mPictoHeight, mPictoHeight, true, false, 0xFF80FF00 | fading);
      }
  }

  // Supports
  if (hasFlag(mDecorations, RecalboxConf::GamelistDecoration::SupportNumber))
  {
    if (game.Metadata().HasSupportInformation())
    {
      x -= mPictoHMargin;
      // Right part
      Texture rightTexture = mCaches.GetSupportRightPart(mPictoHeight);
      int w = rightTexture.Size().x();
      x -= w ;
      rightTexture.Render(x, y, w, mPictoHeight, false, tl, tr, br, bl);
      // Total?
      if (game.Metadata().SupportTotal() != 0)
      {
        // Draw total
        Texture totalTexture = mCaches.GetSupportNumber(game.Metadata().SupportTotal(), mPictoHeight);
        w = totalTexture.Size().x();
        x -= w;
        totalTexture.Render(x, y, w, mPictoHeight, false, tl, tr, br, bl);
        // Need separator ?
        if (game.Metadata().SupportIndex() != 0 || game.Metadata().SupportSide() != SupportSides::None)
        {
          // Draw separator
          Texture separatorTexture = mCaches.GetSupportSeparator(mPictoHeight);
          w = separatorTexture.Size().x();
          x -= w;
          separatorTexture.Render(x, y, w, mPictoHeight, false, tl, tr, br, bl);
        }
      }
      // Side ?
      if (game.Metadata().SupportSide() != SupportSides::None)
      {
        // Draw side
        Texture sideTexture = mCaches.GetSupportSide(game.Metadata().SupportSide(), mPictoHeight);
        w = sideTexture.Size().x();
        x -= w;
        sideTexture.Render(x, y, w, mPictoHeight, false, tl, tr, br, bl);
      }
      // Number ?
      if (game.Metadata().SupportIndex() != 0)
      {
        // Draw Number
        Texture indexTexture = mCaches.GetSupportNumber(game.Metadata().SupportIndex(), mPictoHeight);
        w = indexTexture.Size().x();
        x -= w;
        indexTexture.Render(x, y, w, mPictoHeight, false, tl, tr, br, bl);
      }
      // left part
      Texture leftTexture = mCaches.GetSupportLeftPart(mPictoHeight);
      w = leftTexture.Size().x();
      x -= w;
      leftTexture.Render(x, y, w, mPictoHeight, false, tl, tr, br, bl);
    }
  }
}

void LogoGamelist::RenderDecorations(const Rectangle& area, const FileData& game, int fading, bool selected) const
{
  if (mDecorations == 0) return;
  // Regions
  int x = (int)area.Right() + mPictoHMargin;
  int y = (int)area.Top() + mPictoVMargin;
  Colors::ColorRGBA tl = HalfColor(mDecorationColorTopLeft, fading, !selected);
  Colors::ColorRGBA tr = HalfColor(mDecorationColorTopRight, fading, !selected);
  Colors::ColorRGBA br = HalfColor(mDecorationColorBottomRight, fading, !selected);
  Colors::ColorRGBA bl = HalfColor(mDecorationColorBottomLeft, fading, !selected);
  if (hasFlag(mDecorations, RecalboxConf::GamelistDecoration::Regions))
  {
    x -= mPictoWidth + mPictoHMargin;
    for (int r = Regions::RegionPack::sMaxRegions; --r >= 0;)
      if (Regions::GameRegions region = game.Metadata().Region().Regions[r]; region != Regions::GameRegions::Unknown)
      {
        // Draw
        mCaches.GetFlag(region, mPictoHeight).Render(x, y, mPictoWidth, mPictoHeight, false,tl, tr, br, bl);
        y += mPictoHeight + mPictoVMargin;
      }
  }

  if (hasFlags(mDecorations, RecalboxConf::GamelistDecoration::Genre, RecalboxConf::GamelistDecoration::Players, RecalboxConf::GamelistDecoration::SupportType))
  {
    x -= mPictoWidth + mPictoHMargin;
    y = (int)area.Top() + mPictoVMargin;
    // Genre
    if (hasFlag(mDecorations, RecalboxConf::GamelistDecoration::Genre))
      if (game.Metadata().GenreId() != GameGenres::None)
      {
        mCaches.GetGenre(game.Metadata().GenreId(), mPictoHeight).Render(x, y, mPictoWidth, mPictoHeight, false, tl, tr, br, bl);
        y += mPictoHeight + mPictoVMargin;
      }

    // Players
    if (hasFlag(mDecorations, RecalboxConf::GamelistDecoration::Players))
    {
      mCaches.GetPlayers(game.Metadata().PlayerMin(), game.Metadata().PlayerMax(), mPictoHeight).Render(x, y, mPictoWidth, mPictoHeight, false, tl, tr, br, bl);
      y += mPictoHeight + mPictoVMargin;
    }

    // Support type
    if (hasFlag(mDecorations, RecalboxConf::GamelistDecoration::SupportType))
      if (game.Metadata().SupportType() != SupportTypes::Unknown)
      {
        // Draw side
        Texture typeTexture = mCaches.GetSupportType(game.Metadata().SupportType(), mPictoHeight);
        //int w = typeTexture.Size().x();
        typeTexture.Render(x, y, mPictoWidth, mPictoHeight, false, tl, tr, br, bl);
      }
  }

  // Metadata
  if (System().IsVirtual() && (game.Metadata().Favorite() || game.Metadata().Hidden() || game.Metadata().Adult()))
  {
    x -= mPictoHeight + mPictoHMargin;
    y = (int)area.Top() + mPictoVMargin;
    if (game.Metadata().Favorite())
      if (Texture t = mCaches.GetMetadataIconType(MetadataIcons::Favorite, mPictoHeight); t.MakeReady())
      {
        t.Render(x, y, mPictoHeight, mPictoHeight, true, false, 0xFFC00000 | fading);
        y += mPictoHeight + mPictoVMargin;
      }
    if (game.Metadata().Hidden())
      if (Texture t = mCaches.GetMetadataIconType(MetadataIcons::Hidden, mPictoHeight); t.MakeReady())
      {
        t.Render(x, y, mPictoHeight, mPictoHeight, true, false, 0x80808000 | fading);
        y += mPictoHeight + mPictoVMargin;
      }
    if (game.Metadata().Adult())
      if (Texture t = mCaches.GetMetadataIconType(MetadataIcons::Adult, mPictoHeight); t.MakeReady())
      {
        t.Render(x, y, mPictoHeight, mPictoHeight, true, false, 0xFF80FF00 | fading);
        y += mPictoHeight + mPictoVMargin;
      }
  }

  // Supports
  if (hasFlag(mDecorations, RecalboxConf::GamelistDecoration::SupportNumber))
  {
    x = (int)area.Right();
    y = (int)area.Bottom() - mPictoVMargin - mPictoHeight;
    if (game.Metadata().Region().Count() == sVerticalDivider)
      x -= mPictoWidth + mPictoHMargin;

    if (game.Metadata().HasSupportInformation())
    {
      // Right part
      Texture rightTexture = mCaches.GetSupportRightPart(mPictoHeight);
      int w = rightTexture.Size().x();
      x -= w;
      rightTexture.Render(x, y, w, mPictoHeight, false, tl, tr, br, bl);
      // Total?
      if (game.Metadata().SupportTotal() != 0)
      {
        // Draw total
        Texture totalTexture = mCaches.GetSupportNumber(game.Metadata().SupportTotal(), mPictoHeight);
        w = totalTexture.Size().x();
        x -= w;
        totalTexture.Render(x, y, w, mPictoHeight, false, tl, tr, br, bl);
        // Need separator ?
        if (game.Metadata().SupportIndex() != 0 || game.Metadata().SupportSide() != SupportSides::None)
        {
          // Draw separator
          Texture separatorTexture = mCaches.GetSupportSeparator(mPictoHeight);
          w = separatorTexture.Size().x();
          x -= w;
          separatorTexture.Render(x, y, w, mPictoHeight, false, tl, tr, br, bl);
        }
      }
      // Side ?
      if (game.Metadata().SupportSide() != SupportSides::None)
      {
        // Draw side
        Texture sideTexture = mCaches.GetSupportSide(game.Metadata().SupportSide(), mPictoHeight);
        w = sideTexture.Size().x();
        x -= w;
        sideTexture.Render(x, y, w, mPictoHeight, false, tl, tr, br, bl);
      }
      // Number ?
      if (game.Metadata().SupportIndex() != 0)
      {
        // Draw Number
        Texture indexTexture = mCaches.GetSupportNumber(game.Metadata().SupportIndex(), mPictoHeight);
        w = indexTexture.Size().x();
        x -= w;
        indexTexture.Render(x, y, w, mPictoHeight, false, tl, tr, br, bl);
      }
      // left part
      Texture leftTexture = mCaches.GetSupportLeftPart(mPictoHeight);
      w = leftTexture.Size().x();
      x -= w;
      leftTexture.Render(x, y, w, mPictoHeight, false, tl, tr, br, bl);
    }
  }
}

void LogoGamelist::RenderMetadataIcons(const Rectangle& area, const FileData& game, int fading, bool selected) const
{
  (void)selected;
  if (game.IsGame())
  {
    int height = Math::roundi(area.Height() / 3.f);
    Rectangle icon(area); icon.SetSize((float)height, (float)height).Round();
    if (game.Metadata().Favorite())
    {
      if (Texture t = mCaches.GetMetadataIconType(MetadataIcons::Favorite, height); t.MakeReady())
        t.Render((int)icon.Left(), (int)icon.Top(), (int)icon.Width(), (int)icon.Height(), true, false, 0xFFC00000 | fading);
    }
    else if (game.Metadata().Hidden())
      if (Texture t = mCaches.GetMetadataIconType(MetadataIcons::Hidden, height); t.MakeReady())
        t.Render((int)icon.Left(), (int)icon.Top(), (int)icon.Width(), (int)icon.Height(), true, false, 0x80808000 | fading);

    if (game.Metadata().Adult())
    {
      icon.Move(0, (float)(2 * height));
      if (Texture t = mCaches.GetMetadataIconType(MetadataIcons::Adult, height); t.MakeReady())
        t.Render((int)icon.Left(), (int)icon.Top(), (int)icon.Width(), (int)icon.Height(), true, false, 0xFF80FF00 | fading);
    }
  }
}

int LogoGamelist::MoveToParent(int index) const
{
  // Up to the first non child item
  while(index >= 0 && AbstractGamelist::IsChildAt(index)) --index;
  return index;
}

int LogoGamelist::MoveToChild(int index)
{
  // Up to the first non child item
  if (AbstractGamelist::GetRawEntryAt(index).IsUnfolded() &&
      index < Count() - 1 &&
      AbstractGamelist::GetRawEntryAt(index + 1).IsChild())
    index++;
  return index;
}

void LogoGamelist::JumpToNextLetter(bool forward)
{
  // Get selected index
  int cursorIndex = Target();
  // Up to the first non child item
  cursorIndex = MoveToParent(cursorIndex);
  // Get first letter
  String::Unicode baseChar = String::UpperUnicode(GetNameAt(cursorIndex).ReadFirstUTF8()); // Change to dynamic naming ASAP

  int max = GetGameCount();
  int step = max + (forward ? 1 : -1);
  for(int i = cursorIndex; (i = (i + step) % max) != cursorIndex; )
    if (String::Unicode unicode = String::UpperUnicode(GetNameAt(i).ReadFirstUTF8()); unicode != baseChar && !AbstractGamelist::IsChildAt(i)) // Change to dynamic naming ASAP
    {
      /*if (!forward) // In backward move, go to the latest same letter
      {
        baseChar = unicode;
        for(int j = i; (j = (j + step) % max) != cursorIndex; )
          if (TextListComponent::getObjectAt(j).IsChild()) continue;
          else if (String::UpperUnicode(GetSelectedNameAt(j).ReadFirstUTF8()) == baseChar) i = j;
          else break;
      }*/
      SetSelectedGameIndex(i, false);
      break;
    }
}

void LogoGamelist::BaseListIndexChanged(int newIndex, FileData* game)
{
  ResetMarquee();
  mEvents.OnGameSelected(*this, game, newIndex);

  // Manage selection extension
  AbstractGamelist::Entry& entry = AbstractGamelist::GetRawEntryAt(newIndex);
  mExtendDown = entry.IsUnfolded();
}

bool LogoGamelist::ProcessListInput(const InputCompactEvent& event)
{
  if (HasGame())
  {
    AbstractGamelist::Entry& entry = AbstractGamelist::GetRawEntry();
    FileData* game = entry.mGame;
    int index = Index();

    // Horizontal move
    if ((event.AnyPrimaryRightReleased() || event.AnyPrimaryLeftReleased()) && entry.IsUnfolded())
    {
      if (index < Count() - 1)
        if (AbstractGamelist::Entry& child = AbstractGamelist::GetRawEntryAt(index + 1); child.IsChild())
          MoveChild(child, event.AnyPrimaryRightReleased(), index + 1);
      return true;
    }

    // Vertical move
    if (event.AnyPrimaryDownPressed()) { StartScrolling(  1); return true; }
    if (event.AnyPrimaryUpPressed())   { StartScrolling( -1); return true; }
    if (event.R2Pressed())             { StartScrolling( 10); return true; }
    if (event.L2Pressed())             { StartScrolling(-10); return true; }
    if (event.AnyPrimaryDownReleased() || event.AnyPrimaryUpReleased() || event.R2Released() || event.L2Released()) { StartScrolling(0); return true; }

    if (event.ValidReleased())
    {
      // ============ SELECTION

      if (game->IsGame()) mEvents.OnGameValidated(*this, game, index);
      else if (game->IsFolder()) mEvents.OnFolderValidated(*this, (FolderData*)game, index);

      // ============ FOLD/UNFOLD

      else if (game->IsHeader())
      {
        HeaderData* header = (HeaderData*)game;
        if (header->IsFolded()) mEvents.OnUnfoldHeader(*this, header, index);
        else mEvents.OnFoldHeader(*this, header, index);
      }

      return true;
    }

    if (event.AnythingReleased())
    {
      // ============ FOLD/UNFOLD

      if (event.AnyHotkeyCombination())
      {
        bool vertical = event.HotkeyDownReleased() || event.HotkeyUpReleased();
        if (event.HotkeyLeftReleased()  && !vertical) { index = MoveToParent(index); game = GetGameAt(index); mEvents.OnFoldGame(*this, game, index); return true; }
        if (event.HotkeyRightReleased() && !vertical) { mEvents.OnUnfoldGame(*this, game, index); return true; }
        if (event.HotkeyUpReleased())                 { mEvents.OnFoldAllGame(*this); return true; }
        if (event.HotkeyDownReleased())               { mEvents.OnUnfoldAllGame(*this); return true; }
      }

      // ============ MOVES

      if (event.AnyHotkeyCombination())
      {
        if (event.HotkeyL1Released()) { MoveToPreviousHeader(); return true; }
        if (event.HotkeyR1Released()) { MoveToNextHeader(); return true; }
      }

      if (event.L1Released() || (!RecalboxConf::Instance().GetQuickSystemSelect() && event.AnyPrimaryLeftReleased())) { JumpToNextLetter(false); return true; }
      if (event.R1Released() || (!RecalboxConf::Instance().GetQuickSystemSelect() && event.AnyPrimaryRightReleased())) { JumpToNextLetter(true); return true; }
    }
  }
  return AbstractGamelist::ProcessInput(event);
}

void LogoGamelist::Update(int deltaTime)
{
  mTimeCounter += deltaTime;

  if (mScrollbarTimer > 0)
    if (mScrollbarTimer -= deltaTime; mScrollbarTimer < 0)
      mScrollbarTimer = 0;

  AbstractGamelist::DoScrolling(deltaTime);

  mShowScrollbar = IsReallyScrolling();
  if (mShowScrollbar)
  {
    mScrollbarTimer = sBarFadeTime + sBarFixedTime;
    mShowScrollbar = false;
  }

  if (mExtendDown) { if (mExtensionDown < mSelectorHeight) mExtensionDown = (mExtensionDown + mSelectorHeight + 1) / 2; }
  else             { if (mExtensionDown > 0              ) mExtensionDown = mExtensionDown / 2;                         }

  ProcessMarquee(deltaTime);

  AbstractGamelist<LogoEntry>::Update(deltaTime);
}

void LogoGamelist::Render(const Transform4x4f& parentTrans)
{
  Transform4x4f trans = parentTrans * getTransform();
  trans.round();
  Renderer::SetMatrix(trans);

  // Global clipping
  Renderer::Instance().Clip(trans, mSize);

  // Get y center
  Rectangle globalArea(0.f, 0.f, mSize.x(), mSize.y());
  globalArea.Round();
  //Renderer::DrawRectangle(globalArea, 0xFF00FF80);
  int y = (int)(globalArea.YCenter() - ((float)mSelectorHeight / 2.f));
  y -= ((Offset() * mSelectorHeight) >> 16) + (mExtensionDown / 2);

  // Get forward item count
  int yMax = y;
  int forwardCount = 1;
  int yLimit = (int)globalArea.Bottom();
  while (yMax + mSelectorHeight < yLimit) { yMax += mSelectorHeight; forwardCount++; }
  // Get backward item count
  int backwardCount = Math::roundi((((float)y - globalArea.Top()) / (float)mSelectorHeight) + .5f);

  // Render background
  int mainIndex = Index();
  RenderItemBackground(globalArea, -backwardCount, forwardCount, yMax, mainIndex);

  // Draw selector
  float radius = (float)Math::max(mHorizontalMargin, mVerticalMargin);
  Renderer::DrawRoundRectangle(0, (int)globalArea.YCenter() - (mSelectorHeight / 2) - (mExtensionDown / 2),
                               (int)globalArea.Width(), mSelectorHeight + mExtensionDown, radius, mSelectorColor);

  // Foreground loop
  int yItem = yMax;
  for (int i = forwardCount; --i >= -backwardCount; )
  {
    // Draw logos
    Entry& entry = GetRawEntryAt(mainIndex + i);
    FileData& game = *entry.mGame;
    Texture& front = entry.mGame->IsFolder() ? mFolder : entry.mData.mFront;
    Texture& back = entry.mData.mBack;
    if (front.MakeReady() || back.MakeReady())
    {
      LogoEntry& logoEntry = entry.mData;

      // Get item rectangle
      Rectangle itemArea(globalArea.Left(), (float)yItem, globalArea.Width(), (float)mSelectorHeight);
      itemArea.Contract((float)mHorizontalMargin, (float)mVerticalMargin);

      // Increase logo opacity
      if (logoEntry.mFrontOpacity < 255) logoEntry.mFrontOpacity = Math::clampi(logoEntry.mFrontOpacity + 20, 0, 255);
      // Items out of current range are dimmed
      int shiftColor = ((unsigned int)(mainIndex + i) >= (unsigned int)Count()) ? 2 : 0;

      // Draw system logo
      if (System().IsVirtual() && game.IsGame())
        RenderSystemLogo(itemArea, game, logoEntry.mFrontOpacity >> shiftColor, trans, entry.IsChild(), !entry.IsUnfolded());

      // Render item
      if (game.IsGame())
      {
        // Render background
        RenderGameBackground(itemArea, entry, shiftColor);

        // Render left metadata icon for non virtual systems
        if (!System().IsVirtual() && (!entry.IsChild() || entry.mData.mChildIndex != 0))
          RenderMetadataIcons(itemArea, game, logoEntry.mFrontOpacity >> shiftColor, i == 0);

        // Render foreground
        RenderGameForeground(itemArea, entry, shiftColor, i);
      }
      else if (game.IsFolder()) RenderFolder(itemArea, CasedName(entry), logoEntry, shiftColor);
      else if (game.IsHeader()) RenderHeader(itemArea, (HeaderData*)entry.mGame, logoEntry, shiftColor);
    }

    // Previous y position
    yItem -= mSelectorHeight;
  }

  // Scrollbar
  Renderer::SetMatrix(trans);
  RenderScrollbar(globalArea, mainIndex);

  // Unclip
  Renderer::Instance().Unclip();
}

void LogoGamelist::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
  if (hasFlag(properties, ThemePropertyCategory::Color))
  {
    if (element.HasProperty(ThemePropertyName::SelectorColor))  mSelectorColor = (unsigned int)element.AsInt(ThemePropertyName::SelectorColor);
    if (element.HasProperty(ThemePropertyName::SelectedColor))  mSelectedColor = (unsigned int)element.AsInt(ThemePropertyName::SelectedColor);
    if (element.HasProperty(ThemePropertyName::PrimaryColor))   SetColor(0, (unsigned int)element.AsInt(ThemePropertyName::PrimaryColor));
    if (element.HasProperty(ThemePropertyName::SecondaryColor)) SetColor(1, (unsigned int)element.AsInt(ThemePropertyName::SecondaryColor));
  }

  mFont = &FontManager::Instance().FromTheme(element, properties, mFont);
  mFolderFont = &FontManager::Instance().FromFont(*mFont, (float)(mSelectorHeight >> 1), true);

  if (hasFlag(properties, ThemePropertyCategory::Text))
  {
    if (element.HasProperty(ThemePropertyName::Alignment)) mAlignment = AlignmentExtractHorizontal(element.AsAlignment(ThemePropertyName::Alignment));
    if (element.HasProperty(ThemePropertyName::HorizontalMargin))
    {
      mHorizontalMargin = Math::roundi(element.AsFloat(ThemePropertyName::HorizontalMargin) * ((this->mParent != nullptr) ? this->mParent->getSize().x() : Renderer::Instance().DisplayWidthAsFloat()));
      mVerticalMargin = mHorizontalMargin / 2;
    }
    if (element.HasProperty(ThemePropertyName::VerticalMargin))
      mVerticalMargin = Math::roundi(element.AsFloat(ThemePropertyName::VerticalMargin) * ((this->mParent != nullptr) ? this->mParent->getSize().y() : Renderer::Instance().DisplayHeightAsFloat()));

    if (element.HasProperty(ThemePropertyName::ForceUppercase)) mUppercase = element.AsBool(ThemePropertyName::ForceUppercase);
    else mUppercase = true;

    if (element.HasProperty(ThemePropertyName::SelectorHeight)) mSelectorHeight = Math::roundi(element.AsFloat(ThemePropertyName::SelectorHeight) * Renderer::Instance().DisplayHeightAsFloat());
    else mSelectorHeight = mFont->Height() * 4;
  }

  if (mSelectorHeight <= 0) mSelectorHeight = Math::roundi(0.1f * Renderer::Instance().DisplayHeightAsFloat()); // Security

  // Floor the row height on small displays so logos stay readable, even when the theme picks a small ratio.
  if (true || Renderer::Instance().Is480pOrLower())
  {
    int floorHeight = Math::roundi(0.20f * Renderer::Instance().DisplayHeightAsFloat());
    if (mSelectorHeight < floorHeight) mSelectorHeight = floorHeight;
  }

  SetItemHeightInPixel(mSelectorHeight);
  UpdateScrollbarColor(); // #TODO: Include color in theme
  GeneratePictoSize();
  mFolder.SetTargetSize(mSelectorHeight, (mSelectorHeight * 4) / 3);
}

Path LogoGamelist::GetPrimaryMediaPath(const FileData* game)
{
  return game->Metadata().Logo();
}

Path LogoGamelist::GetSecondaryMediaPath(const FileData* game)
{
  Path background(game->Metadata().Image());
  if (background.IsEmpty()) background = Path(":/back1.png");
  return background;
}
