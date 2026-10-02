//
// Created by bkg2k on 13/01/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include <guis/menus/MenuPadsPair.h>
#include <recalbox/RecalboxSystem.h>
#include <guis/GuiMsgBox.h>
#include <utils/locale/LocaleHelper.h>

MenuPadsPair::MenuPadsPair(WindowManager& window, const String::List& deviceList)
  : Menu(window, InheritableContext(), _("PAIR BLUETOOTH CONTROLLERS"))
  , mDevices(deviceList)
{
}

void MenuPadsPair::BuildMenuItems()
{
  int index = -1;
  for (const auto & controllerString : mDevices)
    AddAction(controllerString, _("PAIR"), ++index, true, this, String::Empty);
}

bool MenuPadsPair::Execute(GuiWaitLongExecution<String, bool>& from, const String& parameter)
{
  (void)from;
  return RecalboxSystem::pairBluetooth(parameter);
}

void MenuPadsPair::Completed(const String& parameter, const bool& result)
{
  (void)parameter;
  mWindow.pushGui(new GuiMsgBox(mWindow, result ? _("CONTROLLER PAIRED") : _("UNABLE TO PAIR CONTROLLER"), _("OK")));
}

void MenuPadsPair::MenuActionTriggered(ItemAction& item, int id)
{
  (void)item;
  String device = mDevices[id];
  String text = _("PAIRING %s ...").Replace("%s", device);
  mWindow.pushGui((new GuiWaitLongExecution<String, bool>(mWindow, *this))->Execute(device, text));
}
