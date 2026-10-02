//
// Created by bkg2k on 26/07/26.
//

#ifdef USE_OPENGL

#include "RendererGLCommon.h"
#include "rendering/opengl/GLBlend.h"
#include "rendering/opengl/Debug.h"
#include "rendering/RenderStats.h"
#include <utils/Log.h>

// ---------------------------------------------------------------------------
// Driver debug output
// ---------------------------------------------------------------------------

// glDebugMessageCallback is a desktop GL 4.3 entry point. The condition used to
// be "not GLES2", which wrongly let a GLES 3.1 build in — GLES has no such call.
#if defined(DEBUG) && defined(USE_OPENGL_CORE)

static void APIENTRY GLDebugCallback(GLenum source,
                                     GLenum type,
                                     GLuint id,
                                     GLenum severity,
                                     GLsizei length,
                                     const GLchar* message,
                                     const void* userParam)
{
  (void)source;
  (void)length;
  (void)userParam;

  const char* typeString = "UNKNOWN";
  switch (type)
  {
    case GL_DEBUG_TYPE_ERROR: typeString = "ERROR"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeString = "DEPRECATED_BEHAVIOR"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: typeString = "UNDEFINED_BEHAVIOR"; break;
    case GL_DEBUG_TYPE_PORTABILITY: typeString = "PORTABILITY"; break;
    case GL_DEBUG_TYPE_PERFORMANCE: typeString = "PERFORMANCE"; break;
    case GL_DEBUG_TYPE_OTHER: typeString = "OTHER"; break;
    default: break;
  }

  const char* severityString = "UNKNOWN";
  switch (severity)
  {
    case GL_DEBUG_SEVERITY_LOW: severityString = "LOW"; break;
    case GL_DEBUG_SEVERITY_MEDIUM: severityString = "MEDIUM"; break;
    case GL_DEBUG_SEVERITY_HIGH: severityString = "HIGH"; break;
    default: break;
  }

  { LOG(LogError) << "[Renderer] GL Error [Type:" << typeString << " - Severity:" << severityString << "] {id:" << id << "} " << message; }
}

