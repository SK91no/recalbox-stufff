#pragma once

#ifdef USE_OPENGL_CORE

#include "RendererGLCommon.h"
#include "rendering/opengl/Shader.h"

/*!
 * @brief Desktop OpenGL 3.3 Core renderer implementation.
 *        Uses GLSL 3.30 shaders.  Differs from the GLES2/GLES3 backends in
 *        two important ways imposed by the Core profile:
 *          • A Vertex Array Object MUST be bound — the default VAO 0 is
 *            invalid in Core 3.1+.  We create one VAO at init and keep it
 *            bound for the lifetime of the renderer.
 *          • Client-side vertex arrays are forbidden — every glDrawArrays
 *            must source its data from a VBO.  We stream all vertex data
 *            through a pair of dynamic VBOs (positions/vertices + colours)
 *            using buffer orphaning (glBufferData(NULL) + glBufferSubData).
 *
 *        Targeting GL 3.3 (released March 2010) covers virtually every
 *        desktop GPU produced since ~2008 and supports >99 % of the
 *        RetroArch GLSL shader catalog out of the box.  GLSL 3.30 brings
 *        `layout(location)` for vertex attributes and fragment outputs,
 *        which lets us drop the `glBindAttribLocation` /
 *        `glBindFragDataLocation` boilerplate that was needed in 1.30.
 *
 *        The entire class is guarded by #ifdef USE_OPENGL_CORE to prevent
 *        link errors on non-Core builds.
 */
class RendererGLCore : public RendererGLCommon
{
  public:
    /*!
     * @brief Build the GL Core 3.3 renderer and initialize the SDL surface + GL context.
     * @param width    Desired physical viewport width
     * @param height   Desired physical viewport height
     * @param windowed True for windowed mode, false for fullscreen
     * @param rotation Initial viewport rotation (default: none)
     */
    RendererGLCore(int width, int height, bool windowed, RotationType rotation = RotationType::None);

  protected:
    /*!
     * @brief Set GL context profile / version attributes prior to SDL_GL_CreateContext.
     *        Selects the OpenGL 3.3 Core profile (GLSL 3.30 shaders).
     */
    void DoConfigureGLProfile() override;

    /*!
     * @brief Post-context initialization. Creates the streaming VAO + VBOs and
     *        compiles/links the three GLSL 3.30 shader programs (color, RGBA
     *        texture, alpha texture).
     * @return true on success, false if any GL resource creation fails
     */
    bool DoInitContext() override;

    /*!
     * @brief Pre-destruction GL teardown. Deletes the three shader programs,
     *        the streaming VBOs, and the VAO.
     */
    void DoFinalizeContext() noexcept override;

    /*!
     * @brief Reset the projection matrix to an orthographic mapping (0,w,h,0).
     *        The result is stored in mProjMatrix and pushed to shaders later.
     * @param w Projection width
     * @param h Projection height
     */
    void DoBeginProjection(int w, int h) override;

    /*!
     * @brief Apply a rotation/translation to mProjMatrix so the virtual viewport
     *        ends up oriented like the physical surface.
     * @param rotation Rotation type
     * @param viewW    Physical viewport width  (mViewportSize.x())
     * @param viewH    Physical viewport height (mViewportSize.y())
     */
    void DoApplyRotationToProjection(RotationType rotation, int viewW, int viewH) override;

    /*!
     * @brief Apply the global scale to mProjMatrix and reset the model-view
     *        matrix to identity.
     * @param sx Horizontal scale factor (mScale.x())
     * @param sy Vertical   scale factor (mScale.y())
     */
    void DoEndProjection(float sx, float sy) override;

    /*!
     * @brief Replace the model-view matrix used by the shaders with the
     *        supplied transform. Combined with mProjMatrix at draw time.
     * @param transform Model-view matrix
     */
    void DoSetMatrix(const Transform4x4f& transform) override;

    /*!
     * @brief Draw a uniform-colored rectangle (two triangles) with the
     *        color-only shader and a custom blend equation.
     * @param x       Top-left X
     * @param y       Top-left Y
     * @param w       Width
     * @param h       Height
     * @param color   ARGB color
     * @param sfactor Source blend factor
     * @param dfactor Destination blend factor
     */
    void DoDrawColorRect(int x, int y, int w, int h, Colors::ColorARGB color, BlendMode blend) override;

    /*!
     * @brief Draw a rectangle with one color per corner (gradient) using the
     *        color-only shader.
     * @param x  Top-left X
     * @param y  Top-left Y
     * @param w  Width
     * @param h  Height
     * @param tl Top-left color
     * @param tr Top-right color
     * @param br Bottom-right color
     * @param bl Bottom-left color
     */
    void DoDrawColorRect4(int x, int y, int w, int h,
                          Colors::ColorARGB tl, Colors::ColorARGB tr,
                          Colors::ColorARGB br, Colors::ColorARGB bl) override;

    /*!
     * @brief Draw a GL_TRIANGLE_FAN with a uniform color via the color-only
     *        shader. Used to fan-fill rounded-corner geometry.
     * @param verts Interleaved (x,y) float pairs
     * @param count Vertex count
     * @param color ARGB color applied to every vertex
     */
    void DoDrawColorFan(const float* verts, int count, unsigned int color) override;

    /*!
     * @brief Draw untextured triangles with one colour per vertex.
     *        Streams both attributes through the VBOs, as the core profile
     *        rejects client-side arrays.
     * @param verts  Vertex positions, three per triangle
     * @param colors One GPU-byte-ordered RGBA colour per vertex
     * @param count  Vertex count, a multiple of three
     */
    void DoDrawColorTriangles(const Vector2f* verts, const ColorByte* colors, int count) override;

