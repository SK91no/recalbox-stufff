//
// Created by bkg2k on 28/07/26.
//

#ifdef USE_VULKAN

#include "RendererVulkan.h"
#include "rendering/vulkan/ShadersSpv.h"
#include <utils/Log.h>
#include <SDL_vulkan.h>
#include <cstring>
#include <cmath>

Transform4x4f RendererVulkan::BuildOrtho(float w, float h)
{
  // Maps (0,0)-(w,h) with the origin top-left onto Vulkan NDC, where X spans
  // -1..1 left to right, Y spans -1..1 TOP to BOTTOM (the opposite of GL), and
  // Z spans 0..1. The GL backends' BuildOrtho differs on the last two points.
  return {
    { 2.f / w,     0.f, 0.f, 0.f },
    {     0.f, 2.f / h, 0.f, 0.f },
    {     0.f,     0.f, 1.f, 0.f },
    {    -1.f,    -1.f, 0.f, 1.f }
  };
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

RendererVulkan::RendererVulkan(int width, int height, bool windowed, RotationType rotation)
  : Renderer(width, height, windowed, rotation)
  , mProjMatrix()
  , mMVMatrix()
{
  mClearColor.color = { { 0.f, 0.f, 0.f, 0.f } };
  Initialize(width, height);
}

// ---------------------------------------------------------------------------
// Context lifecycle
// ---------------------------------------------------------------------------

void RendererVulkan::DoConfigureContext()
{
  // Nothing to pre-set: Vulkan needs no SDL_GL_SetAttribute equivalent.
}

unsigned int RendererVulkan::DoWindowFlags() const
{
  return SDL_WINDOW_VULKAN;
}

bool RendererVulkan::DoCreateContext(SDL_Window* window)
{
  if (!CreateInstance(window)) return false;

  if (SDL_Vulkan_CreateSurface(window, mInstance, &mSurface) != SDL_TRUE)
  {
    { LOG(LogError) << "[Vulkan] SDL_Vulkan_CreateSurface failed: " << SDL_GetError(); }
    return false;
  }

  return PickPhysicalDevice()
      && CreateLogicalDevice()
      && CreateSwapchain()
      && CreateDepthStencil()
      && CreateRenderPass()
      && CreateFramebuffers()
      && CreateCommandsAndSync()
      && CreateBuffers();
}

bool RendererVulkan::CreateInstance(SDL_Window* window)
{
  // The window MUST have been created with SDL_WINDOW_VULKAN (see DoWindowFlags):
  // that flag is what makes SDL load the Vulkan library. Without it this call
  // fails with "Vulkan is not loaded" — verified, not assumed. Passing the window
  // rather than nullptr keeps that dependency explicit instead of relying on
  // load order.
  unsigned int extensionCount = 0;
  if (SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr) != SDL_TRUE)
  {
    { LOG(LogError) << "[Vulkan] SDL_Vulkan_GetInstanceExtensions failed: " << SDL_GetError(); }
    return false;
  }
  Array<const char*> extensions;
  extensions.ExpandTo((int)extensionCount);
  if (SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, (const char**)extensions.BufferReadWrite()) != SDL_TRUE)
  {
    { LOG(LogError) << "[Vulkan] SDL_Vulkan_GetInstanceExtensions (fetch) failed: " << SDL_GetError(); }
    return false;
  }

  VkApplicationInfo app {};
  app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app.pApplicationName = "EmulationStation";
  app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app.pEngineName = "es-core";
  app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  // 1.0 keeps the door open for older ARM drivers; nothing here needs more.
  app.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo info {};
  info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  info.pApplicationInfo = &app;

  #ifdef DEBUG
  // Turn on validation when the layer is installed. It is not shipped on any
  // Recalbox target, so this is strictly a development aid — but without it the
  // object-lifetime and synchronisation rules this backend has to respect are
  // invisible until something crashes on a user's machine.
  static const char* sValidationLayer = "VK_LAYER_KHRONOS_validation";
  unsigned int layerCount = 0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  Array<VkLayerProperties> layers;
  layers.ExpandTo((int)layerCount);
  if (layerCount != 0) vkEnumerateInstanceLayerProperties(&layerCount, layers.BufferReadWrite());

  bool hasValidation = false;
  for (int i = 0; i < (int)layerCount; ++i)
    if (strcmp(layers[i].layerName, sValidationLayer) == 0) { hasValidation = true; break; }

  if (hasValidation)
  {
    info.enabledLayerCount = 1;
    info.ppEnabledLayerNames = &sValidationLayer;
    // The messenger needs this extension; SDL does not ask for it.
    extensions.Add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    { LOG(LogInfo) << "[Vulkan] Validation layer enabled."; }
  }
  else
  { LOG(LogInfo) << "[Vulkan] Validation layer not installed, continuing without it."; }
  #endif

  // Count from the array, not from what SDL reported: DEBUG may have added one.
  info.enabledExtensionCount = (unsigned int)extensions.Count();
  info.ppEnabledExtensionNames = extensions.BufferReadOnly();

  if (vkCreateInstance(&info, nullptr, &mInstance) != VK_SUCCESS)
  {
    { LOG(LogError) << "[Vulkan] vkCreateInstance failed"; }
    return false;
  }

  #ifdef DEBUG
  if (hasValidation) CreateDebugMessenger();
  #endif
  return true;
}

