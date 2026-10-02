#ifdef USE_OPENGL_ES31

#include "RendererGLES3.h"
#include "rendering/opengl/GLBlend.h"
#include <utils/Log.h>
#include <cmath>

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

RendererGLES3::RendererGLES3(int width, int height, bool windowed, RotationType rotation)
  : RendererGLCommon(width, height, windowed, rotation)
{
  Initialize(width, height);
}

// ---------------------------------------------------------------------------
// DoConfigureGLProfile
// ---------------------------------------------------------------------------

void RendererGLES3::DoConfigureGLProfile()
{
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
}

// ---------------------------------------------------------------------------
// DoInitContext / InitShaders
// ---------------------------------------------------------------------------

bool RendererGLES3::InitShaders()
{
  // ---- GLSL ES 3.00 shader sources ----
  // Key differences vs GLSL ES 1.00 (GLES2):
  //   • #version 300 es
  //   • layout(location = N) in  — fixed attribute slots, no glBindAttribLocation
  //   • out / in  instead of  attribute / varying
  //   • explicit fragment output (out vec4 fragColor)  instead of  gl_FragColor
  //   • texture()  instead of  texture2D()
  // These are essentially identical to GLSL 1.30 (GL Core 3.0), differing only
  // in the #version directive and the mandatory precision qualifier.

  const char* vertColorSrc =
    "#version 300 es\n"
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
    "#version 300 es\n"
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
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec4 v_col;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "  fragColor = v_col;\n"
    "}\n";

  const char* fragTexSrc =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec2 v_tex;\n"
    "in vec4 v_col;\n"
    "uniform sampler2D u_sampler;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "  fragColor = texture(u_sampler, v_tex) * v_col;\n"
    "}\n";

  const char* fragAlphaSrc =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec2 v_tex;\n"
    "in vec4 v_col;\n"
    "uniform sampler2D u_sampler;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "  fragColor = vec4(v_col.rgb, texture(u_sampler, v_tex).a * v_col.a);\n"
    "}\n";

  // Compilation, linking and error reporting all live in Shader. No attribute
  // bindings to pass: GLSL ES 3.00 fixes the locations with layout qualifiers
  // in the source (0 = a_pos, 1 = a_col, 2 = a_tex).
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
  // Bind texture sampler to unit 0 in both texture programs
  UseProgram(mShaderTexture.Program()); glUniform1i(glGetUniformLocation(mShaderTexture.Program(), "u_sampler"), 0);
  UseProgram(mShaderAlpha.Program());   glUniform1i(glGetUniformLocation(mShaderAlpha.Program(),   "u_sampler"), 0);

  UseProgram(mShaderColor.Program()); // default active program
  { LOG(LogInfo) << "[RendererGLES3] GLSL ES 3.00 shader programs initialised successfully."; }
  return true;
}

bool RendererGLES3::DoInitContext()
{
  ResetStateCache(); // fresh GL context: every cached binding is stale
  // Forget the old names WITHOUT deleting them: they belong to a context that no
  // longer exists, so glDeleteProgram on them would address a dead object.
  mShaderColor.Reset(false);
  mShaderTexture.Reset(false);
  mShaderAlpha.Reset(false);
  return InitShaders();
}

// ---------------------------------------------------------------------------
// DoFinalizeContext
// ---------------------------------------------------------------------------

void RendererGLES3::DoFinalizeContext() noexcept
{
  // Still inside a live context here, so the programs are really deleted. That
  // also leaves the Shader members empty, which makes their destructors no-ops
  // later on, whichever teardown path got us there.
  mShaderColor.Reset();
  mShaderTexture.Reset();
  mShaderAlpha.Reset();
}

// ---------------------------------------------------------------------------
// Projection
// ---------------------------------------------------------------------------

void RendererGLES3::DoBeginProjection(int w, int h)
{
  mProjMatrix = BuildOrtho(0.f, (float)w, (float)h, 0.f, -1.f, 1.f);
}

void RendererGLES3::DoApplyRotationToProjection(RotationType rotation, int viewW, int viewH)
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

