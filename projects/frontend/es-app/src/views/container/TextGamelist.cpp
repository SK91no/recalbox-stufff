//
// Created by bkg2k on 11/06/25.
//

#include "TextGamelist.h"
#include <games/FolderData.h>
#include <systems/SystemData.h>

String TextGamelist::sFolderPrefix("\uF07C ");
String TextGamelist::sHiddenPrefix("\uF070");
String TextGamelist::sOpenPrefix("▼");
String TextGamelist::sClosePrefix("▶");
String TextGamelist::sNoFoldPrefix("•");
String TextGamelist::sNoArcadePrefix("\uF1C2");
String TextGamelist::sArcadeParentPrefix("\uF1F0");
String TextGamelist::sArcadeClonePrefix("\uF1F1");
String TextGamelist::sArcadeOrphanPrefix("\uF1F2");
String TextGamelist::sArcadeBiosPrefix("\uF1F3");

TextGamelist::TextGamelist(WindowManager& window, SystemData* system, IGamelistEvents& eventReceiver, PictogramCaches& pictogramCaches)
  : TextListComponent<TypedFileData>(window)
  , mPictogramCaches(pictogramCaches)
  , mEventReceiver(eventReceiver)
  , mSystem(system)
  , mIndividualDecorations(system == nullptr)
  , mFlagWidth(0)
  , mFlagMargin(Renderer::Instance().Is480pOrLower() ? 1 : 2)
  , mFolderPrefixWidth(0)
  , mHiddenPrefixWidth(0)
  , mOpenPrefixWidth(0)
  , mClosePrefixWidth(0)
  , mNoFoldPrefixWidth(0)
  , mNoArcadePrefixWidth(0)
  , mArcadeParentPrefixWidth(0)
  , mArcadeClonePrefixWidth(0)
  , mArcadeOrphanPrefixWidth(0)
  , mArcadeBiosPrefixWidth(0)
  , mDecorationColorTopLeft(0xFFFFFFFF)
  , mDecorationColorTopRight(0xFFFFFFFF)
  , mDecorationColorBottomLeft(0xFFFFFFFF)
  , mDecorationColorBottomRight(0xFFFFFFFF)
  , mSystemDecoration(system != nullptr ? RecalboxConf::Instance().GetSystemGamelistDecoration(*system) : RecalboxConf::GamelistDecoration::None)
  , mHeaderStars(getFont()->Height())
{
  // Default "theming"
  const MenuThemeData& menu = ThemeManager::Instance().Menu();
  setFont(menu.Text().font);
  SetColor(0, menu.Text().color);
  SetColor(1, menu.Text().color);
  setSelectorColor(menu.Text().selectorColor);
  setSelectedColor(menu.Text().selectedColor);
  SetDefaultSelectorHeight();
  setHorizontalMargin(0.01f * Renderer::Instance().DisplayWidthAsFloat());
  setAlignment(HorizontalAlignment::Left);

  // Set callback interfaces
  SetCursorChangedCallback(this);
  SetOverlayInterface(this);
}

bool TextGamelist::SetSelectedGame(FileData* game, bool instant)
{
  (void)instant;
  bool ok = setCursor({ .mData=game, .mType=GameType::Normal });
  if (!ok)
  {
    if (game != nullptr) RebuildFolderStack(game);
    ok = setCursor({ .mData=game, .mType=GameType::Normal });
  }
  mEventReceiver.OnGameSelected(*this, TextListComponent<TypedFileData>::getSelected().mData, TextListComponent<TypedFileData>::getCursorIndex());
  return ok;
}

IGamelistContainer& TextGamelist::RebuildFolderStack(FileData* from)
{
  FolderData* folder = from->Parent();
  mFolderStack.Clear();
  while((folder != nullptr) && !folder->IsRoot())
  {
    mFolderStack.Push(folder);
    folder = folder->Parent();
  }
  mFolderStack.Reverse();
  return *this;
}

String::List TextGamelist::GetBreadcrumb()
{
  String::List result;
  for(FolderData* folder : mFolderStack)
    result.push_back(folder->Metadata().RomFileOnly().Filename());
  return result;
}

IGamelistContainer& TextGamelist::EnterFolder(FolderData* folder)
{
  if (LookupGameIndex(folder) < 0) RebuildFolderStack(folder);
  mFolderStack.Push(folder);
  return *this;
}

