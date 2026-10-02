//
// Created by bkg2k on 14/01/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include <fcntl.h>
#include "MountMonitor.h"
#include <sys/poll.h>
#include "utils/Files.h"
#include <blkid/blkid.h>

MountMonitor::MountMonitor(IMountMonitorNotifications* interface)
  : mEvent(*this)
  , mShareMountPoint(Path::Empty, Path::Empty, String::Empty, String::Empty, String::Empty)
  , mShareRomsMountPoint(Path::Empty, Path::Empty, String::Empty, String::Empty, String::Empty)
  , mInterface(interface)
  , mState(true)
{
  // Initialize mount points before starting the thread (no lock needed yet)
  mMountPoints = LoadMountPoints(true);

  { LOG(LogInfo) << "[MountMonitor] Mount monitoring starts"; }
  Start("MountMonitor");
}

MountMonitor::~MountMonitor()
{
  Stop();
  { LOG(LogInfo) << "[MountMonitor] Mount monitoring stops"; }
}

void MountMonitor::Run()
{
  int fd = open(sMountPointFile, O_RDONLY);
  if (fd < 0) { LOG(LogError) << "[MountMonitor] Fatal error monitoring mount devices!"; return; }

  while(IsRunning())
  {
    struct pollfd poller { .fd = fd, .events = POLLPRI, .revents = 0 };
    if (poll(&poller, 1, 500) != 1 || (poller.revents & POLLPRI) == 0) continue;

    // Optimistic check first to avoid the I/O of LoadMountPoints when monitoring is off
    if (!mState.load()) continue;

    DeviceMountList newMounts = LoadMountPoints(false);
    bool hasEvents = false;
    {
      Mutex::AutoLock lock(mMutex);
      // Re-check under the lock: SetMonitoringState(false) may have happened during LoadMountPoints,
      // and we must not queue any new event detected by a cycle that was racing with the disable.
      if (!mState.load()) continue;
      std::vector<std::pair<bool, DeviceMount>> events = Process(mMountPoints, newMounts);
      mMountPoints = std::move(newMounts);
      if (!events.empty())
      {
        for(std::pair<bool, DeviceMount>& e : events)
          mPendingEvents.push_back(std::move(e));
        hasEvents = true;
      }
    }
    // Send a single trigger outside the lock so ReceiveSyncMessage can acquire it
    if (hasEvents) mEvent.Send(true);
  }

  close(fd);
}

void MountMonitor::SetMonitoringState(bool state)
{
  // Take the mutex as a barrier with both Run() (event detection) and ReceiveSyncMessage (dispatch).
  // When disabling, also discard any event that has been detected but not yet dispatched so no
  // notification can fire after this call returns.
  Mutex::AutoLock lock(mMutex);
  mState.store(state);
  if (!state) mPendingEvents.clear();
}

MountMonitor::DeviceMountList MountMonitor::MountPoints()
{
  Mutex::AutoLock lock(mMutex);
  return mMountPoints;
}

MountMonitor::DeviceMountList MountMonitor::LoadMountPoints(bool initializeSpecialMountPoints)
{
  static constexpr const int sDeviceIndex = 0;
  static constexpr const int sMountPointIndex = 1;
  static constexpr const int sTypeIndex = 2;
  static constexpr const int sOptionsIndex = 3;

  { LOG(LogDebug) << "[MountMonitor] Loading available mount points"; }
  DeviceMountList result;

  // Get all valid mount point
  for(const String& line : Files::LoadFile(Path(sMountPointFile)).Split('\n')) // For every entry
    if (!line.empty())
    {
      String::List items = line.Split(' ');
      if (items.size() < 4) continue; // malformed line guard

      const Path device(items[sDeviceIndex]);
      const Path mountPoint(items[sMountPointIndex]);
      const String& type = items[sTypeIndex];
      const String& options = items[sOptionsIndex];

      // Physical USB devices
      if (device.StartWidth(String(LEGACY_STRING("/dev/")))) // starting with /dev/
      {
        #ifndef DEBUG
        if (mountPoint.StartWidth(sRecalboxRootMountPoint))    // is it valid?
        #endif
        result.push_back(DeviceMount(device, mountPoint, GetPartitionLabel(device), type, options)); // so store it in the list
        { LOG(LogDebug) << "[MountMonitor] Candidate: " << device.ToString() << " mounted to " << mountPoint.ToString() << " (" << GetPartitionLabel(device) << ')';  }
      }
      // Network?
      if (type == "cifs" || type.StartsWith(LEGACY_STRING("nfs")))
        //#ifndef DEBUG
        if (mountPoint.StartWidth(sRecalboxRootMountPoint)) // is it valid?
        //#endif
        {
          result.push_back(DeviceMount(device, mountPoint, "Network", type, options)); // so store it in the list
          { LOG(LogDebug) << "[MountMonitor] Candidate: " << device.ToString() << " mounted to " << mountPoint.ToString() << " (" << GetPartitionLabel(device) << ')';  }
        }

      // Initialize special mount points
      if (initializeSpecialMountPoints)
      {
        if (mountPoint == sSharePath)
        {
          mShareMountPoint = DeviceMount(device, mountPoint, GetPartitionLabel(device), type, options);
          { LOG(LogDebug) << "[MountMonitor] Share mount point found on device: " << mShareMountPoint.Device().ToString(); }
        }
        else if (mountPoint == sShareRomsPath)
        {
          mShareRomsMountPoint = DeviceMount(device, mountPoint, GetPartitionLabel(device), type, options);
          { LOG(LogDebug) << "[MountMonitor] Roms mount point found on device: " << mShareRomsMountPoint.Device().ToString(); }
        }
      }
    }

  // Seek & destroy any Mount device that match the share device
  for(int i = (int)result.size(); --i >= 0;)
    if (result[i].Device() == mShareMountPoint.Device())
      result.erase(result.begin() + i);

  // Final result
  return result;
}

