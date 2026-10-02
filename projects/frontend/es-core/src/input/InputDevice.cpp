#include "InputDevice.h"
#include "InputEvent.h"
#include "utils/udev/UDevDevice.h"
#include "utils/udev/UDev.h"
#include "utils/udev/UDevEnumerate.h"
#include <utils/String.h>
#include <SDL2/SDL.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <utils/Files.h>

#include "hardware/Board.h"
#include "hardware/RGPProxy.h"

unsigned int InputDevice::mReferenceTimes[32];

String InputDevice::EntryToString(InputDevice::Entry entry)
{
  switch(entry)
  {
    case InputDevice::Entry::None: return "none";
    case InputDevice::Entry::Start: return "start";
    case InputDevice::Entry::Select: return "select";
    case InputDevice::Entry::Hotkey: return "hotkey";
    case InputDevice::Entry::East: return "east";
    case InputDevice::Entry::South: return "south";
    case InputDevice::Entry::EastPrime: return "eastprime";
    case InputDevice::Entry::North: return "north";
    case InputDevice::Entry::West: return "west";
    case InputDevice::Entry::NorthPrime: return "northprime";
    case InputDevice::Entry::L1: return "l1";
    case InputDevice::Entry::R1: return "r1";
    case InputDevice::Entry::L2: return "l2";
    case InputDevice::Entry::R2: return "r2";
    case InputDevice::Entry::L3: return "l3";
    case InputDevice::Entry::R3: return "r3";
    case InputDevice::Entry::Up: return "up";
    case InputDevice::Entry::Right: return "right";
    case InputDevice::Entry::Down: return "down";
    case InputDevice::Entry::Left: return "left";
    case InputDevice::Entry::Joy1AxisH: return "joystick1left";
    case InputDevice::Entry::Joy1AxisV: return "joystick1up";
    case InputDevice::Entry::Joy2AxisH: return "joystick2left";
    case InputDevice::Entry::Joy2AxisV: return "joystick2up";
    case InputDevice::Entry::VolumeUp: return "vol+";
    case InputDevice::Entry::VolumeDown: return "vol-";
    case InputDevice::Entry::BrightnessUp: return "lum+";
    case InputDevice::Entry::BrightnessDown: return "lum-";
    case InputDevice::Entry::__Count:
    default:
    {
      { LOG(LogError) << "[InputDevice] Unknown int Input entry: " << (int)entry; }
      return "error";
    }
  }
}

InputDevice::Entry InputDevice::StringToEntry(const String& entry)
{
  static HashMap<String, InputDevice::Entry> sStringToEntry
  {
    { "none", InputDevice::Entry::None },
    { "start", InputDevice::Entry::Start },
    { "select", InputDevice::Entry::Select },
    { "hotkey", InputDevice::Entry::Hotkey },
    // Agnostic cardinal names (written by current versions)
    { "east", InputDevice::Entry::East },
    { "south", InputDevice::Entry::South },
    { "eastprime", InputDevice::Entry::EastPrime },
    { "north", InputDevice::Entry::North },
    { "west", InputDevice::Entry::West },
    { "northprime", InputDevice::Entry::NorthPrime },
    // Backward-compatible read of legacy SNES-based names (B=south, A=east, X=north, Y=west)
    { "a", InputDevice::Entry::East },
    { "b", InputDevice::Entry::South },
    { "c", InputDevice::Entry::EastPrime },
    { "x", InputDevice::Entry::North },
    { "y", InputDevice::Entry::West },
    { "z", InputDevice::Entry::NorthPrime },
    { "l1", InputDevice::Entry::L1 },
    { "r1", InputDevice::Entry::R1 },
    { "l2", InputDevice::Entry::L2 },
    { "r2", InputDevice::Entry::R2 },
    { "l3", InputDevice::Entry::L3 },
    { "r3", InputDevice::Entry::R3 },
    { "up", InputDevice::Entry::Up },
    { "right", InputDevice::Entry::Right },
    { "down", InputDevice::Entry::Down },
    { "left", InputDevice::Entry::Left },
    { "joystick1left", InputDevice::Entry::Joy1AxisH },
    { "joystick1up", InputDevice::Entry::Joy1AxisV },
    { "joystick2left", InputDevice::Entry::Joy2AxisH },
    { "joystick2up", InputDevice::Entry::Joy2AxisV },
    { "vol+", InputDevice::Entry::VolumeUp },
    { "vol-", InputDevice::Entry::VolumeDown },
    { "lum+", InputDevice::Entry::BrightnessUp },
    { "lum-", InputDevice::Entry::BrightnessDown },
  };
  InputDevice::Entry* result = sStringToEntry.try_get(entry.ToLowerCase());
  if (result != nullptr) return *result;
  { LOG(LogError) << "[InputDevice] Unknown string Input entry: " << entry; }
  return InputDevice::Entry::None;
}

