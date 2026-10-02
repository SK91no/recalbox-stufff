//
// Created by bkg2k on 30/10/2020.
// Modified by davidb2111 for the RG351V board
//

#include "RG351VBoard.h"
#include <input/InputCompactEvent.h>
#include <input/InputManager.h>
#include <RecalboxConf.h>
#include <utils/math/Misc.h>
#include <utils/Files.h>
#include <audio/AudioController.h>

bool RG351VBoard::ProcessSpecialInputs(InputCompactEvent& inputEvent, ISpecialGlobalAction* action)
{
  (void)action;

  // Trap hotkey event, as it is needed to handle volup/voldown or brightup/brightdown with HK
  if (inputEvent.HotkeyPressed())
    mVolumeReader.HotkeyPressed();
  if (inputEvent.HotkeyReleased())
    mVolumeReader.HotkeyReleased();

  return false;
}

void RG351VBoard::SetLowestBrightness()
{
  Files::SaveFile(Path("/sys/class/backlight/backlight/brightness"), "0");
}

void RG351VBoard::SetBrightness(int step)
{
  String maxValue = Files::LoadFile(Path("/sys/class/backlight/backlight/max_brightness"));
  int max = 100; // Max RG value
  (void)maxValue.Trim("\r\n").TryAsInt(max);
  int value = 1 << step; if (value > max) value = max;
  Files::SaveFile(Path("/sys/class/backlight/backlight/brightness"), String(value));
}

void RG351VBoard::SetPerformanceGovernance(IBoardInterface::PerformanceGovernance cpuGovernance)
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

int RG351VBoard::BatteryChargePercent()
{
  static Path sBatteryCharge(sBatteryCapacityPath);
  int charge = -1;
  (void)Files::LoadFile(sBatteryCharge).Trim('\n').TryAsInt(charge);
  return charge;
}

bool RG351VBoard::IsBatteryCharging()
{
  static Path sBatteryStatus(sBatteryStatusPath);
  return Files::LoadFile(sBatteryStatus).Trim('\n') == "Charging";
}

void RG351VBoard::HeadphonePlugged()
{
  if (system("amixer sset 'Playback Path' HP") != 0)
  { LOG(LogError) << "[RG351VBoard] Error setting headphone on!"; }
}

void RG351VBoard::HeadphoneUnplugged()
{
  if (system("amixer sset 'Playback Path' SPK") != 0)
  { LOG(LogError) << "[RG351VBoard] Error setting headphone off!"; }
}

void RG351VBoard::SetFrontendPerformanceGovernor()
{
  SetPerformanceGovernance(PerformanceGovernance::OnDemand);
}

const RotationCapability RG351VBoard::GetRotationCapabilities() const {
  return {.rotationAvailable = false, .systemRotationAvailable = false, .defaultRotationWhenTate = RotationType::Left, .rotateControls = false, .autoRotateGames = false};
}
