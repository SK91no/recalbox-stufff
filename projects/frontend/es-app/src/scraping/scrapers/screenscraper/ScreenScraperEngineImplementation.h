//
// Created by bkg2k on 23/03/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <scraping/scrapers/screenscraper/ScreenScraperEngineBase.h>
#include "ScreenScraperEndPoints.h"

class ScreenScraperEngineImplementation : public ScreenScraperEngineBase
{
  public:
    explicit ScreenScraperEngineImplementation(IScraperEngineFreezer* freezer)
      : ScreenScraperEngineBase(Endpoint(), freezer)
      , mLanguage(Languages::Unknown)
      , mRegion(Regions::GameRegions::Unknown)
      , mMainImage(ScreenScraperEnums::ScreenScraperImageType::MixV1)
      , mBox(ScreenScraperEnums::ScreenScraperBoxType::Box2d)
      , mVideo(ScreenScraperEnums::ScreenScraperVideoType::None)
      , mLogo(ScreenScraperEnums::ScreenScraperLogoType::Clear)
      , mWantManual(false)
      , mWantMaps(false)
      , mWantP2K(false)
    {
    }

  private:
    //! Screenscraper credentials: Login
    String mLogin;
    //! Screenscraper credentials: Password
    String mPassword;
    //! Favorite language
    Languages mLanguage;
    //! Favorite region
    Regions::GameRegions mRegion;
    //! Main image
    ScreenScraperEnums::ScreenScraperImageType mMainImage;
    //! Thumbnail image
    ScreenScraperEnums::ScreenScraperBoxType mBox;
    //! Video
    ScreenScraperEnums::ScreenScraperVideoType mVideo;
    //! Wheel
    ScreenScraperEnums::ScreenScraperLogoType mLogo;
    //! Manual
    bool mWantManual;
    //! Maps
    bool mWantMaps;
    //! Tips & Trickx
    bool mWantTips;
    //! Pad 2 keyboard
    bool mWantP2K;

    //! EndPoint providers
    static ScreenScraperEndPoints& Endpoint()
    {
      static ScreenScraperEndPoints sEndPoints;
      return sEndPoints;
    }

    /*
     * ScreenScraperApis::IConfiguration implementation
     */

    //! Reinitialize configuration
    void ResetConfiguration() override
    {
      RecalboxConf& conf = RecalboxConf::Instance();
      // Credentials
      mLogin          = conf.GetScreenScraperLogin().Trim();
      mPassword       = conf.GetScreenScraperPassword().Trim();

      // Language & region
      mRegion         = conf.GetScreenScraperRegion();
      mLanguage       = LanguagesTools::GetScrapingLanguage();

      // Medias
      mMainImage      = conf.GetScreenScraperMainMedia();
      mBox            = conf.GetScreenScraperBox();
      mVideo          = conf.GetScreenScraperVideo();
      mLogo           = conf.GetScreenScraperLogo();
      mWantManual     = conf.GetScreenScraperWantManual();
      mWantMaps       = conf.GetScreenScraperWantMaps();
      mWantTips       = conf.GetScreenScraperWantTips();
      mWantP2K        = conf.GetScreenScraperWantP2K();
    }

    //! Get screenscraper login
    [[nodiscard]] String GetLogin() const override { return mLogin; }

    //! Get screenscraper password
    [[nodiscard]] String GetPassword() const override { return mPassword; }

    //! TODO: Change that we should create a bearer vs basic auth interface
    [[nodiscard]] String GetBearer() const override { return String::Empty; }

    //! Get favorite language
    [[nodiscard]] Languages GetFavoriteLanguage() const override { return mLanguage; };

    //! Get favorite region
    [[nodiscard]] Regions::GameRegions GetFavoriteRegion() const override { return mRegion; }

    //! Get main image type
    [[nodiscard]] ScreenScraperEnums::ScreenScraperImageType GetImageType() const override { return mMainImage; }

    //! Get box image typ
    [[nodiscard]] ScreenScraperEnums::ScreenScraperBoxType GetBoxType() const override { return mBox; }

    //! Check if video are required
    [[nodiscard]] ScreenScraperEnums::ScreenScraperVideoType GetVideo() const override { return mVideo; }

    //! Check if logo are required
    [[nodiscard]] ScreenScraperEnums::ScreenScraperLogoType GetLogo() const override { return mLogo; }

    //! Check if manual are required
    [[nodiscard]] bool GetWantManual() const override { return mWantManual; }

    //! Check if maps are required
    [[nodiscard]] bool GetWantMaps() const override { return mWantMaps; }

    //! Check if tips & tricks are required
    [[nodiscard]] bool GetWantTips() const override { return mWantTips; }

    //! Check if p2k are required
    [[nodiscard]] bool GetWantP2K() const override { return mWantP2K; }
};



