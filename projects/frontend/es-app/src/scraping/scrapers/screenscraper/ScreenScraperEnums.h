//
// Created by gugue_U on 18/02/2021.
//

#pragma once

#include <utils/String.h>

class ScreenScraperEnums
{
  public:
    enum class ScreenScraperRegionPriority
    {
      DetectedRegion,
      FavoriteRegion,
    };

    enum class ScreenScraperImageType
    {
      ScreenshotIngame, //!< In-game screenshot
      ScreenshotTitle,  //!< Title screenshot
      MixV1,            //!< Server-side mix v1
      MixV2,            //!< Server-side mix v2
    };

    enum class ScreenScraperBoxType
    {
      Box2d,            //!< Box 2d (front)
      Box3d,            //!< Box 3d (front)
    };

    enum class ScreenScraperLogoType
    {
      Clear,            //!< Clear logo
      Carbon,           //!< Carbon background & frame
      Steel,            //!< Steel background & frame
      ShortMarquee,     //!< Short (2:1 ratio)
      LongMarquee,      //!< long (4:1 ratio)
      SteamGrid,       //!< Steam grid (composite logo)
    };

    enum class ScreenScraperVideoType
    {
      None,      //!< No video
      Raw,       //!< Unprocessed video
      Optimized, //!< Optimized video
    };

    static ScreenScraperRegionPriority ScreenScraperRegionPriorityFromString(const String& scraperRegionOptions);
    static const String& ScreenScraperRegionPriorityFromEnum(ScreenScraperRegionPriority scraperRegionOptions);

    static ScreenScraperImageType ScreenScraperImageTypeFromString(const String& imageType);
    static const String& ScreenScraperImageTypeFromEnum(ScreenScraperImageType imageType);

    static ScreenScraperBoxType ScreenScraperBoxTypeFromString(const String& boxType);
    static const String& ScreenScraperBoxTypeFromEnum(ScreenScraperBoxType boxType);

    static ScreenScraperVideoType ScreenScraperVideoTypeFromString(const String& videoType);
    static const String& ScreenScraperVideoTypeFromEnum(ScreenScraperVideoType videoType);

    static ScreenScraperLogoType ScreenScraperLogoTypeFromString(const String& logoType);
    static const String& ScreenScraperLogoTypeFromEnum(ScreenScraperLogoType logoType);
};