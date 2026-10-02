//
// Recalbox Gamepad Proxy (R.G.P.) frontend wrapper.
// Owns the bridge between the user-facing RecalboxConf state and the
// kernel module sysfs at /sys/class/recalbox_gamepad_proxy/.
//
#pragma once

#include "utils/String.h"

class RGPProxy
{
  public:
    static RGPProxy& Instance();

    [[nodiscard]] bool   IsAvailable() const;
    [[nodiscard]] String Version() const;

    [[nodiscard]] static bool IsProxified(int sdlJoystickIndex);

    /*!
     * @brief Read the kernel-assigned UID stamped on a joystick (source or proxy).
     * Returns 0 if the device is not tracked by the R.G.P. module (file absent
     * or empty), since kernel UIDs always start at 1.
     */
    [[nodiscard]] static unsigned int GetUid(int sdlJoystickIndex);

    /*!
     * @brief True iff the joystick at @p sdlJoystickIndex is the proxy that
     * mirrors the source bearing UID @p uid. Detects proxies by the
     * "(R.G.P.)" suffix the kernel module appends to vdev names.
     */
    [[nodiscard]] static bool IsProxyForUid(int sdlJoystickIndex, unsigned int uid);

    /*!
     * @brief Scan all currently visible SDL joysticks and return the index
     * of the proxy carrying @p uid, or -1 if none.
     */
    [[nodiscard]] static int FindProxyForUid(unsigned int uid);

    [[nodiscard]] String::List GetDeviceFilters() const;
    RGPProxy&                  SetDeviceFilters(const String::List& filters);
    bool                       AddDeviceFilter(const String& controller);
    bool                       RemoveDeviceFilter(const String& controller);
    RGPProxy&                  ClearDeviceFilters();

    RGPProxy& Apply();

    /*!
     * @brief Snapshot of the kernel "exit combo" toggles read from sysfs.
     * Exit combos are the in-game shortcuts that quit the running emulator:
     * L3+R3 (`enable_thumbstick_exit`) and START long-press
     * (`enable_longpress_exit`). They must be neutralised while the user
     * configures or tests a controller — otherwise pressing those buttons
     * during the wizard would kill the parent process / break the flow.
     * Always pair DisableExitCombos() with RestoreExitCombos() at the end.
     */
    struct ExitCombosState
    {
      bool valid = false;          //!< False if the kernel module is absent
      bool thumbstickExit = false; //!< Snapshot of enable_thumbstick_exit
      bool longpressExit = false;  //!< Snapshot of enable_longpress_exit
    };

    /*!
     * @brief Read the current exit-combo toggles, write 0 to both, return
     * the captured state. The caller MUST hand the returned value back
     * to RestoreExitCombos() once the input flow is over (success OR
     * cancel — call it from the destructor).
     */
    [[nodiscard]] ExitCombosState DisableExitCombos();

    /*!
     * @brief Re-apply the toggle values captured by DisableExitCombos().
     * No-op when @p state was returned without the kernel module being
     * present (state.valid == false), so it's always safe to call.
     */
    void RestoreExitCombos(const ExitCombosState& state);

  private:
    RGPProxy() = default;
    RGPProxy(const RGPProxy&) = delete;
    RGPProxy& operator=(const RGPProxy&) = delete;

    void PushDeviceFilters(const String::List& filters) const;
    void PushToggles() const;
};
