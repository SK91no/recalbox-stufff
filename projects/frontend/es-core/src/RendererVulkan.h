//
// Created by bkg2k on 28/07/26.
//
#pragma once

#ifdef USE_VULKAN

#include "Renderer.h"
#include <vulkan/vulkan.h>
#include <utils/storage/Array.h>
#include <utils/storage/HashMap.h>
#include <functional>

/*!
 * @brief EXPERIMENTAL Vulkan renderer.
 *
 * Derives from Renderer directly, not from RendererGLCommon: that separation is
 * the whole point of the API neutralization work (see VULKAN.md). Never selected
 * automatically — only when the build is configured with -DFORCE_VULKAN=ON.
 *
 * Design notes, because Vulkan does not map onto the immediate-mode API for free:
 *
 *  - **Lazy frame.** The Renderer API has no BeginFrame(); drawing may start at
 *    any point after a present. The first draw of a frame acquires a swapchain
 *    image and opens the render pass; SwapBuffers() closes and presents it.
 *
 *  - **One vertex format.** GL fed positions, colours and texcoords from separate
 *    client arrays. Here everything is interleaved into VertexData and streamed
 *    through a per-frame host-visible ring buffer. The colour-only shaders simply
 *    ignore the texcoord attribute.
 *
 *  - **Pipelines are cached, not enumerated.** Every (shader, blend, topology,
 *    stencil) combination the frontend actually uses is built on first use and
 *    kept. Blending cannot be changed dynamically the way glBlendFunc does.
 *
 *  - **One descriptor set per texture**, allocated when the texture is created.
 *    This is what makes per-draw texture binding cheap enough.
 */
class RendererVulkan : public Renderer
{
  public:
    /*!
     * @brief Build the Vulkan renderer and initialize the surface + device.
     * @param width    Desired physical viewport width
     * @param height   Desired physical viewport height
     * @param windowed True for windowed mode, false for fullscreen
     * @param rotation Initial viewport rotation
     */
    RendererVulkan(int width, int height, bool windowed, RotationType rotation = RotationType::None);

  protected:
    // ---- context lifecycle -------------------------------------------------
    void DoConfigureContext() override;
    [[nodiscard]] unsigned int DoWindowFlags() const override;
    bool DoCreateContext(SDL_Window* window) override;
    void DoDestroyContext() noexcept override;
    [[nodiscard]] bool DoHasContext() const override { return mDevice != VK_NULL_HANDLE; }
    void DoPresent(SDL_Window* window) override;
    bool DoInitContext() override;
    void DoFinalizeContext() noexcept override;

    // ---- projection --------------------------------------------------------
    void DoBeginProjection(int w, int h) override;
    void DoApplyRotationToProjection(RotationType rotation, int viewW, int viewH) override;
    void DoEndProjection(float sx, float sy) override;
    void DoSetMatrix(const Transform4x4f& transform) override;

    // ---- drawing -----------------------------------------------------------
    void DoDrawColorRect(int x, int y, int w, int h, Colors::ColorARGB color, BlendMode blend) override;
    void DoDrawColorRect4(int x, int y, int w, int h,
                          Colors::ColorARGB tl, Colors::ColorARGB tr,
                          Colors::ColorARGB br, Colors::ColorARGB bl) override;
    void DoDrawColorFan(const float* verts, int count, unsigned int color) override;
    void DoDrawColorTriangles(const Vector2f* verts, const ColorByte* colors, int count) override;
    void DoWriteStencilFan(const float* verts, int count) override;
    void DoDrawLines(const Vector2f* coords, const Colors::ColorARGB* colors, int count) override;
    void DoDrawLinesUniform(const Vector2f* coords, Colors::ColorARGB color, int count) override;
    void DoDrawTTWithColors(const Vertex* verts, const ColorByte* colors, int count, TextureType type) override;
    void DoDrawTTUniform(const Vertex* verts, Colors::ColorARGB color, int count, TextureType type) override;
    void DoSubmitTexQuadUniform(const Vertex* verts, int count,
                                float r, float g, float b, float a, TextureType type) override;
    void DoSubmitTexQuadColors(const Vertex* verts, const Colors::ColorRGBA* colors,
                               int count, TextureType type) override;
    void DoSubmitTexQuadColorsTwoDraws(const Vertex* v1, const Colors::ColorRGBA* c1,
                                       const Vertex* v2, const Colors::ColorRGBA* c2,
                                       int count, TextureType type) override;

