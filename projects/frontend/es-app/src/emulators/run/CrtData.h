//
// Created by bkg2k on 26/12/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <hardware/Board.h>
#include <CrtConf.h>
#include "../EmulatorData.h"
#include "RotationManager.h"

class CrtData
{
  public:
    //! Video system
    enum class CrtRegion
    {
      AUTO, //!< Automatic selection
      JP,  //!< Forced Japan
      US, //!< Forced US
      EU, //!< Forced Europe
    };

    enum class CrtVideoStandard
    {
        AUTO, //!< Automatic selection
        PAL,  //!< Forced Pal
        NTSC, //!< Forced Ntsc
    };

    enum class CrtMode
    {
      Auto, //!< Automatic selection for the screen. Will take in account the screen capabilities
      Force240, //!< Force 240p (on 15khz screen -> avoid interlaced, on Multisync force 240p)
      Force480i, //<! Force 480i mode
      Force480, //<! Force 480 mode on 31kHz screens, and let emulator choose 480i when supported
      DoubleFreq, //<! Force 240p@120Hz on 31Khz screens
      Force384p
    };

    static std::string CrtModeToString(CrtMode mode) {
      switch (mode)
      {
        case CrtMode::Force240: return "240";
        case CrtMode::Force480: return "480";
        case CrtMode::Force480i: return "480i";
        case CrtMode::DoubleFreq: return "doublefreq";
        case CrtMode::Force384p: return "384p";
        case CrtMode::Auto:
        default: return "auto";
      }
    }

    enum class CrtSignal
    {
      RGB,
      Composite,
    };

    static std::string CrtSignalToString(CrtSignal mode) {
      switch (mode)
      {
        case CrtSignal::RGB: return "RGB";
        case CrtSignal::Composite: return "Composite";
        default: return "RGB";
      }
    }

    //! Default constructor
    CrtData()
      : mCrt(&Board::Instance().CrtBoard())
      , mConf(&CrtConf::Instance())
      , mRegionOrVideoStandardConfigured(false)
      , mForceResolutionConfigured(false)
      , mForceSignalConfigured(false)
      , mVideoStandard(CrtVideoStandard::AUTO)
      , mRegion(CrtRegion::AUTO)
      , mCrtMode(CrtMode::Auto)
      , mCrtSignal(CrtSignal::RGB)
    {
    }


    /*!
     * @brief Check if there is a CRT board and the user requested to choose individual NTSC options
     * @return True if the class needs to be configured, false otherwise
     */
    [[nodiscard]] bool IsRegionOrStandardConfigured() const
    {
      if (!mRegionOrVideoStandardConfigured)
        if (mCrt->IsCrtAdapterAttached())
          if (mConf->GetSystemCRTGameRegionSelect())
            return true;
      return false;
    }

    /*!
     * @brief Configure crt data
     * @param ntsc True for NTSC, false for PAL
     */
    void ConfigureVideoStandard(CrtVideoStandard standard)
    {
        mVideoStandard = standard;
        mRegionOrVideoStandardConfigured = true;
    }

    void ConfigureRegion(CrtRegion region)
    {
        mRegion = region;
        mRegionOrVideoStandardConfigured = true;
    }

    /*!
     * @brief Check if the target system requires choosing between PAL or NTSC
     * @param system target system
     * @return True if the choice is required, false otherwise
     */
    [[nodiscard]] bool MustChoosePALorNTSC(const SystemData& system) const
    {
      return system.Descriptor().CrtMultiRegion() &&        // System must support multi-region
             mCrt->IsCrtAdapterAttached() &&
             !mCrt->MustForce50Hz() && // & hardware must not force 50hz
             mCrt->GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHz15; // & and we are 15khz
    }

    /*!
     * @brief Check if there is a CRT board and the user requested to choose individual 480 or 240 options
     * @return True if the class needs to be configured, false otherwise
     */
    [[nodiscard]] bool IsForceResolutionSelectionConfigured() const
    {
      if (!mForceResolutionConfigured)
        if (mCrt->IsCrtAdapterAttached())
          if (mConf->GetSystemCRTGameResolutionSelect())
            return true;
      return false;
    }


    /*!
     * @brief Configure crt data
     * @param highRez True for 480, false for 240
     */
    void ConfigureForceResolution(CrtMode mode)
    {
      if (!mForceResolutionConfigured)
      {
        mCrtMode = mode;
        mForceResolutionConfigured = true;
      }
    }

    /*!
     * @brief Check if there is a CRT board and the user requested to choose RGB/Composite selection
     * @return True if the class needs to be configured, false otherwise
     */
    [[nodiscard]] bool IsForceSignalSelectionConfigured() const
    {
      if (!mForceSignalConfigured)
        if (mCrt->IsCrtAdapterAttached())
          if (mConf->GetSystemCRTSignalSelect())
            return true;
      return false;
    }

    /*!
     * @brief Configure crt signal
     */
    void ConfigureForceSignal(CrtSignal signal)
    {
      if (!mForceSignalConfigured)
      {
        mCrtSignal = signal;
        mForceSignalConfigured = true;
      }
    }

    /*!
     * @brief Check if the target system requires choosing between RGB and Composite
     * @param game target game
     * @return True if the choice is required, false otherwise
     */
    [[nodiscard]] bool MustChooseCRTSignal() const
    {
      return Board::Instance().CrtBoard().HasCompositeSupport() && ! Board::Instance().CrtBoard().MustForceComposite();
    }

