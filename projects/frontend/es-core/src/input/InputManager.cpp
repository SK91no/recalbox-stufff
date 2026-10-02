#include "pugixml/pugixml.hpp"
#include "SDL2/SDL.h"
#include "SDL2/SDL_joystick.h"
#include <input/InputManager.h>
#include <RootFolders.h>
#include <WindowManager.h>
#include <input/InputMapper.h>
#include <input/AutoMapper.h>
#include <guis/GuiInfoPopup.h>
#include <utils/locale/LocaleHelper.h>
#include <utils/hash/Crc16.h>
#include <utils/String.h>
#include <RecalboxConf.h>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>

#define KEYBOARD_GUID_STRING { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
#define EMPTY_GUID_STRING { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

InputManager::InputManager(IKeyboardShortcut* interface)
  : StaticLifeCycleControler<InputManager>("inputmanager")
  , mIndexToId {}
  , mKeyboard(nullptr, InputEvent::sKeyboardDevice, (int)InputEvent::sKeyboardDevice, "Keyboard", KEYBOARD_GUID_STRING, 0, 0, 125)
  , mMousse(nullptr, InputEvent::sMouseDevice, (int)InputEvent::sMouseDevice, "Mouse", KEYBOARD_GUID_STRING, 0, 0, 5)
  , mScancodeStates()
  , mScancodePreviousStates()
  , mHub("DevInputWatch")
  , mShortcutInterface(interface)
  , mKeyboardLayoutMapper(RecalboxConf::Instance().AsString("system.kblayout", "us"))
  , mAltGrPressed(false)
  , mJoystickChangePending(false)
  , mJoystickChangePendingRemoved(false)
  , mHasJvsLightGun(DetectJvsLightGun())
{
  memset(mScancodeStates, 0, sizeof(mScancodeStates));
  memset(mScancodePreviousStates, 0, sizeof(mScancodePreviousStates));
  { LOG(LogInfo) << "[InputManager] Initial JVS light gun present: " << (mHasJvsLightGun ? "yes" : "no"); }
  // Create keyboard
  LoadDefaultKeyboardConfiguration();
  // Watcher
  mWatcher.WatchFile(Path("/dev/input"), EventType::Remove | EventType::Create);
  mHub.AddWatcher(mWatcher, *this);
  // Add mapper to pad change callbacks
  mNotificationInterfaces.Add(&mMapper);
}

int InputManager::GetDeviceIndexFromId(SDL_JoystickID deviceId)
{
  // Already exists?
  InputDevice** device = mIdToDevices.try_get(deviceId);
  if (device != nullptr)
    return (*device)->Index();

  //{ LOG(LogError) << "[Input] Unexisting device!"; }
  return -1;
}

String InputManager::GetDeviceNameFromId(SDL_JoystickID deviceId)
{
  // Already exists?
  InputDevice** device = mIdToDevices.try_get(deviceId);
  if (device != nullptr)
    return (*device)->Name();

  { LOG(LogError) << "[Input] GetDeviceNameFromId: Unexisting device!"; }
  return "Unknown";
}

InputDevice& InputManager::GetDeviceConfigurationFromId(SDL_JoystickID deviceId)
{
  // Already exists?
  InputDevice** device = mIdToDevices.try_get(deviceId);
  if (device != nullptr)
    return **device;

  { LOG(LogError) << "[Input] GetDeviceConfigurationFromId: Unexisting device!"; }
  static InputDevice sEmptyDevice(nullptr, InputEvent::sEmptyDevice, (int)InputEvent::sEmptyDevice, "Empty Device", EMPTY_GUID_STRING, 0, 0, 0);
  return sEmptyDevice;
}

void InputManager::InitializeSDL2JoystickSystem()
{
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
  /*if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
  {
    LOG(LogError) << "[InputManager] Error initializing SDL Joystick system";
    return;
  }*/
  SDL_JoystickEventState(SDL_ENABLE);
  SDL_JoystickUpdate();
  { LOG(LogInfo) << "[InputManager] Initialize SDL Joystick system"; }
}

