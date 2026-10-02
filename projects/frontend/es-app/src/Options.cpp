//
// Created by bkg2k on 21/01/24.
//

#include <cstring>
#include <cstdlib>
#include "Options.h"
#include <utils/Log.h>
#include <EmulationStation.h>

Options::Options(int argc, char** argv)
  : mWidth(0)
  , mHeight(0)
  , mTATERotation(RotationType::None)
  , mDebug(false)
  , mTrace(false)
  , mFullscreen(true)
  , mEmulateRGBDual(false)
  , mEmulateRGBJamma(false)
  , mFPS(false)
  , mEmulateTATE(false)
{
  Parse(argc, argv);
}

void Options::Parse(int argc, char** argv)
{
  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "--resolution") == 0)
    {
      if (i >= argc - 2) { LOG(LogFatal) << "[Params] Invalid resolution supplied."; }
      mWidth = String(argv[i + 1]).AsInt();
      mHeight = String(argv[i + 2]).AsInt();
      i += 2; // skip the argument value
    }
    else if (strcmp(argv[i], "--draw-framerate") == 0) mFPS = true;
    else if (strcmp(argv[i], "--debug") == 0) mDebug = true;
    else if (strcmp(argv[i], "--trace") == 0) mTrace = true;
    else if (strcmp(argv[i], "--windowed") == 0) mFullscreen = false;
    else if (strcmp(argv[i], "--tateright") == 0) { mEmulateTATE = true; mTATERotation = RotationType::Right; }
    else if (strcmp(argv[i], "--tateleft") == 0) { mEmulateTATE = true; mTATERotation = RotationType::Left; }
    else if (strcmp(argv[i], "--rrgbd") == 0) mEmulateRGBDual = true;
    else if (strcmp(argv[i], "--jamma") == 0) mEmulateRGBJamma = true;
    else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
    {
      printf("EmulationStation, a graphical front-end for ROM browsing.\n"
             "Written by Alec \"Aloshi\" Lofquist.\n"
             "Version " PROGRAM_VERSION_STRING ", built " PROGRAM_BUILT_STRING "\n\n"
             "Command line arguments:\n"
             "--resolution [width] [height] try and force a particular resolution\n"
             "--draw-framerate              display the framerate\n"
             "--debug                       more logging\n"
             "--trace                       event more logging!\n"
             "--windowed                    not fullscreen, should be used with --resolution\n"
             "--rgbdual                     simulate an rgbdual\n"
             "--rgbjamma                    simulate an rgbjamma\n"
             "--help, -h                    summon a sentient, angry tuba\n\n"
             "More information available in README.md.\n");
      exit(0);
    }
  }
}
