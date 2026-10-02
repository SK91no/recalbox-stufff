#include "Renderer.h"
#include "rendering/RenderStats.h"
#include "ImageIO.h"
#include "../data/Resources.h"
#include "ResolutionAdapter.h"
#include "CrtConf.h"
#include <hardware/Board.h>

#include <cmath>

float Renderer::mLastXTranslation = 0.f;
float Renderer::mLastYTranslation = 0.f;
pthread_t Renderer::sGLContextThread = 0;
//HashMap<float, Renderer::FourRoundedCorner> Renderer::mCornerCache;

// ---------------------------------------------------------------------------
// Round rectangle arc cache
// ---------------------------------------------------------------------------
// Arc offsets are keyed by integer radius (nearest pixel).  Each entry stores
// the (dx, dy) offsets from the four corner centers for all N+1 arc points,
// in clockwise screen order: TR(-π/2→0) BR(0→π/2) BL(π/2→π) TL(π→3π/2).
// Fixed-size struct avoids any heap allocation; at most ~20KB for ~10 radii.
namespace {
  static constexpr int kMaxArcSegs = 64;                 // hard cap on N
  static constexpr int kArcDataLen = 4 * (kMaxArcSegs + 1) * 2; // 520 floats

  struct RoundArcEntry {
    int   N;                  //!< Actual segments per corner stored
    float r;                  //!< Exact radius used to build the offsets
    float data[kArcDataLen];  //!< (dx,dy) pairs: 4*(N+1) used, rest unused
  };

  static HashMap<int, RoundArcEntry> sRoundArcCache;

  // Maximum float count for a fan vertex buffer (center + 4*(N+1) + close)
  static constexpr int kFanBufferFloats = 2 * (1 + 4 * (kMaxArcSegs + 1) + 1); // 524

  // Build (or retrieve from cache) arc offsets for radius `r`.
  // Returns a const ref to the entry — valid until next cache insertion.
  const RoundArcEntry& GetArcEntry(float r)
  {
    int key = (int)roundf(r);
    if (RoundArcEntry* e = sRoundArcCache.try_get(key); e != nullptr)
      return *e;

    RoundArcEntry& e = sRoundArcCache[key];
    e.N = std::min(kMaxArcSegs, std::max(4, (int)(sqrtf(r) * 3.f)));
    e.r = r;

    static constexpr float kHalfPi = (float)M_PI_2;
    const float ang0[4] = { -kHalfPi, 0.f, kHalfPi, (float)M_PI };
    int idx = 0;
    for (int c = 0; c < 4; ++c)
      for (int i = 0; i <= e.N; ++i)
      {
        float a = ang0[c] + kHalfPi * (float)i / (float)e.N;
        e.data[idx++] = r * cosf(a);
        e.data[idx++] = r * sinf(a);
      }
    return e;
  }

  // Build a GL_TRIANGLE_FAN vertex array for a round rectangle.
  // r must already be clamped and >= 1. Returns vertex count.
  int BuildRoundRectFan(float x, float y, float w, float h, float r, float* verts)
  {
    const RoundArcEntry& arc = GetArcEntry(r);
    int N   = arc.N;
    int idx = 0;

    verts[idx++] = x + w * 0.5f;   // center
    verts[idx++] = y + h * 0.5f;

    const float cornX[4] = { x+w-r, x+w-r, x+r,   x+r   };
    const float cornY[4] = { y+r,   y+h-r, y+h-r, y+r   };
    const float* off = arc.data;
    for (int c = 0; c < 4; ++c)
    {
      float cx = cornX[c], cy = cornY[c];
      for (int i = 0; i <= N; ++i, off += 2)
      {
        verts[idx++] = cx + off[0];
        verts[idx++] = cy + off[1];
      }
    }
    verts[idx++] = cornX[0] + arc.data[0]; // close
    verts[idx++] = cornY[0] + arc.data[1];
    return idx / 2;
  }

  // -------------------------------------------------------------------------
  // Rounded rectangles with a border
  //
  // Both edges of a border are the same shape at two sizes, so they are built
  // by the same walker with the same segment count. Matching counts are what
  // make the border a plain ring: outer[i] pairs with inner[i], and two
  // triangles per step close the gap. Corner radii of zero still emit their
  // full run of points, collapsed onto the corner; the degenerate triangles
  // cost nothing to draw and keep the pairing exact when only one of the two
  // edges is actually rounded.
  // -------------------------------------------------------------------------

  //! Segments per quarter arc. Same rule as the uniform-colour fan above.
  int ArcSegments(float radius)
  { return radius < 1.f ? 4 : std::min(kMaxArcSegs, std::max(4, (int)(sqrtf(radius) * 3.f))); }

  //! Unit quarter-arc points, cached per segment count.
  //! Keyed on the count rather than the radius: the table is scaled at use, so
  //! one entry serves every size, where the radius-keyed cache above needs a
  //! fresh entry per distinct radius.
  const Array<Vector2f>& UnitArc(int segments)
  {
    static HashMap<int, Array<Vector2f>> sUnitArcs;
    if (const Array<Vector2f>* found = sUnitArcs.try_get(segments); found != nullptr) return *found;

    Array<Vector2f>& arc = sUnitArcs[segments];
    arc.ExpandTo(segments + 1);
    for (int i = 0; i <= segments; ++i)
    {
      float angle = (float)M_PI_2 * (float)i / (float)segments;
      arc(i) = Vector2f(cosf(angle), sinf(angle));
    }
    return arc;
  }

  //! Shrink radii together until no two of them overlap on a shared edge.
  //! Scaled as a group rather than clamped one by one, so an over-large set
  //! keeps its proportions instead of flattening whichever corner is read last.
  RoundedCorners ClampRadii(RoundedCorners radii, float w, float h)
  {
    if (radii.TopLeft     < 0.f) radii.TopLeft     = 0.f;
    if (radii.TopRight    < 0.f) radii.TopRight    = 0.f;
    if (radii.BottomRight < 0.f) radii.BottomRight = 0.f;
    if (radii.BottomLeft  < 0.f) radii.BottomLeft  = 0.f;

    float scale = 1.f;
    auto limit = [&scale](float sum, float length)
    { if (sum > length && sum > 0.f) scale = std::min(scale, length / sum); };
    limit(radii.TopLeft + radii.TopRight,       w);
    limit(radii.BottomLeft + radii.BottomRight, w);
    limit(radii.TopLeft + radii.BottomLeft,     h);
    limit(radii.TopRight + radii.BottomRight,   h);

    if (scale < 1.f)
    {
      radii.TopLeft     *= scale;
      radii.TopRight    *= scale;
      radii.BottomRight *= scale;
      radii.BottomLeft  *= scale;
    }
    return radii;
  }

  //! Walk the closed outline clockwise from the top-left corner.
  //! @return Point count, always 4 * (segments + 1)
  int BuildContour(float x, float y, float w, float h, const RoundedCorners& radii,
                   int segments, Vector2f* out)
  {
    const Array<Vector2f>& arc = UnitArc(segments);
    const int steps = segments + 1;
    int n = 0;

    // Top-left: left edge to top edge
    for (int i = 0; i < steps; ++i)
    {
      float cx = x + radii.TopLeft, cy = y + radii.TopLeft;
      out[n++] = Vector2f(cx - radii.TopLeft * arc[i].x(), cy - radii.TopLeft * arc[i].y());
    }
    // Top-right: top edge to right edge
    for (int i = 0; i < steps; ++i)
    {
      float cx = x + w - radii.TopRight, cy = y + radii.TopRight;
      out[n++] = Vector2f(cx + radii.TopRight * arc[i].y(), cy - radii.TopRight * arc[i].x());
    }
    // Bottom-right: right edge to bottom edge
    for (int i = 0; i < steps; ++i)
    {
      float cx = x + w - radii.BottomRight, cy = y + h - radii.BottomRight;
      out[n++] = Vector2f(cx + radii.BottomRight * arc[i].x(), cy + radii.BottomRight * arc[i].y());
    }
    // Bottom-left: bottom edge to left edge
    for (int i = 0; i < steps; ++i)
    {
      float cx = x + radii.BottomLeft, cy = y + h - radii.BottomLeft;
      out[n++] = Vector2f(cx - radii.BottomLeft * arc[i].y(), cy + radii.BottomLeft * arc[i].x());
    }
    return n;
  }

