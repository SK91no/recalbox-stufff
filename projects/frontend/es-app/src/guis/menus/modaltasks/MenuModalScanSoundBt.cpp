//
// Created by bkg2k on 20/12/24.
//

#include "MenuModalScanSoundBt.h"
#include "recalbox/RecalboxSystem.h"
#include "guis/GuiMsgBox.h"
#include "guis/menus/MenuSoundPair.h"

String::List MenuModalScanSoundBT::TaskExecute(const bool& parameter)
{
  (void)parameter;
  return RecalboxSystem::scanBluetooth();
}

void MenuModalScanSoundBT::TaskComplete(const String::List& result)
{
  mWindow.pushGui(result.empty()
                  ? (Gui*)new GuiMsgBox(mWindow, _("NO AUDIO DEVICE FOUND"), _("OK"))
                  : (Gui*)new MenuSoundPair(mWindow, result));
}
