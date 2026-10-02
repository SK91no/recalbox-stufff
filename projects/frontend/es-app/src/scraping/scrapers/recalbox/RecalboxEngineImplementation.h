//
// Created by bkg2k on 23/03/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include "RecalboxEndPoints.h"
#include <scraping/scrapers/screenscraper/ScreenScraperEngineBase.h>

class RecalboxEngineImplementation : public ScreenScraperEngineBase
{
  public:
    explicit RecalboxEngineImplementation(IScraperEngineFreezer* freezer)
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
    //! Recalbox credentials: token
    String mBearer;
    //! Favorite language
    Languages mLanguage;
    //! Favorite region
    Regions::GameRegions mRegion;
    //! Main image
    ScreenScraperEnums::ScreenScraperImageType mMainImage;
    //! Box image
    ScreenScraperEnums::ScreenScraperBoxType mBox;
    //! Video
    ScreenScraperEnums::ScreenScraperVideoType mVideo;
    //! Wheel
    ScreenScraperEnums::ScreenScraperLogoType mLogo;
    //! Manual
    bool mWantManual;
    //! Maps
    bool mWantMaps;
    //! Tips
    bool mWantTips;
    //! Pad 2 keyboard
    bool mWantP2K;

    //! EndPoint providers
    static RecalboxEndPoints& Endpoint()
    {
      static RecalboxEndPoints sEndPoints;
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
      mBearer          = conf.GetRecalboxPrivateKey().Trim();

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

    //! Never used
    [[nodiscard]] String GetLogin() const override { return String::Empty; }

    //! Never used
    [[nodiscard]] String GetPassword() const override { return String::Empty; }

    //! Get recalbox bearer
    [[nodiscard]] String GetBearer() const override { return mBearer; }

    //! Get favorite language
    [[nodiscard]] Languages GetFavoriteLanguage() const override { return mLanguage; };

    //! Get favorite region
    [[nodiscard]] Regions::GameRegions GetFavoriteRegion() const override { return mRegion; }

    //! Get main image type
    [[nodiscard]] ScreenScraperEnums::ScreenScraperImageType GetImageType() const override { return mMainImage; }

    //! Get thumbnail image typ
    [[nodiscard]] ScreenScraperEnums::ScreenScraperBoxType GetBoxType() const override { return mBox; }

    //! Check if video are required
    [[nodiscard]] ScreenScraperEnums::ScreenScraperVideoType GetVideo() const override { return mVideo; }

    //! Check if logo are required
    [[nodiscard]] ScreenScraperEnums::ScreenScraperLogoType GetLogo() const override { return mLogo; }

    //! Check if manual are required
    [[nodiscard]] bool GetWantManual() const override { return mWantManual; }

    //! Check if maps are required
    [[nodiscard]] bool GetWantMaps() const override { return mWantMaps; }

    //! Check if tips&tricks are required
    [[nodiscard]] bool GetWantTips() const override { return mWantTips; }

    //! Check if p2k are required
    [[nodiscard]] bool GetWantP2K() const override { return mWantP2K; }
};



