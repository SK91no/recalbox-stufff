//
// Created by bkg2k on 15/12/23.
//
#include "IViewChanged.h"
#include <WindowManager.h>

IViewChanged::IViewChanged(WindowManager& windowManager)
  : mWindowManager(windowManager)
{
  windowManager.Register(this);
}

IViewChanged::~IViewChanged()
{
  mWindowManager.Unregister(this);
}