  //! Bilinear blend of four corner colours at a point of the given box.
  //! Interpolating from the box rather than along the outline is what keeps a
  //! gradient continuous across a corner instead of restarting at each arc.
  Colors::ColorRGBA ColorAt(const CornerColors& colors, float x, float y, float w, float h,
                            float px, float py)
  {
    float u = w > 0.f ? (px - x) / w : 0.f;
    float v = h > 0.f ? (py - y) / h : 0.f;
    if (u < 0.f) u = 0.f; else if (u > 1.f) u = 1.f;
    if (v < 0.f) v = 0.f; else if (v > 1.f) v = 1.f;

    Colors::ColorRGBA result = 0;
    for (int shift = 24; shift >= 0; shift -= 8)
    {
      float topLeft     = (float)((colors.TopLeft     >> shift) & 0xFF);
      float topRight    = (float)((colors.TopRight    >> shift) & 0xFF);
      float bottomRight = (float)((colors.BottomRight >> shift) & 0xFF);
      float bottomLeft  = (float)((colors.BottomLeft  >> shift) & 0xFF);
      float top    = topLeft    + (topRight    - topLeft)    * u;
      float bottom = bottomLeft + (bottomRight - bottomLeft) * u;
      float value  = top + (bottom - top) * v;
      result |= (Colors::ColorRGBA)(int)(value + 0.5f) << shift;
    }
    return result;
  }

  //! Scratch geometry for one rounded-rectangle draw. File-scope and only ever
  //! grown: rendering is single-threaded (CHECKGLCONTEXT), and the worst case
  //! is a few thousand vertices, far too much to leave on the stack — and a
  //! local Array would mean a malloc and a free on every shape drawn.
  Array<Vector2f>  sShapeVertices;
  Array<ColorByte> sShapeColors;
  Array<Vector2f>  sOuterContour;
  Array<Vector2f>  sInnerContour;

  //! Append one vertex and its colour to the scratch buffers.
  void PushShapeVertex(int& n, const Vector2f& point, Colors::ColorRGBA color)
  {
    sShapeVertices(n) = point;
    Colors::StoreGPUColor(sShapeColors.BufferReadWrite() + (n * 4), color);
    ++n;
  }

  // Build a GL_TRIANGLE_FAN vertex array for a full circle.
  // radius must be >= 1. Returns vertex count.
  int BuildCircleFan(float cx, float cy, float radius, float* verts)
  {
    const RoundArcEntry& arc = GetArcEntry(radius);
    int N   = arc.N;
    int idx = 0;

    verts[idx++] = cx;  // center
    verts[idx++] = cy;

    const float* off = arc.data;
    for (int c = 0; c < 4; ++c)
      for (int i = 0; i <= N; ++i, off += 2)
      {
        verts[idx++] = cx + off[0];
        verts[idx++] = cy + off[1];
      }
    verts[idx++] = cx + arc.data[0]; // close
    verts[idx++] = cy + arc.data[1];
    return idx / 2;
  }
} // namespace

#ifdef DEBUG
  #define CHECKGLCONTEXT assert(Renderer::sGLContextThread == pthread_self() && "Bad thread accessing GL methods !")
#else
  #define CHECKGLCONTEXT do{ if (Renderer::sGLContextThread != pthread_self()) { LOG(LogError) << "[Renderer] Bad thread accessing GL methods !"; } }while(false)
#endif

Renderer::Renderer(int width, int height, bool windowed, RotationType rotation)
  : StaticLifeCycleControler<Renderer>("Renderer")
  , mSdlWindow(nullptr)
  , mViewportSize(Vector2i(0,0))
  , mVirtualViewportSizeFloat(Vector2f(0,0))
  , mVirtualViewportSize(Vector2i(0,0))
  , mScale(Vector2f(1,1))
  , mRotate(rotation)
  , mViewPortInitialized(false)
  , mInitialCursorState(false)
  , mWindowed(windowed)
  , mOffscreenTexture(InvalidTextureHandle)
  , mOffscreenSize(Vector2i(0,0))
{
  // Derived class constructors are responsible for calling:
  //   mViewPortInitialized = Initialize(width, height);
  (void)width;
  (void)height;
}

Renderer::~Renderer()
{
  Finalize();
}

void Renderer::InformationLogs()
{
  SDL_version linked;
  SDL_GetVersion(&linked);
  LOG(LogInfo) << (_F("[Renderer] SDL2 Version {0}.{1}.{2}") / (int)linked.major / (int)linked.minor/ (int)linked.patch).ToString();

  int numVideoDrivers = SDL_GetNumVideoDrivers();
  String drivers;
  for (int i = 0; i < numVideoDrivers; i++)
  {
    if (!drivers.empty()) drivers.Append(", ");
    drivers.Append(SDL_GetVideoDriver(i));
  }
  LOG(LogInfo) << "[Renderer] Available video drivers: "<< drivers;
  LOG(LogInfo) << (_F("[Renderer] Using video driver: {0}") / SDL_GetCurrentVideoDriver()).ToString();

  int numAudioDrivers = SDL_GetNumAudioDrivers();
  drivers.clear();
  for (int i = 0; i < numAudioDrivers; i++)
  {
    if (!drivers.empty()) drivers.Append(", ");
    drivers.Append(SDL_GetAudioDriver(i));
  }
  LOG(LogInfo) << "[Renderer] Available audio drivers: " << drivers;
  const char* currentAudioDriver = SDL_GetCurrentAudioDriver();
  if (currentAudioDriver != nullptr) LOG(LogInfo) << (_F("[Renderer] Using audio driver: {0}") / currentAudioDriver).ToString();

  LOG(LogInfo) << "[Renderer] GL Vendor: " << GetGLVendor();
  LOG(LogInfo) << "[Renderer] GL Renderer: " << GetGLRenderer();
  LOG(LogInfo) << "[Renderer] GL Version: " << GetGLVersion();
  LOG(LogInfo) << "[Renderer] GL Extensions: " << GetGLExtensions();
}

String Renderer::GetGLExtensions()
{
  return Instance().DoGetDriverInfo(DriverInfo::Extensions);
}

String Renderer::GetGLVendor()
{
  return Instance().DoGetDriverInfo(DriverInfo::Vendor);
}

String Renderer::GetGLRenderer()
{
  return Instance().DoGetDriverInfo(DriverInfo::Renderer);
}

String Renderer::GetGLVersion()
{
  return Instance().DoGetDriverInfo(DriverInfo::Version);
}