void InputManager::FinalizeSDL2JoystickSystem()
{
  SDL_JoystickEventState(SDL_DISABLE);
  //SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
  { LOG(LogInfo) << "[InputManager] Finalize SDL Joystick system"; }
}

void InputManager::Finalize()
{
  ClearAllConfigurations();
  FinalizeSDL2JoystickSystem();
}

void InputManager::Initialize()
{
  ClearAllConfigurations();
  InitializeSDL2JoystickSystem();
  LoadAllJoysticksConfiguration(std::vector<InputDevice>(), nullptr, false);
}

void InputManager::Refresh(WindowManager* window, bool padPlugged)
{
  InitializeSDL2JoystickSystem();
  mHasJvsLightGun = DetectJvsLightGun();
  std::vector<InputDevice> previousList = BuildCurrentDeviceList();
  ClearAllConfigurations();
  LoadAllJoysticksConfiguration(previousList, window, padPlugged);
  { LOG(LogInfo) << "[InputManager] Refresh joysticks (JVS light gun present: " << (mHasJvsLightGun ? "yes" : "no") << ')'; }
}

bool InputManager::DetectJvsLightGun()
{
  // RecalboxJVS guns expose themselves as mouse devices, so they never show up in
  // SDL's joystick layer: look them up directly in the kernel input device list.
  // (read by hand: /proc files report a size of 0 and confuse Files::LoadFile)
  int fd = open("/proc/bus/input/devices", O_RDONLY);
  if (fd < 0) return false;
  String content;
  char buffer[4096];
  for (int r; (r = (int)read(fd, buffer, sizeof(buffer))) > 0; )
    content.Append(buffer, r);
  close(fd);
  return content.Contains("Name=\"RecalboxJVS Gun");
}

void InputManager::LoadDefaultKeyboardConfiguration()
{
  // Load default configuration
  mKeyboard.ClearAll();
  mKeyboard.Set(InputDevice::Entry::Up, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_UP, 1));
  mKeyboard.Set(InputDevice::Entry::Down, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_DOWN, 1));
  mKeyboard.Set(InputDevice::Entry::Left, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_LEFT, 1));
  mKeyboard.Set(InputDevice::Entry::Right, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_RIGHT, 1));

  mKeyboard.Set(InputDevice::Entry::East, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_s, 1));
  mKeyboard.Set(InputDevice::Entry::South, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_a, 1));
  mKeyboard.Set(InputDevice::Entry::North, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_p, 1));
  mKeyboard.Set(InputDevice::Entry::West, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_l, 1));
  mKeyboard.Set(InputDevice::Entry::Start, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_RETURN, 1));
  mKeyboard.Set(InputDevice::Entry::Select, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_SPACE, 1));

  mKeyboard.Set(InputDevice::Entry::L1, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_PAGEUP, 1));
  mKeyboard.Set(InputDevice::Entry::R1, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_PAGEDOWN, 1));
  mKeyboard.Set(InputDevice::Entry::L2, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_HOME, 1));
  mKeyboard.Set(InputDevice::Entry::R2, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_END, 1));

  mKeyboard.Set(InputDevice::Entry::Hotkey, InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, SDLK_ESCAPE, 1));

  //WriteDeviceXmlConfiguration(mKeyboard);

  mMousse.ClearAll();
  mMousse.Set(InputDevice::Entry::South, InputEvent(InputEvent::sMouseDevice, InputEvent::EventType::MouseButton, 1, 1));
  mMousse.Set(InputDevice::Entry::Start, InputEvent(InputEvent::sMouseDevice, InputEvent::EventType::MouseButton, 2, 1));
  mMousse.Set(InputDevice::Entry::East, InputEvent(InputEvent::sMouseDevice, InputEvent::EventType::MouseButton, 3, 1));
  mMousse.Set(InputDevice::Entry::Up, InputEvent(InputEvent::sMouseDevice, InputEvent::EventType::MouseWheel, SDL_MOUSEWHEEL_NORMAL, 1));
  mMousse.Set(InputDevice::Entry::Down, InputEvent(InputEvent::sMouseDevice, InputEvent::EventType::MouseWheel, SDL_MOUSEWHEEL_NORMAL, -1));
  mMousse.Set(InputDevice::Entry::Left, InputEvent(InputEvent::sMouseDevice, InputEvent::EventType::MouseWheel, SDL_MOUSEWHEEL_FLIPPED, 1));
  mMousse.Set(InputDevice::Entry::Right, InputEvent(InputEvent::sMouseDevice, InputEvent::EventType::MouseWheel, SDL_MOUSEWHEEL_FLIPPED, -1));

  // Load configuration
  LookupDeviceXmlConfiguration(mKeyboard);
  LookupDeviceXmlConfiguration(mMousse);
}