FolderData* TextGamelist::ExitFolder()
{
  if (!mFolderStack.Empty()) mFolderStack.PopAndDelete();
  return !mFolderStack.Empty() ? mFolderStack.Peek() : &mSystem->MasterRoot();
}

FileData::List TextGamelist::GetHeaderGames(HeaderData* header)
{
  FileData::List result;
  bool found = false;
  for(auto& entry : TextListComponent::mEntries)
  {
    if (found && entry.object.mData->IsGame())
      result.Add(entry.object.mData);
    if (entry.object.mData->IsHeader())
    {
      if (found) break; // found another header, just break;
      if (entry.object.mData == header) found = true; // found the requested header
    }
  }
  return result;
}

bool TextGamelist::MoveToNextHeader()
{
  for(int i = TextListComponent::getCursorIndex() + 1; i < TextListComponent::size(); ++i)
    if (TextListComponent::getObjectAt(i).mData->IsHeader())
    {
      TextListComponent::setCursorIndex(i);
      return getCursorIndex() == i;
    }
  return false;
}

bool TextGamelist::MoveToPreviousHeader()
{
  for(int i = TextListComponent::getCursorIndex(); --i >= 0;)
    if (TextListComponent::getObjectAt(i).mData->IsHeader())
    {
      TextListComponent::setCursorIndex(i);
      return getCursorIndex() == i;
    }
  return false;
}

IGamelistContainer& TextGamelist::MoveRelative(int step)
{
  int position = GetSelectedGameIndex();
  // Add step & clamp into list range
  int newPosition = Math::clampi(position + step, 0, (int)mEntries.size());
  // If the position was already on boundaries, wrap
  if (position == newPosition) newPosition = newPosition == 0 ? (int)mEntries.size() - 1 : 0;
  // Set
  SetSelectedGameIndex(newPosition, true);
  return *this;
}

  void TextGamelist::JumpToNextLetter(bool forward)
  {
    // Get selected index
    int cursorIndex = GetSelectedGameIndex();
    // Up to the first non child item
    while(cursorIndex >= 0 && TextListComponent::getObjectAt(cursorIndex).IsChild()) --cursorIndex;
    // Get first letter
    String::Unicode baseChar = String::UpperUnicode(GetNameAt(cursorIndex).ReadFirstUTF8()); // Change to dynamic naming ASAP

    int max = GetGameCount();
    int step = max + (forward ? 1 : -1);
    for(int i = cursorIndex; (i = (i + step) % max) != cursorIndex; )
      if (String::Unicode unicode = String::UpperUnicode(GetNameAt(i).ReadFirstUTF8()); unicode != baseChar && !TextListComponent::getObjectAt(i).IsChild()) // Change to dynamic naming ASAP
      {
        /*if (!forward) // In backward move, go to the latest same letter
        {
          baseChar = unicode;
          for(int j = i; (j = (j + step) % max) != cursorIndex; )
            if (TextListComponent::getObjectAt(j).IsChild()) continue;
            else if (String::UpperUnicode(GetSelectedNameAt(j).ReadFirstUTF8()) == baseChar) i = j;
            else break;
        }*/
        SetSelectedGameIndex(i, true);
        break;
      }
  }

/* ===================================================
 *  Component
 * =================================================== */

void TextGamelist::Update(int deltatime)
{
  TextListComponent::Update(deltatime);
}

void TextGamelist::Render(const Transform4x4f& parentTrans)
{
  // Calculate some constant per frame
  mFlagWidth = (int)((float)getFont()->Height() * (71.f / 48.f));

  // Render
  TextListComponent::Render(parentTrans);
}