bool Renderer::CreateSdlSurface(int width, int height)
{
  { LOG(LogInfo) << "[Renderer] Creating surface..."; }

  //hide mouse cursor
  mInitialCursorState = SDL_ShowCursor(0) == 1;

  // Let the backend set whatever SDL attributes it needs before the window exists
  DoConfigureContext();

  mViewportSize.Set(width, height);
  { LOG(LogInfo) << "[Renderer] Trying Resolution: " << width << ',' << height; }

  int flags = (int)DoWindowFlags();
  if (!mWindowed) flags |= SDL_WINDOW_FULLSCREEN;
  mSdlWindow = SDL_CreateWindow("EmulationStation",
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                mViewportSize.x(),
                                mViewportSize.y(),
                                flags | SDL_WINDOW_ALWAYS_ON_TOP);

  if (mSdlWindow == nullptr)
  {
    { LOG(LogError) << "[Renderer] Error creating SDL window!\n\t" << SDL_GetError(); }
    return false;
  }

  { LOG(LogInfo) << "[Renderer] Created window successfully."; }

  //set an icon for the window
  size_t icwidth = 0;
  size_t icheight = 0;
  std::vector<unsigned char> rawData = ImageIO::loadFromMemoryRGBA32(window_icon_256_png_data, window_icon_256_png_size,
                                                                     icwidth, icheight);
  if (!rawData.empty())
  {
    ImageIO::flipPixelsVert(rawData.data(), icwidth, icheight);

    //SDL interprets each pixel as a 32-bit number, so our masks must depend on the endianness (byte order) of the machine
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
    Uint32 rmask = 0xff000000; Uint32 gmask = 0x00ff0000; Uint32 bmask = 0x0000ff00; Uint32 amask = 0x000000ff;
    #else
    Uint32 rmask = 0x000000ff;
    Uint32 gmask = 0x0000ff00;
    Uint32 bmask = 0x00ff0000;
    Uint32 amask = 0xff000000;
    #endif
    //try creating SDL surface from logo data
    SDL_Surface* logoSurface = SDL_CreateRGBSurfaceFrom((void*) rawData.data(), (int)icwidth, (int)icheight, 32, (int)(icwidth * 4),
                                                        rmask, gmask, bmask, amask);
    if (logoSurface != nullptr)
    {
      SDL_SetWindowIcon(mSdlWindow, logoSurface);
      SDL_FreeSurface(logoSurface);
    }
  }

  // Create the rendering context/device & store thread id that created it
  if (!DoCreateContext(mSdlWindow))
  {
    { LOG(LogError) << "[Renderer] DoCreateContext() failed!"; }
    return false;
  }
  sGLContextThread = pthread_self();

  // Let the backend initialise its resources (shaders, pipelines, etc.)
  if (!DoInitContext())
  {
    { LOG(LogError) << "[Renderer] DoInitContext() failed!"; }
    return false;
  }

  // Activate driver debug output after context creation (DEBUG builds only)
  #ifdef DEBUG
  DoActivateDebug();
  #endif

  // Log info
  InformationLogs();

  return true;
}

void Renderer::SwapBuffers()
{
  DoPresent(mSdlWindow);
  RenderStats::Instance().EndFrame();
  DoClearBuffers();
}

