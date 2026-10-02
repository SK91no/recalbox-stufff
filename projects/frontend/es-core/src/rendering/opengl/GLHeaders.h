//
// Created by bkg2k on 26/07/26.
//
// The one and only place where an OpenGL header gets included.
//
// Only GL backends and the few components still issuing raw GL calls should
// include this. Anything that merely draws through the Renderer API must
// include rendering/RenderingTypes.h instead — see VULKAN.md.
//
#pragma once

// The build system defines exactly one of these:
//   USE_OPENGL_ES     -> OpenGL ES 2.0
//   USE_OPENGL_ES31   -> OpenGL ES 3.1
//   USE_OPENGL_CORE   -> Desktop OpenGL Core

#ifdef USE_OPENGL_ES31
  #include <GLES3/gl31.h>
#elif defined(USE_OPENGL_ES)
  #include <GLES2/gl2.h>
#elif defined(USE_OPENGL_CORE)
  #define GL_GLEXT_PROTOTYPES
  #include <SDL_opengl.h>
#endif