void RendererGLCommon::DoActivateDebug()
{
  // GL_DEBUG_OUTPUT is what actually turns the channel on. It defaults to on
  // only inside a debug context, and nothing was requesting one, so the callback
  // installed below never received a single message. DoConfigureContext now asks
  // for the debug context, and this enables the output explicitly so the feature
  // does not depend on the driver's default either way.
  glEnable(GL_DEBUG_OUTPUT);
  // Synchronous so the callback fires inside the offending call and a breakpoint
  // there still has the real stack.
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(GLDebugCallback, nullptr);
  // Enable every source, type and severity. The id list is empty, so it must be
  // a null pointer rather than the address of an unused variable.
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

  GLint flags = 0;
  glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
  if ((flags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0)
  { LOG(LogInfo) << "[Renderer] GL debug output active on a debug context."; }
  else
  { LOG(LogWarning) << "[Renderer] GL debug output enabled, but the driver gave a non-debug context: messages may be sparse."; }
}

#else

void RendererGLCommon::DoActivateDebug()
{
  { LOG(LogInfo) << "[Renderer] GL Debug not activated in GLES."; }
}

#endif

// ---------------------------------------------------------------------------
// Context lifecycle
// ---------------------------------------------------------------------------

void RendererGLCommon::DoConfigureContext()
{
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  // multisample anti-aliasing
  //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);

  // A debug context is what makes GL_DEBUG_OUTPUT meaningful: outside one, the
  // driver is free to report nothing at all. It must be asked for before the
  // context is created, which is why it lives here and not in DoActivateDebug.
  #ifdef DEBUG
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
  #endif

  // Let the concrete backend pick profile/version (GLES2 vs GLES3 vs Desktop GL)
  DoConfigureGLProfile();
}

unsigned int RendererGLCommon::DoWindowFlags() const
{
  return SDL_WINDOW_OPENGL;
}

bool RendererGLCommon::DoCreateContext(SDL_Window* window)
{
  mSdlGLContext = SDL_GL_CreateContext(window);
  if (mSdlGLContext == nullptr)
  {
    { LOG(LogError) << "[Renderer] Error creating GL context!\n\t" << SDL_GetError(); }
    return false;
  }

  { LOG(LogInfo) << "[Renderer] Activating vertical sync'"; }
  if (SDL_GL_SetSwapInterval(1) == 0) { LOG(LogInfo) << "[Renderer] Normal VSync' activated."; }
  else { LOG(LogWarning) << "[Renderer] Tried to enable vsync, but failed! (" << SDL_GetError() << ')'; }

  return true;
}

void RendererGLCommon::DoDestroyContext() noexcept
{
  SDL_GL_DeleteContext(mSdlGLContext);
  mSdlGLContext = nullptr;
}

void RendererGLCommon::DoPresent(SDL_Window* window)
{
  SDL_GL_SwapWindow(window);
}

void RendererGLCommon::ResetStateCache()
{
  mTextureWrap.Clear();
  mBoundTexture   = InvalidTextureHandle;
  mBlendEnabled   = false;
  mBlendModeKnown = false;
  mBlendMode      = BlendMode::AlphaBlend;
  mScissorEnabled = false;
  mScissorKnown   = false;
  ResetProgramCache();
}

// ---------------------------------------------------------------------------
// Programs and matrices
// ---------------------------------------------------------------------------

void RendererGLCommon::ResetProgramCache()
{
  for (ProgramState& state : mProgramStates) state = ProgramState();
  mActiveProgram = 0;
  mMatrixSerial  = 1;
}

RenderingError RendererGLCommon::DrainErrors(const char* what)
{
  RenderingError result = RenderingError::NoError;
  for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
  {
    // Out of memory outranks everything: it is the one the texture cache acts on.
    if (error == GL_OUT_OF_MEMORY) result = RenderingError::OutOfGPUMemory;
    else if (result == RenderingError::NoError) result = RenderingError::NoResource;
    { LOG(LogError) << "[Renderer] " << what << " raised GL error 0x" << String().AssignHexa((int)error); }
  }
  return result;
}

void RendererGLCommon::RegisterProgram(ProgramSlot slot, unsigned int program)
{
  ProgramState& state = mProgramStates[(int)slot];
  state.Program = program;
  // Resolved once, here, rather than by name on every draw.
  state.ProjLocation = glGetUniformLocation(program, "u_proj");
  state.MVLocation   = glGetUniformLocation(program, "u_mv");
  // 0 means "has seen no serial", so the first use always uploads.
  state.MatrixSerial = 0;
}

void RendererGLCommon::UseProgram(unsigned int program)
{
  if (program != mActiveProgram)
  {
    glUseProgram(program);
    RenderStats::Instance().CountIssuedStateCalls(1);
    mActiveProgram = program;
  }

  for (ProgramState& state : mProgramStates)
  {
    if (state.Program != program) continue;
    if (state.MatrixSerial == mMatrixSerial) return;   // already current

    if (state.ProjLocation >= 0)
    {
      glUniformMatrix4fv(state.ProjLocation, 1, GL_FALSE, (const float*)&mProjMatrix);
      RenderStats::Instance().CountIssuedStateCalls(1);
    }
    if (state.MVLocation >= 0)
    {
      glUniformMatrix4fv(state.MVLocation, 1, GL_FALSE, (const float*)&mMVMatrix);
      RenderStats::Instance().CountIssuedStateCalls(1);
    }
    state.MatrixSerial = mMatrixSerial;
    return;
  }
}

void RendererGLCommon::SetWrapOf(TextureHandle id, WrapState state)
{
  if ((int)id >= mTextureWrap.Count())
    while (mTextureWrap.Count() <= (int)id)
      mTextureWrap.Add(WrapState::Unknown);
  mTextureWrap((int)id) = state;
}

void RendererGLCommon::BindTextureCached(TextureHandle id)
{
  if (id == mBoundTexture) return;
  glBindTexture(GL_TEXTURE_2D, id);
  RenderStats::Instance().CountIssuedStateCalls(1);
  mBoundTexture = id;
}

TextureHandle RendererGLCommon::DoCreateTexture(bool nearest)
{
  TextureHandle id = InvalidTextureHandle;

  glGenTextures(1, &id);
  BindTextureCached(id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest ? GL_NEAREST : GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest ? GL_NEAREST : GL_LINEAR);

  // GL handles get recycled after a delete, so never inherit a stale wrap state
  SetWrapOf(id, WrapState::Unknown);

  return id;
}

void RendererGLCommon::DoDestroyTexture(TextureHandle id)
{
  glDeleteTextures(1, &id);
  SetWrapOf(id, WrapState::Unknown);
  // GL unbinds a deleted texture on its own; keep the cache in step
  if (id == mBoundTexture) mBoundTexture = InvalidTextureHandle;
}

void RendererGLCommon::DoBindTexture(TextureHandle id)
{
  RenderStats::Instance().CountBindTexture(id);
  // A null handle means "keep whatever is bound"
  if (id != InvalidTextureHandle)
    BindTextureCached(id);
}

void RendererGLCommon::DoSetTextureTiling(bool tiled)
{
  RenderStats::Instance().CountTiling(tiled);

  WrapState wanted = tiled ? WrapState::Repeat : WrapState::Clamp;

  // Wrap belongs to the texture object, so the cache is keyed on the bound one.
  // With no texture bound there is nothing meaningful to set.
  if (mBoundTexture == InvalidTextureHandle) return;
  if (WrapOf(mBoundTexture) == wanted) return;

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tiled ? GL_REPEAT : GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tiled ? GL_REPEAT : GL_CLAMP_TO_EDGE);
  RenderStats::Instance().CountIssuedStateCalls(2);
  SetWrapOf(mBoundTexture, wanted);
}