void InputManager::ClearAllConfigurations()
{
  // Close SDL devices
  mIdToSdlJoysticks.clear();
  // Delete InputDevices
  for (const auto& it : mIdToDevices)
    delete it.second;
  mIdToDevices.clear();
}

std::vector<InputDevice> InputManager::BuildCurrentDeviceList()
{
  std::vector<InputDevice> result;
  for(const auto& item : mIdToDevices)
    result.push_back(*item.second);
  std::ranges::sort(result, [] (const InputDevice& a, const InputDevice& b)
  {
    if (a.Name() < b.Name()) return true;
    if (a.GUID() < b.GUID()) return true;
    if (a.AxeCount() < b.AxeCount()) return true;
    if (a.HatCount() < b.HatCount()) return true;
    return (a.ButtonCount() < b.ButtonCount());
  });

  return result;
}

void InputManager::LoadAllJoysticksConfiguration(std::vector<InputDevice> previous, WindowManager* window, bool padPlugged)
{
  int numJoysticks = SDL_NumJoysticks();
  { LOG(LogInfo) << "[InputManager] Joystick count: " << numJoysticks; }
  for (int i = 0; i < numJoysticks; i++)
    LoadJoystickConfiguration(i);

  //! Notify
  for(IInputChange* input : mNotificationInterfaces)
    input->PadsAddedOrRemoved(mJoystickChangePendingRemoved);

  // No info popup ?
  if (window == nullptr) return;
  if (!padPlugged) return;

  // Build current list & make diff with previous list
  std::vector<InputDevice> current = BuildCurrentDeviceList();
  KeepDifferentPads(current, previous);
  // Popup every added pad
  for(const InputDevice& added : current)
  {
    // Build the text
    String text = added.Name();
    text.Append(' ').Append(_(" has been plugged!")).Append("\n\n");
    if (added.IsConfigured()) text.Append(_("Ready to play!"));
    else text.Append(_("Not configured yet! Press a button to enter the configuration window."));

    GuiInfoPopupBase* popup = new GuiInfoPopup(*window, text, 10, PopupType::Pads);
    window->InfoPopupAdd(popup);
  }
  // Popup every added pad
  for(const InputDevice& removed : previous)
  {
    // Build the text
    String text = removed.Name();
    text.Append(' ').Append(_(" has been unplugged!"));

    GuiInfoPopupBase* popup = new GuiInfoPopup(*window, text, 10, PopupType::Pads);
    window->InfoPopupAdd(popup);
  }
}

String InputManager::DeviceGUIDString(SDL_Joystick* joystick)
{
  char guid[128];
  SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joystick), guid, sizeof(guid));
  return guid;
}

