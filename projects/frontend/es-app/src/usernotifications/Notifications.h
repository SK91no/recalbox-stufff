//
// Created by bkg2k on 30/05/24.
//
#pragma once

#include <utils/cplusplus/Bitflags.h>

enum class Notification
{
  None                 = 0x00000, //!< Non triggered event
  Start                = 0x00001, //!< ES start or restart. Parameter: start count
  Stop                 = 0x00002, //!< ES stops. Parameter: start count
  Shutdown             = 0x00004, //!< The whole system is required to shutdown
  Reboot               = 0x00008, //!< The whole system is required to reboot
  Relaunch             = 0x00010, //!< ES is going to relaunch
  Quit                 = 0x00020, //!< ES is going to quit (ex: GPI case power button)
  SystemBrowsing       = 0x00040, //!< The user is browsing system list and selected a new system. Parameter: system short name
  GamelistBrowsing     = 0x00080, //!< The user is browsing the game list and selected a new game. Parameter: game path
  RunGame              = 0x00100, //!< A game is going to launch. Parameter: game path
  RunDemo              = 0x00200, //!< A game is going to launch in demo mode. Parameter: game path
  EndGame              = 0x00400, //!< Game session end. Parameter: game path
  EndDemo              = 0x00800, //!< Game demo session end. Parameter: game path
  Sleep                = 0x01000, //!< EmulationStation is entering sleep state.
  WakeUp               = 0x02000, //!< EmulationStation is waking up
  ScrapStart           = 0x04000, //!< A multiple game scraping session starts
  ScrapStop            = 0x08000, //!< Scraping session end. Parameter: scraped game count
  ScrapGame            = 0x10000, //!< A game has been scraped. Parameter: game path
  ConfigurationChanged = 0x20000, //!< The user changed something in the configuration.
  RunKodi              = 0x40000, //!< Run kodi!
  StartGameClip        = 0x60000, //!< Start a game clip
  StopGameClip         = 0x80000, //!< Stop a game clip
};

DEFINE_BITFLAG_ENUM(Notification, int)

