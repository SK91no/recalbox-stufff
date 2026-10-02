//
// Created by bkg2k on 26/07/26.
//
#pragma once

// GL-only translation unit: absent from a Vulkan build entirely.
#ifdef USE_OPENGL

#include "Renderer.h"
// This is the GL side of the fence: everything deriving from this class talks
// OpenGL, so the GL header belongs here rather than in Renderer.h.
#include "rendering/opengl/GLHeaders.h"
#include <utils/storage/Array.h>

/*!
 * @brief Shared OpenGL implementation of the backend-neutral texture
 *        operations declared by Renderer.
 *
 * All three GL backends (GLES2, GLES3.1, Desktop GL Core) derive from this
 * class so the texture code exists exactly once. A future Vulkan backend
 * derives from Renderer directly and provides its own implementation — that
 * separation is the whole point. See VULKAN.md.
 *
 * Still abstract: the drawing primitives remain pure virtual here.
 */
class RendererGLCommon : public Renderer
{
  public:
    /*!
     * @brief Forward construction parameters to Renderer. Initialization of the
     *        SDL surface and GL context stays the concrete backend's job.
     */
    RendererGLCommon(int width, int height, bool windowed, RotationType rotation)
      : Renderer(width, height, windowed, rotation)
      , mProjMatrix(Transform4x4f::Identity())
      , mMVMatrix(Transform4x4f::Identity())
    {
    }

  protected:
    /*!
     * @brief Set the SDL GL attributes common to every GL backend, then let the
     *        concrete backend pick its profile and version.
     */
    void DoConfigureContext() final;

    //! @return SDL_WINDOW_OPENGL
    [[nodiscard]] unsigned int DoWindowFlags() const final;

    /*!
     * @brief Create the SDL GL context on the window and enable vsync.
     * @param window Window to create the context for
     * @return true on success
     */
    bool DoCreateContext(SDL_Window* window) final;

    //! @brief Delete the SDL GL context
    void DoDestroyContext() noexcept final;

    //! @return true when the GL context exists
    [[nodiscard]] bool DoHasContext() const final { return mSdlGLContext != nullptr; }

    /*!
     * @brief Swap the window buffers.
     * @param window Window to present
     */
    void DoPresent(SDL_Window* window) final;

    /*!
     * @brief Select the GL profile and version. Called by DoConfigureContext()
     *        once the attributes shared by every GL backend are set.
     */
    virtual void DoConfigureGLProfile() = 0;

    /*!
     * @brief Forget every cached GL state.
     *
     * MUST be called by each backend at the top of DoInitContext(): the context is
     * brand new there (ReInitialize / Rotate destroy and recreate it), so every
     * cached binding is stale. GL also recycles texture handles, so a stale
     * mBoundTexture could make a legitimate bind be skipped.
     */
    void ResetStateCache();

    /*!
     * @brief Create a GL texture object and set its min/mag filters.
     * @param nearest True for GL_NEAREST filtering, false for GL_LINEAR
     * @return Texture handle, InvalidTextureHandle on failure
     */
    TextureHandle DoCreateTexture(bool nearest) override;

    /*!
     * @brief Delete a GL texture object.
     * @param id Texture handle
     */
    void DoDestroyTexture(TextureHandle id) override;

    /*!
     * @brief Bind a texture for subsequent draw calls. A null handle is a no-op,
     *        leaving whatever texture is already bound in place.
     * @param id Texture handle
     */
    void DoBindTexture(TextureHandle id) override;

    /*!
     * @brief Set the wrap mode of the currently bound texture.
     * @param tiled True to repeat, false to clamp to edge
     */
    void DoSetTextureTiling(bool tiled) override;

    /*!
     * @brief Allocate and fill the whole texture image.
     * @param id Texture handle
     * @param width Image width
     * @param height Image height
     * @param data Pixel data, or nullptr to allocate without uploading
     * @param type RGBA (4 bytes per pixel) or Alpha (1 byte per pixel)
     * @return NoError, NoResource or OutOfGPUMemory
     */
    RenderingError DoUploadTexture(TextureHandle id, int width, int height,
                                   const void* data, TextureType type) override;

    /*!
     * @brief Upload a sub-rectangle into an already allocated texture image.
     * @param id Texture handle
     * @param x Left coordinate in the destination image
     * @param y Top coordinate in the destination image
     * @param width Sub-image width
     * @param height Sub-image height
     * @param data Pixel data
     * @param type RGBA (4 bytes per pixel) or Alpha (1 byte per pixel)
     * @return NoError, NoResource or OutOfGPUMemory
     */
    RenderingError DoUploadTexturePart(TextureHandle id, int x, int y, int width, int height,
                                       const void* data, TextureType type) override;

