//
// Created by bkg2k on 26/07/26.
//
// Draw-call instrumentation for step 2 of VULKAN.md (batching).
//
// Answers one question: how many draw calls does a real session issue, and how
// many would survive if consecutive compatible draws were merged into one?
// That ratio is what decides whether batching is worth building.
//
// Cost is a handful of integer increments per draw call, so this can stay
// enabled in a normal build while measuring. Remove once step 2 is decided.
//
#pragma once

#include "rendering/RenderingTypes.h"

class RenderStats
{
  public:
    //! Geometry families, as submitted by the Renderer facade
    enum class DrawKind
    {
      ColorRect,         //!< Uniform colored rectangle
      ColorGradient,     //!< Rectangle with one color per corner
      ColorFan,          //!< Triangle fan (round rectangles, circles)
      ColorTriangles,    //!< Untextured triangles with a colour per vertex
      StencilFan,        //!< Triangle fan written into the stencil buffer
      Lines,             //!< Polylines
      TexturedTriangles, //!< Textured triangles with a color array
      TexturedQuad,      //!< Textured quad
      Count              //!< Sentinel
    };

    //! @brief Single instance accessor. Rendering is single-threaded (CHECKGLCONTEXT).
    static RenderStats& Instance() { static RenderStats instance; return instance; }

    /*!
     * @brief Account for one draw call about to be handed to the backend.
     * @param kind Geometry family
     * @param vertices Vertex count submitted
     * @param alphaTexture True when sampling an alpha-only texture (different shader)
     */
    void CountDraw(DrawKind kind, int vertices, bool alphaTexture = false);

    /*!
     * @brief Account for a texture bind request.
     * @param id Texture being bound
     */
    void CountBindTexture(TextureHandle id);

    /*!
     * @brief Account for a tiling/wrap mode change request.
     * @param tiled Requested tiling
     */
    void CountTiling(bool tiled);

    /*!
     * @brief Account for a blend enable request.
     * @param blend Requested blending equation
     */
    void CountEnableBlend(BlendMode blend);

    //! @brief Account for a blend disable request
    void CountDisableBlend();

    /*!
     * @brief Account for GL calls the state cache actually let through.
     *        Compare against the request counts to see the cache working.
     * @param count Number of GL calls issued
     */
    void CountIssuedStateCalls(int count) { mStateCallsIssued += count; }

    //! @brief Close the current frame and fold its counters into the totals
    void EndFrame();

    /*!
     * @brief Log everything gathered so far, then start over.
     *        No-op when no frame was rendered since the last dump, so the
     *        Finalize() / ~Renderer() pair does not log twice.
     * @param reason Short label telling why stats are being flushed
     */
    void DumpAndReset(const char* reason);

  private:
    RenderStats() = default;

    //! Identifies what a draw call needs from the pipeline. Two consecutive
    //! draws sharing a key could be merged into a single one by a batcher.
    struct BatchKey
    {
      TextureHandle Texture = InvalidTextureHandle;
      int           Shader  = -1;   //!< 0 = color, 1 = RGBA texture, 2 = alpha texture
      BlendMode     Blend   = BlendMode::AlphaBlend;
      bool          Tiled   = false;

      bool operator != (const BatchKey& o) const
      { return Texture != o.Texture || Shader != o.Shader || Blend != o.Blend || Tiled != o.Tiled; }
    };

    //! Batch key of the previous draw call, to detect breaks
    BatchKey mPreviousKey;
    //! True once mPreviousKey holds a real draw
    bool mHasPreviousKey = false;

    // ---- current GL state, as last requested ----
    TextureHandle mCurrentTexture = InvalidTextureHandle;
    bool          mCurrentTiled   = false;
    bool          mBlendEnabled   = false;
    BlendMode     mCurrentBlend   = BlendMode::AlphaBlend;

    // ---- per frame ----
    long long mFrameDrawCalls  = 0;
    long long mFrameBatchBreaks = 0;

    // ---- totals ----
    long long mFrames          = 0;
    long long mDrawCalls       = 0;
    long long mBatchBreaks     = 0;
    long long mVertices        = 0;
    long long mMaxDrawCalls    = 0;
    long long mMaxBatchBreaks  = 0;
    long long mPerKind[(int)DrawKind::Count] = {};

    // ---- redundant state requests (what a state cache would remove) ----
    long long mBindRequests      = 0;
    long long mBindRedundant     = 0;
    long long mTilingRequests    = 0;
    long long mTilingRedundant   = 0;
    long long mBlendRequests     = 0;
    long long mBlendRedundant    = 0;

    //! GL state calls that survived the cache and reached the driver
    long long mStateCallsIssued  = 0;

    //! @brief Reset every counter
    void Reset();
};
