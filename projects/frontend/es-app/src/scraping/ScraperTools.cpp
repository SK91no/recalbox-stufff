//
// Created by bkg2k on 09/02/2020.
//
#include "ScraperTools.h"
#include "utils/math/Misc.h"

ScraperNameOptions ScraperTools::Clamp(int option)
{
  return (ScraperNameOptions)Math::clampi(option, (int)ScraperNameOptions::GetFromScraper, (int)ScraperNameOptions::GetFromFilenameUndecorated);
}
ScraperNameOptions ScraperTools::ScraperNameOptionsFromString(const String& menu)
{
  if (menu == "fileundecorated") return ScraperNameOptions::GetFromFilenameUndecorated;
  if (menu == "filename") return ScraperNameOptions::GetFromFilename;
  return ScraperNameOptions::GetFromScraper;
}

const String& ScraperTools::ScraperNameOptionsFromEnum(ScraperNameOptions scraperNameOptions)
{
  switch (scraperNameOptions)
  {
    case ScraperNameOptions::GetFromFilename: { static String sFileName = "filename"; return sFileName; }
    case ScraperNameOptions::GetFromFilenameUndecorated: { static String sFileName = "filenameundecorated"; return sFileName; }
    case ScraperNameOptions::GetFromScraper: break;
  }
  static String sScraper = "scraper";
  return sScraper;
}