    /*!
     * @brief Enable GL blending with the factors matching the given mode.
     * @param blend Blending equation
     */
    void DoEnableBlend(BlendMode blend) override;

    //! @brief Disable GL blending
    void DoDisableBlend() override;

    /*!
     * @brief Set the scissor rectangle and enable the scissor test.
     * @param x Left coordinate, GL convention (origin bottom-left)
     * @param y Bottom coordinate, GL convention
     * @param w Width
     * @param h Height
     */
    void DoSetScissor(int x, int y, int w, int h) override;

    //! @brief Disable the scissor test
    void DoDisableScissor() override;

    //! @brief Disable the stencil test and wipe the stencil buffer
    void DoClearStencil() override;

    /*!
     * @brief Set the GL viewport.
     * @param x Left coordinate
     * @param y Bottom coordinate
     * @param w Width
     * @param h Height
     */
    void DoSetViewport(int x, int y, int w, int h) override;

    /*!
     * @brief Set the clear color.
     * @param r Red   in [0,1]
     * @param g Green in [0,1]
     * @param b Blue  in [0,1]
     * @param a Alpha in [0,1]
     */
    void DoSetClearColor(float r, float g, float b, float a) override;

    //! @brief Clear color and depth buffers
    void DoClearBuffers() override;

    /*!
     * @brief Return one of the GL driver identification strings.
     * @param info Which string to return
     * @return Driver string, empty if the driver reports none
     */
    String DoGetDriverInfo(DriverInfo info) override;

    //! @brief Hook glDebugMessageCallback. Desktop GL DEBUG builds only; no-op elsewhere.
    void DoActivateDebug() override;

    /*!
     * @brief Create the offscreen FBO and its colour texture.
     * @param w Target width
     * @param h Target height
     * @param texture Receives the colour texture handle
     * @return true if the FBO is complete
     */
    bool DoCreateOffscreen(int w, int h, TextureHandle& texture) override;

    /*!
     * @brief Delete the offscreen FBO and its colour texture.
     * @param texture Colour texture handle, reset to InvalidTextureHandle
     */
    void DoDestroyOffscreen(TextureHandle& texture) noexcept override;

    /*!
     * @brief Bind the offscreen FBO and size the viewport to it.
     * @param w Offscreen width
     * @param h Offscreen height
     */
    void DoBeginOffScreen(int w, int h) override;

    /*!
     * @brief Bind the default framebuffer back and restore the physical viewport.
     * @param w Physical viewport width
     * @param h Physical viewport height
     */
    void DoEndOffScreen(int w, int h) override;

  private:
    //! SDL GL context
    SDL_GLContext mSdlGLContext = nullptr;

    //! Offscreen FBO identifier (0 = not created)
    GLuint mOffscreenFBO = 0;
    //! Depth+stencil renderbuffer attached to the offscreen FBO (0 = none).
    //! Without it the offscreen target has no stencil at all, and every rounded
    //! clip drawn there would silently do nothing.
    GLuint mOffscreenDepthStencil = 0;

    // -----------------------------------------------------------------------
    // Redundant state elimination
    //
    // Measured on a real session: 93% of wrap-mode requests and every single
    // blend enable/disable pair were pointless. That was ~300 wasted GL calls
    // per frame out of ~820. See VULKAN.md.
    //
    // INVARIANT: every glBindTexture / glEnable(GL_BLEND) / glBlendFunc /
    // glTexParameteri(WRAP_*) in the GL backends must go through the helpers
    // below, or the cache silently desynchronises from the driver.
    // -----------------------------------------------------------------------

    //! Wrap mode currently set on each texture, indexed by handle.
    enum class WrapState : unsigned char { Unknown = 0, Clamp = 1, Repeat = 2 };

    //! Wrap state per texture handle. GL hands out small increasing integers,
    //! so a flat array indexed by handle is O(1) and far cheaper than the two
    //! glTexParameteri calls it avoids. Wrap is a per-texture property, not a
    //! global one — caching it globally would apply one texture's mode to the
    //! next. Hence the per-handle table.
    Array<WrapState> mTextureWrap;

    //! Texture currently bound to GL_TEXTURE_2D
    TextureHandle mBoundTexture = InvalidTextureHandle;

