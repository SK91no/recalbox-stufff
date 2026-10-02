#pragma once

// Backend-neutral types used all over the public API below.
// No OpenGL header is included here, and none must be: this interface is meant
// to be implementable by a non-GL backend. Whoever still issues raw GL calls
// includes rendering/opengl/GLHeaders.h explicitly. See VULKAN.md.
#include "rendering/RenderingTypes.h"

#include <SDL.h>
#include <utils/storage/HashMap.h>
#include <utils/storage/Array.h>
#include <utils/math/Vector2f.h>
#include <utils/math/Vector4i.h>
#include <utils/math/Vector2i.h>
#include <utils/math/Transform4x4f.h>
#include <utils/cplusplus/StaticLifeCycleControler.h>
#include "rendering/opengl/Rectangle.h"
#include "rendering/opengl/Colors.h"
#include "rendering/RenderingError.h"
#include "rendering/opengl/Vertex.h"
#include <utils/storage/Stack.h>
#include <hardware/RotationType.h>
#include <utils/String.h>
#include <mutex>

// Forward declaration
class Component;
class Font;

/*!
 * @brief Corner radii of a rounded rectangle, in pixels, clockwise from top-left.
 *
 * Radii that would overlap on a shared edge are scaled down together rather than
 * clamped one by one, so the shape keeps its proportions instead of turning into
 * whatever the first clamp happened to allow.
 */
struct RoundedCorners
{
  float TopLeft     = 0.f;
  float TopRight    = 0.f;
  float BottomRight = 0.f;
  float BottomLeft  = 0.f;

  RoundedCorners() = default;
  //! @brief Same radius on all four corners
  explicit RoundedCorners(float all) : TopLeft(all), TopRight(all), BottomRight(all), BottomLeft(all) {}
  //! @brief One radius per corner, clockwise from top-left
  RoundedCorners(float topLeft, float topRight, float bottomRight, float bottomLeft)
    : TopLeft(topLeft), TopRight(topRight), BottomRight(bottomRight), BottomLeft(bottomLeft) {}

  //! @return true when every corner is square, so the arc code can be skipped
  [[nodiscard]] bool AllSquare() const
  { return TopLeft < 1.f && TopRight < 1.f && BottomRight < 1.f && BottomLeft < 1.f; }
};

/*!
 * @brief One colour per corner, clockwise from top-left.
 *
 * Colours are interpolated bilinearly across the shape from the four corners of
 * its bounding box, which is what makes a gradient continuous along a rounded
 * outline instead of breaking at each corner.
 */
struct CornerColors
{
  Colors::ColorRGBA TopLeft     = 0;
  Colors::ColorRGBA TopRight    = 0;
  Colors::ColorRGBA BottomRight = 0;
  Colors::ColorRGBA BottomLeft  = 0;

  CornerColors() = default;
  //! @brief Same colour on all four corners
  explicit CornerColors(Colors::ColorRGBA all)
    : TopLeft(all), TopRight(all), BottomRight(all), BottomLeft(all) {}
  //! @brief One colour per corner, clockwise from top-left
  CornerColors(Colors::ColorRGBA topLeft, Colors::ColorRGBA topRight,
               Colors::ColorRGBA bottomRight, Colors::ColorRGBA bottomLeft)
    : TopLeft(topLeft), TopRight(topRight), BottomRight(bottomRight), BottomLeft(bottomLeft) {}

  //! @return true when all four corners are fully transparent, so nothing to draw
  [[nodiscard]] bool Invisible() const
  { return ((TopLeft | TopRight | BottomRight | BottomLeft) & 0xFF) == 0; }
};

class Renderer : public StaticLifeCycleControler<Renderer>
{
  public:
    enum class TextureType { RGBA, Alpha };

    //! Driver identification strings the backend can report
    enum class DriverInfo { Vendor, Renderer, Version, Extensions };

  private:
    /*
    //! Convenient type for array of vertex points
    typedef Array<Vertex::Point> ArrayOfPoint;

    //! Round corner cache structure
    struct FourRoundedCorner
    {
      ArrayOfPoint mTopLeft;
      ArrayOfPoint mTopRight;
      ArrayOfPoint mBottomRight;
      ArrayOfPoint mBottomLeft;
      [[nodiscard]] int Count() const { return mTopLeft.Count() + mTopRight.Count() + mBottomRight.Count() + mBottomLeft.Count(); }
      static void Copy(const ArrayOfPoint& source, ArrayOfPoint& destination, float xOffset, float yOffset)
      {
        int start = destination.Count();
        destination.CopyFrom(source);
        Vertex::Point* p = destination.BufferReadWrite() + start;
        for(int i = source.Count(); --i >= 0; ++p) { p->X += xOffset; p->Y += yOffset; }
      }
    };

    //! Round corner cache
    static HashMap<float, FourRoundedCorner> mCornerCache;
    */

    //! GL Context thread handle
    static pthread_t sGLContextThread;

    //! New clipping rectangle stack
    Stack<Rectangle> mClippings;
    //! Current clipping rectangle
    Rectangle mCurrentClipping;

    //! SDL Surface
    SDL_Window*   mSdlWindow;

    //! Viewport size as ints
    Vector2i   mViewportSize;
    //! Virtual viewport size as floats
    Vector2f   mVirtualViewportSizeFloat;
    //! Virtual viewport size as ints
    Vector2i   mVirtualViewportSize;

    //! Horizontal and vertical scaling
    Vector2f   mScale;
    //! Viewport rotation
    RotationType mRotate;

    //! True if both surface and context have been initialized
    bool mViewPortInitialized;
    //! Initial cursor state
    bool mInitialCursorState;
    //! Windowed mode
    bool mWindowed;

    //! Last X translation
    static float mLastXTranslation;
    //! Last Y translation
    static float mLastYTranslation;

    //! Offscreen colour texture attached to the offscreen target
    TextureHandle mOffscreenTexture;
    //! Power-of-2 size of the offscreen texture
    Vector2i mOffscreenSize;

    /*!
     * @brief Returns the smallest power of two >= v.
     *        Uses the classic successive-shift fill technique.
     */
    static int NextPow2(int v)
    {
      --v;
      v |= v >>  1;
      v |= v >>  2;
      v |= v >>  4;
      v |= v >>  8;
      v |= v >> 16;
      return v + 1;
    }

