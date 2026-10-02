//
// Created by bkg2k on 28/07/26.
//
// Frame lifecycle, pipelines, drawing, textures and render state for the
// Vulkan backend. Split from RendererVulkan.cpp, which holds device and
// swapchain setup, purely to keep both files readable.
//

#ifdef USE_VULKAN

#include "RendererVulkan.h"
#include <utils/Log.h>
#include <cstring>

// ---------------------------------------------------------------------------
// Memory helpers
// ---------------------------------------------------------------------------

int RendererVulkan::FindMemoryType(unsigned int typeBits, VkMemoryPropertyFlags properties) const
{
  VkPhysicalDeviceMemoryProperties memory {};
  vkGetPhysicalDeviceMemoryProperties(mPhysical, &memory);
  for (unsigned int i = 0; i < memory.memoryTypeCount; ++i)
    if (((typeBits & (1u << i)) != 0) &&
        (memory.memoryTypes[i].propertyFlags & properties) == properties)
      return (int)i;
  return -1;
}

bool RendererVulkan::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                  VkMemoryPropertyFlags properties,
                                  VkBuffer& buffer, VkDeviceMemory& memory)
{
  VkBufferCreateInfo info {};
  info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  info.size = size;
  info.usage = usage;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateBuffer(mDevice, &info, nullptr, &buffer) != VK_SUCCESS) return false;

  VkMemoryRequirements requirements {};
  vkGetBufferMemoryRequirements(mDevice, buffer, &requirements);
  int type = FindMemoryType(requirements.memoryTypeBits, properties);
  if (type < 0) return false;

  VkMemoryAllocateInfo alloc {};
  alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc.allocationSize = requirements.size;
  alloc.memoryTypeIndex = (unsigned int)type;
  if (vkAllocateMemory(mDevice, &alloc, nullptr, &memory) != VK_SUCCESS) return false;

  return vkBindBufferMemory(mDevice, buffer, memory, 0) == VK_SUCCESS;
}

