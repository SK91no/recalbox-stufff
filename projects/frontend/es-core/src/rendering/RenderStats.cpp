//
// Created by bkg2k on 26/07/26.
//

#include "RenderStats.h"
#include <utils/Log.h>
#include <utils/String.h>

namespace {
  //! Render one "x per frame" figure with a single decimal, without floats in the log
  String PerFrame(long long total, long long frames)
  {
    if (frames <= 0) return String("n/a");
    long long tenths = (total * 10 + frames / 2) / frames;
    return String((int)(tenths / 10)).Append('.').Append((int)(tenths % 10));
  }

  //! Render a percentage of part over total
  String Percent(long long part, long long total)
  {
    if (total <= 0) return String("n/a");
    return String((int)((part * 100 + total / 2) / total)).Append('%');
  }

  const char* KindName(RenderStats::DrawKind kind)
  {
    switch(kind)
    {
      case RenderStats::DrawKind::ColorRect        : return "color rects";
      case RenderStats::DrawKind::ColorGradient    : return "gradient rects";
      case RenderStats::DrawKind::ColorFan         : return "color fans";
      case RenderStats::DrawKind::ColorTriangles   : return "color triangles";
      case RenderStats::DrawKind::StencilFan       : return "stencil fans";
      case RenderStats::DrawKind::Lines            : return "line strips";
      case RenderStats::DrawKind::TexturedTriangles: return "textured tris";
      case RenderStats::DrawKind::TexturedQuad     : return "textured quads";
      case RenderStats::DrawKind::Count            :
      default                                      : return "unknown";
    }
  }
}

void RenderStats::CountDraw(DrawKind kind, int vertices, bool alphaTexture)
{
  bool textured = (kind == DrawKind::TexturedTriangles || kind == DrawKind::TexturedQuad);

  BatchKey key;
  key.Texture = textured ? mCurrentTexture : InvalidTextureHandle;
  key.Shader  = textured ? (alphaTexture ? 2 : 1) : 0;
  key.Blend   = mCurrentBlend;
  key.Tiled   = textured && mCurrentTiled;

  // A draw whose key differs from the previous one forces a new draw call even
  // with a perfect batcher. Same key = mergeable.
  if (!mHasPreviousKey || key != mPreviousKey)
  {
    ++mBatchBreaks;
    ++mFrameBatchBreaks;
    mPreviousKey = key;
    mHasPreviousKey = true;
  }

  ++mDrawCalls;
  ++mFrameDrawCalls;
  mVertices += vertices;
  mPerKind[(int)kind]++;
}

void RenderStats::CountBindTexture(TextureHandle id)
{
  // A null handle means "keep whatever is bound", it is not a state change
  if (id == InvalidTextureHandle) return;

  ++mBindRequests;
  if (id == mCurrentTexture) ++mBindRedundant;
  mCurrentTexture = id;
}

void RenderStats::CountTiling(bool tiled)
{
  ++mTilingRequests;
  if (tiled == mCurrentTiled) ++mTilingRedundant;
  mCurrentTiled = tiled;
}

void RenderStats::CountEnableBlend(BlendMode blend)
{
  ++mBlendRequests;
  if (mBlendEnabled && blend == mCurrentBlend) ++mBlendRedundant;
  mBlendEnabled = true;
  mCurrentBlend = blend;
}

void RenderStats::CountDisableBlend()
{
  ++mBlendRequests;
  if (!mBlendEnabled) ++mBlendRedundant;
  mBlendEnabled = false;
}

void RenderStats::EndFrame()
{
  ++mFrames;
  if (mFrameDrawCalls   > mMaxDrawCalls)   mMaxDrawCalls   = mFrameDrawCalls;
  if (mFrameBatchBreaks > mMaxBatchBreaks) mMaxBatchBreaks = mFrameBatchBreaks;
  mFrameDrawCalls = 0;
  mFrameBatchBreaks = 0;

  // State does not carry over a frame boundary for batching purposes
  mHasPreviousKey = false;
}

