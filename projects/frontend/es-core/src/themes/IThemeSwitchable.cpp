//
// Created by bkg2k on 15/12/23.
//
#include "IThemeSwitchable.h"
#include <themes/ThemeManager.h>

IThemeSwitchable::IThemeSwitchable()
{
  ThemeManager::Instance().Register(*this);
}

IThemeSwitchable::~IThemeSwitchable()
{
  ThemeManager::Instance().Unregister(*this);
}