void InputManager::LoadJoystickConfiguration(int index)
{
  bool autoConfigured = true;
  { LOG(LogInfo) << "[InputManager] Load configuration for Joystick #: " << index; }

  // Open joystick & add to our list
  SDL_Joystick* joy = SDL_JoystickOpen(index);
  if (joy == nullptr) return;

  // Get device properties
  int buttons = SDL_JoystickNumButtons(joy);
  int axes    = SDL_JoystickNumAxes(joy);
  int hats    = SDL_JoystickNumHats(joy);

  // Record device properties
  SDL_JoystickID identifier = SDL_JoystickInstanceID(joy);
  mIdToSdlJoysticks[identifier] = joy;
  mIndexToId[index] = identifier;

  // Create device configuration
  InputDevice* device = new InputDevice(joy,
                                        identifier,
                                        index,
                                        SDL_JoystickName(joy),
                                        SDL_JoystickGetGUID(joy),
                                        axes,
                                        hats,
                                        buttons);

  // Try to load from configuration file
  if (!LookupDeviceXmlConfiguration(*device))
  {
    String autoMapping = AutoMapper(index).GetSDLMapping();
    autoConfigured = !autoMapping.empty();
    if (autoConfigured)
    {
      autoConfigured = device->LoadAutoConfiguration(autoMapping);
      WriteDeviceXmlConfiguration(*device);
    }
  }

  mIdToDevices[identifier] = device;
  if (!autoConfigured)
    { LOG(LogWarning) << "[Input] Unknown joystick " << SDL_JoystickName(joy)
                      << " (GUID: " << DeviceGUIDString(joy) << ", Instance ID: " << identifier
                      << ", Device Index: " << index
                      << ", Axis: " << SDL_JoystickNumAxes(joy)
                      << ", Hats: " << SDL_JoystickNumHats(joy)
                      << ", Buttons: " << SDL_JoystickNumButtons(joy) << ')'; }
  else
    // Store
    { LOG(LogInfo) << "[Input] Added joystick " << SDL_JoystickName(joy)
                      << " (GUID: " << DeviceGUIDString(joy) << ", Instance ID: " << identifier
                      << ", Device Index: " << index
                      << ", Axis: " << SDL_JoystickNumAxes(joy)
                      << ", Hats: " << SDL_JoystickNumHats(joy)
                      << ", Buttons: " << SDL_JoystickNumButtons(joy) << ')'; }
}

int InputManager::ConfiguredControllersCount()
{
  int num = 0;
  for (auto& mInputConfig : mIdToDevices)
    if (mInputConfig.second->IsConfigured())
      num++;
  return num;
}

InputCompactEvent InputManager::ManageAxisEvent(const SDL_JoyAxisEvent& axis)
{
  // Normalize value
  int value = axis.value < -InputDevice::sJoystickDeadZone ? -1 : (axis.value > InputDevice::sJoystickDeadZone ? 1 : 0);

  // Check if the axis enter or exit from the dead area
  InputDevice& device = GetDeviceConfigurationFromId(axis.which);
  /* Drop R.G.P. proxified sources: SDL exposes both source and mirror, so every
   * press would otherwise reach ES twice. */
  if (device.IsProxifiedSource()) return InputCompactEvent(device);
  // On a JVS arcade cabinet equipped with light guns, holding the A button (the
  // gun's secondary button) means the gun aims off-screen: the JVS gamepad's
  // analog axes then carry garbage. Drop those analog events while A is held;
  // let them through otherwise.
  if (mHasJvsLightGun && device.IsJvsController())
  {
    bool block = device.IsEastButtonHeld();
    if (block != device.JvsAnalogBlocked())
    {
      { LOG(LogInfo) << "[Input] JVS analog gating for '" << device.Name() << "' → "
                     << (block ? "BLOCKED (A held)" : "OPEN (A released)"); }
      device.SetJvsAnalogBlocked(block);
    }
    if (block) return InputCompactEvent(device);
  }
  if (value != device.PreviousAxisValues(axis.axis))
    return device.ConvertToCompact(InputEvent(axis.which, InputEvent::EventType::Axis, axis.axis, value));
  return InputCompactEvent(device);
}

InputCompactEvent InputManager::ManageButtonEvent(const SDL_JoyButtonEvent& button)
{
  InputDevice& device = GetDeviceConfigurationFromId(button.which);
  if (device.IsProxifiedSource()) return InputCompactEvent(device);
  // padding1 carries the long-press marker set by InputDevice::GetRawButtonEvents on the
  // synthesized "long release" event (the button is still physically held at that point).
  bool longPress = button.padding1 != 0;
  return device.ConvertToCompact(InputEvent(button.which, InputEvent::EventType::Button, button.button, button.state == SDL_PRESSED ? 1 : 0), longPress);
}

