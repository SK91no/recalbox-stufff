//
// Created by bkg2k on 01/10/2020.
//
#pragma once

// DEAD CODE, EXCLUDED FROM THE BUILD.
//
// Nothing calls Primitives:: anymore, and it could not work if it did: the
// fixed-pipeline calls it makes (glEnableClientState / glVertexPointer /
// glColorPointer) are GLES 1.x, absent from every context the renderer creates
// (GLES2, GLES 3.1, GL Core). It only ever compiled because GLPlatform.h used
// to pull the GLES 1.x header in.
//
// The guard below is never defined: this is a deliberate exclusion, pending
// outright removal. See VULKAN.md.
#ifdef USE_LEGACY_GL1_PRIMITIVES

#include "Colors.h"
#include "Rectangle.h"
#include "rendering/GLPlatform.h"

class Primitives
{
  public:
    #ifdef USE_OPENGL_ES
    typedef GLshort GLNative;
    static constexpr GLenum GLNativeType = GL_SHORT;
    #else
    typedef GLint GLNative;
    static constexpr GLenum GLNativeType = GL_INT;
    #endif

    /*!
     * @brief Draw rectangle
     * @param x Left coordinate
     * @param y Top coordinate
     * @param w Width
     * @param h Height
     * @param color Color
     */
    static void DrawRectangle(const Rectangle& area, Colors::ColorARGB color);

    /*!
     * @brief Draw rectangle
     * @param x Left coordinate
     * @param y Top coordinate
     * @param w Width
     * @param h Height
     * @param color Color
     */
    static void DrawRectangle(int x, int y, int w, int h, Colors::ColorARGB color);

    /*!
     * @brief Draw rectangle
     * @param x Left coordinate
     * @param y Top coordinate
     * @param w Width
     * @param h Height
     * @param color Color
     */
    static void DrawRectangle(float x, float y, float w, float h, Colors::ColorARGB color);

    /*!
     * @brief Draw rectangle
     * @param x Left coordinate
     * @param y Top coordinate
     * @param w Width
     * @param h Height
     * @param color Color
     * @param blend_sfactor Source blending
     * @param blend_dfactor Destination blending
     */
    static void DrawRectangle(const Rectangle& area, Colors::ColorARGB color, GLenum blendSourceFactor, GLenum blendDestinationFactor);

    /*!
     * @brief Draw rectangle
     * @param x Left coordinate
     * @param y Top coordinate
     * @param w Width
     * @param h Height
     * @param color Color
     * @param blend_sfactor Source blending
     * @param blend_dfactor Destination blending
     */
    static void DrawRectangle(int x, int y, int w, int h, Colors::ColorARGB color, GLenum blendSourceFactor, GLenum blendDestinationFactor);

    /*!
     * @brief Draw rectangle
     * @param x Left coordinate
     * @param y Top coordinate
     * @param w Width
     * @param h Height
     * @param color Color
     * @param blend_sfactor Source blending
     * @param blend_dfactor Destination blending
     */
    static void DrawRectangle(float x, float y, float w, float h, Colors::ColorARGB color, GLenum blendSourceFactor, GLenum blendDestinationFactor);

  private:
    /*!
     * @brief Draw non textured triangles using the given vertex list
     * @param vertex Vertexes
     * @param colors Colors
     * @param vertexCount Vertex count
     * @param blendSourceFactor Source blending factor
     * @param blendDestinationFactor Destination blending factor
     */
    static void DrawNonTexturedTriangles(GLNative* vertex, Colors::ColorARGB color, int vertexCount, GLenum blendSourceFactor, GLenum blendDestinationFactor);

    /*!
     * @brief Draw non textured triangles using the given vertex list
     * @param vertex Vertexes
     * @param colors Colors
     * @param vertexCount Vertex count
     */
    static void DrawNonTexturedTriangles(GLNative* vertex, Colors::ColorARGB color, int vertexCount);
};

#endif // USE_LEGACY_GL1_PRIMITIVES
