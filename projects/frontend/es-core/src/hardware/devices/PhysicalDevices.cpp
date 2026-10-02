//
// Created by bkg2k on 24/04/2026.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "PhysicalDevices.h"
#include <utils/Files.h>

PhysicalDevices::PhysicalDevices()
  : StaticLifeCycleControler<PhysicalDevices>("PhysicalDevs")
{
  Initialize();
}

void PhysicalDevices::Initialize()
{
  mDevices.clear();

  // Read /proc/mounts once: maps device path → mount point
  HashMap<String, String> mountedPaths;
  for(const String& line : Files::LoadFile(Path("/proc/mounts")).Split('\n'))
  {
    String::List items = line.Split(' ', true);
    if (items.size() >= 2 && items[0].StartsWith("/dev/"))
      mountedPaths[items[0]] = items[1];
  }

  for(const Path& entry : Path("/sys/block").GetDirectoryContent(true))
  {
    String name = entry.Filename();
    if (!IsPhysicalDisk(name)) continue;
    if (IsReadOnly(name)) continue;
    long long size = ReadSize(name);
    if (size < sMinimumSizeBytes) continue;
    String displayName = ReadName(name);
    bool hasPartitions = CheckHasPartitions(name);
    bool hasData = hasPartitions && CheckHasDataOnPartitions(name, mountedPaths);
    bool isBoot = hasPartitions && CheckIsBootDevice(name, mountedPaths);
    mDevices.push_back(Device(String("/dev/").Append(name), displayName, size, hasPartitions, hasData, isBoot));
  }
}

bool PhysicalDevices::IsPhysicalDisk(const String& name)
{
  if (name.StartsWith("loop") ||
      name.StartsWith("ram")  ||
      name.StartsWith("zram") ||
      name.StartsWith("dm-")  ||
      name.StartsWith("md"))
    return false;
  return Path(String("/sys/block/").Append(name).Append("/device")).Exists();
}

bool PhysicalDevices::IsReadOnly(const String& name)
{
  String ro = Files::LoadFile(Path(String("/sys/block/").Append(name).Append("/ro"))).Trim();
  return ro == "1";
}

long long PhysicalDevices::ReadSize(const String& name)
{
  String sizeStr = Files::LoadFile(Path(String("/sys/block/").Append(name).Append("/size"))).Trim();
  return sizeStr.AsInt64() * 512LL;
}

String PhysicalDevices::ReadName(const String& name)
{
  Path base(String("/sys/block/").Append(name).Append("/device"));
  String vendor = Files::LoadFile(base / "vendor").Trim();
  String model = Files::LoadFile(base / "model").Trim();
  String result;
  if (!vendor.empty()) result.Append(vendor).Append(' ');
  result.Append(model);
  return result.Trim();
}

bool PhysicalDevices::CheckHasPartitions(const String& name)
{
  for(const Path& sub : Path(String("/sys/block/").Append(name)).GetDirectoryContent(true))
    if (sub.Filename().StartsWith(name))
      return true;
  return false;
}

bool PhysicalDevices::CheckHasDataOnPartitions(const String& name, const HashMap<String, String>& mountedPaths)
{
  for(const Path& sub : Path(String("/sys/block/").Append(name)).GetDirectoryContent(true))
  {
    String partName = sub.Filename();
    if (!partName.StartsWith(name)) continue;

    const String* mountPoint = mountedPaths.try_get(String("/dev/").Append(partName));
    if (mountPoint == nullptr) continue;

    if (!Path(*mountPoint).GetDirectoryContent(true).empty())
      return true;
  }
  return false;
}

bool PhysicalDevices::CheckIsBootDevice(const String& name, const HashMap<String, String>& mountedPaths)
{
  for(const Path& sub : Path(String("/sys/block/").Append(name)).GetDirectoryContent(true))
  {
    String partName = sub.Filename();
    if (!partName.StartsWith(name)) continue;

    const String* mountPoint = mountedPaths.try_get(String("/dev/").Append(partName));
    if (mountPoint != nullptr && *mountPoint == "/boot")
      return true;
  }
  return false;
}
