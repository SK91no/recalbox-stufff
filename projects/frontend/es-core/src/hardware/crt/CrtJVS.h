//
// Created by digitalLumberjack on 24/06/2024.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <hardware/crt/ICrtInterface.h>
#include <hardware/JvsCab.h>
#include <filesystem>

class CrtJVS : public ICrtInterface
{
  public:
    //! Constructor
    explicit CrtJVS(bool automaticallyDetected, BoardType boardType) : ICrtInterface(automaticallyDetected, boardType)
    {
    }

    //! A JVS is attached
    bool IsCrtAdapterAttached() const override {
      #ifdef DEBUG
      return true;
      #else
      return Files::LoadFile(Path(vgaCard0Connected)) == "connected\n"
          || Files::LoadFile(Path(vgaCard1Connected)) == "connected\n"
          || Files::LoadFile(Path(vgaCard2Connected)) == "connected\n" ;
      #endif
    }

    //! This adapter is a JVS
    CrtAdapterType GetCrtAdapter() const override { return CrtAdapterType::JVS; }

    //! JVS supports 31kHz unless the cabinet DIP "15kHz only" is set
    bool Has31KhzSupport() const override { return !MustForce15kHz(); }

    //! True when the JVS firmware reports the cabinet is 15kHz-only (DIP).
    //! Drives the menu lock + the S05jvs-screentype boot sync.
    bool MustForce15kHz() const override {
      return Files::LoadFile(Path(sJvsKhz15Only)) == "1\n";
    }

    //! 31kHz-only cabs (Naomi + Vewlix) lock the screentype menu to 31kHz.
    bool MustForce31kHz() const override {
      return !MustForce15kHz() && JvsCab::Is31kHzOnly();
    }

    //! Return select output frequency. Forced 15kHz wins over forced 31kHz wins over cfg.
    HorizontalFrequency GetHorizontalFrequency() const override {
      if (MustForce15kHz()) return ICrtInterface::HorizontalFrequency::KHz15;
      if (MustForce31kHz()) return ICrtInterface::HorizontalFrequency::KHz31;
      HorizontalFrequency configFreq = static_cast<HorizontalFrequency>(CrtConf::Instance().GetSystemCRTScreenType());
      if(configFreq == ICrtInterface::HorizontalFrequency::Auto) return ICrtInterface::HorizontalFrequency::KHz31;
      return configFreq;
    }

    [[nodiscard]] bool HasInterlacedSupport() const override {
      return Files::LoadFile(Path(sInterlacedSupported)) == "true";
    }

    //! This adapter has no support of forced 50hz
    bool HasForced50hzSupport() const override { return false; }

    //! Get 50hz switch state
    bool MustForce50Hz() const override { return false; }

    std::string& Name() const override
    {
      static std::string adapterString("Recalbox JVS"); return adapterString;
    }

    std::string& ShortName() const override { static std::string adapterShortString("recalboxjvs"); return adapterShortString; }

    bool HasMultiSyncSupport() const override { return !MustForce15kHz() && !MustForce31kHz(); }

  private:
    static constexpr const char* vgaCard0Connected = "/sys/class/drm/card0-VGA-1/status";
    static constexpr const char* vgaCard1Connected = "/sys/class/drm/card1-VGA-1/status";
    static constexpr const char* vgaCard2Connected = "/sys/class/drm/card2-VGA-1/status";
    static constexpr const char* sInterlacedSupported = "/sys/kernel/recalboxrgbdual2/interlaced_supported";
    static constexpr const char* sJvsKhz15Only       = "/sys/devices/platform/recalboxjvs/khz15_only";
};