const String& InputDevice::GetEntryName(Entry entry, bool icon) const
{
  (void)icon; // Icon font not ready yet: icon mode falls back to the plain-text name for now

  if (!IsPad()) return String::Empty;

  switch(entry)
  {
    // Generic entries: identical wording regardless of pad type
    case Entry::Up:             { static String s("UP");          return s; }
    case Entry::Right:          { static String s("RIGHT");       return s; }
    case Entry::Down:           { static String s("DOWN");        return s; }
    case Entry::Left:           { static String s("LEFT");        return s; }
    case Entry::Joy1AxisH:
    case Entry::Joy1AxisV:      { static String s("LEFT STICK");  return s; }
    case Entry::Joy2AxisH:
    case Entry::Joy2AxisV:      { static String s("RIGHT STICK"); return s; }
    case Entry::L3:             { static String s("L3");          return s; }
    case Entry::R3:             { static String s("R3");          return s; }
    case Entry::VolumeUp:       { static String s("VOL+");        return s; }
    case Entry::VolumeDown:     { static String s("VOL-");        return s; }
    case Entry::BrightnessUp:   { static String s("LUM+");        return s; }
    case Entry::BrightnessDown: { static String s("LUM-");        return s; }

    // Pad-dependent entries: real button name for this device's actual pad type
    case Entry::Start:
    case Entry::Select:
    case Entry::Hotkey:
    case Entry::East:
    case Entry::South:
    case Entry::EastPrime:
    case Entry::North:
    case Entry::West:
    case Entry::NorthPrime:
    case Entry::L1:
    case Entry::R1:
    case Entry::L2:
    case Entry::R2:
    {
      // Columns, matching Entry's own declaration order: Start, Select, Hotkey,
      // East, South, EastPrime, North, West, NorthPrime, L1, R1, L2, R2
      static const String sNames[PadTypes::__Count][13] =
      {
        /* HeightBitdo */ { "START",   "SELECT", "HEART",     "A",         "B",            "C", "X",            "Y",         "Z", "L1",           "R1",           "L2", "R2" },
        /* XBox        */ { "MENU",    "VIEW",   "GUIDE",     "A",         "B",            "C", "X",            "Y",         "Z", "LB",           "RB",           "LT", "RT" },
        /* Playstation */ { "OPTIONS", "SHARE",  "PS BUTTON", "CIRCLE",    "CROSS",        "C", "TRIANGLE",     "SQUARE",    "Z", "L1",           "R1",           "L2", "R2" },
        /* Arcade8     */ { "START",   "SELECT", "HOTKEY",    "TOP ROW 2", "TOP ROW 1",    "C", "TOP ROW 4",    "TOP ROW 3", "Z", "BOTTOM ROW 1", "BOTTOM ROW 2", "BOTTOM ROW 3", "BOTTOM ROW 4" },
        /* Arcade6     */ { "START",   "SELECT", "HOTKEY",    "TOP ROW 2", "TOP ROW 1",    "C", "BOTTOM ROW 1", "TOP ROW 3", "Z", "BOTTOM ROW 2", "BOTTOM ROW 3", "L2", "R2" },
        /* Arcade4     */ { "START",   "SELECT", "HOTKEY",    "BUTTON 2",  "BUTTON 1",     "C", "BUTTON 4",     "BUTTON 3",  "Z", "L1",           "R1",           "L2", "R2" },
        /* Gamecube    */ { "START",   "SELECT", "HOTKEY",    "A",         "B",            "C", "X",            "Y",         "Z", "L",            "R",            "Z",  "R2" },
        /* Megadrive6  */ { "START",   "SELECT", "HOTKEY",    "A",         "B",            "C", "X",            "Y",         "Z", "L1",           "R1",           "L",  "R"  },
        /* Megadrive3  */ { "START",   "SELECT", "HOTKEY",    "A",         "B",            "C", "X",            "Y",         "Z", "L1",           "R1",           "L2", "R2" },
        /* n64         */ { "START",   "SELECT", "HOTKEY",    "A",         "B",            "C", "X",            "Y",         "Z", "L",            "R",            "L2", "R2" },
      };
      int column = (int)entry - (int)Entry::Start;
      return sNames[(int)mPadType][column];
    }

    case Entry::None:
    case Entry::__Count:
    default: break;
  }
  return String::Empty;
}

InputDevice::InputDevice(SDL_Joystick* device, SDL_JoystickID deviceId, int deviceIndex, const String& deviceName, const SDL_JoystickGUID& deviceGUID, int deviceNbAxes, int deviceNbHats, int deviceNbButtons)
  : mDeviceName(deviceName)
  , mDeviceGUID(deviceGUID)
  , mDeviceSDL(device)
  , mLastTimeBatteryCheck(0)
  , mBatteryLevel(0)
  , mDeviceId(deviceId)
  , mDeviceIndex(deviceIndex)
  , mDeviceNbAxes(deviceNbAxes)
  , mDeviceNbHats(deviceNbHats)
  , mDeviceNbButtons(deviceNbButtons)
  , mConfigurationBits(0)
  , mPreviousHatsValues { 0 }
  , mPreviousAxisValues { 0 }
  , mNeutralAxisValues { 0 }
  , mButtonStates(CaptureCurrentButtonState())
  , mHatStates(CaptureCurrentHatState())
  , mButtonPressTime{ 0 }
  , mButtonLongFired(0)
  , mBatteryCharging(false)
  , mIsATruePad(false)
  , mIsProxifiedSource(RGPProxy::IsProxified(deviceIndex))
  , mPortBound(false)
  , mConfiguring(false)
  , mHotkeyState(false)
  , mKillSelect(false)
  , mJvsAnalogBlocked(false)
{
  #ifdef SDL_JOYSTICK_IS_OVERRIDEN_BY_RECALBOX
  mPath = SDL_JoystickDevicePathById(deviceIndex);
  #else
    #ifdef _RECALBOX_PRODUCTION_BUILD_
      #pragma GCC error "SDL_JOYSTICK_IS_OVERRIDEN_BY_RECALBOX undefined in production build!"
    #endif
    mPath = LookupPath();
  #endif

  mUDevDeviceName = UDevString(deviceIndex, UDevInfo::LowlevelName); // Default SDL2 name
  mUDevUniqID = UDevString(deviceIndex, UDevInfo::UniqueID);
  mUDevPhysicalPath = UDevString(deviceIndex, UDevInfo::PhysicalPath);
  mUDevPowerPath = Path(UDevString(deviceIndex, UDevInfo::PowerPath));

  memset(mPreviousAxisValues, 0, sizeof(mPreviousAxisValues));
  memset(mPreviousHatsValues, 0, sizeof(mPreviousHatsValues));

  RecordAxisNeutralPosition();
}

void InputDevice::ClearAll()
{
  for(int i = (int)Entry::__Count; --i >= 0;)
    mInputEvents[i] = InputEvent();
  mConfigurationBits = 0;
}

void InputDevice::LoadFrom(const InputDevice& source)
{
  *this = source;
}

String InputDevice::NameExtented()
{
  String result(Name());
  String::Unicode powerLevel = BatteryLevelIcon();
  if (powerLevel != 0)
    result.Append(' ').AppendUTF8(powerLevel);
  return result;
}

int InputDevice::BatteryLevel()
{
  if (!mUDevPowerPath.IsEmpty())
  {
    if (int now = (int)SDL_GetTicks(); now - mLastTimeBatteryCheck > 10000)
    {
      mLastTimeBatteryCheck = now;
      mBatteryCharging = Files::LoadFile(mUDevPowerPath / "status").Trim() == "Charging";
      int level = 0;
      if (Files::LoadFile(mUDevPowerPath / "capacity").Trim().TryAsInt(level)) return mBatteryLevel = level;
    }
    return mBatteryLevel;
  }
  return -1; // Unknown / unavailable
}

String::Unicode InputDevice::BatteryLevelIcon()
{
  if (mBatteryCharging) return 0xF1b4; // in charge
  int level = BatteryLevel();
  if (level <    0) return 0;      // Unknown / Wired
  if (level >= 100) return 0xF1B7; // Max
  if (level >   80) return 0xF1BA; // Full
  if (level >   40) return 0xF1B8; // Medium
  if (level >   15) return 0xF1B1; // Low
  return 0xF1b5;                   // Empty!
}