#ifdef DEBUG

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* userData)
{
  (void)type;
  (void)userData;
  if (data == nullptr || data->pMessage == nullptr) return VK_FALSE;

  if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
  { LOG(LogError) << "[Vulkan/validation] " << data->pMessage; }
  else if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0)
  { LOG(LogWarning) << "[Vulkan/validation] " << data->pMessage; }
  else
  { LOG(LogInfo) << "[Vulkan/validation] " << data->pMessage; }

  // Always false: returning true would abort the call that triggered the
  // message, which is only meant for testing the layer itself.
  return VK_FALSE;
}

void RendererVulkan::CreateDebugMessenger()
{
  // Extension entry points are not exported by the loader; they must be resolved.
  auto create = (PFN_vkCreateDebugUtilsMessengerEXT)
                vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");
  if (create == nullptr) return;

  VkDebugUtilsMessengerCreateInfoEXT info {};
  info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                   | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                   | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  info.pfnUserCallback = VulkanDebugCallback;

  if (create(mInstance, &info, nullptr, &mDebugMessenger) != VK_SUCCESS)
  { LOG(LogWarning) << "[Vulkan] Could not install the validation messenger"; }
}

void RendererVulkan::DestroyDebugMessenger() noexcept
{
  if (mDebugMessenger == VK_NULL_HANDLE || mInstance == VK_NULL_HANDLE) return;
  auto destroy = (PFN_vkDestroyDebugUtilsMessengerEXT)
                 vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
  if (destroy != nullptr) destroy(mInstance, mDebugMessenger, nullptr);
  mDebugMessenger = VK_NULL_HANDLE;
}

#else

void RendererVulkan::CreateDebugMessenger() {}
void RendererVulkan::DestroyDebugMessenger() noexcept {}

#endif

