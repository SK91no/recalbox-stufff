//
// Recalbox Gamepad Proxy (R.G.P.) frontend wrapper.
//
#include "RGPProxy.h"

#include <SDL2/SDL_joystick.h>
#include <unistd.h>

#include "CrtConf.h"
#include "RecalboxConf.h"
#include "utils/Files.h"

#define RGP_SYSFS(name) (Path("/sys/class/recalbox_gamepad_proxy/" name))

RGPProxy& RGPProxy::Instance()
{
  static RGPProxy instance;
  return instance;
}

bool RGPProxy::IsAvailable() const
{
  return Path("/sys/class/recalbox_gamepad_proxy").Exists();
}

String RGPProxy::Version() const
{
  return Files::LoadFile(RGP_SYSFS("version")).ToTrim();
}

bool RGPProxy::IsProxified(int sdlJoystickIndex)
{
  const char* devpath = SDL_JoystickPathForIndex(sdlJoystickIndex);
  if (!devpath) return false;

  String pathStr(devpath);
  size_t lastSlash = pathStr.rfind('/');
  if (lastSlash == String::npos) return false;

  String eventName = pathStr.SubString(lastSlash);
  String sysPath = String("/sys/class/input").Append(eventName).Append("/device/rgp_proxied");

  return access(sysPath.c_str(), F_OK) == 0;
}

unsigned int RGPProxy::GetUid(int sdlJoystickIndex)
{
  const char* devpath = SDL_JoystickPathForIndex(sdlJoystickIndex);
  if (!devpath) return 0;

  String pathStr(devpath);
  size_t lastSlash = pathStr.rfind('/');
  if (lastSlash == String::npos) return 0;

  String eventName = pathStr.SubString(lastSlash);
  Path sysPath(String("/sys/class/input").Append(eventName).Append("/device/rgp_uid"));
  if (!sysPath.Exists()) return 0;

  return (unsigned int) Files::LoadFile(sysPath).ToTrim().AsInt();
}

bool RGPProxy::IsProxyForUid(int sdlJoystickIndex, unsigned int uid)
{
  if (uid == 0) return false;
  const char* name = SDL_JoystickNameForIndex(sdlJoystickIndex);
  if (!name) return false;
  if (!String(name).EndsWith("(R.G.P.)")) return false;
  return GetUid(sdlJoystickIndex) == uid;
}

int RGPProxy::FindProxyForUid(unsigned int uid)
{
  if (uid == 0) return -1;
  int count = SDL_NumJoysticks();
  for (int i = 0; i < count; ++i)
    if (IsProxyForUid(i, uid)) return i;
  return -1;
}

String::List RGPProxy::GetDeviceFilters() const
{
  String::List clean;
  for (const String& f : RecalboxConf::Instance().GetGamepadProxyFilters())
  {
    String t = f.ToTrim();
    if (!t.empty()) clean.push_back(t);
  }
  return clean;
}

RGPProxy& RGPProxy::SetDeviceFilters(const String::List& filters)
{
  String::List clean;
  for (const String& f : filters)
  {
    String t = f.ToTrim();
    if (!t.empty()) clean.push_back(t);
  }
  RecalboxConf::Instance().SetGamepadProxyFilters(clean).Save();
  PushDeviceFilters(clean);
  return *this;
}

bool RGPProxy::AddDeviceFilter(const String& controller)
{
  String name = controller.ToTrim();
  if (name.empty()) return false;

  String::List current = GetDeviceFilters();
  for (const String& f : current)
    if (f == name) return false;

  current.push_back(name);
  RecalboxConf::Instance().SetGamepadProxyFilters(current).Save();
  PushDeviceFilters(current);
  return true;
}

bool RGPProxy::RemoveDeviceFilter(const String& controller)
{
  String name = controller.ToTrim();
  if (name.empty()) return false;

  String::List current = GetDeviceFilters();
  String::List next;
  bool removed = false;
  for (const String& f : current)
  {
    if (!removed && f == name) { removed = true; continue; }
    next.push_back(f);
  }
  if (!removed) return false;

  RecalboxConf::Instance().SetGamepadProxyFilters(next).Save();
  PushDeviceFilters(next);
  return true;
}

RGPProxy& RGPProxy::ClearDeviceFilters()
{
  RecalboxConf::Instance().SetGamepadProxyFilters({}).Save();
  PushDeviceFilters({});
  return *this;
}

RGPProxy& RGPProxy::Apply()
{
  PushToggles();
  PushDeviceFilters(GetDeviceFilters());
  return *this;
}

void RGPProxy::PushDeviceFilters(const String::List& filters) const
{
  if (!RGP_SYSFS("device_filter").Exists()) return;
  Files::WriteToFile(RGP_SYSFS("device_filter"), String::Join(filters, ','), 0);
}

RGPProxy::ExitCombosState RGPProxy::DisableExitCombos()
{
  ExitCombosState state;
  Path thumb = RGP_SYSFS("enable_thumbstick_exit");
  Path lp    = RGP_SYSFS("enable_longpress_exit");
  // If the module isn't loaded the sysfs files don't exist — leave state.valid
  // at false so RestoreExitCombos() short-circuits to a no-op.
  if (!thumb.Exists() || !lp.Exists()) return state;
  state.thumbstickExit = Files::LoadFile(thumb).ToTrim() == "1";
  state.longpressExit  = Files::LoadFile(lp).ToTrim()    == "1";
  Files::WriteToFile(thumb, "0", 0);
  Files::WriteToFile(lp,    "0", 0);
  state.valid = true;
  return state;
}

void RGPProxy::RestoreExitCombos(const ExitCombosState& state)
{
  if (!state.valid) return;
  Path thumb = RGP_SYSFS("enable_thumbstick_exit");
  Path lp    = RGP_SYSFS("enable_longpress_exit");
  if (thumb.Exists()) Files::WriteToFile(thumb, state.thumbstickExit ? "1" : "0", 0);
  if (lp.Exists())    Files::WriteToFile(lp,    state.longpressExit  ? "1" : "0", 0);
}

void RGPProxy::PushToggles() const
{
  const CrtConf& c = CrtConf::Instance();
  if (RGP_SYSFS("enable_shift_credit").Exists())
    Files::WriteToFile(RGP_SYSFS("enable_shift_credit"),
                       c.GetSystemCRTJammaStartBtn1Credit() ? "1" : "0", 0);
  if (RGP_SYSFS("enable_shift_mode_passthrough").Exists())
    Files::WriteToFile(RGP_SYSFS("enable_shift_mode_passthrough"),
                       c.GetSystemCRTJammaHKOnStart() ? "1" : "0", 0);
  if (RGP_SYSFS("enable_shift_volume").Exists())
    Files::WriteToFile(RGP_SYSFS("enable_shift_volume"),
                       c.GetSystemCRTJammaSoundOnStart() ? "1" : "0", 0);
  if (RGP_SYSFS("enable_longpress_exit").Exists())
    Files::WriteToFile(RGP_SYSFS("enable_longpress_exit"),
                       c.GetSystemCRTJammaExitOnStart() ? "1" : "0", 0);
}