String MountMonitor::GetPartitionLabel(const Path& devicePath)
{
  blkid_probe pr = blkid_new_probe_from_filename(devicePath.ToChars());
  if (pr == nullptr)
  {
    { LOG(LogError) << "[MountMonitor] Can't get BLKID structure from " << devicePath.ToString(); }
    return "UNKNOWN";
  }
  const char* string = nullptr;
  blkid_do_probe(pr);
  blkid_probe_lookup_value(pr, "LABEL", &string, nullptr);
  if (string == nullptr)
  {
    { LOG(LogWarning) << "[MountMonitor] Can't get LABEL from " << devicePath.ToString(); }
    blkid_probe_lookup_value(pr, "UUID", &string, nullptr);
    if (string == nullptr)
    {
      { LOG(LogWarning) << "[MountMonitor] Can't get UUID from " << devicePath.ToString(); }
      string = "UNKNOWN";
    }
  }
  String result(string); // Store result before destroying blkid structures!
  blkid_free_probe(pr);
  { LOG(LogDebug) << "[MountMonitor] " << devicePath.ToString() << " name is: " << result; }
  return result;
}

void MountMonitor::ReceiveSyncMessage(bool)
{
  // Drain the pending event queue under lock, then dispatch without holding the lock.
  // If monitoring has been disabled in between Send() and now, discard everything.
  std::vector<std::pair<bool, DeviceMount>> events;
  {
    Mutex::AutoLock lock(mMutex);
    if (!mState.load()) { mPendingEvents.clear(); return; }
    events = std::move(mPendingEvents);
  }
  if (mInterface != nullptr)
    for(const std::pair<bool, DeviceMount>& event : events)
    {
      if (event.first) mInterface->NotifyDeviceMount(event.second);
      else mInterface->NotifyDeviceUnmount(event.second);
    }
}

std::vector<std::pair<bool, DeviceMount>> MountMonitor::Process(DeviceMountList& oldList, const DeviceMountList& newListOriginal)
{
  std::vector<std::pair<bool, DeviceMount>> events;
  DeviceMountList newList = newListOriginal;
  // Kill duplicates in both lists — remaining items are the actual changes
  for(int i = (int)oldList.size(); --i >= 0;)
    for(int j = (int)newList.size(); --j >= 0;)
      if (oldList[i] == newList[j])
      {
        oldList.erase(oldList.begin() + i);
        newList.erase(newList.begin() + j);
        break;
      }
  // Remaining points in old list are removed mount points
  for(const DeviceMount& mp : oldList)
  {
    { LOG(LogInfo) << "[MountMonitor] Mount point removed: " << mp.MountPoint().ToString(); }
    events.push_back({false, mp});
  }
  // Remaining points in new list are added mount points
  for(const DeviceMount& mp : newList)
  {
    { LOG(LogInfo) << "[MountMonitor] Mount point added: " << mp.MountPoint().ToString(); }
    events.push_back({true, mp});
  }
  return events;
}

std::optional<DeviceMount> MountMonitor::SizeOf(const Path& path)
{
  Mutex::AutoLock lock(mMutex);
  // In the external mount point list?
  for(DeviceMount& device : mMountPoints)
    if (!device.MountPoint().IsEmpty())
      if (path.StartWidth(device.MountPoint()))
        return device.UpdateSize();
  // Share/roms?
  if (!mShareRomsMountPoint.MountPoint().IsEmpty())
    if (path.StartWidth(mShareRomsMountPoint.MountPoint()))
      return mShareRomsMountPoint.UpdateSize();
  // Share?
  if (!mShareMountPoint.MountPoint().IsEmpty())
    if (path.StartWidth(mShareMountPoint.MountPoint()))
      return mShareMountPoint.UpdateSize();
  return std::nullopt;
}

MountMonitor::DeviceMountList MountMonitor::AllMountPoints()
{
  Mutex::AutoLock lock(mMutex);
  DeviceMountList result;
  result.push_back(mShareMountPoint.UpdateSize());
  if (!mShareRomsMountPoint.MountPoint().IsEmpty())
    result.push_back(mShareRomsMountPoint.UpdateSize());
  for(DeviceMount& device : mMountPoints)
    result.push_back(device.UpdateSize());
  return result;
}
