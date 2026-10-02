//
// Created by bkg2k on 21/02/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "utils/locale/LocaleHelper.h"
#include "hardware/Board.h"
#include "RootFolders.h"
#include "StorageDevices.h"
#include "utils/Files.h"
#include <recalbox/BootConf.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/hdreg.h>

StorageDevices::StorageDevices()
  : StaticLifeCycleControler<StorageDevices>("StorageDevs")
  , mBootConfiguration(BootConf::Instance())
  , mShareInRAM(false)
{
  Initialize();
}

void StorageDevices::Initialize()
{
  mShareDevices.clear();
  mStorageDevices.clear();
  mBootRoot.clear();
  mShareInRAM = false;

  String::List mountLines = GetMountedDeviceList();
  AnalyseMounts(mountLines);
  String current = GetShareDevice();

  // Get storage sizes
  DeviceSizeInfo sizeInfos = GetFileSystemInfo();

  // Link to mount point
  HashMap<String, String> mountPointLinks;
  HashMap<String, bool> readOnlyLinks;        // keyed by device path
  HashMap<String, bool> mountPointReadOnly;   // keyed by mount point (for shares with no real device path)
  for(const String& line : mountLines)
  {
    String::List chunks = line.Split(' ', true);
    if (chunks.size() >= 3)
    {
      mountPointLinks[chunks[0]] = chunks[2];
      readOnlyLinks[chunks[0]] = false;
      mountPointReadOnly[chunks[2]] = false;
      if (chunks.size() >= 6)
        for(const String& option : chunks[5].ToTrim("()").Split(','))
          if (option == "ro")
          {
            readOnlyLinks[chunks[0]] = true;
            mountPointReadOnly[chunks[2]] = true;
          }
    }
  }

  // Ram?
  if (mShareInRAM)
  {
    current = "RAM";
    mShareDevices.push_back(Device(Types::Ram, "SHARE", "/recalbox/share", sInMemory, "RECALBOX", "tmpfs", _("In Memory!") + " \u26a0", true, sizeInfos, mountPointReadOnly["/recalbox/share"]));
    mShareDevices.back().MediaType = PhysicalType::RAM;
    { LOG(LogWarning) << "[Storage] Share is stored in memory!"; }
  }

  // Add Internal
  mShareDevices.push_back(Device(Types::Internal, "SHARE", "/recalbox/share", sInternal, "INTERNAL", "exfat", _("Internal Share Partition"), current == sInternal, sizeInfos, mountPointReadOnly["/recalbox/share"]));
  mShareDevices.back().MediaType = DetectPhysicalType(mBootRoot);

  // Network
  if (mBootConfiguration.HasKeyStartingWith("sharenetwork_") || mBootConfiguration.AsString("sharedevice") == "NETWORK")
  {
    mShareDevices.push_back(Device(Types::Network, "", "/recalbox/share", sNetwork, "NETWORK", "", _("Network Share"), current == sNetwork, sizeInfos, mountPointReadOnly["/recalbox/share"]));
    mShareDevices.back().MediaType = PhysicalType::Network;
    { LOG(LogDebug) << "[Storage] Network share configuration detected"; }
  }

  // Go advance does not support external storage
  if ((Board::Instance().GetBoardType() == BoardType::OdroidAdvanceGo) ||
      (Board::Instance().GetBoardType() == BoardType::OdroidAdvanceGoSuper) ||
      (Board::Instance().GetBoardType() == BoardType::RG351P)) return;

  // Any external
  if (mBootConfiguration.AsString("sharedevice") == "ANYEXTERNAL") {
    mShareDevices.push_back(Device(Types::External, "", "/recalbox/share", sAnyExternal, "", "", _("Any External Device").Append(" (").Append(_("Deprecated")).Append(')'), current == sAnyExternal, sizeInfos, mountPointReadOnly["/recalbox/share"]));
    { LOG(LogDebug) << "[Storage] Any external share configuration detected"; }
  }

  // External Devices
  String devicePath;
  String propertyLine;
  for(const String& line : GetRawDeviceList())
    if (line.Extract(':', devicePath, propertyLine, true))
    {
      PropertyMap properties = ExtractProperties(propertyLine);
      String uuid = "DEV " + properties.get_or_return_default("UUID");
      String label = properties.get_or_return_default("LABEL");
      String filesystem = properties.get_or_return_default("TYPE");
      if (filesystem.empty()) filesystem = "fs?";
      String displayable = LookupName(devicePath);
      String mountPoint;
      if (String* mp = mountPointLinks.try_get(devicePath); mp != nullptr) mountPoint = *mp;

      // --------- Storage device
      if (!devicePath.StartsWith(mBootRoot) || label == "SHARE")
      {
        if (devicePath.StartsWith(mBootRoot) && label == "SHARE") displayable = "ORIGINAL SHARE";
        // Store
        mStorageDevices.push_back(Device(Types::External, devicePath, mountPoint, uuid, label, filesystem, displayable, current == uuid, sizeInfos, readOnlyLinks[devicePath]));
        mStorageDevices.back().MediaType = DetectPhysicalType(devicePath);
        { LOG(LogDebug) << "[Storage] External storage: " << devicePath << ' ' << uuid << " \"" << label << "\" " << filesystem; }
      }

      // --------- Share devices
      if (!devicePath.StartsWith(mBootRoot))
      {
        // Extract device properties
        if (displayable.empty())
        {
          displayable = _("Device %d - %l (%f)");
          displayable.Replace("%d", Path(devicePath).Filename()).Replace("%l", label).Replace("%f", filesystem);
        }
        // Store
        mShareDevices.push_back(Device(Types::External, devicePath, mountPoint, uuid, label, filesystem, displayable, current == uuid, sizeInfos, readOnlyLinks[devicePath]));
        mShareDevices.back().MediaType = DetectPhysicalType(devicePath);
        { LOG(LogDebug) << "[Storage] External share: " << devicePath << ' ' << uuid << " \"" << label << "\" " << filesystem; }
      }
    }
}