// ---------------------------------------------------------------------------
// Single-channel (font glyph) texture format
//
// GL_ALPHA belongs to the fixed pipeline. GLES 2 and GLES 3 keep it as a legacy
// unsized format, but Desktop GL removed it from the core profile: there,
// glTexImage2D(GL_ALPHA) fails with GL_INVALID_VALUE and the texture is left
// with no storage at all. Sampling an incomplete texture yields (0, 0, 0, 1),
// so the alpha the glyph shader reads is always 1 and every character renders
// as a solid filled rectangle.
//
// Core uses the sized single-channel format instead. It carries the coverage
// value in the RED channel, so the alpha shader of that backend samples .r
// where the GLES ones sample .a — the same split the Vulkan backend already
// makes with VK_FORMAT_R8_UNORM. Keep the two in step: changing the format here
// without changing the channel in the shader brings the solid boxes straight
// back.
// ---------------------------------------------------------------------------
#ifdef USE_OPENGL_CORE
  static constexpr GLint  sAlphaInternalFormat = GL_R8;
  static constexpr GLenum sAlphaFormat         = GL_RED;
#else
  static constexpr GLint  sAlphaInternalFormat = GL_ALPHA;
  static constexpr GLenum sAlphaFormat         = GL_ALPHA;
#endif

RenderingError RendererGLCommon::DoUploadTexture(TextureHandle id, int width, int height,
                                                 const void* data, TextureType type)
{
  // Start from a clean queue so what follows is reported against this upload
  // alone, not against something a previous call left behind.
  DrainErrors("a call before texture upload");

  BindTextureCached(id);
  if (RenderingError error = DrainErrors("texture bind"); error != RenderingError::NoError) return error;

  if (type == TextureType::Alpha)
  {
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, sAlphaInternalFormat, width, height, 0, sAlphaFormat, GL_UNSIGNED_BYTE, data);
  }
  else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

  return DrainErrors("glTexImage2D");
}

