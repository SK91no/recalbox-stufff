//
// Created by bkg2k on 19/04/22.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

// GL error decoding: needs both a debug build and an actual GL backend.
#if defined(DEBUG) && defined(USE_OPENGL)

#include <rendering/opengl/Debug.h>
#include <rendering/GLPlatform.h>
#include <utils/Log.h>

bool CheckGLErrors(const char* function)
{
  bool result = false;
  for(bool exit = false; !exit; )
    switch(glGetError())
    {
      case GL_NO_ERROR: exit = true; break;
      case GL_INVALID_ENUM: { LOG(LogError) << "[GLError] " << "An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag." << '\n' << function; } result = true; break;
      case GL_INVALID_VALUE: { LOG(LogError) << "[GLError] " << "A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag." << '\n' << function; } result = true; break;
      case GL_INVALID_OPERATION: { LOG(LogError) << "[GLError] " << "The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag." << '\n' << function; } result = true; break;
      #ifdef GL_INVALID_FRAMEBUFFER_OPERATION
      case GL_INVALID_FRAMEBUFFER_OPERATION: { LOG(LogError) << "[GLError] " << "The framebuffer object is not complete. The offending command is ignored and has no other side effect than to set the error flag." << '\n' << function; } result = true; break;
      #endif
      case GL_OUT_OF_MEMORY: { LOG(LogError) << "[GLError] " << "There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded." << '\n' << function; } result = true; break;
      // Desktop GL only: GLES has no internal matrix stacks, hence no such errors.
      #ifdef USE_OPENGL_CORE
      case GL_STACK_UNDERFLOW: { LOG(LogError) << "[GLError] " << "An attempt has been made to perform an operation that would cause an internal stack to underflow." << '\n' << function; } result = true; break;
      case GL_STACK_OVERFLOW: { LOG(LogError) << "[GLError] " << "An attempt has been made to perform an operation that would cause an internal stack to overflow." << '\n' << function; } result = true; break;
      #endif
      default: { LOG(LogError) << "[GLError] " << "Unknown error." << '\n' << function; } result = true; break;
    }

  return result;
}

#endif