//
// Created by bkg2k on 07/03/24.
//

#include "PictogramCaches.h"
#include <rendering/textures/TextureManager.h>

#include "container/MetadataIcons.h"
#include "themes/ThemeManager.h"

Texture PictogramCaches::GetFlag(Regions::GameRegions region, int height)
{
  Mutex::AutoLock lock(mLocker);
  HashMap<Regions::GameRegions, Texture>* perHeight = mRegionToTextures.try_get(height);
  if (perHeight == nullptr)
  {
    mRegionToTextures[height].clear(); // Create the HashMap instance
    perHeight = mRegionToTextures.try_get(height);
  }
  Texture* flag = perHeight->try_get(region);
  if (flag == nullptr)
  {
    // Load flag - use height as discriminent
    Texture texture = TextureManager::Instance().Create(Path(":/regions/" + Regions::GameRegionsFromEnum(region) + ".svg"), 0, height, TextureHolder::Properties::NoCache);
    perHeight->insert(region, texture);
    flag = perHeight->try_get(region);
  }
  return *flag;
}

Texture PictogramCaches::GetGenre(GameGenres genre, int height)
{
  Mutex::AutoLock lock(mLocker);
  HashMap<GameGenres, Texture>* perHeight = mGenreToTextures.try_get(height);
  if (perHeight == nullptr)
  {
    mGenreToTextures[height].clear(); // Create the HashMap instance
    perHeight = mGenreToTextures.try_get(height);
  }
  Texture* ptexture = perHeight->try_get(genre);
  if (ptexture == nullptr)
  {
    // Load genre
    Texture texture = TextureManager::Instance().Create(Path(String(Genres::GetResourcePath(genre).ToString()).Replace("/genre/", "/genreflags/")), 0, height, TextureHolder::Properties::NoCache);
    perHeight->insert(genre, texture);
    ptexture = perHeight->try_get(genre);
  }
  return *ptexture;
}

Texture PictogramCaches::GetPlayers(int playermin, int playermax, int height)
{
  // Some control first
  if (playermin == 0xFFFF) { playermin = playermax; playermax = 0; }
  playermin = Math::clampi(playermin, 1, 12);
  playermax = Math::clampi(playermax, 1, 13);
  if (playermax < playermin && playermax != 0) playermax = playermin;

  int playerMinMax = (playermin << 16) | playermax;
  HashMap<int, Texture>* perHeight = mPlayersToTextures.try_get(height);
  if (perHeight == nullptr)
  {
    mPlayersToTextures[height].clear(); // Create the HashMap instance
    perHeight = mPlayersToTextures.try_get(height);
  }
  Texture* pPlayers = perHeight->try_get(playerMinMax);
  if (pPlayers == nullptr)
  {
    String resourceName(":/players/");
    resourceName.Append(playermin);
    if (playermin != playermax)
    {
      if (playermax <= 12) resourceName.Append("to").Append(playermax);
      else resourceName.Append("plus");
    }
    resourceName.Append(".svg");
    // Load players
    Texture texture = TextureManager::Instance().Create(Path(resourceName), 0, height, TextureHolder::Properties::NoCache);
    perHeight->insert(playerMinMax, texture);
    pPlayers = perHeight->try_get(playerMinMax);
  }
  return *pPlayers;
}

Texture PictogramCaches::GetSupportLeftPart(int height)
{
  Texture* texture = mSupportLeft.try_get(height);
  if (texture == nullptr)
  {
    mSupportLeft[height] = TextureManager::Instance().Create(Path(":/supports/first.svg"), 0, height, TextureHolder::Properties::NoCache);
    texture = mSupportLeft.try_get(height);
  }
  return *texture;
}

Texture PictogramCaches::GetSupportRightPart(int height)
{
  Texture* texture = mSupportRight.try_get(height);
  if (texture == nullptr)
  {
    mSupportRight[height] = TextureManager::Instance().Create(Path(":/supports/last.svg"), 0, height, TextureHolder::Properties::NoCache);
    texture = mSupportRight.try_get(height);
  }
  return *texture;
}

Texture PictogramCaches::GetSupportSeparator(int height)
{
  Texture* texture = mSupportSeparator.try_get(height);
  if (texture == nullptr)
  {
    mSupportSeparator[height] = TextureManager::Instance().Create(Path(":/supports/separator.svg"), 0, height, TextureHolder::Properties::NoCache);
    texture = mSupportSeparator.try_get(height);
  }
  return *texture;
}

Texture PictogramCaches::GetSupportSide(SupportSides side, int height)
{
  HashMap<SupportSides, Texture>* perHeight = mSupportSideToTextures.try_get(height);
  if (perHeight == nullptr)
  {
    mSupportSideToTextures[height].clear(); // Create the HashMap instance
    perHeight = mSupportSideToTextures.try_get(height);
  }
  Texture* pSide = perHeight->try_get(side);
  if (pSide == nullptr)
  {
    String name;
    switch(side)
    {
      case SupportSides::None: break;
      case SupportSides::A: name = "A"; break;
      case SupportSides::B: name = "B"; break;
    }
    Texture texture = TextureManager::Instance().Create(Path(String(":/supports/").Append(name).Append(".svg")), 0, height, TextureHolder::Properties::NoCache);
    perHeight->insert(side, texture);
    pSide = perHeight->try_get(side);
  }
  return *pSide;
}

