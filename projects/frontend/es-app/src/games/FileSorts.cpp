#include "utils/locale/LocaleHelper.h"
#include "FileSorts.h"
#include "systems/SystemData.h"

bool FileSorts::sUseDatabaseNames = false;
bool FileSorts::sUseFileName = false;
bool FileSorts::sUseTopFavorites = false;

void FileSorts::Initialize()
{
  mAllSorts.clear();
  mAllSorts.push_back(SortType(Sorts::FileNameAscending      , &compareFileName     , &compareFileNameArcade     , true , "\uF15d " + _("NAME")));
  mAllSorts.push_back(SortType(Sorts::FileNameDescending     , &compareFileName     , &compareFileNameArcade     , false, "\uF15e " + _("NAME")));
  mAllSorts.push_back(SortType(Sorts::RatingAscending        , &compareRating       , &compareRatingArcade       , true , "\uF165 " + _("RATING")));
  mAllSorts.push_back(SortType(Sorts::RatingDescending       , &compareRating       , &compareRatingArcade       , false, "\uF164 " + _("RATING")));
  mAllSorts.push_back(SortType(Sorts::PlayCountAscending     , &comparePlayCount    , &comparePlayCountArcade    , true , "\uF162 " + _("TIMES PLAYED")));
  mAllSorts.push_back(SortType(Sorts::PlayCountDescending    , &comparePlayCount    , &comparePlayCountArcade    , false, "\uF163 " + _("TIMES PLAYED")));
  mAllSorts.push_back(SortType(Sorts::TotalPlayTimeAscending , &compareTotalPlayTime, &compareTotalPlayTimeArcade, true , "\uF16A " + _("TOTAL PLAYING TIME")));
  mAllSorts.push_back(SortType(Sorts::TotalPlayTimeDescending, &compareTotalPlayTime, &compareTotalPlayTimeArcade, false, "\uF16B " + _("TOTAL PLAYING TIME")));
  mAllSorts.push_back(SortType(Sorts::LastPlayedAscending    , &compareLastPlayed   , &compareLastPlayedArcade   , true , "\uF168 " + _("LAST PLAYED")));
  mAllSorts.push_back(SortType(Sorts::LastPlayedDescending   , &compareLastPlayed   , &compareLastPlayedArcade   , false, "\uF169 " + _("LAST PLAYED")));
  mAllSorts.push_back(SortType(Sorts::RegionAscending        , &compareRegion       , &compareRegionArcade       , true , "\uF16C " + _("REGIONS")));
  mAllSorts.push_back(SortType(Sorts::RegionDescending       , &compareRegion       , &compareRegionArcade       , false, "\uF16D " + _("REGIONS")));
  mAllSorts.push_back(SortType(Sorts::PlayersAscending       , &compareNumberPlayers, &compareNumberPlayersArcade, true , "\uF162 " + _("NUMBER OF PLAYERS")));
  mAllSorts.push_back(SortType(Sorts::PlayersDescending      , &compareNumberPlayers, &compareNumberPlayersArcade, false, "\uF163 " + _("NUMBER OF PLAYERS")));
  mAllSorts.push_back(SortType(Sorts::DeveloperAscending     , &compareDevelopper   , &compareDevelopperArcade   , true , "\uF15d " + _("DEVELOPER")));
  mAllSorts.push_back(SortType(Sorts::DeveloperDescending    , &compareDevelopper   , &compareDevelopperArcade   , false, "\uF15e " + _("DEVELOPER")));
  mAllSorts.push_back(SortType(Sorts::PublisherAscending     , &comparePublisher    , &comparePublisherArcade    , true , "\uF15d " + _("PUBLISHER")));
  mAllSorts.push_back(SortType(Sorts::PublisherDescending    , &comparePublisher    , &comparePublisherArcade    , false, "\uF15e " + _("PUBLISHER")));
  mAllSorts.push_back(SortType(Sorts::GenreAscending         , &compareGenre        , &compareGenreArcade        , true , "\uF15d " + _("GENRE")));
  mAllSorts.push_back(SortType(Sorts::GenreDescending        , &compareGenre        , &compareGenreArcade        , false, "\uF15e " + _("GENRE")));
  mAllSorts.push_back(SortType(Sorts::SystemAscending        , &compareSystemName   , &compareFileNameArcade     , true , "\uF166 " + _("SYSTEM NAME")));
  mAllSorts.push_back(SortType(Sorts::SystemDescending       , &compareSystemName   , &compareFileNameArcade     , false, "\uF167 " + _("SYSTEM NAME")));
  mAllSorts.push_back(SortType(Sorts::ReleaseDateAscending   , &compareReleaseDate  , &compareReleaseDateArcade  , true , "\uF168 " + _("RELEASE DATE")));
  mAllSorts.push_back(SortType(Sorts::ReleaseDateDescending  , &compareReleaseDate  , &compareReleaseDateArcade  , false, "\uF169 " + _("RELEASE DATE")));
}