bool RendererVulkan::RunOneShot(const std::function<void(VkCommandBuffer)>& recorder)
{
  if (mUploadCommand == VK_NULL_HANDLE || mUploadFence == VK_NULL_HANDLE) return false;

  vkResetCommandBuffer(mUploadCommand, 0);

  VkCommandBufferBeginInfo begin {};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (vkBeginCommandBuffer(mUploadCommand, &begin) != VK_SUCCESS) return false;
  recorder(mUploadCommand);
  if (vkEndCommandBuffer(mUploadCommand) != VK_SUCCESS) return false;

  vkResetFences(mDevice, 1, &mUploadFence);

  VkSubmitInfo submit {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &mUploadCommand;
  if (vkQueueSubmit(mQueue, 1, &submit, mUploadFence) != VK_SUCCESS) return false;

  // Waiting on this submission's own fence, not on the whole queue. The staging
  // buffer is a single shared allocation, so the copy has to have read it before
  // the next upload overwrites it — but there is no reason to also wait for
  // every frame already in flight, which vkQueueWaitIdle did.
  //
  // Truly asynchronous uploads would need a staging ring and a transfer command
  // buffer batched per frame; this keeps the single-buffer invariant intact.
  return vkWaitForFences(mDevice, 1, &mUploadFence, VK_TRUE, UINT64_MAX) == VK_SUCCESS;
}

void RendererVulkan::TransitionLayout(VkCommandBuffer cmd, VkImage image,
                                      VkImageLayout from, VkImageLayout to)
{
  VkImageMemoryBarrier barrier {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = from;
  barrier.newLayout = to;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags source = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags destination = VK_PIPELINE_STAGE_TRANSFER_BIT;

  if (from == VK_IMAGE_LAYOUT_UNDEFINED && to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  }
  else if (from == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && to == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    source = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (from == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    source = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    destination = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }

  vkCmdPipelineBarrier(cmd, source, destination, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

// ---------------------------------------------------------------------------
// Deferred destruction
//
// Vulkan objects may not be destroyed while a command buffer that names them is
// still pending — and a command buffer being *recorded* counts, because it will
// be submitted before anything else runs. vkDeviceWaitIdle does not cover that
// case: it waits on submitted work only. Everything therefore goes through the
// retirement queue and is freed one full frame-slot cycle later, when that
// slot's fence has been waited on and its commands are provably done.
// ---------------------------------------------------------------------------

void RendererVulkan::RetireResource(VkImage image, VkDeviceMemory memory, VkImageView view, VkDescriptorSet set)
{
  if (image == VK_NULL_HANDLE && memory == VK_NULL_HANDLE &&
      view == VK_NULL_HANDLE && set == VK_NULL_HANDLE) return;

  RetiredResource retired;
  retired.Image  = image;
  retired.Memory = memory;
  retired.View   = view;
  retired.Set    = set;
  retired.Frame  = mFrame;
  mRetired.Add(retired);
}

void RendererVulkan::DrainRetired(int frame)
{
  if (mRetired.Empty()) return;

  // Compact in place: entries belonging to other slots stay.
  int kept = 0;
  for (int i = 0; i < mRetired.Count(); ++i)
  {
    const RetiredResource& retired = mRetired[i];
    if (retired.Frame != frame) { mRetired(kept++) = retired; continue; }

    if (retired.Set != VK_NULL_HANDLE)    vkFreeDescriptorSets(mDevice, mDescriptorPool, 1, &retired.Set);
    if (retired.View != VK_NULL_HANDLE)   vkDestroyImageView(mDevice, retired.View, nullptr);
    if (retired.Image != VK_NULL_HANDLE)  vkDestroyImage(mDevice, retired.Image, nullptr);
    if (retired.Memory != VK_NULL_HANDLE) vkFreeMemory(mDevice, retired.Memory, nullptr);
  }
  mRetired.TruncateTo(kept);
}

void RendererVulkan::DrainAllRetired() noexcept
{
  if (mDevice == VK_NULL_HANDLE) { mRetired.Clear(); return; }

  for (int i = 0; i < mRetired.Count(); ++i)
  {
    const RetiredResource& retired = mRetired[i];
    if (retired.Set != VK_NULL_HANDLE && mDescriptorPool != VK_NULL_HANDLE)
      vkFreeDescriptorSets(mDevice, mDescriptorPool, 1, &retired.Set);
    if (retired.View != VK_NULL_HANDLE)   vkDestroyImageView(mDevice, retired.View, nullptr);
    if (retired.Image != VK_NULL_HANDLE)  vkDestroyImage(mDevice, retired.Image, nullptr);
    if (retired.Memory != VK_NULL_HANDLE) vkFreeMemory(mDevice, retired.Memory, nullptr);
  }
  mRetired.Clear();
}

bool RendererVulkan::AllocateDescriptorSet(VulkanTexture& texture)
{
  if (texture.View == VK_NULL_HANDLE) return false;

  VkDescriptorSetAllocateInfo setAlloc {};
  setAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  setAlloc.descriptorPool = mDescriptorPool;
  setAlloc.descriptorSetCount = 1;
  setAlloc.pSetLayouts = &mSetLayout;

  VkDescriptorSet set = VK_NULL_HANDLE;
  if (vkAllocateDescriptorSets(mDevice, &setAlloc, &set) != VK_SUCCESS)
  {
    { LOG(LogError) << "[Vulkan] vkAllocateDescriptorSets failed (descriptor pool exhausted?)"; }
    return false;
  }

  VkDescriptorImageInfo descriptor {};
  descriptor.sampler = mSamplers[texture.Nearest ? 1 : 0][texture.Tiled ? 1 : 0];
  descriptor.imageView = texture.View;
  descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkWriteDescriptorSet write {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = set;
  write.dstBinding = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo = &descriptor;
  vkUpdateDescriptorSets(mDevice, 1, &write, 0, nullptr);

  // The previous set may be bound in the command buffer being recorded, so it
  // is retired rather than freed.
  RetireResource(VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, texture.Set);
  texture.Set = set;
  return true;
}

void RendererVulkan::SignalFenceEmpty(int frame)
{
  // A submission with no command buffers is legal and exists precisely for
  // this: it signals the fence without asking the GPU to do anything.
  vkQueueSubmit(mQueue, 0, nullptr, mInFlight[frame]);
}

// ---------------------------------------------------------------------------
// Frame lifecycle
// ---------------------------------------------------------------------------

bool RendererVulkan::RecreateSwapchain()
{
  vkDeviceWaitIdle(mDevice);
  DestroySwapchain();
  if (!CreateSwapchain() || !CreateDepthStencil() || !CreateFramebuffers()) return false;
  mSwapchainLost = false;
  return true;
}

bool RendererVulkan::BeginFrameIfNeeded()
{
  if (mFrameStarted) return true;
  if (mDevice == VK_NULL_HANDLE) return false;

  if (mSwapchainLost && !RecreateSwapchain())
  {
    static bool once = false; if (!once) { once = true; LOG(LogError) << "[Vulkan] BeginFrame: swapchain recreation failed"; }
    return false;
  }

  vkWaitForFences(mDevice, 1, &mInFlight[mFrame], VK_TRUE, UINT64_MAX);

  // This slot's previous submission is complete, so whatever was retired while
  // it was recording can finally go.
  DrainRetired(mFrame);

  VkResult acquired = vkAcquireNextImageKHR(mDevice, mSwapchain, UINT64_MAX,
                                            mImageAvailable[mFrame], VK_NULL_HANDLE, &mImageIndex);
  if (acquired == VK_ERROR_OUT_OF_DATE_KHR)
  {
    if (!RecreateSwapchain()) return false;
    acquired = vkAcquireNextImageKHR(mDevice, mSwapchain, UINT64_MAX,
                                     mImageAvailable[mFrame], VK_NULL_HANDLE, &mImageIndex);
  }
  if (acquired != VK_SUCCESS && acquired != VK_SUBOPTIMAL_KHR)
  {
    static bool once = false; if (!once) { once = true; LOG(LogError) << "[Vulkan] BeginFrame: vkAcquireNextImageKHR failed (" << (int)acquired << ')'; }
    return false;
  }
  if ((int)mImageIndex >= mRenderFinished.Count())
  {
    { LOG(LogError) << "[Vulkan] BeginFrame: image index " << (int)mImageIndex << " outside the swapchain"; }
    return false;
  }

  // There are usually three images for two frame slots, so the image just
  // acquired may still be owned by an older submission. Wait on the fence that
  // one used before recording over it, then claim the image for this slot.
  if (VkFence owner = mImagesInFlight[(int)mImageIndex]; owner != VK_NULL_HANDLE)
    vkWaitForFences(mDevice, 1, &owner, VK_TRUE, UINT64_MAX);
  mImagesInFlight((int)mImageIndex) = mInFlight[mFrame];

  vkResetFences(mDevice, 1, &mInFlight[mFrame]);
  vkResetCommandBuffer(mCommands[mFrame], 0);

  VkCommandBufferBeginInfo begin {};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (vkBeginCommandBuffer(mCommands[mFrame], &begin) != VK_SUCCESS)
  {
    static bool once = false; if (!once) { once = true; LOG(LogError) << "[Vulkan] BeginFrame: vkBeginCommandBuffer failed"; }
    // The fence was reset just above and nothing will ever signal it now.
    // Without this the next visit to this slot blocks forever on the wait at
    // the top, which looks like a hang rather than a dropped frame.
    SignalFenceEmpty(mFrame);
    return false;
  }

  VkClearValue clears[2];
  clears[0] = mClearColor;
  clears[1].depthStencil = { 1.f, 0 };

  VkRenderPassBeginInfo pass {};
  pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  pass.renderPass = mRenderPass;
  pass.framebuffer = mFramebuffers[(int)mImageIndex];
  pass.renderArea.offset = { 0, 0 };
  pass.renderArea.extent = mExtent;
  pass.clearValueCount = 2;
  pass.pClearValues = clears;
  vkCmdBeginRenderPass(mCommands[mFrame], &pass, VK_SUBPASS_CONTENTS_INLINE);

  // Viewport and scissor are dynamic: they change many times per frame.
  mFrameStarted = true;      // ApplyViewportAndScissor records into this frame
  ApplyViewportAndScissor(mExtent);

  mVertexUsed = 0;
  mCurrentPipeline = VK_NULL_HANDLE;
  mInOffscreen = false;      // a new frame always starts on the main pass
  return true;
}

bool RendererVulkan::EndFrame()
{
  if (!mFrameStarted) return true;

  // Any failure below must still leave the frame closed, the fence signalled
  // and the slot advanced, or the next frame deadlocks on a fence nothing will
  // ever signal. That is what this lambda guarantees on every early exit.
  auto abandonFrame = [this]()
  {
    SignalFenceEmpty(mFrame);
    mImagesInFlight((int)mImageIndex) = VK_NULL_HANDLE;
    mFrameStarted = false;
    mSwapchainLost = true;   // rebuild rather than trust the current state
    mFrame = (mFrame + 1) % sFramesInFlight;
  };

  // Held in a local: both the submit and the present point at it, and it must
  // stay put for the duration of both calls.
  VkSemaphore renderFinished = mRenderFinished[(int)mImageIndex];

  vkCmdEndRenderPass(mCommands[mFrame]);
  if (vkEndCommandBuffer(mCommands[mFrame]) != VK_SUCCESS)
  {
    static bool once = false; if (!once) { once = true; LOG(LogError) << "[Vulkan] EndFrame: vkEndCommandBuffer failed"; }
    abandonFrame();
    return false;
  }

  VkPipelineStageFlags wait = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = &mImageAvailable[mFrame];
  submit.pWaitDstStageMask = &wait;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &mCommands[mFrame];
  submit.signalSemaphoreCount = 1;
  // Signalled per image, not per slot: the present below waits on it and the
  // frame fence says nothing about when a present completes.
  submit.pSignalSemaphores = &renderFinished;
  if (VkResult r = vkQueueSubmit(mQueue, 1, &submit, mInFlight[mFrame]); r != VK_SUCCESS)
  {
    static bool once = false; if (!once) { once = true; LOG(LogError) << "[Vulkan] EndFrame: vkQueueSubmit failed (" << (int)r << ')'; }
    abandonFrame();
    return false;
  }

  VkPresentInfoKHR present {};
  present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present.waitSemaphoreCount = 1;
  present.pWaitSemaphores = &renderFinished;
  present.swapchainCount = 1;
  present.pSwapchains = &mSwapchain;
  present.pImageIndices = &mImageIndex;

  VkResult presented = vkQueuePresentKHR(mQueue, &present);
  if (presented == VK_ERROR_OUT_OF_DATE_KHR || presented == VK_SUBOPTIMAL_KHR)
    mSwapchainLost = true;

  mFrameStarted = false;
  mFrame = (mFrame + 1) % sFramesInFlight;
  return true;
}

void RendererVulkan::DoPresent(SDL_Window* window)
{
  (void)window; // presentation goes through the swapchain, not SDL
  // A frame with no draw at all still needs to be presented, or the screen
  // would freeze on whatever was shown last.
  if (!mFrameStarted) BeginFrameIfNeeded();
  EndFrame();
}

void RendererVulkan::DoClearBuffers()
{
  // Clearing is the render pass' loadOp; nothing to do outside a frame.
}

// ---------------------------------------------------------------------------
// Pipelines
// ---------------------------------------------------------------------------

int RendererVulkan::PipelineKey(ShaderKind shader, BlendMode blend,
                                VkPrimitiveTopology topology, StencilMode stencil)
{
  return ((int)shader) | ((int)blend << 4) | ((int)topology << 8) | ((int)stencil << 16);
}

VkPipeline RendererVulkan::GetPipeline(ShaderKind shader, BlendMode blend,
                                       VkPrimitiveTopology topology, StencilMode stencil)
{
  int key = PipelineKey(shader, blend, topology, stencil);
  if (VkPipeline* existing = mPipelines.try_get(key); existing != nullptr) return *existing;

  VkPipelineShaderStageCreateInfo stages[2] {};
  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = shader == ShaderKind::Color ? mColorVert : mTexVert;
  stages[0].pName = "main";
  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = shader == ShaderKind::Color ? mColorFrag
                   : shader == ShaderKind::Alpha ? mAlphaFrag : mTexFrag;
  stages[1].pName = "main";

  VkVertexInputBindingDescription binding {};
  binding.binding = 0;
  binding.stride = sizeof(VertexData);
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  // One vertex layout for every pipeline. Colour-only shaders simply do not
  // declare the texcoord input; unconsumed attributes are legal.
  VkVertexInputAttributeDescription attributes[3] {};
  attributes[0] = { 0, 0, VK_FORMAT_R32G32_SFLOAT,  (unsigned int)offsetof(VertexData, X) };
  attributes[1] = { 1, 0, VK_FORMAT_R8G8B8A8_UNORM, (unsigned int)offsetof(VertexData, Color) };
  attributes[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT,  (unsigned int)offsetof(VertexData, U) };

  VkPipelineVertexInputStateCreateInfo vertexInput {};
  vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInput.vertexBindingDescriptionCount = 1;
  vertexInput.pVertexBindingDescriptions = &binding;
  vertexInput.vertexAttributeDescriptionCount = shader == ShaderKind::Color ? 2 : 3;
  vertexInput.pVertexAttributeDescriptions = attributes;

  VkPipelineInputAssemblyStateCreateInfo assembly {};
  assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  assembly.topology = topology;

  VkPipelineViewportStateCreateInfo viewport {};
  viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport.viewportCount = 1;
  viewport.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo raster {};
  raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  raster.polygonMode = VK_POLYGON_MODE_FILL;
  raster.cullMode = VK_CULL_MODE_NONE;   // 2D: winding is not controlled
  raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  raster.lineWidth = 1.f;

  VkPipelineMultisampleStateCreateInfo multisample {};
  multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  // Blending is baked in, which is why BlendMode is part of the pipeline key.
  VkPipelineColorBlendAttachmentState attachment {};
  attachment.blendEnable = VK_TRUE;
  attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  attachment.colorBlendOp = VK_BLEND_OP_ADD;
  attachment.alphaBlendOp = VK_BLEND_OP_ADD;
  switch(blend)
  {
    case BlendMode::InverseMultiply:
      attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
      attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      break;
    case BlendMode::Additive:
      attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
      attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
      attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      break;
    case BlendMode::AlphaBlend:
    default:
      attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
      attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
      attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      break;
  }

  VkPipelineColorBlendStateCreateInfo blending {};
  blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blending.attachmentCount = 1;
  blending.pAttachments = &attachment;

  // Stencil: Write rasterizes the mask with colour writes off, Test keeps
  // subsequent draws inside it. Mirrors DoWriteStencilFan in the GL backends.
  VkPipelineDepthStencilStateCreateInfo depthStencil {};
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_FALSE;
  depthStencil.depthWriteEnable = VK_FALSE;
  depthStencil.stencilTestEnable = stencil == StencilMode::Off ? VK_FALSE : VK_TRUE;
  if (stencil == StencilMode::Write)
  {
    depthStencil.front.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencil.front.passOp = VK_STENCIL_OP_REPLACE;
    depthStencil.front.failOp = VK_STENCIL_OP_KEEP;
    depthStencil.front.depthFailOp = VK_STENCIL_OP_KEEP;
    depthStencil.front.compareMask = 0xFF;
    depthStencil.front.writeMask = 0xFF;
    depthStencil.front.reference = 1;
    attachment.colorWriteMask = 0; // mask only, no colour output
    blending.pAttachments = &attachment;
  }
  else if (stencil == StencilMode::Test)
  {
    depthStencil.front.compareOp = VK_COMPARE_OP_EQUAL;
    depthStencil.front.passOp = VK_STENCIL_OP_KEEP;
    depthStencil.front.failOp = VK_STENCIL_OP_KEEP;
    depthStencil.front.depthFailOp = VK_STENCIL_OP_KEEP;
    depthStencil.front.compareMask = 0xFF;
    depthStencil.front.writeMask = 0;
    depthStencil.front.reference = 1;
  }
  depthStencil.back = depthStencil.front;

  VkDynamicState dynamics[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  VkPipelineDynamicStateCreateInfo dynamic {};
  dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic.dynamicStateCount = 2;
  dynamic.pDynamicStates = dynamics;

  VkGraphicsPipelineCreateInfo info {};
  info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  info.stageCount = 2;
  info.pStages = stages;
  info.pVertexInputState = &vertexInput;
  info.pInputAssemblyState = &assembly;
  info.pViewportState = &viewport;
  info.pRasterizationState = &raster;
  info.pMultisampleState = &multisample;
  info.pDepthStencilState = &depthStencil;
  info.pColorBlendState = &blending;
  info.pDynamicState = &dynamic;
  info.layout = mPipelineLayout;
  info.renderPass = mRenderPass;
  info.subpass = 0;

  VkPipeline pipeline = VK_NULL_HANDLE;
  if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) != VK_SUCCESS)
  {
    { LOG(LogError) << "[Vulkan] vkCreateGraphicsPipelines failed (key " << key << ')'; }
    return VK_NULL_HANDLE;
  }

  mPipelines.insert_unique((int)key, (VkPipeline)pipeline);
  return pipeline;
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

bool RendererVulkan::PushVertices(const VertexData* vertices, int count, VkDeviceSize& offset)
{
  VkDeviceSize size = (VkDeviceSize)count * sizeof(VertexData);
  // Keep each draw's data 16-byte aligned; cheap and avoids surprises.
  VkDeviceSize aligned = (mVertexUsed + 15) & ~(VkDeviceSize)15;
  if (aligned + size > sVertexRingSize)
  {
    { LOG(LogError) << "[Vulkan] Vertex ring exhausted, dropping a draw"; }
    return false;
  }
  memcpy((char*)mVertexMapped[mFrame] + aligned, vertices, (size_t)size);
  offset = aligned;
  mVertexUsed = aligned + size;
  return true;
}

void RendererVulkan::PushMatrix()
{
  // The shaders take a single combined matrix: two mat4 would exactly fill the
  // 128-byte guaranteed push constant budget, leaving nothing spare.
  Transform4x4f mvp = mProjMatrix * mMVMatrix;
  vkCmdPushConstants(mCommands[mFrame], mPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                     0, sizeof(float) * 16, (const void*)&mvp);
}

void RendererVulkan::Draw(ShaderKind shader, VkPrimitiveTopology topology,
                          const VertexData* vertices, int count)
{
  if (count <= 0) return;
  if (!BeginFrameIfNeeded()) return;

  // Resolve the texture BEFORE recording anything. A textured pipeline with no
  // descriptor set bound is undefined behaviour, so a texture that is unknown,
  // or created but never uploaded (DoCreateTexture leaves Set null until the
  // first upload gives it a size), must drop the draw entirely rather than
  // reach vkCmdDraw with nothing bound.
  VulkanTexture* texture = nullptr;
  if (shader != ShaderKind::Color)
  {
    if (mBoundTexture == InvalidTextureHandle) return;
    texture = mTextures.try_get(mBoundTexture);
    if (texture == nullptr || texture->Set == VK_NULL_HANDLE) return;
  }

  VkPipeline pipeline = GetPipeline(shader, mBlend, topology, mStencil);
  if (pipeline == VK_NULL_HANDLE) return;

  VkDeviceSize offset = 0;
  if (!PushVertices(vertices, count, offset)) return;

  VkCommandBuffer cmd = mCommands[mFrame];
  if (pipeline != mCurrentPipeline)
  {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    mCurrentPipeline = pipeline;
  }

  if (texture != nullptr)
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout,
                            0, 1, &texture->Set, 0, nullptr);

  PushMatrix();
  vkCmdBindVertexBuffers(cmd, 0, 1, &mVertexBuffer[mFrame], &offset);
  vkCmdDraw(cmd, (unsigned int)count, 1, 0, 0);
}

void RendererVulkan::DoDrawColorRect(int x, int y, int w, int h, Colors::ColorARGB color, BlendMode blend)
{
  mBlend = blend;
  unsigned int packed = Colors::ToGPUBytes(color);
  VertexData vertices[6] =
  {
    { (float)x,       (float)y,       packed, 0.f, 0.f },
    { (float)x,       (float)(y + h), packed, 0.f, 0.f },
    { (float)(x + w), (float)y,       packed, 0.f, 0.f },
    { (float)(x + w), (float)y,       packed, 0.f, 0.f },
    { (float)x,       (float)(y + h), packed, 0.f, 0.f },
    { (float)(x + w), (float)(y + h), packed, 0.f, 0.f },
  };
  Draw(ShaderKind::Color, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertices, 6);
}

void RendererVulkan::DoDrawColorRect4(int x, int y, int w, int h,
                                      Colors::ColorARGB tl, Colors::ColorARGB tr,
                                      Colors::ColorARGB br, Colors::ColorARGB bl)
{
  mBlend = BlendMode::AlphaBlend;   // matches DoDrawColorRect4 in the GL backends
  unsigned int ptl = Colors::ToGPUBytes(tl), ptr = Colors::ToGPUBytes(tr);
  unsigned int pbr = Colors::ToGPUBytes(br), pbl = Colors::ToGPUBytes(bl);
  VertexData vertices[6] =
  {
    { (float)x,       (float)y,       ptl, 0.f, 0.f },
    { (float)x,       (float)(y + h), pbl, 0.f, 0.f },
    { (float)(x + w), (float)y,       ptr, 0.f, 0.f },
    { (float)(x + w), (float)y,       ptr, 0.f, 0.f },
    { (float)x,       (float)(y + h), pbl, 0.f, 0.f },
    { (float)(x + w), (float)(y + h), pbr, 0.f, 0.f },
  };
  Draw(ShaderKind::Color, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertices, 6);
}

void RendererVulkan::DoDrawColorTriangles(const Vector2f* verts, const ColorByte* colors, int count)
{
  mBlend = BlendMode::AlphaBlend;
  Array<VertexData>& vertices = mScratchVertices;   // reused, never reallocated per draw
  vertices.ExpandTo(count);
  // The colour array is already in GPU byte order, which is exactly what the
  // R8G8B8A8_UNORM vertex attribute expects, so it is copied word for word.
  const unsigned int* packed = (const unsigned int*)colors;
  for (int i = 0; i < count; ++i)
    vertices(i) = { verts[i].x(), verts[i].y(), packed[i], 0.f, 0.f };
  Draw(ShaderKind::Color, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertices.BufferReadOnly(), count);
}

void RendererVulkan::DoDrawColorFan(const float* verts, int count, unsigned int color)
{
  // The GL backends call DoEnableBlend(AlphaBlend) at the top of this primitive.
  // Here blending is baked into the pipeline, so the equivalent is to select it
  // explicitly — otherwise the fan silently inherits whatever mode the last
  // DoDrawColorRect happened to leave behind.
  mBlend = BlendMode::AlphaBlend;
  Array<VertexData>& vertices = mScratchVertices;   // reused, never reallocated per draw
  vertices.ExpandTo(count);
  unsigned int packed = Colors::ToGPUBytes(color);
  for (int i = 0; i < count; ++i)
    vertices(i) = { verts[i * 2], verts[i * 2 + 1], packed, 0.f, 0.f };
  Draw(ShaderKind::Color, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, vertices.BufferReadOnly(), count);
}

void RendererVulkan::DoWriteStencilFan(const float* verts, int count)
{
  Array<VertexData>& vertices = mScratchVertices;   // reused, never reallocated per draw
  vertices.ExpandTo(count);
  for (int i = 0; i < count; ++i)
    vertices(i) = { verts[i * 2], verts[i * 2 + 1], 0xFFFFFFFFu, 0.f, 0.f };

  // The mask replaces the previous one, it does not add to it: the GL backends
  // open with glClear(GL_STENCIL_BUFFER_BIT). Without this, two successive
  // CircleStencil() calls with no ClearStencil() between them would clip to the
  // union of both shapes instead of just the second.
  if (!BeginFrameIfNeeded()) return;
  VkClearAttachment clear {};
  clear.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
  clear.clearValue.depthStencil = { 1.f, 0 };
  VkClearRect rect {};
  rect.rect = { { 0, 0 }, mInOffscreen ? mOffscreenExtent : mExtent };
  rect.layerCount = 1;
  vkCmdClearAttachments(mCommands[mFrame], 1, &clear, 1, &rect);

  mBlend = BlendMode::AlphaBlend;
  mStencil = StencilMode::Write;
  Draw(ShaderKind::Color, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, vertices.BufferReadOnly(), count);
  // Everything drawn from now on is clipped to the mask, until ClearStencil().
  mStencil = StencilMode::Test;
  mCurrentPipeline = VK_NULL_HANDLE;
}

// LINE_LIST, not LINE_STRIP: the GL backends issue GL_LINES, so the caller
// hands over pairs of endpoints forming disjoint segments. A strip would join
// every segment to the next and draw lines nobody asked for.
void RendererVulkan::DoDrawLines(const Vector2f* coords, const Colors::ColorARGB* colors, int count)
{
  mBlend = BlendMode::AlphaBlend;
  Array<VertexData>& vertices = mScratchVertices;   // reused, never reallocated per draw
  vertices.ExpandTo(count);
  for (int i = 0; i < count; ++i)
    vertices(i) = { coords[i].x(), coords[i].y(), Colors::ToGPUBytes(colors[i]), 0.f, 0.f };
  Draw(ShaderKind::Color, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, vertices.BufferReadOnly(), count);
}

void RendererVulkan::DoDrawLinesUniform(const Vector2f* coords, Colors::ColorARGB color, int count)
{
  mBlend = BlendMode::AlphaBlend;
  Array<VertexData>& vertices = mScratchVertices;   // reused, never reallocated per draw
  vertices.ExpandTo(count);
  unsigned int packed = Colors::ToGPUBytes(color);
  for (int i = 0; i < count; ++i)
    vertices(i) = { coords[i].x(), coords[i].y(), packed, 0.f, 0.f };
  Draw(ShaderKind::Color, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, vertices.BufferReadOnly(), count);
}

void RendererVulkan::DoDrawTTWithColors(const Vertex* verts, const ColorByte* colors, int count, TextureType type)
{
  Array<VertexData>& vertices = mScratchVertices;   // reused, never reallocated per draw
  vertices.ExpandTo(count);
  const unsigned int* packed = (const unsigned int*)colors;
  for (int i = 0; i < count; ++i)
    vertices(i) = { verts[i].Target.X, verts[i].Target.Y, packed[i], verts[i].Source.X, verts[i].Source.Y };
  Draw(type == TextureType::Alpha ? ShaderKind::Alpha : ShaderKind::Texture,
       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertices.BufferReadOnly(), count);
}

void RendererVulkan::DoDrawTTUniform(const Vertex* verts, Colors::ColorARGB color, int count, TextureType type)
{
  Array<VertexData>& vertices = mScratchVertices;   // reused, never reallocated per draw
  vertices.ExpandTo(count);
  unsigned int packed = Colors::ToGPUBytes(color);
  for (int i = 0; i < count; ++i)
    vertices(i) = { verts[i].Target.X, verts[i].Target.Y, packed, verts[i].Source.X, verts[i].Source.Y };
  Draw(type == TextureType::Alpha ? ShaderKind::Alpha : ShaderKind::Texture,
       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertices.BufferReadOnly(), count);
}

void RendererVulkan::DoSubmitTexQuadUniform(const Vertex* verts, int count,
                                            float r, float g, float b, float a, TextureType type)
{
  unsigned int packed = ((unsigned int)(r * 255.f) & 0xFF)
                      | (((unsigned int)(g * 255.f) & 0xFF) << 8)
                      | (((unsigned int)(b * 255.f) & 0xFF) << 16)
                      | (((unsigned int)(a * 255.f) & 0xFF) << 24);
  Array<VertexData>& vertices = mScratchVertices;   // reused, never reallocated per draw
  vertices.ExpandTo(count);
  for (int i = 0; i < count; ++i)
    vertices(i) = { verts[i].Target.X, verts[i].Target.Y, packed, verts[i].Source.X, verts[i].Source.Y };
  Draw(type == TextureType::Alpha ? ShaderKind::Alpha : ShaderKind::Texture,
       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertices.BufferReadOnly(), count);
}

void RendererVulkan::DoSubmitTexQuadColors(const Vertex* verts, const Colors::ColorRGBA* colors,
                                           int count, TextureType type)
{
  Array<VertexData>& vertices = mScratchVertices;   // reused, never reallocated per draw
  vertices.ExpandTo(count);
  for (int i = 0; i < count; ++i)
    vertices(i) = { verts[i].Target.X, verts[i].Target.Y, colors[i], verts[i].Source.X, verts[i].Source.Y };
  Draw(type == TextureType::Alpha ? ShaderKind::Alpha : ShaderKind::Texture,
       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertices.BufferReadOnly(), count);
}

void RendererVulkan::DoSubmitTexQuadColorsTwoDraws(const Vertex* v1, const Colors::ColorRGBA* c1,
                                                   const Vertex* v2, const Colors::ColorRGBA* c2,
                                                   int count, TextureType type)
{
  DoSubmitTexQuadColors(v1, c1, count, type);
  DoSubmitTexQuadColors(v2, c2, count, type);
}

// ---------------------------------------------------------------------------
// Projection
// ---------------------------------------------------------------------------

void RendererVulkan::DoBeginProjection(int w, int h)
{
  mProjMatrix = BuildOrtho((float)w, (float)h);
  mMVMatrix = Transform4x4f::Identity();
}

void RendererVulkan::DoApplyRotationToProjection(RotationType rotation, int viewW, int viewH)
{
  switch(rotation)
  {
    case RotationType::Left:
      mProjMatrix.rotateZ((float)(-M_PI / 2)).translate({ -(float)viewH, 0.f, 0.f });
      break;
    case RotationType::Right:
      mProjMatrix.rotateZ((float)(M_PI / 2)).translate({ 0.f, -(float)viewW, 0.f });
      break;
    case RotationType::Upsidedown:
      mProjMatrix.rotateZ((float)M_PI).translate({ -(float)viewW, -(float)viewH, 0.f });
      break;
    case RotationType::None:
    default: break;
  }
}

void RendererVulkan::DoEndProjection(float sx, float sy)
{
  mProjMatrix.scale({ sx, sy, 1.f });
  mMVMatrix = Transform4x4f::Identity();
}

void RendererVulkan::DoSetMatrix(const Transform4x4f& transform)
{
  // round() snaps the translation to whole pixels, exactly as the GL backends
  // do. Without it text and icons land on fractional coordinates and the
  // bilinear sampler smears every glyph edge, which reads as a blurrier UI
  // than GL for no other reason.
  mMVMatrix = Transform4x4f(transform).round();
}

// ---------------------------------------------------------------------------
// Render state
// ---------------------------------------------------------------------------

void RendererVulkan::DoEnableBlend(BlendMode blend)
{
  // Blending lives in the pipeline; just remember which one the next draw needs.
  mBlend = blend;
}

void RendererVulkan::DoDisableBlend()
{
  // Same reasoning as the GL backend: nothing here renders with blending off.
}

void RendererVulkan::DoSetScissor(int x, int y, int w, int h)
{
  // The caller works in GL convention (origin bottom-left); Vulkan's is top-left.
  // The flip is against whichever attachment is being rendered into, which is
  // not the swapchain while an offscreen pass is open.
  VkExtent2D target = mInOffscreen ? mOffscreenExtent : mExtent;
  int top = (int)target.height - (y + h);
  if (top < 0) top = 0;
  if (w < 0) w = 0;
  if (h < 0) h = 0;

  mScissor.offset = { x, top };
  mScissor.extent = { (unsigned int)w, (unsigned int)h };
  mScissorSet = true;
  if (mFrameStarted) vkCmdSetScissor(mCommands[mFrame], 0, 1, &mScissor);
}

void RendererVulkan::DoDisableScissor()
{
  mScissorSet = false;
  if (mFrameStarted)
  {
    VkRect2D full { { 0, 0 }, mInOffscreen ? mOffscreenExtent : mExtent };
    vkCmdSetScissor(mCommands[mFrame], 0, 1, &full);
  }
}

void RendererVulkan::DoClearStencil()
{
  mStencil = StencilMode::Off;
  mCurrentPipeline = VK_NULL_HANDLE;
  if (!mFrameStarted) return;

  // Wipe the mask inside the pass, the way glClear(GL_STENCIL_BUFFER_BIT) did.
  VkClearAttachment clear {};
  clear.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
  clear.clearValue.depthStencil = { 1.f, 0 };
  VkClearRect rect {};
  rect.rect = { { 0, 0 }, mInOffscreen ? mOffscreenExtent : mExtent };
  rect.layerCount = 1;
  vkCmdClearAttachments(mCommands[mFrame], 1, &clear, 1, &rect);
}

void RendererVulkan::DoSetViewport(int x, int y, int w, int h)
{
  (void)x; (void)y; (void)w; (void)h;
  // The viewport always covers the swapchain image; the projection matrix is
  // what maps virtual coordinates onto it.
}

void RendererVulkan::DoSetClearColor(float r, float g, float b, float a)
{
  mClearColor.color = { { r, g, b, a } };
}

String RendererVulkan::DoGetDriverInfo(DriverInfo info)
{
  if (mPhysical == VK_NULL_HANDLE) return {};
  switch(info)
  {
    case DriverInfo::Vendor:
      return String("Vulkan vendor 0x").Append(String().AssignHexa((int)mDeviceProperties.vendorID));
    case DriverInfo::Renderer:
      return String(mDeviceProperties.deviceName);
    case DriverInfo::Version:
    {
      unsigned int version = mDeviceProperties.apiVersion;
      return String("Vulkan ").Append((int)VK_VERSION_MAJOR(version)).Append('.')
                              .Append((int)VK_VERSION_MINOR(version)).Append('.')
                              .Append((int)VK_VERSION_PATCH(version));
    }
    case DriverInfo::Extensions:
    default: return {};
  }
}

void RendererVulkan::DoActivateDebug()
{
  // Nothing to do at this point in the lifecycle. Vulkan's equivalent is the
  // validation layer, which has to be requested when the instance is created,
  // long before this is called — see CreateInstance. It switches itself on in a
  // DEBUG build whenever the layer is installed on the machine, and stays out of
  // the way otherwise, since no Recalbox target ships it.
}

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------

TextureHandle RendererVulkan::DoCreateTexture(bool nearest)
{
  // Storage is allocated by the first upload, which is when the size is known.
  TextureHandle handle = mNextHandle++;
  VulkanTexture texture;
  texture.Nearest = nearest;
  mTextures.insert_unique((TextureHandle)handle, (VulkanTexture)texture);
  return handle;
}

void RendererVulkan::DoDestroyTexture(TextureHandle id)
{
  VulkanTexture* texture = mTextures.try_get(id);
  if (texture == nullptr) return;

  // Deferred, not immediate. TextureManager evicts textures from inside the
  // render loop, so the command buffer being recorded right now may already
  // carry a bind or a draw naming this image. vkDeviceWaitIdle would not save
  // us: that command buffer has not been submitted yet, so there is nothing for
  // it to wait on.
  RetireResource(texture->Image, texture->Memory, texture->View, texture->Set);

  mTextures.erase(id);
  if (mBoundTexture == id) mBoundTexture = InvalidTextureHandle;
}

void RendererVulkan::DoBindTexture(TextureHandle id)
{
  if (id != InvalidTextureHandle) mBoundTexture = id;
}

void RendererVulkan::DoSetTextureTiling(bool tiled)
{
  // Wrap mode is a property of the texture, never a global. A "have we already
  // asked for this?" flag shared by every texture would let the second tiled
  // texture in a row keep the clamped sampler of the first, which shows up as a
  // background that stretches instead of repeating. The GL backend keys its
  // cache on the bound handle for exactly this reason; do the same here.
  VulkanTexture* texture = mTextures.try_get(mBoundTexture);
  if (texture == nullptr || texture->View == VK_NULL_HANDLE) return;
  if (texture->Tiled == tiled) return;

  // The sampler is baked into the descriptor set, so switching wrap mode means
  // pointing the texture at a new set built with the other sampler. Rewriting
  // the existing one in place would be undefined: the command buffer being
  // recorded may already have bound it.
  texture->Tiled = tiled;
  if (!AllocateDescriptorSet(*texture))
  {
    // Out of descriptor sets. Leave the old set in place and put the flag back
    // so the next attempt tries again rather than believing it succeeded.
    texture->Tiled = !tiled;
  }
}

RenderingError RendererVulkan::DoUploadTexture(TextureHandle id, int width, int height,
                                               const void* data, TextureType type)
{
  VulkanTexture* texture = mTextures.try_get(id);
  if (texture == nullptr)
  {
    { LOG(LogError) << "[Vulkan] Upload to unknown texture handle " << (int)id; }
    return RenderingError::NoResource;
  }
  if (width <= 0 || height <= 0)
  {
    { LOG(LogError) << "[Vulkan] Upload with invalid size " << width << 'x' << height; }
    return RenderingError::NoResource;
  }

  // Alpha textures (font atlases) have no alpha-only format in Vulkan, so they
  // become single-channel R8 and the shader samples .r — see alpha.frag.
  VkFormat format = type == TextureType::Alpha ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;

  // Recreate storage whenever the shape changes; glTexImage2D did the same.
  if (texture->Image == VK_NULL_HANDLE || texture->Width != width ||
      texture->Height != height || texture->Type != type)
  {
    // The old image, and the descriptor set pointing at it, may be referenced by
    // the command buffer currently being recorded. Retire them instead of
    // destroying them, and build a fresh set below rather than rewriting one
    // that could already be bound.
    RetireResource(texture->Image, texture->Memory, texture->View, texture->Set);
    texture->View   = VK_NULL_HANDLE;
    texture->Image  = VK_NULL_HANDLE;
    texture->Memory = VK_NULL_HANDLE;
    texture->Set    = VK_NULL_HANDLE;

    VkImageCreateInfo image {};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = format;
    image.extent = { (unsigned int)width, (unsigned int)height, 1 };
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(mDevice, &image, nullptr, &texture->Image) != VK_SUCCESS)
    {
      { LOG(LogError) << "[Vulkan] vkCreateImage failed for " << width << 'x' << height; }
      return RenderingError::OutOfGPUMemory;
    }

    VkMemoryRequirements requirements {};
    vkGetImageMemoryRequirements(mDevice, texture->Image, &requirements);
    int memoryType = FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memoryType < 0)
    {
      { LOG(LogError) << "[Vulkan] No device-local memory type for the texture"; }
      return RenderingError::OutOfGPUMemory;
    }

    VkMemoryAllocateInfo alloc {};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = requirements.size;
    alloc.memoryTypeIndex = (unsigned int)memoryType;
    if (vkAllocateMemory(mDevice, &alloc, nullptr, &texture->Memory) != VK_SUCCESS)
    {
      { LOG(LogError) << "[Vulkan] vkAllocateMemory failed (" << (int)requirements.size << " bytes)"; }
      return RenderingError::OutOfGPUMemory;
    }
    vkBindImageMemory(mDevice, texture->Image, texture->Memory, 0);

    VkImageViewCreateInfo view {};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = texture->Image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = format;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.layerCount = 1;
    if (vkCreateImageView(mDevice, &view, nullptr, &texture->View) != VK_SUCCESS)
    {
      { LOG(LogError) << "[Vulkan] vkCreateImageView failed"; }
      return RenderingError::OutOfGPUMemory;
    }

    // Record the new shape before describing it: a caller that retries after a
    // descriptor failure must not find a texture claiming the old size.
    texture->Width = width;
    texture->Height = height;
    texture->Type = type;

    if (!AllocateDescriptorSet(*texture)) return RenderingError::OutOfGPUMemory;

    // A freshly created image is in UNDEFINED layout. Move it to the sampleable
    // layout unconditionally: DoUploadTexturePart below always transitions
    // *from* SHADER_READ_ONLY_OPTIMAL, so leaving it UNDEFINED when pixels are
    // supplied would make that first transition describe the wrong old layout.
    RunOneShot([&](VkCommandBuffer cmd)
    {
      TransitionLayout(cmd, texture->Image, VK_IMAGE_LAYOUT_UNDEFINED,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
      TransitionLayout(cmd, texture->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });
  }

  if (data == nullptr) return RenderingError::NoError;
  return DoUploadTexturePart(id, 0, 0, width, height, data, type);
}

RenderingError RendererVulkan::DoUploadTexturePart(TextureHandle id, int x, int y, int width, int height,
                                                   const void* data, TextureType type)
{
  // An empty sub-upload is legitimate and MUST succeed: a space glyph has
  // width == rows == 0 *and* a null bitmap buffer. glTexSubImage2D silently
  // no-ops on those. The font cache reads a failure as "out of texture memory",
  // so returning an error here sent NewFont::RenderGlyphInTexture into an
  // endless unload/retry loop. Test this first — the null-data check below
  // would otherwise reject the very same glyph on its pointer instead.
  if (width <= 0 || height <= 0) return RenderingError::NoError;

  VulkanTexture* texture = mTextures.try_get(id);
  if (texture == nullptr || texture->Image == VK_NULL_HANDLE)
  {
    { LOG(LogError) << "[Vulkan] Sub-upload to texture " << (int)id << " with no image allocated"; }
    return RenderingError::NoResource;
  }
  if (data == nullptr)
  {
    { LOG(LogError) << "[Vulkan] Sub-upload with null data (" << width << 'x' << height << ')'; }
    return RenderingError::NoResource;
  }

  int bytesPerPixel = type == TextureType::Alpha ? 1 : 4;
  VkDeviceSize size = (VkDeviceSize)width * height * bytesPerPixel;
  if (size > sStagingSize)
  {
    { LOG(LogError) << "[Vulkan] Upload of " << (int)size << " bytes exceeds the staging buffer"; }
    return RenderingError::OutOfGPUMemory;
  }

  memcpy(mStagingMapped, data, (size_t)size);

  if (!RunOneShot([&](VkCommandBuffer cmd)
  {
    TransitionLayout(cmd, texture->Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy copy {};
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.layerCount = 1;
    copy.imageOffset = { x, y, 0 };
    copy.imageExtent = { (unsigned int)width, (unsigned int)height, 1 };
    vkCmdCopyBufferToImage(cmd, mStaging, texture->Image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    TransitionLayout(cmd, texture->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }))
  {
    { LOG(LogError) << "[Vulkan] Texture sub-upload could not be submitted"; }
    return RenderingError::NoResource;
  }

  return RenderingError::NoError;
}

// ---------------------------------------------------------------------------
// Offscreen target
//
// The frontend has no caller for BeginOffScreen()/EndOffScreen() yet, but the
// Renderer API declares them, so the backend implements them. See the header
// for why the offscreen and resume passes reuse the swapchain and depth formats
// rather than picking their own.
// ---------------------------------------------------------------------------

bool RendererVulkan::CreateOffscreenPasses()
{
  // Shared shape: same attachment count, formats and sample counts as the main
  // pass, which is what "render pass compatible" means and what lets every
  // cached pipeline be bound inside these passes too.
  VkAttachmentDescription attachments[2] {};
  attachments[0].format = mSwapFormat;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[1].format = mDepthFormat;
  attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;

  VkAttachmentReference colorRef { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
  VkAttachmentReference depthRef { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

  VkSubpassDescription subpass {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;
  subpass.pDepthStencilAttachment = &depthRef;

  VkRenderPassCreateInfo info {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = 2;
  info.pAttachments = attachments;
  info.subpassCount = 1;
  info.pSubpasses = &subpass;

  // --- Offscreen pass: clear, then hand the result to the sampler ---
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  // Legal with loadOp CLEAR whatever the image really holds, and it saves the
  // caller from tracking whether this is the first use.
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  // Wait for any sampling of the previous contents before overwriting them, and
  // make the writes visible to the sampler afterwards.
  VkSubpassDependency offscreenDeps[2] {};
  offscreenDeps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  offscreenDeps[0].dstSubpass = 0;
  offscreenDeps[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  offscreenDeps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  offscreenDeps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  offscreenDeps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  offscreenDeps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
  offscreenDeps[1].srcSubpass = 0;
  offscreenDeps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  offscreenDeps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  offscreenDeps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  offscreenDeps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  offscreenDeps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  offscreenDeps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
  info.dependencyCount = 2;
  info.pDependencies = offscreenDeps;

  if (vkCreateRenderPass(mDevice, &info, nullptr, &mOffscreenPass) != VK_SUCCESS)
  {
    { LOG(LogError) << "[Vulkan] vkCreateRenderPass failed for the offscreen pass"; }
    return false;
  }

  // --- Resume pass: same as the main pass, but LOAD instead of CLEAR ---
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // where the main pass left it
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;         // depth is never tested
  attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;       // the clip mask must survive
  attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDependency resumeDep {};
  resumeDep.srcSubpass = VK_SUBPASS_EXTERNAL;
  resumeDep.dstSubpass = 0;
  resumeDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  resumeDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  resumeDep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  resumeDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  info.dependencyCount = 1;
  info.pDependencies = &resumeDep;

  if (vkCreateRenderPass(mDevice, &info, nullptr, &mResumePass) != VK_SUCCESS)
  {
    { LOG(LogError) << "[Vulkan] vkCreateRenderPass failed for the resume pass"; }
    return false;
  }
  return true;
}

bool RendererVulkan::DoCreateOffscreen(int w, int h, TextureHandle& texture)
{
  texture = InvalidTextureHandle;
  if (mDevice == VK_NULL_HANDLE || w <= 0 || h <= 0) return false;

  DestroyOffscreenTarget();

  if (mOffscreenPass == VK_NULL_HANDLE && !CreateOffscreenPasses()) return false;

  mOffscreenExtent = { (unsigned int)w, (unsigned int)h };

  // --- Colour target, registered as a normal texture so the frontend can draw
  //     it back with the ordinary textured path ---
  VulkanTexture target;
  target.Width = w;
  target.Height = h;
  target.Type = TextureType::RGBA;

  VkImageCreateInfo image {};
  image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image.imageType = VK_IMAGE_TYPE_2D;
  image.format = mSwapFormat;
  image.extent = { (unsigned int)w, (unsigned int)h, 1 };
  image.mipLevels = 1;
  image.arrayLayers = 1;
  image.samples = VK_SAMPLE_COUNT_1_BIT;
  image.tiling = VK_IMAGE_TILING_OPTIMAL;
  image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  if (vkCreateImage(mDevice, &image, nullptr, &target.Image) != VK_SUCCESS)
  {
    { LOG(LogError) << "[Vulkan] Offscreen colour image creation failed"; }
    return false;
  }

  VkMemoryRequirements requirements {};
  vkGetImageMemoryRequirements(mDevice, target.Image, &requirements);
  int memoryType = FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (memoryType < 0) { vkDestroyImage(mDevice, target.Image, nullptr); return false; }

  VkMemoryAllocateInfo alloc {};
  alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc.allocationSize = requirements.size;
  alloc.memoryTypeIndex = (unsigned int)memoryType;
  if (vkAllocateMemory(mDevice, &alloc, nullptr, &target.Memory) != VK_SUCCESS)
  {
    { LOG(LogError) << "[Vulkan] Offscreen colour memory allocation failed"; }
    vkDestroyImage(mDevice, target.Image, nullptr);
    return false;
  }
  vkBindImageMemory(mDevice, target.Image, target.Memory, 0);

  VkImageViewCreateInfo view {};
  view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view.image = target.Image;
  view.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view.format = mSwapFormat;
  view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view.subresourceRange.levelCount = 1;
  view.subresourceRange.layerCount = 1;
  if (vkCreateImageView(mDevice, &view, nullptr, &target.View) != VK_SUCCESS)
  {
    { LOG(LogError) << "[Vulkan] Offscreen colour view creation failed"; }
    vkFreeMemory(mDevice, target.Memory, nullptr);
    vkDestroyImage(mDevice, target.Image, nullptr);
    return false;
  }

  if (!AllocateDescriptorSet(target))
  {
    vkDestroyImageView(mDevice, target.View, nullptr);
    vkFreeMemory(mDevice, target.Memory, nullptr);
    vkDestroyImage(mDevice, target.Image, nullptr);
    return false;
  }

  // Sampleable straight away: the frontend may draw the target before anything
  // has ever been rendered into it, and the descriptor promises that layout.
  RunOneShot([&](VkCommandBuffer cmd)
  {
    TransitionLayout(cmd, target.Image, VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    TransitionLayout(cmd, target.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  });

  mOffscreenHandle = mNextHandle++;
  mTextures.insert_unique((TextureHandle)mOffscreenHandle, (VulkanTexture)target);

  // --- Depth+stencil for the offscreen pass, at the offscreen size ---
  VkImageCreateInfo depth {};
  depth.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  depth.imageType = VK_IMAGE_TYPE_2D;
  depth.format = mDepthFormat;
  depth.extent = { (unsigned int)w, (unsigned int)h, 1 };
  depth.mipLevels = 1;
  depth.arrayLayers = 1;
  depth.samples = VK_SAMPLE_COUNT_1_BIT;
  depth.tiling = VK_IMAGE_TILING_OPTIMAL;
  depth.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  depth.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  if (vkCreateImage(mDevice, &depth, nullptr, &mOffscreenDepthImage) != VK_SUCCESS)
  { DestroyOffscreenTarget(); return false; }

  vkGetImageMemoryRequirements(mDevice, mOffscreenDepthImage, &requirements);
  memoryType = FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (memoryType < 0) { DestroyOffscreenTarget(); return false; }
  alloc.allocationSize = requirements.size;
  alloc.memoryTypeIndex = (unsigned int)memoryType;
  if (vkAllocateMemory(mDevice, &alloc, nullptr, &mOffscreenDepthMemory) != VK_SUCCESS)
  { DestroyOffscreenTarget(); return false; }
  vkBindImageMemory(mDevice, mOffscreenDepthImage, mOffscreenDepthMemory, 0);

  VkImageViewCreateInfo depthView {};
  depthView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  depthView.image = mOffscreenDepthImage;
  depthView.viewType = VK_IMAGE_VIEW_TYPE_2D;
  depthView.format = mDepthFormat;
  depthView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  depthView.subresourceRange.levelCount = 1;
  depthView.subresourceRange.layerCount = 1;
  if (vkCreateImageView(mDevice, &depthView, nullptr, &mOffscreenDepthView) != VK_SUCCESS)
  { DestroyOffscreenTarget(); return false; }

  // --- Framebuffer ---
  VkImageView views[2] = { mTextures.try_get(mOffscreenHandle)->View, mOffscreenDepthView };
  VkFramebufferCreateInfo framebuffer {};
  framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebuffer.renderPass = mOffscreenPass;
  framebuffer.attachmentCount = 2;
  framebuffer.pAttachments = views;
  framebuffer.width = (unsigned int)w;
  framebuffer.height = (unsigned int)h;
  framebuffer.layers = 1;
  if (vkCreateFramebuffer(mDevice, &framebuffer, nullptr, &mOffscreenFramebuffer) != VK_SUCCESS)
  {
    { LOG(LogError) << "[Vulkan] Offscreen framebuffer creation failed"; }
    DestroyOffscreenTarget();
    return false;
  }

  texture = mOffscreenHandle;
  return true;
}

void RendererVulkan::DestroyOffscreenTarget() noexcept
{
  if (mDevice == VK_NULL_HANDLE) return;

  if (mOffscreenFramebuffer != VK_NULL_HANDLE)
  { vkDestroyFramebuffer(mDevice, mOffscreenFramebuffer, nullptr); mOffscreenFramebuffer = VK_NULL_HANDLE; }
  if (mOffscreenDepthView != VK_NULL_HANDLE)
  { vkDestroyImageView(mDevice, mOffscreenDepthView, nullptr); mOffscreenDepthView = VK_NULL_HANDLE; }
  if (mOffscreenDepthImage != VK_NULL_HANDLE)
  { vkDestroyImage(mDevice, mOffscreenDepthImage, nullptr); mOffscreenDepthImage = VK_NULL_HANDLE; }
  if (mOffscreenDepthMemory != VK_NULL_HANDLE)
  { vkFreeMemory(mDevice, mOffscreenDepthMemory, nullptr); mOffscreenDepthMemory = VK_NULL_HANDLE; }

  // The colour target is a normal texture, so it leaves through the normal
  // deferred path — a frame being recorded may still be sampling it.
  if (mOffscreenHandle != InvalidTextureHandle)
  {
    DoDestroyTexture(mOffscreenHandle);
    mOffscreenHandle = InvalidTextureHandle;
  }
  mOffscreenExtent = { 0, 0 };
  mInOffscreen = false;
}

void RendererVulkan::DoDestroyOffscreen(TextureHandle& texture) noexcept
{
  DestroyOffscreenTarget();

  // The passes outlive individual targets, but Renderer only ever destroys the
  // offscreen on teardown or before a resize, so release them here too.
  if (mDevice != VK_NULL_HANDLE)
  {
    if (mOffscreenPass != VK_NULL_HANDLE) { vkDestroyRenderPass(mDevice, mOffscreenPass, nullptr); mOffscreenPass = VK_NULL_HANDLE; }
    if (mResumePass != VK_NULL_HANDLE)    { vkDestroyRenderPass(mDevice, mResumePass, nullptr); mResumePass = VK_NULL_HANDLE; }
  }
  texture = InvalidTextureHandle;
}

void RendererVulkan::ApplyViewportAndScissor(VkExtent2D extent)
{
  VkViewport viewport {};
  viewport.width = (float)extent.width;
  viewport.height = (float)extent.height;
  viewport.maxDepth = 1.f;
  vkCmdSetViewport(mCommands[mFrame], 0, 1, &viewport);

  VkRect2D scissor = mScissorSet ? mScissor : VkRect2D { { 0, 0 }, extent };
  vkCmdSetScissor(mCommands[mFrame], 0, 1, &scissor);
}

void RendererVulkan::DoBeginOffScreen(int w, int h)
{
  (void)w; (void)h;   // the target was sized at creation; these must agree
  if (mInOffscreen) return;
  if (mOffscreenFramebuffer == VK_NULL_HANDLE)
  {
    static bool once = false; if (!once) { once = true; LOG(LogError) << "[Vulkan] BeginOffScreen with no offscreen target"; }
    return;
  }
  if (!BeginFrameIfNeeded()) return;

  // Close the main pass and open the offscreen one. Only one render pass may be
  // active at a time, so this is a swap, not a nesting.
  vkCmdEndRenderPass(mCommands[mFrame]);

  VkClearValue clears[2];
  clears[0] = mClearColor;
  clears[1].depthStencil = { 1.f, 0 };

  VkRenderPassBeginInfo pass {};
  pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  pass.renderPass = mOffscreenPass;
  pass.framebuffer = mOffscreenFramebuffer;
  pass.renderArea.offset = { 0, 0 };
  pass.renderArea.extent = mOffscreenExtent;
  pass.clearValueCount = 2;
  pass.pClearValues = clears;
  vkCmdBeginRenderPass(mCommands[mFrame], &pass, VK_SUBPASS_CONTENTS_INLINE);

  ApplyViewportAndScissor(mOffscreenExtent);
  // Dynamic state and pipeline bindings do not survive a pass change.
  mCurrentPipeline = VK_NULL_HANDLE;
  mInOffscreen = true;
}

void RendererVulkan::DoEndOffScreen(int w, int h)
{
  (void)w; (void)h;
  if (!mInOffscreen) return;
  mInOffscreen = false;
  if (!mFrameStarted) return;

  vkCmdEndRenderPass(mCommands[mFrame]);

  // Back to the swapchain image, loading what was already drawn there rather
  // than clearing it. EndFrame() closes this pass exactly as it closes the main
  // one, so nothing downstream needs to know which of the two is open.
  VkRenderPassBeginInfo pass {};
  pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  pass.renderPass = mResumePass;
  pass.framebuffer = mFramebuffers[(int)mImageIndex];
  pass.renderArea.offset = { 0, 0 };
  pass.renderArea.extent = mExtent;
  pass.clearValueCount = 0;   // every attachment loads
  vkCmdBeginRenderPass(mCommands[mFrame], &pass, VK_SUBPASS_CONTENTS_INLINE);

  ApplyViewportAndScissor(mExtent);
  mCurrentPipeline = VK_NULL_HANDLE;
}

#endif // USE_VULKAN