bool TextGamelist::ProcessListInput(const InputCompactEvent& event)
{
  if (!mEntries.empty())
  {
    FileData* game = GetSelectedGame();
    int index = GetSelectedGameIndex();

    if (event.ValidReleased())
    {
      // ============ SELECTION

      if (game->IsGame()) mEventReceiver.OnGameValidated(*this, game, index);
      else if (game->IsFolder()) mEventReceiver.OnFolderValidated(*this, (FolderData*)game, index);

      // ============ FOLD/UNFOLD

      else if (game->IsHeader())
      {
        HeaderData* header = (HeaderData*)game;
        if (header->IsFolded()) mEventReceiver.OnUnfoldHeader(*this, header, index);
        else mEventReceiver.OnFoldHeader(*this, header, index);
      }

      return true;
    }

    if (event.AnythingReleased())
    {
      // ============ FOLD/UNFOLD

      if (game->IsHeader())
        if (event.AnyHotkeyCombination())
        {
          bool vertical = event.HotkeyDownReleased() || event.HotkeyUpReleased();
          HeaderData* header = (HeaderData*)game;
          if (event.HotkeyLeftReleased()  && !header->IsFolded() && !vertical) { mEventReceiver.OnFoldHeader(*this, header, index); return true; }
          if (event.HotkeyRightReleased() && header->IsFolded()  && !vertical) { mEventReceiver.OnUnfoldHeader(*this, header, index); return true; }
          //if (event.HotkeyUpReleased())                 { mEventReceiver.OnFoldAllHeader(*this); return true; }
          //if (event.HotkeyDownReleased())               { mEventReceiver.OnUnfoldAllHeader(*this); return true; }
        }

      if (game->IsGame())
        if (event.AnyHotkeyCombination())
        {
          bool vertical = event.HotkeyDownReleased() || event.HotkeyUpReleased();
          if (event.HotkeyLeftReleased()  && !vertical) { index = MoveToParent(index); game = GetGameAt(index); mEventReceiver.OnFoldGame(*this, game, index); return true; }
          if (event.HotkeyRightReleased() && !vertical) { mEventReceiver.OnUnfoldGame(*this, game, index); return true; }
          if (event.HotkeyUpReleased())                 { index = MoveToParent(index); game = GetGameAt(index); mEventReceiver.OnFoldAllGame(*this); return true; }
          if (event.HotkeyDownReleased())               { mEventReceiver.OnUnfoldAllGame(*this); return true; }
        }

      // ============ MOVES

      if (event.AnyHotkeyCombination())
      {
        if (event.HotkeyL1Released()) { MoveToPreviousHeader(); return true; }
        if (event.HotkeyR1Released()) { MoveToNextHeader(); return true; }
      }

      if (event.L2Released() || (!RecalboxConf::Instance().GetQuickSystemSelect() && event.AnyPrimaryLeftReleased())) { MoveRelative(-10); TextListComponent::PopScrollbar(); return true; }
      if (event.R2Released() || (!RecalboxConf::Instance().GetQuickSystemSelect() && event.AnyPrimaryRightReleased())) { MoveRelative(10); TextListComponent::PopScrollbar(); return true; }

      if (event.L1Released() || (!RecalboxConf::Instance().GetQuickSystemSelect() && event.AnyPrimaryLeftReleased())) { JumpToNextLetter(false); TextListComponent::PopScrollbar(); return true; }
      if (event.R1Released() || (!RecalboxConf::Instance().GetQuickSystemSelect() && event.AnyPrimaryRightReleased())) { JumpToNextLetter(true); TextListComponent::PopScrollbar(); return true; }
    }
  }
  return TextListComponent::ProcessInput(event);
}

int TextGamelist::MoveToParent(int index)
{
  // Up to the first non child item
  while(index >= 0 && TextListComponent::getObjectAt(index).IsChild()) --index;
  return index;
}

/* ===================================================
 *  Left Decorations
 * =================================================== */

/* ===================================================
 *  Right Decorations
 * =================================================== */

RecalboxConf::GamelistDecoration TextGamelist::Decoration(FileData* game)
{
  if (mSystem != nullptr) return mSystemDecoration;
  RecalboxConf::GamelistDecoration* decoration = mSystemDecorations.try_get(&game->System());
  if (decoration != nullptr) return *decoration;
  RecalboxConf::GamelistDecoration newDecoration = RecalboxConf::Instance().GetSystemGamelistDecoration(game->System());
  mSystemDecorations[&game->System()] = newDecoration;
  return newDecoration;
}

/* ===================================================
 *  List overlays
 * =================================================== */