int FileSorts::supportCompare(const FileData& fd1, const FileData& fd2)
{
  bool hasSupport1 = fd1.Metadata().HasSupportInformation();
  bool hasSupport2 = fd2.Metadata().HasSupportInformation();
  // If at leas one has no support
  if (!hasSupport1 || !hasSupport2)
  {
    if (!hasSupport1) return hasSupport2 ? -1 : 0; // fd1 has no support => first
    return 1; // fd2 has no support => first
  }
  // Compare region first, tu gather region series
  if (fd1.Metadata().Region().Pack != fd2.Metadata().Region().Pack)
    return fd1.Metadata().Region().Pack - fd2.Metadata().Region().Pack;
  // Compare total
  if (fd1.Metadata().SupportTotal() != fd2.Metadata().SupportTotal())
    return fd1.Metadata().SupportTotal() - fd2.Metadata().SupportTotal();
  // Compare index
  if (fd1.Metadata().SupportIndex() != fd2.Metadata().SupportIndex())
    return fd1.Metadata().SupportIndex() - fd2.Metadata().SupportIndex();
  // Compare faces
  return (int)fd1.Metadata().SupportSide() - (int)fd2.Metadata().SupportSide();
}

int FileSorts::unicodeCompareUppercase(const String& a, const String& b)
{
  for (int apos = 0, bpos = 0;; )
  {
    int c1 = (int)String::UpperUnicode(a.ReadUTF8(apos));
    int c2 = (int)String::UpperUnicode(b.ReadUTF8(bpos));
    if ((c1 | c2) == 0) return 0;
    int c = c1 - c2;
    if (c != 0) return c;
  }
}

int FileSorts::compareFoldersAndGames(const FileData& fd1, const FileData& fd2)
{
  ItemType f1 = fd1.Type();
  ItemType f2 = fd2.Type();
  if (f1 == f2)
  {
    if (sUseTopFavorites && f1 == ItemType::Game)
    {
      bool fv1 = fd1.Metadata().Favorite() || fd1.Metadata().HasFavoriteChild();
      bool fv2 = fd2.Metadata().Favorite() || fd2.Metadata().HasFavoriteChild();
      if (fv1 == fv2) return 0; // Both are favorites or not
      if (fv1) return -1;       // fv1 is favorite, fv2 is not
      return 1;                 // fv2 is favorite, fv1 is not
    }
    return 0;                            // Both folders
  }
  if (f1 == ItemType::Folder) return -1; // f1 is a folder, f2 is a game
  return 1;                              // f2 is a folder
}

#define CheckFoldersAndGames(f1, f2) { int folderComparison = compareFoldersAndGames(f1, f2); if (folderComparison != 0) return folderComparison; }

ImplementSortMethod(compareSystemName)
{
  const SystemData& system1 = file1.System();
  const SystemData& system2 = file2.System();
  const int result = unicodeCompareUppercase(system1.SortingName(), system2.SortingName());
  if (result != 0) return result;
  return compareFileName(file1, file2);
}

