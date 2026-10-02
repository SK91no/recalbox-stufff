#ifdef USE_OPENGL

#include "Shader.h"
#include <utils/Log.h>
#include <cstring>

namespace
{
  /*!
   * @brief Read the first #version directive of a GLSL source.
   * @param source Source to scan, may be null
   * @param number Receives the version number, e.g. 100, 300, 330
   * @param isES   Receives true when the directive carries the "es" profile
   * @return true when a directive with a number was found
   */
  bool ParseVersionDirective(const char* source, int& number, bool& isES)
  {
    if (source == nullptr) return false;
    const char* cursor = strstr(source, "#version");
    if (cursor == nullptr) return false;
    cursor += 8; // past "#version"

    while (*cursor == ' ' || *cursor == '\t') ++cursor;
    if (*cursor < '0' || *cursor > '9') return false;

    number = 0;
    while (*cursor >= '0' && *cursor <= '9') number = number * 10 + (*cursor++ - '0');

    // The profile, when present, follows the number: "es", "core" or "compatibility"
    while (*cursor == ' ' || *cursor == '\t') ++cursor;
    isES = (cursor[0] == 'e' && cursor[1] == 's');
    return true;
  }
}

GLuint Shader::CompileStage(GLenum stageType, const char* source)
{
  GLuint shader = glCreateShader(stageType);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  GLint compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled)
  {
    char errorLog[512];
    glGetShaderInfoLog(shader, 512, nullptr, errorLog);
    { LOG(LogError) << "[Shader] Compilation error in "
                    << (stageType == GL_VERTEX_SHADER ? "vertex" : "fragment")
                    << " stage: " << errorLog; }
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

Shader::Shader(const char* vertexSource, const char* fragmentSource)
{
  Build(vertexSource, fragmentSource);
}

bool Shader::Build(const char* vertexSource, const char* fragmentSource,
                   const AttributeBinding* bindings, int bindingCount)
{
  Reset();

  GLuint vertexShader   = CompileStage(GL_VERTEX_SHADER,   vertexSource);
  GLuint fragmentShader = CompileStage(GL_FRAGMENT_SHADER, fragmentSource);

  if (vertexShader == 0 || fragmentShader == 0)
  {
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return false;
  }

  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);

  // Between attach and link: glBindAttribLocation is only honoured here, and it
  // is what lets GLSL ES 1.00 sources, which have no layout qualifier, still
  // land on the slots the renderer expects.
  for (int i = 0; i < bindingCount; ++i)
    if (bindings != nullptr && bindings[i].Name != nullptr)
      glBindAttribLocation(program, bindings[i].Location, bindings[i].Name);

  glLinkProgram(program);
  // Safe now: the program keeps them alive until it is itself deleted.
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  GLint linked = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (!linked)
  {
    char errorLog[512];
    glGetProgramInfoLog(program, 512, nullptr, errorLog);
    { LOG(LogError) << "[Shader] Link error: " << errorLog; }
    glDeleteProgram(program);
    return false;
  }

  mProgram = program;
  return true;
}

void Shader::Reset(bool contextAlive)
{
  if (mProgram == 0) return;
  // After the context is gone the name means nothing and deleting it would be a
  // call into a dead context; just forget it.
  if (contextAlive) glDeleteProgram(mProgram);
  mProgram = 0;
}

Shader::~Shader()
{
  Reset();
}

Shader::Compatibility Shader::Analyse(const char* vertexSource, const char* fragmentSource)
{
  // Returns true if the given pattern appears anywhere in source (nullptr-safe)
  auto inSource = [](const char* source, const char* pattern) -> bool
  {
    return source != nullptr && strstr(source, pattern) != nullptr;
  };

  // Returns true if the pattern appears in either source
  auto inEither = [&](const char* pattern) -> bool
  {
    return inSource(vertexSource, pattern) || inSource(fragmentSource, pattern);
  };

  // --- Version directive ---
  //
  // Matching whole strings like "#version 130" only recognised three spellings,
  // and none of the ones this renderer actually uses: its sources are
  // "#version 300 es" and "#version 330 core", plus GLES 2 shaders with no
  // directive at all. Reading the number out of the directive covers every
  // spelling instead of enumerating a few.
  int  version      = 0;
  bool isES         = false;
  bool hasDirective = ParseVersionDirective(vertexSource, version, isES) ||
                      ParseVersionDirective(fragmentSource, version, isES);

  // --- GLSL ES 1.00 spellings, absent from ES 3.00 and desktop 1.30+ ---
  const bool hasES1Constructs = inEither("attribute ")
                             || inEither("varying ")
                             || inSource(fragmentSource, "gl_FragColor")
                             || inEither("texture2D(")
                             || inEither("textureCube(");

  // --- Spellings that only exist from ES 3.00 / desktop 1.30 onwards ---
  const bool hasModernConstructs = inEither("layout(")
                                || inEither("in vec")
                                || inEither("out vec")
                                || inEither("texture(");

  Compatibility result = Compatibility::None;

  // GLES 2 wants GLSL ES 1.00: no directive at all, or an ES one up to 100.
  if ((!hasDirective || (isES && version <= 100)) && !hasModernConstructs)
    result |= Compatibility::GLES2;

  // GLES 3 wants an ES directive of 300 or later.
  if (hasDirective && isES && version >= 300 && !hasES1Constructs)
    result |= Compatibility::GLES3;

  // Desktop core wants a non-ES directive of 130 or later.
  //
  // The precision qualifier used to disqualify this profile, which was simply
  // wrong: desktop GLSL accepts precision statements from 1.20 onwards, exactly
  // so that sources can be shared with ES. It no longer takes part here.
  if (hasDirective && !isES && version >= 130 && !hasES1Constructs)
    result |= Compatibility::GLCore3;

  return result;
}
#endif // USE_OPENGL