String::List StorageDevices::GetCommandOutput(const String& command)
{
  String output;
  char buffer[4096];
  FILE* pipe = popen(command.data(), "r");
  if (pipe != nullptr)
  {
    while (feof(pipe) == 0)
      if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        output.Append(buffer);
    pclose(pipe);
  }
  return output.Split('\n');
}

String::List StorageDevices::GetRawDeviceList()
{
  return GetCommandOutput("blkid");
}

String::List StorageDevices::GetMountedDeviceList()
{
  return GetCommandOutput("mount");
}

StorageDevices::DeviceSizeInfo StorageDevices::GetFileSystemInfo()
{
  DeviceSizeInfo result;
  for(const String& line : GetCommandOutput("df -kP"))
  {
    String::List items = line.Split(' ', true);
    if (items.size() >= 6)
    {
      long long size = items[1].AsInt64();
      long long free = items[3].AsInt64();
      result[items[0]] = SizeInfo(size, free);
      // Special cases
      if (items[5] == RootFolders::DataRootFolder.ToString())
        result["SHARE"] = SizeInfo(size, free);
    }
  }
  return result;
}

StorageDevices::PropertyMap StorageDevices::ExtractProperties(const String& properties)
{
  PropertyMap map;

  String key;
  String value;
  for(const String& kv : properties.SplitQuoted(' '))
    if (kv.Extract('=', key, value, true))
      map[key] = value.Trim('"');

  return map;
}

void StorageDevices::SetShareDevice(const Device& device)
{
  mBootConfiguration.SetString(sShareDevice, device.UUID);
  mBootConfiguration.Save();
}

String StorageDevices::GetShareDevice()
{
  return mBootConfiguration.AsString(sShareDevice, sInternal);
}

