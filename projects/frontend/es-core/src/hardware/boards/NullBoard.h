//
// Created by bkg2k on 02/11/2020.
//

#include <hardware/IBoardInterface.h>

class NullBoard: public IBoardInterface
{
  public:
    explicit NullBoard(HardwareMessageSender& messageSender)
      : IBoardInterface(messageSender)
    {
    }

  private:
    void StartGlobalBackgroundProcesses() final {}

    void StopGlobalBackgroundProcesses() final {}

    void StartInGameBackgroundProcesses(Sdl2Runner&) final {}

    void StopInGameBackgroundProcesses(Sdl2Runner&) final {}

    bool HasBattery() final { return false; }

    bool HasPerformanceGovernance() final { return false; }

    bool HasMappableVolumeButtons() final { return false; }

    bool HasMappableBrightnessButtons() final { return false; }

    bool HasBrightnessSupport() final { return false; }

    bool HasSuspendResume() final { return false; }

    void SetPerformanceGovernance(PerformanceGovernance governance) final { (void)governance; }

    void SetLowestBrightness() final {}

    void SetBrightness(int brighness) final { (void)brighness; }

    int BatteryChargePercent() final { return -1; }

    bool IsBatteryCharging() final { return false; }

    bool ProcessSpecialInputs(InputCompactEvent& inputEvent, ISpecialGlobalAction* action) final { (void)inputEvent; (void)action; return false; }

    void Suspend() final {}

    bool OnRebootOrShutdown() { return false; }

    void HeadphonePlugged() final {};

    void HeadphoneUnplugged() final {};

    void SetFrontendPerformanceGovernor() final {};

    void PostResumeActions() final {};

    bool HasVulkanSupport() final { return false;};
};