void RenderStats::DumpAndReset(const char* reason)
{
  // Nothing rendered since last dump: stay quiet. Keeps Finalize() followed by
  // ~Renderer() from logging an empty report twice.
  if (mFrames == 0) return;

  long long saved = mDrawCalls - mBatchBreaks;

  LOG(LogInfo) << "[RenderStats] ===== Rendering statistics (" << reason << ") =====";
  LOG(LogInfo) << "[RenderStats] Frames rendered       : " << mFrames;
  LOG(LogInfo) << "[RenderStats] Draw calls            : " << mDrawCalls
               << " total, " << PerFrame(mDrawCalls, mFrames) << " avg/frame, "
               << mMaxDrawCalls << " max/frame";
  LOG(LogInfo) << "[RenderStats] After ideal batching  : " << mBatchBreaks
               << " total, " << PerFrame(mBatchBreaks, mFrames) << " avg/frame, "
               << mMaxBatchBreaks << " max/frame";
  LOG(LogInfo) << "[RenderStats] Draw calls removable  : " << saved
               << " (" << Percent(saved, mDrawCalls) << " of all draw calls)";
  LOG(LogInfo) << "[RenderStats] Vertices              : " << mVertices
               << " total, " << PerFrame(mVertices, mFrames) << " avg/frame";

  LOG(LogInfo) << "[RenderStats] --- draw calls per frame, by kind ---";
  for (int i = 0; i < (int)DrawKind::Count; ++i)
    if (mPerKind[i] != 0)
      LOG(LogInfo) << "[RenderStats]   " << KindName((DrawKind)i) << " : "
                   << PerFrame(mPerKind[i], mFrames) << " avg/frame ("
                   << mPerKind[i] << " total)";

  LOG(LogInfo) << "[RenderStats] --- redundant state requests (what a state cache would drop) ---";
  LOG(LogInfo) << "[RenderStats]   texture binds : " << mBindRedundant << " / " << mBindRequests
               << " (" << Percent(mBindRedundant, mBindRequests) << ')';
  LOG(LogInfo) << "[RenderStats]   tiling/wrap   : " << mTilingRedundant << " / " << mTilingRequests
               << " (" << Percent(mTilingRedundant, mTilingRequests) << ')';
  LOG(LogInfo) << "[RenderStats]   blend toggles : " << mBlendRedundant << " / " << mBlendRequests
               << " (" << Percent(mBlendRedundant, mBlendRequests) << ')';

  // What the state cache lets through, versus what a cacheless renderer would
  // have issued: bind (1) + wrap (2) + blend enable (2) / disable (1).
  long long uncached = mBindRequests + mTilingRequests * 2 + mBlendRequests + mBlendRequests / 2;
  LOG(LogInfo) << "[RenderStats] --- state GL calls actually issued ---";
  LOG(LogInfo) << "[RenderStats]   issued        : " << mStateCallsIssued
               << " (" << PerFrame(mStateCallsIssued, mFrames) << " per frame)";
  LOG(LogInfo) << "[RenderStats]   without cache : " << uncached
               << " (" << PerFrame(uncached, mFrames) << " per frame), saved "
               << Percent(uncached - mStateCallsIssued, uncached);
  LOG(LogInfo) << "[RenderStats] =============================================";

  Reset();
}

void RenderStats::Reset()
{
  mHasPreviousKey = false;
  mPreviousKey = BatchKey();

  mFrameDrawCalls = mFrameBatchBreaks = 0;
  mFrames = mDrawCalls = mBatchBreaks = mVertices = 0;
  mMaxDrawCalls = mMaxBatchBreaks = 0;
  for (long long& v : mPerKind) v = 0;

  mBindRequests = mBindRedundant = 0;
  mTilingRequests = mTilingRedundant = 0;
  mBlendRequests = mBlendRedundant = 0;
  mStateCallsIssued = 0;
}
