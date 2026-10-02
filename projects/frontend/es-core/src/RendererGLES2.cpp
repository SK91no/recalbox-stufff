#ifdef USE_OPENGL_ES

#include "RendererGLES2.h"
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

RendererGLES2::RendererGLES2(int width, int height, bool windowed, RotationType rotation)
  : RendererGLCommon(width, height, windowed, rotation)
{
  Initialize(width, height);
}

// ---------------------------------------------------------------------------
// DoConfigureGLProfile
// ---------------------------------------------------------------------------

void RendererGLES2::DoConfigureGLProfile()
{
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
}

// ---------------------------------------------------------------------------
// DoInitContext / InitShaders
// ---------------------------------------------------------------------------

bool RendererGLES2::InitShaders()
{
  // ---- GLSL ES 1.00 shader sources ----

  const char* vertColorSrc =
    "attribute vec2 a_pos;\n"
    "attribute vec4 a_col;\n"
    "uniform mat4 u_proj;\n"
    "uniform mat4 u_mv;\n"
    "varying vec4 v_col;\n"
    "void main() {\n"
    "  gl_Position = u_proj * u_mv * vec4(a_pos, 0.0, 1.0);\n"
    "  v_col = a_col;\n"
    "}\n";

  const char* vertTexSrc =
    "attribute vec2 a_pos;\n"
    "attribute vec2 a_tex;\n"
    "attribute vec4 a_col;\n"
    "uniform mat4 u_proj;\n"
    "uniform mat4 u_mv;\n"
    "varying vec2 v_tex;\n"
    "varying vec4 v_col;\n"
    "void main() {\n"
    "  gl_Position = u_proj * u_mv * vec4(a_pos, 0.0, 1.0);\n"
    "  v_tex = a_tex;\n"
    "  v_col = a_col;\n"
    "}\n";

  const char* fragColorSrc =
    "precision mediump float;\n"
    "varying vec4 v_col;\n"
    "void main() {\n"
    "  gl_FragColor = v_col;\n"
    "}\n";

  const char* fragTexSrc =
    "precision mediump float;\n"
    "varying vec2 v_tex;\n"
    "varying vec4 v_col;\n"
    "uniform sampler2D u_sampler;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(u_sampler, v_tex) * v_col;\n"
    "}\n";

  const char* fragAlphaSrc =
    "precision mediump float;\n"
    "varying vec2 v_tex;\n"
    "varying vec4 v_col;\n"
    "uniform sampler2D u_sampler;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(v_col.rgb, texture2D(u_sampler, v_tex).a * v_col.a);\n"
    "}\n";

  // GLSL ES 1.00 has no layout qualifier, so the slots are named here and Shader
  // applies them between attaching the stages and linking, which is the only
  // window where glBindAttribLocation has any effect. Every program gets the
  // same table, so a vertex buffer set up for one works with all three.
  static const Shader::AttributeBinding sBindings[] =
  {
    { "a_pos", 0 },
    { "a_col", 1 },
    { "a_tex", 2 },
  };
  constexpr int sBindingCount = (int)(sizeof(sBindings) / sizeof(sBindings[0]));

  if (!mShaderColor.Build(vertColorSrc, fragColorSrc, sBindings, sBindingCount)) return false;
  if (!mShaderTexture.Build(vertTexSrc, fragTexSrc, sBindings, sBindingCount)) return false;
  if (!mShaderAlpha.Build(vertTexSrc, fragAlphaSrc, sBindings, sBindingCount)) return false;

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
  { LOG(LogInfo) << "[RendererGLES2] Shader programs initialised successfully."; }
  return true;
}

bool RendererGLES2::DoInitContext()
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

void RendererGLES2::DoFinalizeContext() noexcept
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

void RendererGLES2::DoBeginProjection(int w, int h)
{
  mProjMatrix = BuildOrtho(0.f, (float)w, (float)h, 0.f, -1.f, 1.f);
}

void RendererGLES2::DoApplyRotationToProjection(RotationType rotation, int viewW, int viewH)
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

void RendererGLES2::DoEndProjection(float sx, float sy)
{
  mProjMatrix.scale(Vector3f(sx, sy, 1.f));
  InvalidateMatrices();
}

