//
// Created by digitalLumberjack on 25/11/2024.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <hardware/crt/ICrtInterface.h>
#include <utils/Files.h>
#include <filesystem>
#include "CrtConf.h"

class CrtRGBDual2 : public ICrtInterface
{
  public:
    //! Constructor
    explicit CrtRGBDual2(bool automaticallyDetected, BoardType boardType) : ICrtInterface(automaticallyDetected, boardType) {}

    //! An RGB Dual is attached
    bool IsCrtAdapterAttached() const override
    {
      #ifdef OPTION_RECALBOX_SIMULATE_RRGBD
      return true;
      #else
      return Files::LoadFile(Path(vgaCard0Connected)) == "connected\n"
             || Files::LoadFile(Path(vgaCard1Connected)) == "connected\n"
             || Files::LoadFile(Path(vgaCard2Connected)) == "connected\n"
             || Files::LoadFile(Path(compositeCard0Connected)) == "connected\n"
             || Files::LoadFile(Path(compositeCard1Connected)) == "connected\n"
             || Files::LoadFile(Path(compositeCard2Connected)) == "connected\n";
      #endif
    }

    //! This adapter is an RGB Dual
    [[nodiscard]] CrtAdapterType GetCrtAdapter() const override { return CrtAdapterType::RGBDual2; }

    [[nodiscard]] bool HasInterlacedSupport() const override {
      return ! std::filesystem::exists("/sys/firmware/devicetree/base/recalboxrgbdual2/avoid_interlaced");
    }

    //! RGB Dual has support for 31khz
    [[nodiscard]] bool Has31KhzSupport() const override { return true; }

    //! RGB Dual has support for 120hz modes
    [[nodiscard]] bool Has120HzSupport() const override { return GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHz31; }

    //! RGB Dual has support for composite
    [[nodiscard]] bool HasCompositeSupport() const override { return true; }

    [[nodiscard]] bool HasWideScreenSupport() const override { return true; }

    [[nodiscard]] bool ShouldWideScreen() const override { return CrtConf::Instance().GetSystemCRTWideScreen() && GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHz15; }

    [[nodiscard]] bool MustForceComposite() const override { return GetRGBDualCompositeSwitchState(); };

    //! Return select output frequency
    [[nodiscard]] HorizontalFrequency GetHorizontalFrequency() const override {
      HorizontalFrequency configFreq = static_cast<HorizontalFrequency>(CrtConf::Instance().GetSystemCRTScreenType());
      if(GetRGBDualCompositeSwitchState())
        return HorizontalFrequency::KHz15;
      if(MustForce31kHzByDefault() && configFreq != ICrtInterface::HorizontalFrequency::Auto)
        return configFreq;
      return MustForce31kHzByDefault() ? HorizontalFrequency::KHz31 : HorizontalFrequency::KHz15;
    }

    //! This adapter has support of forced 50hz
    [[nodiscard]] bool HasForced50hzSupport() const override { return true; }
    [[nodiscard]] bool HasForced31khzSupport() const override { return true; }
    [[nodiscard]] bool HasForcedCompositeSupport() const override { return true; }

    //! Get 50hz switch state
    [[nodiscard]] bool MustForce50Hz() const override { return GetRGBDual50hzSwitchState(); }

    //! 31kHz DIP — soft default, user can still override via menu.
    [[nodiscard]] bool MustForce31kHzByDefault() const override { return GetRGBDual31khzSwitchState(); }

    //! The comment is here to tell you that the name will be returned bby this methode named Name()
    [[nodiscard]] std::string& Name() const override { static std::string adapterString("Recalbox RGB Dual 2"); return adapterString; }

    [[nodiscard]] std::string& ShortName() const override { static std::string adapterShortString("recalboxrgbdual2"); return adapterShortString; }

    [[nodiscard]] bool HasForcedResolution(int &w, int &h) const override {
      if(GetRGBDualCompositeSwitchState())
      {
        if(MustForce50Hz())
        {
          if(CrtConf::Instance().GetSystemCRTResolution() == "480")
          {
            w = 720; h = 576;
          }
          else {
            w = 720; h = 288;
          }
        }
        else
        {
          if(CrtConf::Instance().GetSystemCRTResolution() == "480")
          {
            w = 720; h = 480;
          }
          else
          {
            w = 720; h = 240;
          }
        }
        return true;
      }
      return false;
    }


  private:
    static constexpr const char* sRGBDual31khzSwitch = "/sys/devices/platform/recalboxrgbdual2/dipswitch-31khz/value";
    static constexpr const char* sRGBDual50hzSwitch = "/sys/devices/platform/recalboxrgbdual2/dipswitch-50hz/value";
    static constexpr const char* sRGBDualCompositeSwitch = "/sys/devices/platform/recalboxrgbdual2/dipswitch-composite/value";
    static constexpr const char* vgaCard0Connected = "/sys/class/drm/card0-VGA-1/status";
    static constexpr const char* vgaCard1Connected = "/sys/class/drm/card1-VGA-1/status";
    static constexpr const char* vgaCard2Connected = "/sys/class/drm/card2-VGA-1/status";
    static constexpr const char* compositeCard0Connected = "/sys/class/drm/card0-Composite-1/status";
    static constexpr const char* compositeCard1Connected = "/sys/class/drm/card1-Composite-1/status";
    static constexpr const char* compositeCard2Connected = "/sys/class/drm/card2-Composite-1/status";

    //! Get 31khz switch state on RGB dual board
    static bool GetRGBDual31khzSwitchState() { return Files::LoadFile(Path(sRGBDual31khzSwitch)) == "0\n"; }
    //! Get 50hz switch state on RGB dual board
    static bool GetRGBDual50hzSwitchState() { return Files::LoadFile(Path(sRGBDual50hzSwitch)) == "0\n"; }
    //! Get composite switch state on RGB dual board
    static bool GetRGBDualCompositeSwitchState() { return Files::LoadFile(Path(sRGBDualCompositeSwitch)) == "0\n"; }


};



