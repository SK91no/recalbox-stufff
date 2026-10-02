//
// Created by digitalLumberjack on 30/03/2023.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <hardware/crt/ICrtInterface.h>
#include <filesystem>


class CrtRGBJamma : public ICrtInterface
{
  public:
    //! Constructor
    explicit CrtRGBJamma(bool automaticallyDetected, BoardType boardType) : ICrtInterface(automaticallyDetected, boardType)
    {
    }

    //! A RGB Jamma is attached
    bool IsCrtAdapterAttached() const override {
      #ifdef DEBUG
      return true;
      #else
      return Files::LoadFile(Path(vgaCard0Connected)) == "connected\n"
          || Files::LoadFile(Path(vgaCard1Connected)) == "connected\n"
          || Files::LoadFile(Path(vgaCard2Connected)) == "connected\n" ;
      #endif
    }

    //! This adapter is an RGB JAMMA
    CrtAdapterType GetCrtAdapter() const override { return CrtAdapterType::RGBJamma; }

    //! RGB Jamma has support for 31khz
    bool Has31KhzSupport() const override { return true; }

    //! Return select output frequency
    HorizontalFrequency GetHorizontalFrequency() const override {
      HorizontalFrequency configFreq = static_cast<HorizontalFrequency>(CrtConf::Instance().GetSystemCRTScreenType());
      if(configFreq == ICrtInterface::HorizontalFrequency::Auto) return ICrtInterface::HorizontalFrequency::KHz15;
      return configFreq;
    }

    [[nodiscard]] bool IsV2() const {
      return std::filesystem::exists("/sys/firmware/devicetree/base/recalboxrgbjamma/recalbox-rgb-jamma-2");
    }
    [[nodiscard]] bool HasInterlacedSupport() const override {
      return IsV2();
    }

    //! This adapter has no support of forced 50hz
    bool HasForced50hzSupport() const override { return false; }

    //! Get 50hz switch state
    bool MustForce50Hz() const override { return false; }

    std::string& Name() const override
    {
      if (IsV2())
      {
        static std::string adapterString("Recalbox RGB JAMMA 2"); return adapterString;
      }
      static std::string adapterString("Recalbox RGB JAMMA"); return adapterString;
    }

    std::string& ShortName() const override { static std::string adapterShortString("recalboxrgbjamma"); return adapterShortString; }

    bool HasMultiSyncSupport() const override { return true; }

  private:
    static constexpr const char* vgaCard0Connected = "/sys/class/drm/card0-VGA-1/status";
    static constexpr const char* vgaCard1Connected = "/sys/class/drm/card1-VGA-1/status";
    static constexpr const char* vgaCard2Connected = "/sys/class/drm/card2-VGA-1/status";
};