InputCompactEvent InputManager::ManageHatEvent(const SDL_JoyHatEvent& hat)
{
  InputDevice& device = GetDeviceConfigurationFromId(hat.which);
  if (device.IsProxifiedSource()) return InputCompactEvent(device);
  InputCompactEvent event = device.ConvertToCompact(InputEvent(hat.which, InputEvent::EventType::Hat, hat.hat, hat.value));
  return event;
}

InputCompactEvent InputManager::ManageKeyEvent(const SDL_KeyboardEvent& key, bool down)
{
  // Track AltGr key state (Right Alt)
  if (key.keysym.sym == SDLK_RALT || key.keysym.scancode == SDL_SCANCODE_RALT)
  {
    mAltGrPressed = down;
  }

  // Map scancode to keycode using the configured keyboard layout
  SDL_Keycode mappedKey = mKeyboardLayoutMapper.MapScancode(key.keysym.scancode);
  InputEvent event = InputEvent(InputEvent::sKeyboardDevice, InputEvent::EventType::Key, mappedKey, down ? 1 : 0);
  // Ignore repeat events
  if (key.repeat != 0u) return { InputCompactEvent::Entry::Nothing, InputCompactEvent::Entry::Nothing, 0, mKeyboard, event };
  // Quit?
  if ((key.keysym.mod & KMOD_ALT) != 0 && !down)
  {
    switch(key.keysym.sym)
    {
      case SDLK_F3:
      {
        if (mShortcutInterface != nullptr) mShortcutInterface->ShortcutTriggered(IKeyboardShortcut::Shortcut::Search);
        return { InputCompactEvent::Entry::Nothing, InputCompactEvent::Entry::Nothing, 0, mKeyboard, event };
      }
      case SDLK_F4:
      {
        if (mShortcutInterface != nullptr) mShortcutInterface->ShortcutTriggered(IKeyboardShortcut::Shortcut::Quit);
        return { InputCompactEvent::Entry::Nothing, InputCompactEvent::Entry::Nothing, 0, mKeyboard, event };
      }
      case SDLK_F5:
      {
        if (mShortcutInterface != nullptr) mShortcutInterface->ShortcutTriggered(IKeyboardShortcut::Shortcut::Refresh);
        return { InputCompactEvent::Entry::Nothing, InputCompactEvent::Entry::Nothing, 0, mKeyboard, event };
      }
      case SDLK_F7:
      {
        if (mShortcutInterface != nullptr) mShortcutInterface->ShortcutTriggered(IKeyboardShortcut::Shortcut::MenuReload);
        return { InputCompactEvent::Entry::Nothing, InputCompactEvent::Entry::Nothing, 0, mKeyboard, event };
      }
      case SDLK_F8:
      {
        if (mShortcutInterface != nullptr) mShortcutInterface->ShortcutTriggered(IKeyboardShortcut::Shortcut::MenuTemplate);
        return { InputCompactEvent::Entry::Nothing, InputCompactEvent::Entry::Nothing, 0, mKeyboard, event };
      }
      default: break;
    }
  }
  return mKeyboard.ConvertToCompact(event);
}

InputCompactEvent InputManager::ManageMouseButtonEvent(const SDL_MouseButtonEvent& button, bool down)
{
  InputEvent event = InputEvent((int)button.which, InputEvent::EventType::MouseButton, button.button, down  ? 1 : 0);
  return mMousse.ConvertToCompact(event);
}

InputCompactEvent InputManager::ManageMouseWheelEvent(const SDL_MouseWheelEvent& wheel)
{
  InputEvent event = InputEvent(InputEvent::sMouseDevice, InputEvent::EventType::MouseWheel,
                                wheel.y != 0 ? SDL_MOUSEWHEEL_NORMAL : wheel.x != 0 ? SDL_MOUSEWHEEL_FLIPPED : -1,
                                (wheel.y != 0 ? wheel.y : wheel.x) << ((int)wheel.which < 0 ? 1 : 0));
  return mMousse.ConvertToCompact(event);
}

