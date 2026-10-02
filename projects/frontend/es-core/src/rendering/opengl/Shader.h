#pragma once

#ifdef USE_OPENGL

// Was a hand-rolled #ifdef pair that ignored USE_OPENGL_ES31 entirely, so this
// header did not compile in a GLES 3.1 build. GLHeaders.h covers all three.
#include "GLHeaders.h"

#include <utils/cplusplus/INoCopy.h>
#include <utils/cplusplus/Bitflags.h>

/*!
 * @brief Compiles a vertex + fragment shader pair and links them into a GL program.
 *        Ownership of the GL program is tied to the object lifetime.
 *
 * Held as a member by each GL backend, which is why it is default-constructible
 * and rebuildable: the GL context is destroyed and recreated every time a game
 * is launched and left, and the programs have to be rebuilt with it.
 *
 * Usage:
 *   Shader shader;
 *   if (!shader.Build(vertexSource, fragmentSource)) { ... handle error ... }
 *   glUseProgram(shader.Program());
 */
class Shader : public INoCopy
{
  public:
    /*!
     * @brief GL profile compatibility flags returned by Analyse().
     *        Each value is a single bit; results may be combined with | .
     */
    enum class Compatibility : unsigned int
    {
      None    = 0,
      GLES2   = 1 << 0,  //!< Compatible with OpenGL ES 2.0 / GLSL ES 1.00
      GLES3   = 1 << 1,  //!< Compatible with OpenGL ES 3.0 / GLSL ES 3.00
      GLCore3 = 1 << 2,  //!< Compatible with OpenGL 3.0 Core Profile / GLSL 1.30
    };

    /*!
     * @brief One attribute whose location must be fixed before linking.
     *
     * GLSL ES 1.00 has no layout qualifier, so the GLES 2 backend has to name
     * its attribute slots explicitly, and glBindAttribLocation only takes effect
     * if it runs between attaching the stages and linking them. That window is
     * inside Build(), which is why the bindings are passed in rather than
     * applied by the caller afterwards.
     */
    struct AttributeBinding
    {
      const char*  Name;      //!< Attribute name as written in the shader source
      unsigned int Location;  //!< Slot to bind it to
    };

    /*!
     * @brief Build an empty, invalid shader. Call Build() to fill it in.
     */
    Shader() = default;

    /*!
     * @brief Compile and link a GL program from GLSL source strings.
     *        On failure, Valid() returns false and Program() returns 0.
     * @param vertexSource   GLSL source of the vertex stage
     * @param fragmentSource GLSL source of the fragment stage
     */
    Shader(const char* vertexSource, const char* fragmentSource);

    /*!
     * @brief Destructor — deletes the GL program if it was successfully created.
     */
    ~Shader();

    /*!
     * @brief Compile, bind attribute locations, and link. Any program held
     *        beforehand is deleted first, so this doubles as "rebuild".
     * @param vertexSource   GLSL source of the vertex stage
     * @param fragmentSource GLSL source of the fragment stage
     * @param bindings       Attribute locations to force, or nullptr for none
     * @param bindingCount   Number of entries in @p bindings
     * @return true when the program is linked and usable
     */
    bool Build(const char* vertexSource, const char* fragmentSource,
               const AttributeBinding* bindings = nullptr, int bindingCount = 0);

    /*!
     * @brief Delete the program and return to the invalid state.
     *
     * Safe to call after the GL context is gone: the name is simply dropped.
     * @param contextAlive false when the context has already been destroyed, so
     *        glDeleteProgram must not be called on a dead name
     */
    void Reset(bool contextAlive = true);

    /*!
     * @brief Returns true if compilation and linking both succeeded.
     */
    [[nodiscard]] bool Valid() const { return mProgram != 0; }

    /*!
     * @brief Returns the GL program identifier, or 0 if not valid.
     */
    [[nodiscard]] GLuint Program() const { return mProgram; }

    /*!
     * @brief Perform a best-effort static analysis of GLSL sources to determine
     *        which GL profiles they are compatible with.
     *        The analysis detects syntactic markers (keywords, version directives,
     *        built-in names) without running a full GLSL parser.
     * @param vertexSource   GLSL source of the vertex stage
     * @param fragmentSource GLSL source of the fragment stage
     * @return Bitfield of Compatibility values — each set bit indicates a profile
     *         for which no disqualifying construct was found in either source.
     */
    [[nodiscard]] static Compatibility Analyse(const char* vertexSource, const char* fragmentSource);

  private:
    //! GL program identifier — 0 means invalid / compilation or link failure
    GLuint mProgram = 0;

    /*!
     * @brief Compile a single shader stage.
     * @param stageType GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
     * @param source    Null-terminated GLSL source string
     * @return GL shader identifier, or 0 on compilation failure
     */
    static GLuint CompileStage(GLenum stageType, const char* source);
};

DEFINE_BITFLAG_ENUM(Shader::Compatibility, unsigned int)
#endif // USE_OPENGL