    /*!
     * @brief Create the offscreen FBO + texture sized to the next power of two
     *        above the current viewport.  Safe to call repeatedly (destroys any
     *        existing resources first).
     */
    void CreateOffscreen();

    /*!
     * @brief Release offscreen FBO and texture.  No-op if not created.
     */
    void DestroyOffscreen();

    /*!
     * @brief Create SDL display surface
     * @return True if the surface has been created successfuly
     */
    bool CreateSdlSurface(int width, int height);

    /*!
     * @brief Destroy SDL displaysurface
     */
    void DestroySdlSurface();

    /*!
     * @brief Get resolution from configuration file
     * @param w Width to fill
     * @param h Height to fill
     */
    static void GetResolutionFromConfiguration(int& w, int& h);

    /*!
     * @brief Show useful information in logs to help debug remote issues
     */
    static void InformationLogs();

    /*!
     * @brief Rotating the GL viewport of required
     */
    void RotateViewport();

    /*!
     * @brief Draw a GL_TRIANGLE_FAN with a uniform color (delegates to DoDrawColorFan).
     *        Blend is enabled with standard src-alpha / one-minus-src-alpha.
     * @param verts  Interleaved (x,y) float pairs
     * @param count  Vertex count
     * @param color  ARGB color
     */
    static void DrawColorFan(const float* verts, int count, unsigned int color);

    /*!
     * @brief Write a GL_TRIANGLE_FAN shape into the stencil buffer (delegates to DoWriteStencilFan).
     * @param verts  Interleaved (x,y) float pairs
     * @param count  Vertex count
     */
    static void WriteStencilFan(const float* verts, int count);

  protected:
    // -----------------------------------------------------------------------
    // Pure virtual interface — implemented by RendererGLES2 / RendererGLES3 / RendererGLCore
    // -----------------------------------------------------------------------

    //! Called before SDL_CreateWindow — set whatever SDL attributes the backend needs.
    virtual void DoConfigureContext() = 0;

    //! Extra SDL window flags the backend requires (SDL_WINDOW_OPENGL, SDL_WINDOW_VULKAN...).
    [[nodiscard]] virtual unsigned int DoWindowFlags() const = 0;

    //! Called after SDL_CreateWindow — create the rendering context or device
    //! on that window, and set up presentation (vsync, swapchain...).
    //! @param window Freshly created SDL window
    //! @return true on success
    virtual bool DoCreateContext(SDL_Window* window) = 0;

    //! Called before the window goes away — release the context or device.
    //! Non-pure for the same reason as DoFinalizeContext below.
    virtual void DoDestroyContext() noexcept {}

    //! @return true when a rendering context currently exists
    [[nodiscard]] virtual bool DoHasContext() const = 0;

    //! Present the finished frame.
    //! @param window Window owning the presentation surface
    virtual void DoPresent(SDL_Window* window) = 0;

    //! Called after the context exists — compile shaders and build pipelines.
    //! @return true on success
    virtual bool DoInitContext() = 0;

    //! Called before context destruction — delete shader programs / VAOs / VBOs.
    //! Defaults to a no-op so that ~Renderer() can still call it safely after
    //! the derived vtable has been torn down (a pure-virtual call there would
    //! abort the process).  Derived classes are expected to override.
    virtual void DoFinalizeContext() noexcept {}

    //! Set up the orthographic projection matrix.
    //! Called inside Initialize(), before RotateViewport().
    virtual void DoBeginProjection(int w, int h) = 0;

    //! Apply a rotation/translation to the projection.
    //! Called inside RotateViewport() once the common mVirtualViewportSize/mScale
    //! have already been updated.
    //! @param rotation  Rotation type
    //! @param viewW     Physical viewport width  (mViewportSize.x())
    //! @param viewH     Physical viewport height (mViewportSize.y())
    virtual void DoApplyRotationToProjection(RotationType rotation, int viewW, int viewH) = 0;

    //! Finalize the projection: apply scale and switch to model-view mode.
    //! Called inside Initialize(), after RotateViewport().
    //! @param sx  Horizontal scale factor (mScale.x())
    //! @param sy  Vertical   scale factor (mScale.y())
    virtual void DoEndProjection(float sx, float sy) = 0;

    //! Update the model-view matrix.
    virtual void DoSetMatrix(const Transform4x4f& transform) = 0;

    // -----------------------------------------------------------------------
    // Texture objects — implemented once for GL by RendererGLCommon.
    // A Vulkan backend implements these against VkImage/VkSampler instead.
    // -----------------------------------------------------------------------

    //! Create a texture object and return its handle (InvalidTextureHandle on failure).
    virtual TextureHandle DoCreateTexture(bool nearest) = 0;

    //! Destroy a texture object.
    virtual void DoDestroyTexture(TextureHandle id) = 0;

    //! Bind a texture for subsequent draws. A null handle leaves the current binding alone.
    virtual void DoBindTexture(TextureHandle id) = 0;

    //! Set the wrap/tiling mode applied to the currently bound texture.
    virtual void DoSetTextureTiling(bool tiled) = 0;

    //! Allocate and fill a whole texture image. @p data may be nullptr to allocate only.
    virtual RenderingError DoUploadTexture(TextureHandle id, int width, int height,
                                           const void* data, TextureType type) = 0;

    //! Upload a sub-rectangle into an already allocated texture image.
    virtual RenderingError DoUploadTexturePart(TextureHandle id, int x, int y, int width, int height,
                                               const void* data, TextureType type) = 0;

    // -----------------------------------------------------------------------
    // Render state — implemented once for GL by RendererGLCommon.
    // -----------------------------------------------------------------------

    //! Enable blending with the given equation.
    virtual void DoEnableBlend(BlendMode blend) = 0;

    //! Disable blending.
    virtual void DoDisableBlend() = 0;

    //! Arm the scissor test with the given rectangle, in physical viewport coordinates.
    virtual void DoSetScissor(int x, int y, int w, int h) = 0;

    //! Disable the scissor test entirely.
    virtual void DoDisableScissor() = 0;

    //! Drop any active stencil mask so drawing is no longer clipped by it.
    virtual void DoClearStencil() = 0;

