//
// Created by bkg2k on 31/05/23.
//

#include "ArcadeGameListView.h"
#include <systems/arcade/ArcadeVirtualSystems.h>
#include "utils/locale/LocaleHelper.h"

ArcadeGameListView::ArcadeGameListView(WindowManager& window, SystemManager& systemManager, SystemData& system, const IGlobalVariableResolver& resolver, PictogramCaches& flagCache)
  : DetailedGameListView(window, systemManager, system, resolver, flagCache)
  , mDatabase(nullptr)
{
}

void ArcadeGameListView::populateList(const FolderData& folder, FileData* target = nullptr)
{
  mPopulatedFolder = &folder;

  // Default filter
  FileData::Filter includesFilter = FileData::Filter::Normal | FileData::Filter::Favorite;
  // Favorites only?
  if (RecalboxConf::Instance().GetFavoritesOnly()) includesFilter = FileData::Filter::Favorite;

  // Get items
  bool flatfolders = mSystem.IsAlwaysFlat() || (RecalboxConf::Instance().GetSystemFlatFolders(mSystem));
  FileData::List items;
  if (flatfolders) folder.GetItemsRecursivelyTo(items, includesFilter, mSystem.Excludes(), false);
  else folder.GetItemsTo(items, includesFilter, mSystem.Excludes(), true);

  // Check emptyness
  if (items.Empty())
  {
    if (!Gamelist().IsInRootFolder())
    {
      FolderData* current = Gamelist().GetCurrentFolder();
      populateList(*Gamelist().ExitFolder(), target);
      Gamelist().SetSelectedGame(current, true);
      return;
    }
    // Otherwise, insert en emergency empty item
    items.Add(&mEmptyListItem); // Insert "EMPTY SYSTEM" item
  }

  // Sort
  FileSorts::SortSets set = mSystem.IsVirtual() ? FileSorts::SortSets::MultiSystem : FileSorts::SortSets::Arcade;
  FileSorts::Sorts sort = FileSorts::Clamp(RecalboxConf::Instance().GetSystemSort(mSystem), set);
  FileSorts& sorts = FileSorts::Instance();
  BuildAndSortArcadeGames(items, sorts.ComparerArcadeFromSort(sort, mSystem.IsLastPlayed() || mSystem.IsFavorite()), sorts.IsAscending(sort));
  BuildList(nullptr);
}