    // ---- textures ----------------------------------------------------------
    TextureHandle DoCreateTexture(bool nearest) override;
    void DoDestroyTexture(TextureHandle id) override;
    void DoBindTexture(TextureHandle id) override;
    void DoSetTextureTiling(bool tiled) override;
    RenderingError DoUploadTexture(TextureHandle id, int width, int height,
                                   const void* data, TextureType type) override;
    RenderingError DoUploadTexturePart(TextureHandle id, int x, int y, int width, int height,
                                       const void* data, TextureType type) override;

    // ---- render state ------------------------------------------------------
    void DoEnableBlend(BlendMode blend) override;
    void DoDisableBlend() override;
    void DoSetScissor(int x, int y, int w, int h) override;
    void DoDisableScissor() override;
    void DoClearStencil() override;
    void DoSetViewport(int x, int y, int w, int h) override;
    void DoSetClearColor(float r, float g, float b, float a) override;
    void DoClearBuffers() override;
    String DoGetDriverInfo(DriverInfo info) override;
    void DoActivateDebug() override;

    // ---- offscreen ---------------------------------------------------------
    bool DoCreateOffscreen(int w, int h, TextureHandle& texture) override;
    void DoDestroyOffscreen(TextureHandle& texture) noexcept override;
    void DoBeginOffScreen(int w, int h) override;
    void DoEndOffScreen(int w, int h) override;

  private:
    //! Frames recorded ahead of the GPU
    static constexpr int sFramesInFlight = 2;
    //! Size of each frame's vertex ring buffer
    static constexpr VkDeviceSize sVertexRingSize = 4 * 1024 * 1024;
    //! Size of the texture upload staging buffer
    static constexpr VkDeviceSize sStagingSize = 8 * 1024 * 1024;

    //! Interleaved vertex fed to every pipeline. Colour-only shaders ignore UV.
    struct VertexData
    {
      float X, Y;        //!< Position
      unsigned int Color;//!< Packed colour, GPU byte order (R,G,B,A)
      float U, V;        //!< Texture coordinates
    };

    //! Which shader pair a draw uses
    enum class ShaderKind { Color, Texture, Alpha };

    //! How a pipeline interacts with the stencil buffer
    enum class StencilMode { Off, Write, Test };

    //! Everything backing one texture handle
    struct VulkanTexture
    {
      VkImage         Image  = VK_NULL_HANDLE;
      VkDeviceMemory  Memory = VK_NULL_HANDLE;
      VkImageView     View   = VK_NULL_HANDLE;
      VkDescriptorSet Set    = VK_NULL_HANDLE;
      int             Width  = 0;
      int             Height = 0;
      bool            Nearest = false;
      bool            Tiled   = false;
      TextureType     Type   = TextureType::RGBA;
    };

    // ---- instance / device -------------------------------------------------
    VkInstance       mInstance    = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysical    = VK_NULL_HANDLE;
    VkDevice         mDevice      = VK_NULL_HANDLE;
    VkSurfaceKHR     mSurface     = VK_NULL_HANDLE;
    VkQueue          mQueue       = VK_NULL_HANDLE;
    unsigned int     mQueueFamily = 0;
    VkPhysicalDeviceProperties mDeviceProperties {};

    //! Validation messenger, DEBUG builds only, and only when the layer is
    //! actually installed on the machine. Validation is the tool that catches
    //! object-lifetime and synchronisation mistakes in this backend, so being
    //! unable to switch it on was itself a defect.
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;

    // ---- swapchain ---------------------------------------------------------
    VkSwapchainKHR      mSwapchain   = VK_NULL_HANDLE;
    VkFormat            mSwapFormat  = VK_FORMAT_UNDEFINED;
    VkExtent2D          mExtent      { 0, 0 };
    Array<VkImage>      mSwapImages;
    Array<VkImageView>  mSwapViews;
    Array<VkFramebuffer> mFramebuffers;
    VkRenderPass        mRenderPass  = VK_NULL_HANDLE;

    //! Depth+stencil attachment. The frontend uses the stencil for round clipping.
    VkImage        mDepthImage  = VK_NULL_HANDLE;
    VkDeviceMemory mDepthMemory = VK_NULL_HANDLE;
    VkImageView    mDepthView   = VK_NULL_HANDLE;
    VkFormat       mDepthFormat = VK_FORMAT_UNDEFINED;

