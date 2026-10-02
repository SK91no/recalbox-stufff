//
// Created by bkg2k on 02/06/25.
//

#include <utils/String.h>
#include "SupportTypes.h"

String SupportTypeToString(SupportTypes supporttype)
{
  switch(supporttype)
  {
    case SupportTypes::Cartridge: { static String s("Cartridge"); return s; }
    case SupportTypes::CD: { static String s("CD/DVD"); return s; }
    case SupportTypes::HDD: { static String s("Harddisk"); return s; }
    case SupportTypes::FilesAndFolders: { static String s("Files"); return s; }
    case SupportTypes::Tape: { static String s("Tape"); return s; }
    case SupportTypes::QuickDisk: { static String s("Quick Disc"); return s; }
    case SupportTypes::Floppy3: { static String s("3\" Floppy"); return s; }
    case SupportTypes::Floppy35: { static String s("3\".5 Floppy"); return s; }
    case SupportTypes::Floppy525: { static String s("5\".25 Floppy"); return s; }
    case SupportTypes::PCB: { static String s("PCB"); return s; }
    case SupportTypes::Microdisc: { static String s("Microdisc"); return s; }
    case SupportTypes::Unknown:
    default: break;
  }

  static String s("Unknown");
  return s;
}