void StorageDevices::AnalyseMounts(const String::List& mountLines)
{
  mBootRoot = "/dev/bootdevice";
  for(const String& line : mountLines)
  {
    String::List items = line.Split(' ', true);
    if (items.size() < 6)
    {
      { LOG(LogError) << "[Storage] Incomplete mount line: " << line; }
      continue;
    }
    if (items[2] == "/recalbox/share") mShareInRAM =  (items[4] == "tmpfs");
    else if (items[2] == "/boot") mBootRoot = items[0].Trim("0123456789");
  }
  if (mBootRoot.empty()) mBootRoot = "/dev/boot"; // for testing purpose only :)
  { LOG(LogDebug) << "[Storage] BootRoot: " << mBootRoot << " - Is In Ram: " << mShareInRAM; }
}

String StorageDevices::StripPartitionSuffix(const String& deviceName)
{
  String result(deviceName);
  int last = result.Count() - 1;

  // Find the first trailing digit
  int l = last;
  while (l > 0 && result[l] >= '0' && result[l] <= '9') --l;

  if (l == last) return result; // no trailing digits — no partition suffix

  // mmcblk0p1 / nvme0n1p1 style: 'p' preceded by a digit → strip 'pN...'
  if (l > 0 && result[l] == 'p' && result[l - 1] >= '0' && result[l - 1] <= '9')
    return result.Delete(l, result.Count() - l);

  // sdaN style: just strip trailing digits
  return result.Delete(l + 1, result.Count() - l - 1);
}

StorageDevices::PhysicalType StorageDevices::DetectPhysicalType(const String& devicePath)
{
  String device = StripPartitionSuffix(Path(devicePath).Filename());
  Path sysBase(String("/sys/block/").Append(device));

  // NVMe SSD
  if (device.StartsWith("nvme")) return PhysicalType::NVMe;

  // MMC family: eMMC vs SD card
  if (device.StartsWith("mmcblk"))
  {
    String type = Files::LoadFile(sysBase / "device" / "type").Trim();
    return (type == "MMC") ? PhysicalType::eMMC : PhysicalType::SD;
  }

  // Removable block device → USB mass storage
  String removable = Files::LoadFile(sysBase / "removable").Trim();
  if (removable == "1") return PhysicalType::USB;

  // Rotational flag: 1 = HDD, 0 = SSD
  String rot = Files::LoadFile(sysBase / "queue" / "rotational").Trim();
  if (rot == "1") return PhysicalType::HDD;
  if (rot == "0") return PhysicalType::SSD;

  return PhysicalType::Unknown;
}

String StorageDevices::PhysicalTypeToString(PhysicalType type)
{
  switch (type)
  {
    case PhysicalType::HDD:     return "HDD";
    case PhysicalType::SSD:     return "SSD";
    case PhysicalType::NVMe:    return "NVMe";
    case PhysicalType::USB:     return "USB";
    case PhysicalType::SD:      return "SD";
    case PhysicalType::eMMC:    return "eMMC";
    case PhysicalType::Network: return "Network";
    case PhysicalType::RAM:     return "RAM";
    case PhysicalType::Unknown:
    default:                    return "?";
  }
}

String StorageDevices::LookupName(const String& originalPath)
{
  String partName = Path(originalPath).Filename();
  String device   = StripPartitionSuffix(partName);

  Path base = Path(String("/sys/block/").Append(device).Append("/device"));
  String result = Files::LoadFile(base / "vendor").Trim();
  if (!result.empty()) result.Append(' ');
  result.Append(Files::LoadFile(base / "model").Trim());
  if (!result.empty()) result.Append(' ');
  result.Append(Files::LoadFile(base / "name").Trim());

  // Append the partition suffix when the result has a device label
  if (!result.Trim().empty() && partName != device)
    result.Append(", part. ").Append(String(partName).Delete(0, device.Count()));

  return result;
}