bool RendererVulkan::PickPhysicalDevice()
{
  unsigned int count = 0;
  vkEnumeratePhysicalDevices(mInstance, &count, nullptr);
  if (count == 0)
  {
    { LOG(LogError) << "[Vulkan] No Vulkan-capable device found"; }
    return false;
  }
  Array<VkPhysicalDevice> devices;
  devices.ExpandTo((int)count);
  vkEnumeratePhysicalDevices(mInstance, &count, devices.BufferReadWrite());

  // Prefer a discrete GPU, else take the first device that can present.
  int chosen = -1;
  for (int i = 0; i < (int)count; ++i)
  {
    unsigned int families = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &families, nullptr);
    Array<VkQueueFamilyProperties> props;
    props.ExpandTo((int)families);
    vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &families, props.BufferReadWrite());

    for (int f = 0; f < (int)families; ++f)
    {
      VkBool32 present = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], (unsigned int)f, mSurface, &present);
      if (((props[f].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) && present == VK_TRUE)
      {
        VkPhysicalDeviceProperties dp {};
        vkGetPhysicalDeviceProperties(devices[i], &dp);
        if (chosen < 0 || dp.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
          chosen = i;
          mQueueFamily = (unsigned int)f;
          mDeviceProperties = dp;
        }
        break;
      }
    }
  }

  if (chosen < 0)
  {
    { LOG(LogError) << "[Vulkan] No device with a graphics+present queue"; }
    return false;
  }

  mPhysical = devices[chosen];
  { LOG(LogInfo) << "[Vulkan] Device: " << mDeviceProperties.deviceName; }
  return true;
}

bool RendererVulkan::CreateLogicalDevice()
{
  float priority = 1.f;
  VkDeviceQueueCreateInfo queue {};
  queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue.queueFamilyIndex = mQueueFamily;
  queue.queueCount = 1;
  queue.pQueuePriorities = &priority;

  const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

  VkPhysicalDeviceFeatures features {};

  VkDeviceCreateInfo info {};
  info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  info.queueCreateInfoCount = 1;
  info.pQueueCreateInfos = &queue;
  info.enabledExtensionCount = 1;
  info.ppEnabledExtensionNames = deviceExtensions;
  info.pEnabledFeatures = &features;

  if (vkCreateDevice(mPhysical, &info, nullptr, &mDevice) != VK_SUCCESS)
  {
    { LOG(LogError) << "[Vulkan] vkCreateDevice failed"; }
    return false;
  }
  vkGetDeviceQueue(mDevice, mQueueFamily, 0, &mQueue);
  return true;
}