void TextGamelist::OverlayApply(const Transform4x4f& parentTrans, int leftWidth, int rightWidth, int labelWidth, const Rectangle& area, TypedFileData const& data, int index, unsigned int& color)
{
  (void)labelWidth;
  (void)parentTrans;
  (void)color;
  (void)index;

  int flagHeight = getFont()->Height();

  // Right part
  if (rightWidth != 0 && data.mData->IsGame())
  {
    int y = (int) (area.Top() + ((area.Height() - (float) flagHeight) / 2.f));
    int x = (int)(area.Left() + area.Width());

    // Color shift
    bool fade = data.mData != getSelected().mData;

    /*
     * ----------- Fixed length decorations
     */

    RecalboxConf::GamelistDecoration decorations = Decoration(data.mData);

    // Genre
    if (hasFlag(decorations, RecalboxConf::GamelistDecoration::Genre))
      if (data.mData->Metadata().GenreId() != GameGenres::None)
      {
        Texture genreTexture = mPictogramCaches.GetGenre(data.mData->Metadata().GenreId(), flagHeight);
        // Draw
        x -= mFlagWidth;
        genreTexture.Render(x, y, mFlagWidth, flagHeight, false,
                            HalfColor(mDecorationColorTopLeft, fade),
                            HalfColor(mDecorationColorTopRight, fade),
                            HalfColor(mDecorationColorBottomRight, fade),
                            HalfColor(mDecorationColorBottomLeft, fade));
        x -= mFlagMargin;
      }

    // Players
    if (hasFlag(decorations, RecalboxConf::GamelistDecoration::Players))
    {
      Texture playerTexture = mPictogramCaches.GetPlayers(data.mData->Metadata().PlayerMin(), data.mData->Metadata().PlayerMax(), flagHeight);
      // Draw
      x -= mFlagWidth;
      playerTexture.Render(x, y, mFlagWidth, flagHeight, false,
                           HalfColor(mDecorationColorTopLeft, fade),
                           HalfColor(mDecorationColorTopRight, fade),
                           HalfColor(mDecorationColorBottomRight, fade),
                           HalfColor(mDecorationColorBottomLeft, fade));
      x -= mFlagMargin;
    }

    // Support type
    if (hasFlag(decorations, RecalboxConf::GamelistDecoration::SupportType))
      if (data.mData->Metadata().SupportType() != SupportTypes::Unknown)
      {
        // Draw side
        Texture typeTexture = mPictogramCaches.GetSupportType(data.mData->Metadata().SupportType(), flagHeight);
        int w = typeTexture.Size().x();
        x -= w;
        typeTexture.Render(x, y, w, flagHeight, false,
                           HalfColor(mDecorationColorTopLeft, fade),
                           HalfColor(mDecorationColorTopRight, fade),
                           HalfColor(mDecorationColorBottomRight, fade),
                           HalfColor(mDecorationColorBottomLeft, fade));
        x -= mFlagMargin;
      }

    /*
     * ----------- Variable length decorations
     */

    // Regions
    if (hasFlag(decorations, RecalboxConf::GamelistDecoration::Regions))
    {
      for (int r = Regions::RegionPack::sMaxRegions; --r >= 0;)
        if (Regions::GameRegions region = data.mData->Metadata().Region().Regions[r]; region != Regions::GameRegions::Unknown)
        {
          Texture flagTexture = mPictogramCaches.GetFlag(region, flagHeight);
          // Draw
          x -= mFlagWidth;
          flagTexture.Render(x, y, mFlagWidth, flagHeight, false,
                             HalfColor(mDecorationColorTopLeft, fade),
                             HalfColor(mDecorationColorTopRight, fade),
                             HalfColor(mDecorationColorBottomRight, fade),
                             HalfColor(mDecorationColorBottomLeft, fade));
          x -= mFlagMargin;
        }
    }

    // Supports
    if (hasFlag(decorations, RecalboxConf::GamelistDecoration::SupportNumber))
      if (data.mData->Metadata().HasSupportInformation())
      {
        // Right part
        Texture rightTexture = mPictogramCaches.GetSupportRightPart(flagHeight);
        int w = rightTexture.Size().x();
        x -= w;
        rightTexture.Render(x, y, w, flagHeight, false,
                            HalfColor(mDecorationColorTopLeft, fade),
                            HalfColor(mDecorationColorTopRight, fade),
                            HalfColor(mDecorationColorBottomRight, fade),
                            HalfColor(mDecorationColorBottomLeft, fade));
        // Total?
        if (data.mData->Metadata().SupportTotal() != 0)
        {
          // Draw total
          Texture totalTexture = mPictogramCaches.GetSupportNumber(data.mData->Metadata().SupportTotal(), flagHeight);
          w = totalTexture.Size().x();
          x -= w;
          totalTexture.Render(x, y, w, flagHeight, false,
                              HalfColor(mDecorationColorTopLeft, fade),
                              HalfColor(mDecorationColorTopRight, fade),
                              HalfColor(mDecorationColorBottomRight, fade),
                              HalfColor(mDecorationColorBottomLeft, fade));
          // Need separator ?
          if (data.mData->Metadata().SupportIndex() != 0 || data.mData->Metadata().SupportSide() != SupportSides::None)
          {
            // Draw separator
            Texture separatorTexture = mPictogramCaches.GetSupportSeparator(flagHeight);
            w = separatorTexture.Size().x();
            x -= w;
            separatorTexture.Render(x, y, w, flagHeight, false,
                                    HalfColor(mDecorationColorTopLeft, fade),
                                    HalfColor(mDecorationColorTopRight, fade),
                                    HalfColor(mDecorationColorBottomRight, fade),
                                    HalfColor(mDecorationColorBottomLeft, fade));
          }
        }
        // Side ?
        if (data.mData->Metadata().SupportSide() != SupportSides::None)
        {
          // Draw side
          Texture sideTexture = mPictogramCaches.GetSupportSide(data.mData->Metadata().SupportSide(), flagHeight);
          w = sideTexture.Size().x();
          x -= w;
          sideTexture.Render(x, y, w, flagHeight, false,
                             HalfColor(mDecorationColorTopLeft, fade),
                             HalfColor(mDecorationColorTopRight, fade),
                             HalfColor(mDecorationColorBottomRight, fade),
                             HalfColor(mDecorationColorBottomLeft, fade));
        }
        // Number ?
        if (data.mData->Metadata().SupportIndex() != 0)
        {
          // Draw Number
          Texture indexTexture = mPictogramCaches.GetSupportNumber(data.mData->Metadata().SupportIndex(), flagHeight);
          w = indexTexture.Size().x();
          x -= w;
          indexTexture.Render(x, y, w, flagHeight, false,
                              HalfColor(mDecorationColorTopLeft, fade),
                              HalfColor(mDecorationColorTopRight, fade),
                              HalfColor(mDecorationColorBottomRight, fade),
                              HalfColor(mDecorationColorBottomLeft, fade));
        }
        // left part
        Texture leftTexture = mPictogramCaches.GetSupportLeftPart(flagHeight);
        w = leftTexture.Size().x();
        x -= w;
        leftTexture.Render(x, y, w, flagHeight, false,
                           HalfColor(mDecorationColorTopLeft, fade),
                           HalfColor(mDecorationColorTopRight, fade),
                           HalfColor(mDecorationColorBottomRight, fade),
                           HalfColor(mDecorationColorBottomLeft, fade));
        //x -= mFlagMargin;
      }
  }

  // Left part
  if (leftWidth != 0)
  {
    if (data.mData->IsGame())
    {
      String prefix;
      Rectangle indent(area);
      // Hierarchy
      switch(data.mType)
      {
        case GameType::Normal:
        case GameType::ParentGameFolded:
        case GameType::ParentGameUnfolded: break;
        case GameType::ChildGame: indent.MoveLeft((float)getFont()->Height()); break;
        case GameType::ArcadeParentAlone:
        case GameType::ArcadeParentFolded:
        case GameType::ArcadeParentUnfolded: prefix.Append(sArcadeParentPrefix); break;
        case GameType::ArcadeClone: indent.MoveLeft((float)getFont()->Height()); prefix.Append(sArcadeClonePrefix); break;
        case GameType::ArcadeOrphan: prefix.Append(sArcadeOrphanPrefix); break;
        case GameType::ArcadeBios: prefix.Append(sArcadeBiosPrefix); break;
        case GameType::ArcadeIllegal: prefix.Append(sNoArcadePrefix); break;
      }
      // Open/Close
      switch(data.mType)
      {
        case GameType::ParentGameFolded:
        case GameType::ArcadeParentFolded: prefix.Append(sClosePrefix); break;
        case GameType::ParentGameUnfolded:
        case GameType::ArcadeParentUnfolded: prefix.Append(sOpenPrefix); break;
        case GameType::ArcadeParentAlone:
        case GameType::ArcadeClone:
        case GameType::ArcadeOrphan: prefix.Append(sNoFoldPrefix); break;
        case GameType::ArcadeBios:
        case GameType::ArcadeIllegal:
        case GameType::Normal:
        case GameType::ChildGame:
        default: break;
      }
      // Crossed out eye for hidden things
      if (data.mData->Metadata().Hidden()) prefix.Append(sHiddenPrefix);
      // System icon, for Favorite games
      if (mSystem == nullptr || mSystem->IsVirtual() || data.mData->Metadata().Favorite()) prefix.Append(data.mData->System().Descriptor().IconPrefix());

      getFont()->DrawText(prefix, indent, color, ::Alignment::CenterLeft);
    }
    else if (data.mData->IsFolder())
      getFont()->DrawText(sFolderPrefix, area, color, ::Alignment::CenterLeft);
    else if (data.mData->IsHeader())
      switch(((HeaderData*)data.mData)->Sort())
      {
        case FileSorts::Sorts::FileNameAscending:
        case FileSorts::Sorts::FileNameDescending:
        case FileSorts::Sorts::SystemAscending:
        case FileSorts::Sorts::SystemDescending: break;
        case FileSorts::Sorts::RatingAscending:
        case FileSorts::Sorts::RatingDescending:
        {
          int h = mHeaderStars.Height();
          int y = (int)(area.Top() + ((area.Height() - (float) h) / 2.f));
          mHeaderStars.Render(0, (float)y, ((HeaderData*)data.mData)->Float(), 0xFFFFFFFF);
          break;
        }
        case FileSorts::Sorts::PlayCountAscending:
        case FileSorts::Sorts::PlayCountDescending:
        case FileSorts::Sorts::TotalPlayTimeAscending:
        case FileSorts::Sorts::TotalPlayTimeDescending:
        case FileSorts::Sorts::LastPlayedAscending:
        case FileSorts::Sorts::LastPlayedDescending: break;
        case FileSorts::Sorts::PlayersAscending:
        case FileSorts::Sorts::PlayersDescending:
        {
          int playerMinMax = ((HeaderData*)data.mData)->Int();
          int playerMax = playerMinMax >> 16;
          int playerMin = playerMinMax & 0xFFFF;
          Texture playersTexture = mPictogramCaches.GetPlayers(playerMin, playerMax, flagHeight);
          // Draw
          int h = getFont()->Height();
          int y = (int)(area.Top() + ((area.Height() - (float) h) / 2.f));
          playersTexture.Render(0, y, mFlagWidth, h, true, 0xFFFFFFFF);
          break;
        }
        case FileSorts::Sorts::DeveloperAscending:
        case FileSorts::Sorts::DeveloperDescending:
        case FileSorts::Sorts::PublisherAscending:
        case FileSorts::Sorts::PublisherDescending: break;
        case FileSorts::Sorts::GenreAscending:
        case FileSorts::Sorts::GenreDescending:
        {
          GameGenres genre = (GameGenres)((HeaderData*)data.mData)->Int();
          if (genre != GameGenres::None)
          {
            Texture genreTexture = mPictogramCaches.GetGenre(genre, flagHeight);
            // Draw
            int h = getFont()->Height();
            int y = (int)(area.Top() + ((area.Height() - (float) h) / 2.f));
            genreTexture.Render(0, y, mFlagWidth, h, true, 0xFFFFFFFF);
          }
          break;
        }
        case FileSorts::Sorts::ReleaseDateAscending:
        case FileSorts::Sorts::ReleaseDateDescending: break;
        case FileSorts::Sorts::RegionAscending:
        case FileSorts::Sorts::RegionDescending:
        {
          Regions::RegionPack pack;
          pack.Pack = ((HeaderData*)data.mData)->Int();
          int drawn = 0;
          int y = (int) (area.Top() + ((area.Height() - (float) flagHeight) / 2.f));
          for (int r = Regions::RegionPack::sMaxRegions; --r >= 0;)
            if (Regions::GameRegions region = pack.Regions[r]; region != Regions::GameRegions::Unknown)
            {
              Texture flagTexture = mPictogramCaches.GetFlag(region, flagHeight);
              // Draw
              int x = (int)area.Left() + ((mFlagMargin + mFlagWidth) * drawn);
              flagTexture.Render(x, y, mFlagWidth, flagHeight, true,
                                 mDecorationColorTopLeft,
                                 mDecorationColorTopRight,
                                 mDecorationColorBottomRight,
                                 mDecorationColorBottomLeft);
              drawn++;
            }
          break;
        }
        default: break;
      }
  }
}

