//
// Created by bkg2k on 25/11/24.
//

#include "MenuModalFactoryReset.h"
#include "utils/Files.h"
#include <recalbox/RecalboxSystem.h>
#include <recalbox/BootConf.h>
#include <hardware/Board.h>

Path MenuModalFactoryReset::sFlagPath("/overlay/.configs/.files-pending");
HashMap<Path, Path> MenuModalFactoryReset::sBackups
{
  { Path("/recalbox/share/system/configs/retroarch/retroarchcustom.cfg.origin"), Path("/overlay/.configs/.retroarchcustom.cfg.origin.backup") },
};

bool MenuModalFactoryReset::TaskExecute(const bool& parameter)
{
  (void)parameter;
  String::List deleteMe
    ({
       "/recalbox/share/system",                       // Recalbox & emulator configurations
       "/overlay/upper/*",                             // System overlay
       "/overlay/.configs/*",                          // System configurations
       "/overlay/upper.old",                           // System overlay backup
       "/overlay/.config",                             // Old system configurations
       "/boot/recalbox-backup.conf",                   // Recalbox configuration backup
       "/boot/crt/",                                   // CRT Configuration
     });

  // Backup some files
  bool written = false;
  for(const auto& it : sBackups)
    if (it.first.Exists())
    {
      Files::SaveFile(it.second, Files::LoadFile(it.first));
      if (!written) { Files::SaveFile(sFlagPath, "1"); written = true; }
    }

  // Make boot partition writable
  if (!RecalboxSystem::MakeBootReadWrite())
  { LOG(LogError) << "[ResetFactory] Error making boot r/w"; }

  // Delete all required folder/files
  for(const String& path : deleteMe)
    if (system(String("rm -rf ").Append(path).data()) != 0)
    { LOG(LogError) << "[ResetFactory] Error removing folder " << path; }
    else Thread::Sleep(200);

  IniFile& recalboxBoot(BootConf::Instance());
  // Reset rotation
  recalboxBoot.SetString("screen.rotation", "0");
  // Special case for rpizero plus GPiCase2W, that cannot be detected. Can be removed after the rpizero image is frozen
  if(Board::Instance().GetBoardType() == BoardType::Pi0 && recalboxBoot.AsString("case").starts_with("GPi2W"))
    recalboxBoot.SetString("case", "GPi2W");
  else
    recalboxBoot.SetString("case", "");

  recalboxBoot.Save();

  // Reset!
  if (system("shutdown -r now") != 0)
  { LOG(LogError) << "[ResetFactory] Error rebooting system"; }

  return true;
}

void MenuModalFactoryReset::ReinstallFilesAfterReboot()
{
  // Backup some files
  if (sFlagPath.Exists())
  {
    for (const auto& it: sBackups)
      if (it.second.Exists())
      {
        Files::SaveFile(it.first, Files::LoadFile(it.second));
        (void) it.second.Delete();
      }
    (void)sFlagPath.Delete();
  }
}