    //! Set the physical viewport.
    virtual void DoSetViewport(int x, int y, int w, int h) = 0;

    //! Set the color the framebuffer is cleared to, components in [0,1].
    virtual void DoSetClearColor(float r, float g, float b, float a) = 0;

    //! Clear the color and depth buffers.
    virtual void DoClearBuffers() = 0;

    //! Return one of the driver identification strings.
    virtual String DoGetDriverInfo(DriverInfo info) = 0;

    //! Hook the driver's debug output, if it has one. DEBUG builds only.
    //! Non-pure: a backend without a debug channel simply does nothing.
    virtual void DoActivateDebug() {}

    // -----------------------------------------------------------------------
    // Offscreen target
    // -----------------------------------------------------------------------

    //! Create an offscreen render target of the given size, filling @p texture.
    //! @return true on success
    virtual bool DoCreateOffscreen(int w, int h, TextureHandle& texture) = 0;

    //! Release the offscreen render target and reset @p texture.
    //! Non-pure on purpose: Finalize() reaches this from ~Renderer(), where the
    //! derived vtable is already gone and a pure-virtual call would abort. Same
    //! reasoning as DoFinalizeContext().
    virtual void DoDestroyOffscreen(TextureHandle& texture) noexcept { (void)texture; }

    //! Redirect rendering into the offscreen target, sized @p w x @p h.
    virtual void DoBeginOffScreen(int w, int h) = 0;

    //! Restore rendering to the default framebuffer, viewport sized @p w x @p h.
    virtual void DoEndOffScreen(int w, int h) = 0;

    //! Draw a colored rectangle (two triangles, uniform color).
    virtual void DoDrawColorRect(int x, int y, int w, int h,
                                 Colors::ColorARGB color,
                                 BlendMode blend) = 0;

    //! Draw a colored rectangle with per-corner colors.
    virtual void DoDrawColorRect4(int x, int y, int w, int h,
                                  Colors::ColorARGB tl, Colors::ColorARGB tr,
                                  Colors::ColorARGB br, Colors::ColorARGB bl) = 0;

    //! Draw a GL_TRIANGLE_FAN with a uniform color.
    virtual void DoDrawColorFan(const float* verts, int count, unsigned int color) = 0;

    /*!
     * @brief Draw independent, untextured triangles with one colour per vertex.
     *
     * The only colour primitive that is neither a plain rectangle nor a
     * single-colour shape. A rounded outline cannot use DoDrawColorRect4, which
     * only knows the four corners of a straight rectangle, and cannot use
     * DoDrawColorFan, which carries one colour for the whole shape.
     * @param verts  Vertex positions, three per triangle
     * @param colors One GPU-byte-ordered RGBA colour per vertex, four bytes each
     * @param count  Vertex count, a multiple of three
     */
    virtual void DoDrawColorTriangles(const Vector2f* verts, const ColorByte* colors, int count) = 0;

    //! Write a GL_TRIANGLE_FAN into the stencil buffer and arm the stencil test.
    virtual void DoWriteStencilFan(const float* verts, int count) = 0;

    //! Draw polylines with per-vertex colors.
    virtual void DoDrawLines(const Vector2f* coords, const Colors::ColorARGB* colors, int count) = 0;

    //! Draw polylines with a uniform color.
    virtual void DoDrawLinesUniform(const Vector2f* coords, Colors::ColorARGB color, int count) = 0;

    //! Submit textured triangles with a per-vertex color array.
    virtual void DoDrawTTWithColors(const Vertex* verts, const ColorByte* colors, int count, TextureType type) = 0;

    //! Submit textured triangles with a single uniform color.
    virtual void DoDrawTTUniform(const Vertex* verts, Colors::ColorARGB color, int count, TextureType type) = 0;

    //! Submit a textured quad with a single uniform RGBA color (components in [0,1]).
    virtual void DoSubmitTexQuadUniform(const Vertex* verts, int count,
                                        float r, float g, float b, float a,
                                        TextureType type) = 0;

    //! Submit a textured quad with a per-vertex color array (GPU byte ordered).
    virtual void DoSubmitTexQuadColors(const Vertex* verts, const Colors::ColorRGBA* colors,
                                       int count, TextureType type) = 0;

    //! Submit two textured quads back-to-back (used for the alpha-reflection effect).
    //! v1/c1 = first draw data; v2/c2 = second (reflected) draw data.
    virtual void DoSubmitTexQuadColorsTwoDraws(const Vertex* v1, const Colors::ColorRGBA* c1,
                                               const Vertex* v2, const Colors::ColorRGBA* c2,
                                               int count, TextureType type) = 0;

  public:
    static bool IsContextThread() { return sGLContextThread == pthread_self(); }
    bool HasGLContext() { return DoHasContext(); }

    /*!
     * @brief Return the GL extension string reported by the driver
     * @return Space-separated list of supported GL extensions
     */
    static String GetGLExtensions();

    //! @brief Return the GL vendor string
    static String GetGLVendor();
    //! @brief Return the GL renderer string
    static String GetGLRenderer();
    //! @brief Return the GL version string
    static String GetGLVersion();

    //! Store one color into a byte array, in the R,G,B,A order the GPU expects
    static void ColorToByteArray(ColorByte* array, Colors::ColorRGBA color) { Colors::StoreGPUColor(array, color); }

    //! Reorder a packed color into the R,G,B,A byte order the GPU expects
    static Colors::ColorRGBA ColorToGL(Colors::ColorRGBA color) { return Colors::ToGPUBytes(color); }

    /*!
     * @brief Constructor — stores parameters; does NOT initialise SDL/GL.
     *        Derived class constructors are responsible for calling Initialize().
     */
    Renderer(int width, int height, bool windowed, RotationType rotation = RotationType::None);

    /*!
     * @brief Destructor
     */
    virtual ~Renderer();

    /*!
     * @brief Initialize GL viewport
     * @param w Required Width (0 = display width)
     * @param h Required Height (0 = display height)
     * @return true if everything is working fine, false otherwise
     */
    bool Initialize(int w, int h);

    /*!
     * @brief Reinitialize video using previous parameters
     * @return true if everything is working fine, false otherwise
     */
    bool ReInitialize();