void InputDevice::Set(Entry input, InputEvent event)
{
  mInputEvents[(int)input] = event;
  mConfigurationBits |= 1 << (int)input;
}

void InputDevice::Unset(Entry input)
{
  mInputEvents[(int)input] = InputEvent();
  mConfigurationBits &= ~(1 << (int)input);
}

bool InputDevice::GetEntryConfiguration(Entry input, InputEvent& result) const
{
  if ((mConfigurationBits & (1 << (int)input)) != 0)
  {
    result = mInputEvents[(int)input];
    return true;
  }
  return false;
}

bool InputDevice::IsMatching(Entry input, InputEvent event) const
{
  if ((mConfigurationBits & (1 << (int)input)) != 0)
  {
    const InputEvent& comp = mInputEvents[(int) input];
    if (comp.Type() == event.Type() && comp.Id() == event.Id())
    {
      switch(comp.Type())
      {
        case InputEvent::EventType::Button:
        case InputEvent::EventType::Key: return true;
        case InputEvent::EventType::Hat: return (event.Value() == 0 || ((event.Value() & comp.Value()) != 0));
        case InputEvent::EventType::Axis: return event.Value() == 0 || comp.Value() == event.Value();
        case InputEvent::EventType::MouseWheel:
        case InputEvent::EventType::MouseButton:
        case InputEvent::EventType::Unknown:
        default: break;
      }
    }
  }
  return false;
}

InputDevice::Entry InputDevice::GetMatchedEntry(InputEvent input) const
{
  std::vector<String> maps;

  for (int i = (int)Entry::__Count; --i >= 0; )
    if ((mConfigurationBits & (1 << i)) != 0)
    {
      const InputEvent& chk = mInputEvents[i];

      if(chk.Device() == input.Device() && chk.Type() == input.Type() && chk.Id() == input.Id())
      {
        if(chk.Type() == InputEvent::EventType::Hat)
        {
          if (input.Value() == 0 || ((input.Value() & chk.Value()) != 0))
            return (Entry) i;
        }
        else if(input.Type() == InputEvent::EventType::Axis)
        {
          if(input.Value() == 0 || chk.Value() == input.Value())
            return (Entry)i;
        }
        else
        {
          return (Entry)i;
        }
      }
    }

  return Entry::None;
}

bool InputDevice::LoadAutoConfiguration(const String& configuration)
{
  bool result = false;

  { LOG(LogInfo) << "[InputDevice] Autoconfiguration from " << configuration; }
  String::List mappingList = configuration.Split(',');
  for(const String& mapping : mappingList)
    if (String key, value; mapping.Extract(':', key, value, false))
    {
      static HashMap<String, Entry> mapper
      ({
         { "dpup",          Entry::Up },
         { "dpright",       Entry::Right },
         { "dpdown",        Entry::Down },
         { "dpleft",        Entry::Left },
         { "a",             Entry::South }, // A <=> B and X <=> Y to match ES buttons.
         { "b",             Entry::East },
         { "c",             Entry::EastPrime },
         { "x",             Entry::West },
         { "y",             Entry::North },
         { "z",             Entry::NorthPrime },
         { "back",          Entry::Select },
         { "start",         Entry::Start },
         { "guide",         Entry::Hotkey },
         { "leftshoulder",  Entry::L1 },
         { "rightshoulder", Entry::R1 },
         { "lefttrigger",   Entry::L2 },
         { "righttrigger",  Entry::R2 },
         { "leftstick",     Entry::L3 },
         { "rightstick",    Entry::R3 },
         { "leftx",         Entry::Joy1AxisH },
         { "lefty",         Entry::Joy1AxisV },
         { "rightx",        Entry::Joy2AxisH },
         { "righty",        Entry::Joy2AxisV },
      });

      Entry* entry = mapper.try_get(key);
      if (entry == nullptr || value.length() < 2)
      {
        { LOG(LogError) << "[InputDevice] Unknown mapping: " << mapping; }
        continue;
      }

      // Default values
      InputEvent::EventType typeEnum = InputEvent::EventType::Unknown;
      int id = -1;
      int val = 0;
      int code = -1;
      int sign = 0;
      int parserIndex = 0;

      // Sign?
      switch(value[parserIndex])
      {
        case '-': { sign = -1; ++parserIndex; break; }
        case '+': { sign = +1; ++parserIndex; break; }
        default: break;
      }

      bool parsed = false;
      switch(value[parserIndex])
      {
        case 'a':
        {
          if (value.TryAsInt(parserIndex + 1, id))
          {
            typeEnum = InputEvent::EventType::Axis;
            if (sign == 0) val = -1; // non-signed axes are affected to joysticks: always left or up
            else val = sign;         // Otherwise, take the sign as-is
            // RecalboxJVS pedals: unsigned L2/R2 axes follow the firmware HID convention (press = +1).
            if (sign == 0 && IsJvsController() && (*entry == Entry::L2 || *entry == Entry::R2))
              val = +1;
            #ifdef SDL_JOYSTICK_IS_OVERRIDEN_BY_RECALBOX
              code = SDL_JoystickAxisEventCodeById(mDeviceIndex, id);
            #else
              #ifdef _RECALBOX_PRODUCTION_BUILD_
                #pragma GCC error "SDL_JOYSTICK_IS_OVERRIDEN_BY_RECALBOX undefined in production build!"
              #endif
            #endif
            parsed = true;
            { LOG(LogInfo) << "[InputDevice] Auto-Assign " << EntryToString(*entry) << " to Axis " << id << " (Code: " << code << ')'; }
          }
          break;
        }
        case 'b':
        {
          if (value.TryAsInt(parserIndex + 1, id))
          {
            typeEnum = InputEvent::EventType::Button;
            val = 1;
            #ifdef SDL_JOYSTICK_IS_OVERRIDEN_BY_RECALBOX
            code = SDL_JoystickButtonEventCodeById(mDeviceIndex, id);
            #else
              #ifdef _RECALBOX_PRODUCTION_BUILD_
                #pragma GCC error "SDL_JOYSTICK_IS_OVERRIDEN_BY_RECALBOX undefined in production build!"
              #endif
            #endif
            parsed = true;
            { LOG(LogInfo) << "[InputDevice] Auto-Assign " << EntryToString(*entry) << " to Button " << id << " (Code: " << code << ')'; }
          }
          break;
        }
        case 'h':
        {
          if (value.TryAsInt(parserIndex + 1, '.', id))
            if (value.TryAsInt(parserIndex + 3, val))
            {
              typeEnum = InputEvent::EventType::Hat;
              #ifdef SDL_JOYSTICK_IS_OVERRIDEN_BY_RECALBOX
              code = SDL_JoystickHatEventCodeById(mDeviceIndex, id);
              #else
                #ifdef _RECALBOX_PRODUCTION_BUILD_
                  #pragma GCC error "SDL_JOYSTICK_IS_OVERRIDEN_BY_RECALBOX undefined in production build!"
                #endif
              #endif
              parsed = true;
              { LOG(LogInfo) << "[InputDevice] Auto-Assign " << EntryToString(*entry) << " to Hat " << id << '/' << val << " (Code: " << code << ')'; }
            }
          break;
        }
        default:
        {
        }
      }
      if (!parsed)
      {
        { LOG(LogError) << "[InputDevice] Error parsing mapping: " << mapping; }
        continue;
      }

      Set(*entry, InputEvent(mDeviceId, typeEnum, id, val, code));
      result = true; // At least one mapping has been done
    }

  // Manage pads with only one analog joystick and no dpad
  if (mInputEvents[(int)Entry::Up].Type() == InputEvent::EventType::Unknown)
    if (mInputEvents[(int)Entry::Right].Type() == InputEvent::EventType::Unknown)
      if (mInputEvents[(int)Entry::Down].Type() == InputEvent::EventType::Unknown)
        if (mInputEvents[(int)Entry::Left].Type() == InputEvent::EventType::Unknown)
          if (mInputEvents[(int)Entry::Joy1AxisH].Type() != InputEvent::EventType::Unknown)
            if (mInputEvents[(int)Entry::Joy1AxisV].Type() != InputEvent::EventType::Unknown)
            {
              const InputEvent& V = mInputEvents[(int)Entry::Joy1AxisV];
              const InputEvent& H = mInputEvents[(int)Entry::Joy1AxisH];

              // Add DPAD configuration
              mInputEvents[(int)Entry::Up] = V;
              mInputEvents[(int)Entry::Down] = InputEvent(mDeviceId, V.Type(), V.Id(), -V.Value(), V.Code());
              mInputEvents[(int)Entry::Left] = H;
              mInputEvents[(int)Entry::Right] = InputEvent(mDeviceId, H.Type(), H.Id(), -H.Value(), H.Code());
              mConfigurationBits |= (1 << (int)Entry::Up) | (1 << (int)Entry::Right) |
                                    (1 << (int)Entry::Down) | (1 << (int)Entry::Left);

              // Remove left analog configuration
              mInputEvents[(int)Entry::Joy1AxisV] = mInputEvents[(int)Entry::Joy1AxisH] = InputEvent();
              mConfigurationBits &= ~((1 << (int)Entry::Joy1AxisV) | (1 << (int)Entry::Joy1AxisH));

              { LOG(LogInfo) << "[InputDevice] No DPAD detected. First analog joystick moved to DPAD"; }
            }

  // Still no HK? Use select
  if (mInputEvents[(int)Entry::Hotkey].Type() == InputEvent::EventType::Unknown)
  {
    { LOG(LogInfo) << "[InputDevice] Not hotkey button guessed. Using SELECT as Hotkey"; }
    mInputEvents[(int) Entry::Hotkey] = mInputEvents[(int) Entry::Select];
  }

  // Force hotkey on select for XBOX 360 generics
  static const uint8_t xboxGUID[16] = {3, 0, 3, 240, 94, 4, 0, 0, 142, 2, 0, 0, 20, 1, 0, 0};

  if(memcmp(mDeviceGUID.data, xboxGUID, 16) == 0)
  {
    { LOG(LogInfo) << "[InputDevice] XBox 360 generic X Input. Using SELECT as Hotkey"; }
    mInputEvents[(int) Entry::Hotkey] = mInputEvents[(int) Entry::Select];
  }

  if (PadTypes::Type detected = PadTypes::FromControllerName(mDeviceName); detected != PadTypes::__Count)
  {
    mPadType = detected;
    { LOG(LogInfo) << "[InputDevice] Auto-typed '" << mDeviceName << "' as "
                   << PadTypes::ToString(detected); }
  }

  return result;
}