// ---------------------------------------------------------------------------
// DoSetMatrix
// ---------------------------------------------------------------------------

void RendererGLES2::DoSetMatrix(const Transform4x4f& transform)
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

void RendererGLES2::DoDrawColorRect(int x, int y, int w, int h,
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
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 2, GL_SHORT,         GL_FALSE, 0, points);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  0, colors);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  DoDisableBlend();
}

void RendererGLES2::DoDrawColorRect4(int x, int y, int w, int h,
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

void RendererGLES2::DoDrawColorTriangles(const Vector2f* verts, const ColorByte* colors, int count)
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

void RendererGLES2::DoDrawColorFan(const float* verts, int count, unsigned int color)
{
  DoEnableBlend(BlendMode::AlphaBlend);
  UseProgram(mShaderColor.Program());
  glVertexAttrib4f(1,
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

void RendererGLES2::DoWriteStencilFan(const float* verts, int count)
{
  glEnable(GL_STENCIL_TEST);
  glClear(GL_STENCIL_BUFFER_BIT);

  // Write 1 into stencil where the shape is drawn; suppress all colour output
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDepthMask(GL_FALSE);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

  UseProgram(mShaderColor.Program());
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
  glDrawArrays(GL_TRIANGLE_FAN, 0, count);
  glDisableVertexAttribArray(0);

  // Restore colour/depth writes and arm stencil test for subsequent draws
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
  glStencilFunc(GL_EQUAL, 1, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  // GL_STENCIL_TEST stays enabled until ClearStencil()
}

// ---------------------------------------------------------------------------
// Lines
// ---------------------------------------------------------------------------

void RendererGLES2::DoDrawLines(const Vector2f* coords, const Colors::ColorARGB* colors, int count)
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

void RendererGLES2::DoDrawLinesUniform(const Vector2f* coords, Colors::ColorARGB color, int count)
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

void RendererGLES2::DoDrawTTWithColors(const Vertex* verts, const ColorByte* colors, int count, TextureType type)
{
  UseProgram(type == TextureType::Alpha ? mShaderAlpha.Program() : mShaderTexture.Program());
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 2, GL_FLOAT,         GL_FALSE, (GLsizei)sizeof(Vertex), &verts[0].Target);
  glVertexAttribPointer(2, 2, GL_FLOAT,         GL_FALSE, (GLsizei)sizeof(Vertex), &verts[0].Source);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  0,                       colors);
  glDrawArrays(GL_TRIANGLES, 0, count);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(2);
  glDisableVertexAttribArray(1);
  // No "restore the colour program" here any more. Every draw selects the
  // program it needs, so switching back was a guaranteed extra glUseProgram —
  // and two per draw instead of zero across a run of text, which is the most
  // common case in this UI.
}

void RendererGLES2::DoDrawTTUniform(const Vertex* verts, Colors::ColorARGB color, int count, TextureType type)
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

void RendererGLES2::DoSubmitTexQuadUniform(const Vertex* verts, int count,
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

void RendererGLES2::DoSubmitTexQuadColors(const Vertex* verts, const Colors::ColorRGBA* colors,
                                           int count, TextureType type)
{
  UseProgram(type == TextureType::Alpha ? mShaderAlpha.Program() : mShaderTexture.Program());
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 2, GL_FLOAT,         GL_FALSE, (GLsizei)sizeof(Vertex), &verts[0].Target);
  glVertexAttribPointer(2, 2, GL_FLOAT,         GL_FALSE, (GLsizei)sizeof(Vertex), &verts[0].Source);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  0,                       colors);
  glDrawArrays(GL_TRIANGLES, 0, count);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(2);
  glDisableVertexAttribArray(1);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
}

void RendererGLES2::DoSubmitTexQuadColorsTwoDraws(const Vertex* v1, const Colors::ColorRGBA* c1,
                                                   const Vertex* v2, const Colors::ColorRGBA* c2,
                                                   int count, TextureType type)
{
  UseProgram(type == TextureType::Alpha ? mShaderAlpha.Program() : mShaderTexture.Program());
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(1);

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
  glDisableVertexAttribArray(2);
  glDisableVertexAttribArray(1);
  glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);
}

#endif // USE_OPENGL_ES