    /*!
     * @brief Reinitialize video using previous parameters
     * @return true if everything is working fine, false otherwise
     */
    bool Rotate(RotationType rotation);

    /*!
     * Finalize GL viewport
     */
    void Finalize();

    /*!
     * @brief Redirect subsequent rendering into the offscreen FBO.
     *        The viewport is set to the offscreen texture dimensions.
     *        Must be paired with EndOffScreen().
     */
    static void BeginOffScreen();

    /*!
     * @brief Restore rendering to the default framebuffer and the physical viewport.
     *        Must follow a BeginOffScreen() call.
     */
    static void EndOffScreen();

    //! @brief Return the offscreen colour texture (0 if not available)
    [[nodiscard]] TextureHandle OffscreenTexture() const { return mOffscreenTexture; }

    //! @brief Return the power-of-2 dimensions of the offscreen texture
    [[nodiscard]] const Vector2i& OffscreenSize() const { return mOffscreenSize; }

    /*!
     * @brief Applmy the given matrix to openGL context
     * @param transform Matrix
     */
    static void SetMatrix(const Transform4x4f& transform);

    /*!
     * @brief Swap working and dipslayed buffers in double buffering context
     */
    void SwapBuffers();

    /*
     * Clipping
     */

    /*!
     * @brief Initialize clipping stack & stored translations
     */
    void InitializeClipping();

    /*!
     * @brief Push a new clipping rectangle relative to the whole screen
     * @param area Rectangle in abslolute coordinates
     */
    void ClipAbsolute(const Rectangle& area);

    /*!
     * @brief Push a new clipping rectangle relative to the last matrix
     * @param area area to clip
     */
    void Clip(const Rectangle& area);

    /*!
     * @brief Pus a new clip rectangle relative to tranformation and of the given size
     * @param trans Transformation
     * @param size Cliping size
     */
    void Clip(const Transform4x4f& trans, const Vector2f& size);

    /*!
     * @brief Pop latest clipping rectangle
     */
    void Unclip();

    /*!
     * @brief Get current clipping
     * @return Current
     */
    [[nodiscard]] Rectangle CurrentClipping() const
    {
      switch(mRotate)
      {
        case RotationType::Left: return Rectangle(mCurrentClipping.Top() + mCurrentClipping.Height(), mCurrentClipping.Left(), mCurrentClipping.Height(), mCurrentClipping.Width()).Scale(1.f / mScale.x(), 1.f / mScale.y()).Move(-mLastXTranslation, -mLastYTranslation);
        case RotationType::Right: return Rectangle(mCurrentClipping.Top(), (float)mViewportSize.x() - mCurrentClipping.Right(), mCurrentClipping.Height(), mCurrentClipping.Width()).Scale(1.f / mScale.x(), 1.f / mScale.y()).Move(-mLastXTranslation, -mLastYTranslation);
        case RotationType::None:
        case RotationType::Upsidedown:
        default: break;
      }
      return Rectangle(mCurrentClipping).Scale(1.f / mScale.x(), 1.f / mScale.y()).Move(-mLastXTranslation, -mLastYTranslation);
    }

    /*
     * Drawing
     */

    /*!
     * @brief Draw rectangle
     * @param area Rectangle area
     * @param color Color
     * @param blend Blending equation
     */
    static void DrawRectangle(const Rectangle& area, Colors::ColorARGB color, BlendMode blend = BlendMode::AlphaBlend);

    /*!
     * @brief Draw rectangle
     * @param x Left coordinate
     * @param y Top coordinate
     * @param w Width
     * @param h Height
     * @param color Color
     * @param blend Blending equation
     */
    static void DrawRectangle(int x, int y, int w, int h, Colors::ColorARGB color, BlendMode blend = BlendMode::AlphaBlend);

    /*!
     * @brief Draw rectangle
     * @param x Left coordinate
     * @param y Top coordinate
     * @param w Width
     * @param h Height
     * @param color Color
     * @param blend Blending equation
     */
    static void DrawRectangle(float x, float y, float w, float h, Colors::ColorARGB color, BlendMode blend = BlendMode::AlphaBlend);

    /*!
     * @brief Draw rectangle
     * @param x Left coordinate
     * @param y Top coordinate
     * @param w Width
     * @param h Height
     * @param topleftcolor Top Left Color
     * @param toprightcolor Top Right Color
     * @param bottomleftcolor Bottom Left Color
     * @param bottomrightcolor Bottom Right Color
     */
    static void DrawRectangle(int x, int y, int w, int h,
                              Colors::ColorARGB topleftcolor,
                              Colors::ColorARGB toprightcolor,
                              Colors::ColorARGB bottomrightcolor,
                              Colors::ColorARGB bottomleftcolor);

    /*!
     * @brief Draw rectangle
     * @param x Left coordinate
     * @param y Top coordinate
     * @param w Width
     * @param h Height
     * @param topleftcolor Top Left Color
     * @param toprightcolor Top Right Color
     * @param bottomleftcolor Bottom Left Color
     * @param bottomrightcolor Bottom Right Color
     */
    static void DrawRectangle(float x, float y, float w, float h,
                            Colors::ColorARGB topleftcolor,
                            Colors::ColorARGB toprightcolor,
                            Colors::ColorARGB bottomrightcolor,
                            Colors::ColorARGB bottomleftcolor) { DrawRectangle((int)x, (int)y, (int)w, (int)h, topleftcolor, toprightcolor, bottomrightcolor, bottomleftcolor); }

    /*!
     * @brief Draw polylines
     * @param vertices Vertice list
     * @param colors Color array
     * @param count Vertex count
     */
    static void DrawLines(const Vector2f vertices[], const Colors::ColorARGB colors[], int count);

    /*!
     * @brief Draw polylines using a single uniform color
     * @param vertices Vertice list
     * @param color Uniform color applied to all vertices
     * @param count Vertex count
     */
    static void DrawLines(const Vector2f vertices[], Colors::ColorARGB color, int count);

    /*!
     * @brief Draw textured triangles
     * @param id GL texture id
     * @param vertices Vertice list
     * @param colors Color list
     * @param count Vertice count
     * @param tiled draw tiled texture
     */
    static void DrawTexturedTriangles(TextureHandle id, const Vertex vertices[], const ColorByte colors[], int count, bool tiled, TextureType type = TextureType::RGBA);