void Renderer::DestroySdlSurface()
{
  DoDestroyContext();

  SDL_DestroyWindow(mSdlWindow);
  mSdlWindow = nullptr;

  //show mouse cursor
  SDL_ShowCursor(mInitialCursorState ? 1 : 0);
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void Renderer::GetResolutionFromConfiguration(int& w, int& h)
{
  ResolutionAdapter adapter;
  Resolutions::SimpleResolution output { .Width=0, .Height=0 };
  String resolution = RecalboxConf::Instance().GetEmulationstationVideoMode().LowerCase().Trim();
  { LOG(LogInfo) << "[Renderer] Get resolution from recalbox.conf: " << w << 'x' << h; }

  if (!adapter.AdjustResolution(0, resolution, output))
  {
    resolution = RecalboxConf::Instance().GetGlobalVideoMode().LowerCase();
    adapter.AdjustResolution(0, resolution, output);
  }
  if (output.IsValid())
  {
    w = output.Width;
    h = output.Height;
  }
  { LOG(LogInfo) << "[Renderer] Get resolution from configuration (adjusted): " << w << 'x' << h; }
}

bool Renderer::ReInitialize()
{
  // Reinitialize with real resolution settings
  return Initialize(mViewportSize.x(), mViewportSize.y());
}

bool Renderer::Rotate(RotationType rotation)
{
  this->mRotate = rotation;
  return true;
}

bool Renderer::Initialize(int w, int h)
{
  { LOG(LogInfo) << "[Renderer] Initial resolution: " << w << 'x' << h; }

  bool createdSurface = false;
  bool isCrt = Board::Instance().CrtBoard().IsCrtAdapterAttached();
  // Get resolution from config or crt fixed resolutions if either w or h is nul (not set from command line)
  if (w <= 0 || h <= 0)
  {
    if (isCrt = ResolutionAdapter::GetCRTResolution(w, h); !isCrt)
      GetResolutionFromConfiguration(w, h);
  }
  // Try to create surface if the resolution is valid
  if (w > 0 && h > 0) createdSurface = CreateSdlSurface(w, h);

  // Fallback to SDL2 default resolution
  if (!createdSurface)
  {
    ResolutionAdapter adapter;
    ResolutionAdapter::Resolution defaultRes = adapter.DefaultResolution();
    w = defaultRes.Width;
    h = defaultRes.Height;
    { LOG(LogInfo) << "[Renderer] Get default resolution from Resolution Adapter: " << w << 'x' << h; }
    if (w > 0 && h > 0)
      createdSurface = CreateSdlSurface(defaultRes.Width, defaultRes.Height);
  }
  if (!createdSurface)
    return false;

  mScale.Set(1.f, 1.f);
  mVirtualViewportSize.Set(mViewportSize.x(), mViewportSize.y());
  mVirtualViewportSizeFloat.Set((float)mViewportSize.x(), (float)mViewportSize.y());

  float scaledWidth = (float)mViewportSize.x();
  if (
    #ifdef OPTION_RECALBOX_SIMULATE_RRGBD
    true
    #else
    RecalboxConf::Instance().GetESForce43()
    #endif
  )
    scaledWidth = (float)mViewportSize.y() * 1.333333334f;
  if (isCrt)
  {
    if(Board::Instance().CrtBoard().ShouldWideScreen())
      scaledWidth = (float)mViewportSize.y() * 1.777777778f;
    else
      scaledWidth = (float)mViewportSize.y() * 1.333333334f;
  }

  mScale.Set((float)mViewportSize.x() / scaledWidth, 1);
  mVirtualViewportSize.Set((int) (scaledWidth), mViewportSize.y());
  mVirtualViewportSizeFloat.Set(scaledWidth, (float)mViewportSize.y());

  DoSetViewport(0, 0, mViewportSize.x(), mViewportSize.y());

  // Step 1: Set up the orthographic projection (GL-specific)
  DoBeginProjection(w, h);
  // Step 2: Apply rotation (common viewport size updates + GL-specific matrix ops)
  RotateViewport();
  // Step 3: Apply scale and switch matrix mode (GL-specific)
  DoEndProjection(mScale.x(), mScale.y());

  DoSetClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  InitializeClipping();
  CreateOffscreen();

  mViewPortInitialized = true;
  return true;
}

void Renderer::RotateViewport()
{
  switch(mRotate)
  {
    case RotationType::Right:
    {
      // Reverse virtual height and width
      mVirtualViewportSize.Set(mVirtualViewportSize.y(), mVirtualViewportSize.x());
      mVirtualViewportSizeFloat.Set(mVirtualViewportSizeFloat.y(), mVirtualViewportSizeFloat.x());
      mScale.Set(mScale.y(), mScale.x());
      DoApplyRotationToProjection(RotationType::Right, mViewportSize.x(), mViewportSize.y());
      break;
    }
    case RotationType::Left:
    {
      mVirtualViewportSize.Set(mVirtualViewportSize.y(), mVirtualViewportSize.x());
      mVirtualViewportSizeFloat.Set(mVirtualViewportSizeFloat.y(), mVirtualViewportSizeFloat.x());
      mScale.Set(mScale.y(), mScale.x());
      DoApplyRotationToProjection(RotationType::Left, mViewportSize.x(), mViewportSize.y());
      break;
    }
    case RotationType::Upsidedown:
    {
      DoApplyRotationToProjection(RotationType::Upsidedown, mViewportSize.x(), mViewportSize.y());
      break;
    }
    case RotationType::None:
    default:
      DoApplyRotationToProjection(RotationType::None, mViewportSize.x(), mViewportSize.y());
      break;
  }
}

void Renderer::Finalize()
{
  // Flush rendering statistics while the session that produced them is still
  // identifiable. Reached on application exit and whenever a game is launched.
  RenderStats::Instance().DumpAndReset("renderer shutdown");

  DestroyOffscreen();
  DoFinalizeContext();
  DestroySdlSurface();
}

// ---------------------------------------------------------------------------
// Offscreen FBO
// ---------------------------------------------------------------------------

void Renderer::CreateOffscreen()
{
  DestroyOffscreen(); // release any previous resources before (re-)creating

  int w = NextPow2(mViewportSize.x());
  int h = NextPow2(mViewportSize.y());
  mOffscreenSize.Set(w, h);

  if (!DoCreateOffscreen(w, h, mOffscreenTexture))
  {
    { LOG(LogError) << "[Renderer] Offscreen target creation failed"; }
    DestroyOffscreen();
    return;
  }
  { LOG(LogInfo) << "[Renderer] Offscreen target created: " << w << 'x' << h; }
}

void Renderer::DestroyOffscreen()
{
  DoDestroyOffscreen(mOffscreenTexture);
  mOffscreenSize.Set(0, 0);
}

void Renderer::BeginOffScreen()
{
  CHECKGLCONTEXT;
  Renderer& r = Instance();
  r.DoBeginOffScreen(r.mOffscreenSize.x(), r.mOffscreenSize.y());
}

void Renderer::EndOffScreen()
{
  CHECKGLCONTEXT;
  Renderer& r = Instance();
  r.DoEndOffScreen(r.mViewportSize.x(), r.mViewportSize.y());
}

void Renderer::BuildGLColorArray(ColorByte* ptr, Colors::ColorRGBA color, int vertCount)
{
  Colors::FillGPUColorArray(ptr, color, vertCount);
}

void Renderer::InitializeClipping()
{
  // Push a default clipping so that any current clipping call will return a valid clipping
  mClippings.Clear();
  mLastXTranslation = mLastYTranslation = 0;
  Clip(Rectangle(0, 0, mVirtualViewportSize.x(), mVirtualViewportSize.y()));
}

void Renderer::ClipAbsolute(const Rectangle& area)
{
  CHECKGLCONTEXT;
  // Copy
  Rectangle clip(area);
  clip.Scale(mScale.x(), mScale.y());
  // Rotate
  switch(mRotate)
  {
    case RotationType::None: break;
    case RotationType::Right: clip = Rectangle((float)mViewportSize.x() - clip.Bottom(), clip.Left(), clip.Height(), clip.Width()); break;
    case RotationType::Upsidedown: clip = Rectangle((float)mViewportSize.x() - clip.Left() - clip.Right(), (float)mViewportSize.y() - clip.Top() - clip.Bottom(), clip.Right(), clip.Bottom()); break;
    case RotationType::Left: clip = Rectangle(clip.Top(), (float)mViewportSize.y() - clip.Right(), clip.Height(), clip.Width()); break;
  }
  // Reverse the rectangle to match the GL 0,0 in bottom left
  clip.FlipOnScreen(mViewportSize.y());

  // constraint the new clipping whithin the last clipping limits
  if (!mClippings.Empty())
    clip.Intersect(mClippings.Peek());

  // Validate (ensure no negative values)
  clip.Validate();

  // Store new current clipping
  mCurrentClipping = clip;
  mCurrentClipping.FlipOnScreen(mViewportSize.y()); // Revert current clipping

  // Clip!
  mClippings.Push(clip);
  DoSetScissor((int)clip.Left(), (int)clip.Top(), (int)clip.Width(), (int)clip.Height());
}

void Renderer::Clip(const Transform4x4f& trans, const Vector2f& size)
{
  Vector2i clipPos((int)trans.translation().x(), (int)trans.translation().y());
  Vector3f dimScaled = trans * Vector3f(size.x(), size.y(), 0);
  Vector2i clipDim((int)(dimScaled.x() - trans.translation().x()), (int)(dimScaled.y() - trans.translation().y()));

  ClipAbsolute(Rectangle(clipPos, clipDim));
}

void Renderer::Clip(const Rectangle& area)
{
  // Translate clipping rectangle into absolute screen coordinate
  ClipAbsolute(Rectangle(area).Move(mLastXTranslation, mLastYTranslation));
}

void Renderer::Unclip()
{
  CHECKGLCONTEXT;

  // Check
  if (mClippings.Empty()) { LOG(LogError) << "Tried to popClipRect while the stack was empty!"; return; }

  // Pop!
  mClippings.Pop();

  // No more clippings?
  if (mClippings.Empty()) { DoDisableScissor(); return; }

  // Reclip using top clipping
  Rectangle& clip = mClippings.Peek();
  DoSetScissor((int)clip.Left(), (int)clip.Top(), (int)clip.Width(), (int)clip.Height());

  // Store new current clipping
  mCurrentClipping = clip;
  mCurrentClipping.FlipOnScreen(mViewportSize.y()); // Revert current clipping
}

TextureHandle Renderer::CreateGLTexture(bool nearest)
{
  CHECKGLCONTEXT;
  return Instance().DoCreateTexture(nearest);
}

void Renderer::DestroyGLTexture(TextureHandle id)
{
  CHECKGLCONTEXT;
  Instance().DoDestroyTexture(id);
}

void Renderer::DrawRectangle(const Rectangle& area, Colors::ColorARGB color, BlendMode blend)
{
  DrawRectangle(Math::roundi(area.Left()), Math::roundi(area.Top()),
                Math::roundi(area.Width()), Math::roundi(area.Height()),
                color, blend);
}

void
Renderer::DrawRectangle(float x, float y, float w, float h, Colors::ColorARGB color, BlendMode blend)
{
  DrawRectangle(Math::roundi(x), Math::roundi(y), Math::roundi(w), Math::roundi(h), color, blend);
}

void Renderer::DrawRectangle(int x, int y, int w, int h, Colors::ColorARGB color, BlendMode blend)
{
  CHECKGLCONTEXT;
  RenderStats::Instance().CountDraw(RenderStats::DrawKind::ColorRect, Vertex::sVertexPerRectangle);
  Instance().DoDrawColorRect(x, y, w, h, color, blend);
}

void Renderer::SetMatrix(const Transform4x4f& transform)
{
  CHECKGLCONTEXT;

  mLastXTranslation = transform.r3().x();
  mLastYTranslation = transform.r3().y();
  Instance().DoSetMatrix(transform);
}

RenderingError Renderer::UploadAlpha(TextureHandle id, int width, int height, const void* data)
{
  CHECKGLCONTEXT;
  return Instance().DoUploadTexture(id, width, height, data, TextureType::Alpha);
}

RenderingError Renderer::UploadRGBA(TextureHandle id, int width, int height, const void* data)
{
  CHECKGLCONTEXT;
  return Instance().DoUploadTexture(id, width, height, data, TextureType::RGBA);
}

RenderingError Renderer::UploadAlphaPart(TextureHandle id, int x, int y, int width, int height, const void* data)
{
  CHECKGLCONTEXT;
  return Instance().DoUploadTexturePart(id, x, y, width, height, data, TextureType::Alpha);
}

RenderingError Renderer::UploadRGBAPart(TextureHandle id, int x, int y, int width, int height, const void* data)
{
  CHECKGLCONTEXT;
  return Instance().DoUploadTexturePart(id, x, y, width, height, data, TextureType::RGBA);
}

void Renderer::DrawLines(const Vector2f coordinates[], const Colors::ColorARGB colors[], int count)
{
  CHECKGLCONTEXT;
  RenderStats::Instance().CountDraw(RenderStats::DrawKind::Lines, count);
  Instance().DoDrawLines(coordinates, colors, count);
}

void Renderer::DrawLines(const Vector2f coordinates[], Colors::ColorARGB color, int count)
{
  CHECKGLCONTEXT;
  RenderStats::Instance().CountDraw(RenderStats::DrawKind::Lines, count);
  Instance().DoDrawLinesUniform(coordinates, color, count);
}

// ---------------------------------------------------------------------------
// DrawColorFan  — shared by DrawRoundRectangle, DrawCircle
// ---------------------------------------------------------------------------
void Renderer::DrawColorFan(const float* verts, int count, unsigned int color)
{
  RenderStats::Instance().CountDraw(RenderStats::DrawKind::ColorFan, count);
  Instance().DoDrawColorFan(verts, count, color);
}

// ---------------------------------------------------------------------------
// WriteStencilFan  — shared by CircleStencil, RoundRectangleStencil
// ---------------------------------------------------------------------------
void Renderer::WriteStencilFan(const float* verts, int count)
{
  RenderStats::Instance().CountDraw(RenderStats::DrawKind::StencilFan, count);
  Instance().DoWriteStencilFan(verts, count);
}

// ---------------------------------------------------------------------------
// DrawRoundRectangle / DrawCircle  — delegate to builders + DrawColorFan
// ---------------------------------------------------------------------------
void Renderer::DrawRoundRectangle(float x, float y, float width, float height, float radius, unsigned int color)
{
  CHECKGLCONTEXT;
  float r = std::min(radius, std::min(width * 0.5f, height * 0.5f));
  if (r < 1.f) { DrawRectangle(x, y, width, height, color); return; }
  float verts[kFanBufferFloats];
  DrawColorFan(verts, BuildRoundRectFan(x, y, width, height, r, verts), color);
}

void Renderer::DrawRoundRectangle(const Rectangle& area, const RoundedCorners& radii, int thickness,
                                  const CornerColors& border, const CornerColors& fill)
{
  CHECKGLCONTEXT;

  float x = area.Left(), y = area.Top(), w = area.Width(), h = area.Height();
  if (w <= 0.f || h <= 0.f) return;

  bool wantBorder = thickness > 0 && !border.Invisible();
  bool wantFill   = !fill.Invisible();
  if (!wantBorder && !wantFill) return;

  RoundedCorners outerRadii = ClampRadii(radii, w, h);

  // A border can never eat more than half the box: past that the two edges
  // cross and there is no interior left to speak of.
  float weight = wantBorder ? std::min((float)thickness, std::min(w, h) * 0.5f) : 0.f;

  // One segment count for the whole shape, taken from the largest radius. Both
  // edges must agree on it for the ring to pair up, and a corner tessellated
  // more finely than its own radius needs costs only a few flat triangles.
  float largest = std::max(std::max(outerRadii.TopLeft, outerRadii.TopRight),
                           std::max(outerRadii.BottomRight, outerRadii.BottomLeft));
  int segments = ArcSegments(largest);
  int contourCount = 4 * (segments + 1);

  // Interior box, inset by the border weight, with concentric corners.
  float ix = x + weight, iy = y + weight;
  float iw = w - weight * 2.f, ih = h - weight * 2.f;
  bool hasInterior = iw > 0.f && ih > 0.f;
  RoundedCorners innerRadii(std::max(outerRadii.TopLeft     - weight, 0.f),
                            std::max(outerRadii.TopRight    - weight, 0.f),
                            std::max(outerRadii.BottomRight - weight, 0.f),
                            std::max(outerRadii.BottomLeft  - weight, 0.f));

  // Worst case: the interior as a triangle per contour step, plus two per step
  // for the ring.
  int maxVertices = contourCount * 9;
  sShapeVertices.ExpandTo(maxVertices);
  sShapeColors.ExpandTo(maxVertices * 4);

  Array<Vector2f>& outer = sOuterContour; outer.ExpandTo(contourCount);
  Array<Vector2f>& inner = sInnerContour; inner.ExpandTo(contourCount);
  BuildContour(x, y, w, h, outerRadii, segments, outer.BufferReadWrite());
  if (wantBorder && hasInterior)
    BuildContour(ix, iy, iw, ih, innerRadii, segments, inner.BufferReadWrite());

  int n = 0;

  // --- Interior ---
  // Filled as a fan of triangles around the centre, but emitted as a plain
  // triangle list so the interior and the ring travel in one single draw.
  if (wantBorder && !hasInterior)
  {
    // Thicker than half the box: nothing but border left, so paint the whole
    // outer shape in the border colours and skip the ring entirely.
    Vector2f centre(x + w * 0.5f, y + h * 0.5f);
    Colors::ColorRGBA centreColor = ColorAt(border, x, y, w, h, centre.x(), centre.y());
    for (int i = 0; i < contourCount; ++i)
    {
      const Vector2f& a = outer[i];
      const Vector2f& b = outer[(i + 1) % contourCount];
      PushShapeVertex(n, centre, centreColor);
      PushShapeVertex(n, a, ColorAt(border, x, y, w, h, a.x(), a.y()));
      PushShapeVertex(n, b, ColorAt(border, x, y, w, h, b.x(), b.y()));
    }
  }
  else if (wantFill)
  {
    // With no border the fill owns the outer shape; with one it stops at the
    // inner edge so a translucent border composites against the background
    // once rather than over the fill as well.
    const Array<Vector2f>& shape = wantBorder ? inner : outer;
    float fx = wantBorder ? ix : x, fy = wantBorder ? iy : y;
    float fw = wantBorder ? iw : w, fh = wantBorder ? ih : h;

    Vector2f centre(fx + fw * 0.5f, fy + fh * 0.5f);
    Colors::ColorRGBA centreColor = ColorAt(fill, fx, fy, fw, fh, centre.x(), centre.y());
    for (int i = 0; i < contourCount; ++i)
    {
      const Vector2f& a = shape[i];
      const Vector2f& b = shape[(i + 1) % contourCount];
      PushShapeVertex(n, centre, centreColor);
      PushShapeVertex(n, a, ColorAt(fill, fx, fy, fw, fh, a.x(), a.y()));
      PushShapeVertex(n, b, ColorAt(fill, fx, fy, fw, fh, b.x(), b.y()));
    }
  }

  // --- Border ring ---
  if (wantBorder && hasInterior)
    for (int i = 0; i < contourCount; ++i)
    {
      int next = (i + 1) % contourCount;
      const Vector2f& outerA = outer[i];
      const Vector2f& outerB = outer[next];
      const Vector2f& innerA = inner[i];
      const Vector2f& innerB = inner[next];
      Colors::ColorRGBA colorOuterA = ColorAt(border, x, y, w, h, outerA.x(), outerA.y());
      Colors::ColorRGBA colorOuterB = ColorAt(border, x, y, w, h, outerB.x(), outerB.y());
      Colors::ColorRGBA colorInnerA = ColorAt(border, x, y, w, h, innerA.x(), innerA.y());
      Colors::ColorRGBA colorInnerB = ColorAt(border, x, y, w, h, innerB.x(), innerB.y());

      PushShapeVertex(n, outerA, colorOuterA);
      PushShapeVertex(n, outerB, colorOuterB);
      PushShapeVertex(n, innerA, colorInnerA);

      PushShapeVertex(n, innerA, colorInnerA);
      PushShapeVertex(n, outerB, colorOuterB);
      PushShapeVertex(n, innerB, colorInnerB);
    }

  if (n == 0) return;

  RenderStats::Instance().CountDraw(RenderStats::DrawKind::ColorTriangles, n);
  Instance().DoDrawColorTriangles(sShapeVertices.BufferReadOnly(), sShapeColors.BufferReadOnly(), n);
}

void Renderer::DrawCircle(float cx, float cy, float radius, int thickness,
                          const CornerColors& border, const CornerColors& fill)
{
  if (radius < 1.f) return;

  // A circle is its bounding square with every corner rounded to half the side.
  // Routing it through the rounded rectangle keeps one implementation of the
  // contour walk, the concentric inner edge and the colour interpolation,
  // instead of a second one free to drift from it. ClampRadii leaves these
  // radii alone: two of them sum to exactly the side, which is the limit, not
  // past it.
  DrawRoundRectangle(Rectangle(cx - radius, cy - radius, radius * 2.f, radius * 2.f),
                     RoundedCorners(radius), thickness, border, fill);
}

void Renderer::DrawCircle(float cx, float cy, float radius, unsigned int color)
{
  CHECKGLCONTEXT;
  if (radius < 1.f) return;
  float verts[kFanBufferFloats];
  DrawColorFan(verts, BuildCircleFan(cx, cy, radius, verts), color);
}

// ---------------------------------------------------------------------------
// Stencil public API
// ---------------------------------------------------------------------------
void Renderer::CircleStencil(float cx, float cy, float radius)
{
  CHECKGLCONTEXT;
  if (radius < 1.f) return;
  float verts[kFanBufferFloats];
  WriteStencilFan(verts, BuildCircleFan(cx, cy, radius, verts));
}

void Renderer::RoundRectangleStencil(float x, float y, float w, float h, float radius)
{
  CHECKGLCONTEXT;
  float r = std::min(radius, std::min(w * 0.5f, h * 0.5f));
  if (r < 1.f)
  {
    // Degenerate: stencil a plain rectangle via its own triangle fan
    float verts[12] =
    {
      x + w*0.5f, y + h*0.5f,  // center
      x,          y,            // TL
      x,          y + h,        // BL
      x + w,      y + h,        // BR
      x + w,      y,            // TR
      x,          y,            // close
    };
    WriteStencilFan(verts, 6);
    return;
  }
  float verts[kFanBufferFloats];
  WriteStencilFan(verts, BuildRoundRectFan(x, y, w, h, r, verts));
}

void Renderer::ClearStencil()
{
  CHECKGLCONTEXT;
  Instance().DoClearStencil();
}

void Renderer::DrawRectangle(int x, int y, int w, int h,
                             Colors::ColorARGB topleftcolor,
                             Colors::ColorARGB toprightcolor,
                             Colors::ColorARGB bottomrightcolor,
                             Colors::ColorARGB bottomleftcolor)
{
  CHECKGLCONTEXT;
  RenderStats::Instance().CountDraw(RenderStats::DrawKind::ColorGradient, Vertex::sVertexPerRectangle);
  Instance().DoDrawColorRect4(x, y, w, h, topleftcolor, toprightcolor, bottomrightcolor, bottomleftcolor);
}

void Renderer::DrawTexturedTriangles(TextureHandle id, const Vertex vertices[], const ColorByte colors[], int count, bool tiled, TextureType type)
{
  CHECKGLCONTEXT;

  Instance().DoBindTexture(id);

  Instance().DoEnableBlend(BlendMode::AlphaBlend);
  Instance().DoSetTextureTiling(tiled);

  RenderStats::Instance().CountDraw(RenderStats::DrawKind::TexturedTriangles, count, type == TextureType::Alpha);
  Instance().DoDrawTTWithColors(vertices, colors, count, type);

  Instance().DoDisableBlend();
}

void Renderer::DrawTexturedTriangles(TextureHandle id, const Vertex vertices[], Colors::ColorARGB color, int count, bool tiled, TextureType type)
{
  CHECKGLCONTEXT;

  Instance().DoBindTexture(id);

  Instance().DoEnableBlend(BlendMode::AlphaBlend);
  Instance().DoSetTextureTiling(tiled);

  RenderStats::Instance().CountDraw(RenderStats::DrawKind::TexturedTriangles, count, type == TextureType::Alpha);
  Instance().DoDrawTTUniform(vertices, color, count, type);

  Instance().DoDisableBlend();
}

void Renderer::DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool keepratio, bool tiled, TextureType type)
{
  CHECKGLCONTEXT;

  Instance().DoBindTexture(id);

  if (keepratio && textureW != 0 && textureH != 0)
  {
    float areaRatio = (float)w / (float)h;
    float textureRatio = (float)textureW / (float)textureH;
    if (areaRatio < textureRatio)
    {
      double height = (float)w / textureRatio;
      y += (h - (int) height) / 2;
      h = (int)height;
    }
    else
    {
      double width = (float)h * textureRatio;
      x += (w - (int) width) / 2;
      w = (int)width;
    }
  }

  float tx = tiled ? (float)w / (float)textureW : 1.f;
  float ty = tiled ? (float)h / (float)textureH : 1.f;
  Vertex vertices[Vertex::sVertexPerRectangle]
    {
      { .Target={ (float)x       , (float)y       }, .Source={ 0.f,  ty } },
      { .Target={ (float)x       , (float)(y + h) }, .Source={ 0.f, 0.f } },
      { .Target={ (float)(x + w) , (float)y       }, .Source={  tx,  ty } },
      { .Target={ (float)(x + w) , (float)y       }, .Source={  tx,  ty } },
      { .Target={ (float)x       , (float)(y + h) }, .Source={ 0.f, 0.f } },
      { .Target={ (float)(x + w) , (float)(y + h) }, .Source={  tx, 0.f } }
    };

  Instance().DoEnableBlend(BlendMode::AlphaBlend);
  Instance().DoSetTextureTiling(tiled);

  RenderStats::Instance().CountDraw(RenderStats::DrawKind::TexturedQuad, Vertex::sVertexPerRectangle, type == TextureType::Alpha);
  Instance().DoSubmitTexQuadUniform(vertices, Vertex::sVertexPerRectangle, 1.f, 1.f, 1.f, 1.f, type);

  Instance().DoDisableBlend();
}

