//
// Created by bkg2k on 12/01/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "MenuPads.h"
#include "MenuPadsPair.h"
#include "guis/GuiBluetoothDevices.h"
#include <guis/MenuMessages.h>
#include <guis/GuiMsgBox.h>
#include <recalbox/RecalboxSystem.h>
#include <guis/GuiDetectDevice.h>
#include <algorithm>

MenuPads::MenuPads(WindowManager& window)
  : Menu(window, InheritableContext(), _("CONTROLLERS SETTINGS"))
  , mMapper(InputManager::Instance().Mapper())
  , mRefreshing(false)
{
  // Subscribe refresh event
  InputManager::Instance().AddNotificationInterface(this);

  // Force OSD when thius menu is on
  mWindow.OSD().GetPadOSD().ForcedPadOSDActivation(true);
}

void MenuPads::BuildMenuItems()
{
  // Configure a pad
  if(InputManager::Instance().ConfigurableDeviceCount() > 0)
    AddSubMenu(_("CONFIGURE A CONTROLLER"), (int)Components::Configure, this, _(MENUMESSAGE_CONTROLLER_CONF_HELP_MSG));

  // Test a pad (read-only: lights up the configured bindings as the user presses them)
  if(InputManager::Instance().ConfigurableDeviceCount() > 0)
    AddSubMenu(_("TEST A CONTROLLER"), (int)Components::Test, this);

  // Pair a pad
  AddSubMenu(_("PAIR BLUETOOTH CONTROLLERS"), (int)Components::Pair, this, _(MENUMESSAGE_CONTROLLER_BT_HELP_MSG));

  // Unpair all pads
  AddSubMenu(_("FORGET BLUETOOTH CONTROLLERS"), (int)Components::Unpair, this, _(MENUMESSAGE_CONTROLLER_FORGET_HELP_MSG));

  // Automatic pairing on boot
  AddSwitch(_("AUTO PAIR ON BOOT"), RecalboxConf::Instance().GetAutoPairOnBoot(), (int)Components::AutoPairOnBoot, this, _(MENUMESSAGE_CONTROLLER_AUTOPAIRONBOOT_HELP_MSG));

  // Pad OSD always on
  AddSwitch(_("ALWAYS SHOW PAD OSD"), RecalboxConf::Instance().GetPadOSD(), (int)Components::PadOSD, this, _(MENUMESSAGE_CONTROLLER_PADOSD_HELP_MSG));

  // Pad OSD type
  AddList<RecalboxConf::PadOSDType>(_("PAD OSD TYPE"), (int)Components::PadOSDType, this, GetPadOSDType(), RecalboxConf::Instance().GetPadOSDType(), RecalboxConf::PadOSDType::Snes, _(MENUMESSAGE_CONTROLLER_PADOSDTYPE_HELP_MSG));

  // Rumble
  AddSwitch(_("RUMBLE"), RecalboxConf::Instance().GetControllersRumble(), (int)Components::Rumble, this, _(MENUMESSAGE_CONTROLLER_RUMBLE_HELP_MSG));

  // Pad list
  SelectorEntry<int>::List entries = GetDeviceList();
  for(int i = 0; i < Input::sMaxInputDevices; ++i)
  {
    static const char* balls[10] ={ "\u2776", "\u2777", "\u2778", "\u2779", "\u277a", "\u277b", "\u277c", "\u277d", "\u277e", "\u277f" };
    String name(balls[i]);
    name.Append(' ').Append(_("INPUT P%i")).Replace("%i", String(i + 1));
    mDevices[i] = AddList<int>(name, (int)Components::Pads + i, this, entries, SlotInitialValue(i), -1);
  }
}

MenuPads::~MenuPads()
{
  InputManager::Instance().RemoveNotificationInterface(this);
  mWindow.OSD().GetPadOSD().ForcedPadOSDActivation(false);
}

void MenuPads::PadsAddedOrRemoved(bool removed)
{
  (void)removed;
  RefreshDevices();
}