    /*!
     * @brief Draw textured triangles using a single color
     * @param id GL texture id
     * @param vertices Vertice list
     * @param color Color
     * @param count Vertice count
     * @param tiled draw tiled texture
     */
    static void DrawTexturedTriangles(TextureHandle id, const Vertex vertices[], Colors::ColorARGB color, int count, bool tiled, TextureType type = TextureType::RGBA);

    /*!
     * @brief Upload Alpha texture data to GPU memory
     * @param id GL Texture id
     * @param width Data width
     * @param height Data height
     * @param data Alpha bytes (1 byte = 1 pixel)
     * @return NoError or OutOfGPUMemory
     */
    static RenderingError UploadAlpha(TextureHandle id, int width, int height, const void* data);

    /*!
     * @brief Upload RGBA texture data to GPU memory
     * @param id GL Texture id
     * @param width Data width
     * @param height Data height
     * @param data RGBA bytes (4 byte = 1 pixel)
     * @return NoError or OutOfGPUMemory
     */
    static RenderingError UploadRGBA(TextureHandle id, int width, int height, const void* data);

    /*!
     * @brief Upload Alpha texture data to GPU memory
     * @param id GL Texture id
     * @param width Data width
     * @param height Data height
     * @param data Alpha bytes (1 byte = 1 pixel)
     * @return NoError or OutOfGPUMemory
     */
    static RenderingError UploadAlphaPart(TextureHandle id, int x, int y, int width, int height, const void* data);

    /*!
     * @brief Upload RGBA texture data to GPU memory
     * @param id GL Texture id
     * @param width Data width
     * @param height Data height
     * @param data RGBA bytes (4 byte = 1 pixel)
     * @return NoError or OutOfGPUMemory
     */
    static RenderingError UploadRGBAPart(TextureHandle id, int x, int y, int width, int height, const void* data);

    /*!
     * @brief Build a GL color array
     * @param array Array pointer
     * @param color Color
     * @param count Count x Color => array
     */
    static void BuildGLColorArray(ColorByte* array, Colors::ColorRGBA color, int count);

    /*!
     * @brief Create a new texture and return its GL Identifier
     * @return GL Texture identifier
     */
    static TextureHandle CreateGLTexture(bool nearest = false);

    /*!
     * @brief Destroy the texture associated to the given id
     * @param id GL Texture identifier
     */
    static void DestroyGLTexture(TextureHandle id);

    /*
     * High level GL with low-level parameters (for use in new textures)
     */