RenderingError RendererGLCommon::DoUploadTexturePart(TextureHandle id, int x, int y, int width, int height,
                                                     const void* data, TextureType type)
{
  // Same reasoning as DoUploadTexture: start from a clean error queue.
  DrainErrors("a call before texture sub-upload");

  BindTextureCached(id);
  if (RenderingError error = DrainErrors("texture bind"); error != RenderingError::NoError) return error;

  // Glyphs come in one byte per pixel at arbitrary widths, so rows are not
  // necessarily 4-aligned. DoUploadTexture happens to have set this already
  // when it allocated the atlas, but relying on that leftover state would
  // break the day a part upload reaches a texture allocated elsewhere.
  if (type == TextureType::Alpha) glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height,
                  type == TextureType::Alpha ? sAlphaFormat : GL_RGBA, GL_UNSIGNED_BYTE, data);

  return DrainErrors("glTexSubImage2D");
}

// ---------------------------------------------------------------------------
// Render state
// ---------------------------------------------------------------------------

void RendererGLCommon::DoEnableBlend(BlendMode blend)
{
  RenderStats::Instance().CountEnableBlend(blend);

  if (!mBlendEnabled)
  {
    glEnable(GL_BLEND);
    RenderStats::Instance().CountIssuedStateCalls(1);
    mBlendEnabled = true;
  }
  if (!mBlendModeKnown || blend != mBlendMode)
  {
    ApplyGLBlendMode(blend);
    RenderStats::Instance().CountIssuedStateCalls(1);
    mBlendMode = blend;
    mBlendModeKnown = true;
  }
}

void RendererGLCommon::DoDisableBlend()
{
  RenderStats::Instance().CountDisableBlend();

  // Deliberately left enabled. Every draw path in the frontend asks for
  // blending back immediately afterwards, so the enable/disable pair around
  // each draw was pure overhead — measured at ~187 GL calls per frame for no
  // visible effect. Nothing renders with blending off: the only draw that
  // skips DoEnableBlend is the stencil fan, which writes with the colour mask
  // closed. Fragments at alpha 255 composite identically either way.
  //
  // Re-enable the real glDisable here if a future draw path ever needs the
  // blender out of the way.
}

void RendererGLCommon::DoSetScissor(int x, int y, int w, int h)
{
  // Clipping is pushed and popped constantly — every scrollable container and
  // every text run does it — and the base clip pushed at startup is never
  // popped, so the test itself was being re-enabled thousands of times a frame
  // while already on. Same treatment as the bind and blend caches.
  if (!mScissorEnabled)
  {
    glEnable(GL_SCISSOR_TEST);
    RenderStats::Instance().CountIssuedStateCalls(1);
    mScissorEnabled = true;
  }
  if (mScissorKnown && mScissorRect[0] == x && mScissorRect[1] == y &&
      mScissorRect[2] == w && mScissorRect[3] == h) return;

  glScissor(x, y, w, h);
  RenderStats::Instance().CountIssuedStateCalls(1);
  mScissorRect[0] = x; mScissorRect[1] = y; mScissorRect[2] = w; mScissorRect[3] = h;
  mScissorKnown = true;
}

void RendererGLCommon::DoDisableScissor()
{
  if (!mScissorEnabled) return;
  glDisable(GL_SCISSOR_TEST);
  RenderStats::Instance().CountIssuedStateCalls(1);
  mScissorEnabled = false;
}

void RendererGLCommon::DoClearStencil()
{
  glDisable(GL_STENCIL_TEST);
  glClear(GL_STENCIL_BUFFER_BIT);
}

void RendererGLCommon::DoSetViewport(int x, int y, int w, int h)
{
  glViewport(x, y, w, h);
}

void RendererGLCommon::DoSetClearColor(float r, float g, float b, float a)
{
  glClearColor(r, g, b, a);
}

void RendererGLCommon::DoClearBuffers()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

