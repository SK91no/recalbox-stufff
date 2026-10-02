//
// Created by bkg2k on 21/02/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <utils/String.h>
#include "utils/Sizes.h"
#include "utils/IniFile.h"
#include <hardware/devices/mount/MountDevice.h>

class StorageDevices : public StaticLifeCycleControler<StorageDevices>
{
  private:
    //! Size info
    struct SizeInfo
    {
      long long Size; //!< Size in Kbyte
      long long Free; //!< Free in Kbyte
      SizeInfo() : Size(0), Free(0) {}
      SizeInfo(long long s, long long f) : Size(s), Free(f) {}
    };

  public:
    //! Device Property Map type
    typedef HashMap<String, String>  PropertyMap;
    //! Device Property Map type
    typedef HashMap<String, struct SizeInfo>  DeviceSizeInfo;

    //! Storage types
    enum class Types
    {
      Internal, //!< Default partition
      Ram,      //!< tmpfs partition
      Network,  //!< Network share
      External, //!< External device
    };

    //! Physical media type
    enum class PhysicalType
    {
      Unknown, //!< Unknown / not detected
      HDD,     //!< Rotational hard disk drive
      SSD,     //!< SATA/PCIe solid state drive
      NVMe,    //!< NVMe solid state drive
      USB,     //!< USB mass storage
      SD,      //!< SD / microSD card
      eMMC,    //!< Embedded MMC (eMMC) flash storage
      Network, //!< Network share (CIFS / NFS)
      RAM,     //!< tmpfs in-memory storage
    };

    //! Storage descriptor
    struct Device
    {
      Types        Type;          //!< Device type
      PhysicalType MediaType;     //!< Physical media type
      String       DevicePath;    //!< Device path, i.e. /dev/sda1
      String       UUID;          //!< Device UUID
      String       PartitionName; //!< Partition name
      String       FileSystem;    //!< Partition filesystem
      String       DisplayName;   //!< Displayable name
      String       MountPoint;    //!< Mount point if available
      long long    Size;          //!< Total size in byte
      long long    Free;          //!< Free size in byte
      bool         Current;       //!< True for the current device
      bool         ReadOnly;      //!< Read only partition

      // Constructor
      Device()
        : Type(Types::Internal)
        , MediaType(PhysicalType::Unknown)
        , Size(0)
        , Free(0)
        , Current(false)
        , ReadOnly(true)
      {}

      // Constructor
      Device(Types type, const String& devicePath, const String& mountPoint, const String& uuid, const String& partitionName, const String& fileSystem, const String& displayName, bool current, const DeviceSizeInfo& size, bool readonly)
        : Type(type)
        , DevicePath(devicePath)
        , UUID(uuid)
        , PartitionName(partitionName)
        , FileSystem(fileSystem)
        , DisplayName(displayName)
        , MountPoint(mountPoint)
        , Size(0)
        , Free(0)
        , Current(current)
        , ReadOnly(readonly)
      {
        SizeInfo* info = size.try_get(devicePath);
        if (info != nullptr)
        {
          Size = (info->Size) << 10;
          Free = (info->Free) << 10;
        }
      }

      [[nodiscard]] String DisplayableName() const
      {
        String result(PartitionName);
        if (result.empty()) result = DisplayName;
        if (result.empty()) result = Path(DevicePath).Filename();
        if (Size != 0) result.Append(' ').Append(HumanSize());
        if (!FileSystem.empty()) result.Append(" (").Append(FileSystem).Append(')');
        return result;
      }

      [[nodiscard]] String DisplayableShortName() const
      {
        String result(PartitionName);
        if (result.empty()) result = DisplayName;
        if (result.empty()) result = Path(DevicePath).Filename();
        return result;
      }

      [[nodiscard]] DeviceMount ToDeviceMount() const { return DeviceMount(Path(DevicePath), Path(MountPoint), DisplayableShortName(), FileSystem, ReadOnly ? "ro" : ""); }

      bool operator ==(const Device& right) const { return UUID == right.UUID; }

