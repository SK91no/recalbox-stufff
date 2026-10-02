//
// Created by bkg2k on 30/10/2020.
// Modified by davidb2111 for the RG353x series of boards
//

#include "RG353XBoard.h"
#include <input/InputCompactEvent.h>
#include <input/InputManager.h>
#include <RecalboxConf.h>
#include <utils/math/Misc.h>
#include <utils/Files.h>
#include <audio/AudioController.h>

bool RG353XBoard::ProcessSpecialInputs(InputCompactEvent& inputEvent, ISpecialGlobalAction* action)
{
  (void)action;

  // Trap hotkey event, as it is needed to handle volup/voldown or brightup/brightdown with HK
  if (inputEvent.HotkeyPressed())
    mVolumeReader.HotkeyPressed();
  if (inputEvent.HotkeyReleased())
    mVolumeReader.HotkeyReleased();

  return false;
}

void RG353XBoard::SetLowestBrightness()
{
  Files::SaveFile(Path("/sys/class/backlight/backlight/brightness"), "0");
}

void RG353XBoard::SetBrightness(int step)
{
  String maxValue = Files::LoadFile(Path("/sys/class/backlight/backlight/max_brightness"));
  int max = 1666; // Max RG value
  (void)maxValue.Trim("\r\n").TryAsInt(max);
  int value = ((max >> 8) + 1) << step; if (value > max) value = max;
  Files::SaveFile(Path("/sys/class/backlight/backlight/brightness"), String(value));
}

void RG353XBoard::SetPerformanceGovernance(IBoardInterface::PerformanceGovernance cpuGovernance)
{
  switch (cpuGovernance)
  {
    case PerformanceGovernance::PowerSave:
    {
      { LOG(LogInfo) << "[Performance] Set powersaving mode"; }
      Files::SaveFile(Path(sCpuGovernancePath), "powersave");
      Files::SaveFile(Path(sGpuGovernancePath), "powersave");
      Files::SaveFile(Path(sDmcGovernancePath), "powersave");
      break;
    }
    case PerformanceGovernance::OnDemand:
    {
      { LOG(LogInfo) << "[Performance] Set on-demand mode"; }
      Files::SaveFile(Path(sCpuGovernancePath), "ondemand");
      Files::SaveFile(Path(sGpuGovernancePath), "simple_ondemand");
      Files::SaveFile(Path(sDmcGovernancePath), "dmc_ondemand");
      break;
    }
    case PerformanceGovernance::FullSpeed:
    {
      { LOG(LogInfo) << "[Performance] Set performance mode"; }
      Files::SaveFile(Path(sCpuGovernancePath), "performance");
      Files::SaveFile(Path(sGpuGovernancePath), "performance");
      Files::SaveFile(Path(sDmcGovernancePath), "performance");
      break;
    }
    default: break;
  }
}

int RG353XBoard::BatteryChargePercent()
{
  static Path sBatteryCharge(sBatteryCapacityPath);
  int charge = -1;
  (void)Files::LoadFile(sBatteryCharge).Trim('\n').TryAsInt(charge);
  return charge;
}

bool RG353XBoard::IsBatteryCharging()
{
  static Path sBatteryStatus(sBatteryStatusPath);
  return Files::LoadFile(sBatteryStatus).Trim('\n') == "Charging";
}

void RG353XBoard::HeadphonePlugged()
{
  AudioController::Instance().SetOutputPort("rk817-headphones");
}

void RG353XBoard::HeadphoneUnplugged()
{
  AudioController::Instance().SetOutputPort("rk817-speaker");
}

void RG353XBoard::SetFrontendPerformanceGovernor()
{
  SetPerformanceGovernance(PerformanceGovernance::OnDemand);
}

const RotationCapability RG353XBoard::GetRotationCapabilities() const {
  return {.rotationAvailable = true, .systemRotationAvailable = false, .defaultRotationWhenTate = RotationType::Left, .rotateControls = true, .autoRotateGames = true};
}
