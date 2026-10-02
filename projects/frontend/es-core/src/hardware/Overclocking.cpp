//
// Created by bkg2k on 26/11/24.
//

#include "Overclocking.h"
#include "utils/Files.h"
#include "utils/locale/LocaleHelper.h"
#include "recalbox/RecalboxSystem.h"
#include <hardware/BoardType.h>
#include <hardware/Board.h>

void Overclocking::Load()
{
  // Build final path to fetch overclocking configs from
  Path basePath(sOverclockBaseFolder);
  String boardFolder;
  switch(Board::Instance().GetBoardType())
  {
    case BoardType::Pi0: boardFolder = "rpi0"; break;
    case BoardType::Pi02: boardFolder = "rpi02"; break;
    case BoardType::Pi1: boardFolder = "rpi1"; break;
    case BoardType::Pi2: boardFolder = "rpi2"; break;
    case BoardType::Pi3: boardFolder = "rpi3"; break;
    case BoardType::Pi3plus: boardFolder = "rpi3plus"; break;
    case BoardType::Pi4: boardFolder = "rpi4"; break;
    case BoardType::Pi400: boardFolder = "rpi400"; break;
    case BoardType::Pi5: boardFolder = "rpi5"; break;
    case BoardType::Pi500: boardFolder = "rpi500"; break;
    case BoardType::UndetectedYet:
    case BoardType::Unknown:
    case BoardType::PiUnknown:
    case BoardType::OdroidAdvanceGo:
    case BoardType::OdroidAdvanceGoSuper:
    case BoardType::OdroidUnknown:
    case BoardType::RG351P:
    case BoardType::RG353P:
    case BoardType::RG353V:
    case BoardType::RG353M:
    case BoardType::RG503:
    case BoardType::RG351V:
    case BoardType::AnbernicUnknown:
    case BoardType::PCx86:
    case BoardType::PCx64:
    case BoardType::SteamDeckOLED:
    case BoardType::SteamDeckLCD:
      break;
  }
  { LOG(LogDebug) << "[Overclock] Board: " << (int)Board::Instance().GetBoardType(); }
  if (boardFolder.empty()) return;
  Path finalPath = basePath / boardFolder;
  { LOG(LogDebug) << "[Overclock] Final source folder: " <<finalPath.ToString(); }

  // Get O/C config list
  Path::PathList list = finalPath.GetDirectoryContent();

  // Load and get description
  for(const Path& path : list)
  {
    String trash;
    String desc;
    bool hazard = false;
    int freq = 0;
    // Extract lines - doesn't matter if the file does not load, it returns an empty string
    for(const String& line : Files::LoadFile(path).Split('\n'))
    {
      if (line.StartsWith(LEGACY_STRING("# Description: "))) (void)line.Extract(':', trash, desc, true);
      else if (line.StartsWith(LEGACY_STRING("# Warning")))  hazard = true;
      else if (line.StartsWith(LEGACY_STRING("# Frequency: ")))
      {
        String freqStr;
        (void)line.Extract(':', trash,freqStr, true);
        (void)freqStr.TryAsInt(freq);
      }
    }
    // Record?
    if (!desc.empty() && freq != 0)
    {
      mOverclocks.push_back({ path, desc, freq, hazard });
      { LOG(LogInfo) << "[Overclock] File " << path.ToString() << " validated."; }
    }
    else { LOG(LogError) << "[Overclock] Invalid file " << path.ToString(); }
  }
}

bool Overclocking::Install(const Overclocking::Overclock& oc)
{
  RecalboxConf::Instance().SetOverclocking(oc.FilePath().ToString()).Save();

  if (RecalboxSystem::MakeBootReadWrite())
  {
    Files::SaveFile(Path(sOverclockFile), Files::LoadFile(oc.FilePath()));
    RecalboxSystem::MakeBootReadOnly();
    { LOG(LogInfo) << "[Overclock] Overclocking set to " << oc.Description(); }
    return true;
  }
  { LOG(LogError) << "[Overclock] Cannot make boot read write"; }
  return false;
}

const Overclocking::Overclock& Overclocking::NoOverclock()
{
  static Overclock noOverclock { Path::Empty, _("NONE"), 0, false};
  return noOverclock;
}

Overclocking::Overclock Overclocking::GetCurrentOverclock() const
{
  String currentOC = RecalboxConf::Instance().GetOverclocking();
  for(const Overclock& overclock : mOverclocks)
    if (currentOC == overclock.FilePath().ToString())
      return overclock;
  return NoOverclock();
}

void Overclocking::Reset()
{
  Files::SaveFile(Path(sOverclockFile), String::Empty);
}