int InputDevice::LoadFromXml(pugi::xml_node root)
{
  ClearAll();

  int loaded = 0;
  for (pugi::xml_node input = root.child("input"); input != nullptr; input = input.next_sibling("input"))
  {
    String name = input.attribute("name").as_string();
    String type = input.attribute("type").as_string();
    InputEvent::EventType typeEnum = InputEvent::StringToType(type);

    if(typeEnum == InputEvent::EventType::Unknown)
    {
      { LOG(LogError) << "[InputDevice] InputConfig load error - input of type \"" << type << "\" is invalid! Skipping input \"" << name << "\".\n"; }
      continue;
    }

    int id = input.attribute("id").as_int();
    int value = input.attribute("value").as_int();
    int code = input.attribute("code").as_int();

    if(value == 0)
    { LOG(LogWarning) << "[InputDevice] WARNING: InputConfig value is 0 for " << type << ' ' << id << "!\n"; }

    // Adapt seamlessly old l1/r1
    if (name == "pageup") name = "l1";
    if (name == "pagedown") name = "r1";
    Entry entry = StringToEntry(name);
    if (entry != Entry::None)
    {
      Set(entry, InputEvent(mDeviceId, typeEnum, id, value, code));
      loaded++;
    }
    else { LOG(LogError) << "[InputDevice] Unknown Joystick configuration entry: " << name << " of type " << type << "!\n"; }
  }
  return loaded;
}

void InputDevice::SaveToXml(pugi::xml_node parent) const
{
  pugi::xml_node cfg = parent.append_child("inputConfig");

  if(mDeviceId == InputEvent::sKeyboardDevice)
  {
    cfg.append_attribute("type") = "keyboard";
    cfg.append_attribute("deviceName") = "Keyboard";
  }else{
    cfg.append_attribute("type") = "joystick";
    cfg.append_attribute("deviceName") = mDeviceName.data();
  }

  cfg.append_attribute("deviceGUID") = GUID().c_str();
  cfg.append_attribute("deviceNbAxes") = mDeviceNbAxes;
  cfg.append_attribute("deviceNbHats") = mDeviceNbHats;
  cfg.append_attribute("deviceNbButtons") = mDeviceNbButtons;
  // Note: must NOT reuse "type" here — it's already set above to keyboard/joystick
  // and `configgen/controllers/controller.py` reads padType from `gamepadtype`.
  cfg.append_attribute("gamepadtype") = PadTypes::ToString(mPadType).c_str();
  cfg.append_attribute("version") = "2";
  // Only written when twins are around: it narrows this entry down to one port
  if (mPortBound && !mUDevPhysicalPath.empty())
    cfg.append_attribute("devicePhysicalPath") = mUDevPhysicalPath.c_str();

  for (int i = (int)Entry::__Count; --i >= 0; )
  {
    if ((mConfigurationBits & (1 << i)) == 0) continue;
    const InputEvent& entry = mInputEvents[i];

    pugi::xml_node input = cfg.append_child("input");
    input.append_attribute("name") = EntryToString((Entry)i).c_str();
    input.append_attribute("type") = InputEvent::TypeToString(entry.Type()).c_str();
    input.append_attribute("id").set_value(entry.Id());
    input.append_attribute("value").set_value(entry.Value());
    input.append_attribute("code").set_value(entry.Code());
  }
}

