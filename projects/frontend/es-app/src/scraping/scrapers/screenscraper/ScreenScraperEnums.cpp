//
// Created by gugue_u on 18/02/2021.
//

#include <scraping/ScraperTools.h>
#include "ScreenScraperEnums.h"

ScreenScraperEnums::ScreenScraperRegionPriority ScreenScraperEnums::ScreenScraperRegionPriorityFromString(const String& regionPriority)
{
  if (regionPriority == "favoriteRegion") return ScreenScraperEnums::ScreenScraperRegionPriority::FavoriteRegion;
  return ScreenScraperEnums::ScreenScraperRegionPriority::DetectedRegion;
}

const String& ScreenScraperEnums::ScreenScraperRegionPriorityFromEnum(ScreenScraperRegionPriority regionPriority)
{
  switch (regionPriority)
  {
    case ScreenScraperEnums::ScreenScraperRegionPriority::FavoriteRegion:
    {
      static String sScraper = "favoriteRegion";
      return sScraper;
    }
    case ScreenScraperEnums::ScreenScraperRegionPriority::DetectedRegion:
    {
      break;
    }
  }
  static String sScraper = "DetectedRegion";
  return sScraper;
}

ScreenScraperEnums::ScreenScraperBoxType ScreenScraperEnums::ScreenScraperBoxTypeFromString(const String& boxType)
{
  if (boxType == "box3d") return ScreenScraperBoxType::Box3d;
  return ScreenScraperBoxType::Box2d;
}

const String& ScreenScraperEnums::ScreenScraperBoxTypeFromEnum(ScreenScraperBoxType boxType)
{
  switch (boxType)
  {
    case ScreenScraperBoxType::Box3d: { static String sBoxType = "box3d"; return sBoxType; }
    case ScreenScraperBoxType::Box2d:
    default: break;
  }
  static String sBoxType = "box2d";
  return sBoxType;
}

ScreenScraperEnums::ScreenScraperImageType ScreenScraperEnums::ScreenScraperImageTypeFromString(const String& imageType)
{
  if (imageType == "InGameScreenShot") return ScreenScraperImageType::ScreenshotIngame;
  if (imageType == "TitleScreenshot") return ScreenScraperImageType::ScreenshotTitle;
  if (imageType == "MixV1") return ScreenScraperImageType::MixV1;
  if (imageType == "MixV2") return ScreenScraperImageType::MixV2;
  return ScreenScraperImageType::ScreenshotIngame;
}

const String& ScreenScraperEnums::ScreenScraperImageTypeFromEnum(ScreenScraperImageType imageType)
{
  switch (imageType)
  {
    case ScreenScraperImageType::ScreenshotIngame: break;
    case ScreenScraperImageType::ScreenshotTitle: { static String sImageType = "TitleScreenshot"; return sImageType; }
    case ScreenScraperImageType::MixV1: { static String sImageType = "MixV1"; return sImageType; }
    case ScreenScraperImageType::MixV2: { static String sImageType = "MixV2"; return sImageType; }
  }
  static String sImageType = "InGameScreenShot";
  return sImageType;
}

ScreenScraperEnums::ScreenScraperVideoType ScreenScraperEnums::ScreenScraperVideoTypeFromString(const String& videoType)
{
  if (videoType == "NormalizedVideo") return ScreenScraperVideoType::Optimized;
  if (videoType == "OriginalVideo") return ScreenScraperVideoType::Raw;
  return ScreenScraperVideoType::None;
}

const String& ScreenScraperEnums::ScreenScraperVideoTypeFromEnum(ScreenScraperVideoType videoType)
{
  switch (videoType)
  {
    case ScreenScraperVideoType::None: { static String sScraper = "None"; return sScraper; }
    case ScreenScraperVideoType::Raw: { static String sFileName = "OriginalVideo"; return sFileName; }
    case ScreenScraperVideoType::Optimized: { break; }
  }
  static String sFileName = "NormalizedVideo";
  return sFileName;
}

ScreenScraperEnums::ScreenScraperLogoType ScreenScraperEnums::ScreenScraperLogoTypeFromString(const String& logoType)
{
  if (logoType == "carbon") return ScreenScraperLogoType::Carbon;
  if (logoType == "steel") return ScreenScraperLogoType::Steel;
  if (logoType == "short") return ScreenScraperLogoType::ShortMarquee;
  if (logoType == "long") return ScreenScraperLogoType::LongMarquee;
  if (logoType == "steamgrid") return ScreenScraperLogoType::SteamGrid;
  return ScreenScraperLogoType::Clear;
}

const String& ScreenScraperEnums::ScreenScraperLogoTypeFromEnum(ScreenScraperEnums::ScreenScraperLogoType logoType)
{
  switch (logoType)
  {
    case ScreenScraperLogoType::Clear: break;
    case ScreenScraperLogoType::Carbon: { static String sLogoName = "carbon"; return sLogoName; }
    case ScreenScraperLogoType::Steel: { static String sLogoName = "steel"; return sLogoName; }
    case ScreenScraperLogoType::ShortMarquee: { static String sLogoName = "short"; return sLogoName; }
    case ScreenScraperLogoType::LongMarquee: { static String sLogoName = "long"; return sLogoName; }
    case ScreenScraperLogoType::SteamGrid: { static String sLogoName = "steamgrid"; return sLogoName; }
  }
  static String sLogoName = "clear";
  return sLogoName;
}