void RendererGLES3::DoEndProjection(float sx, float sy)
{
  mProjMatrix.scale(Vector3f(sx, sy, 1.f));
  InvalidateMatrices();
}

// ---------------------------------------------------------------------------
// DoSetMatrix
// ---------------------------------------------------------------------------

void RendererGLES3::DoSetMatrix(const Transform4x4f& transform)
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

void RendererGLES3::DoDrawColorRect(int x, int y, int w, int h,
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
  glEnableVertexAttribArray(0); // a_pos
  glEnableVertexAttribArray(1); // a_col
  glVertexAttribPointer(0, 2, GL_SHORT,         GL_FALSE, 0, points);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  0, colors);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  DoDisableBlend();
}

void RendererGLES3::DoDrawColorRect4(int x, int y, int w, int h,
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
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 2, GL_SHORT,         GL_FALSE, 0, points);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  0, colors);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  DoDisableBlend();
}

// ---------------------------------------------------------------------------
// Fan primitives
// ---------------------------------------------------------------------------

void RendererGLES3::DoDrawColorTriangles(const Vector2f* verts, const ColorByte* colors, int count)
{
  DoEnableBlend(BlendMode::AlphaBlend);
  UseProgram(mShaderColor.Program());
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 2, GL_FLOAT,         GL_FALSE, 0, verts);
  // Already in GPU byte order, so normalised bytes go straight to the shader.
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  0, colors);
  glDrawArrays(GL_TRIANGLES, 0, count);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
}

void RendererGLES3::DoDrawColorFan(const float* verts, int count, unsigned int color)
{
  DoEnableBlend(BlendMode::AlphaBlend);
  UseProgram(mShaderColor.Program());
  glVertexAttrib4f(1 /* a_col */,
                   (float)(GLubyte)(color >> 24) / 255.f,
                   (float)(GLubyte)(color >> 16) / 255.f,
                   (float)(GLubyte)(color >>  8) / 255.f,
                   (float)(GLubyte)(color >>  0) / 255.f);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
  glDrawArrays(GL_TRIANGLE_FAN, 0, count);
  glDisableVertexAttribArray(0);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
  DoDisableBlend();
}

void RendererGLES3::DoWriteStencilFan(const float* verts, int count)
{
  glEnable(GL_STENCIL_TEST);
  glClear(GL_STENCIL_BUFFER_BIT);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDepthMask(GL_FALSE);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

  UseProgram(mShaderColor.Program());
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
  glDrawArrays(GL_TRIANGLE_FAN, 0, count);
  glDisableVertexAttribArray(0);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
  glStencilFunc(GL_EQUAL, 1, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

// ---------------------------------------------------------------------------
// Lines
// ---------------------------------------------------------------------------

void RendererGLES3::DoDrawLines(const Vector2f* coords, const Colors::ColorARGB* colors, int count)
{
  DoEnableBlend(BlendMode::AlphaBlend);
  UseProgram(mShaderColor.Program());
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 2, GL_FLOAT,         GL_FALSE, 0, coords);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  0, colors);
  glDrawArrays(GL_LINES, 0, count);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  DoDisableBlend();
}

void RendererGLES3::DoDrawLinesUniform(const Vector2f* coords, Colors::ColorARGB color, int count)
{
  DoEnableBlend(BlendMode::AlphaBlend);
  UseProgram(mShaderColor.Program());
  glVertexAttrib4f(1,
                   (float)(GLubyte)(color >> 24) / 255.f,
                   (float)(GLubyte)(color >> 16) / 255.f,
                   (float)(GLubyte)(color >>  8) / 255.f,
                   (float)(GLubyte)(color >>  0) / 255.f);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, coords);
  glDrawArrays(GL_LINES, 0, count);
  glDisableVertexAttribArray(0);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
  DoDisableBlend();
}

// ---------------------------------------------------------------------------
// Textured triangles
// ---------------------------------------------------------------------------