    /*!
     * @brief Draw a texture using full coordinates, width/height, keeping ratio yes/no
     * @param id GL Texture ID
     * @param textureW Texture width
     * @param textureH Texture width
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio, false to stretch
     */
    static void DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool keepratio, bool tiled, TextureType type = TextureType::RGBA);

    /*!
     * @brief Draw a texture using full coordinates & texture width/height
     * @param id GL Texture ID
     * @param textureW Texture width
     * @param textureH Texture width
     * @param x X coordinate
     * @param y Y coordinate
     */
    static void DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, bool tiled, TextureType type = TextureType::RGBA) { DrawTexture(id, textureW, textureH, x, y, textureW, textureH, false, tiled, type); }

    /*!
     * @brief Draw a texture using full coordinates & width/height
     * @param id GL Texture ID
     * @param textureW Texture width
     * @param textureH Texture width
     * @param x X coordinate
     * @param y Y coordinate
     */
    static void DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool tiled, TextureType type = TextureType::RGBA) { DrawTexture(id, textureW, textureH, x, y, w, h, false, tiled, type); }

    /*!
     * @brief Draw a texture using full coordinates, width/height, keeping ratio yes/no and a given blending alpha
     * @param id GL Texture ID
     * @param textureW Texture width
     * @param textureH Texture width
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio, false to stretch
     * @param alpha Blending alpha
     */
    static void DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool keepratio, bool tiled, unsigned char alpha, TextureType type = TextureType::RGBA);

    /*!
     * @brief Draw a texture using full coordinates, texture width/height and a given blending alpha
     * @param id GL Texture ID
     * @param textureW Texture width
     * @param textureH Texture width
     * @param x X coordinate
     * @param y Y coordinate
     * @param alpha Blending alpha
     */
    static void DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, bool tiled, unsigned char alpha, TextureType type = TextureType::RGBA) { DrawTexture(id, textureW, textureH, x, y, textureW, textureH, false, tiled, alpha, type); }

    /*!
     * @brief Draw a texture using full coordinates, width/height and a given blending alpha
     * @param id GL Texture ID
     * @param textureW Texture width
     * @param textureH Texture width
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param alpha Blending alpha
     */
    static void DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool tiled, unsigned char alpha, TextureType type = TextureType::RGBA) { DrawTexture(id, textureW, textureH, x, y, w, h, false, tiled, alpha, type); }

    /*!
     * @brief Draw a texture using full coordinates, width/height, keeping ratio yes/no and a given blending color
     * @param id GL Texture ID
     * @param textureW Texture width
     * @param textureH Texture width
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio, false to stretch
     * @param color Blending color
     */
    static void DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool keepratio, bool tiled, Colors::ColorARGB color, TextureType type = TextureType::RGBA);

    /*!
     * @brief Draw a texture using full coordinates, width/height, keeping ratio yes/no and a given blending color
     * @param id GL Texture ID
     * @param textureW Texture width
     * @param textureH Texture width
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio, false to stretch
     * @param color Blending color
     */
    static void DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool flipX, bool flipY, bool keepratio, bool tiled, Colors::ColorARGB color, TextureType type = TextureType::RGBA);

    /*!
     * @brief Draw a texture using full coordinates, texture width/height and a given blending color
     * @param id GL Texture ID
     * @param textureW Texture width
     * @param textureH Texture width
     * @param x X coordinate
     * @param y Y coordinate
     * @param color Blending color
     */
    static void DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, bool tiled, Colors::ColorARGB color, TextureType type = TextureType::RGBA) { DrawTexture(id, textureW, textureH, x, y, textureW, textureH, false, tiled, color, type); }

    /*!
     * @brief Draw a texture using full coordinates, width/height and a given blending color
     * @param id GL Texture ID
     * @param textureW Texture width
     * @param textureH Texture width
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param color Blending color
     */
    static void DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool tiled, Colors::ColorARGB color, TextureType type = TextureType::RGBA)  { DrawTexture(id, textureW, textureH, x, y, w, h, false, tiled, color, type); }

    /*!
     * @brief Draw a texture using full coordinates, width/height, ratio & and blending colors
     * @param id GL Texture ID
     * @param textureW Texture width
     * @param textureH Texture width
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio
     * @param topleftcolor Top left color blending
     * @param toprightcolor  Top right color blending
     * @param bottomrightcolor Bottom Right color blending
     * @param bottomleftcolor Bottom Left color blending
     */
    static void DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool keepratio, bool tiled,
                            Colors::ColorARGB topleftcolor,
                            Colors::ColorARGB toprightcolor,
                            Colors::ColorARGB bottomrightcolor,
                            Colors::ColorARGB bottomleftcolor,
                            TextureType type = TextureType::RGBA);

    /*!
     * @brief Draw a texture using full coordinates, width/height and blending colors
     * @param id GL Texture ID
     * @param textureW Texture width
     * @param textureH Texture width
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param topleftcolor Top left color blending
     * @param toprightcolor  Top right color blending
     * @param bottomrightcolor Bottom Right color blending
     * @param bottomleftcolor Bottom Left color blending
     */
    static void DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool tiled,
                            Colors::ColorARGB topleftcolor,
                            Colors::ColorARGB toprightcolor,
                            Colors::ColorARGB bottomrightcolor,
                            Colors::ColorARGB bottomleftcolor,
                            TextureType type = TextureType::RGBA) { DrawTexture(id, textureW, textureH, x, y, w, h, false, tiled, topleftcolor, toprightcolor, bottomrightcolor, bottomleftcolor, type); }

    /*!
     * @brief Draw a texture using full coordinates, width/height, ratio,  blending colors & lower reflection
     * @param id GL Texture ID
     * @param textureW Texture width
     * @param textureH Texture width
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio
     * @param flipOnX True to flip texture around vertical axis
     * @param flipOnY True to flip texture around horizontal axis
     * @param topleftcolor Top left color blending
     * @param toprightcolor  Top right color blending
     * @param bottomrightcolor Bottom Right color blending
     * @param bottomleftcolor Bottom Left color blending
     * @param topAlphaReflection Top alpha reflection
     * @param bottomAlphaReflection Bottom alpha reflection
     */
    static void DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool keepratio, bool tiled,
                            bool flipOnX, bool flipOnY,
                            Colors::ColorARGB topleftcolor,
                            Colors::ColorARGB toprightcolor,
                            Colors::ColorARGB bottomrightcolor,
                            Colors::ColorARGB bottomleftcolor,
                            float topAlphaReflection,
                            float bottomAlphaReflection,
                            TextureType type = TextureType::RGBA);

    /*!
     * @brief Draw a texture using full coordinates, width/height,  blending colors & lower reflection
     * @param id GL Texture ID
     * @param textureW Texture width
     * @param textureH Texture width
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param flipOnX True to flip texture around vertical axis
     * @param flipOnY True to flip texture around horizontal axis
     * @param topleftcolor Top left color blending
     * @param toprightcolor  Top right color blending
     * @param bottomrightcolor Bottom Right color blending
     * @param bottomleftcolor Bottom Left color blending
     * @param topAlphaReflection Top alpha reflection
     * @param bottomAlphaReflection Bottom alpha reflection
     */
    static void DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool tiled,
                            bool flipOnX, bool flipOnY,
                            Colors::ColorARGB topleftcolor,
                            Colors::ColorARGB toprightcolor,
                            Colors::ColorARGB bottomrightcolor,
                            Colors::ColorARGB bottomleftcolor,
                            float topAlphaReflection,
                            float bottomAlphaReflection,
                            TextureType type = TextureType::RGBA) { DrawTexture(id, textureW, textureH, x, y, w, h, false, tiled, flipOnX, flipOnY, topleftcolor, toprightcolor, bottomrightcolor, bottomleftcolor, topAlphaReflection, bottomAlphaReflection, type); }

    /*!
     * @brief Draw a rounding rectangle
     * @param x Left coordinate
     * @param y Top coordinate
     * @param width Rectangle width
     * @param height Rectangle height
     * @param radius Corner radius in pixel
     * @param color Fill color
     */
    static void DrawRoundRectangle(float x, float y, float width, float height, float radius, unsigned int color);

    /*!
     * @brief Draw a rounding rectangle
    * @param x Left coordinate
     * @param y Top coordinate
     * @param width Rectangle width
     * @param height Rectangle height
     * @param radius Corner radius in pixel
     * @param color Fill color
     */
    static void DrawRoundRectangle(int x, int y, int width, int height, float radius, unsigned int color) { DrawRoundRectangle((float)x, (float)y, (float)width, (float)height, radius, color); }

    /*!
     * @brief Draw a rounding rectangle
     * @param area Rectangle area
     * @param radius Corner radius in pixel
     * @param color Fill color
     */
    static void DrawRoundRectangle(const Rectangle& area, float radius, unsigned int color) { DrawRoundRectangle(area.Left(), area.Top(), area.Width(), area.Height(), radius, color); }

    /*
     * Rounded rectangles with a border
     *
     * Shared contract for the whole family below:
     *  - @p area is always the OUTER edge of the shape. A border never grows the
     *    footprint, so a component can lay itself out without knowing whether it
     *    will be drawn with a border or not.
     *  - @p thickness is in whole pixels and is drawn INWARD from that outer
     *    edge. Zero, or less, means no border at all.
     *  - The fill stops at the inner edge of the border rather than running
     *    underneath it, so a translucent border composites against the
     *    background once instead of twice.
     *  - Corner radii apply to the outer edge. The inner edge keeps concentric
     *    corners, its radii being the outer ones minus the thickness, down to a
     *    square corner when the border is thicker than the radius.
     */

    /*!
     * @brief Draw a rounded rectangle with a border and a fill, the full form:
     *        a radius and two colours per corner.
     * @param area      Outer area of the shape
     * @param radii     Corner radii, applied to the outer edge
     * @param thickness Border thickness in pixels, drawn inward, 0 for none
     * @param border    Border colours, one per corner
     * @param fill      Fill colours, one per corner
     */
    static void DrawRoundRectangle(const Rectangle& area, const RoundedCorners& radii, int thickness,
                                   const CornerColors& border, const CornerColors& fill);

    /*!
     * @brief Border and fill, a radius per corner, one colour each.
     * @param area      Outer area of the shape
     * @param radii     Corner radii, applied to the outer edge
     * @param thickness Border thickness in pixels, drawn inward, 0 for none
     * @param border    Border colour
     * @param fill      Fill colour
     */
    static void DrawRoundRectangle(const Rectangle& area, const RoundedCorners& radii, int thickness,
                                   Colors::ColorRGBA border, Colors::ColorRGBA fill)
    { DrawRoundRectangle(area, radii, thickness, CornerColors(border), CornerColors(fill)); }

    /*!
     * @brief Border and fill, one radius for all corners, one colour each.
     * @param area      Outer area of the shape
     * @param radius    Corner radius applied to all four corners
     * @param thickness Border thickness in pixels, drawn inward, 0 for none
     * @param border    Border colour
     * @param fill      Fill colour
     */
    static void DrawRoundRectangle(const Rectangle& area, float radius, int thickness,
                                   Colors::ColorRGBA border, Colors::ColorRGBA fill)
    { DrawRoundRectangle(area, RoundedCorners(radius), thickness, CornerColors(border), CornerColors(fill)); }

    /*!
     * @brief Fill only, a radius per corner, a colour per corner.
     * @param area  Outer area of the shape
     * @param radii Corner radii
     * @param fill  Fill colours, one per corner
     */
    static void DrawRoundRectangle(const Rectangle& area, const RoundedCorners& radii, const CornerColors& fill)
    { DrawRoundRectangle(area, radii, 0, CornerColors(), fill); }

    /*!
     * @brief Fill only, one radius for all corners, a colour per corner.
     * @param area   Outer area of the shape
     * @param radius Corner radius applied to all four corners
     * @param fill   Fill colours, one per corner
     */
    static void DrawRoundRectangle(const Rectangle& area, float radius, const CornerColors& fill)
    { DrawRoundRectangle(area, RoundedCorners(radius), 0, CornerColors(), fill); }

    /*!
     * @brief Border only, no fill: a radius and a colour per corner.
     * @param area      Outer area of the shape
     * @param radii     Corner radii, applied to the outer edge
     * @param thickness Border thickness in pixels, drawn inward
     * @param border    Border colours, one per corner
     */
    static void DrawRoundRectangleBorder(const Rectangle& area, const RoundedCorners& radii, int thickness,
                                         const CornerColors& border)
    { DrawRoundRectangle(area, radii, thickness, border, CornerColors()); }

    /*!
     * @brief Border only, no fill: a radius per corner, one colour.
     * @param area      Outer area of the shape
     * @param radii     Corner radii, applied to the outer edge
     * @param thickness Border thickness in pixels, drawn inward
     * @param border    Border colour
     */
    static void DrawRoundRectangleBorder(const Rectangle& area, const RoundedCorners& radii, int thickness,
                                         Colors::ColorRGBA border)
    { DrawRoundRectangle(area, radii, thickness, CornerColors(border), CornerColors()); }

    /*!
     * @brief Border only, no fill: one radius for all corners, one colour.
     * @param area      Outer area of the shape
     * @param radius    Corner radius applied to all four corners
     * @param thickness Border thickness in pixels, drawn inward
     * @param border    Border colour
     */
    static void DrawRoundRectangleBorder(const Rectangle& area, float radius, int thickness,
                                         Colors::ColorRGBA border)
    { DrawRoundRectangle(area, RoundedCorners(radius), thickness, CornerColors(border), CornerColors()); }

    /*!
     * @brief Draw a filled circle
     * @param cx Center X coordinate
     * @param cy Center Y coordinate
     * @param radius Circle radius in pixels
     * @param color Fill color
     */
    static void DrawCircle(float cx, float cy, float radius, unsigned int color);

    /*!
     * @brief Draw a filled circle (integer coordinates)
     * @param cx Center X coordinate
     * @param cy Center Y coordinate
     * @param radius Circle radius in pixels
     * @param color Fill color
     */
    static void DrawCircle(int cx, int cy, float radius, unsigned int color) { DrawCircle((float)cx, (float)cy, radius, color); }

    /*
     * Circles with a border
     *
     * Same contract as the rounded rectangles above: @p radius is the OUTER
     * one, so a border never grows the disc, @p thickness is in whole pixels
     * and is drawn INWARD, and the fill stops at the inner edge rather than
     * running under the border.
     *
     * The colour quartet has no corners to sit on here. It is laid on the four
     * corners of the square that bounds the circle, top-left first and
     * clockwise, and the disc samples the gradient spanning it. Four identical
     * colours give a flat disc; two over two give a straight gradient across
     * it; four different ones give the same bilinear blend a rectangle gets.
     */

    /*!
     * @brief Draw a circle with a border and a fill, the full form: a colour
     *        quartet for each.
     * @param cx        Centre X coordinate
     * @param cy        Centre Y coordinate
     * @param radius    Outer radius in pixels
     * @param thickness Border thickness in pixels, drawn inward, 0 for none
     * @param border    Border colours, on the bounding square's corners
     * @param fill      Fill colours, on the bounding square's corners
     */
    static void DrawCircle(float cx, float cy, float radius, int thickness,
                           const CornerColors& border, const CornerColors& fill);

    /*!
     * @brief Circle with a border and a fill, one colour each.
     * @param cx        Centre X coordinate
     * @param cy        Centre Y coordinate
     * @param radius    Outer radius in pixels
     * @param thickness Border thickness in pixels, drawn inward, 0 for none
     * @param border    Border colour
     * @param fill      Fill colour
     */
    static void DrawCircle(float cx, float cy, float radius, int thickness,
                           Colors::ColorRGBA border, Colors::ColorRGBA fill)
    { DrawCircle(cx, cy, radius, thickness, CornerColors(border), CornerColors(fill)); }

    /*!
     * @brief Circle with a border and a fill, one colour each, integer centre.
     * @param cx        Centre X coordinate
     * @param cy        Centre Y coordinate
     * @param radius    Outer radius in pixels
     * @param thickness Border thickness in pixels, drawn inward, 0 for none
     * @param border    Border colour
     * @param fill      Fill colour
     */
    static void DrawCircle(int cx, int cy, float radius, int thickness,
                           Colors::ColorRGBA border, Colors::ColorRGBA fill)
    { DrawCircle((float)cx, (float)cy, radius, thickness, CornerColors(border), CornerColors(fill)); }

    /*!
     * @brief Filled circle, no border, with a colour quartet.
     * @param cx     Centre X coordinate
     * @param cy     Centre Y coordinate
     * @param radius Radius in pixels
     * @param fill   Fill colours, on the bounding square's corners
     */
    static void DrawCircle(float cx, float cy, float radius, const CornerColors& fill)
    { DrawCircle(cx, cy, radius, 0, CornerColors(), fill); }

    /*!
     * @brief Circle outline, no fill, with a colour quartet.
     * @param cx        Centre X coordinate
     * @param cy        Centre Y coordinate
     * @param radius    Outer radius in pixels
     * @param thickness Border thickness in pixels, drawn inward
     * @param border    Border colours, on the bounding square's corners
     */
    static void DrawCircleBorder(float cx, float cy, float radius, int thickness,
                                 const CornerColors& border)
    { DrawCircle(cx, cy, radius, thickness, border, CornerColors()); }

    /*!
     * @brief Circle outline, no fill, one colour.
     * @param cx        Centre X coordinate
     * @param cy        Centre Y coordinate
     * @param radius    Outer radius in pixels
     * @param thickness Border thickness in pixels, drawn inward
     * @param border    Border colour
     */
    static void DrawCircleBorder(float cx, float cy, float radius, int thickness,
                                 Colors::ColorRGBA border)
    { DrawCircle(cx, cy, radius, thickness, CornerColors(border), CornerColors()); }

    /*!
     * @brief Circle outline, no fill, one colour, integer centre.
     * @param cx        Centre X coordinate
     * @param cy        Centre Y coordinate
     * @param radius    Outer radius in pixels
     * @param thickness Border thickness in pixels, drawn inward
     * @param border    Border colour
     */
    static void DrawCircleBorder(int cx, int cy, float radius, int thickness,
                                 Colors::ColorRGBA border)
    { DrawCircle((float)cx, (float)cy, radius, thickness, CornerColors(border), CornerColors()); }

    /*
     * Stencil
     */

    /*!
     * @brief Set a circular stencil mask.  All drawing calls until ClearStencil()
     *        will be clipped to the interior of this circle.
     * @param cx     Center X
     * @param cy     Center Y
     * @param radius Circle radius in pixels
     */
    static void CircleStencil(float cx, float cy, float radius);
    static void CircleStencil(int   cx, int   cy, float radius) { CircleStencil((float)cx, (float)cy, radius); }

    /*!
     * @brief Set a rounded-rectangle stencil mask.  All drawing calls until
     *        ClearStencil() will be clipped to the interior of this shape.
     * @param x      Left coordinate
     * @param y      Top coordinate
     * @param w      Width
     * @param h      Height
     * @param radius Corner radius in pixels
     */
    static void RoundRectangleStencil(float x, float y, float w, float h, float radius);
    static void RoundRectangleStencil(int   x, int   y, int   w, int   h, float radius) { RoundRectangleStencil((float)x, (float)y, (float)w, (float)h, radius); }
    static void RoundRectangleStencil(const Rectangle& area, float radius) { RoundRectangleStencil(area.Left(), area.Top(), area.Width(), area.Height(), radius); }

    /*!
     * @brief Remove the active stencil mask.  Drawing is no longer clipped.
     */
    static void ClearStencil();

    /*
     * Accessors
     */

    //! Get display Width as integer
    [[nodiscard]] int DisplayWidthAsInt() const { return mVirtualViewportSize.x(); }
    //! Get display Height as integer
    [[nodiscard]] int DisplayHeightAsInt() const { return mVirtualViewportSize.y(); }
    //! Get display Width as float
    [[nodiscard]] float DisplayWidthAsFloat() const { return mVirtualViewportSizeFloat.x(); }
    //! Get display Height as float
    [[nodiscard]] float DisplayHeightAsFloat() const { return mVirtualViewportSizeFloat.y(); }

    //! Is small resolution?
    [[nodiscard]] bool Is240p() const { return IsRotatedSide() ? mVirtualViewportSize.y() <= 480 || mViewportSize.x() <= 320 : mVirtualViewportSize.x() <= 480 || mViewportSize.y() <= 320; }
    //! Is middle resolution?
    [[nodiscard]] bool Is480pOrLower() const { return mViewportSize.y() <= 576; }
    //! Return true window width (not scaled, not rotated)
    [[nodiscard]] int RealDisplayWidthAsInt() const { return mViewportSize.x(); }
    //! Return true window height
    [[nodiscard]] int RealDisplayHeightAsInt() const { return mViewportSize.y(); }
    //! Return true if the screen is rotated either left or right
    [[nodiscard]] bool IsRotatedSide() const { return mRotate == RotationType::Left || mRotate == RotationType::Right; }
    //! Return the screen rotation
    [[nodiscard]] RotationType Rotation() const { return mRotate; }
    //! Is resolution QVGA or less?
    [[nodiscard]] bool IsQVGA() const { return RealDisplayHeightAsInt() <= 288; }
    //! Is Resolution between QVGA (excluded) & VGA (included)?
    [[nodiscard]] bool IsVGA() const { int h = RealDisplayHeightAsInt(); return h > 288 && h <= 576; }
    //! Is Resolution between VGA (excluded) & HD (included)?
    [[nodiscard]] bool IsHD() const { int h = RealDisplayHeightAsInt(); return h > 576 && h <= 920; }
    //! Is Resolution higher than HD?
    [[nodiscard]] bool IsFHD() const { return RealDisplayHeightAsInt() > 920; }


    //! Check if the Renderer is properly initialized
    [[nodiscard]] bool Initialized() const { return mViewPortInitialized; }
};