void MenuPads::StartConfiguring()
{
  RunDeviceDetection(this);
  /*Gui* msgBox = new GuiMsgBox(mWindow,
                              _("YOU ARE GOING TO CONFIGURE A CONTROLLER. IF YOU HAVE ONLY ONE JOYSTICK, "
                                "CONFIGURE THE DIRECTIONS KEYS AND SKIP JOYSTICK CONFIG BY HOLDING A BUTTON. "
                                "IF YOU DO NOT HAVE A SPECIAL KEY FOR HOTKEY, CHOOSE THE SELECT BUTTON. SKIP "
                                "ALL BUTTONS YOU DO NOT HAVE BY HOLDING A KEY. BUTTONS NAMES ARE BASED ON THE "
                                "SNES CONTROLLER."), _("OK"), std::bind(MenuPads::RunDeviceDetection, this));
  mWindow.pushGui(msgBox);*/
}

void MenuPads::RunDeviceDetection(MenuPads* thiz)
{
  thiz->mWindow.pushGui(new GuiDetectDevice(thiz->mWindow, false, thiz));
}

void MenuPads::StartTesting()
{
  RunDeviceDetectionForTest(this);
}

void MenuPads::RunDeviceDetectionForTest(MenuPads* thiz)
{
  thiz->mWindow.pushGui(new GuiDetectDevice(thiz->mWindow, false, thiz, /*testOnly=*/true));
}

void MenuPads::DeviceConfigured(const InputDevice& device)
{
  (void)device;
  RefreshDevices();
}

String::List MenuPads::Execute(GuiWaitLongExecution<bool, String::List>& from, const bool& parameter)
{
  (void)from;
  (void)parameter;
  return RecalboxSystem::scanBluetooth();
}

void MenuPads::Completed(const bool& parameter, const String::List& result)
{
  (void)parameter;
  mWindow.pushGui(result.empty()
                  ? (Gui*)new GuiMsgBox(mWindow, _("NO CONTROLLERS FOUND"), _("OK"))
                  : (Gui*)new MenuPadsPair(mWindow, result));
}

void MenuPads::StartScanningDevices()
{
  BTAutopairManager::Instance().StartDiscovery();
  mWindow.pushGui(new GuiBluetoothDevices(mWindow));
}

void MenuPads::UnpairAll()
{
  RecalboxSystem::forgetBluetoothControllers();
  mWindow.pushGui(new GuiMsgBox(mWindow, _("CONTROLLERS LINKS HAVE BEEN DELETED."), _("OK")));
}

SelectorEntry<int>::List MenuPads::GetDeviceList()
{
  SelectorEntry<int>::List result;
  mValueToUuid.clear();
  /* Entry value = pad's mPosition + parallel UUID snapshot, so MenuSingleChanged
   * can resolve the chosen pad by stable identity if mPads gets reshuffled
   * between popup open and click (e.g. hot-plug/unplug). */
  for (const InputMapper::Pad& p : mMapper.GetPads())
    if (p.IsConnected())
    {
      result.push_back({ mMapper.GetDecoratedName(p.mPosition), p.mPosition });
      mValueToUuid[p.mPosition] = p.mUUID;
    }
  std::sort(result.begin(), result.end(),
            [](const SelectorEntry<int>& a, const SelectorEntry<int>& b)
            { return a.mText < b.mText; });
  result.push_back({ _("NONE"), -1 });
  return result;
}

int MenuPads::SlotInitialValue(int slot) const
{
  // Row N pre-selects the Nth connected pad (historical UX).
  InputMapper::PadList list = mMapper.GetPads();
  if (slot < (int)list.size()) return list[slot].mPosition;
  return -1;
}

void MenuPads::RefreshDevices()
{
  SelectorEntry<int>::List entries = GetDeviceList();
  for(int i = 0; i < Input::sMaxInputDevices; ++i)
    mDevices[i]->ChangeSelectionItems(entries, SlotInitialValue(i), -1);
}