void ArcadeGameListView::BuildList(FileData* target)
{
  Gamelist().ClearGames();
  mHeaderText.setText(mSystem.FullName());

  // Empty gamelist might be caused by the lack of database, so fallback to normal view
  if (mGameList.empty())
  {
    DetailedGameListView::populateList(*mPopulatedFolder, nullptr);
    return;
  }

  // Region filtering?
  Regions::GameRegions currentRegion = Regions::Clamp(RecalboxConf::Instance().GetSystemRegionFilter(mSystem));
  bool activeRegionFiltering = false;
  if (currentRegion != Regions::GameRegions::Unknown)
  {
    Regions::List availableRegion = AvailableRegionsInGames(mGameList);
    // Check if our region is in the available ones
    for(Regions::GameRegions region : availableRegion)
    {
      activeRegionFiltering = (region == currentRegion);
      if (activeRegionFiltering) break;
    }
  }

  // Prepare manufacturer filtering
  std::vector<ArcadeDatabase::Manufacturer> manufacturerList = GetManufacturerList();
  HashSet<int> hiddenManufacturers;
  for(const ArcadeDatabase::Manufacturer& manufacturer : manufacturerList)
    if (RecalboxConf::Instance().IsInArcadeSystemHiddenManufacturers(mSystem, manufacturer.Name.empty() ? ArcadeVirtualSystems::sAllOtherManufacturers : manufacturer.Name))
      hiddenManufacturers.insert(manufacturer.Index);
  bool mustHideManufacturers = !hiddenManufacturers.empty();

  // Add to list
  bool filterOutBios = RecalboxConf::Instance().GetArcadeViewHideBios();
  bool filterOutUnknown = RecalboxConf::Instance().GetArcadeViewHideNonWorking();
  bool onlyTate = RecalboxConf::Instance().GetShowOnlyTateGames();
  bool onlyYoko = RecalboxConf::Instance().GetShowOnlyYokoGames();
  FileData* previous = nullptr;
  HeaderData* lastHeader = nullptr;

  // Process favorites first
  bool hasTopFavorites = false;
  FileSorts::SortSets set = mSystem.IsVirtual() ? FileSorts::SortSets::MultiSystem : FileSorts::SortSets::Arcade;
  FileSorts::Sorts sort = FileSorts::Clamp(RecalboxConf::Instance().GetSystemSort(mSystem), set);
  FileSorts& sorts = FileSorts::Instance();
  SetInternalSort(sort);

  if (!IsFavoriteSystem() && RecalboxConf::Instance().GetFavoritesFirst())
  {
    // Has any favorite?
    for(int t = (int)mGameList.size() - 1, i = (int)(mGameList.size() + 1) / 2; --i >= 0; )
      if (mGameList[i].mGame->Metadata().Favorite() || mGameList[t - i].mGame->Metadata().Favorite()) { hasTopFavorites = true; break; }
    // If we have favorite + favorite first + descending order, move favorites on top
    if (hasTopFavorites && !sorts.IsAscending(sort))
    {
      int favoriteCount = 0;
      int favoriteBase = 0;
      for(int i = (int)mGameList.size(); --i >= 0; )
        if (mGameList[i].mGame->Metadata().Favorite())
        {
          for(int j = i + 1; --j >= 0; )
            if (mGameList[j].mGame->Metadata().Favorite()) { favoriteCount++; favoriteBase = j; }
            else break;
          break;
        }
      for(int i = favoriteCount; --i >= 0; )
      {
        ParentTupple tmp = mGameList[i];
        mGameList[i] = mGameList[favoriteBase + i];
        mGameList[favoriteBase + i] = tmp;
      }
    }
  }

  // Add items
  for (const ParentTupple& parent : mGameList)
  {
    int colorIndexOffset = BaseColor;
    if (!parent.mGame->IsFolder())
    {
      if (parent.mArcade == nullptr && filterOutUnknown) continue;
      // Tate filtering only on parent - clones have presumably the same rotation :)
      if (onlyTate && !RotationUtils::IsTate(parent.mGame->Metadata().Rotation())) continue;
      if (onlyYoko && RotationUtils::IsTate(parent.mGame->Metadata().Rotation())) continue;
      // Non games
      GameType type = GameType::ArcadeIllegal;
      if (parent.mArcade != nullptr)
      {
        if (filterOutBios)
          if (parent.mArcade->Hierarchy() == ArcadeGame::Type::Bios) continue;
        if (mustHideManufacturers)
          if (HasMatchingManufacturer(hiddenManufacturers, parent.mArcade->LimitedManufacturer())) continue;

        type = parent.mCloneList == nullptr ? GameType::ArcadeParentAlone : parent.mFolded ? GameType::ArcadeParentFolded : GameType::ArcadeParentUnfolded;
        switch (parent.mArcade->Hierarchy())
        {
          case ArcadeGame::Type::Clone: { LOG(LogFatal) << "[ArcadeGamelistView] Clone detected in parent-only list !"; break; }
          case ArcadeGame::Type::Orphaned: type = GameType::ArcadeOrphan; break;
          case ArcadeGame::Type::Bios: type = GameType::ArcadeBios; break;
          case ArcadeGame::Type::Parent:
          default: break;
        }
      }
      // Region highlighting?
      if (activeRegionFiltering)
        if (!Regions::IsIn4Regions(parent.mGame->Metadata().Region().Pack, currentRegion))
          colorIndexOffset = HighlightColor;

      // Header
      if (parent.mGame->IsGame())
        if (HeaderData* header = NeedHeader(previous, parent.mGame, hasTopFavorites); header != nullptr)
        {
          Gamelist().AddHeader(header, GameColor, HeaderColor);
          lastHeader = header;
        }

      // Store
      if (lastHeader == nullptr || !lastHeader->IsFolded())
        Gamelist().AddGames(parent.mGame, GetDisplayName(parent, *parent.mGame), type, colorIndexOffset + GameColor);
      if (parent.mGame->IsGame())
        previous = parent.mGame;
    }
    else Gamelist().AddFolder((FolderData*)parent.mGame, FolderColor);

    // Children?
    if (lastHeader == nullptr || !lastHeader->IsFolded())
      if (!parent.mFolded && parent.mCloneList != nullptr)
      {
        for (const ArcadeTupple& clone : *parent.mCloneList)
        {
          if (mustHideManufacturers)
            if (HasMatchingManufacturer(hiddenManufacturers, clone.mArcade->LimitedManufacturer())) continue;
          // Region filtering?
          colorIndexOffset = BaseColor;
          if (activeRegionFiltering)
            if (!Regions::IsIn4Regions(clone.mGame->Metadata().Region().Pack, currentRegion))
              colorIndexOffset = HighlightColor;
          // Store
          Gamelist().AddGames(clone.mGame, GetDisplayName(clone, *clone.mGame), GameType::ArcadeClone, colorIndexOffset + GameColor);
        }
      }
  }

  // Check emptyness
  if (!Gamelist().HasGame())
    Gamelist().AddGames(&mEmptyListItem, _S(mEmptyListItem.Name()));

  if (target != nullptr)
    Gamelist().SetSelectedGame(target, true);
  else
    (void)Gamelist().SetSelectedGameIndex(0, true);
}