      [[nodiscard]] long long UsedSize()   const { return Size - Free; }
      [[nodiscard]] String   HumanSize()  const { return Sizes(Size).ToHumanSize(); }
      [[nodiscard]] String   HumanFree()  const { return Sizes(Free).ToHumanSize(); }
      [[nodiscard]] String   HumanUsed()  const { return Sizes(UsedSize()).ToHumanSize(); }
      [[nodiscard]] int      PercentFree() const { return Size > 0 ? (int)((double)Free * 100.0 / (double)Size) : 0; }
      [[nodiscard]] int      PercentUsed() const { return Size > 0 ? (int)((double)UsedSize() * 100.0 / (double)Size) : 0; }

      [[nodiscard]] bool RecalboxCompatible() const { return !MountPoint.empty() && (IsNTFS() || FileSystem == "vfat" || FileSystem == "exfat" || FileSystem.StartsWith("ext")); }
      [[nodiscard]] bool IsNTFS() const { return FileSystem == "ntfs"; }
    };

    /*!
     * @brief Get storage device list
     * @return Storage list
     */
    [[nodiscard]] const std::vector<Device>& GetStorageDevices() const { return mStorageDevices; }

    /*!
     * @brief Get share device list
     * @return Storage list
     */
    [[nodiscard]] const std::vector<Device>& GetShareDevices() const { return mShareDevices; }

    /*!
     * @brief Set storage device
     * @param device Device to set as share device
     */
    void SetShareDevice(const Device& device);

    /*!
     * @brief Get share device
     * @return Device selected as share device
     */
    String GetShareDevice();

    //! Refresh devices
    StorageDevices& Refresh() { Initialize(); return *this; }

    /*!
     * @brief Detect the physical media type of a device from its sysfs attributes
     * @param devicePath Full device path (e.g. /dev/sda1) or bare device name
     * @return Detected PhysicalType
     */
    static PhysicalType DetectPhysicalType(const String& devicePath);

    /*!
     * @brief Return a short human-readable label for a PhysicalType
     * @param type Physical media type
     * @return e.g. "HDD", "SSD", "NVMe", "USB", "SD", "eMMC", "Network", "RAM", "?"
     */
    static String PhysicalTypeToString(PhysicalType type);

    /*!
     * @brief Strip the partition suffix from a device name to get the parent device
     * @details Handles both sda1→sda and mmcblk0p1→mmcblk0 / nvme0n1p1→nvme0n1 patterns
     * @param deviceName Bare device name (not a full path)
     * @return Parent device name
     */
    static String StripPartitionSuffix(const String& deviceName);

    //! Constructor
    StorageDevices();

  private:
    //! Share device key
    static constexpr const char* sShareDevice = "sharedevice";
    //! Internal string
    static constexpr const char* sInMemory = "RAM";
    //! Internal string
    static constexpr const char* sInternal = "INTERNAL";
    //! Any external string
    static constexpr const char* sAnyExternal = "ANYEXTERNAL";
    //! Network string
    static constexpr const char* sNetwork = "NETWORK";

    //! Boot configuration file
    IniFile& mBootConfiguration;
    //! All share devices
    std::vector<Device> mShareDevices;
    //! All storage devices, including original share
    std::vector<Device> mStorageDevices;
    //! Boot root device name
    String mBootRoot;
    //! Share in ram?
    bool mShareInRAM;

    //! Initialize all devices
    void Initialize();

    //! Get raw output of the given command
    static String::List GetCommandOutput(const String& command);

    //! Get raw device list from blkid command
    static String::List GetRawDeviceList();

    //! Get mounted device list from mount command
    static String::List GetMountedDeviceList();

    //! Get file system info from df command
    static DeviceSizeInfo GetFileSystemInfo();

    //! Extract properies from the given string
    static PropertyMap ExtractProperties(const String& properties);

    //! Lookup name of a device
    static String LookupName(const String& originalPath);

    //! Analyse mounted devices - Get boot device & check is share is in ram
    void AnalyseMounts(const String::List& mountLines);
};



