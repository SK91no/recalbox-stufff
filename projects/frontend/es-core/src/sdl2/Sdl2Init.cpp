//
// Created by bkg2k on 29/09/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "Sdl2Init.h"
#include <utils/Log.h>
#include <SDL2/SDL.h>

#include "utils/os/system/Thread.h"

void Sdl2Init::Initialize()
{
  for (int retry = 3; retry-- >= 0; )
  {
    if (TryInitialize() == 0) return;
    Thread::Sleep(200);
  }
  { LOG(LogFatal) << "[SDL2] Fatal error initializing SDL 2"; }
}

int Sdl2Init::TryInitialize()
{
  return SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK
    #ifndef OPTION_RECALBOX_NO_AUDIO
    | SDL_INIT_AUDIO
    #endif
    );
}

void Sdl2Init::Finalize()
{
  SDL_Quit();
}
