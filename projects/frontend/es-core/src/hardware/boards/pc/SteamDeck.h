//
// Created by davidb2111 on 17/09/2025
//
#pragma once

#include <hardware/messaging/IHardwareNotifications.h>
#include "hardware/IBoardInterface.h"
#include "hardware/boards/pc/SteamDeckPowerEventReader.h"

class SteamDeck: public IBoardInterface
{
  public:
    explicit SteamDeck(HardwareMessageSender& messageSender, BoardType model)
      : IBoardInterface(messageSender)
      , mPowerReader(messageSender)
      , mModel(model)
    {
    }

  private:
    static constexpr const char* sCpuGovernancePath    = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";
    static constexpr const char* sEppProfilePath       = "/sys/devices/system/cpu/cpu0/cpufreq/energy_performance_preference";
    static constexpr const char* sBatteryCapacityPath1 = "/sys/class/power_supply/BAT0/capacity";
    static constexpr const char* sBatteryStatusPath1   = "/sys/class/power_supply/BAT0/status";
    static constexpr const char* sBatteryCapacityPath2 = "/sys/class/power_supply/BAT1/capacity";
    static constexpr const char* sBatteryStatusPath2   = "/sys/class/power_supply/BAT1/status";

    //! Power button event reader
    SteamDeckPowerEventReader mPowerReader;
    //! Type
    BoardType mModel;

    /*!
     * @brief Start optional global background processes
     * This method is called when ES starts
     */
    void StartGlobalBackgroundProcesses() final
    {
      mPowerReader.StartReader();
    }

    /*!
     * @brief Stop optional global background processes
     * This method is called when ES stops
     */
    void StopGlobalBackgroundProcesses() final {}

    /*!
     * @brief Start optional in-game background processes.
     * This method is called when a game starts
     */
    void StartInGameBackgroundProcesses(Sdl2Runner& runner) final { (void)runner; }

    /*!
     * @brief Stop optional in-game background processes.
     * This method is called when a game stops
     */
    void StopInGameBackgroundProcesses(Sdl2Runner& runner) final { (void)runner; }

    /*!
     * @brief Has Battery?
     */
    bool HasBattery() final { return true; };

    /*!
     * @brief Has Performance governance? (and is it worth the use)
     */
    bool HasPerformanceGovernance() final { return true; }

    /*!
     * @brief Has physical volume buttons?
     */
    bool HasMappableVolumeButtons() final { return false; }

    /*!
     * @brief Has physical brightness buttons?
     */
    bool HasMappableBrightnessButtons() final { return false; }

    /*!
     * @brief Has physical brightness buttons?
     */
    bool HasBrightnessSupport() final { return true; }

    /*!
     * @brief Has hardware suspend/resume?
     */
    bool HasSuspendResume() final { return true; }

    /*!
     * @brief Set lowest brightess possible, including black screen
     */
    void SetLowestBrightness() final;

    /*!
     * @brief Set brightness
     * @param brighness brightness step from 0 to 8 included
     */
    void SetBrightness(int brighness) final;

    /*!
     * @brief Set new Performance governance
     * @param governance cpu governance
     */
    void SetPerformanceGovernance(PerformanceGovernance governance) final;

    /*!
     * @brief Process special input if any
     * @param inputEvent Input to process
     * @return True if the input has been processed, false otherwise
     */
    bool ProcessSpecialInputs(InputCompactEvent& inputEvent, ISpecialGlobalAction* action) final { (void)inputEvent; (void)action; return false; }

    /*!
     * @brief Suspend!
     */
    void Suspend() final { mPowerReader.Suspend(); }

    /*!
     * @brief Get battery charge in percent
     * @return Battery charge (-1 = no battery)
     */
    int BatteryChargePercent() final;

    /*!
     * @brief Check if the battery is charging
     * @return True = charging, False = discharging or no battery
     */
    bool IsBatteryCharging() final;

    /*!
     * @brief The reboot or shutdown is managed by MainRunner, but the board can have some features to manage
     * @return True if a side effect has been triggered
     */
    bool OnRebootOrShutdown() override { return false; }

    void HeadphonePlugged() final {};
    void HeadphoneUnplugged() final {};

    /*!
     * @brief Set the Performance governor for EmulationStation
     */
    void SetFrontendPerformanceGovernor() final;

    void PostResumeActions() final;
    /*!
    * @brief Has vulkan support
    */
    bool HasVulkanSupport() final
    { return true; }
};
