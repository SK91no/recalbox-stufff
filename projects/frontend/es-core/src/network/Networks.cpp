//
// Created by bkg2k on 13/12/24.
//

#include "Networks.h"
#include "utils/storage/Set.h"
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <algorithm>
#include <arpa/nameser.h>
#include <resolv.h>

Networks::Networks()
  : StaticLifeCycleControler<Networks>("WifiScanner")
  , mScan(false)
  , mSender(*this)
{
  CollectWLANInterfaces();
  Thread::Start("Wifi");
}

void Networks::Run()
{
  bool firstTime = true;
  while(IsRunning())
  {
    if (firstTime || mScan)
    {
      { LOG(LogInfo) << "Start scanning WIFI hotspot"; }
      StartScan();      if (!IsRunning()) break;
      Wait(5000);       if (!IsRunning()) break;
      CollectResults(); if (!IsRunning()) break;
      StopScan();       if (!IsRunning()) break;
      mSender.Send();
      firstTime = false;
      { Mutex::AutoLock lock(mGuardian); LOG(LogInfo) << "[Wifi] Found " << mHotSpotList.size() << " Hotspots"; }
    }
    // Sleep a bit when scanning, or stop and wait for signal
    if (mScan) Wait(sSleepTime);
    else mSignal.WaitSignal();
  }
}

String::List Networks::Execute(const String& command)
{
  String output;
  char buffer[1024];
  FILE* pipe = popen(command.c_str(), "r");
  if (pipe != nullptr)
  {
    while (feof(pipe) == 0)
      if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        output.Append(buffer);
    pclose(pipe);
  }
  return output.Split('\n');
}

Networks::SignalStrength Networks::ConvertLevel(const String& sLevel)
{
  int l = sLevel.AsInt();
  if (l == 0) return SignalStrength::Unknown;
  if (l < 0)
  {
    if (l <= -95) return SignalStrength::Unusable;
    if (l <= -75) return SignalStrength::Bad;
    if (l <= -67) return SignalStrength::Average;
    if (l <= -50) return SignalStrength::Good;
    return SignalStrength::VeryGood;
  }
  if (l >= 70) return SignalStrength::VeryGood;
  if (l >= 50) return SignalStrength::Good;
  if (l >= 35) return SignalStrength::Average;
  if (l >= 20) return SignalStrength::Bad;
  return SignalStrength::Unusable;
}

void Networks::ReceiveSyncMessage()
{
  for(IWifiHotspotListChanged* interface : mNotifications)
    interface->WifiHotspotListChanged(*this);
}

void Networks::CollectWLANInterfaces()
{
  Path::PathList list = Path("/var/run/wpa_supplicant").GetDirectoryContent(false);
  if (list.empty()) list = Path("/var/run/wpa_supplicant_global").GetDirectoryContent(false);

  mWlanInterfaces.clear();
  for(const Path& path : list)
  {
    mWlanInterfaces.push_back(path.Filename());
    { LOG(LogInfo) << "[Networks] WIFI Interface: " << path.Filename(); }
  }
}

void Networks::StartScan()
{
  CollectWLANInterfaces();
  for(const String& interface : mWlanInterfaces)
    (void)Execute(String("wpa_cli -i ").Append(interface).Append(" scan"));
}

void Networks::StopScan()
{
  for(const String& interface : mWlanInterfaces)
    (void)Execute(String("wpa_cli -i ").Append(interface).Append(" abort_scan"));
}

void Networks::CollectResults()
{
  HashSet<String> HotspotNames;
  std::vector<HotSpot> newList;
  for(const String& interface : mWlanInterfaces)
  {
    String::List list = Execute(String("wpa_cli -i ").Append(interface).Append(" scan_result"));
    if (!list.empty()) list.erase(list.begin());
    for (String line: list)
      if (String::List parts = line.Trim().Replace('\t', ' ').Split(' ', 5, true); parts.size() >= 5 && !HotspotNames.contains(parts[4]))
      {
        newList.push_back(HotSpot(parts[4], ConvertLevel(parts[2]), parts[3] != "[ESS]", parts[1].AsInt() >= 5000));
        HotspotNames.insert(parts[4]);
      }
  }
  std::ranges::sort(newList, [] (const HotSpot& a, const HotSpot& b) -> bool { if (a.Level() != b.Level()) return a.Level() > b.Level(); return a.Name() < b.Name(); });
  mGuardian.Lock();
  mHotSpotList = std::move(newList);
  mGuardian.UnLock();
}

String Networks::FormatHotspot(const HotSpot& hotspot)
{
  String result;
  switch(hotspot.Level())
  {
    case Networks::SignalStrength::Unusable: result.AssignUTF8(0xF1C5); break;
    case Networks::SignalStrength::Bad: result.AssignUTF8(0xF1C6); break;
    case Networks::SignalStrength::Average: result.AssignUTF8(0xF1C7); break;
    case Networks::SignalStrength::Good:
    case Networks::SignalStrength::VeryGood: result.AssignUTF8(0xF1C8); break;
    case Networks::SignalStrength::Unknown:
    default: result.AssignUTF8(0xF1C9); break;
  }
  result.Append(' ').Append(hotspot.Name()).Append(' ').AppendUTF8(hotspot.Secure() ? 0xF1CB : 0xF1CC);
  if (hotspot.FiveGhz()) result.Append(' ').AppendUTF8(0xF1CD);

  return result;
}

void Networks::Wait(int ms)
{
  for(int i = ms / 20; --i >= 0 && IsRunning(); ) Thread::Sleep(20);
}

