#pragma once

// Legacy alias for rendering/opengl/GLHeaders.h, which is now the single place
// where a GL header gets included.
//
// This file used to carry its own #ifdef ladder covering only USE_OPENGL_ES and
// USE_OPENGL_CORE — it knew nothing about USE_OPENGL_ES31, so every translation
// unit including it failed to compile in a GLES 3.1 build. Delegating fixes that
// and keeps the three variants defined in exactly one place.
//
// Note the ES branch used to pull GLES 1.x; it now resolves to GLES 2/3 like
// everything else, so any remaining fixed-pipeline caller will no longer build.

#ifdef WIN32
  #define sleep Sleep
#endif

#include "rendering/opengl/GLHeaders.h"