Texture PictogramCaches::GetSupportNumber(int number, int height)
{
  HashMap<int, Texture>* perHeight = mSupportNumberToTextures.try_get(height);
  if (perHeight == nullptr)
  {
    mSupportNumberToTextures[height].clear(); // Create the HashMap instance
    perHeight = mSupportNumberToTextures.try_get(height);
  }
  Texture* pNumber = perHeight->try_get(number);
  if (pNumber == nullptr)
  {
    Texture texture = TextureManager::Instance().Create(Path(String(":/supports/").Append(number).Append(".svg")), 0, height, TextureHolder::Properties::NoCache);
    perHeight->insert(number, texture);
    pNumber = perHeight->try_get(number);
  }
  return *pNumber;
}

Texture PictogramCaches::GetSupportType(SupportTypes type, int height)
{
  HashMap<SupportTypes, Texture>* perHeight = mSupportTypeToTextures.try_get(height);
  if (perHeight == nullptr)
  {
    mSupportTypeToTextures[height].clear(); // Create the HashMap instance
    perHeight = mSupportTypeToTextures.try_get(height);
  }
  Texture* pSide = perHeight->try_get(type);
  if (pSide == nullptr)
  {
    String name;
    switch(type)
    {
      case SupportTypes::Unknown: break;
      case SupportTypes::Cartridge: name = "cart"; break;
      case SupportTypes::CD: name = "cd"; break;
      case SupportTypes::HDD: name = "hdd"; break;
      case SupportTypes::FilesAndFolders: name = "file"; break;
      case SupportTypes::Tape: name = "k7"; break;
      case SupportTypes::QuickDisk: name = "qd"; break;
      case SupportTypes::Floppy3: name = "disk3"; break;
      case SupportTypes::Floppy35: name = "disk35"; break;
      case SupportTypes::Floppy525: name = "disk525"; break;
      case SupportTypes::Microdisc: name = "md"; break;
      case SupportTypes::PCB: name = "pcb"; break;
    }
    Texture texture = TextureManager::Instance().Create(Path(String(":/supports/types/").Append(name).Append(".svg")), 0, height, TextureHolder::Properties::NoCache);
    perHeight->insert(type, texture);
    pSide = perHeight->try_get(type);
  }
  return *pSide;
}

Texture PictogramCaches::GetMetadataIconType(MetadataIcons icon, int height)
{
  HashMap<MetadataIcons, Texture>* perHeight = mMetadataIconsToTextures.try_get(height);
  if (perHeight == nullptr)
  {
    mMetadataIconsToTextures[height].clear(); // Create the HashMap instance
    perHeight = mMetadataIconsToTextures.try_get(height);
  }
  Texture* pIcon = perHeight->try_get(icon);
  if (pIcon == nullptr)
  {
    String name;
    switch(icon)
    {
      case MetadataIcons::Favorite: name = "favorite"; break;
      case MetadataIcons::Hidden: name = "hidden"; break;
      case MetadataIcons::Adult: name = "adult"; break;
    }
    Texture texture = TextureManager::Instance().Create(Path(String(":/gameicons/").Append(name).Append(".svg")), 0, height, TextureHolder::Properties::NoCache);
    perHeight->insert(icon, texture);
    pIcon = perHeight->try_get(icon);
  }
  return *pIcon;
}

Texture PictogramCaches::GetArrow(Arrow arrow, int height)
{
  HashMap<Arrow, Texture>* perHeight = mArrowsToTextures.try_get(height);
  if (perHeight == nullptr)
  {
    mArrowsToTextures[height].clear(); // Create the HashMap instance
    perHeight = mArrowsToTextures.try_get(height);
  }
  Texture* pIcon = perHeight->try_get(arrow);
  if (pIcon == nullptr)
  {
    String name;
    switch(arrow)
    {
      case Arrow::Top: name = "top"; break;
      case Arrow::Left: name = "left"; break;
      case Arrow::Down: name = "down"; break;
      case Arrow::Right: name = "right"; break;
    }
    Texture texture = TextureManager::Instance().Create(Path(String(":/").Append(name).Append(".svg")), 0, height, TextureHolder::Properties::NoCache);
    perHeight->insert(arrow, texture);
    pIcon = perHeight->try_get(arrow);
  }
  return *pIcon;
}

Texture PictogramCaches::GetSystemLogo(SystemData& system, int height)
{
  Texture* texture = mSystemLogoToTexture.try_get(&system);
  if (texture == nullptr)
  {
    // Load genre
    Path logoPath;
    (void)ThemeManager::GetDefaultSystemLogo(system, logoPath);
    Texture newLogo = TextureManager::Instance().Create(logoPath, 0, height, TextureHolder::Properties::None);
    mSystemLogoToTexture.insert(&system, newLogo);
    texture = mSystemLogoToTexture.try_get(&system);
  }
  return *texture;
}
