#pragma once

#include <utils/String.h>

struct EmulatorDefaults
{
  String core;
  String emulator;
};

/*!
 * @brief Interface between EmulationStation and the Recalbox System
 */
class RecalboxSystem
{
  public:
    static void EmptyUpdateFolder();

    static bool MakeBootReadOnly();

    static bool MakeBootReadWrite();

    static long long GetMinimumFreeSpaceOnSharePartition() { return 3LL << 30; } // 3Gb

    static unsigned long long getFreeSpace(const String& mountpoint);

    static unsigned long getFreeSpaceGB(const String& mountpoint);

    static bool isFreeSpaceLimit();

    static bool isFreeSpaceUnderLimit(long long size);

    static bool getWifiWps();

    static bool saveWifiWps();

    static bool getWifiConfiguration(String& ssid, String& psk);

    static bool ping();

    static bool kodiExists();

    static bool enableWifi();

    static bool disableWifi();

    static std::vector<String> scanBluetooth();

    static bool pairBluetooth(const String& basic_string);

    static bool forgetBluetoothControllers();

    static std::pair<String, int> execute(const String& command);

    static std::pair<String, int> getSDLBatteryInfo();

    static bool getSysBatteryInfo(int& charge, int& unicodeIcon);

    static bool IsLiteVersion();

  private:
    //! Share path
    static constexpr const char* sSharePath = "/recalbox/share/";
    static constexpr const char* sConfigScript = "/recalbox/scripts/recalbox-config.sh";
    static constexpr const char* sLiteFlagFile = "/recalbox/recalbox.lite";
    static constexpr const char* sLiteFlagTrackFile = "/overlay/recalbox.lite";

    static String BuildSettingsCommand(const String& arguments);

    static String::List ExecuteSettingsCommand(const String& arguments);
};

