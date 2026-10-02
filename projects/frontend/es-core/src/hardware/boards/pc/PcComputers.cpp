//
// Created by bkg2k on 02/11/2020.
//

#include <utils/os/fs/Path.h>
#include <utils/Files.h>
#include <utils/Log.h>
#include "PcComputers.h"

void PcComputers::SetPerformanceGovernance(IBoardInterface::PerformanceGovernance cpuGovernance)
{
  switch (cpuGovernance)
  {
    case PerformanceGovernance::PowerSave:
    {
      { LOG(LogInfo) << "[Performance] Set powersaving mode"; }
      Files::SaveFile(Path(sCpuGovernancePath), "powersave");
      break;
    }
    case PerformanceGovernance::OnDemand:
    {
      { LOG(LogInfo) << "[Performance] Set on-demand mode"; }
      Files::SaveFile(Path(sCpuGovernancePath), "ondemand");
      break;
    }
    case PerformanceGovernance::FullSpeed:
    {
      { LOG(LogInfo) << "[Performance] Set performance mode"; }
      Files::SaveFile(Path(sCpuGovernancePath), "performance");
      break;
    }
    default: break;
  }
}

bool PcComputers::HasBattery()
{
  static bool hasBattery = Path(sBatteryCapacityPath1).Exists() ||
                           Path(sBatteryCapacityPath2).Exists();
  return hasBattery;
}

int PcComputers::BatteryChargePercent()
{
  static Path sBatteryCharge(Path(sBatteryCapacityPath1).Exists() ? sBatteryCapacityPath1 : sBatteryCapacityPath2);
  int charge = -1;
  (void)Files::LoadFile(sBatteryCharge).Trim('\n').TryAsInt(charge);
  return charge;
}

bool PcComputers::IsBatteryCharging()
{
  static Path sBatteryStatus(Path(sBatteryStatusPath1).Exists() ? sBatteryStatusPath1 : sBatteryStatusPath2);
  return Files::LoadFile(sBatteryStatus).Trim('\n') == "Charging";
}

void PcComputers::SetFrontendPerformanceGovernor()
{
  SetPerformanceGovernance(PerformanceGovernance::PowerSave);
}
