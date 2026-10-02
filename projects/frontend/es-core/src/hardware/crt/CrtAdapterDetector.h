//
// Created by bkg2k on 20/12/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <hardware/crt/CrtAdapterType.h>
#include <hardware/crt/ICrtInterface.h>
#include "Options.h"

class CrtAdapterDetector
{
  public:
    static constexpr const char* sHatProductFile = "/sys/firmware/devicetree/base/hat/product";
    static constexpr const char* sHatVendorFile = "/sys/firmware/devicetree/base/hat/vendor";
    static constexpr const char* sRRGBD2File = "/sys/bus/platform/devices/recalboxrgbdual2/of_node/name";

    static constexpr const char* sRGBDualProductString = "Recalbox RGB Dual";
    static constexpr const char* sRecalboxVendorString = "Recalbox";
    static constexpr const char* sRRGBD2String = "recalboxrgbdual2";

    /*!
     * @brief Detect & create CRT board
     * @return CRT board implementation or nullptr
     */
    static ICrtInterface* CreateCrtBoard(BoardType boardType, const Options& options);

  private:
    /*!
     * @brief Detect CRT Adapter is any
     * @return Adapter type (or none)
     */
    static CrtAdapterType DetectCrtAdapter(bool& automaticallyDetected);
};



