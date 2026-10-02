//
// Created by bkg2k on 15/12/23.
//
#pragma once

//! View type
enum class ViewType
{
  None,                   //!< Uninitialized
  SplashScreen,           //!< Splash screen (startup or stop)
  SystemList,             //!< System list
  GameList,               //!< Game list
  GameClip,               //!< Game clip
  CrtCalibration,         //!< CRT Calibration screen
  CardReaderConsoleView,  //!< Console view for Card Reader
  Bouncing,               //!< Bouncing screensaver
};