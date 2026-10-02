#include "FileData.h"
#include <systems/SystemData.h>
#include <games/adapter/GameAdapter.h>

#include <utils/Zip.h>
#include <utils/hash/Crc32File.h>
#include <games/FolderData.h>

FileData::FileData(ItemType type, const Path& path, RootFolderData& ancestor)
	: mTopAncestor(ancestor)
  , mParent(nullptr)
  , mType(type)
  , mMetadata(path, type != ItemType::Root ? GameAdapter::RawDisplayName(ancestor.System(), path) : path.FilenameWithoutExtension(), type)
{
}

FileData::FileData(const Path& path, RootFolderData& ancestor) : FileData(ItemType::Game, path, ancestor)
{
}

bool FileData::HasP2K() const
{
  // Check game file
  Path p2k(P2KPath());
  if (p2k.Exists()) return true;

  // Check folder file until reaching the root
  for(p2k = p2k.Directory(); !p2k.IsEmpty(); p2k = p2k.Directory())
    if ((p2k / ".p2k.cfg").Exists())
      return true;

  return false;
}

SystemData& FileData::System() const
{
  return mTopAncestor.RootSystem();
}

FileData& FileData::CalculateHash()
{
  if (mMetadata.RomCrc32() != 0) return *this;
  Path path(mMetadata.Rom());

  if (mType != ItemType::Game) return *this;
  if (path.Size() > (20 << 20)) return *this; // Ignore file larger than 20Mb

  bool done = false;
  if (path.Extension().LowerCase() == ".zip")
  {
    Zip zip(path);
    if (zip.Count() == 1)
    {
      mMetadata.SetRomCrc32(zip.Crc32(0));
      done = true;
    }
  }

  if (!done)
  {
    // Hash file
    unsigned int result = 0;
    if (Crc32File(path).Crc32(result)) mMetadata.SetRomCrc32((int) result);
  }

  return *this;
}

/*String FileData::Regions()
{
  CheckLock();
  String fileName = mMetadata.RomFileOnly().ToString();
  Regions::RegionPack regions = Regions::ExtractRegionsFromNoIntroName(fileName);
  if (!regions.HasRegion()) regions = Regions::ExtractRegionsFromTosecName(fileName);
  if (!regions.HasRegion()) regions = Regions::ExtractRegionsFromName(Name());
  if (!regions.HasRegion()) regions = Meta().Region();

  return Regions::Serialize4Regions(regions.Sort());
}*/

bool FileData::IsDisplayable(TopLevelFilter topfilter) const
{
  // A folder is not displayable if there is no game inside
  if (IsFolder()) return false;

  if ((topfilter & TopLevelFilter::Favorites        ) != 0 && !mMetadata.Favorite()                           ) return false;
  if ((topfilter & TopLevelFilter::Hidden           ) != 0 && mMetadata.Hidden()                              ) return false;
  if ((topfilter & TopLevelFilter::Adult            ) != 0 && mMetadata.Adult()                               ) return false;
  if ((topfilter & TopLevelFilter::Preinstalled     ) != 0 && TopAncestor().PreInstalled()                    ) return false;
  if ((topfilter & TopLevelFilter::Yoko             ) != 0 && ! RotationUtils::IsTate(mMetadata.Rotation())   ) return false;
  if ((topfilter & TopLevelFilter::LatestVersion    ) != 0 && !mMetadata.LatestVersion()                      ) return false;
  if ((topfilter & TopLevelFilter::NotAGame         ) != 0 && mMetadata.NoGame()                              ) return false;
  if ((topfilter & TopLevelFilter::Board            ) != 0 && mMetadata.GenreId() == GameGenres::Board        ) return false;
  if ((topfilter & TopLevelFilter::OneAndTwoPlayers ) != 0 && mMetadata.PlayerMax() <= 2                      ) return false;
  if ((topfilter & TopLevelFilter::Tate             ) != 0 && RotationUtils::IsTate(mMetadata.Rotation())     ) return false;

  return true;
}

FileData::TopLevelFilter FileData::BuildTopLevelFilter()
{
  RecalboxConf& conf = RecalboxConf::Instance();
  TopLevelFilter result = TopLevelFilter::None;

  if (conf.GetFavoritesOnly()         ) result |= TopLevelFilter::Favorites;
  if (!conf.GetShowHidden()           ) result |= TopLevelFilter::Hidden;
  if (conf.GetFilterAdultGames()      ) result |= TopLevelFilter::Adult;
  if (conf.GetGlobalHidePreinstalled()) result |= TopLevelFilter::Preinstalled;
  if (conf.GetShowOnlyTateGames()     ) result |= TopLevelFilter::Yoko;
  if (conf.GetShowOnlyLatestVersion() ) result |= TopLevelFilter::LatestVersion;
  if (conf.GetHideNoGames()           ) result |= TopLevelFilter::NotAGame;
  if (conf.GetHideBoardGames()        ) result |= TopLevelFilter::Board;
  if (conf.GetShowOnly3PlusPlayers()  ) result |= TopLevelFilter::OneAndTwoPlayers;
  if (conf.GetShowOnlyYokoGames()     ) result |= TopLevelFilter::Tate;

  return result;
}

