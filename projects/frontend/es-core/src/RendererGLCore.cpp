#ifdef USE_OPENGL_CORE

#include "RendererGLCore.h"
#include "rendering/opengl/GLBlend.h"
#include <utils/Log.h>
#include <cmath>
#include <cstddef>

typedef GLshort GLType;

namespace {
  // Equivalent to glOrtho(l, r, b, t, n, f) — builds a column-major
  // orthographic projection stored as Transform4x4f.
  Transform4x4f BuildOrtho(float l, float r, float b, float t, float n, float f)
  {
    return {
      {  2.f/(r-l),           0.f,           0.f, 0.f },
      {        0.f,     2.f/(t-b),           0.f, 0.f },
      {        0.f,           0.f,   -2.f/(f-n), 0.f },
      { -(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n), 1.f }
    };
  }
} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

RendererGLCore::RendererGLCore(int width, int height, bool windowed, RotationType rotation)
  : RendererGLCommon(width, height, windowed, rotation)
  , mVAO(0)
  , mVBOVerts(0)
  , mVBOColors(0)
{
  Initialize(width, height);
}

// ---------------------------------------------------------------------------
// DoConfigureGLProfile
// ---------------------------------------------------------------------------

void RendererGLCore::DoConfigureGLProfile()
{
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
}

// ---------------------------------------------------------------------------
// Streaming VBO helper
// ---------------------------------------------------------------------------

