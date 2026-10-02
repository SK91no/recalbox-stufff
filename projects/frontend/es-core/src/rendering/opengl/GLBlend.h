//
// Created by bkg2k on 26/07/26.
//
// BlendMode -> OpenGL blend factor translation, shared by every GL backend.
//
#pragma once

#ifdef USE_OPENGL

#include "GLHeaders.h"
#include "rendering/RenderingTypes.h"

/*!
 * @brief Apply the GL blend factors matching the given backend-neutral mode.
 *        Does not enable/disable GL_BLEND: the caller owns that.
 * @param mode Blending equation to install
 */
static inline void ApplyGLBlendMode(BlendMode mode)
{
  switch(mode)
  {
    case BlendMode::InverseMultiply: glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR); break;
    case BlendMode::Additive       : glBlendFunc(GL_ONE, GL_ONE); break;
    case BlendMode::AlphaBlend     :
    default                        : glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
  }
}

#endif // USE_OPENGL
