//
// Created by bkg2k on 24/04/2026.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <utils/String.h>
#include <utils/Sizes.h>
#include <utils/os/fs/Path.h>
#include <utils/cplusplus/StaticLifeCycleControler.h>
#include <utils/storage/HashMap.h>

class PhysicalDevices : public StaticLifeCycleControler<PhysicalDevices>
{
  public:
    //! Physical disk descriptor
    struct Device
    {
      String    DevicePath;          //!< Device path, e.g. /dev/sda
      String    DisplayName;         //!< Vendor + model string
      long long Size;                //!< Size in bytes
      bool      HasPartitions;       //!< True if the disk has at least one partition
      bool      HasDataOnPartitions; //!< True if a mounted partition has content in its root
      bool      IsBootDevice;        //!< True if one of this disk's partitions is mounted at /boot

      Device() : Size(0), HasPartitions(false), HasDataOnPartitions(false), IsBootDevice(false) {}
      Device(const String& path, const String& name, long long size, bool hasPartitions, bool hasDataOnPartitions, bool isBootDevice)
        : DevicePath(path), DisplayName(name), Size(size), HasPartitions(hasPartitions), HasDataOnPartitions(hasDataOnPartitions), IsBootDevice(isBootDevice) {}

      [[nodiscard]] String DisplayableName() const
      {
        String result(DisplayName);
        if (result.empty()) result = Path(DevicePath).Filename();
        if (Size != 0) result.Append(' ').Append(Sizes(Size).ToHumanSize());
        return result;
      }
    };

    //! Constructor
    PhysicalDevices();

    //! Re-enumerate physical disks (call before opening the install menu)
    PhysicalDevices& Refresh() { Initialize(); return *this; }

    //! Get enumerated physical disk list (only disks >= 15 GB)
    [[nodiscard]] const std::vector<Device>& GetDevices() const { return mDevices; }

  private:
    //! Minimum disk size: 15 GB decimal so all "16 GB" advertised devices qualify
    static constexpr long long sMinimumSizeBytes = 15LL * 1000LL * 1000LL * 1000LL;

    //! Enumerated physical disks
    std::vector<Device> mDevices;

    //! Enumerate and populate mDevices
    void Initialize();

    //! Returns true if the block device name is a real physical disk (not virtual)
    static bool IsPhysicalDisk(const String& name);

    //! Returns true if the block device is read-only
    static bool IsReadOnly(const String& name);

    //! Read disk size in bytes from /sys/block/{name}/size (512-byte sectors)
    static long long ReadSize(const String& name);

    //! Build human-readable name from /sys/block/{name}/device/{vendor,model}
    static String ReadName(const String& name);

    //! Returns true if the disk has at least one partition subdirectory in /sys/block/{name}/
    static bool CheckHasPartitions(const String& name);

    //! Returns true if any partition of the disk is mounted and has data in its filesystem root
    static bool CheckHasDataOnPartitions(const String& name, const HashMap<String, String>& mountedPaths);

    //! Returns true if any partition of the disk is mounted at /boot (i.e. this is the running boot disk)
    static bool CheckIsBootDevice(const String& name, const HashMap<String, String>& mountedPaths);
};