void Renderer::DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool keepratio, bool tiled, unsigned char alpha, TextureType type)
{
  CHECKGLCONTEXT;

  Instance().DoBindTexture(id);

  if (keepratio && textureW != 0 && textureH != 0)
  {
    float areaRatio = (float)w / (float)h;
    float textureRatio = (float)textureW / (float)textureH;
    if (areaRatio < textureRatio)
    {
      double height = (float)w / textureRatio;
      y += (h - (int) height) / 2;
      h = (int)height;
    }
    else
    {
      double width = (float)h * textureRatio;
      x += (w - (int) width) / 2;
      w = (int)width;
    }
  }

  float tx = tiled ? (float)w / (float)textureW : 1.f;
  float ty = tiled ? (float)h / (float)textureH : 1.f;
  Vertex vertices[Vertex::sVertexPerRectangle]
    {
      { .Target={ (float)x       , (float)y       }, .Source={ 0.f,  ty } },
      { .Target={ (float)x       , (float)(y + h) }, .Source={ 0.f, 0.f } },
      { .Target={ (float)(x + w) , (float)y       }, .Source={  tx,  ty } },
      { .Target={ (float)(x + w) , (float)y       }, .Source={  tx,  ty } },
      { .Target={ (float)x       , (float)(y + h) }, .Source={ 0.f, 0.f } },
      { .Target={ (float)(x + w) , (float)(y + h) }, .Source={  tx, 0.f } }
    };

  Instance().DoEnableBlend(BlendMode::AlphaBlend);
  Instance().DoSetTextureTiling(tiled);

  RenderStats::Instance().CountDraw(RenderStats::DrawKind::TexturedQuad, Vertex::sVertexPerRectangle, type == TextureType::Alpha);
  Instance().DoSubmitTexQuadUniform(vertices, Vertex::sVertexPerRectangle, 1.f, 1.f, 1.f, (float)alpha / 255.f, type);

  Instance().DoDisableBlend();
}

