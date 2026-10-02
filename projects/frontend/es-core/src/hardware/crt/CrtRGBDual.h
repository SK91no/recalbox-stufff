//
// Created by bkg2k on 19/12/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <hardware/crt/ICrtInterface.h>
#include <utils/Files.h>
#include "CrtConf.h"

class CrtRGBDual : public ICrtInterface
{
  public:
    //! Constructor
    explicit CrtRGBDual(bool automaticallyDetected, BoardType boardType) : ICrtInterface(automaticallyDetected, boardType) {}

    //! An RGB Dual is attached
    bool IsCrtAdapterAttached() const override
    {
      #ifdef OPTION_RECALBOX_SIMULATE_RRGBD
      return true;
      #else
      return Files::LoadFile(Path(vgaCard0Connected)) == "connected\n"
             || Files::LoadFile(Path(vgaCard1Connected)) == "connected\n"
             || Files::LoadFile(Path(vgaCard2Connected)) == "connected\n";
      #endif
    }

    //! This adapter is an RGB Dual
    [[nodiscard]] CrtAdapterType GetCrtAdapter() const override { return CrtAdapterType::RGBDual; }

    //! RGB Dual has support for 31khz
    [[nodiscard]] bool Has31KhzSupport() const override { return true; }

    //! RGB Dual has support for 120hz modes
    [[nodiscard]] bool Has120HzSupport() const override { return GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHz31; }

    //! Return select output frequency
    [[nodiscard]] HorizontalFrequency GetHorizontalFrequency() const override {
      HorizontalFrequency configFreq = static_cast<HorizontalFrequency>(CrtConf::Instance().GetSystemCRTScreenType());
      if(configFreq != ICrtInterface::HorizontalFrequency::Auto)
        return configFreq;
      return GetRGBDual31khzSwitchState() ? HorizontalFrequency::KHz31 : HorizontalFrequency::KHz15;
    }

    //! This adapter has support of forced 50hz
    [[nodiscard]] bool HasForced50hzSupport() const override { return true; }

    //! Get 50hz switch state
    [[nodiscard]] bool MustForce50Hz() const override { return GetRGBDual50hzSwitchState(); }

    //! The comment is here to tell you that the name will be returned bby this methode named Name()
    [[nodiscard]] std::string& Name() const override { static std::string adapterString("Recalbox RGB Dual"); return adapterString; }

    [[nodiscard]] std::string& ShortName() const override { static std::string adapterShortString("recalboxrgbdual"); return adapterShortString; }
  private:
    static constexpr const char* sRGBDual31khzSwitch = "/sys/devices/platform/recalboxrgbdual/dipswitch-31khz/value";
    static constexpr const char* sRGBDual50hzSwitch = "/sys/devices/platform/recalboxrgbdual/dipswitch-50hz/value";
    static constexpr const char* vgaCard0Connected = "/sys/class/drm/card0-VGA-1/status";
    static constexpr const char* vgaCard1Connected = "/sys/class/drm/card1-VGA-1/status";
    static constexpr const char* vgaCard2Connected = "/sys/class/drm/card2-VGA-1/status";

    //! Get 31khz switch state on RGB dual board
    static bool GetRGBDual31khzSwitchState() { return Files::LoadFile(Path(sRGBDual31khzSwitch)) == "0\n"; }
    //! Get 50hz switch state on RGB dual board
    static bool GetRGBDual50hzSwitchState() { return Files::LoadFile(Path(sRGBDual50hzSwitch)) == "0\n"; }
};