InputCompactEvent InputManager::ManageSDLEvent(WindowManager* window, const SDL_Event& ev)
{
  switch (ev.type)
  {
    case SDL_JOYAXISMOTION: return ManageAxisEvent(ev.jaxis);
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP: return ManageButtonEvent(ev.jbutton);
    case SDL_JOYHATMOTION: return ManageHatEvent(ev.jhat);
    case SDL_KEYDOWN:
    case SDL_KEYUP: return ManageKeyEvent(ev.key, ev.type == SDL_KEYDOWN);
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: return ManageMouseButtonEvent(ev.button, ev.type == SDL_MOUSEBUTTONDOWN);
    case SDL_MOUSEWHEEL: return ManageMouseWheelEvent(ev.wheel);
    case SDL_JOYDEVICEADDED:
    case SDL_JOYDEVICEREMOVED:
    {
      { LOG(LogInfo) << "[Input] Reinitialize because of joystick added/removed."; }
      SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
      SDL_InitSubSystem(SDL_INIT_JOYSTICK);
      Refresh(window, true);
      break;
    }
    default: break;
  }

  return {InputCompactEvent::Entry::Nothing, InputCompactEvent::Entry::Nothing, 0, mKeyboard, InputEvent() };
}

Path InputManager::ConfigurationPath()
{
  return RootFolders::DataRootFolder / "system/.emulationstation/es_input.cfg";
}

bool InputManager::ValidateContent(const String& content)
{
  XmlDocument gameList;
  XmlResult result = gameList.load_string(content.data());
  if (!result) { LOG(LogError) << "[Input] Validator cannot parse xml content"; return false; }
  return true;
}


bool InputManager::LookupDeviceXmlConfiguration(InputDevice& device)
{
  Path path = ConfigurationPath();
  if (!path.Exists()) return false;

  String content;
  if (!SecuredFile::LoadSecuredFile(true, path, Path::Empty, content, "Pad Configuration", true, this))
  {
    { LOG(LogError) << "[Input] Cannot load input configuration file"; }
    return false;
  }

  pugi::xml_document doc;
  pugi::xml_parse_result res = doc.load_string(content.data());
  if (!res)
  {
    { LOG(LogError) << "[Input] Error parsing input config: " << res.description(); }
    return false;
  }

  { LOG(LogDebug) << "[Input] Looking for configuration for " << device.Name() << " (UUID: " << device.GUID()
                  << ") - Axis: " << device.AxeCount()
                  << " - Hats: " << device.HatCount()
                  << " - Buttons: " << device.ButtonCount(); }

  pugi::xml_node root = doc.child("inputList");
  if (root == nullptr) return false;

  // An entry reserved to this very port wins over the one every twin shares
  if (!device.PhysicalPath().empty())
    for (pugi::xml_node item = root.child("inputConfig"); item != nullptr; item = item.next_sibling("inputConfig"))
      if (MatchesSignature(device, item) &&
          device.PhysicalPath() == item.attribute("devicePhysicalPath").value())
        return LoadDeviceXmlEntry(device, item, true);

  // Any entry of that model otherwise: a twin keeps working on the configuration of its sibling
  for (pugi::xml_node item = root.child("inputConfig"); item != nullptr; item = item.next_sibling("inputConfig"))
    if (MatchesSignature(device, item))
      return LoadDeviceXmlEntry(device, item, false);

  return false;
}

bool InputManager::MatchesSignature(const InputDevice& device, pugi::xml_node item)
{
  // check the guid
  bool guid;
  if (String(item.attribute("version").value()) == "2" || device.IsKeyboard())
  {
    // sdl 2.26+
    String tempGUID(item.attribute("deviceGUID").as_string());
    guid = device.GUID().LowerCase() == tempGUID.LowerCase();
  }else {
    // sdl 2.0
    String CRC = String::ToHexa(gen_crc16((uint8_t*)item.attribute("deviceName").value(), strlen(item.attribute("deviceName").value()), true), 4, String::Hexa::None);
    String tempGUID = "";
    if (strlen(item.attribute("deviceGUID").value()) > 8)
      tempGUID = String(item.attribute("deviceGUID").value()).replace(4, 4, CRC);
    guid = (device.GUID().ToLowerCase() == tempGUID.ToLowerCase());
  }
  bool axes    = (device.AxeCount() == item.attribute("deviceNbAxes").as_int()) || device.IsKeyboard();
  bool hats    = (device.HatCount() == item.attribute("deviceNbHats").as_int()) || device.IsKeyboard();
  bool buttons = (device.ButtonCount() == item.attribute("deviceNbButtons").as_int()) || device.IsKeyboard();
  return guid && axes && hats && buttons;
}