bool ArcadeGameListView::HasMatchingManufacturer(const HashSet<int>& manufacturerSet, const ArcadeGame::LimitedManufacturerHolder& manufacturers)
{
  for(int i = manufacturers.Count(); --i >= 0; )
    if (manufacturerSet.contains(manufacturers.Manufacturer(i)))
      return true;
  return false;
}

FileData::List ArcadeGameListView::GetHighLevelFileList()
{
  FileData::List items;
  for (const ParentTupple& parent : mGameList)
    if (parent.mCloneList == nullptr)
      items.Add(parent.mGame);
  return items;
}

bool ArcadeGameListView::BuildAndSortArcadeGames(FileData::List& items, FileSorts::ComparerArcade comparer, bool ascending)
{
  // Split lists
  ParentTuppleList parents;
  HashMap<const FileData*, ArcadeTuppleList*> clones;
  ParentTuppleList orphaned;
  ParentTuppleList bios;
  ParentTuppleList notWorking;
  ParentTuppleList folders;
  // Virtual arcade only - Avoid too much lookups by keeping already found database localy
  // and by comparing parent pointers so that database changes only when parent changes
  HashMap<const FolderData*, const ArcadeDatabase*> mDatabaseLookup;
  FolderData* previousParent = nullptr;

  // Initialize database early for True arcade systems
  if (mSystem.IsTrueArcade())
  {
    mDatabase = mSystem.ArcadeDatabases().LookupDatabase(*mPopulatedFolder, mDefaultEmulator, mDefaultCore);
    if (mDatabase == nullptr || !mDatabase->IsValid())
      return false;
  }

  // Get initial fold status
  bool folded = RecalboxConf::Instance().GetArcadeViewFoldClones();

  mGameList.clear();
  for(FileData* item : items)
  {
    if (item->Parent() !=  previousParent)
    {
      previousParent = item->Parent();
      const ArcadeDatabase** db = mDatabaseLookup.try_get(previousParent);
      if (db != nullptr) mDatabase = *db;
      else
      {
        mDatabase = previousParent->System().ArcadeDatabases().LookupDatabase(*previousParent);
        mDatabaseLookup[previousParent] = mDatabase;
      }
    }
    // Safety check
    if (mDatabase == nullptr)
    {
      LOG(LogError) << "[ArcadeGameListView] No database for game " << item->RomPath().ToString() << " in system " << mSystem.FullName();
      continue;
    }
    // Lookup game from the current database
    const ArcadeGame* arcade = mDatabase->LookupGame(*item);
    // Distribute tuples in lists according to their type
    // Clones are stored in parent's lists
    if (arcade == nullptr)
    {
      if (item->IsFolder()) folders.push_back(ParentTupple(nullptr, item, false));
      else notWorking.push_back(ParentTupple(nullptr, item, false));
    }
    else
      switch(arcade->Hierarchy())
      {
        case ArcadeGame::Type::Parent: parents.push_back(ParentTupple(arcade, item, folded)); break;
        case ArcadeGame::Type::Clone:
        {
          if (clones[arcade->Parent()] == nullptr) clones[arcade->Parent()] = new ArcadeTuppleList();
          clones[arcade->Parent()]->push_back(ArcadeTupple(arcade, item)); break;
        }
        case ArcadeGame::Type::Orphaned: orphaned.push_back(ParentTupple(arcade, item, false)); break;
        case ArcadeGame::Type::Bios: bios.push_back(ParentTupple(arcade, item, false)); break;
      }
  }

  // Folders first
  AddSortedCategories({ &folders }, comparer, ascending);

  // Sorts parents / orphaned
  AddSortedCategories({ &parents, &orphaned }, comparer, ascending);

  // Insert clones as children
  for(ParentTupple& parent : mGameList)
  {
    if (parent.mGame->IsFolder()) continue; // Ignore folders
    if (parent.mArcade->Hierarchy() != ArcadeGame::Type::Parent) continue;
    ArcadeTuppleList** children = clones.try_get(parent.mGame);
    if (children != nullptr)
    {
      ArcadeTupplePointerList sortedList;
      for(ArcadeTupple& clone : **children) sortedList.push_back(&clone);
      FileSorts::SortArcade(sortedList, comparer, ascending); // Sort clones for a single parent
      ArcadeTuppleList* dynamicCloneList = new ArcadeTuppleList();
      for(ArcadeTupple* item : sortedList) dynamicCloneList->push_back(*item);
      parent.AddClones(dynamicCloneList);
      delete *children;
      clones.erase(parent.mGame);
    }
  }

  // Sort & add bios & unknowns
  AddSortedCategories({ &bios }, comparer, ascending);
  AddSortedCategories({ &notWorking }, comparer, ascending);

  // For virtual arcade systems, cleanup database
  if (mSystem.IsVirtualArcade()) mDatabase = nullptr;

  return true;
}