String Networks::IP(Networks::IPVersion ipVersion, Networks::Interfaces interfaces)
{
  String result;
  struct ifaddrs* ifAddrStruct = nullptr;
  getifaddrs(&ifAddrStruct);

  if (hasFlag(ipVersion, IPVersion::IPv4))
    for (struct ifaddrs* ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
      if (ifa->ifa_addr != nullptr && ifa->ifa_addr->sa_family == AF_INET)
      {
        char ipAsString[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &((struct sockaddr_in*) ifa->ifa_addr)->sin_addr, ipAsString, INET_ADDRSTRLEN);
        if (!IsLoopbackIPv4(ipAsString))
          if ((hasFlag(interfaces, Interfaces::Ethernet) && IsWiredInterface(*ifa)) ||
              (hasFlag(interfaces, Interfaces::Wifi) && IsWifiInterface(*ifa)))
          {
            { LOG(LogDebug) << "[Network] Interface " << ifa->ifa_name << " has IPv4 address " << ipAsString; }
            result = ipAsString;
            break;
          }
      }

  if (result.empty() && hasFlag(ipVersion, IPVersion::IPv6))
    for (struct ifaddrs* ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
      if (ifa->ifa_addr != nullptr && ifa->ifa_addr->sa_family == AF_INET6)
      {
        char ipAsString[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &((struct sockaddr_in6*) ifa->ifa_addr)->sin6_addr, ipAsString, INET6_ADDRSTRLEN);
        if (!IsLoopbackIPv6(ipAsString))
          if ((hasFlag(interfaces, Interfaces::Ethernet) && IsWiredInterface(*ifa)) ||
              (hasFlag(interfaces, Interfaces::Wifi) && IsWifiInterface(*ifa)))
          {
            { LOG(LogDebug) << "[Network] Interface " << ifa->ifa_name << " has IPv6 address " << ipAsString; }
            result = ipAsString;
            break;
          }
      }

  if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
  return result;
}

bool Networks::HasIP(Networks::IPVersion ipVersion, Networks::Interfaces interfaces)
{
  bool result = false;
  struct ifaddrs* ifAddrStruct = nullptr;
  getifaddrs(&ifAddrStruct);

  if (hasFlag(ipVersion, IPVersion::IPv4))
    for (struct ifaddrs* ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
      if (ifa->ifa_addr != nullptr && ifa->ifa_addr->sa_family == AF_INET)
      {
        char ipAsString[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &((struct sockaddr_in*) ifa->ifa_addr)->sin_addr, ipAsString, INET_ADDRSTRLEN);
        if (!IsLoopbackIPv4(ipAsString))
          if ((hasFlag(interfaces, Interfaces::Ethernet) && IsWiredInterface(*ifa)) ||
              (hasFlag(interfaces, Interfaces::Wifi) && IsWifiInterface(*ifa)))
          {
            { LOG(LogDebug) << "[Network] Interface " << ifa->ifa_name << " has IPv4 address " << ipAsString; }
            result = true;
            break;
          }
      }

  if (!result && hasFlag(ipVersion, IPVersion::IPv6))
    for (struct ifaddrs* ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
      if (ifa->ifa_addr == nullptr && ifa->ifa_addr->sa_family == AF_INET6)
      {
        char ipAsString[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &((struct sockaddr_in6*) ifa->ifa_addr)->sin6_addr, ipAsString, INET6_ADDRSTRLEN);
        if (!IsLoopbackIPv6(ipAsString))
          if ((hasFlag(interfaces, Interfaces::Ethernet) && IsWiredInterface(*ifa)) ||
              (hasFlag(interfaces, Interfaces::Wifi) && IsWifiInterface(*ifa)))
          {
            { LOG(LogDebug) << "[Network] Interface " << ifa->ifa_name << " has IPv6 address " << ipAsString; }
            result = true;
            break;
          }
      }

  if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
  return result;
}

bool Networks::IsLoopbackIPv4(const String& ip)
{
  if (ip.StartsWith("127.")) return true;
  return false;
}

bool Networks::IsLoopbackIPv6(const String& ip)
{
  if (ip == "::1") return true;
  return false;
}

bool Networks::IsLocalIPv4(const String& ip)
{
  if (IsLoopbackIPv4(ip)) return true;
  if (ip.StartsWith("169.254.")) return true;
  if (ip.StartsWith("10.")) return true;
  if (ip.StartsWith("192.168.")) return true;
  return false;
}

bool Networks::IsLocalIPv6(const String& ip)
{
  if (IsLoopbackIPv4(ip)) return true;
  if (ip.StartsWith("fe80:")) return true;
  if (ip.StartsWith("fd")) return true;
  return false;
}

bool Networks::IsWifiInterface(ifaddrs& addr)
{
  String name(addr.ifa_name);
  if (name.StartsWith("docker")) return false;
  for(const String& interface : mWlanInterfaces)
    if (name == interface)
      return true;
  return false;
}

bool Networks::IsWiredInterface(ifaddrs& addr)
{
  String name(addr.ifa_name);
  if (name == "lo") return false;
  if (name.StartsWith("docker")) return false;
  for(const String& interface : mWlanInterfaces)
    if (name == interface)
      return false;
  return true;
}

String Networks::QueryDNSRecord(const String& subDomain)
{
  unsigned char buffer[4096];
  int l = res_query(subDomain.c_str(), C_IN, ns_t_txt, buffer, sizeof (buffer));
  if (l >= 0)
  {
    ns_msg msg;
    ns_initparse(buffer, l, &msg);
    ns_rr rr;
    if (ns_parserr(&msg, ns_s_an, 0, &rr) == 0)
    {
      String result = String((char*) (ns_rr_rdata(rr) + 1), (int)ns_rr_rdata(rr)[0]);
      { LOG(LogDebug) << "[Update] Domain: " << result; }
      return result;
    }
  }
  { LOG(LogError) << "[Update] No result for Domain " << subDomain; }
  return String::Empty;
}