void Renderer::DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool keepratio, bool tiled, Colors::ColorARGB color, TextureType type)
{
  CHECKGLCONTEXT;

  Instance().DoBindTexture(id);

  if (keepratio && textureW != 0 && textureH != 0)
  {
    float areaRatio = (float)w / (float)h;
    float textureRatio = (float)textureW / (float)textureH;
    if (areaRatio < textureRatio)
    {
      double height = (float)w / textureRatio;
      y += (h - (int) height) / 2;
      h = (int)height;
    }
    else
    {
      double width = (float)h * textureRatio;
      x += (w - (int) width) / 2;
      w = (int)width;
    }
  }

  float tx = tiled ? (float)w / (float)textureW : 1.f;
  float ty = tiled ? (float)h / (float)textureH : 1.f;
  Vertex vertices[Vertex::sVertexPerRectangle]
    {
      { .Target={ (float)x       , (float)y       }, .Source={ 0.f,  ty } },
      { .Target={ (float)x       , (float)(y + h) }, .Source={ 0.f, 0.f } },
      { .Target={ (float)(x + w) , (float)y       }, .Source={  tx,  ty } },
      { .Target={ (float)(x + w) , (float)y       }, .Source={  tx,  ty } },
      { .Target={ (float)x       , (float)(y + h) }, .Source={ 0.f, 0.f } },
      { .Target={ (float)(x + w) , (float)(y + h) }, .Source={  tx, 0.f } }
    };

  Instance().DoEnableBlend(BlendMode::AlphaBlend);
  Instance().DoSetTextureTiling(tiled);

  Instance().DoSubmitTexQuadUniform(vertices, Vertex::sVertexPerRectangle,
    (float)(ColorByte)(color >> 24) / 255.f,
    (float)(ColorByte)(color >> 16) / 255.f,
    (float)(ColorByte)(color >>  8) / 255.f,
    (float)(ColorByte)(color >>  0) / 255.f,
    type);

  Instance().DoDisableBlend();
}