void RendererGLCore::StreamUpload(GLuint vbo, const void* data, GLsizeiptr size)
{
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  // Orphan the previous storage to avoid CPU/GPU sync, then upload fresh data.
  glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STREAM_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

// ---------------------------------------------------------------------------
// DoInitContext / InitShaders
// ---------------------------------------------------------------------------

bool RendererGLCore::InitShaders()
{
  // ---- GLSL 3.30 shader sources ----
  // Mirrors the GLSL ES 3.00 path used by RendererGLES3, with two desktop
  // adjustments:
  //   • #version 330 core  (vs `300 es`)
  //   • no precision qualifier (desktop GLSL has no mediump/highp requirement)
  // Otherwise: layout(location) for attributes and fragment outputs, in/out,
  // texture() overload, explicit fragColor.

  const char* vertColorSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec2 a_pos;\n"
    "layout(location = 1) in vec4 a_col;\n"
    "uniform mat4 u_proj;\n"
    "uniform mat4 u_mv;\n"
    "out vec4 v_col;\n"
    "void main() {\n"
    "  gl_Position = u_proj * u_mv * vec4(a_pos, 0.0, 1.0);\n"
    "  v_col = a_col;\n"
    "}\n";

  const char* vertTexSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec2 a_pos;\n"
    "layout(location = 1) in vec4 a_col;\n"
    "layout(location = 2) in vec2 a_tex;\n"
    "uniform mat4 u_proj;\n"
    "uniform mat4 u_mv;\n"
    "out vec2 v_tex;\n"
    "out vec4 v_col;\n"
    "void main() {\n"
    "  gl_Position = u_proj * u_mv * vec4(a_pos, 0.0, 1.0);\n"
    "  v_tex = a_tex;\n"
    "  v_col = a_col;\n"
    "}\n";

  const char* fragColorSrc =
    "#version 330 core\n"
    "in vec4 v_col;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() {\n"
    "  fragColor = v_col;\n"
    "}\n";

  const char* fragTexSrc =
    "#version 330 core\n"
    "in vec2 v_tex;\n"
    "in vec4 v_col;\n"
    "uniform sampler2D u_sampler;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() {\n"
    "  fragColor = texture(u_sampler, v_tex) * v_col;\n"
    "}\n";

  // Coverage is sampled from RED, not ALPHA: the core profile has no GL_ALPHA
  // texture format, so RendererGLCommon uploads glyph atlases as GL_R8 on this
  // backend. See the format note above DoUploadTexture — the two must agree, or
  // glyphs turn back into solid rectangles.
  const char* fragAlphaSrc =
    "#version 330 core\n"
    "in vec2 v_tex;\n"
    "in vec4 v_col;\n"
    "uniform sampler2D u_sampler;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() {\n"
    "  fragColor = vec4(v_col.rgb, texture(u_sampler, v_tex).r * v_col.a);\n"
    "}\n";

  // Compilation, linking and error reporting all live in Shader. No attribute
  // bindings to pass: GLSL 3.30 fixes both the attribute slots and the fragment
  // output location with layout qualifiers in the source.
  if (!mShaderColor.Build(vertColorSrc, fragColorSrc)) return false;
  if (!mShaderTexture.Build(vertTexSrc, fragTexSrc)) return false;
  if (!mShaderAlpha.Build(vertTexSrc, fragAlphaSrc)) return false;

  // Resolve u_proj / u_mv once per program. From here on a matrix change costs
  // nothing until a draw actually needs that program.
  RegisterProgram(ProgramSlot::Color,   mShaderColor.Program());
  RegisterProgram(ProgramSlot::Texture, mShaderTexture.Program());
  RegisterProgram(ProgramSlot::Alpha,   mShaderAlpha.Program());

  mMVMatrix = Transform4x4f::Identity();
  InvalidateMatrices();

  for (GLuint prog : { mShaderColor.Program(), mShaderTexture.Program(), mShaderAlpha.Program() })
  {
    UseProgram(prog);
    glVertexAttrib4f(1 /* a_col */, 1.f, 1.f, 1.f, 1.f);
  }
  // Bind texture sampler to unit 0 in both texture programs.
  UseProgram(mShaderTexture.Program()); glUniform1i(glGetUniformLocation(mShaderTexture.Program(), "u_sampler"), 0);
  UseProgram(mShaderAlpha.Program());   glUniform1i(glGetUniformLocation(mShaderAlpha.Program(),   "u_sampler"), 0);

  UseProgram(mShaderColor.Program()); // default active program
  { LOG(LogInfo) << "[RendererGLCore] GLSL 3.30 shader programs initialised successfully."; }
  return true;
}

bool RendererGLCore::DoInitContext()
{
  ResetStateCache(); // fresh GL context: every cached binding is stale
  // Wipe any previous resources (in case of ReInitialize without a full Finalize).
  // Forget the old program names WITHOUT deleting them: they belong to a context
  // that no longer exists, so glDeleteProgram on them would address a dead object.
  mShaderColor.Reset(false);
  mShaderTexture.Reset(false);
  mShaderAlpha.Reset(false);
  if (mVBOVerts  != 0) { glDeleteBuffers(1, &mVBOVerts);  mVBOVerts  = 0; }
  if (mVBOColors != 0) { glDeleteBuffers(1, &mVBOColors); mVBOColors = 0; }
  if (mVAO       != 0) { glDeleteVertexArrays(1, &mVAO);  mVAO       = 0; }

  // Core profile requires a non-zero VAO bound at all times.  We use a single
  // VAO for the whole renderer; attribute pointers are reset per draw.
  glGenVertexArrays(1, &mVAO);
  glBindVertexArray(mVAO);

  // Streaming VBOs (sizes grow on demand via glBufferData inside StreamUpload).
  glGenBuffers(1, &mVBOVerts);
  glGenBuffers(1, &mVBOColors);

  return InitShaders();
}

// ---------------------------------------------------------------------------
// DoFinalizeContext
// ---------------------------------------------------------------------------

void RendererGLCore::DoFinalizeContext() noexcept
{
  // Still inside a live context here, so the programs are really deleted. That
  // also leaves the Shader members empty, which makes their destructors no-ops
  // later on, whichever teardown path got us there.
  mShaderColor.Reset();
  mShaderTexture.Reset();
  mShaderAlpha.Reset();
  if (mVBOVerts  != 0) { glDeleteBuffers(1, &mVBOVerts);  mVBOVerts  = 0; }
  if (mVBOColors != 0) { glDeleteBuffers(1, &mVBOColors); mVBOColors = 0; }
  if (mVAO       != 0) { glDeleteVertexArrays(1, &mVAO);  mVAO       = 0; }
}

// ---------------------------------------------------------------------------
// Projection
// ---------------------------------------------------------------------------

void RendererGLCore::DoBeginProjection(int w, int h)
{
  mProjMatrix = BuildOrtho(0.f, (float)w, (float)h, 0.f, -1.f, 1.f);
}

void RendererGLCore::DoApplyRotationToProjection(RotationType rotation, int viewW, int viewH)
{
  switch (rotation)
  {
    case RotationType::Right:
      mProjMatrix.rotateZ((float)M_PI_2);
      mProjMatrix.translate(0.f, -(float)viewW);
      break;
    case RotationType::Left:
      mProjMatrix.rotateZ(-(float)M_PI_2);
      mProjMatrix.translate(-(float)viewH, 0.f);
      break;
    case RotationType::Upsidedown:
      mProjMatrix.rotateZ((float)M_PI);
      mProjMatrix.translate(-(float)viewW, -(float)viewH);
      break;
    case RotationType::None:
    default:
      break;
  }
}

void RendererGLCore::DoEndProjection(float sx, float sy)
{
  mProjMatrix.scale(Vector3f(sx, sy, 1.f));
  InvalidateMatrices();
}

// ---------------------------------------------------------------------------
// DoSetMatrix
// ---------------------------------------------------------------------------

void RendererGLCore::DoSetMatrix(const Transform4x4f& transform)
{
  // No GL call here on purpose. The matrix reaches a program the next time that
  // program is selected for a draw, and only if it has not seen this one yet —
  // see RendererGLCommon::UseProgram.
  mMVMatrix = Transform4x4f(transform).round();
  InvalidateMatrices();
}

// ---------------------------------------------------------------------------
// Colour rectangle
// ---------------------------------------------------------------------------

void RendererGLCore::DoDrawColorRect(int x, int y, int w, int h,
                                      Colors::ColorARGB color,
                                      BlendMode blend)
{
  GLType points[12]
  {
    (GLType)x      , (GLType)y      ,
    (GLType)x      , (GLType)(y + h),
    (GLType)(x + w), (GLType)y      ,
    (GLType)(x + w), (GLType)y      ,
    (GLType)x      , (GLType)(y + h),
    (GLType)(x + w), (GLType)(y + h),
  };
  GLubyte colors[6 * 4];
  BuildGLColorArray(colors, color, 6);

  DoEnableBlend(blend);
  UseProgram(mShaderColor.Program());

  StreamUpload(mVBOVerts, points, sizeof(points));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_SHORT, GL_FALSE, 0, (void*)0);

  StreamUpload(mVBOColors, colors, sizeof(colors));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  DoDisableBlend();
}