float TextGamelist::OverlayGetLeftOffset(TypedFileData const& data, int labelWidth)
{
  (void)labelWidth;

  // Game
  if (data.mData->IsGame())
  {
    float width = 0.f;
    // Open/Close
    switch(data.mType)
    {
      case GameType::ParentGameFolded:
      case GameType::ArcadeParentFolded: width += (float)mClosePrefixWidth; break;
      case GameType::ParentGameUnfolded:
      case GameType::ArcadeParentUnfolded: width += (float)mOpenPrefixWidth; break;
      case GameType::ArcadeParentAlone:
      case GameType::ArcadeClone:
      case GameType::ArcadeOrphan: width += (float)mNoFoldPrefixWidth; break;
      case GameType::ArcadeBios:
      case GameType::ArcadeIllegal:
      case GameType::Normal:
      case GameType::ChildGame:
      default: break;
    }
    // Hierarchy
    switch(data.mType)
    {
      case GameType::Normal:
      case GameType::ParentGameFolded:
      case GameType::ParentGameUnfolded: break;
      case GameType::ChildGame: width += (float)getFont()->Height(); break;
      case GameType::ArcadeParentAlone:
      case GameType::ArcadeParentFolded:
      case GameType::ArcadeParentUnfolded: width += (float)mArcadeParentPrefixWidth; break;
      case GameType::ArcadeClone: width += (float)getFont()->Height() + (float)mArcadeClonePrefixWidth; break;
      case GameType::ArcadeOrphan: width += (float)mArcadeOrphanPrefixWidth; break;
      case GameType::ArcadeBios: width += (float)mArcadeBiosPrefixWidth; break;
      case GameType::ArcadeIllegal: width += (float)mNoArcadePrefixWidth; break;
    }
    // Crossed out eye for hidden things
    if (data.mData->Metadata().Hidden()) width += (float)mHiddenPrefixWidth;
    // System icon, for Favorite games
    if (mSystem == nullptr || mSystem->IsVirtual() || data.mData->Metadata().Favorite()) width += (float)getFont()->TextWidth(data.mData->System().Descriptor().IconPrefix());

    if (width != 0) width += (float)getFont()->CharWidth(' ');
    return width;
  }
  // Folders
  if (data.mData->IsFolder()) return (float)mFolderPrefixWidth;
  // Header
  if (data.mData->IsHeader())
    switch(((HeaderData*)data.mData)->Sort())
    {
      case FileSorts::Sorts::RatingAscending:
      case FileSorts::Sorts::RatingDescending:
      {
        mHeaderStars.ResetSize(getFont()->Height());
        return (float)mHeaderStars.Width() + (float)mFlagMargin;
      }
      case FileSorts::Sorts::PlayersAscending:
      case FileSorts::Sorts::PlayersDescending:
      case FileSorts::Sorts::GenreAscending:
      case FileSorts::Sorts::GenreDescending: return (float)(mFlagWidth + mFlagMargin);
      case FileSorts::Sorts::FileNameAscending:
      case FileSorts::Sorts::FileNameDescending:
      case FileSorts::Sorts::SystemAscending:
      case FileSorts::Sorts::SystemDescending:
      case FileSorts::Sorts::DeveloperAscending:
      case FileSorts::Sorts::DeveloperDescending:
      case FileSorts::Sorts::PublisherAscending:
      case FileSorts::Sorts::PublisherDescending:
      case FileSorts::Sorts::PlayCountAscending:
      case FileSorts::Sorts::PlayCountDescending:
      case FileSorts::Sorts::TotalPlayTimeAscending:
      case FileSorts::Sorts::TotalPlayTimeDescending:
      case FileSorts::Sorts::LastPlayedAscending:
      case FileSorts::Sorts::LastPlayedDescending:
      case FileSorts::Sorts::ReleaseDateAscending:
      case FileSorts::Sorts::ReleaseDateDescending: break;
      case FileSorts::Sorts::RegionAscending:
      case FileSorts::Sorts::RegionDescending:
      {
        Regions::RegionPack pack;
        pack.Pack = ((HeaderData*)data.mData)->Int();
        int regionCount = pack.Count();
        return (float)((mFlagWidth + mFlagMargin) * regionCount);
      }
      default: break;
    }

  return 0.f;
}