bool RendererVulkan::CreateSwapchain()
{
  VkSurfaceCapabilitiesKHR caps {};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysical, mSurface, &caps);

  // Pick a format. B8G8R8A8_UNORM is the near-universal presentation format;
  // it concerns the swapchain only, never our textures (see VULKAN.md).
  unsigned int formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysical, mSurface, &formatCount, nullptr);
  if (formatCount == 0)
  {
    { LOG(LogError) << "[Vulkan] The surface reports no format at all"; }
    return false;
  }
  Array<VkSurfaceFormatKHR> formats;
  formats.ExpandTo((int)formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysical, mSurface, &formatCount, formats.BufferReadWrite());

  VkSurfaceFormatKHR chosen = formats[0];
  for (int i = 0; i < (int)formatCount; ++i)
    if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
        formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    { chosen = formats[i]; break; }

  mSwapFormat = chosen.format;
  // mViewportSize is private to Renderer; the public accessors expose the same
  // physical size, which is what the surface fallback needs.
  mExtent = caps.currentExtent.width != 0xFFFFFFFFu
          ? caps.currentExtent
          : VkExtent2D { (unsigned int)RealDisplayWidthAsInt(), (unsigned int)RealDisplayHeightAsInt() };

  // The fallback size comes from our own configuration and owes the surface
  // nothing, so clamp it. Out-of-range extents are a validation error and some
  // drivers simply fail the create.
  if (mExtent.width  < caps.minImageExtent.width)  mExtent.width  = caps.minImageExtent.width;
  if (mExtent.height < caps.minImageExtent.height) mExtent.height = caps.minImageExtent.height;
  if (mExtent.width  > caps.maxImageExtent.width)  mExtent.width  = caps.maxImageExtent.width;
  if (mExtent.height > caps.maxImageExtent.height) mExtent.height = caps.maxImageExtent.height;

  // A minimised window reports 0x0. There is nothing to create and nothing to
  // draw into; say so quietly rather than letting vkCreateSwapchainKHR fail and
  // log on every frame of the retry loop.
  if (mExtent.width == 0 || mExtent.height == 0) return false;

  unsigned int images = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && images > caps.maxImageCount) images = caps.maxImageCount;

  VkSwapchainCreateInfoKHR info {};
  info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  info.surface = mSurface;
  info.minImageCount = images;
  info.imageFormat = mSwapFormat;
  info.imageColorSpace = chosen.colorSpace;
  info.imageExtent = mExtent;
  info.imageArrayLayers = 1;
  info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  info.preTransform = caps.currentTransform;
  info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  // FIFO is the only mode guaranteed present, and it is what vsync means here.
  info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  info.clipped = VK_TRUE;
  info.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(mDevice, &info, nullptr, &mSwapchain) != VK_SUCCESS)
  {
    { LOG(LogError) << "[Vulkan] vkCreateSwapchainKHR failed"; }
    return false;
  }

  unsigned int count = 0;
  vkGetSwapchainImagesKHR(mDevice, mSwapchain, &count, nullptr);
  mSwapImages.Clear();
  mSwapImages.ExpandTo((int)count);
  vkGetSwapchainImagesKHR(mDevice, mSwapchain, &count, mSwapImages.BufferReadWrite());

  mSwapViews.Clear();
  for (int i = 0; i < (int)count; ++i)
  {
    VkImageViewCreateInfo view {};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = mSwapImages[i];
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = mSwapFormat;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.layerCount = 1;

    VkImageView created = VK_NULL_HANDLE;
    if (vkCreateImageView(mDevice, &view, nullptr, &created) != VK_SUCCESS)
    {
      { LOG(LogError) << "[Vulkan] vkCreateImageView failed"; }
      return false;
    }
    mSwapViews.Add(created);
  }

  // One render-finished semaphore per image, and one slot to remember which
  // frame fence last touched each image. Both belong to the swapchain because
  // the image count is only known here and changes when it is rebuilt.
  VkSemaphoreCreateInfo semaphore {};
  semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  mRenderFinished.Clear();
  mImagesInFlight.Clear();
  for (int i = 0; i < (int)count; ++i)
  {
    VkSemaphore created = VK_NULL_HANDLE;
    if (vkCreateSemaphore(mDevice, &semaphore, nullptr, &created) != VK_SUCCESS)
    {
      { LOG(LogError) << "[Vulkan] vkCreateSemaphore failed for swapchain image " << i; }
      return false;
    }
    mRenderFinished.Add(created);
    mImagesInFlight.Add(VK_NULL_HANDLE);
  }

  { LOG(LogInfo) << "[Vulkan] Swapchain: " << (int)mExtent.width << 'x' << (int)mExtent.height
                 << ", " << (int)count << " images"; }
  return true;
}

bool RendererVulkan::CreateDepthStencil()
{
  // The frontend needs a stencil for round clipping; depth comes along for free.
  const VkFormat candidates[] = { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT };
  mDepthFormat = VK_FORMAT_UNDEFINED;
  for (VkFormat candidate : candidates)
  {
    VkFormatProperties props {};
    vkGetPhysicalDeviceFormatProperties(mPhysical, candidate, &props);
    if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)
    { mDepthFormat = candidate; break; }
  }
  if (mDepthFormat == VK_FORMAT_UNDEFINED)
  {
    { LOG(LogError) << "[Vulkan] No usable depth/stencil format"; }
    return false;
  }

  VkImageCreateInfo image {};
  image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image.imageType = VK_IMAGE_TYPE_2D;
  image.format = mDepthFormat;
  image.extent = { mExtent.width, mExtent.height, 1 };
  image.mipLevels = 1;
  image.arrayLayers = 1;
  image.samples = VK_SAMPLE_COUNT_1_BIT;
  image.tiling = VK_IMAGE_TILING_OPTIMAL;
  image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  if (vkCreateImage(mDevice, &image, nullptr, &mDepthImage) != VK_SUCCESS) return false;

  VkMemoryRequirements requirements {};
  vkGetImageMemoryRequirements(mDevice, mDepthImage, &requirements);
  int type = FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (type < 0) return false;

  VkMemoryAllocateInfo alloc {};
  alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc.allocationSize = requirements.size;
  alloc.memoryTypeIndex = (unsigned int)type;
  if (vkAllocateMemory(mDevice, &alloc, nullptr, &mDepthMemory) != VK_SUCCESS) return false;
  vkBindImageMemory(mDevice, mDepthImage, mDepthMemory, 0);

  VkImageViewCreateInfo view {};
  view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view.image = mDepthImage;
  view.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view.format = mDepthFormat;
  view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  view.subresourceRange.levelCount = 1;
  view.subresourceRange.layerCount = 1;
  return vkCreateImageView(mDevice, &view, nullptr, &mDepthView) == VK_SUCCESS;
}