    /*!
     * @brief check if the game is HD
     * @param game
     * @param emulator
     * @return
     */
    bool GameIsHD(const FileData& game, const EmulatorData& emulator) const
    {
      bool gameIsHd = game.System().Descriptor().CrtHighResolution();
      if(game.System().IsArcade())
      {
        String emu = emulator.Emulator();
        String core =  emulator.Core();
        const ArcadeDatabase* database = game.System().ArcadeDatabases().LookupDatabase(game, emu, core);
        if (database != nullptr){
          const ArcadeGame* arcade = database->LookupGame(game);
          if(arcade != nullptr)
          {
            if (RotationUtils::IsTate(arcade->ScreenRotation()))
              gameIsHd |= arcade->Width() >= 384;
            else
              gameIsHd |= arcade->Height() >= 384;
          }
        }
      }
      return gameIsHd;
    }

    /*!
     * @brief Check if the target system requires choosing between 240 or 480
     * @param game target game
     * @return True if the choice is required, false otherwise
     */
    [[nodiscard]] bool MustChooseResolution(FileData* game, const EmulatorData& emulator) const
    {
      bool gameIsHd = GameIsHD(*game, emulator);
      if(RotationManager::IsTateOnYokoOrYokoOnTate(*game)) return false;
      // If 15Khz, the system must support high rez and
      //    (composite must have been selected
      //    or interlaced must be supported by board)
      //    and not interlaced disabled
      // If 31khz, the board must support 120Hz
      // If multisync1524, return true if the system supports hd and the 31 + 15 is supported
      // If multisync1531 or trifreq, return true
      ICrtInterface::HorizontalFrequency freq = Board::Instance().CrtBoard().GetHorizontalFrequency();
      return (gameIsHd && freq == ICrtInterface::HorizontalFrequency::KHz15 && (
              (Board::Instance().CrtBoard().HasInterlacedSupport() || mCrtSignal == CrtSignal::Composite) && !CrtConf::Instance().GetSystemCRTAvoidInterlaced()))
          || (freq == ICrtInterface::HorizontalFrequency::KHz31 && Board::Instance().CrtBoard().Has120HzSupport())
          || (gameIsHd && freq == ICrtInterface::HorizontalFrequency::KHzMulti1525)
          || freq >= ICrtInterface::HorizontalFrequency::KHzMulti1531;
    }

    /*
     * Accessors
     */

    [[nodiscard]] CrtMode GetCrtMode(const FileData& game, const EmulatorData& emulator) const {
      // Force 240p for HD games only when the screen can actually do 240p:
      //  - any frequency that natively scans 15kHz (KHz15, Multi15/25, Multi15/31, TriFreq), OR
      //  - pure 31kHz adapters that support 120Hz (240p @ 120Hz = same line rate as 480p @ 60Hz).
      // On a pure 31kHz adapter without 120Hz support (JVS, Jamma 1), 240p is not
      // achievable: fall through to mCrtMode and let switchres pick a real 480p mode.
      auto freq = Board::Instance().CrtBoard().GetHorizontalFrequency();
      bool can240p = (freq != ICrtInterface::HorizontalFrequency::KHz31)
                  || Board::Instance().CrtBoard().Has120HzSupport();
      if(GameIsHD(game, emulator) && !mForceResolutionConfigured && !Board::Instance().CrtBoard().HasInterlacedSupport() && can240p){
        // Not configured and no interlaced and game is hd, we force 240p
        return CrtMode::Force240;
      }
      // If game is not HD and we are on multi freq, we must limit to 15kHz to avoid dynamic modes to switch to 480p in game started in 240p
      if (!GameIsHD(game, emulator) && mCrtMode == CrtMode::Auto &&
        (freq >= ICrtInterface::HorizontalFrequency::KHzMulti1531 || freq == ICrtInterface::HorizontalFrequency::KHzMulti1525))
        return CrtMode::Force480i; // 480i force 15khz only
      return mCrtMode;
    }

    [[nodiscard]] CrtSignal GetCrtSignal() const { return mCrtSignal; }
    [[nodiscard]] CrtScanlines Scanlines(const SystemData& system) const
    {
      return ((mCrtMode == CrtMode::Auto || mCrtMode == CrtMode::Force480) && !system.Descriptor().CrtHighResolution() &&
                    (Board::Instance().CrtBoard().GetHorizontalFrequency() >= ICrtInterface::HorizontalFrequency::KHz31)) ?
                    CrtConf::Instance().GetSystemCRTScanlines31kHz() : CrtScanlines::None;
    }
    [[nodiscard]] CrtVideoStandard VideoStandard() const { return mVideoStandard; }
    [[nodiscard]] CrtRegion Region() const { return mRegion; }

  private:
    //! ICrtInterface reference
    ICrtInterface* mCrt;
    //! Configuration
    CrtConf* mConf;
    //! NTSC configured
    bool mRegionOrVideoStandardConfigured;
    //! forced resolution configured
    bool mForceResolutionConfigured;
    //! forced signal configured
    bool mForceSignalConfigured;
    //! Video system (default: auto
    CrtVideoStandard mVideoStandard;
    CrtRegion mRegion;
    //! 480? (default: 240p)
    CrtMode mCrtMode;
    CrtSignal mCrtSignal;
};