void MenuPads::SubMenuSelected(const ItemSubMenu& item, int id)
{
  (void)item;
  switch((Components)id)
  {
    case Components::Configure: StartConfiguring(); break;
    case Components::Test: StartTesting(); break;
    case Components::Pair:
    {
      Gui* msgBox = new GuiMsgBox(mWindow,
                                  _("The bluetooth pairing will start and run for several minutes.\n"
                                  "During this time, you just have to apply the pairing procedure on any bluetooth controller you want to pair.\n"
                                  "The next window will display all detected bluetooth devices and their status for information purposes only.\n"
                                  "You can close it at any time, while continuing to pair your bluetooth devices for as long as the bluetooth icon is blinking on the top left."),
                                  _("OK"), [this] { StartScanningDevices(); });
      mWindow.pushGui(msgBox);
      break;
    }
    case Components::Unpair: UnpairAll(); break;
    case Components::Pads:
    case Components::Driver:
    case Components::PadOSD:
    case Components::PadOSDType:
    case Components::AutoPairOnBoot:
    case Components::Rumble:
    default: break;
  }
}

void MenuPads::MenuSingleChanged(ItemSelectorBase<int>& item, int id, int index, const int& value)
{
  (void)item;
  (void)index;
  if (mRefreshing) return;
  if (value < 0) return; // NONE: nothing to do

  /* Resolve `value` (the pad's mPosition at popup creation) to the pad's
   * current mPosition by UUID — robust if mPads got reshuffled between popup
   * open and click (hot-plug/unplug). */
  String* uuid = mValueToUuid.try_get(value);
  if (uuid == nullptr) return;
  int currentPos = -1;
  for (const InputMapper::Pad& p : mMapper.GetPads())
    if (p.IsConnected() && p.mUUID == *uuid) { currentPos = p.mPosition; break; }
  if (currentPos < 0) return; // chosen pad no longer connected
  mMapper.Swap(id, currentPos);
  RefreshDevices();
}

void MenuPads::MenuSingleChanged(ItemSelectorBase<String>& item, int id, int index, const String& value)
{
  (void)item;
  (void)index;
  if ((Components)id == Components::Driver)
    RecalboxConf::Instance().SetGlobalInputDriver(value).Save();
}

void MenuPads::MenuSwitchChanged(const ItemSwitch& item, bool& status, int id)
{
  (void)item;
  if ((Components)id == Components::PadOSD)
    RecalboxConf::Instance().SetPadOSD(status).Save();
  if ((Components)id == Components::AutoPairOnBoot)
    RecalboxConf::Instance().SetAutoPairOnBoot(status).Save();
  if ((Components)id == Components::Rumble)
    RecalboxConf::Instance().SetControllersRumble(status).Save();
}

void MenuPads::MenuSingleChanged(ItemSelectorBase<RecalboxConf::PadOSDType>& item, int id, int index, const RecalboxConf::PadOSDType& value)
{
  (void)item;
  (void)index;
  if ((Components)id == Components::PadOSDType)
  {
    RecalboxConf::Instance().SetPadOSDType(value).Save();
    mWindow.OSD().GetPadOSD().UpdatePadIcon();
  }
}

SelectorEntry<RecalboxConf::PadOSDType>::List MenuPads::GetPadOSDType()
{
  return SelectorEntry<RecalboxConf::PadOSDType>::List
  {
    { "Nintendo SNES", RecalboxConf::PadOSDType::Snes },
    { "Nintendo N64", RecalboxConf::PadOSDType::N64 },
    { "Sega Megadrive", RecalboxConf::PadOSDType::MD },
    { "Sega Dreamcast", RecalboxConf::PadOSDType::DC },
    { "Sony Playstation", RecalboxConf::PadOSDType::PSX },
    { "Microsoft XBox", RecalboxConf::PadOSDType::XBox },
  };
}

