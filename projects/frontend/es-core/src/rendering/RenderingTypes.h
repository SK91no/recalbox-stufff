//
// Created by bkg2k on 26/07/26.
//
// Backend-neutral rendering types.
//
// These types deliberately carry no OpenGL dependency so that the public
// Renderer API can be consumed without pulling in a GL header, and so that a
// non-GL backend (Vulkan) can implement the same interface. See VULKAN.md.
//
// TextureHandle and ColorByte are intentionally aliased onto the very same
// underlying types as GLuint and GLubyte: existing call sites that still spell
// them the GL way keep compiling untouched.
//
#pragma once

//! Opaque texture identifier handed out by the renderer backend
typedef unsigned int TextureHandle;

//! Value of a TextureHandle that references no texture
static constexpr TextureHandle InvalidTextureHandle = 0;

//! Single color component, as stored in packed color arrays
typedef unsigned char ColorByte;

/*!
 * @brief Blending equations supported by the renderer.
 *
 * The whole application only ever uses these three. Keeping the set closed
 * (rather than exposing raw blend factors) is what allows a backend to bake
 * blending into a fixed set of pipeline objects.
 */
enum class BlendMode
{
  //! src.a * src + (1 - src.a) * dst — regular alpha compositing
  AlphaBlend,
  //! (1 - src) * dst — darkens the destination by the source color
  InverseMultiply,
  //! src + dst — additive lighting
  Additive,
};
