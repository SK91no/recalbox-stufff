//
// Created by Bkg2k on 10/01/2020.
//
#pragma once

#include <atomic>
#include <platform_gl.h>

#include "utils/datetime/DateTime.h"
#include "utils/os/fs/Path.h"
#include "utils/os/system/Mutex.h"
#include <utils/math/Vector2i.h>
#include "utils/cplusplus/Bitflags.h"
#include "utils/cplusplus/INoCopy.h"
#include "IUnloadable.h"
#include "utils/storage/Array.h"
#include "rendering/opengl/Vertex.h"
#include "rendering/images/IImage.h"
#include "rendering/images/vectors/Svg.h"
#include "rendering/images/pixels/Png.h"
#include "rendering/opengl/Colors.h"
#include "rendering/opengl/Rectangle.h"
#include <Renderer.h>

class TextureHolder : private INoCopy, public IUnloadable
{
  public:
    //! Texture properties ad a bitflag
    enum class Properties
    {
      None          =  0, //!< No properties
      ImmediateLoad =  1, //!< Require to be loaded immediately (no threaded-load)
      NoCache       =  2, //!< Texture must not be cached
      Mutable       =  4, //!< Texture is a mutable texture, must not be cached
      Free          =  8, //!< Free texture, not loaded from file, created once, always available
      Alpha         = 16, //!< Alpha only texture (non ARGB). Only active when used with Free
      NoFiltering   = 32, //!< Do not use bilinear filtering. Use nearest instead
    };

    //! Load status
    enum class Status
    {
      Unloaded, //!< Texture is not loaded yet or has been unloaded
      Queued,   //!< Texture has been queued for loading
      Loading,  //!< Texture is currently loading
      Loaded,   //!< Texture is loaded
      Error,    //!< Texture cannot be loaded
    };

    static unsigned long long Tick()
    {
      timespec time = { .tv_sec=0, .tv_nsec=0 };
      (void)clock_gettime(CLOCK_REALTIME, &time);
      return ((long long)time.tv_sec << 24) | (time.tv_nsec >> 8);
    }

    /*!
     * @brief Build a new texture
     * @param path File to load
     * @param properties Texture properties
     */
    explicit TextureHolder(const Path& path, Properties properties, IHttpConfiguration& httpConfigure);

    /*!
     * @brief Build a new texture
     * @param path File to load
     * @param targetWidth target width (for SVG only)
     * @param targetWidth target height (for SVG only)
     * @param properties Texture properties
     */
    explicit TextureHolder(const Path& path, int targetWidth, int targetHeight, Properties properties, IHttpConfiguration& httpConfigure);

    /*!
     * @brief Destructor
     */
    virtual ~TextureHolder();

    /*!
     * @brief Special method for fast texture update (video player)
     * @param data RGBA data
     * @param width Data (and texture) width
     * @param height Data (and texture) height
     * @return True
     */
    [[nodiscard]] bool UpdateFromRawData(const unsigned char* data, int width, int height);

    //! Clear texture data and reset size
    void Clear() { Unload(); }

    /*!
     * @brief Upload an alpha bitmap into the texture
     * @param width Bitmap width
     * @param height Bitmap Height
     * @param data Pointer to Bitmap data, one byte per pixel
     */
    [[nodiscard]] bool UploadAlpha(int width, int height, const void* data);

    /*!
     * @brief Upload an RGBA bitmap into the texture
     * @param width Bitmap width
     * @param height Bitmap Height
     * @param data Pointer to Bitmap data, 4 byte per pixel
     */
    [[nodiscard]] bool UploadRGBA(int width, int height, const void* data);

    /*!
     * @brief Upload an alpha bitmap into the texture
     * @param x Destination x coordinate (in pixel)
     * @param y Destination y coordinate (in pixel)
     * @param width Bitmap width
     * @param height Bitmap Height
     * @param data Pointer to Bitmap data, one byte per pixel
     */
    [[nodiscard]] bool UploadAlphaPart(int x, int y, int width, int height, const void* data);