void RendererGLCore::DoDrawColorRect4(int x, int y, int w, int h,
                                       Colors::ColorARGB tl, Colors::ColorARGB tr,
                                       Colors::ColorARGB br, Colors::ColorARGB bl)
{
  GLType points[12]
  {
    (GLType)x      , (GLType)y      ,
    (GLType)x      , (GLType)(y + h),
    (GLType)(x + w), (GLType)y      ,
    (GLType)(x + w), (GLType)y      ,
    (GLType)x      , (GLType)(y + h),
    (GLType)(x + w), (GLType)(y + h),
  };
  GLuint colors[6]
  {
    ColorToGL(tl),
    ColorToGL(bl),
    ColorToGL(tr),
    ColorToGL(tr),
    ColorToGL(bl),
    ColorToGL(br),
  };

  DoEnableBlend(BlendMode::AlphaBlend);
  UseProgram(mShaderColor.Program());

  StreamUpload(mVBOVerts, points, sizeof(points));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_SHORT, GL_FALSE, 0, (void*)0);

  StreamUpload(mVBOColors, colors, sizeof(colors));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  DoDisableBlend();
}

// ---------------------------------------------------------------------------
// Fan primitives
// ---------------------------------------------------------------------------

void RendererGLCore::DoDrawColorTriangles(const Vector2f* verts, const ColorByte* colors, int count)
{
  DoEnableBlend(BlendMode::AlphaBlend);
  UseProgram(mShaderColor.Program());

  // Core profile forbids client-side arrays, so both streams go through the
  // streaming VBOs, positions and colours in their own buffer.
  StreamUpload(mVBOVerts, verts, (GLsizeiptr)((size_t)count * sizeof(Vector2f)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

  StreamUpload(mVBOColors, colors, (GLsizeiptr)((size_t)count * 4));
  glEnableVertexAttribArray(1);
  // Already in GPU byte order, so normalised bytes go straight to the shader.
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);

  glDrawArrays(GL_TRIANGLES, 0, count);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
}

void RendererGLCore::DoDrawColorFan(const float* verts, int count, unsigned int color)
{
  DoEnableBlend(BlendMode::AlphaBlend);
  UseProgram(mShaderColor.Program());
  glVertexAttrib4f(1 /* a_col */,
                   (float)(GLubyte)(color >> 24) / 255.f,
                   (float)(GLubyte)(color >> 16) / 255.f,
                   (float)(GLubyte)(color >>  8) / 255.f,
                   (float)(GLubyte)(color >>  0) / 255.f);

  StreamUpload(mVBOVerts, verts, (GLsizeiptr)(count * 2 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glDrawArrays(GL_TRIANGLE_FAN, 0, count);

  glDisableVertexAttribArray(0);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
  DoDisableBlend();
}

void RendererGLCore::DoWriteStencilFan(const float* verts, int count)
{
  glEnable(GL_STENCIL_TEST);
  glClear(GL_STENCIL_BUFFER_BIT);

  // Write 1 into stencil where the shape is drawn; suppress all colour output.
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDepthMask(GL_FALSE);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

  UseProgram(mShaderColor.Program());

  StreamUpload(mVBOVerts, verts, (GLsizeiptr)(count * 2 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glDrawArrays(GL_TRIANGLE_FAN, 0, count);

  glDisableVertexAttribArray(0);

  // Restore colour/depth writes and arm stencil test for subsequent draws.
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
  glStencilFunc(GL_EQUAL, 1, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  // GL_STENCIL_TEST stays enabled until ClearStencil()
}

// ---------------------------------------------------------------------------
// Lines
// ---------------------------------------------------------------------------

void RendererGLCore::DoDrawLines(const Vector2f* coords, const Colors::ColorARGB* colors, int count)
{
  DoEnableBlend(BlendMode::AlphaBlend);
  UseProgram(mShaderColor.Program());

  StreamUpload(mVBOVerts, coords, (GLsizeiptr)(count * sizeof(Vector2f)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

  StreamUpload(mVBOColors, colors, (GLsizeiptr)(count * sizeof(Colors::ColorARGB)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);

  glDrawArrays(GL_LINES, 0, count);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  DoDisableBlend();
}

void RendererGLCore::DoDrawLinesUniform(const Vector2f* coords, Colors::ColorARGB color, int count)
{
  DoEnableBlend(BlendMode::AlphaBlend);
  UseProgram(mShaderColor.Program());
  glVertexAttrib4f(1,
                   (float)(GLubyte)(color >> 24) / 255.f,
                   (float)(GLubyte)(color >> 16) / 255.f,
                   (float)(GLubyte)(color >>  8) / 255.f,
                   (float)(GLubyte)(color >>  0) / 255.f);

  StreamUpload(mVBOVerts, coords, (GLsizeiptr)(count * sizeof(Vector2f)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glDrawArrays(GL_LINES, 0, count);

  glDisableVertexAttribArray(0);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
  DoDisableBlend();
}

// ---------------------------------------------------------------------------
// Textured triangles
// ---------------------------------------------------------------------------

void RendererGLCore::DoDrawTTWithColors(const Vertex* verts, const ColorByte* colors, int count, TextureType type)
{
  UseProgram(type == TextureType::Alpha ? mShaderAlpha.Program() : mShaderTexture.Program());

  StreamUpload(mVBOVerts, verts, (GLsizeiptr)(count * sizeof(Vertex)));
  glEnableVertexAttribArray(0); // a_pos
  glEnableVertexAttribArray(2); // a_tex
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Target));
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Source));

  StreamUpload(mVBOColors, colors, (GLsizeiptr)(count * 4 * sizeof(ColorByte)));
  glEnableVertexAttribArray(1); // a_col
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);

  glDrawArrays(GL_TRIANGLES, 0, count);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  // No "restore the colour program" here any more. Every draw selects the
  // program it needs, so switching back was a guaranteed extra glUseProgram —
  // and two per draw instead of zero across a run of text, which is the most
  // common case in this UI.
}

void RendererGLCore::DoDrawTTUniform(const Vertex* verts, Colors::ColorARGB color, int count, TextureType type)
{
  UseProgram(type == TextureType::Alpha ? mShaderAlpha.Program() : mShaderTexture.Program());
  glVertexAttrib4f(1,
                   (float)(GLubyte)(color >> 24) / 255.f,
                   (float)(GLubyte)(color >> 16) / 255.f,
                   (float)(GLubyte)(color >>  8) / 255.f,
                   (float)(GLubyte)(color >>  0) / 255.f);

  StreamUpload(mVBOVerts, verts, (GLsizeiptr)(count * sizeof(Vertex)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Target));
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Source));

  glDrawArrays(GL_TRIANGLES, 0, count);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(2);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
}

// ---------------------------------------------------------------------------
// Texture quads
// ---------------------------------------------------------------------------

void RendererGLCore::DoSubmitTexQuadUniform(const Vertex* verts, int count,
                                             float r, float g, float b, float a,
                                             TextureType type)
{
  UseProgram(type == TextureType::Alpha ? mShaderAlpha.Program() : mShaderTexture.Program());
  glVertexAttrib4f(1, r, g, b, a);

  StreamUpload(mVBOVerts, verts, (GLsizeiptr)(count * sizeof(Vertex)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Target));
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Source));

  glDrawArrays(GL_TRIANGLES, 0, count);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(2);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
}

void RendererGLCore::DoSubmitTexQuadColors(const Vertex* verts, const Colors::ColorRGBA* colors,
                                            int count, TextureType type)
{
  UseProgram(type == TextureType::Alpha ? mShaderAlpha.Program() : mShaderTexture.Program());

  StreamUpload(mVBOVerts, verts, (GLsizeiptr)(count * sizeof(Vertex)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Target));
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Source));

  StreamUpload(mVBOColors, colors, (GLsizeiptr)(count * sizeof(GLuint)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);

  glDrawArrays(GL_TRIANGLES, 0, count);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
}

void RendererGLCore::DoSubmitTexQuadColorsTwoDraws(const Vertex* v1, const Colors::ColorRGBA* c1,
                                                    const Vertex* v2, const Colors::ColorRGBA* c2,
                                                    int count, TextureType type)
{
  UseProgram(type == TextureType::Alpha ? mShaderAlpha.Program() : mShaderTexture.Program());

  // Pack both quads' verts/colors into single uploads to keep the VBO
  // streaming cheap.
  const GLsizeiptr vertsSize  = (GLsizeiptr)(count * sizeof(Vertex));
  const GLsizeiptr colorsSize = (GLsizeiptr)(count * sizeof(GLuint));

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);

  // First draw
  StreamUpload(mVBOVerts, v1, vertsSize);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Target));
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Source));
  StreamUpload(mVBOColors, c1, colorsSize);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);
  glDrawArrays(GL_TRIANGLES, 0, count);

  // Second draw (reflection)
  StreamUpload(mVBOVerts, v2, vertsSize);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Target));
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Source));
  StreamUpload(mVBOColors, c2, colorsSize);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);
  glDrawArrays(GL_TRIANGLES, 0, count);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
}

#endif // USE_OPENGL_CORE