ImplementSortMethod(compareFileName)
{
  CheckFoldersAndGames(file1, file2)
  int result = sUseFileName
             ? unicodeCompareUppercase(file1.Metadata().RomFileOnly().ToString(), file2.Metadata().RomFileOnly().ToString())
             : unicodeCompareUppercase(file1.Name(), file2.Name());
  return result != 0 ? result : supportCompare(file1, file2);
}

ImplementSortMethod(compareRating)
{
  CheckFoldersAndGames(file1, file2)
  float c = file1.Metadata().Rating() - file2.Metadata().Rating();
  if (c < 0) return -1;
  if (c > 0) return 1;
  return compareFileName(file1, file2);
}

ImplementSortMethod(compareTotalPlayTime)
{
  CheckFoldersAndGames(file1, file2)
  int playTime = (file1).Metadata().TotalPlayTime() - (file2).Metadata().TotalPlayTime();
  if (playTime != 0) return playTime;
  return compareFileName(file1, file2);
}

ImplementSortMethod(comparePlayCount)
{
  CheckFoldersAndGames(file1, file2)
  int playCount = (file1).Metadata().PlayCount() - (file2).Metadata().PlayCount();
  if (playCount != 0) return playCount;
  return compareFileName(file1, file2);
}

ImplementSortMethod(compareLastPlayed)
{
  CheckFoldersAndGames(file1, file2)
  unsigned int ep1 = (file1).Metadata().LastPlayedEpoc();
  unsigned int ep2 = (file2).Metadata().LastPlayedEpoc();
  if (ep1 == 0) ep1 = 0xFFFFFFFF;
  if (ep2 == 0) ep2 = 0xFFFFFFFF;
  if (ep1 == ep2)
    return compareFileName(file1, file2);
  return ep1 < ep2 ? -1 : 1;
}

ImplementSortMethod(compareNumberPlayers)
{
  CheckFoldersAndGames(file1, file2)
  int players = (file1).Metadata().PlayerRange() - (file2).Metadata().PlayerRange();
  if (players != 0) return players;
  return compareFileName(file1, file2);
}

ImplementSortMethod(compareRegion)
{
  CheckFoldersAndGames(file1, file2)
  int diff = (file1).Metadata().Region() - (file2).Metadata().Region();
  if (diff != 0) return diff;
  return compareFileName(file1, file2);
}

ImplementSortMethod(compareDevelopper)
{
  CheckFoldersAndGames(file1, file2)
  int result = unicodeCompareUppercase(file1.Metadata().Developer().Trim(), file2.Metadata().Developer().Trim());
  if (result != 0) return result;
   return compareFileName(file1, file2);
}

ImplementSortMethod(comparePublisher)
{
  CheckFoldersAndGames(file1, file2)
  int result = unicodeCompareUppercase(file1.Metadata().Publisher().Trim(), file2.Metadata().Publisher().Trim());
  if (result != 0) return result;
  return compareFileName(file1, file2);
}

ImplementSortMethod(compareGenre)
{
  CheckFoldersAndGames(file1, file2)
  int genre = (int)(file1).Metadata().GenreId() - (int)(file2).Metadata().GenreId();
  if (genre != 0) return genre;
  return compareFileName(file1, file2);
}

ImplementSortMethod(compareReleaseDate)
{
  CheckFoldersAndGames(file1, file2)
  int releasedate = (int)(file1).Metadata().ReleaseDateEpoc() - (int)(file2).Metadata().ReleaseDateEpoc();
  if (releasedate != 0) return releasedate;
  return compareFileName(file1, file2);
}

int FileSorts::compareFileNameArcade(const ArcadeTupple& at1, const ArcadeTupple& at2)
{
  if (sUseDatabaseNames && at1.mArcade != nullptr && at2.mArcade != nullptr)
  {
    CheckFoldersAndGames(*at1.mGame, *at2.mGame)
    return unicodeCompareUppercase(at1.mArcade->ArcadeName(), at2.mArcade->ArcadeName());
  }
  // Both non arcade games
  return compareFileName(*at1.mGame, *at2.mGame);
}