    /*!
     * @brief Upload an RGBA bitmap into the texture
     * @param x Destination x coordinate (in pixel)
     * @param y Destination y coordinate (in pixel)
     * @param width Bitmap width
     * @param height Bitmap Height
     * @param data Pointer to Bitmap data, 4 byte per pixel
     */
    [[nodiscard]] bool UploadRGBAPart(int x, int y, int width, int height, const void* data);

    /*!
     * @brief Blit part of the texture
     * @param vertex Vertex list
     * @param color Color list
     * @param vertexCount Vectex count
     * @param tiled Draw tiled texture
     */
    void Render(const Vertex vertex[], const ColorByte color[], int vertexCount, bool tiled, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Blit part of the texture
     * @param vertex Vertex list
     * @param color Color list
     * @param vertexCount Vectex count
     * @param tiled Draw tiled texture
     */
    void Render(const Vertex vertex[], unsigned int color, int vertexCount, bool tiled, Renderer::TextureType type = Renderer::TextureType::RGBA);

    //! Get last use timestamp
    [[nodiscard]] unsigned long long LastUse() const { return mLastUse; }

    //! Check if the current texture has to be loaded immediately
    [[nodiscard]] bool MustLoadImmediately() const { return ((int)mProperties & (int)Properties::ImmediateLoad) != 0; }

    //! Check if the texture is not cached
    [[nodiscard]] bool IsNotCached() const { return ((int)mProperties & (int)Properties::NoCache) != 0; }

    //! Check if the texture is mutable
    [[nodiscard]] bool IsMutable() const { return ((int)mProperties & (int)Properties::Mutable) != 0; }

    //! Check if the texture is mutable
    [[nodiscard]] bool IsFree() const { return ((int)mProperties & (int)Properties::Free) != 0; }

    //! Check if the texture is built from an SVG file
    [[nodiscard]] bool IsSVG() const { return mIsSVG; }

    //! Get current status
    [[nodiscard]] enum Status Status() const { return mStatus.load(); }

    //! Check if the target size has been set
    [[nodiscard]] bool HasTargetSize() const { return (mTargetWidth | mTargetHeight) != 0; }

    //! Return the texture as a IUnloadable object
    IUnloadable& AsIUnloadable() { return *((IUnloadable*)this); }

    /*!
     * @brief Set the status to Loading by anticipation, to avoid re-entry issues
     */
    void StartLoading() { mStatus = Status::Queued; Touch(); }

    /*!
     * @brief Get resource size
     * @return Size
     */
    Vector2i Size();

    /*!
     * @brief Free all possible CPU caches of this object
     */
    void FreeCPUCaches() { Mutex::AutoLock lock(mMutex); mImage.ClearCaches(); }

    /*!
     * @brief Non-blocking CPU cache free for bulk eviction. NEVER blocks on a busy
     * (loading/downloading/rendering) texture: such a texture is simply skipped. This is what keeps
     * the eviction path from deadlocking against a load that holds the texture lock across slow I/O.
     * @param onlyIfBackedByGpu If true, only drop the cache when the texture already has a GPU copy,
     *        so work-in-progress textures (not yet uploaded) keep their bitmap and can still display
     * @return True if the texture was processed, false if it was busy and skipped
     */
    bool TryFreeCPUCaches(bool onlyIfBackedByGpu)
    {
      if (!mMutex.TryLock()) return false;
      if (!onlyIfBackedByGpu || mGLTextureID != 0) mImage.ClearCaches();
      mMutex.UnLock();
      return true;
    }

    /*!
     * @brief Non-blocking eviction for bulk eviction. NEVER blocks on a busy texture.
     * @param alsoCpu Also drop the CPU cache (else keep it for fast re-upload)
     * @return Bytes freed, or -1 if the texture was busy and skipped
     */
    long long TryEvict(bool alsoCpu);

    /*!
     * @brief Get file path
     * @return File path (may be empty)
     */
    const Path& FilePath() { return mImage.FilePath(); }

    /*
     * Free texture only
     */

    /*!
     * @brief Set target size (SVG only, FreeTexture only)
     * @param width Required width
     * @param height Required height
     * @param force Bypass lowest size control and force a new rasterization for SVG images
     * @note If the texture is already loaded, it is unloaded
     */
    void SetTargetSize(int width, int height, bool force);

    /*!
     * @brief Set file path
     * @param path new file path
     * @note If the texture is already loaded, it is unloaded
     */
    //void SetPath(const Path& path);

    /*
     * Statistics
     */

    /*!
     * @brief Return consumed CPU by this object
     * @return Byte consumed
     */
    [[nodiscard]] int CPUConsummed() const { return mImage.CacheSize(); }

    /*!
     * @brief Return estimated consumed GPU by this object
     * @return Byte consumed
     */
    [[nodiscard]] int GPUConsummed() const { return mGLTextureID != 0 ? mWidth * mHeight * 4 : 0; }

    /*
     * High level drawings
     */

    /*!
     * @brief Draw the current texture using full coordinates, width/height, keeping ratio yes/no
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio, false to stretch
     */
    void Render(int x, int y, int w, int h, bool keepratio, bool tiled, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates & texture width/height
     * @param x X coordinate
     * @param y Y coordinate
     */
    void Render(int x, int y, bool tiled, Renderer::TextureType type = Renderer::TextureType::RGBA) { Render(x, y, mWidth, mHeight, false, tiled, type); }

    /*!
     * @brief Draw the current texture using full coordinates & width/height
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     */
    void Render(int x, int y, int w, int h, bool tiled, Renderer::TextureType type = Renderer::TextureType::RGBA) { Render(x, y, w, h, false, tiled, type); }

    /*!
     * @brief Draw the current texture using full coordinates, width/height, keeping ratio yes/no and a given blending alpha
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio, false to stretch
     * @param alpha Blending alpha
     */
    void Render(int x, int y, int w, int h, bool keepratio, bool tiled, unsigned char alpha, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, texture width/height and a given blending alpha
     * @param x X coordinate
     * @param y Y coordinate
     * @param alpha Blending alpha
     */
    void Render(int x, int y, bool tiled, unsigned char alpha, Renderer::TextureType type = Renderer::TextureType::RGBA) { Render(x, y, mWidth, mHeight, false, tiled, alpha, type); }

    /*!
     * @brief Draw the current texture using full coordinates, width/height and a given blending alpha
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param alpha Blending alpha
     */
    void Render(int x, int y, int w, int h, bool tiled, unsigned char alpha, Renderer::TextureType type = Renderer::TextureType::RGBA) { Render(x, y, w, h, false, tiled, alpha, type); }

    /*!
     * @brief Draw the current texture using full coordinates, width/height, keeping ratio yes/no and a given blending color
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio, false to stretch
     * @param color Blending color
     */
    void Render(int x, int y, int w, int h, bool keepratio, bool tiled, Colors::ColorARGB color, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, width/height, keeping ratio yes/no and a given blending color
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio, false to stretch
     * @param color Blending color
     */
    void Render(int x, int y, int w, int h, bool flipX, bool flipY, bool keepratio, bool tiled, Colors::ColorARGB color, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, texture width/height and a given blending color
     * @param x X coordinate
     * @param y Y coordinate
     * @param color Blending color
     */
    void Render(int x, int y, bool tiled, Colors::ColorARGB color, Renderer::TextureType type = Renderer::TextureType::RGBA) { Render(x, y, mWidth, mHeight, false, tiled, color, type); }

    /*!
     * @brief Draw the current texture using full coordinates, width/height and a given blending color
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param color Blending color
     */
    void Render(int x, int y, int w, int h, bool tiled, Colors::ColorARGB color, Renderer::TextureType type = Renderer::TextureType::RGBA) { Render(x, y, w, h, false, tiled, color, type); }

    /*!
     * @brief Draw the current texture using full coordinates, width/height, ratio & and blending colors
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
    void Render(int x, int y, int w, int h, bool keepratio, bool tiled,
                Colors::ColorARGB topleftcolor,
                Colors::ColorARGB toprightcolor,
                Colors::ColorARGB bottomrightcolor,
                Colors::ColorARGB bottomleftcolor,
                Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, width/height and blending colors
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param topleftcolor Top left color blending
     * @param toprightcolor  Top right color blending
     * @param bottomrightcolor Bottom Right color blending
     * @param bottomleftcolor Bottom Left color blending
     */
    void Render(int x, int y, int w, int h, bool tiled,
                Colors::ColorARGB topleftcolor,
                Colors::ColorARGB toprightcolor,
                Colors::ColorARGB bottomrightcolor,
                Colors::ColorARGB bottomleftcolor,
                Renderer::TextureType type = Renderer::TextureType::RGBA) { Render(x, y, w, h, false, tiled, topleftcolor, toprightcolor, bottomrightcolor, bottomleftcolor, type); }

    /*!
     * @brief Draw the current texture using full coordinates, width/height, ratio,  blending colors & lower reflection
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
    void Render(int x, int y, int w, int h, bool keepratio, bool tiled,
                bool flipOnX, bool flipOnY,
                Colors::ColorARGB topleftcolor,
                Colors::ColorARGB toprightcolor,
                Colors::ColorARGB bottomrightcolor,
                Colors::ColorARGB bottomleftcolor,
                float topAlphaReflection,
                float bottomAlphaReflection,
                Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, width/height,  blending colors & lower reflection
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
    void Render(int x, int y, int w, int h, bool tiled,
                bool flipOnX, bool flipOnY,
                Colors::ColorARGB topleftcolor,
                Colors::ColorARGB toprightcolor,
                Colors::ColorARGB bottomrightcolor,
                Colors::ColorARGB bottomleftcolor,
                float topAlphaReflection,
                float bottomAlphaReflection,
                Renderer::TextureType type = Renderer::TextureType::RGBA) { Render(x, y, w, h, false, tiled, flipOnX, flipOnY, topleftcolor, toprightcolor, bottomrightcolor, bottomleftcolor, topAlphaReflection, bottomAlphaReflection, type); }

  protected:
    //! Resource path
    IImage& mImage;

    //! Texture loading/unloading protection
    Mutex mMutex;

    //! Last use timestamp
    unsigned long long mLastUse;

    //! Load error count
    int mErrorCount;

    //! Reference counter
    std::atomic_int mReferenceCount;

    //! GL Texture id
    TextureHandle mGLTextureID;

    //! Texture width
    int mWidth;
    //! Texture height
    int mHeight;
    //! Target width (SVG)
    int mTargetWidth;
    //! Target height (SVG)
    int mTargetHeight;

    //! Properties
    Properties mProperties;

    //! Status
    std::atomic<enum Status> mStatus;

    //! SVG?
    bool mIsSVG;

    /*!
     * @brief Create and upload content to a GL texture
     * @return Error code
     */
    RenderingError Prepare();

    /*!
     * @brief Set last use to now
     */
    void Touch()
    {
      mLastUse = Tick();
    }

    /*!
     * @brief Load texture from SVG/PNG file and resize to mTargetWidth/mTargetHeight
     * @return Error code
     */
    RenderingError LoadContent();

    //! Increase references
    void IncReference() { ++mReferenceCount; }

    //! Decrease references
    int DecReference() { return --mReferenceCount; }

    /*
     * IUnloadable implementation
     */

    //! Load texture content
    RenderingError Load() final;

    //! Unload texture content
    void Unload() final;

    //! Unload CPU caches
    void UnloadCaches() final;

    /*!
     * @brief Load a PNG file from the given bugger and return raw image
     * @param data Source PNG data
     * @param width Width of the image (output)
     * @param height Height of the image (ouput)
     * @return Raw image data
     */
    static ByteArray DecodeImage(const ByteArray& data, int& width, int& height);

    //! Identify SVG file
    static bool IdentifySVG(const Path& path) { return path.Extension().ToLowerCase() == ".svg"; }

    //! Create image manager regarding file type
    static IImage* CreateImage(const Path& path, IHttpConfiguration& httpConfigure) { return IdentifySVG(path) ? ((IImage*)new Svg(path, httpConfigure)) : ((IImage*)new Png(path, httpConfigure)); }

    friend class Texture;
};

DEFINE_BITFLAG_ENUM(TextureHolder::Properties, int)
