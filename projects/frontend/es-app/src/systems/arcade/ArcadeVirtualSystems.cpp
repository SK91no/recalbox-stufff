//
// Created by bkg2k on 07/06/23.
//

#include "ArcadeVirtualSystems.h"
#include "utils/storage/HashMap.h"
#include <utils/String.h>
#include <hardware/BoardType.h>
#include <hardware/Board.h>

const String::List& ArcadeVirtualSystems::GetVirtualArcadeSystemList()
{
  static String::List sList
  {
    "acclaim",
    "atari",
    "atlus",
    "banpresto",
    "capcom",
    "capcom\\cps1",
    "capcom\\cps2",
    "capcom\\cps3",
    "cave",
    "data\u00a0east",
    "exidy",
    "hng64",
    "igs",
    "irem",
    "irem\\m72",
    "irem\\m92",
    "itech",
    "jaleco",
    "kaneko",
    "konami",
    "konami\\gx",
    "midway",
    "mitchell",
    "namco",
    "namco\\na",
    "namco\\nb",
    "namco\\system1",
    "namco\\system2",
    "namco\\system10",
    "namco\\system11",
    "namco\\system12",
    "neogeo",
    "nichibutsu",
    "nintendo",
    "nmk",
    "psikyo",
    "raizing",
    "sammy",
    "sega",
    "sega\\system16",
    "sega\\system18",
    "sega\\system32",
    "seibu",
    "seta",
    "snk",
    "taito",
    "taito\\f3",
    "taito\\gnet",
    "technos",
    "tecmo",
    "toaplan",
    "visco",
  };

  static bool dynamicInjectionOnce = false;
  if (!dynamicInjectionOnce)
  {
    if (Board::Instance().GetBoardType() == BoardType::PCx64)
      sList.push_back("sega\\stv");
    dynamicInjectionOnce = true;
  }
  return sList;
}

const std::vector<std::pair<String, String>>& ArcadeVirtualSystems::GetVirtualArcadeSystemListExtended()
{
  static std::vector<std::pair<String, String>> sList
    {
      { "acclaim",        "Acclaim" },
      { "atari",          "Atari" },
      { "atlus",          "Atlus" },
      { "banpresto",      "BanPresto" },
      { "capcom",         "Capcom" },
      { "capcom\\cps1",   "Capcom CPS1" },
      { "capcom\\cps2",   "Capcom CPS2" },
      { "capcom\\cps3",   "Capcom CPS3" },
      { "cave",           "CAVE" },
      { "data\u00a0east", "Data East" },
      { "exidy",          "Exidy" },
      { "hng64",          "Hyper Neo-Geo 64" },
      { "igs",            "International Games System (I.G.S)" },
      { "irem",           "IREM Software Engineering" },
      { "irem\\m72",      "IREM M72" },
      { "irem\\m92",      "IREM M92" },
      { "itech",          "Incredible Technologies" },
      { "jaleco",         "Jaleco" },
      { "kaneko",         "Kaneko" },
      { "konami",         "Konami" },
      { "konami\\gx",     "Konami GX" },
      { "midway",         "Midway" },
      { "mitchell",       "Mitchell" },
      { "namco",          "NAMCO" },
      { "namco\\na",      "NAMCO NA" },
      { "namco\\nb",      "NAMCO NB" },
      { "namco\\system1", "NAMCO System 1" },
      { "namco\\system2", "NAMCO System 2" },
      { "namco\\system10","NAMCO System 10" },
      { "namco\\system11","NAMCO System 11" },
      { "namco\\system12","NAMCO System 12" },
      { "neogeo",         "Neo-Geo" },
      { "nichibutsu",     "Nichibutsu" },
      { "nintendo",       "Nintendo" },
      { "nmk",            "NMK" },
      { "psikyo",         "Psikyo" },
      { "raizing",        "Raizing" },
      { "sammy",          "Sammy" },
      { "sega",           "SEGA" },
      { "sega\\system16", "SEGA System 16" },
      { "sega\\system18", "SEGA System 18" },
      { "sega\\system32", "SEGA System 32" },
      { "seibu",          "Seibu" },
      { "seta",           "SETA" },
      { "snk",            "SNK" },
      { "taito",          "TAITO" },
      { "taito\\f3",      "TAITO F3" },
      { "taito\\gnet",    "TAITO G-NET" },
      { "technos",        "Technōs" },
      { "tecmo",          "TECMO" },
      { "toaplan",        "TOAPLAN" },
      { "visco",          "VISCO" },
    };
  if (Board::Instance().GetBoardType() == BoardType::PCx64)
	  sList.push_back({ "sega\\stv",      "SEGA ST-V" });
  return sList;
}

