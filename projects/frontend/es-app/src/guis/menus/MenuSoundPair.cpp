//
// Created by davidb2111 on 31/08/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "MenuSoundPair.h"
#include "recalbox/RecalboxSystem.h"
#include "guis/GuiMsgBox.h"
#include "utils/locale/LocaleHelper.h"

MenuSoundPair::MenuSoundPair(WindowManager& window, const String::List& deviceList)
  : Menu(window, InheritableContext(), _("PAIR A BLUETOOTH AUDIO DEVICE"))
  , mDevices(deviceList)
{
}

void MenuSoundPair::BuildMenuItems()
{
  int index = -1;
  for (const auto & controllerString : mDevices)
    AddAction(controllerString, _("PAIR"), ++index, true, this, String::Empty);
}

bool MenuSoundPair::Execute(GuiWaitLongExecution<String, bool>& from, const String& parameter)
{
  (void)from;
  return RecalboxSystem::pairBluetooth(parameter);
}

void MenuSoundPair::Completed(const String& parameter, const bool& result)
{
  (void)parameter;
  mWindow.pushGui(new GuiMsgBox(mWindow, result ? _("AUDIO DEVICE PAIRED") : _("UNABLE TO PAIR AUDIO DEVICE"), _("OK")));
}

void MenuSoundPair::MenuActionTriggered(ItemAction& item, int id)
{
  (void)item;
  String device = mDevices[id];
  String text = _("PAIRING %s ...").Replace("%s", device);
  mWindow.pushGui((new GuiWaitLongExecution<String, bool>(mWindow, *this))->Execute(device, text));
}