    // ---- offscreen target --------------------------------------------------
    //
    // Rendering elsewhere means a second render pass, and a pipeline is only
    // valid in passes *compatible* with the one it was built against. So the
    // offscreen colour attachment deliberately uses the swapchain format and
    // the shared depth format: same count, same formats, same sample count, so
    // the whole pipeline cache works in both passes unchanged.
    //
    // Leaving the main pass to draw offscreen and coming back also needs a
    // third pass, identical to the main one but loading the framebuffer instead
    // of clearing it — re-entering the main pass would wipe everything drawn
    // before the offscreen section.
    VkRenderPass   mOffscreenPass        = VK_NULL_HANDLE;
    VkRenderPass   mResumePass           = VK_NULL_HANDLE;
    VkFramebuffer  mOffscreenFramebuffer = VK_NULL_HANDLE;
    VkImage        mOffscreenDepthImage  = VK_NULL_HANDLE;
    VkDeviceMemory mOffscreenDepthMemory = VK_NULL_HANDLE;
    VkImageView    mOffscreenDepthView   = VK_NULL_HANDLE;
    VkExtent2D     mOffscreenExtent      { 0, 0 };
    //! Handle of the colour texture the offscreen pass renders into
    TextureHandle  mOffscreenHandle      = InvalidTextureHandle;
    //! True between DoBeginOffScreen and DoEndOffScreen
    bool           mInOffscreen          = false;

    // ---- command recording & synchronisation -------------------------------
    VkCommandPool   mCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer mCommands[sFramesInFlight] {};
    VkSemaphore     mImageAvailable[sFramesInFlight] {};
    VkFence         mInFlight[sFramesInFlight] {};

    //! Render-finished semaphore, one per SWAPCHAIN IMAGE rather than per frame
    //! slot. vkQueuePresentKHR waits on it, and a present is only known to be
    //! over once its image comes back out of vkAcquireNextImageKHR — which the
    //! frame fence says nothing about. With more images than frame slots, an
    //! index by slot lets a pending present share a semaphore with the next
    //! submission, which the spec forbids. Owned by the swapchain: rebuilt with
    //! it, so never touched while a present is outstanding.
    Array<VkSemaphore> mRenderFinished;

    //! For each swapchain image, the fence of the frame slot that last submitted
    //! work for it, or VK_NULL_HANDLE. Non-owning: the fences live in mInFlight.
    //! Waiting on it before recording keeps us off an image the GPU still holds.
    Array<VkFence> mImagesInFlight;

    int             mFrame       = 0;
    unsigned int    mImageIndex  = 0;
    bool            mFrameStarted = false;
    bool            mSwapchainLost = false;

    //! One set of Vulkan objects whose destruction has been deferred.
    struct RetiredResource
    {
      VkImage         Image  = VK_NULL_HANDLE;
      VkDeviceMemory  Memory = VK_NULL_HANDLE;
      VkImageView     View   = VK_NULL_HANDLE;
      VkDescriptorSet Set    = VK_NULL_HANDLE;
      int             Frame  = 0;   //!< Frame slot that was current when retired
    };

    //! Resources waiting for their frame slot to come round again.
    //!
    //! Destroying a texture the moment the frontend asks is unsafe: the command
    //! buffer of the frame being recorded may already carry a bind or a draw
    //! that names it, and vkDeviceWaitIdle does not help — it waits on work that
    //! was *submitted*, and this command buffer has not been. TextureManager
    //! evicts textures from inside the render loop (see NewFont's glyph upload
    //! retry), so this is the nominal path, not an edge case.
    Array<RetiredResource> mRetired;

    // ---- vertex streaming --------------------------------------------------
    VkBuffer       mVertexBuffer[sFramesInFlight] {};
    VkDeviceMemory mVertexMemory[sFramesInFlight] {};
    void*          mVertexMapped[sFramesInFlight] {};
    VkDeviceSize   mVertexUsed = 0;

    // ---- texture upload staging --------------------------------------------
    VkBuffer       mStaging       = VK_NULL_HANDLE;
    VkDeviceMemory mStagingMemory = VK_NULL_HANDLE;
    void*          mStagingMapped = nullptr;

    //! Command buffer and fence reused by every one-shot upload.
    //!
    //! These used to be allocated and freed around each upload, and the wait was
    //! vkQueueWaitIdle — which drains the whole queue, rendering included, not
    //! just the copy. A font atlas is filled one glyph at a time, so that ran
    //! hundreds of times while drawing a single screen of text.
    VkCommandBuffer mUploadCommand = VK_NULL_HANDLE;
    VkFence         mUploadFence   = VK_NULL_HANDLE;