void InputDevice::StartEntry(InputCompactEvent::Entry entry, InputCompactEvent::Entry& bits)
{
  bits |= entry;
  int pos = __builtin_ffs((int)entry) - 1;
  if (mReferenceTimes[pos] == 0)
    mReferenceTimes[pos] = SDL_GetTicks();
}

int InputDevice::StopEntry(InputCompactEvent::Entry entry, InputCompactEvent::Entry& bits)
{
  bits |= entry;
  int pos = __builtin_ffs((int)entry) - 1;
  int result = mReferenceTimes[pos] != 0 ? (int)(SDL_GetTicks() - mReferenceTimes[pos]) : 0;
  mReferenceTimes[pos] = 0;
  return result;
}

int InputDevice::SetEntry(InputCompactEvent::Entry entry, bool active, InputCompactEvent::Entry& on,
                          InputCompactEvent::Entry& off)
{
  if (active) { StartEntry(entry, on); return 0; }
  return StopEntry(entry, off);
}

int InputDevice::ConvertButtonToOnOff(int button, bool pressed, InputCompactEvent::Entry& on, InputCompactEvent::Entry& off)
{
  int elapsed = 0;
  for (int i = (int)Entry::__Count; --i >= 0; )
    if ((mConfigurationBits & (1 << i)) != 0)
      if (const InputEvent& config = mInputEvents[i]; config.Type() == InputEvent::EventType::Button)
        if (config.Id() == button)
          elapsed = SetEntry(ConvertEntry((Entry)i), pressed, on, off);
  return elapsed;
}

int InputDevice::ConvertKeyToOnOff(int key, bool pressed, InputCompactEvent::Entry& on, InputCompactEvent::Entry& off)
{
  int elapsed = 0;
  for (int i = (int)Entry::__Count; --i >= 0; )
    if ((mConfigurationBits & (1 << i)) != 0)
      if (const InputEvent& config = mInputEvents[i]; config.Type() == InputEvent::EventType::Key)
        if (config.Id() == key)
          elapsed = SetEntry(ConvertEntry((Entry)i), pressed, on, off);
  return elapsed;
}

int InputDevice::ConvertHatToOnOff(int hat, int bits, InputCompactEvent::Entry& on, InputCompactEvent::Entry& off)
{
  int elapsed = 0;
  if (hat < sMaxHats)
  {
    int previousBits = mPreviousHatsValues[hat];
    for (int i = (int)Entry::__Count; --i >= 0; )
      if ((mConfigurationBits & (1 << i)) != 0)
        if (const InputEvent& config = mInputEvents[i]; config.Type() == InputEvent::EventType::Hat && config.Id() == hat)
        {
          // Hat bits are matching, set the target direction
          if ((bits & config.Value()) == config.Value())
            StartEntry(ConvertEntry((Entry)i), on);
          // Otherwise, if previous value matched,
          else if ((previousBits & config.Value()) == config.Value()) // Previous value match bits?
            elapsed = StopEntry(ConvertEntry((Entry)i), off);
        }

    // Record previous value
    mPreviousHatsValues[hat] = bits;
  }
  else { LOG(LogDebug) << "[InputDevice] Hat out of range !"; }

  return elapsed;
}

int InputDevice::ConvertAxisToOnOff(int axis, int value, InputCompactEvent::Entry& on, InputCompactEvent::Entry& off)
{
  int elapsed = 0;
  if (axis < sMaxAxis)
  {
    for (int i = (int)Entry::__Count; --i >= 0; )
      if ((mConfigurationBits & (1 << i)) != 0)
        if (const InputEvent& config = mInputEvents[i]; config.Type() == InputEvent::EventType::Axis && config.Id() == axis)
        {
          // For axis, we must distinguish target axis from other binary targets.
          // Axis to axis are converted to negative/center/positive.
          // Axis to binaries are converted to on only for the right sign. off otherwise
          // That is, axis on triggers will be properly converted to on/off buttons.
          if (InputCompactEvent::Entry targetEntry = ConvertEntry((Entry)i); targetEntry >= InputCompactEvent::Entry::J1Left)
          {
            InputCompactEvent::Entry targetOpposite = (InputCompactEvent::Entry)((int)targetEntry << 1);
            // Since configured event are negatives, if we got a positive value in current config, that means
            // the joystick is inverted.
            // Note: values are already normalized to -1/0/+1
            value = (config.Value() > 0) ? -value : value;
            int previousValue = mPreviousAxisValues[axis];
            if (previousValue != value)
            switch(value)
            {
              case -1: // configured axis direction?
              {
                StartEntry(targetEntry, on);
                if (previousValue > 0) elapsed = StopEntry(targetOpposite, off); // In case joystick has been quickly moved from one direction to another
                break;
              }
              case 1: // Opposite of configured axis direction?
              {
                StartEntry(targetOpposite, on);
                if (previousValue < 0) elapsed = StopEntry(targetEntry, off); // In case joystick has been quickly moved from one direction to another
                break;
              }
              default: // Axis centered again
              {
                elapsed = StopEntry(previousValue < 0 ? targetEntry : targetOpposite, off);
                break;
              }
            }
          }
          // Axis has a binary on/off
          else if (value == config.Value() || mPreviousAxisValues[axis] == config.Value())
            elapsed = SetEntry(targetEntry, value == config.Value(), on, off);
        }

    // Record previous value
    mPreviousAxisValues[axis] = value;
  }
  else { LOG(LogDebug) << "[InputDevice] Axis out of range !"; }

  return elapsed;
}

int InputDevice::ConvertMouseButtonToOnOff(int button, bool pressed, InputCompactEvent::Entry& on,
                                           InputCompactEvent::Entry& off)
{
  int elapsed = 0;
  for (int i = (int)Entry::__Count; --i >= 0; )
    if ((mConfigurationBits & (1 << i)) != 0)
      if (const InputEvent& config = mInputEvents[i]; config.Type() == InputEvent::EventType::MouseButton)
        if (config.Id() == button)
          elapsed = SetEntry(ConvertEntry((Entry)i), pressed, on, off);
  return elapsed;
}