    //! True when GL_SCISSOR_TEST is currently enabled
    bool mScissorEnabled = false;
    //! True once mScissorRect reflects the rectangle actually installed
    bool mScissorKnown = false;
    //! Scissor rectangle currently installed: x, y, width, height
    int mScissorRect[4] { 0, 0, 0, 0 };

    //! True when GL_BLEND is currently enabled
    bool mBlendEnabled = false;
    //! True once mBlendMode reflects the blend function actually installed
    bool mBlendModeKnown = false;
    //! Blend equation currently installed
    BlendMode mBlendMode = BlendMode::AlphaBlend;

    /*!
     * @brief Bind a texture, updating the cache. Use instead of raw glBindTexture.
     * @param id Texture to bind
     */
    void BindTextureCached(TextureHandle id);

    /*!
     * @brief Record that the driver's texture binding changed behind our back,
     *        so the next bind is re-issued.
     * @param id Texture the driver is now bound to
     */
    void NoteTextureBound(TextureHandle id) { mBoundTexture = id; }

    /*!
     * @brief Read the wrap state recorded for a texture.
     * @param id Texture handle
     * @return Recorded state, Unknown if never set or out of range
     */
    WrapState WrapOf(TextureHandle id) const
    { return (int)id < mTextureWrap.Count() ? mTextureWrap[(int)id] : WrapState::Unknown; }

  protected:
    // -----------------------------------------------------------------------
    // Programs and matrices
    //
    // All three GL backends run the same three programs (colour, RGBA texture,
    // alpha texture) and feed them the same two matrices, so the bookkeeping
    // lives here once.
    //
    // Uniform locations are fixed at link time, yet the old code called
    // glGetUniformLocation — a lookup by name inside the driver — on every
    // single SetMatrix, and pushed both matrices into all three programs even
    // though a frame typically touches one. SetMatrix is called from the
    // Render() of every component, so that was ten GL calls per component.
    //
    // Now a matrix change only bumps a serial. The upload happens when a
    // program is actually selected for a draw, and only if that program has not
    // already seen the current serial.
    // -----------------------------------------------------------------------

    //! Program slots, in the order RegisterProgram() expects
    enum class ProgramSlot { Color = 0, Texture = 1, Alpha = 2, Count = 3 };

    //! Projection matrix. Built by the backend's DoBeginProjection.
    Transform4x4f mProjMatrix;
    //! Model-view matrix, as last set by DoSetMatrix.
    Transform4x4f mMVMatrix;

    /*!
     * @brief Record a linked program and resolve its uniform locations once.
     * @param slot Which of the three programs this is
     * @param program Linked program name
     */
    void RegisterProgram(ProgramSlot slot, unsigned int program);

    /*!
     * @brief Make a program current and bring its matrices up to date.
     *        Use instead of raw glUseProgram, or the cache desynchronises.
     * @param program Program to activate
     */
    void UseProgram(unsigned int program);

    //! @brief Note that a matrix changed, so programs re-upload on next use
    void InvalidateMatrices() { ++mMatrixSerial; }

    //! @brief Forget every program binding and location. Call on a fresh context.
    void ResetProgramCache();

    /*!
     * @brief Drain the GL error queue, log everything in it, and translate it
     *        into a RenderingError.
     *
     * The old code tested for GL_OUT_OF_MEMORY only and returned NoError for
     * anything else, which is how an illegal texture format could fail on every
     * upload without a single line in the log. Draining also matters: one
     * glGetError pops one error, and a leftover would be blamed on the next
     * operation.
     * @param what Short description of the operation, for the log
     * @return NoError when the queue was empty
     */
    RenderingError DrainErrors(const char* what);

    /*!
     * @brief Record the wrap state of a texture, growing the table as needed.
     * @param id Texture handle
     * @param state State to record
     */
    void SetWrapOf(TextureHandle id, WrapState state);

  private:
    //! Link-time state of one program
    struct ProgramState
    {
      unsigned int Program      = 0;   //!< Program name, 0 when unused
      int          ProjLocation = -1;  //!< Location of u_proj, -1 when absent
      int          MVLocation   = -1;  //!< Location of u_mv, -1 when absent
      unsigned int MatrixSerial = 0;   //!< Serial of the matrices last uploaded
    };

    //! One entry per ProgramSlot
    ProgramState mProgramStates[(int)ProgramSlot::Count];
    //! Program currently bound, 0 for none
    unsigned int mActiveProgram = 0;
    //! Bumped by InvalidateMatrices; starts at 1 so slot serial 0 means "never"
    unsigned int mMatrixSerial = 1;
};

#endif // USE_OPENGL
