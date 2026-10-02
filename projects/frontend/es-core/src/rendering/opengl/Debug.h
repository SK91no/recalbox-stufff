//
// Created by bkg2k on 19/04/22.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

// CheckGLErrors only exists in a debug build that actually talks OpenGL;
// anywhere else CHECKGL degrades to the bare expression.
#if defined(DEBUG) && defined(USE_OPENGL)

  #define CHECKGL(f) (f, CheckGLErrors(#f))
  bool CheckGLErrors(const char* function);

#else

  #define CHECKGL(f) f

#endif