int InputDevice::ConvertMouseWheelToOnOff(int wheel, int direction, InputCompactEvent::Entry& on,
                                          InputCompactEvent::Entry& off)
{
  bool stop = (direction & 0x1) == 0;
  if (stop) direction >>= 1;
  int elapsed = 0;
  for (int i = (int)Entry::__Count; --i >= 0; )
    if ((mConfigurationBits & (1 << i)) != 0)
      if (const InputEvent& config = mInputEvents[i]; config.Type() == InputEvent::EventType::MouseWheel)
        if (config.Id() == wheel)
          if (config.Value() == direction)
            elapsed = SetEntry(ConvertEntry((Entry)i), !stop, on, off);
  return elapsed;
}

bool InputDevice::IsEastButtonHeld() const
{
  // Read SDL's live state (refreshed for the whole poll batch before events are
  // dequeued), so it stays accurate even when an axis event was queued before its
  // matching press. On JVS gamepads the East button (SNES A) is always a plain button.
  if (mDeviceSDL == nullptr) return false;
  if ((mConfigurationBits & (1 << (int)Entry::East)) == 0) return false;
  const InputEvent& cfg = mInputEvents[(int)Entry::East];
  return cfg.Type() == InputEvent::EventType::Button
      && SDL_JoystickGetButton(mDeviceSDL, cfg.Id()) != 0;
}

bool InputDevice::IsLongPressButton(int button) const
{
  // Only these logical buttons trigger a long-press. Directions/hats/triggers are excluded
  // on purpose (e.g. holding the DPAD must keep repeating navigation, not fire a long-press).
  static constexpr Entry sEligible[] =
  {
    Entry::East, Entry::South, Entry::EastPrime, Entry::North, Entry::West, Entry::NorthPrime, Entry::Start, Entry::Select, Entry::L1, Entry::R1, Entry::Hotkey,
  };
  for (Entry entry : sEligible)
    if ((mConfigurationBits & (1 << (int)entry)) != 0)
    {
      const InputEvent& config = mInputEvents[(int)entry];
      if (config.Type() == InputEvent::EventType::Button && config.Id() == button)
        return true;
    }
  return false;
}

InputCompactEvent InputDevice::ConvertToCompact(const InputEvent& event, bool longPress)
{
  //{ LOG(LogError) << "[InputEvent] " << event.ToString(); }
  InputCompactEvent::Entry on  = InputCompactEvent::Entry::Nothing;
  InputCompactEvent::Entry off = InputCompactEvent::Entry::Nothing;
  int elapsed = 0;

  // Need configuration?
  if (!IsConfigured())
    off = (on |= InputCompactEvent::Entry::NeedConfiguration);

  switch(event.Type())
  {
    case InputEvent::EventType::Axis: elapsed = ConvertAxisToOnOff(event.Id(), event.Value(), on, off); break;
    case InputEvent::EventType::Hat: elapsed = ConvertHatToOnOff(event.Id(), event.Value(), on, off); break;
    case InputEvent::EventType::Button: elapsed = ConvertButtonToOnOff(event.Id(), event.Value() != 0, on, off); break;
    case InputEvent::EventType::Key: elapsed = ConvertKeyToOnOff(event.Id(), event.Value() != 0, on, off); break;
    case InputEvent::EventType::MouseButton: elapsed = ConvertMouseButtonToOnOff(event.Id(), event.Value() != 0, on, off); break;
    case InputEvent::EventType::MouseWheel: elapsed = ConvertMouseWheelToOnOff(event.Id(), event.Value(), on, off); break;
    case InputEvent::EventType::Unknown:
    default: { LOG(LogError) << "[InputDevice] Abnormal InputEvent::EventType: " << (int)event.Type(); break; }
  }

  // Process extended
  if ((on & InputCompactEvent::Entry::Hotkey) != 0)
  {
    mKillSelect = false;
    mHotkeyState = true;
    if (IsHotKeyAndSelectKeysTheSame() && (on & InputCompactEvent::Entry::Select) != 0)
      on &= ~InputCompactEvent::Entry::Select;
  }
  if (mHotkeyState)
  {
    // Keep special keys in the low dword
    on  = ( on & InputCompactEvent::Entry::AllSpecials) | (( on << 32) & ~InputCompactEvent::Entry::HotkeyAllSpecials);
    off = (off & InputCompactEvent::Entry::AllSpecials) | ((off << 32) & ~InputCompactEvent::Entry::HotkeyAllSpecials);
    // Anihilate select if a combination is detected
    if (IsHotKeyAndSelectKeysTheSame() && ((off | on) & InputCompactEvent::Entry::HotkeyAll) != 0)
      mKillSelect = true;
  }
  if ((off & InputCompactEvent::Entry::Hotkey) != 0)
  {
    mHotkeyState = false;
    if (IsHotKeyAndSelectKeysTheSame() && (off & InputCompactEvent::Entry::Select) != 0)
    {
      if (mKillSelect) off &= ~InputCompactEvent::Entry::Select; // Kill select from the off events
      else on |= InputCompactEvent::Entry::Select;  // Add select to the on event to make on/off in the same event
      mKillSelect = false;
    }
  }
  // TODO see with bk if we put that in a SpecialInputTransformer class
  if (Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::JVS && Board::Instance().CrtBoard().IsCrtAdapterAttached() /* && RecalboxJVS.mode() == Drive */)
  {
    if (on == InputCompactEvent::Entry::R2)
      on = InputCompactEvent::Entry::South;
    if (off == InputCompactEvent::Entry::R2)
      off = InputCompactEvent::Entry::South;
    if (on == InputCompactEvent::Entry::L2)
      on = InputCompactEvent::Entry::East;
    if (off == InputCompactEvent::Entry::L2)
      off = InputCompactEvent::Entry::East;
    //
    // if(event.Type() == InputEvent::EventType::Axis && event.Id() == 1)
    // {
    //   if (event.Value() == 1)
    //     on = InputCompactEvent::Entry::South;
    //   if (event.Value() == 0)
    //     off = InputCompactEvent::Entry::South;
    // }
    // if(event.Type() == InputEvent::EventType::Axis && event.Id() == 3)
    // {
    //   if (event.Value() == 1)
    //     on = InputCompactEvent::Entry::East;
    //   if (event.Value() == 0)
    //     off = InputCompactEvent::Entry::East;
    // }
  }

  // A long-press is reported as the release ('off') of the same entries, additionally flagged
  // 'long'. off/elapsed have already been computed by the normal release path just above.
  InputCompactEvent::Entry lng = longPress ? off : InputCompactEvent::Entry::Nothing;

  // Build final object
  InputCompactEvent result = { on, off, lng, elapsed, *this, event };
  //{ LOG(LogError) << "[CompactInputEvent] " << result.ToString(); }
  return result;
}

