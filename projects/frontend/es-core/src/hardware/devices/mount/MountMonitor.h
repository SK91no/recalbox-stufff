//
// Created by bkg2k on 14/01/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include "utils/os/system/Thread.h"
#include "IMountMonitorNotifications.h"
#include "utils/sync/SyncMessageSender.h"
#include "MountDevice.h"
#include "utils/os/system/Mutex.h"
#include <optional>
#include <utility>
#include <atomic>

class MountMonitor : private Thread
                   , private ISyncMessageReceiver<bool>
{
  public:
    //! Recalbox Mount point root
    static constexpr const char* sRecalboxRootMountPoint = "/recalbox/share/externals";
    //! Share path
    static constexpr const char* sSharePath = "/recalbox/share";
    //! Share roms path
    static constexpr const char* sShareRomsPath = "/recalbox/share/roms";

    //! Mount info list
    typedef std::vector<DeviceMount> DeviceMountList;

    //! Constructor
    explicit MountMonitor(IMountMonitorNotifications* interface);

    //! Destructor
    ~MountMonitor() override;

    //! Get a snapshot of the current mount point list (thread-safe)
    [[nodiscard]] DeviceMountList MountPoints();

    /*!
     * @brief Get size info for the mount point that contains path (thread-safe)
     * @param path Any path under a monitored mount point
     * @return Copy of the DeviceMount with updated size, or nullopt if path is unknown
     */
    std::optional<DeviceMount> SizeOf(const Path& path);

    /*!
     * @brief Get a snapshot of all mount points with updated sizes (thread-safe)
     * @return Copy of all DeviceMount objects with current free/total sizes
     */
    DeviceMountList AllMountPoints();

    //! Set new state. Acts as a barrier: when called with false, blocks until any in-flight
    //! monitoring cycle has finished and discards any events queued but not yet dispatched.
    void SetMonitoringState(bool state);

  private:
    //! Mount point list file
    static constexpr const char* sMountPointFile = "/proc/mounts";

    //! Syncro events
    SyncMessageSender<bool> mEvent;

    //! Mutex protecting mMountPoints and mPendingEvents
    Mutex mMutex;

    //! Current path list (written by worker thread, read by main thread)
    DeviceMountList mMountPoints;
    //! Share special mount point (init-only, then read-only from both threads)
    DeviceMount mShareMountPoint;
    //! Share/roms special mount point (init-only, then read-only from both threads)
    DeviceMount mShareRomsMountPoint;
    //! Queue of pending mount/unmount events: {true=mount, device}
    std::vector<std::pair<bool, DeviceMount>> mPendingEvents;

    //! Callback interface
    IMountMonitorNotifications* mInterface;

    //! Monitoring on/off
    std::atomic<bool> mState;

    /*!
     * @brief Load mount points from /proc/mounts
     * @param initializeSpecialMountPoints true to initialize share and roms mount points
     * @return Path list
     */
    DeviceMountList LoadMountPoints(bool initializeSpecialMountPoints);

    /*!
     * @brief Compute diff between old and new lists; returns (isMounted, device) pairs
     * @param oldList Old list (modified in-place for diffing)
     * @param newListOriginal New list (copied internally)
     * @return Events to dispatch: true=mounted, false=unmounted
     */
    std::vector<std::pair<bool, DeviceMount>> Process(DeviceMountList& oldList, const DeviceMountList& newListOriginal);

    /*!
     * @brief Get partition name from a device path
     * @param devicePath Device path
     * @return Partition name
     */
    static String GetPartitionLabel(const Path& devicePath);

    /*
     * Thread implementation
     */

    //! Monitor main loop
    void Run() override;

    /*
     * ISyncMessageReceiver implementation
     */

    //! Drain mPendingEvents and dispatch to mInterface
    void ReceiveSyncMessage(bool message) override;
};