bool InputManager::LoadDeviceXmlEntry(InputDevice& device, pugi::xml_node item, bool portBound)
{
  int loaded = device.LoadFromXml(item);
  if (loaded) device.SetPadType(PadTypes::FromString(item.attribute("gamepadtype").as_string()));
  device.SetPortBound(portBound);
  { LOG(LogDebug) << "[Input] Loaded"
                  << " UUID: " << item.attribute("deviceGUID").value()
                  << " - GamepadType: " << item.attribute("gamepadtype").value()
                  << " - Axis: " << item.attribute("deviceNbAxes").as_int()
                  << " - Hats: " << item.attribute("deviceNbHats").as_int()
                  << " - Buttons: " << item.attribute("deviceNbButtons").as_int()
                  << " - Port: " << (portBound ? item.attribute("devicePhysicalPath").value() : "any")
                  << " : " << loaded << " config. entries."; }
  return true;
}

bool InputManager::HasConnectedTwin(const InputDevice& device) const
{
  for(const auto& entry : mIdToDevices)
  {
    const InputDevice* other = entry.second;
    if (other == nullptr || other->Identifier() == device.Identifier()) continue;
    if (other->GUID() == device.GUID() &&
        other->AxeCount() == device.AxeCount() &&
        other->HatCount() == device.HatCount() &&
        other->ButtonCount() == device.ButtonCount())
      return true;
  }
  return false;
}

void InputManager::WriteDeviceXmlConfiguration(InputDevice& device)
{
  // Twins may be wired differently: once one is around, this belongs to the port
  device.SetPortBound(!device.PhysicalPath().empty() &&
                      (device.IsPortBound() || HasConnectedTwin(device)));

  Path path = ConfigurationPath();
  pugi::xml_document doc;
  if (path.Exists())
  {
    pugi::xml_parse_result result = doc.load_file(path.ToChars());
    if (!result) { LOG(LogError) << "[Input] Error parsing input config: " << result.description(); }
    else
    {
      // Delete the entry this one replaces, and only that one
      pugi::xml_node root = doc.child("inputList");
      if (root != nullptr)
        for (pugi::xml_node item = root.child("inputConfig"); item != nullptr; item = item.next_sibling("inputConfig"))
          if (MatchesSignature(device, item))
          {
            pugi::xml_attribute port = item.attribute("devicePhysicalPath");
            bool target = device.IsPortBound() ? (!port.empty() && device.PhysicalPath() == port.value())
                                               : port.empty();
            if (target)
            {
              root.remove_child(item);
              break;
            }
          }
    }
  }

  pugi::xml_node root = doc.child("inputList");
  if (!root) root = doc.append_child("inputList");

  device.SaveToXml(root);

  /*
   * Custom thread-safe writer
   */
  struct XmlWriter : public pugi::xml_writer
  {
    String mOutput;
    void write(const void* data, size_t size) override { mOutput.Append((const char*)data, (int)size); }
  }
  Writer;

  doc.save(Writer);
  if (!SecuredFile::SaveSecuredFile(path, Writer.mOutput, "Pad Configuration", true, this))
  { LOG(LogError) << "[Input] Error saving input configuration file"; }
}

OrderedDevices InputManager::GetMappedDeviceList(const InputMapper& mapper)
{
  OrderedDevices devices;
  InputMapper::PadList list = mapper.GetPads();
  for (int player = 0; player < (int)list.size(); ++player)
  {
    const InputMapper::Pad& pad = list[player];
    if (pad.IsConnected())
      devices.SetDevice(player, *mIdToDevices[mIndexToId[pad.mIndex]]);
  }

  return devices;
}

