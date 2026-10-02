//
// Created by bkg2k on 20/12/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//


#include <hardware/crt/CrtAdapterDetector.h>
#include <utils/Files.h>
#include <hardware/crt/CrtNull.h>
#include <hardware/crt/CrtVga666.h>
#include <hardware/crt/CrtRGBPi.h>
#include <hardware/crt/CrtPi2Scart.h>
#include <hardware/crt/CrtRGBDual.h>
#include <hardware/crt/CrtRGBJamma.h>

#include "CrtJVS.h"
#include "CrtRGBDual2.h"

CrtAdapterType CrtAdapterDetector::DetectCrtAdapter(bool& automaticallyDetected)
{
  // Reset
  CrtAdapterType result = CrtAdapterType::None;
  automaticallyDetected = false;

  #ifdef OPTION_RECALBOX_SIMULATE_RRGBD
  result = CrtAdapterType::RGBDual2;
  automaticallyDetected = true;
  return result;
  #endif
  // RGB Dual hat
  String vendor = Files::LoadFile(Path(sHatVendorFile));
  String product = Files::LoadFile(Path(sHatProductFile));
  if (vendor.rfind(sRecalboxVendorString) == 0 &&
      product.rfind(sRGBDualProductString) == 0)
  {
    result = CrtAdapterType::RGBDual;
    automaticallyDetected = true;
  }
  else
  {
    String rrgbd2 = Files::LoadFile(Path(sRRGBD2File));
    if (rrgbd2.rfind(sRRGBD2String) == 0)
    {
      automaticallyDetected = true;
      result = CrtAdapterType::RGBDual2;
    }
    else
    {
      // User config
      result = CrtConf::Instance().GetSystemCRT();
    }
  }

  return result;
}

ICrtInterface* CrtAdapterDetector::CreateCrtBoard(BoardType boardType, const Options& options)
{
  if (options.EmulateRGBDual()) return new CrtRGBDual(true, boardType);
  if (options.EmulateRGBJamma()) return new CrtRGBJamma(true, boardType);
  bool automatic = false;
  ICrtInterface* adapter = nullptr;
  switch(DetectCrtAdapter(automatic))
  {
    case CrtAdapterType::Vga666: adapter = new CrtVga666(automatic, boardType); break;
    case CrtAdapterType::RGBPi: adapter = new CrtRGBPi(automatic, boardType); break;
    case CrtAdapterType::Pi2Scart: adapter = new CrtPi2Scart(automatic, boardType); break;
    case CrtAdapterType::RGBDual: adapter = new CrtRGBDual(automatic, boardType); break;
    case CrtAdapterType::RGBDual2: adapter = new CrtRGBDual2(automatic, boardType); break;
    case CrtAdapterType::RGBJamma: adapter = new CrtRGBJamma(automatic, boardType); break;
    case CrtAdapterType::JVS: adapter = new CrtJVS(automatic, boardType); break;
    case CrtAdapterType::None:
    default: adapter = new CrtNull(automatic, boardType);;
  }

  if (adapter->GetCrtAdapter() != CrtAdapterType::None)
  {
    int w = 0, h = 0;
    LOG(LogInfo) << "[CrtAdapterDetector] CRT adapter: " << adapter->Name()
                 << " (IsCrtAdapterAttached = " << adapter->IsCrtAdapterAttached()
                 << ", HasInterlacedSupport: " << adapter->HasInterlacedSupport()
                 << ", Has31KhzSupport: " << adapter->Has31KhzSupport()
                 << ", Has120HzSupport: " << adapter->Has120HzSupport()
                 << ", HasMultiSyncSupport: " << adapter->HasMultiSyncSupport()
                 << ", HasCompositeSupport: " << adapter->HasCompositeSupport()
                 << ", HasForced50hzSupport: " << adapter->HasForced50hzSupport()
                 << ", GetHorizontalFrequency: "
                 << ICrtInterface::HorizontalFrequencyToString(adapter->GetHorizontalFrequency())
                 << ", MustForce50Hz: " << adapter->MustForce50Hz()
                 << ", MustForceComposite: " << adapter->MustForceComposite()
                 << ", HasBeenAutomaticallyDetected: " << adapter->HasBeenAutomaticallyDetected()
                 << ", HasForcedResolution: " << adapter->HasForcedResolution(w, h)
                 << ", ForcedResolution: " << w << "x" << h <<")";
  }

  return adapter;
}
