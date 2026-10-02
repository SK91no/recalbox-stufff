//
// Created by digi on 10/13/23.
//

#pragma once

#include "recalbox/RecalboxSystem.h"

class MenuFilter
{
  public:
    enum Menu
    {
      Main,
      Exit,
      Search,
      GamelistOptions
    };
    enum Feature
    {
      Kodi,
      Netplay,
      Favorites,
      P2K
    };

    static bool ShouldDisplayMenu(const enum Menu menu)
    { (void)menu; return RecalboxConf::Instance().GetMenuType() != RecalboxConf::Menu::None; }

    static bool ShouldEnableFeature(const enum Feature feature)
    {
      switch (feature) {
        case Kodi :
          return RecalboxSystem::kodiExists() && RecalboxConf::Instance().GetKodiEnabled() && RecalboxConf::Instance().GetMenuType() != RecalboxConf::Menu::None;
        case Netplay:
          return RecalboxConf::Instance().GetNetplayEnabled() && RecalboxConf::Instance().GetMenuType() != RecalboxConf::Menu::None;
        case Favorites:
        case P2K:
          return RecalboxConf::Instance().GetMenuType() != RecalboxConf::Menu::None;
        default: return true;
      }
    }

    enum MenuEntry
    {
      HDMode,
      Widescreen,
      PiEeprom,
    };

    static bool ShouldDisplayMenuEntry(const enum MenuEntry menuEntry)
    {
      switch (menuEntry)
      {
        case HDMode:
          return Board::Instance().GetBoardType() == BoardType::PCx64 ||
                 Board::Instance().GetBoardType() == BoardType::Pi5 ||
                 Board::Instance().GetBoardType() == BoardType::Pi500;
        case Widescreen:
          return Board::Instance().GetBoardType() == BoardType::PCx64 ||
                 Board::Instance().GetBoardType() == BoardType::Pi5 ||
                 Board::Instance().GetBoardType() == BoardType::Pi500 ||
                 Board::Instance().GetBoardType() == BoardType::Pi4 ||
                 Board::Instance().GetBoardType() == BoardType::Pi400;
        case PiEeprom:
          return Board::Instance().GetBoardType() == BoardType::Pi4 ||
                 Board::Instance().GetBoardType() == BoardType::Pi400 ||
                 Board::Instance().GetBoardType() == BoardType::Pi5 ||
                 Board::Instance().GetBoardType() == BoardType::Pi500;
        default: return false;
      }
    }
};