    /*!
     * @brief Rasterize a GL_TRIANGLE_FAN into the stencil buffer and arm the
     *        stencil test for subsequent draw calls (used for rounded clipping).
     * @param verts Interleaved (x,y) float pairs
     * @param count Vertex count
     */
    void DoWriteStencilFan(const float* verts, int count) override;

    /*!
     * @brief Draw a polyline (GL_LINE_STRIP) with one color per vertex via the
     *        color-only shader.
     * @param coords Vertex coordinates
     * @param colors Per-vertex ARGB colors
     * @param count  Vertex count
     */
    void DoDrawLines(const Vector2f* coords, const Colors::ColorARGB* colors, int count) override;

    /*!
     * @brief Draw a polyline (GL_LINE_STRIP) with a uniform color via the
     *        color-only shader (no per-vertex color attribute).
     * @param coords Vertex coordinates
     * @param color  Uniform ARGB color
     * @param count  Vertex count
     */
    void DoDrawLinesUniform(const Vector2f* coords, Colors::ColorARGB color, int count) override;

    /*!
     * @brief Submit textured triangles with a per-vertex color array.
     *        Picks the alpha or RGBA texture shader depending on @p type.
     * @param verts  Interleaved vertices (position + texcoord)
     * @param colors Per-vertex ARGB color array
     * @param count  Vertex count
     * @param type   RGBA → texture shader, Alpha → alpha shader
     */
    void DoDrawTTWithColors(const Vertex* verts, const ColorByte* colors, int count, TextureType type) override;

    /*!
     * @brief Submit textured triangles with a single uniform color.
     *        Picks the alpha or RGBA texture shader depending on @p type.
     * @param verts Interleaved vertices (position + texcoord)
     * @param color Uniform ARGB color
     * @param count Vertex count
     * @param type  RGBA → texture shader, Alpha → alpha shader
     */
    void DoDrawTTUniform(const Vertex* verts, Colors::ColorARGB color, int count, TextureType type) override;

    /*!
     * @brief Submit a textured quad with a single uniform RGBA color where
     *        each component is in [0,1] (used by the gradient/reflection paths).
     * @param verts Interleaved vertices (position + texcoord)
     * @param count Vertex count (typically 6 for a quad)
     * @param r     Red   in [0,1]
     * @param g     Green in [0,1]
     * @param b     Blue  in [0,1]
     * @param a     Alpha in [0,1]
     * @param type  RGBA or Alpha texture sampling mode
     */
    void DoSubmitTexQuadUniform(const Vertex* verts, int count,
                                float r, float g, float b, float a,
                                TextureType type) override;

    /*!
     * @brief Submit a textured quad with a per-vertex color array (packed ARGB).
     * @param verts  Interleaved vertices (position + texcoord)
     * @param colors Per-vertex packed ARGB colors
     * @param count  Vertex count
     * @param type   RGBA or Alpha texture sampling mode
     */
    void DoSubmitTexQuadColors(const Vertex* verts, const Colors::ColorRGBA* colors,
                               int count, TextureType type) override;

    /*!
     * @brief Submit two textured quads back-to-back, used to render the
     *        alpha-reflection effect (main image + flipped reflection) without
     *        rebinding the texture or the shader program.
     * @param v1    First quad vertices
     * @param c1    First quad per-vertex ARGB colors
     * @param v2    Second (reflected) quad vertices
     * @param c2    Second quad per-vertex ARGB colors
     * @param count Vertex count per quad
     * @param type  RGBA or Alpha texture sampling mode
     */
    void DoSubmitTexQuadColorsTwoDraws(const Vertex* v1, const Colors::ColorRGBA* c1,
                                       const Vertex* v2, const Colors::ColorRGBA* c2,
                                       int count, TextureType type) override;

  private:
    //! Color-only shader program
    Shader mShaderColor;
    //! RGBA texture shader program
    Shader mShaderTexture;
    //! Alpha-channel texture shader program
    Shader mShaderAlpha;

    //! Single VAO bound for the lifetime of the renderer (Core profile requirement)
    GLuint mVAO;
    //! Streaming VBO for positions / interleaved vertex data
    GLuint mVBOVerts;
    //! Streaming VBO for per-vertex colours (when supplied as a separate stream)
    GLuint mVBOColors;

    // mProjMatrix / mMVMatrix now live in RendererGLCommon, alongside the
    // program cache that decides when they actually reach the GPU.

    /*!
     * @brief Compile and link the three GLSL 3.30 shader programs.
     *        Attribute locations are fixed via `layout(location = N)` in the
     *        shader source, so no glBindAttribLocation calls are needed:
     *          slot 0 → a_pos  (vec2 position)
     *          slot 1 → a_col  (vec4 colour)
     *          slot 2 → a_tex  (vec2 texture coord)
     * @return true on success
     */
    bool InitShaders();

    /*!
     * @brief Stream-upload data into a VBO using buffer orphaning so the
     *        previous frame's buffer can still be in flight on the GPU.
     * @param vbo  Target VBO id (will be bound to GL_ARRAY_BUFFER)
     * @param data Pointer to bytes to upload
     * @param size Byte count
     */
    static void StreamUpload(GLuint vbo, const void* data, GLsizeiptr size);
};

#endif // USE_OPENGL_CORE