void RendererGLES3::DoDrawTTWithColors(const Vertex* verts, const ColorByte* colors, int count, TextureType type)
{
  UseProgram(type == TextureType::Alpha ? mShaderAlpha.Program() : mShaderTexture.Program());
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0, 2, GL_FLOAT,         GL_FALSE, (GLsizei)sizeof(Vertex), &verts[0].Target);
  glVertexAttribPointer(2, 2, GL_FLOAT,         GL_FALSE, (GLsizei)sizeof(Vertex), &verts[0].Source);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  0,                       colors);
  glDrawArrays(GL_TRIANGLES, 0, count);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  // No "restore the colour program" here any more. Every draw selects the
  // program it needs, so switching back was a guaranteed extra glUseProgram —
  // and two per draw instead of zero across a run of text, which is the most
  // common case in this UI.
}

void RendererGLES3::DoDrawTTUniform(const Vertex* verts, Colors::ColorARGB color, int count, TextureType type)
{
  UseProgram(type == TextureType::Alpha ? mShaderAlpha.Program() : mShaderTexture.Program());
  glVertexAttrib4f(1,
                   (float)(GLubyte)(color >> 24) / 255.f,
                   (float)(GLubyte)(color >> 16) / 255.f,
                   (float)(GLubyte)(color >>  8) / 255.f,
                   (float)(GLubyte)(color >>  0) / 255.f);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(Vertex), &verts[0].Target);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(Vertex), &verts[0].Source);
  glDrawArrays(GL_TRIANGLES, 0, count);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(2);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
}

// ---------------------------------------------------------------------------
// Texture quads
// ---------------------------------------------------------------------------

void RendererGLES3::DoSubmitTexQuadUniform(const Vertex* verts, int count,
                                            float r, float g, float b, float a,
                                            TextureType type)
{
  UseProgram(type == TextureType::Alpha ? mShaderAlpha.Program() : mShaderTexture.Program());
  glVertexAttrib4f(1, r, g, b, a);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(Vertex), &verts[0].Target);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(Vertex), &verts[0].Source);
  glDrawArrays(GL_TRIANGLES, 0, count);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(2);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
}

void RendererGLES3::DoSubmitTexQuadColors(const Vertex* verts, const Colors::ColorRGBA* colors,
                                           int count, TextureType type)
{
  UseProgram(type == TextureType::Alpha ? mShaderAlpha.Program() : mShaderTexture.Program());
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0, 2, GL_FLOAT,         GL_FALSE, (GLsizei)sizeof(Vertex), &verts[0].Target);
  glVertexAttribPointer(2, 2, GL_FLOAT,         GL_FALSE, (GLsizei)sizeof(Vertex), &verts[0].Source);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  0,                       colors);
  glDrawArrays(GL_TRIANGLES, 0, count);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
}

void RendererGLES3::DoSubmitTexQuadColorsTwoDraws(const Vertex* v1, const Colors::ColorRGBA* c1,
                                                   const Vertex* v2, const Colors::ColorRGBA* c2,
                                                   int count, TextureType type)
{
  UseProgram(type == TextureType::Alpha ? mShaderAlpha.Program() : mShaderTexture.Program());
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);

  // First draw
  glVertexAttribPointer(0, 2, GL_FLOAT,         GL_FALSE, (GLsizei)sizeof(Vertex), &v1[0].Target);
  glVertexAttribPointer(2, 2, GL_FLOAT,         GL_FALSE, (GLsizei)sizeof(Vertex), &v1[0].Source);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  0,                       c1);
  glDrawArrays(GL_TRIANGLES, 0, count);

  // Second draw (reflection)
  glVertexAttribPointer(0, 2, GL_FLOAT,         GL_FALSE, (GLsizei)sizeof(Vertex), &v2[0].Target);
  glVertexAttribPointer(2, 2, GL_FLOAT,         GL_FALSE, (GLsizei)sizeof(Vertex), &v2[0].Source);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  0,                       c2);
  glDrawArrays(GL_TRIANGLES, 0, count);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
}

#endif // USE_OPENGL_ES31