ImplementSortMethodArcade(compareRatingArcade, compareRating)

ImplementSortMethodArcade(comparePlayCountArcade, comparePlayCount)

ImplementSortMethodArcade(compareTotalPlayTimeArcade, compareTotalPlayTime)

ImplementSortMethodArcade(compareLastPlayedArcade, compareLastPlayed)

ImplementSortMethodArcade(compareNumberPlayersArcade, compareNumberPlayers)

ImplementSortMethodArcade(compareDevelopperArcade, compareDevelopper)

ImplementSortMethodArcade(comparePublisherArcade, comparePublisher)

ImplementSortMethodArcade(compareGenreArcade, compareGenre)

ImplementSortMethodArcade(compareReleaseDateArcade, compareReleaseDate)

ImplementSortMethodArcade(compareRegionArcade, compareRegion)

const FileSorts::SortList& FileSorts::AvailableSorts(SortSets set)
{
  switch(set)
  {
    case SortSets::MultiSystem:
    {
      //! Ordered multi-system sorts
      static SortList sMulti =
      {
        Sorts::FileNameAscending,
        Sorts::FileNameDescending,
        Sorts::SystemAscending,
        Sorts::SystemDescending,
        Sorts::GenreAscending,
        Sorts::GenreDescending,
        Sorts::RatingAscending,
        Sorts::RatingDescending,
        Sorts::PlayCountAscending,
        Sorts::PlayCountDescending,
        Sorts::TotalPlayTimeAscending,
        Sorts::TotalPlayTimeDescending,
        Sorts::LastPlayedAscending,
        Sorts::LastPlayedDescending,
        Sorts::RegionAscending,
        Sorts::RegionDescending,
        Sorts::PlayersAscending,
        Sorts::PlayersDescending,
        Sorts::DeveloperAscending,
        Sorts::DeveloperDescending,
        Sorts::PublisherAscending,
        Sorts::PublisherDescending,
        Sorts::ReleaseDateAscending,
        Sorts::ReleaseDateDescending,
      };
      return sMulti;
    }
    case SortSets::Arcade:
    {
      //! Arcade sorts
      static SortList sArcade =
      {
        Sorts::FileNameAscending,
        Sorts::FileNameDescending,
        Sorts::GenreAscending,
        Sorts::GenreDescending,
        Sorts::RatingAscending,
        Sorts::RatingDescending,
        Sorts::PlayCountAscending,
        Sorts::PlayCountDescending,
        Sorts::TotalPlayTimeAscending,
        Sorts::TotalPlayTimeDescending,
        Sorts::LastPlayedAscending,
        Sorts::LastPlayedDescending,
        Sorts::RegionAscending,
        Sorts::RegionDescending,
        Sorts::PlayersAscending,
        Sorts::PlayersDescending,
        Sorts::DeveloperAscending,
        Sorts::DeveloperDescending,
        Sorts::PublisherAscending,
        Sorts::PublisherDescending,
        Sorts::ReleaseDateAscending,
        Sorts::ReleaseDateDescending,
      };
      return sArcade;
    }
    case SortSets::SingleSystem:
    default: break;
  }

  //! Ordered mono-system sorts
  static SortList sSingle =
  {
    Sorts::FileNameAscending,
    Sorts::FileNameDescending,
    Sorts::GenreAscending,
    Sorts::GenreDescending,
    Sorts::RatingAscending,
    Sorts::RatingDescending,
    Sorts::PlayCountAscending,
    Sorts::PlayCountDescending,
    Sorts::TotalPlayTimeAscending,
    Sorts::TotalPlayTimeDescending,
    Sorts::LastPlayedAscending,
    Sorts::LastPlayedDescending,
    Sorts::RegionAscending,
    Sorts::RegionDescending,
    Sorts::PlayersAscending,
    Sorts::PlayersDescending,
    Sorts::DeveloperAscending,
    Sorts::DeveloperDescending,
    Sorts::PublisherAscending,
    Sorts::PublisherDescending,
    Sorts::ReleaseDateAscending,
    Sorts::ReleaseDateDescending,
  };
  return sSingle;
}