void Renderer::DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool flipX, bool flipY, bool keepratio, bool tiled, Colors::ColorARGB color, TextureType type)
{
  CHECKGLCONTEXT;

  Instance().DoBindTexture(id);

  if (keepratio && textureW != 0 && textureH != 0)
  {
    float areaRatio = (float)w / (float)h;
    float textureRatio = (float)textureW / (float)textureH;
    if (areaRatio < textureRatio)
    {
      double height = (float)w / textureRatio;
      y += (h - (int) height) / 2;
      h = (int)height;
    }
    else
    {
      double width = (float)h * textureRatio;
      x += (w - (int) width) / 2;
      w = (int)width;
    }
  }

  int x1 = x;
  int x2 = x;
  if (flipX) x1 += w; else x2 += w;
  int y1 = y;
  int y2 = y;
  if (flipY) y1 += h; else y2 += h;
  float tx = tiled ? (float)w / (float)textureW : 1.f;
  float ty = tiled ? (float)h / (float)textureH : 1.f;
  Vertex vertices[Vertex::sVertexPerRectangle]
    {
      { .Target={ (float)x1, (float)y1 }, .Source={ 0.f,  ty } },
      { .Target={ (float)x1, (float)y2 }, .Source={ 0.f, 0.f } },
      { .Target={ (float)x2, (float)y1 }, .Source={  tx,  ty } },
      { .Target={ (float)x2, (float)y1 }, .Source={  tx,  ty } },
      { .Target={ (float)x1, (float)y2 }, .Source={ 0.f, 0.f } },
      { .Target={ (float)x2, (float)y2 }, .Source={  tx, 0.f } }
    };

  Instance().DoEnableBlend(BlendMode::AlphaBlend);
  Instance().DoSetTextureTiling(tiled);

  Instance().DoSubmitTexQuadUniform(vertices, Vertex::sVertexPerRectangle,
    (float)(ColorByte)(color >> 24) / 255.f,
    (float)(ColorByte)(color >> 16) / 255.f,
    (float)(ColorByte)(color >>  8) / 255.f,
    (float)(ColorByte)(color >>  0) / 255.f,
    type);

  Instance().DoDisableBlend();
}

void Renderer::DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool keepratio, bool tiled,
                           Colors::ColorARGB topleftcolor, Colors::ColorARGB toprightcolor,
                           Colors::ColorARGB bottomrightcolor, Colors::ColorARGB bottomleftcolor,
                           TextureType type)
{
  CHECKGLCONTEXT;

  Instance().DoBindTexture(id);

  if (keepratio && textureW != 0 && textureH != 0)
  {
    float areaRatio = (float)w / (float)h;
    float textureRatio = (float)textureW / (float)textureH;
    if (areaRatio < textureRatio)
    {
      double height = (float)w / textureRatio;
      y += (h - (int) height) / 2;
      h = (int)height;
    }
    else
    {
      double width = (float)h * textureRatio;
      x += (w - (int) width) / 2;
      w = (int)width;
    }
  }

  float tx = tiled ? (float)w / (float)textureW : 1.f;
  float ty = tiled ? (float)h / (float)textureH : 1.f;
  Vertex vertices[Vertex::sVertexPerRectangle]
    {
      { .Target={ (float)x       , (float)y       }, .Source={ 0.f,  ty } },
      { .Target={ (float)x       , (float)(y + h) }, .Source={ 0.f, 0.f } },
      { .Target={ (float)(x + w) , (float)y       }, .Source={  tx,  ty } },
      { .Target={ (float)(x + w) , (float)y       }, .Source={  tx,  ty } },
      { .Target={ (float)x       , (float)(y + h) }, .Source={ 0.f, 0.f } },
      { .Target={ (float)(x + w) , (float)(y + h) }, .Source={  tx, 0.f } }
    };

  Colors::ColorRGBA colors[Vertex::sVertexPerRectangle];
  ColorToByteArray((ColorByte*)&colors[0], topleftcolor);
  ColorToByteArray((ColorByte*)&colors[1], bottomleftcolor);
  ColorToByteArray((ColorByte*)&colors[2], toprightcolor);
  colors[3] = colors[2];
  colors[4] = colors[1];
  ColorToByteArray((ColorByte*)&colors[5], bottomrightcolor);

  Instance().DoEnableBlend(BlendMode::AlphaBlend);
  Instance().DoSetTextureTiling(tiled);

  RenderStats::Instance().CountDraw(RenderStats::DrawKind::TexturedQuad, Vertex::sVertexPerRectangle, type == TextureType::Alpha);
  Instance().DoSubmitTexQuadColors(vertices, colors, Vertex::sVertexPerRectangle, type);

  Instance().DoDisableBlend();
}

