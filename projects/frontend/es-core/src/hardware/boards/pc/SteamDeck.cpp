//
// Created by bkg2k on 02/11/2020.
//

#include <utils/os/fs/Path.h>
#include <utils/Files.h>
#include <utils/Log.h>
#include "SteamDeck.h"

void SteamDeck::SetPerformanceGovernance(IBoardInterface::PerformanceGovernance governance)
{
  switch (governance)
  {
    case PerformanceGovernance::PowerSave:
    {
      { LOG(LogInfo) << "[Performance] Set powersaving mode"; }
      Files::SaveFile(Path(sCpuGovernancePath), "powersave");
      { LOG(LogInfo) << "[EPP] Set power mode"; }
      Files::SaveFile(Path(sEppProfilePath), "power");
      break;
    }
    case PerformanceGovernance::OnDemand:
    {
      { LOG(LogInfo) << "[Performance] Set on-demand mode"; }
      Files::SaveFile(Path(sCpuGovernancePath), "ondemand");
      { LOG(LogInfo) << "[EPP] Set balance power mode"; }
      Files::SaveFile(Path(sEppProfilePath), "ondemand");
      break;
    }
    case PerformanceGovernance::FullSpeed:
    {
      { LOG(LogInfo) << "[Performance] Set performance mode"; }
      Files::SaveFile(Path(sCpuGovernancePath), "performance");
      { LOG(LogInfo) << "[EPP] Set performance mode"; }
      Files::SaveFile(Path(sEppProfilePath), "performance");
      break;
    }
    default: break;
  }
}

void SteamDeck::SetLowestBrightness()
{
  Files::SaveFile(Path("/sys/class/backlight/amdgpu_bl0/brightness"), "0");
}

void SteamDeck::SetBrightness(int step)
{
  String maxValue = Files::LoadFile(Path("/sys/class/backlight/amdgpu_bl0/max_brightness"));
  int max = 255; // Max SD value
  (void)maxValue.Trim("\r\n").TryAsInt(max);
  int value = (step * max) / 8;
  Files::SaveFile(Path("/sys/class/backlight/amdgpu_bl0/brightness"), String(value));
}

int SteamDeck::BatteryChargePercent()
{
  static Path sBatteryCharge(Path(sBatteryCapacityPath1).Exists() ? sBatteryCapacityPath1 : sBatteryCapacityPath2);
  int charge = -1;
  (void)Files::LoadFile(sBatteryCharge).Trim('\n').TryAsInt(charge);
  return charge;
}

bool SteamDeck::IsBatteryCharging()
{
  static Path sBatteryStatus(Path(sBatteryStatusPath1).Exists() ? sBatteryStatusPath1 : sBatteryStatusPath2);
  return Files::LoadFile(sBatteryStatus).Trim('\n') == "Charging";
}

void SteamDeck::SetFrontendPerformanceGovernor()
{
  SetPerformanceGovernance(PerformanceGovernance::PowerSave);
}

void SteamDeck::PostResumeActions()
{
  system("xrandr --output eDP --rotate right --preferred");
  // fix input gamepad and desktop mode that operates both at the same time
  Files::SaveFile(Path("/sys/module/hid_steam/parameters/lizard_mode"), "Y");
}