InputCompactEvent::Entry InputDevice::ConvertEntry(InputDevice::Entry entry)
{
  switch(entry)
  {
    case Entry::Start: return InputCompactEvent::Entry::Start;
    case Entry::Select: return InputCompactEvent::Entry::Select;
    case Entry::Hotkey: return InputCompactEvent::Entry::Hotkey;
    case Entry::East: return InputCompactEvent::Entry::East;
    case Entry::South: return InputCompactEvent::Entry::South;
    case Entry::EastPrime: return InputCompactEvent::Entry::EastPrime;
    case Entry::North: return InputCompactEvent::Entry::North;
    case Entry::West: return InputCompactEvent::Entry::West;
    case Entry::NorthPrime: return InputCompactEvent::Entry::NorthPrime;
    case Entry::L1: return InputCompactEvent::Entry::L1;
    case Entry::R1: return InputCompactEvent::Entry::R1;
    case Entry::L2: return InputCompactEvent::Entry::L2;
    case Entry::R2: return InputCompactEvent::Entry::R2;
    case Entry::L3: return InputCompactEvent::Entry::L3;
    case Entry::R3: return InputCompactEvent::Entry::R3;
    case Entry::Up: return InputCompactEvent::Entry::Up;
    case Entry::Right: return InputCompactEvent::Entry::Right;
    case Entry::Down: return InputCompactEvent::Entry::Down;
    case Entry::Left: return InputCompactEvent::Entry::Left;
    case Entry::Joy1AxisH: return InputCompactEvent::Entry::J1Left;
    case Entry::Joy1AxisV: return InputCompactEvent::Entry::J1Up;
    case Entry::Joy2AxisH: return InputCompactEvent::Entry::J2Left;
    case Entry::Joy2AxisV: return InputCompactEvent::Entry::J2Up;
    case Entry::VolumeUp: return InputCompactEvent::Entry::VolUp;
    case Entry::VolumeDown: return InputCompactEvent::Entry::VolDown;
    case Entry::BrightnessUp: return InputCompactEvent::Entry::LumUp;
    case Entry::BrightnessDown: return InputCompactEvent::Entry::LumDown;
    case Entry::None:
    case Entry::__Count:
    default: break;
  }
  { LOG(LogError) << "[InputDevice] Unknown entry! " << (int)entry; }
  return InputCompactEvent::Entry::Nothing;
}

bool InputDevice::EqualsTo(const InputDevice& to) const
{
  return (mDeviceName == to.mDeviceName) &&
         (memcmp(&mDeviceGUID, &to.mDeviceGUID, sizeof(SDL_JoystickGUID)) == 0) &&
         (mDeviceNbAxes == to.mDeviceNbAxes) &&
         (mDeviceNbHats == to.mDeviceNbHats) &&
         (mDeviceNbButtons == to.mDeviceNbButtons)
         ;
}

bool InputDevice::CheckNeutralPosition(bool skipAxis) const
{
  // Check buttons
  for(int i = mDeviceNbButtons; --i >= 0; )
    if (SDL_JoystickGetButton(mDeviceSDL, i) != 0)
      return false;
  // Check hats
  for(int i = mDeviceNbHats; --i >= 0; )
    if (SDL_JoystickGetHat(mDeviceSDL, i) != 0)
      return false;
  // Check axis
  if (skipAxis) return true;
  for(int i = mDeviceNbAxes; --i >= 0; )
    if (i < sMaxAxis)
    {
      int axis = SDL_JoystickGetAxis(mDeviceSDL, i);
      axis = axis < -sJoystickDeadZone ? -1 : (axis > sJoystickDeadZone ? 1 : 0);
      if (axis != mNeutralAxisValues[i])
        return false;
    }
    else { LOG(LogDebug) << "[InputDevice] Axis out of range !"; }
  return true;
}

String InputDevice::UDevString(int index, UDevInfo info)
{
  if (index >= 0)
  {
    UDev udev;
    UDevEnumerate::DeviceList list = UDevEnumerate(udev).MatchSysname(mPath.Filename()).List();
    if (list.size() == 1)
    {
      const UDevDevice& device = list[0];
      mIsATruePad = device.Property("ID_INPUT_JOYSTICK") == "1";
      switch(info)
      {
        case UDevInfo::LowlevelName:
        {
          //! ID_VENDOR_ENC + ID_MODEL_ENC for bluetooth pads
          if (device.Property("ID_BUS") == "bluetooth")
          {
            String vendor(device.PropertyDecode("ID_VENDOR_ENC"));
            String model(device.PropertyDecode("ID_MODEL_ENC"));
            if (!model.Trim().empty())
            {
              if (!vendor.Trim().empty()) model.Insert(0, ' ').Insert(0, vendor);
              if (!model.Trim().empty()) return model;
            }
          }
          //! ATTRS{name} for all others
          String name(device.Parent().Sysattr("name"));
          if (!name.empty()) return name;

          // Default: SDL2 name
          return mDeviceName;
        }
        case UDevInfo::UniqueID:
        {
          String uniq(device.Parent().Sysattr("uniq"));
          return uniq;
        }
        case UDevInfo::PhysicalPath: return device.Property("ID_PATH");
        case UDevInfo::PowerPath:
        {
          // mUDevUniqueID must have been filled before - don't reorder the inits!
          for(const Path& path : Path("/sys/class/power_supply").GetDirectoryContent())
            if (path.IsDirectory())
              if (path.ToString().EndsWith(mUDevUniqID))
                return path.ToString();
          break;
        }
        default: return String::Empty;
      }
    }
    else { LOG(LogError) << "[InputDevice] Cannot get udev info " << (int)info <<" for pad index " << index; }
  }
  return "";
}

void InputDevice::RecordAxisNeutralPosition()
{
  // Fill neutral values
  for (int i = mDeviceNbAxes; --i >= 0;)
    if (i < sMaxAxis)
    {
      Sint16 state = 0;
      if (SDL_JoystickGetAxisInitialState(mDeviceSDL, i, &state) == SDL_TRUE)
        mNeutralAxisValues[i] = state < -sJoystickDeadZone ? -1 : (state > sJoystickDeadZone ? 1 : 0);
    }
    else { LOG(LogDebug) << "[InputDevice] Axis out of range !"; }
}