void Renderer::DrawTexture(TextureHandle id, int textureW, int textureH, int x, int y, int w, int h, bool keepratio, bool tiled,
                           bool flipOnX, bool flipOnY,
                           Colors::ColorARGB topleftcolor, Colors::ColorARGB toprightcolor,
                           Colors::ColorARGB bottomrightcolor, Colors::ColorARGB bottomleftcolor,
                           float topAlphaReflection, float bottomAlphaReflection,
                           TextureType type)
{
  CHECKGLCONTEXT;

  Instance().DoBindTexture(id);

  if (keepratio && textureW != 0 && textureH != 0)
  {
    float areaRatio = (float)w / (float)h;
    float textureRatio = (float)textureW / (float)textureH;
    if (areaRatio < textureRatio)
    {
      double height = (float)w / textureRatio;
      y += (h - (int) height) / 2;
      h = (int)height;
    }
    else
    {
      double width = (float)h * textureRatio;
      x += (w - (int) width) / 2;
      w = (int)width;
    }
  }

  int x1 = x;
  int x2 = x;
  if (flipOnX) x1 += w; else x2 += w;
  int y1 = y;
  int y2 = y;
  if (flipOnY) y1 += h; else y2 += h;

  // Flipping only mirrors the texture coordinates: the colors and the reflection gradient stay
  // anchored to the screen corners. Vertex 0 sits at (x1,y1), which a flip moves to the opposite
  // corner, so swap the inputs to keep every color on the corner it names.
  if (flipOnX) { std::swap(topleftcolor, toprightcolor); std::swap(bottomleftcolor, bottomrightcolor); }
  if (flipOnY)
  {
    std::swap(topleftcolor, bottomleftcolor);
    std::swap(toprightcolor, bottomrightcolor);
    std::swap(topAlphaReflection, bottomAlphaReflection);
  }

  float tx = tiled ? (float)w / (float)textureW : 1.f;
  float ty = tiled ? (float)h / (float)textureH : 1.f;
  Vertex vertices[Vertex::sVertexPerRectangle]
  {
    { .Target={ (float)x1, (float)y1 }, .Source={ 0.f,  ty } },
    { .Target={ (float)x1, (float)y2 }, .Source={ 0.f, 0.f } },
    { .Target={ (float)x2, (float)y1 }, .Source={  tx,  ty } },
    { .Target={ (float)x2, (float)y1 }, .Source={  tx,  ty } },
    { .Target={ (float)x1, (float)y2 }, .Source={ 0.f, 0.f } },
    { .Target={ (float)x2, (float)y2 }, .Source={  tx, 0.f } }
  };

  Colors::ColorRGBA colors[Vertex::sVertexPerRectangle];
  ColorToByteArray((ColorByte*)&colors[0], topleftcolor);
  ColorToByteArray((ColorByte*)&colors[1], bottomleftcolor);
  ColorToByteArray((ColorByte*)&colors[2], toprightcolor);
  colors[3] = colors[2];
  colors[4] = colors[1];
  ColorToByteArray((ColorByte*)&colors[5], bottomrightcolor);

  Instance().DoEnableBlend(BlendMode::AlphaBlend);
  Instance().DoSetTextureTiling(tiled);

  if (topAlphaReflection != 0.f || bottomAlphaReflection != 0.f)
  {
    // Build the second-draw (reflected) vertex and color arrays
    Vertex vertices2[Vertex::sVertexPerRectangle];
    for (int i = 0; i < Vertex::sVertexPerRectangle; ++i)
    {
      vertices2[i] = vertices[i];
      vertices2[i].Target.Y += (float)h;
      vertices2[i].Source.Y = (vertices2[i].Source.Y == 0.f) ? 1.f : 0.f;
    }
    Colors::ColorRGBA colors2[Vertex::sVertexPerRectangle];
    ColorToByteArray((ColorByte*)&colors2[0], (topleftcolor    & 0xFFFFFF00u) | (unsigned int)((float)(topleftcolor    & 0xFF) * topAlphaReflection));
    ColorToByteArray((ColorByte*)&colors2[1], (bottomleftcolor & 0xFFFFFF00u) | (unsigned int)((float)(bottomleftcolor & 0xFF) * bottomAlphaReflection));
    ColorToByteArray((ColorByte*)&colors2[2], (toprightcolor   & 0xFFFFFF00u) | (unsigned int)((float)(toprightcolor   & 0xFF) * topAlphaReflection));
    colors2[3] = colors2[2];
    colors2[4] = colors2[1];
    ColorToByteArray((ColorByte*)&colors2[5], (bottomrightcolor & 0xFFFFFF00u) | (unsigned int)((float)(bottomrightcolor & 0xFF) * bottomAlphaReflection));

    // Two draws back to back, hence two draw calls accounted for
    RenderStats::Instance().CountDraw(RenderStats::DrawKind::TexturedQuad, Vertex::sVertexPerRectangle, type == TextureType::Alpha);
    RenderStats::Instance().CountDraw(RenderStats::DrawKind::TexturedQuad, Vertex::sVertexPerRectangle, type == TextureType::Alpha);
    Instance().DoSubmitTexQuadColorsTwoDraws(vertices, colors, vertices2, colors2, Vertex::sVertexPerRectangle, type);
  }
  else
  {
    RenderStats::Instance().CountDraw(RenderStats::DrawKind::TexturedQuad, Vertex::sVertexPerRectangle, type == TextureType::Alpha);
    Instance().DoSubmitTexQuadColors(vertices, colors, Vertex::sVertexPerRectangle, type);
  }

  Instance().DoDisableBlend();
}

/*
void Renderer::CreateRoundCorner(float x, float y, float startAngle, float arc, float radius, float pieces, ArrayOfPoint& points)
{
  x += radius * Math::cos(startAngle + (Math::PiF / 2.0f));
  y += radius * Math::sin(startAngle + (Math::PiF / 2.0f));

  int n = (int)ceil(pieces * arc / Math::PiF * 2.0f);
  float step = arc / (float)n;

  points.Reserve(n + 1);
  for (int i = n + 1; --i >= 0; startAngle += step)
  {
    Vertex::Point p(x + (radius * Math::sin(startAngle)), y - (radius * Math::cos(startAngle)));
    points.Add(p);
  }
}

Renderer::ArrayOfPoint Renderer::CreateRoundRect(float x, float y, float width, float height, float radius)
{
  static constexpr float sRoundingPiecePerQuarter = 8.f;
  float pieces = Math::max(radius / 3.0f, sRoundingPiecePerQuarter);

  FourRoundedCorner* corners = mCornerCache.try_get(radius);
  if (corners == nullptr)
  {
    mCornerCache.insert(radius, FourRoundedCorner());
    corners = mCornerCache.try_get(radius);
    CreateRoundCorner(0, radius, 3.0f * Math::PiF / 2.0f, Math::PiF / 2.0f, radius, pieces, corners->mTopLeft);
    CreateRoundCorner(-radius, 0, 0.0, Math::PiF / 2.0f, radius, pieces, corners->mTopRight);
    CreateRoundCorner(0, -radius, Math::PiF / 2.0f, Math::PiF / 2.0f, radius, pieces, corners->mBottomRight);
    CreateRoundCorner(radius, 0, Math::PiF, Math::PiF / 2.0f, radius, pieces, corners->mBottomLeft);
  }
  ArrayOfPoint result(corners->Count());
  FourRoundedCorner::Copy(corners->mTopLeft, result, x, y);
  FourRoundedCorner::Copy(corners->mTopRight, result, x + width, y);
  FourRoundedCorner::Copy(corners->mBottomRight, result, x + width, y + height);
  FourRoundedCorner::Copy(corners->mBottomLeft, result, x, y + height);
  return result;
}

void Renderer::DrawRoundRectangle(float x, float y, float width, float height, float radius, unsigned int color)
{
  glDisable(GL_TEXTURE_2D);

  ArrayOfPoint points = CreateRoundRect(x, y, width, height, radius);
  DrawTriangleFan(points.BufferReadOnly(), points.Count(), color);
}

void Renderer::DrawTriangleFan(const Vertex::Point* coords, int count, unsigned int color)
{
  Instance().DoEnableBlend(BlendMode::AlphaBlend);

  glEnableClientState(GL_VERTEX_ARRAY);

  glVertexPointer(2, GL_FLOAT, sizeof(Vertex::Point), coords);
  glColor4ub((color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);

  glDrawArrays(GL_TRIANGLE_FAN, 0, count);

  glDisableClientState(GL_VERTEX_ARRAY);
  Instance().DoDisableBlend();
}

void Renderer::DrawTriangleFan(const Vertex* vertexes, int count, unsigned int color)
{
  Instance().DoEnableBlend(BlendMode::AlphaBlend);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &vertexes[0].Target);
  glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &vertexes[0].Source);
  glColor4ub((color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);

  glDrawArrays(GL_TRIANGLE_FAN, 0, count);

  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
  Instance().DoDisableBlend();
}
*/