float TextGamelist::OverlayGetRightOffset(TypedFileData const& data, int labelWidth)
{
  (void)labelWidth;
  if (data.mData->IsHeader()) return 0.f;

  RecalboxConf::GamelistDecoration decoration = Decoration(data.mData);

  int result = 0;
  if (hasFlag(decoration, RecalboxConf::GamelistDecoration::Regions))
  {
    int regionCount = data.mData->Metadata().Region().Count();
    result += (mFlagWidth + mFlagMargin) * regionCount;
  }
  if (hasFlag(decoration, RecalboxConf::GamelistDecoration::Players))
    result += mFlagWidth + mFlagMargin;
  if (hasFlag(decoration, RecalboxConf::GamelistDecoration::Genre))
    result += mFlagWidth + mFlagMargin;
  if ((hasFlag(decoration, RecalboxConf::GamelistDecoration::SupportNumber) && data.mData->Metadata().HasSupportInformation()) ||
      hasFlag(decoration, RecalboxConf::GamelistDecoration::SupportType))
  {
    int flagHeight = getFont()->Height();
    if (hasFlag(decoration, RecalboxConf::GamelistDecoration::SupportType))
    {
      if (data.mData->Metadata().SupportType() != SupportTypes::Unknown)
        result += mPictogramCaches.GetSupportType(data.mData->Metadata().SupportType(), flagHeight).Size().x();
    }
    if (hasFlag(decoration, RecalboxConf::GamelistDecoration::SupportNumber))
    {
      result += mPictogramCaches.GetSupportRightPart(flagHeight).Size().x();
      if (data.mData->Metadata().SupportTotal() != 0)
      {
        result += mPictogramCaches.GetSupportNumber(data.mData->Metadata().SupportTotal(), flagHeight).Size().x();
        if (data.mData->Metadata().SupportIndex() != 0 || data.mData->Metadata().SupportSide() != SupportSides::None)
          result += mPictogramCaches.GetSupportSeparator(flagHeight).Size().x();
      }
      if (data.mData->Metadata().SupportSide() != SupportSides::None)
        result += mPictogramCaches.GetSupportSide(data.mData->Metadata().SupportSide(), flagHeight).Size().x();
      if (data.mData->Metadata().SupportIndex() != 0)
        result += mPictogramCaches.GetSupportNumber(data.mData->Metadata().SupportIndex(), flagHeight).Size().x();
      result += mPictogramCaches.GetSupportLeftPart(flagHeight).Size().x() + mFlagMargin;
    }
  }

  return (float)result;
}

void TextGamelist::OnFontChanged()
{
  mFolderPrefixWidth = getFont()->TextWidth(sFolderPrefix);
  mHiddenPrefixWidth = getFont()->TextWidth(sHiddenPrefix);
  mOpenPrefixWidth = getFont()->TextWidth(sOpenPrefix);
  mClosePrefixWidth = getFont()->TextWidth(sClosePrefix);
  mNoFoldPrefixWidth = getFont()->TextWidth(sNoFoldPrefix);
  mNoArcadePrefixWidth = getFont()->TextWidth(sNoArcadePrefix);
  mArcadeParentPrefixWidth = getFont()->TextWidth(sArcadeParentPrefix);
  mArcadeClonePrefixWidth = getFont()->TextWidth(sArcadeClonePrefix);
  mArcadeOrphanPrefixWidth = getFont()->TextWidth(sArcadeOrphanPrefix);
  mArcadeBiosPrefixWidth = getFont()->TextWidth(sArcadeBiosPrefix);
}