/*String ArcadeVirtualSystems::GetRealName(const String& driverName)
{
  static HashMap<String, String> sList
  {
    { "f32", "F2 System" },
    { "funworld", "Fun World" },
    { "fuuki", "Fuuki" },
    { "gaelco", "Gaelco" },
    { "gameplan", "Game Plan" },
    { "gametron", "Game-A-Tron" },
    { "gottlieb", "Gottlieb" },
    { "ice", "ICE" },
    { "igs", "IGS" },
    { "igt", "IGT - International Game Technology" },
    { "irem", "Irem" },
    { "itech", "Incredible Technologies" },
    { "jaleco", "Jaleco" },
    { "jpm", "JPM" },
    { "kaneko", "Kaneko" },
    { "kiwako", "Kiwako" },
    { "konami", "Konami" },
    { "matic", "Eletro Matic Equipamentos Eletromecânicos" },
    { "maygay", "Maygay" },
    { "meadows", "Meadows Games, Inc." },
    { "merit", "Merit" },
    { "metro", "Metro" },
    { "midcoin", "Midcoin" },
    { "midw8080", "Midway" },
    { "midway", "Midway Games" },
    { "miltonbradley", "Roy Abel & Associates" },
    { "mr", "Model Racing" },
    { "namco", "Namco" },
    { "neogeo", "SNK NeoGeo" },
    { "nichibutsu", "Nichibutsu" },
    { "nintendo", "Nintendo" },
    { "nix", "NIX" },
    { "nmk", "NMK" },
    { "olympia", "Olympia" },
    { "omori", "Omori Electric Co., Ltd." },
    { "orca", "Orca" },
    { "pacific", "Pacific Novelty" },
    { "philips", "Philips" },
    { "playmark", "Playmark" },
    { "promat", "Promat" },
    { "psikyo", "Psikyo" },
    { "ramtek", "Ramtek" },
    { "rare", "Rare" },
    { "sanritsu", "Sanritsu" },
    { "sega", "Sega" },
    { "seibu", "Seibu Kaihatsu" },
    { "seta", "Seta" },
    { "sigma", "Sigma" },
    { "skeleton", "Inder" },
    { "snk", "SNK" },
    { "stern", "Stern Electronics" },
    { "subsino", "Subsino" },
    { "suna", "SunA" },
    { "sunelectronics", "Sun Electronics" },
    { "taito", "Taito" },
    { "tatsumi", "Tatsumi" },
    { "tch", "TCH" },
    { "tecfri", "Tecfri" },
    { "technos", "Technos Japan" },
    { "tecmo", "Tecmo" },
    { "thepit", "The Pit - Taito" },
    { "toaplan", "Toaplan" },
    { "unico", "Unico" },
    { "universal", "Universal" },
    { "upl", "UPL" },
    { "valadon", "Valadon Automation" },
    { "venture", "Venture Line" },
    { "vsystem", "V-System Co." },
    { "wing", "Wing" },
    { "yunsung", "Yun Sung" },
    { "zaccaria", "Zaccaria" },
  };

  String* result = sList.try_get(driverName);
  if (result != nullptr) return *result;

  // Check separator and try to isolate constructor/system
  // The return "Constructor name - SYSTEM"
  if (driverName.Contains('/'))
    if (int separator = driverName.Find('/'); separator > 0)
      if (result = sList.try_get(driverName.SubString(0, separator)); result != nullptr)
        return String(*result).Append(" - ").Append(driverName.SubString(separator + 1).ToUpperCaseUTF8());

  return driverName;
}*/