String RendererGLCommon::DoGetDriverInfo(DriverInfo info)
{
  #ifdef USE_OPENGL_CORE
  if (info == DriverInfo::Extensions)
  {
    // glGetString(GL_EXTENSIONS) was removed from the core profile: it returns
    // null and, worse, leaves GL_INVALID_ENUM armed for whoever calls
    // glGetError next. The indexed form is the core-profile way.
    GLint count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &count);
    String result;
    for (GLint i = 0; i < count; ++i)
      if (const GLubyte* name = glGetStringi(GL_EXTENSIONS, (GLuint)i); name != nullptr)
      {
        if (!result.empty()) result.Append(' ');
        result.Append((const char*)name);
      }
    return result;
  }
  #endif

  GLenum name = GL_VENDOR;
  switch(info)
  {
    case DriverInfo::Renderer  : name = GL_RENDERER; break;
    case DriverInfo::Version   : name = GL_VERSION; break;
    case DriverInfo::Extensions: name = GL_EXTENSIONS; break;
    case DriverInfo::Vendor    :
    default                    : name = GL_VENDOR; break;
  }

  const GLubyte* s = glGetString(name);
  return s != nullptr ? String((const char*)s) : String();
}

// ---------------------------------------------------------------------------
// Offscreen target
// ---------------------------------------------------------------------------

bool RendererGLCommon::DoCreateOffscreen(int w, int h, TextureHandle& texture)
{
  DrainErrors("a call before offscreen creation");

  glGenTextures(1, &texture);
  BindTextureCached(texture);
  CHECKGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  SetWrapOf(texture, WrapState::Clamp);
  BindTextureCached(InvalidTextureHandle);

  glGenFramebuffers(1, &mOffscreenFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, mOffscreenFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

  // Colour alone is not enough. The stencil drives every rounded clip, and an
  // FBO without one would accept CircleStencil / RoundRectangleStencil and
  // simply not clip anything.
  glGenRenderbuffers(1, &mOffscreenDepthStencil);
  glBindRenderbuffer(GL_RENDERBUFFER, mOffscreenDepthStencil);
  #if defined(GL_DEPTH24_STENCIL8)
  // GLES 3 and desktop core: one packed renderbuffer serves both attachments.
  CHECKGL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h));
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mOffscreenDepthStencil);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mOffscreenDepthStencil);
  #else
  // Plain GLES 2 headers do not declare the packed format, and the frontend
  // never enables the depth test, so stencil on its own is all that is needed.
  CHECKGL(glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, w, h));
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mOffscreenDepthStencil);
  #endif
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    { LOG(LogError) << "[Renderer] Offscreen FBO incomplete (status: " << String().AssignHexa(status) << ')'; }
    return false;
  }
  DrainErrors("offscreen creation");
  return true;
}

void RendererGLCommon::DoDestroyOffscreen(TextureHandle& texture) noexcept
{
  if (mOffscreenFBO != 0) { glDeleteFramebuffers(1, &mOffscreenFBO); mOffscreenFBO = 0; }
  if (mOffscreenDepthStencil != 0) { glDeleteRenderbuffers(1, &mOffscreenDepthStencil); mOffscreenDepthStencil = 0; }
  if (texture != InvalidTextureHandle)
  {
    glDeleteTextures(1, &texture);
    // Same bookkeeping DoDestroyTexture does. GL recycles names, so a stale
    // entry here would make BindTextureCached skip a legitimate bind, or apply
    // this texture's wrap mode to whichever one inherits the handle.
    // Written directly rather than through SetWrapOf: that one grows the table,
    // and this function is noexcept.
    if ((int)texture < mTextureWrap.Count()) mTextureWrap((int)texture) = WrapState::Unknown;
    if (texture == mBoundTexture) mBoundTexture = InvalidTextureHandle;
    texture = InvalidTextureHandle;
  }
}

void RendererGLCommon::DoBeginOffScreen(int w, int h)
{
  glBindFramebuffer(GL_FRAMEBUFFER, mOffscreenFBO);
  glViewport(0, 0, w, h);
}

void RendererGLCommon::DoEndOffScreen(int w, int h)
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, w, h);
}

#endif // USE_OPENGL