    // ---- pipelines ---------------------------------------------------------
    VkShaderModule       mColorVert = VK_NULL_HANDLE;
    VkShaderModule       mColorFrag = VK_NULL_HANDLE;
    VkShaderModule       mTexVert   = VK_NULL_HANDLE;
    VkShaderModule       mTexFrag   = VK_NULL_HANDLE;
    VkShaderModule       mAlphaFrag = VK_NULL_HANDLE;
    VkPipelineLayout     mPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout mSetLayout     = VK_NULL_HANDLE;
    VkDescriptorPool     mDescriptorPool = VK_NULL_HANDLE;
    //! Pipeline cache keyed by PipelineKey()
    HashMap<int, VkPipeline> mPipelines;

    //! Samplers indexed by [nearest][tiled]
    VkSampler mSamplers[2][2] {};

    // ---- textures ----------------------------------------------------------
    HashMap<TextureHandle, VulkanTexture> mTextures;
    //! Handles are ours to mint: Vulkan has no equivalent of GL object names
    TextureHandle mNextHandle = 1;

    //! Reusable staging area for the vertices of one draw.
    //!
    //! Eight of the eleven primitives used to build a local Array here, which
    //! meant a malloc, a zeroing pass and a free on every single draw call, on
    //! the render thread. One member that only ever grows removes all of that:
    //! after a few frames it is already as big as the largest draw the UI ever
    //! issues. Only ever touched between building and submitting one draw, so
    //! successive draws cannot tread on each other.
    Array<VertexData> mScratchVertices;

    // ---- current state -----------------------------------------------------
    Transform4x4f mProjMatrix;
    Transform4x4f mMVMatrix;
    TextureHandle mBoundTexture = InvalidTextureHandle;
    BlendMode     mBlend        = BlendMode::AlphaBlend;
    StencilMode   mStencil      = StencilMode::Off;
    VkRect2D      mScissor      { { 0, 0 }, { 0, 0 } };
    bool          mScissorSet   = false;
    VkClearValue  mClearColor   {};
    VkPipeline    mCurrentPipeline = VK_NULL_HANDLE;

    // ---- setup helpers -----------------------------------------------------
    /*!
     * @brief Create the Vulkan instance with the extensions SDL requires.
     * @param window The window the surface will be created on. Passing it (rather
     *        than nullptr) is what lets SDL report the extensions without relying
     *        on the Vulkan library having already been loaded — which only happens
     *        as a side effect of SDL_WINDOW_VULKAN.
     * @return true on success
     */
    bool CreateInstance(SDL_Window* window);

    //! @brief Install the validation messenger. No-op outside a DEBUG build.
    void CreateDebugMessenger();
    //! @brief Remove the validation messenger. No-op outside a DEBUG build.
    void DestroyDebugMessenger() noexcept;

    bool PickPhysicalDevice();
    bool CreateLogicalDevice();
    bool CreateSwapchain();
    void DestroySwapchain() noexcept;
    bool CreateRenderPass();
    bool CreateDepthStencil();
    bool CreateFramebuffers();
    bool CreateCommandsAndSync();
    bool CreateBuffers();
    bool CreateDescriptorInfrastructure();
    bool CreateSamplers();
    bool CreateShaderModules();

    /*!
     * @brief Rebuild everything tied to the surface size after a resize or a
     *        VK_ERROR_OUT_OF_DATE_KHR.
     * @return true on success
     */
    bool RecreateSwapchain();

    // ---- frame helpers -----------------------------------------------------
    /*!
     * @brief Acquire an image and open the render pass, unless already done.
     * @return true when a frame is recording and draws may proceed
     */
    bool BeginFrameIfNeeded();

    /*!
     * @brief Close the render pass, submit and present.
     * @return true on success
     */
    bool EndFrame();

    // ---- drawing helpers ---------------------------------------------------
    /*!
     * @brief Copy vertices into the current frame's ring buffer.
     * @param vertices Source vertices
     * @param count Vertex count
     * @param offset Receives the byte offset to bind at
     * @return true when the ring had room
     */
    bool PushVertices(const VertexData* vertices, int count, VkDeviceSize& offset);

    /*!
     * @brief Build the cache key for a pipeline variant.
     * @param shader Shader pair
     * @param blend Blending equation
     * @param topology Primitive topology
     * @param stencil Stencil interaction
     * @return Packed key
     */
    static int PipelineKey(ShaderKind shader, BlendMode blend, VkPrimitiveTopology topology, StencilMode stencil);

    /*!
     * @brief Fetch or build a pipeline for the given combination.
     * @return Pipeline handle, VK_NULL_HANDLE on failure
     */
    VkPipeline GetPipeline(ShaderKind shader, BlendMode blend, VkPrimitiveTopology topology, StencilMode stencil);

