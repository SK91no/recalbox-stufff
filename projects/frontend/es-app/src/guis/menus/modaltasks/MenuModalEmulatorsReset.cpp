//
// Created by bkg2k on 25/11/24.
//

#include "MenuModalEmulatorsReset.h"

bool MenuModalEmulatorsReset::TaskExecute(const bool& parameter)
{
  (void)parameter;
  String::List deleteMe
  ({
     ".cache",        // All caches
     ".local",        // Come obscure emulator configs
     ".config",       // emulator configurations #1
     "configs",       // emulator configurations #2
     ".atari800.cfg", // Atari 8bit config
     ".hatari",       // Hatari standalone config
     ".crocods",      // Amstrad
     ".advance",      // AdvanceMame
     ".dosbox",       // Standalone dosbox
     ".simcoupe",     // SAM Coupé
   });

  Path share("/recalbox/share/system");
  Path shareInit("/recalbox/share_init/system");

  bool result = true;

  // Deleting current settings
  for(const String& path : deleteMe)
    if (Path fullpath(share / path); system(String("rm -rf ").Append(fullpath.ToString()).data()) != 0)
    { LOG(LogError) << "[ResetFactory] Error removing folder " << path; result = false; }
    else { sync(); Thread::Sleep(200); }

  // Copy default values
  for(const String& path : deleteMe)
    if (Path fullpath(shareInit / path); fullpath.Exists())
      if (system(String("cp -r ").Append(fullpath.ToString()).Append(' ').Append(share.ToString()).data()) != 0)
      { LOG(LogError) << "[ResetFactory] Error copying folder/file " << path; result = false; }

  return result;
}

void MenuModalEmulatorsReset::TaskComplete(const bool& result)
{
  if (result)
    mWindow.pushGui(new GuiMsgBox(mWindow, _("ALL YOUR EMULATOR SETTINGS HAVE BEEN RESET TO THEIR FACTORY DEFAULTS."), _("OK"), nullptr));
  else
    mWindow.pushGui(new GuiMsgBox(mWindow, _("SOME ERROR OCCURRED WHILE RESETING ALL YOUR EMULATOR SETTINGS.\n\nIF YOU STILL HAVE ISSUES WITH SOME EMULATORS, REBOOT YOUR RECALBOX AND TRY RESETING EMULATOR SETTINGS AGAIN."), _("OK"), nullptr));
}
