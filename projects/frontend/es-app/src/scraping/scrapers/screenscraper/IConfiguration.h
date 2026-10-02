//
// Created by bkg2k on 17/03/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <scraping/scrapers/screenscraper/ScreenScraperEnums.h>
#include <scraping/scrapers/screenscraper/Languages.h>
#include <games/classifications/Regions.h>

/*!
 * @brief Interface to retrieve credentials
 */
class IConfiguration
{
  public:
    //!Image type
    enum class Image
    {
      None,             //!< No image
      ScreenshotIngame, //!< In-game screenshot
      ScreenshotTitle,  //!< Title screenshot
      Wheel,            //!< Wheel
      Marquee,          //!< Marquee
      Box2d,            //!< Box 2d (front)
      Box3d,            //!< Box 3d (front)
      MixV1,            //!< Server-side mix v1
      MixV2,            //!< Server-side mix v2
    };
    //! Video type
    enum class Video
    {
      None,      //!< No video
      Raw,       //!< Unprocessed video
      Optimized, //!< Optimized video
    };

    //! Default destructor
    virtual ~IConfiguration() = default;

    //! Reinitialize configuration
    virtual void ResetConfiguration() = 0;

    //! Get Token
    [[nodiscard]] virtual String GetBearer() const = 0;
    //! Get Login
    [[nodiscard]] virtual String GetLogin() const = 0;
    //! Get password
    [[nodiscard]] virtual String GetPassword() const = 0;
    //! Get favorite language
    [[nodiscard]] virtual Languages GetFavoriteLanguage() const = 0;
    //! Get favorite region
    [[nodiscard]] virtual Regions::GameRegions GetFavoriteRegion() const = 0;
    //! Get main image type
    [[nodiscard]] virtual ScreenScraperEnums::ScreenScraperImageType GetImageType() const = 0;
    //! Get thumbnail image type
    [[nodiscard]] virtual ScreenScraperEnums::ScreenScraperBoxType GetBoxType() const = 0;
    //! GetVideo type
    [[nodiscard]] virtual ScreenScraperEnums::ScreenScraperVideoType GetVideo() const = 0;
    //! Check if wheel are required
    [[nodiscard]] virtual ScreenScraperEnums::ScreenScraperLogoType GetLogo() const = 0;
    //! Check if marquee are required
    [[nodiscard]] virtual bool GetWantManual() const = 0;
    //! Check if wheel are required
    [[nodiscard]] virtual bool GetWantMaps() const = 0;
    //! Check if wheel are required
    [[nodiscard]] virtual bool GetWantTips() const = 0;
    //! Check if p2k are required
    [[nodiscard]] virtual bool GetWantP2K() const = 0;
};