bool RendererVulkan::CreateRenderPass()
{
  VkAttachmentDescription attachments[2] {};
  // Colour
  attachments[0].format = mSwapFormat;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  // Depth + stencil
  attachments[1].format = mDepthFormat;
  attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;   // depth is never tested
  attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  // The stencil IS stored, unlike the depth. Leaving the offscreen pass and
  // resuming this one reloads the attachment, and a clip mask that was active
  // when the excursion started has to still be there afterwards. Eight bits per
  // pixel once per frame is a fair price for that.
  attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorRef { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
  VkAttachmentReference depthRef { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

  VkSubpassDescription subpass {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;
  subpass.pDepthStencilAttachment = &depthRef;

  VkSubpassDependency dependency {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo info {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = 2;
  info.pAttachments = attachments;
  info.subpassCount = 1;
  info.pSubpasses = &subpass;
  info.dependencyCount = 1;
  info.pDependencies = &dependency;

  if (vkCreateRenderPass(mDevice, &info, nullptr, &mRenderPass) != VK_SUCCESS)
  {
    { LOG(LogError) << "[Vulkan] vkCreateRenderPass failed"; }
    return false;
  }
  return true;
}

bool RendererVulkan::CreateFramebuffers()
{
  mFramebuffers.Clear();
  for (int i = 0; i < mSwapViews.Count(); ++i)
  {
    VkImageView views[2] = { mSwapViews[i], mDepthView };

    VkFramebufferCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = mRenderPass;
    info.attachmentCount = 2;
    info.pAttachments = views;
    info.width = mExtent.width;
    info.height = mExtent.height;
    info.layers = 1;

    VkFramebuffer created = VK_NULL_HANDLE;
    if (vkCreateFramebuffer(mDevice, &info, nullptr, &created) != VK_SUCCESS)
    {
      { LOG(LogError) << "[Vulkan] vkCreateFramebuffer failed"; }
      return false;
    }
    mFramebuffers.Add(created);
  }
  return true;
}

bool RendererVulkan::CreateCommandsAndSync()
{
  VkCommandPoolCreateInfo pool {};
  pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool.queueFamilyIndex = mQueueFamily;
  if (vkCreateCommandPool(mDevice, &pool, nullptr, &mCommandPool) != VK_SUCCESS) return false;

  VkCommandBufferAllocateInfo alloc {};
  alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc.commandPool = mCommandPool;
  alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc.commandBufferCount = sFramesInFlight;
  if (vkAllocateCommandBuffers(mDevice, &alloc, mCommands) != VK_SUCCESS) return false;

  VkSemaphoreCreateInfo semaphore {};
  semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fence {};
  fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  // Only the acquire semaphore and the frame fence are per frame slot. The
  // render-finished semaphore is per swapchain image and is created with the
  // swapchain — see mRenderFinished.
  for (int i = 0; i < sFramesInFlight; ++i)
    if (vkCreateSemaphore(mDevice, &semaphore, nullptr, &mImageAvailable[i]) != VK_SUCCESS ||
        vkCreateFence(mDevice, &fence, nullptr, &mInFlight[i]) != VK_SUCCESS)
      return false;

  // Upload command buffer and fence, allocated once and reused by RunOneShot.
  alloc.commandBufferCount = 1;
  if (vkAllocateCommandBuffers(mDevice, &alloc, &mUploadCommand) != VK_SUCCESS) return false;
  // Created unsignalled: RunOneShot resets it before every submit anyway.
  VkFenceCreateInfo uploadFence {};
  uploadFence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (vkCreateFence(mDevice, &uploadFence, nullptr, &mUploadFence) != VK_SUCCESS) return false;

  return true;
}

bool RendererVulkan::CreateBuffers()
{
  for (int i = 0; i < sFramesInFlight; ++i)
  {
    if (!CreateBuffer(sVertexRingSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      mVertexBuffer[i], mVertexMemory[i]))
      return false;
    if (vkMapMemory(mDevice, mVertexMemory[i], 0, sVertexRingSize, 0, &mVertexMapped[i]) != VK_SUCCESS)
      return false;
  }

  if (!CreateBuffer(sStagingSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    mStaging, mStagingMemory))
    return false;
  return vkMapMemory(mDevice, mStagingMemory, 0, sStagingSize, 0, &mStagingMapped) == VK_SUCCESS;
}

bool RendererVulkan::DoInitContext()
{
  return CreateShaderModules()
      && CreateSamplers()
      && CreateDescriptorInfrastructure();
}

bool RendererVulkan::CreateShaderModules()
{
  struct { const uint32_t* Code; size_t Size; VkShaderModule* Target; } modules[] =
  {
    { kColorVertSpv,   sizeof(kColorVertSpv),   &mColorVert },
    { kColorFragSpv,   sizeof(kColorFragSpv),   &mColorFrag },
    { kTextureVertSpv, sizeof(kTextureVertSpv), &mTexVert   },
    { kTextureFragSpv, sizeof(kTextureFragSpv), &mTexFrag   },
    { kAlphaFragSpv,   sizeof(kAlphaFragSpv),   &mAlphaFrag },
  };

  for (auto& entry : modules)
  {
    VkShaderModuleCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = entry.Size;
    info.pCode = entry.Code;
    if (vkCreateShaderModule(mDevice, &info, nullptr, entry.Target) != VK_SUCCESS)
    {
      { LOG(LogError) << "[Vulkan] vkCreateShaderModule failed"; }
      return false;
    }
  }
  return true;
}

bool RendererVulkan::CreateSamplers()
{
  for (int nearest = 0; nearest < 2; ++nearest)
    for (int tiled = 0; tiled < 2; ++tiled)
    {
      VkSamplerCreateInfo info {};
      info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
      info.magFilter = nearest != 0 ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
      info.minFilter = info.magFilter;
      info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
      VkSamplerAddressMode mode = tiled != 0 ? VK_SAMPLER_ADDRESS_MODE_REPEAT
                                             : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      info.addressModeU = info.addressModeV = info.addressModeW = mode;
      info.maxLod = 0.f;
      info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

      if (vkCreateSampler(mDevice, &info, nullptr, &mSamplers[nearest][tiled]) != VK_SUCCESS)
        return false;
    }
  return true;
}

bool RendererVulkan::CreateDescriptorInfrastructure()
{
  VkDescriptorSetLayoutBinding binding {};
  binding.binding = 0;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.descriptorCount = 1;
  binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layout {};
  layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout.bindingCount = 1;
  layout.pBindings = &binding;
  if (vkCreateDescriptorSetLayout(mDevice, &layout, nullptr, &mSetLayout) != VK_SUCCESS) return false;

  // One set per live texture. The frontend caps texture count well below this.
  VkDescriptorPoolSize size {};
  size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  size.descriptorCount = 4096;

  VkDescriptorPoolCreateInfo pool {};
  pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool.maxSets = 4096;
  pool.poolSizeCount = 1;
  pool.pPoolSizes = &size;
  if (vkCreateDescriptorPool(mDevice, &pool, nullptr, &mDescriptorPool) != VK_SUCCESS) return false;

  VkPushConstantRange push {};
  push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  push.offset = 0;
  push.size = sizeof(float) * 16;

  VkPipelineLayoutCreateInfo pipelineLayout {};
  pipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayout.setLayoutCount = 1;
  pipelineLayout.pSetLayouts = &mSetLayout;
  pipelineLayout.pushConstantRangeCount = 1;
  pipelineLayout.pPushConstantRanges = &push;
  return vkCreatePipelineLayout(mDevice, &pipelineLayout, nullptr, &mPipelineLayout) == VK_SUCCESS;
}

// ---------------------------------------------------------------------------
// Teardown
// ---------------------------------------------------------------------------

void RendererVulkan::DestroySwapchain() noexcept
{
  if (mDevice == VK_NULL_HANDLE) return;

  for (int i = 0; i < mFramebuffers.Count(); ++i) vkDestroyFramebuffer(mDevice, mFramebuffers[i], nullptr);
  mFramebuffers.Clear();
  for (int i = 0; i < mSwapViews.Count(); ++i) vkDestroyImageView(mDevice, mSwapViews[i], nullptr);
  mSwapViews.Clear();
  mSwapImages.Clear();

  // Safe here: every caller waits for the device to go idle first, so no
  // present is left waiting on one of these.
  for (int i = 0; i < mRenderFinished.Count(); ++i) vkDestroySemaphore(mDevice, mRenderFinished[i], nullptr);
  mRenderFinished.Clear();
  mImagesInFlight.Clear();

  if (mDepthView != VK_NULL_HANDLE)   { vkDestroyImageView(mDevice, mDepthView, nullptr); mDepthView = VK_NULL_HANDLE; }
  if (mDepthImage != VK_NULL_HANDLE)  { vkDestroyImage(mDevice, mDepthImage, nullptr); mDepthImage = VK_NULL_HANDLE; }
  if (mDepthMemory != VK_NULL_HANDLE) { vkFreeMemory(mDevice, mDepthMemory, nullptr); mDepthMemory = VK_NULL_HANDLE; }

  if (mSwapchain != VK_NULL_HANDLE) { vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr); mSwapchain = VK_NULL_HANDLE; }
}

void RendererVulkan::DoFinalizeContext() noexcept
{
  if (mDevice == VK_NULL_HANDLE) return;
  vkDeviceWaitIdle(mDevice);

  DestroyAllTextures();
  // The device is idle, so anything still waiting on a frame slot can go now.
  // This must run while the descriptor pool is still alive.
  DrainAllRetired();

  for (auto& entry : mPipelines) vkDestroyPipeline(mDevice, entry.second, nullptr);
  mPipelines.clear();

  if (mPipelineLayout != VK_NULL_HANDLE) { vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr); mPipelineLayout = VK_NULL_HANDLE; }
  if (mDescriptorPool != VK_NULL_HANDLE) { vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr); mDescriptorPool = VK_NULL_HANDLE; }
  if (mSetLayout != VK_NULL_HANDLE)      { vkDestroyDescriptorSetLayout(mDevice, mSetLayout, nullptr); mSetLayout = VK_NULL_HANDLE; }

  for (int nearest = 0; nearest < 2; ++nearest)
    for (int tiled = 0; tiled < 2; ++tiled)
      if (mSamplers[nearest][tiled] != VK_NULL_HANDLE)
      { vkDestroySampler(mDevice, mSamplers[nearest][tiled], nullptr); mSamplers[nearest][tiled] = VK_NULL_HANDLE; }

  VkShaderModule* modules[] = { &mColorVert, &mColorFrag, &mTexVert, &mTexFrag, &mAlphaFrag };
  for (VkShaderModule* module : modules)
    if (*module != VK_NULL_HANDLE) { vkDestroyShaderModule(mDevice, *module, nullptr); *module = VK_NULL_HANDLE; }
}

void RendererVulkan::DoDestroyContext() noexcept
{
  if (mDevice == VK_NULL_HANDLE) return;
  vkDeviceWaitIdle(mDevice);

  for (int i = 0; i < sFramesInFlight; ++i)
  {
    if (mVertexMapped[i] != nullptr) { vkUnmapMemory(mDevice, mVertexMemory[i]); mVertexMapped[i] = nullptr; }
    if (mVertexBuffer[i] != VK_NULL_HANDLE) { vkDestroyBuffer(mDevice, mVertexBuffer[i], nullptr); mVertexBuffer[i] = VK_NULL_HANDLE; }
    if (mVertexMemory[i] != VK_NULL_HANDLE) { vkFreeMemory(mDevice, mVertexMemory[i], nullptr); mVertexMemory[i] = VK_NULL_HANDLE; }
    if (mImageAvailable[i] != VK_NULL_HANDLE) { vkDestroySemaphore(mDevice, mImageAvailable[i], nullptr); mImageAvailable[i] = VK_NULL_HANDLE; }
    if (mInFlight[i] != VK_NULL_HANDLE) { vkDestroyFence(mDevice, mInFlight[i], nullptr); mInFlight[i] = VK_NULL_HANDLE; }
  }

  if (mUploadFence != VK_NULL_HANDLE) { vkDestroyFence(mDevice, mUploadFence, nullptr); mUploadFence = VK_NULL_HANDLE; }
  mUploadCommand = VK_NULL_HANDLE;   // freed with the pool below

  if (mStagingMapped != nullptr) { vkUnmapMemory(mDevice, mStagingMemory); mStagingMapped = nullptr; }
  if (mStaging != VK_NULL_HANDLE) { vkDestroyBuffer(mDevice, mStaging, nullptr); mStaging = VK_NULL_HANDLE; }
  if (mStagingMemory != VK_NULL_HANDLE) { vkFreeMemory(mDevice, mStagingMemory, nullptr); mStagingMemory = VK_NULL_HANDLE; }

  if (mCommandPool != VK_NULL_HANDLE) { vkDestroyCommandPool(mDevice, mCommandPool, nullptr); mCommandPool = VK_NULL_HANDLE; }

  DestroySwapchain();
  if (mRenderPass != VK_NULL_HANDLE) { vkDestroyRenderPass(mDevice, mRenderPass, nullptr); mRenderPass = VK_NULL_HANDLE; }

  vkDestroyDevice(mDevice, nullptr);
  mDevice = VK_NULL_HANDLE;

  if (mSurface != VK_NULL_HANDLE) { vkDestroySurfaceKHR(mInstance, mSurface, nullptr); mSurface = VK_NULL_HANDLE; }
  // Before the instance: the messenger belongs to it.
  DestroyDebugMessenger();
  if (mInstance != VK_NULL_HANDLE) { vkDestroyInstance(mInstance, nullptr); mInstance = VK_NULL_HANDLE; }
}

void RendererVulkan::DestroyAllTextures() noexcept
{
  for (auto& entry : mTextures)
  {
    VulkanTexture& texture = entry.second;
    if (texture.View != VK_NULL_HANDLE)   vkDestroyImageView(mDevice, texture.View, nullptr);
    if (texture.Image != VK_NULL_HANDLE)  vkDestroyImage(mDevice, texture.Image, nullptr);
    if (texture.Memory != VK_NULL_HANDLE) vkFreeMemory(mDevice, texture.Memory, nullptr);
  }
  mTextures.clear();
}

#endif // USE_VULKAN