void ArcadeGameListView::AddSortedCategories(const std::vector<ParentTuppleList*>& categoryLists, FileSorts::ComparerArcade comparer, bool ascending)
{
  ArcadeTupplePointerList sortedList;
  for(ParentTuppleList* categoryList : categoryLists)
    for(ParentTupple& item : *categoryList) sortedList.push_back(&item);
  FileSorts::SortArcade(sortedList, comparer, ascending); // Sort bios
  for(ArcadeTupple* item : sortedList) mGameList.push_back(*((ParentTupple*)item));
}

Regions::List ArcadeGameListView::AvailableRegionsInGames(ArcadeGameListView::ParentTuppleList& games)
{
  bool regionIndexes[256];
  memset(regionIndexes, 0, sizeof(regionIndexes));
  // Run through all games
  for(const ParentTupple& tupple : games)
  {
    unsigned int fourRegions = tupple.mGame->Metadata().Region().Pack;
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

void ArcadeGameListView::FoldAll()
{
  // Get cursor position - Get ancestor if its a clone
  FileData* item = Gamelist().GetSelectedGame();
  const ArcadeDatabase* database = item->System().ArcadeDatabases().LookupDatabase(*item->Parent());
  if (database == nullptr) return;

  const ArcadeGame* arcade = database->LookupGame(*item);
  if (arcade != nullptr)
    if (arcade->Hierarchy() == ArcadeGame::Type::Clone)
      item = (FileData*)arcade->Parent();

  // Fold all
  for(ParentTupple& parent : mGameList)
    if (parent.mCloneList != nullptr)
      if (parent.mArcade->Hierarchy() == ArcadeGame::Type::Parent)
        parent.mFolded = true;

  // Rebuild the UI list
  BuildList(item);
}

void ArcadeGameListView::UnfoldAll()
{
  // Get cursor position
  FileData* item = Gamelist().GetSelectedGame();

  // Fold all
  for(ParentTupple& parent : mGameList)
    if (parent.mCloneList != nullptr)
      if (parent.mArcade->Hierarchy() == ArcadeGame::Type::Parent)
        parent.mFolded = false;

  // Rebuild the UI list
  BuildList(item);
}

void ArcadeGameListView::Fold(FileData* item)
{
  // Get cursor position - Get ancestor if its a clone
  const ArcadeDatabase* database = item->System().ArcadeDatabases().LookupDatabase(*item->Parent());
  if (database == nullptr) return;

  const ArcadeGame* arcade = database->LookupGame(*item);
  if (arcade != nullptr)
    if (arcade->Hierarchy() == ArcadeGame::Type::Clone)
      item = (FileData*)arcade->Parent();

  // Fold all
  bool rebuild =false;
  for(ParentTupple& parent : mGameList)
    if (parent.mCloneList != nullptr)
      if (parent.mArcade->Hierarchy() == ArcadeGame::Type::Parent)
        if (parent.mGame == item)
        {
          parent.mFolded = true;
          rebuild = true;
        }

  // Rebuild the UI list
  if (rebuild) BuildList(item);
}

void ArcadeGameListView::Unfold(FileData* item)
{
  // Fold
  bool rebuild =false;
  for(ParentTupple& parent : mGameList)
    if (parent.mGame == item)
      if (parent.mCloneList != nullptr)
        if (parent.mArcade->Hierarchy() == ArcadeGame::Type::Parent)
        {
          parent.mFolded = false;
          rebuild = true;
        }

  // Rebuild the UI list
  if (rebuild) BuildList(item);
}

std::vector<ArcadeDatabase::Manufacturer> ArcadeGameListView::GetManufacturerList() const
{
  if (mDatabase == nullptr) return std::vector<ArcadeDatabase::Manufacturer>();
  std::vector<ArcadeDatabase::Manufacturer> result = mDatabase->GetLimitedManufacturerList();
  return result;
}

int ArcadeGameListView::GetGameCountForManufacturer(int driverIndex) const
{
  int count = 0;
  for(const ParentTupple& parent : mGameList)
  {
    if (parent.mArcade == nullptr) continue;
    if (parent.mArcade->LimitedManufacturer().Contains(driverIndex)) count++;
    if (parent.mCloneList != nullptr)
      for (const ArcadeTupple& clone: *parent.mCloneList)
        if (clone.mArcade->LimitedManufacturer().Contains(driverIndex)) count++;
  }
  return count;
}

const ArcadeTupple& ArcadeGameListView::Lookup(const FileData& item)
{
  for(const ParentTupple& parent : mGameList)
    if (parent.mGame == &item) return parent;
    else if (parent.mCloneList != nullptr)
      for(const ArcadeTupple& clone : *parent.mCloneList)
        if (clone.mGame == &item)
          return clone;

  { LOG(LogError) << "[ArcadeGameListView] Lookup FileData failed for game " << item.Name(); }
  static ArcadeTupple nullTupple(nullptr, nullptr);
  return nullTupple;
}

String ArcadeGameListView::GetRawDisplayName(const FileData& game)
{
  return GetDisplayName(Lookup(game), game);
}

String ArcadeGameListView::GetDisplayName(const ArcadeTupple& game, const FileData& original)
{
  if (RecalboxConf::Instance().GetArcadeUseDatabaseNames() && game.mArcade != nullptr)
    return game.mArcade->ArcadeName();
  if (game.mGame != nullptr)
    return RecalboxConf::Instance().GetDisplayByFileName() ? game.mGame->Metadata().RomFileOnly().ToString() : game.mGame->Name();
  return RecalboxConf::Instance().GetDisplayByFileName() ? original.Metadata().RomFileOnly().ToString() : original.Name();
}

String ArcadeGameListView::GetDescription(FileData& game)
{
  String emulator;
  String core;
  if (const ArcadeDatabase* database = game.System().ArcadeDatabases().LookupDatabase(game, emulator, core); database != nullptr)
    if (const ArcadeGame* arcade = database->LookupGame(game); arcade != nullptr)
    {
      if (arcade->EmulationStatus() == ArcadeGame::Status::Imperfect)
        return (_F(_("{0} reports the emulation status of this game is 'imperfect'.")) / core).ToString().Append(String::LF, 2).Append(game.Metadata().Description());
      if (arcade->EmulationStatus() == ArcadeGame::Status::Preliminary)
        return (_F(_("{0} reports the emulation status of this game is 'preliminary'. You should expect issues such as bugs or even crashes!")) / core).ToString().Append(String::LF, 2).Append(game.Metadata().Description());
    }
  return game.Metadata().Description();
}

void ArcadeGameListView::RefreshItem(FileData* game)
{
  if (game == nullptr || !game->IsGame()) { LOG(LogError) << "[DetailedGameListView] Trying to refresh null or empty item"; return; }
  Gamelist().Rename(game, GetRawDisplayName(*game));
}