    /*!
     * @brief Record one draw: bind pipeline, texture and vertices, then draw.
     * @param shader Shader pair to use
     * @param topology Primitive topology
     * @param vertices Vertices to stream
     * @param count Vertex count
     */
    void Draw(ShaderKind shader, VkPrimitiveTopology topology, const VertexData* vertices, int count);

    /*!
     * @brief Push the combined projection * model-view matrix as a push constant.
     */
    void PushMatrix();

    /*!
     * @brief Orthographic projection for Vulkan clip space.
     *        Y points DOWN in Vulkan NDC where it points up in GL, and depth
     *        runs 0..1 instead of -1..1 — hence a dedicated builder rather than
     *        the GL backends' BuildOrtho.
     * @param w Virtual viewport width
     * @param h Virtual viewport height
     * @return Projection matrix
     */
    static Transform4x4f BuildOrtho(float w, float h);

    // ---- memory helpers ----------------------------------------------------
    /*!
     * @brief Find a memory type index satisfying the given requirements.
     * @param typeBits Allowed types bitmask from vkGetXxxMemoryRequirements
     * @param properties Required memory property flags
     * @return Memory type index, or -1 when none matches
     */
    int FindMemoryType(unsigned int typeBits, VkMemoryPropertyFlags properties) const;

    /*!
     * @brief Allocate and bind a buffer.
     * @return true on success
     */
    bool CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                      VkBuffer& buffer, VkDeviceMemory& memory);

    /*!
     * @brief Run a one-shot command buffer on the queue and wait for it.
     * @param recorder Callback recording the commands
     * @return true when the commands were submitted and completed. A false here
     *         means the caller's copy never reached the GPU, so it must not
     *         report success — silently losing an upload leaves a texture that
     *         samples as garbage with nothing in the log.
     */
    bool RunOneShot(const std::function<void(VkCommandBuffer)>& recorder);

    /*!
     * @brief Move an image between layouts inside a one-shot command buffer.
     */
    static void TransitionLayout(VkCommandBuffer cmd, VkImage image,
                                 VkImageLayout from, VkImageLayout to);

    //! @brief Release every texture still alive
    void DestroyAllTextures() noexcept;

    // ---- deferred destruction ----------------------------------------------
    /*!
     * @brief Hand a set of objects over to the retirement queue instead of
     *        destroying them now. Any of them may be VK_NULL_HANDLE.
     *        They are freed once the frame slot recording at this moment has
     *        been through a full fence wait, which proves the GPU is done with
     *        whatever commands named them.
     */
    void RetireResource(VkImage image, VkDeviceMemory memory, VkImageView view, VkDescriptorSet set);

    /*!
     * @brief Destroy everything retired while @p frame was the recording slot.
     *        Call only right after waiting on that slot's fence.
     * @param frame Frame slot whose fence has just been waited on
     */
    void DrainRetired(int frame);

    /*!
     * @brief Destroy the whole retirement queue. Requires an idle device.
     */
    void DrainAllRetired() noexcept;

    /*!
     * @brief Allocate a descriptor set and point it at a texture's view with the
     *        sampler matching its nearest/tiled flags.
     *
     * Always a fresh set, never an update in place: rewriting a set that the
     * command buffer being recorded has already bound is undefined without the
     * update-after-bind feature, which is not enabled here.
     * @param texture Texture to describe; its Set field is replaced on success
     * @return true on success
     */
    bool AllocateDescriptorSet(VulkanTexture& texture);

    // ---- offscreen helpers -------------------------------------------------
    /*!
     * @brief Build the offscreen and resume render passes.
     *        Both are compatible with mRenderPass so the pipeline cache is shared.
     * @return true on success
     */
    bool CreateOffscreenPasses();

    /*!
     * @brief Release everything backing the offscreen target. Assumes the caller
     *        has made the device idle, or is happy to go through the retirement
     *        queue for the texture.
     */
    void DestroyOffscreenTarget() noexcept;

    /*!
     * @brief Set the dynamic viewport and scissor for the pass now current.
     * @param extent Size of the attachment being rendered into
     */
    void ApplyViewportAndScissor(VkExtent2D extent);

    /*!
     * @brief Signal a frame fence from an empty submission.
     *
     * Used on every error path that has already reset the fence: leaving it
     * unsignalled makes the next vkWaitForFences on that slot block forever,
     * which reads as a total application freeze rather than a dropped frame.
     * @param frame Frame slot whose fence must end up signalled
     */
    void SignalFenceEmpty(int frame);
};

#endif // USE_VULKAN