int InputDevice::CaptureCurrentButtonState() const
{
  int result = 0;
  for(int i = mDeviceNbButtons; --i >= 0;)
    if (SDL_JoystickGetButton(mDeviceSDL, i) != 0)
      result |= 1 << i;
  return result;
}

int InputDevice::CaptureCurrentHatState() const
{
  int result = 0;
  for(int i = mDeviceNbHats; --i >= 0;)
    result |= (SDL_JoystickGetHat(mDeviceSDL, i) & 0xF) << (i * 4);
  return result;
}

InputDevice& InputDevice::GetRawButtonEvents(Array<SDL_Event>& to)
{
  int newState = CaptureCurrentButtonState();
  int changed = mButtonStates ^ newState;
  int now = (int)SDL_GetTicks();
  // Long-press is disabled while the pad is being (re)configured, so a held button never
  // swallows its own release inside the configuration UI.
  bool longEnabled = !mConfiguring;

  // Emit events on button state changes (press/release)
  if (changed != 0)
    for(int i = mDeviceNbButtons; --i >= 0; )
      if ((changed & (1 << i)) != 0)
      {
        bool pressed = (newState & (1 << i)) != 0;
        if (pressed)
        {
          // Arm the long-press timer only for physical buttons mapped to an eligible logical
          // button. 0 means "not armed"; clamp the very first tick to 1 to keep 0 as sentinel.
          mButtonPressTime[i] = (longEnabled && IsLongPressButton(i)) ? (now != 0 ? now : 1) : 0;
          mButtonLongFired &= ~(1 << i);
          to.Add({ .jbutton = { .type = SDL_JOYBUTTONDOWN, .timestamp = (Uint32)now, .which = mDeviceId, .button = (Uint8)i, .state = SDL_PRESSED, .padding1 = 0, .padding2 = 0 } });
        }
        else
        {
          // Physical release: swallow it if a long-press was already emitted for this button.
          bool swallow = (mButtonLongFired & (1 << i)) != 0;
          mButtonPressTime[i] = 0;
          mButtonLongFired &= ~(1 << i);
          if (!swallow)
            to.Add({ .jbutton = { .type = SDL_JOYBUTTONUP, .timestamp = (Uint32)now, .which = mDeviceId, .button = (Uint8)i, .state = SDL_RELEASED, .padding1 = 0, .padding2 = 0 } });
        }
        { LOG(LogTrace) << "[InputDevice] Button " << i << ' ' << (pressed ? "down" : "up") << " - OldState: " << String().AppendHexa(mButtonStates) << " - NewState: " << String().AppendHexa(newState); }
      }

  // Mid-hold long-press: fire a "long release" as soon as the threshold is crossed, while the
  // button is still physically held. The subsequent physical release is swallowed above.
  if (longEnabled)
    for(int i = mDeviceNbButtons; --i >= 0; )
      if (mButtonPressTime[i] != 0 && (mButtonLongFired & (1 << i)) == 0)
        if (now - mButtonPressTime[i] >= sLongPressDuration)
        {
          mButtonLongFired |= (1 << i);
          // Release event (state RELEASED) flagged 'long' through padding1. Joystick button events
          // are entirely re-synthesized here, so padding1 is free to carry that marker.
          to.Add({ .jbutton = { .type = SDL_JOYBUTTONUP, .timestamp = (Uint32)now, .which = mDeviceId, .button = (Uint8)i, .state = SDL_RELEASED, .padding1 = 1, .padding2 = 0 } });
          { LOG(LogTrace) << "[InputDevice] Button " << i << " long-press fired after " << (now - mButtonPressTime[i]) << "ms"; }
        }

  mButtonStates = newState;
  return *this;
}

InputDevice& InputDevice::GetRawHatEvents(Array<SDL_Event>& to)
{
  int newState = CaptureCurrentHatState();
  int changed = mHatStates ^ newState;

  if (changed != 0)
    for(int i = mDeviceNbHats; --i >= 0; )
      if ((changed & (0xF << (i * 4))) != 0)
      {
        to.Add({ .jhat = { .type = SDL_JOYHATMOTION, .timestamp = SDL_GetTicks(), .which = mDeviceId, .hat = (Uint8)i, .value = (Uint8)((newState >> (i * 4)) & 0xF), .padding1 = 0, .padding2 = 0 } });
        { LOG(LogTrace) << "[InputDevice] Hat " << i << " moved. - OldState: " << String().AppendHexa((mHatStates & (0xF << (i * 4)))) << " - NewState: " << String().AppendHexa((newState & (0xF << (i * 4)))); }
      }

  mHatStates = newState;
  return *this;
}

#ifndef SDL_JOYSTICK_IS_OVERRIDEN_BY_RECALBOX
Path InputDevice::LookupPath()
{
  UDev udev;
  for(int i = 0; i < 64; ++i)
  {
    // Build path
    Path devicePath(String("/dev/input/event").Append(i));
    if (!devicePath.Exists()) continue;
    // Lookup udev device
    UDevEnumerate::DeviceList list = UDevEnumerate(udev).MatchSysname(Path(devicePath).Filename()).List();
    if (list.empty()) continue;
    // Get device
    const UDevDevice& device = list[0];
    // Is it a true pad?
    if (device.Property("ID_INPUT_JOYSTICK") != "1") continue;
    // Get first parent
    const UDevDevice& parent = device.Parent();
    // Extract IDs used in SDL2 GUID
    int busType = String('$').Append(parent.Sysattr("id/bustype")).AsInt();
    int product = String('$').Append(parent.Sysattr("id/product")).AsInt();
    int vendor  = String('$').Append(parent.Sysattr("id/vendor")).AsInt();
    int version = String('$').Append(parent.Sysattr("id/version")).AsInt();
    // Build GUIDs - use a 4 little indian 32bits int for comparison, masked to use only LSW
    union InternalGUID
    {
      SDL_JoystickGUID Sdl;
      int Native[4];
    };
    InternalGUID Sdl { .Sdl = mDeviceGUID };
    InternalGUID Dev { .Native { busType, vendor, product, version } };
    // Compare
    bool match = true;
    for(int n = (int)(sizeof(InternalGUID::Native) / sizeof(InternalGUID::Native[0])); --n >= 0;)
      if ((Sdl.Native[n] & 0xFFFF) != (Dev.Native[n] & 0xFFFF))
      {
        match = false;
        break;
      }
    if (match) return devicePath;
  }
  { LOG(LogError) << "[InputDevice] Cannot lookup device path for pad " << Name(); }
  return Path::Empty;
}

#endif