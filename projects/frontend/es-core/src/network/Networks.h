//
// Created by bkg2k on 13/12/24.
//
#pragma once

#include "utils/os/system/Thread.h"
#include "utils/os/system/Signal.h"
#include "utils/cplusplus/StaticLifeCycleControler.h"
#include "utils/sync/ISyncMessageReceiver.h"
#include "utils/sync/SyncMessageSender.h"
#include <utils/storage/Array.h>
#include "IWifiHotspotListChanged.h"
#include <utils/cplusplus/Bitflags.h>

// declarative menus
class WindowManager;

class Networks : public StaticLifeCycleControler<Networks>
           , public Thread
           , private ISyncMessageReceiver<void>
{
  public:
    enum class SignalStrength
    {
      Unknown,
      Unusable,
      Bad,
      Average,
      Good,
      VeryGood,
    };

    //! Wifi hotspot structure
    class HotSpot
    {
      public:
        //! Constructor
        HotSpot(const String& name, SignalStrength level, bool secure, bool fiveghz)
          : mName(name)
          , mLevel(level)
          , mSecure(secure)
          , mFiveGhz(fiveghz)
        {}

        // Accessors
        [[nodiscard]] const String& Name() const { return mName; }
        [[nodiscard]] SignalStrength Level() const { return mLevel; }
        [[nodiscard]] bool Secure() const { return mSecure; }
        [[nodiscard]] bool FiveGhz() const { return mFiveGhz; }

      private:
        String         mName;  //!< SSID Name
        SignalStrength mLevel; //!< Precalculated level of attenuation, from 0 to 4
        bool mSecure;          //!< True if the hotspot is secure, false if its free
        bool mFiveGhz;         //!< True if the horspot is a 5Ghz hotspot
    };

    /*!
     * @brief Constructor
     * @param window Window manager
     */
    Networks();

    //! Destructor
    ~Networks() override
    {
      Thread::Stop();
    }

    /*!
     * @brief Query Recalbox DNS record to get the txt record corresponding to the given sub-domain
     * @param subDomain sub-domain to query
     * @return Query result
     */
    static String QueryDNSRecord(const String& subDomain);

    /*!
     * @brief Let the thread scan WIFI hotspots every 10 seconds, until the method is called again with false
     * @param on True to start scanning, false to stop.
     */
    void SetScan(bool on)
    {
      Mutex::AutoLock locker(mGuardian);
      mScan = on;
      if (on) mSignal.Fire();
    }

    /*!
     * @brief Get current hostpot list
     * @return
     */
    std::vector<HotSpot> HotspotList() { Mutex::AutoLock locker(mGuardian); return mHotSpotList; }

    /*!
     * @brief Format hotspot in a human readable string
     * @param hotspot Hotspot structure
     * @return Readable hotspot string
     */
    static String FormatHotspot(const HotSpot& hotspot);

    /*
     * Registering/unregistering list change
     */

    //! Register new interface
    void Register(IWifiHotspotListChanged* interface) { mNotifications.Add(interface); }

    //! Unregister new interface
    void Unregister(IWifiHotspotListChanged* interface) { mNotifications.Remove(interface); }

    /*
     * IPs
     */

    //! IP versions
    enum class IPVersion
    {
      IPv4 = 1, //!< IPv4 (bit 1)
      IPv6 = 2, //!< IPv6 (bit 2)
      Both = 3, //!< IPv4 and IPv5 (bit 1 & 2)
    };

    //! Interfaces
    enum class Interfaces
    {
      Ethernet = 1, //!< Wired interfaces (bit 1)
      Wifi     = 2, //!< WIFI interfaces (bit 2)
      Both     = 3, //!< All wired & wifi interfaces (bit 1 & 2)
    };

    /*!
     * @brief Check if the system has at least one ip available regadring the given IP version & interface filters
     * @param ipVersion IP version filter
     * @param interfaces Interface filter
     * @return True if at least one IP is available
     */
    bool HasIP(IPVersion ipVersion, Interfaces interfaces);

    /*!
     * @brief Get the first ip available regadring the given IP version & interface filters
     * @param ipVersion IP version filter
     * @param interfaces Interface filter
     * @return IP or empty string
     */
    String IP(IPVersion ipVersion, Interfaces interfaces);

    /*!
     * @brief Check if there is as least one WIFI interface
     * @return True if at least one WIFI interface is available, false otherwise
     */
    bool HasWIFIInterface() { return !mWlanInterfaces.empty(); }

  private:
    //! Sleep time between 2 scans
    static constexpr int sSleepTime = 1000 * 10;

    //! Notification interface list
    Array<IWifiHotspotListChanged*> mNotifications;
    //! WLAN Interfaces
    String::List mWlanInterfaces;

    //! HotSpot list
    std::vector<HotSpot> mHotSpotList;
    //! Scan flag
    bool mScan;
    //! HotSpot/Scan flag list guardian
    Mutex mGuardian;

    //! Thread signal
    Signal mSignal;

    //! Message sender
    SyncMessageSender<void> mSender;

    /*!
     * @brief "non-blocking" wait. Exit asap if the thread is stopping
     * @param ms Milliseconds to wait
     */
    void Wait(int ms);

    /*!
     * @brief execute th egiven command & return the output in a string list
     * @param command Command to execute
     * @return Comment output
     */
    static String::List Execute(const String& command);

    /*!
     * @brief Convert attenuation level
     * @param sLevel Attenuation level as string
     * @return Integer attenuation level
     */
    static SignalStrength ConvertLevel(const String& sLevel);

    /*!
     * @brief Collect and store all WLAN interfaces
     */
    void CollectWLANInterfaces();

    /*!
     * @brief Run scan on all interfaces
     */
    void StartScan();

    /*!
     * @brief Collect scan result from all interfaces
     * @return Merged scan results
     */
    void CollectResults();

    /*!
     * @brief Stop scan on all interfaces
     */
    void StopScan();

    /*
     * Helpers
     */

    bool IsWiredInterface(struct ifaddrs& addr);

    bool IsWifiInterface(struct ifaddrs& addr);

    static bool IsLocalIPv4(const String& ip);

    static bool IsLocalIPv6(const String& ip);

    static bool IsLoopbackIPv4(const String& ip);

    static bool IsLoopbackIPv6(const String& ip);

    /*
     * Thread implementation
     */

    /*!
     * @brief Wake up the thread on quit
     */
    void Break() final { mSignal.Fire(); }

    /*!
     * @brief Scanner method
     */
    void Run() final;

    /*
     * ISyncMessageReceiver<void> implementation
     */

    void ReceiveSyncMessage() final;
};

DEFINE_BITFLAG_ENUM(Networks::IPVersion, int)
DEFINE_BITFLAG_ENUM(Networks::Interfaces, int)