String InputManager::GetMappedDeviceListConfiguration(const InputMapper& mapper, bool lightgunGame)
{
  String command;
  InputMapper::PadList list = mapper.GetPads(lightgunGame ? InputMapper::LightGuns : InputMapper::Controllers);
  if (list.empty())
    list = mapper.GetPads();
  for (int player = 0; player < (int)list.size(); ++player)
  {
    const InputMapper::Pad& pad = list[player];
    if (pad.IsConnected())
    {
      const InputDevice& device = *mIdToDevices[mIndexToId[pad.mIndex]];
      String p(" -p"); p.Append(player + 1);
      command.Append(p).Append("index ").Append(device.Index())
             .Append(p).Append("guid ").Append(device.GUID())
             .Append(p).Append("name \"").Append(device.Name()).Append('"')
             .Append(p).Append("nbaxes ").Append(device.AxeCount())
             .Append(p).Append("nbhats ").Append(device.HatCount())
             .Append(p).Append("nbbuttons ").Append(device.ButtonCount())
             .Append(p).Append("devicepath ").Append(device.UDevPath().ToString())
             .Append(p).Append("physicalpath \"").Append(device.PhysicalPath()).Append('"');
    }
  }
  { LOG(LogInfo) << "[Input] Configure emulators command : " << command; }
  return command;
}

void InputManager::LogRawEvent(const InputEvent& event)
{
  { LOG(LogDebug) << "[Input] Raw Event: " << event.ToString(); }
}

void InputManager::LogCompactEvent(const InputCompactEvent& event)
{
  { LOG(LogDebug) << "[Input] Compact Event: " << event.ToString(); }
}

void InputManager::AddNotificationInterface(IInputChange* interface)
{
  if (!mNotificationInterfaces.Contains(interface))
    mNotificationInterfaces.Add(interface);
}

void InputManager::RemoveNotificationInterface(IInputChange* interface)
{
  int index = mNotificationInterfaces.IndexOf(interface);
  if (index >= 0)
    mNotificationInterfaces.Delete(index);
}

void InputManager::FileSystemWatcherNotification(FileSystemWatcher& source, const Path& path, EventType event, const DateTime& time)
{
  (void)source;
  (void)time;
  { LOG(LogWarning) << "[/dev/input] Event " << (int)event << " : " << path.ToString(); }
  if (path.Filename().StartsWith("js"))
    if (path.Filename().AsInt(2, -1) != -1)
    {
      mJoystickChangePending = true;
      mJoystickChangePendingRemoved = (event == EventType::Remove);
    }
}

void InputManager::WatchJoystickAddRemove(WindowManager* window)
{
  if (mJoystickChangePending)
  {
    // Reset Joystick to force refresh
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    Refresh(window, true);
    mJoystickChangePending = false;
  }
}

void InputManager::KeepDifferentPads(std::vector<InputDevice>& left, std::vector<InputDevice>& right)
{
  for (int l = (int)left.size(); --l >= 0;)
  {
    const InputDevice& leftDevice = left[l];
    for(int r = (int)right.size(); --r >= 0; )
    {
      const InputDevice& rightDevice = right[r];
      if (leftDevice.EqualsTo(rightDevice))
      {
        left.erase(left.begin() + l);
        right.erase(right.begin() + r);
        break;
      }
    }
  }
}

int InputManager::ConfigurableDeviceCount() const
{
  int count = 0;
  for(const auto& device : mIdToDevices)
    if(device.second->IsConfigurable())
      count++;
  return count;
}

void InputManager::GetRawEvents(Array<SDL_Event>& to)
{
  for(auto& device : mIdToDevices)
    device.second->GetRawButtonEvents(to).GetRawHatEvents(to);
}

String InputManager::RemapTextInput(const char* text) const
{
  return mKeyboardLayoutMapper.RemapText(text, mAltGrPressed);
}