const String& FileSorts::Name(FileSorts::Sorts sort)
{
  for(const FileSorts::SortType& sortType : mAllSorts)
    if (sortType.mSort == sort)
      return sortType.mDescription;

  static String unknown("Unknown sort");
  return unknown;
}

bool FileSorts::IsAscending(FileSorts::Sorts sort)
{
  for(const FileSorts::SortType& sortType : mAllSorts)
    if (sortType.mSort == sort)
      return sortType.mAscending;

  return false;
}


FileSorts::Comparer FileSorts::ComparerFromSort(FileSorts::Sorts sort, bool ignoreFavoritePreferences)
{
  // Lazy initialization
  sUseFileName = RecalboxConf::Instance().GetDisplayByFileName();
  sUseTopFavorites = (!ignoreFavoritePreferences && RecalboxConf::Instance().GetFavoritesFirst());

  for(const FileSorts::SortType& sortType : mAllSorts)
    if (sortType.mSort == sort)
      return sortType.mComparer;

  return nullptr;
}

FileSorts::ComparerArcade FileSorts::ComparerArcadeFromSort(FileSorts::Sorts sort, bool ignoreFavoritePreferences)
{
  // Lazy initialization
  sUseFileName = RecalboxConf::Instance().GetDisplayByFileName();
  sUseTopFavorites = (!ignoreFavoritePreferences && RecalboxConf::Instance().GetFavoritesFirst());
  sUseDatabaseNames = RecalboxConf::Instance().GetArcadeUseDatabaseNames();

  for(const FileSorts::SortType& sortType : mAllSorts)
    if (sortType.mSort == sort)
      return sortType.mComparerArcade;

  return nullptr;
}

FileSorts::Sorts FileSorts::Clamp(FileSorts::Sorts sort, FileSorts::SortSets set)
{
  const SortList& sorts = AvailableSorts(set);

  // Sort available in the set?
  for(FileSorts::Sorts availableSort : sorts)
    if (sort == availableSort)
      return sort;

  // Nope, return the first available
  return sorts[0];
}

void FileSorts::SortArcade(ArcadeTupplePointerList& items, ComparerArcade comparer, bool ascending)
{
  if (items.size() > 1)
  {
    if (ascending)
      QuickSortAscendingArcade(items, 0, (int)items.size() - 1, comparer);
    else
      QuickSortDescendingArcade(items, 0, (int)items.size() - 1, comparer);
  }
}

void FileSorts::QuickSortAscendingArcade(ArcadeTupplePointerList& items, int low, int high, ComparerArcade comparer)
{
  int Low = low;
  int High = high;
  const ArcadeTupple& pivot = *items[(Low + High) >> 1];
  do
  {
    while((*comparer)(*items[Low] , pivot) < 0) Low++;
    while((*comparer)(*items[High], pivot) > 0) High--;
    if (Low <= High)
    {
      ArcadeTupple* Tmp = items[Low]; items[Low] = items[High]; items[High] = Tmp;
      Low++; High--;
    }
  }while(Low <= High);
  if (High > low) QuickSortAscendingArcade(items, low, High, comparer);
  if (Low < high) QuickSortAscendingArcade(items, Low, high, comparer);
}

void FileSorts::QuickSortDescendingArcade(ArcadeTupplePointerList& items, int low, int high, ComparerArcade comparer)
{
  int Low = low;
  int High = high;
  const ArcadeTupple& pivot = *items[(Low + High) >> 1];
  do
  {
    while((*comparer)(*items[Low] , pivot) > 0) Low++;
    while((*comparer)(*items[High], pivot) < 0) High--;
    if (Low <= High)
    {
      ArcadeTupple* Tmp = items[Low]; items[Low] = items[High]; items[High] = Tmp;
      Low++; High--;
    }
  }while(Low <= High);
  if (High > low) QuickSortDescendingArcade(items, low, High, comparer);
  if (Low < high) QuickSortDescendingArcade(items, Low, high, comparer);
}

