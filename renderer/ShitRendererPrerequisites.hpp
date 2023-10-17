/**
 * @file ShitRendererPrerequisites.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once

#include "ShitEnum.hpp"
#include "ShitPredefines.hpp"
#include "ShitUtility.hpp"

#ifdef NDEBUG
#define ST_LOG(...)
#define ST_LOG_VAR(str)
#else
#define ST_LOG(...)                                       \
    {                                                     \
        std::cout << __FILE__ << " " << __LINE__ << ": "; \
        Shit::myprint(std::cout, __VA_ARGS__);            \
        std::cout << std::endl;                           \
    }

#define ST_LOG_VAR(str) std::cout << __FILE__ << " " << __LINE__ << ":  " << #str << ": " << str << std::endl;
#endif

#define ST_THROW(...)                               \
    {                                               \
        std::stringstream ss;                       \
        Shit::myprint(ss, __FILE__, __LINE__, ":"); \
        Shit::myprint(ss, __VA_ARGS__);             \
        throw std::runtime_error(ss.str());         \
    }

// TODO: redefine object as object_T and define object_T* as object

namespace Shit {
class RenderSystem;
class Device;
class Swapchain;
class Shader;
struct Event;
class CommandPool;
class Semaphore;
class CommandBuffer;
class Fence;
class Surface;
class RenderPass;
class Buffer;
class BufferView;
class Image;
class ImageView;
class Sampler;
class DescriptorSetLayout;
class DescriptorSet;
class PipelineLayout;
class Framebuffer;
class Pipeline;
class VertexArray;

// common object types
struct Offset2D {
    int32_t x;
    int32_t y;
};
struct Offset3D {
    int32_t x;
    int32_t y;
    int32_t z;
};
struct Extent2D {
    uint32_t width;
    uint32_t height;
};
struct Extent3D {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};
struct Rect2D {
    Offset2D offset;
    Extent2D extent;
};

struct Rect3D {
    Offset3D offset;
    Extent3D extent;
};

struct RenderSystemCreateInfo {
    RendererVersion version;
    RenderSystemCreateFlagBits flags;
};

using PhysicalDevice = void *;

struct DeviceCreateInfo {
    PhysicalDevice physicalDevice;
    // std::variant<PhysicalDevice, Window *> physicalDevice; // use shitwindow
    // for vulkan
};

// struct WindowCreateInfo
// {
// 	WindowCreateFlagBits flags;
// 	const wchar_t *name;
// 	Rect2D rect; // client area
// 	std::function<void(const Event &)> callBackFunction;
// };

struct SurfaceCreateInfoWin32 {
    void *hwnd;
};

struct SurfacePixelFormat {
    Format format;
    ColorSpace colorSpace;
};

struct SwapchainCreateInfo {
    Device *pDevice;
    uint32_t minImageCount;     // when using gl, 1 means using leftback buffer
    Format format;              //!< opengl can choose  RGBA8_UNORM, RGBA8_SRGB
    ColorSpace colorSpace;      //!< only sRGB
    Extent2D imageExtent;       //!< no use for opengl
    uint32_t imageArrayLayers;  //!< alway 1 unless you are developing a
                                //!< stereoscopic 3D applicaiton
    ImageUsageFlagBits imageUsage;
    PresentMode presentMode;
};

struct ShaderCreateInfo {
    ShadingLanguage shadingLanguage;
    size_t size;
    void const *code;
};

struct SpecializationInfo {
    uint32_t constantValueCount;
    uint32_t const *constantIds;
    uint32_t const *constantValues;  // can be float value
};
struct PipelineShaderStageCreateInfo {
    ShaderStageFlagBits stage;
    Shader const *pShader;
    char const *entryName;
    SpecializationInfo specializationInfo;
};
struct VertexAttributeDescription {
    uint32_t location;  // location in shader
    uint32_t binding;   // index in given buffers
    Format format;
    uint32_t offset;
};

struct VertexBindingDescription {
    uint32_t binding;  // index in given buffers
    uint32_t stride;
    // attributes advance once per divior instances,when 0, advance
    // per vertex, vulkan only support 0 and 1 currently
    uint32_t divisor;
};

struct VertexInputStateCreateInfo {
    uint32_t vertexBindingDescriptionCount;
    VertexBindingDescription const *vertexBindingDescriptions;
    uint32_t vertexAttributeDescriptionCount;
    VertexAttributeDescription const *vertexAttributeDescriptions;
};

// the same as VkViewport
struct Viewport {
    float x;
    float y;
    float width;
    float height;
    float minDepth;
    float maxDepth;
};
struct PipelineInputAssemblyStateCreateInfo {
    PrimitiveTopology topology;
    bool primitiveRestartEnable;
};
struct PipelineViewportStateCreateInfo {
    uint32_t viewportCount;  // canbe 0 when using dynamic state viewport
    Viewport const *viewports;
    uint32_t scissorCount;  // canbe 0 when using dynamic state scissor
    Rect2D const *scissors;
};
struct PipelineTessellationStateCreateInfo {
    uint32_t patchControlPoints;
};
struct PipelineRasterizationStateCreateInfo {
    bool depthClampEnable;
    bool rasterizerDiscardEnbale;
    PolygonMode polygonMode;
    CullMode cullMode;
    FrontFace frontFace;
    bool depthBiasEnable;
    float depthBiasContantFactor;
    float depthBiasClamp;
    float depthBiasSlopeFactor;
    float lineWidth;
};
struct PipelineMultisampleStateCreateInfo {
    SampleCountFlagBits rasterizationSamples;
    bool sampleShadingEnable;
    float minSampleShading;
    uint32_t const *pSampleMask;  // array size is sampleCount
    bool alphaToCoverageEnable;
    bool alphaToOneEnable;
};
struct StencilOpState {
    StencilOp failOp;
    StencilOp passOp;
    StencilOp depthFailOp;
    CompareOp compareOp;
    uint32_t compareMask;
    uint32_t writeMask;
    uint32_t reference;
};
struct PipelineDepthStencilStateCreateInfo {
    bool depthTestEnable;
    bool depthWriteEnable;
    CompareOp depthCompareOp;
    bool depthBoundsTestEnable;
    bool stencilTestEnable;
    StencilOpState front;
    StencilOpState back;
    float minDepthBounds;
    float maxDepthBounds;
};
struct PipelineColorBlendAttachmentState {
    bool blendEnable;
    BlendFactor srcColorBlendFactor;
    BlendFactor dstColorBlendFactor;
    BlendOp colorBlendOp;
    BlendFactor srcAlphaBlendFactor;
    BlendFactor dstAlphaBlendFactor;
    BlendOp alphaBlendOp;
    ColorComponentFlagBits colorWriteMask;
};
struct PipelineColorBlendStateCreateInfo {
    bool logicOpEnable;
    LogicOp logicOp;
    uint32_t attachmentCount;
    PipelineColorBlendAttachmentState const *attachments;
    float blendConstants[4];
};
struct PipelineDynamicStateCreateInfo {
    uint32_t dynamicStateCount;
    DynamicState const *dynamicStates;
};
struct GraphicsPipelineCreateInfo {
    // PipelineCreateFlagBits flags; //for vulkan
    uint32_t stageCount;
    PipelineShaderStageCreateInfo const *stages;
    VertexInputStateCreateInfo vertexInputState;
    PipelineInputAssemblyStateCreateInfo inputAssemblyState;
    PipelineViewportStateCreateInfo viewportState;
    PipelineTessellationStateCreateInfo tessellationState;
    PipelineRasterizationStateCreateInfo rasterizationState;
    PipelineMultisampleStateCreateInfo multisampleState;
    PipelineDepthStencilStateCreateInfo depthStencilState;
    PipelineColorBlendStateCreateInfo colorBlendState;
    PipelineDynamicStateCreateInfo dynamicState;
    PipelineLayout const *pLayout;
    RenderPass const *pRenderPass;
    uint32_t subpass;
};
struct ComputePipelineCreateInfo {
    PipelineShaderStageCreateInfo stage;
    PipelineLayout const *pLayout;
};
struct BufferCreateInfo {
    BufferCreateFlagBits flags;
    uint64_t size;
    BufferUsageFlagBits usage;
    MemoryPropertyFlagBits memoryPropertyFlags;
};
struct QueueFamily {
    QueueFlagBits flags;
    uint32_t index;
    uint32_t count;
};

struct CommandPoolCreateInfo {
    CommandPoolCreateFlagBits flags;
    uint32_t queueFamilyIndex;
};
struct CommandBufferCreateInfo {
    CommandBufferLevel level;
    uint32_t count;
};
struct CommandBufferInheritanceInfo {
    RenderPass const *pRenderPass;
    uint32_t subpass;
    // optional, when use secondary cmdbuffer, this can be set
    Framebuffer const *pFramebuffer;
};
struct CommandBufferBeginInfo {
    CommandBufferUsageFlagBits usage;
    // only used when commandBuffer is a secondary buffer
    CommandBufferInheritanceInfo inheritanceInfo;
};
struct QueueCreateInfo {
    uint32_t queueFamilyIndex;
    uint32_t queueIndex;
};

struct FenceCreateInfo {
    FenceCreateFlagBits flags;
};

struct SemaphoreCreateInfo {
    //		SemaphoreType type;
};

struct ImageCreateInfo {
    ImageCreateFlagBits flags;
    ImageType imageType;
    Format format;
    Extent3D extent;
    uint32_t mipLevels;  // if 1 means no mipmap, 0 means generate all mipmap,
                         // otherwise generate specified mipmap levels
    uint32_t arrayLayers;
    SampleCountFlagBits samples;
    ImageTiling tiling;  // no use for opengl
    ImageUsageFlagBits usageFlags;
    MemoryPropertyFlagBits memoryPropertyFlags;
    ImageLayout initialLayout;  // if image data is not null, layout should not be
                                // undefined
};

// union ClearColorValue
//{
//	float float32[4];
//	int32_t int32[4];
//	uint32_t uint32[4];
// };
// union ClearValue
//{
//	ClearColorValue color;
//	ClearDepthStencilValue depthStencil;
// };

using ClearColorValueFloat = std::array<float, 4>;
using ClearColorValueInt32 = std::array<int32_t, 4>;
using ClearColorValueUint32 = std::array<uint32_t, 4>;

struct ClearDepthStencilValue {
    float depth;
    uint32_t stencil;
};
using ClearColorValue = std::variant<ClearColorValueFloat, ClearColorValueInt32, ClearColorValueUint32>;
// using ClearValue = std::variant<ClearColorValue, ClearDepthStencilValue>;
using ClearValue =
    std::variant<ClearColorValueFloat, ClearColorValueInt32, ClearColorValueUint32, ClearDepthStencilValue>;

struct SamplerCreateInfo {
    Filter magFilter;
    Filter minFilter;
    SamplerMipmapMode mipmapMode;
    SamplerWrapMode wrapModeU;
    SamplerWrapMode wrapModeV;
    SamplerWrapMode wrapModeW;
    float mipLodBias;
    bool anisotropyEnable;
    float maxAnisotropy;
    bool compareEnable;
    CompareOp compareOp;
    float minLod;
    float maxLod;
    BorderColor borderColor;
    // ClearColorValue borderColor;
};

struct BufferCopy {
    uint64_t srcOffset;
    uint64_t dstOffset;
    uint64_t size;
};
struct CopyBufferInfo {
    Buffer const *pSrcBuffer;
    Buffer const *pDstBuffer;
    uint32_t regionCount;
    BufferCopy const *pRegions;
};

struct ImageSubresourceLayers {
    ImageAspectFlagBits aspectMask;
    uint32_t mipLevel;
    uint32_t baseArrayLayer;
    uint32_t layerCount;
};

struct ImageSubresource {
    uint32_t mipLevel;
    uint32_t arrayLayer;
};
struct SubresourceLayout {
    uint64_t offset;
    uint64_t size;
    uint64_t rowPitch;
    uint64_t arrayPitch;
    uint64_t depthPitch;
};

struct ImageCopy {
    ImageSubresourceLayers srcSubresource;
    Offset3D srcOffset;
    ImageSubresourceLayers dstSubresource;
    Offset3D dstOffset;
    Extent3D extent;
};

struct CopyImageInfo {
    Image const *pSrcImage;
    Image const *pDstImage;
    uint32_t regionCount;
    ImageCopy const *pRegions;
};

struct BufferImageCopy {
    uint64_t bufferOffset;
    uint32_t bufferRowLength;    // 0 means tightly packed
    uint32_t bufferImageHeight;  // 0 means tightly packed
    ImageSubresourceLayers imageSubresource;
    Offset3D imageOffset;
    Extent3D imageExtent;
};
struct CopyBufferToImageInfo {
    Buffer const *pSrcBuffer;
    Image const *pDstImage;
    uint32_t regionCount;
    BufferImageCopy const *pRegions;
};
struct CopyImageToBufferInfo {
    Image const *pSrcImage;
    Buffer const *pDstBuffer;
    uint32_t regionCount;
    BufferImageCopy const *pRegions;
};
struct ImageBlit {
    ImageSubresourceLayers srcSubresource;
    Offset3D srcOffsets[2];
    ImageSubresourceLayers dstSubresource;
    Offset3D dstOffsets[2];
};
struct BlitImageInfo {
    Image const *pSrcImage;
    Image const *pDstImage;
    Filter filter;
    uint32_t regionCount;
    ImageBlit const *pRegions;
};

struct DescriptorSetLayoutBinding {
    uint32_t binding;  // shader binding
    DescriptorType descriptorType;
    uint32_t descriptorCount;        // array
    ShaderStageFlagBits stageFlags;  // Vulkan
    uint32_t immutableSamplerCount;
    Sampler const *const *pImmutableSamplers;
};

struct DescriptorSetLayoutCreateInfo {
    uint32_t descriptorSetLayoutBindingCount;
    DescriptorSetLayoutBinding const *pDescriptorSetLayoutBindings;
};

struct ComponentMapping {
    ComponentSwizzle r;
    ComponentSwizzle g;
    ComponentSwizzle b;
    ComponentSwizzle a;
};
struct ImageSubresourceRange {
    ImageAspectFlagBits aspectMask;
    uint32_t baseMipLevel;
    uint32_t levelCount;
    uint32_t baseArrayLayer;
    uint32_t layerCount;
};

struct ImageViewCreateInfo {
    Image const *pImage;
    ImageViewType viewType;
    Format format;
    ComponentMapping components;
    ImageSubresourceRange subresourceRange;
};

struct DescriptorImageInfo {
    Sampler const *pSampler;
    ImageView const *pImageView;
    ImageLayout imageLayout;
};
struct DescriptorBufferInfo {
    Buffer const *pBuffer;
    uint64_t offset;
    uint64_t range;
};
struct WriteDescriptorSet {
    DescriptorSet const *pDstSet;
    uint32_t dstBinding;
    uint32_t dstArrayElement;  // start array index
    uint32_t descriptorCount;
    // must be same as type of dstset at dstbinding
    DescriptorType descriptorType;
    DescriptorImageInfo const *pImageInfo;
    DescriptorBufferInfo const *pBufferInfo;
    BufferView const *const *pTexelBufferView;
};
struct CopyDescriptorSet {
    DescriptorSet const *pSrcSet;
    uint32_t srcBinding;
    uint32_t srcArrayElement;
    DescriptorSet const *pDstSet;
    uint32_t dstBinding;
    uint32_t dstArrayElement;
    uint32_t descriptorCount;
};
struct DescriptorSetAllocateInfo {
    uint32_t setLayoutCount;
    DescriptorSetLayout const *const *setLayouts;
};
struct DescriptorPoolSize {
    DescriptorType type;
    uint32_t count;
};
struct DescriptorPoolCreateInfo {
    uint32_t maxSets;
    uint32_t poolSizeCount;
    DescriptorPoolSize const *poolSizes;
};

// vulkan only
struct PushConstantRange {
    ShaderStageFlagBits stageFlags;  // for vulkan
    uint32_t binding;                // only needed in opengl, use non opaque uniform block as
                                     // push constant
    uint32_t offset;
    uint32_t size;
};

struct PipelineLayoutCreateInfo {
    uint32_t setLayoutCount;
    DescriptorSetLayout const *const *pSetLayouts;
    uint32_t pushConstantRangeCount;
    PushConstantRange const *pPushConstantRanges;
};

struct DrawIndirectCommand {
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
};

struct DrawIndexedIndirectCommand {
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t vertexOffset;  // in bytes
    uint32_t firstInstance;
};
struct DrawIndirectInfo {
    Buffer const *pBuffer;
    uint64_t offset;
    uint32_t drawCount;
    uint32_t stride;
};
struct DrawIndirectCountInfo {
    Buffer const *pBuffer;
    uint64_t offset;
    Buffer const *pCountBuffer;  //!< read a single GLsizei(GL)/uint32_t(VK) type value
    uint64_t countBufferOffset;
    uint32_t maxDrawCount;
    uint32_t stride;
};

struct AttachmentDescription {
    Format format;
    SampleCountFlagBits samples;
    AttachmentLoadOp loadOp;    // glclear
    AttachmentStoreOp storeOp;  // opengl
    AttachmentLoadOp stencilLoadOp;
    AttachmentStoreOp stencilStoreOp;
    ImageLayout initialLayout;
    ImageLayout finalLayout;
};

struct AttachmentReference {
    uint32_t attachment;  // canbe ST_ATTACHMENT_UNUSED, index of attachment
                          // description
    ImageLayout layout;
};

struct SubpassDescription {
    PipelineBindPoint pipelineBindPoint;
    uint32_t inputAttachmentCount;
    AttachmentReference const *pInputAttachments;
    uint32_t colorAttachmentCount;
    AttachmentReference const *pColorAttachments;
    AttachmentReference const *pResolveAttachments;  // either null or same count as color attachments
    AttachmentReference const *pDepthStencilAttachment;
    // uint32_t preserveAttachmentCount;
    // const AttachmentReference const *preserveAttachments;
};
struct SubpassDependency {
    uint32_t srcSubpass;  // canbe ST_SUBPASS_EXTERNAL
    uint32_t dstSubpass;
    PipelineStageFlagBits srcStageMask;
    PipelineStageFlagBits dstStageMask;
    AccessFlagBits srcAccessFlagBits;
    AccessFlagBits dstAccessFlagBits;
};
struct RenderPassMultiViewCreateInfo {
    uint32_t subpassCount;
    uint32_t const *pViewMasks;
    uint32_t dependencyCount;
    int32_t const *pViewOffsets;
    uint32_t correlationMaskCount;
    uint32_t const *pCorrelationMasks;
};
struct RenderPassCreateInfo {
    uint32_t attachmentCount;
    AttachmentDescription const *pAttachments;
    uint32_t subpassCount;
    SubpassDescription const *pSubpasses;
    uint32_t dependencyCount;
    SubpassDependency const *pSubpassDependencies;
    RenderPassMultiViewCreateInfo const *pMultiviewCreateInfo;
};
struct FramebufferAttachmentImageInfo {
    ImageCreateFlagBits flags;
    ImageUsageFlagBits usage;
    uint32_t width;
    uint32_t height;
    uint32_t layoutCount;
    uint32_t viewFormatCount;
    Format const *pViewFormats;
};
// struct FramebufferAttachmentCreateInfo
//{
//	uint32_t attachmentImageInfoCount;
//	FramebufferAttachmentImageInfo const *pAttachmentImageInfos;
// };
struct FramebufferCreateInfo {
    RenderPass const *pRenderPass;
    uint32_t attachmentCount;
    ImageView const *const *pAttachments;  // the same order as attachment
                                           // descriptions in renderpassCreateInfo
    Extent2D extent;
    uint32_t layers;  // if use multiview, must be 1
    // if attachmentcount is not 0, if pAttachmentImageInfos is not nullptr, it
    // means using imageless framebuffer,
    FramebufferAttachmentImageInfo const *pAttachmentImageInfos;
};

struct RenderPassAttachmentBeginInfo {
    uint32_t attachmentCount;
    ImageView const *const *pAttachments;
};
struct BeginRenderPassInfo {
    RenderPass const *pRenderPass;
    Framebuffer const *pFramebuffer;
    Rect2D renderArea;  // gl not support
    uint32_t clearValueCount;
    ClearValue const *pClearValues;
    SubpassContents contents;  // first subpass provide method
    RenderPassAttachmentBeginInfo attachmentBeginInfo;
};

struct GetNextImageInfo {
    uint64_t timeout;
    // must be unsigal, will be signaled after operation
    Semaphore const *pSemaphore;
    Fence const *pFence;
};
struct SubmitInfo {
    uint32_t waitSemaphoreCount;
    //! wait pipeline stage is color attachment output,will be unsignaled
    Semaphore const *const *pWaitSemaphores;
    uint32_t commandBufferCount;
    CommandBuffer const *const *pCommandBuffers;
    uint32_t signalSemaphoreCount;
    Semaphore const *const *pSignalSemaphores;
};
struct PresentInfo {
    uint32_t waitSemaphoreCount;
    Semaphore const *const *pWaitSemaphores;
    uint32_t swapchainCount;
    Swapchain const *const *pSwapchains;
    uint32_t const *pImageIndices;
};
struct PresentInfo2 {
    std::span<Semaphore const *> waitSemaphores;
    std::span<Swapchain const *> swapchains;
    std::span<uint32_t const> imageIndices;
};
struct BindVertexBuffersInfo {
    uint32_t firstBinding;  // not location
    uint32_t bindingCount;
    Buffer const *const *ppBuffers;  // buffer pointer array
    uint64_t const *pOffsets;        // offset array
};
struct BindIndexBufferInfo {
    Buffer const *pBuffer;
    uint64_t offset;  // TODO: attention!!!, gl version donot support index buffer
                      // offset, offset must be 0
    IndexType indexType;
};
struct BindPipelineInfo {
    PipelineBindPoint bindPoint;
    Pipeline const *pPipeline;
};
struct ExecuteCommandsInfo {
    uint32_t count;
    CommandBuffer const *const *pCommandBuffers;
};
struct BindDescriptorSetsInfo {
    PipelineBindPoint pipelineBindPoint;
    PipelineLayout const *pPipelineLayout;
    uint32_t firstset;  // set number to be bound
    uint32_t descriptorSetCount;
    DescriptorSet const *const *ppDescriptorSets;
    uint32_t dynamicOffsetCount;  // dynamic uniform or storage buffer
    uint32_t const *pDynamicOffsets;
};
struct BufferViewCreateInfo {
    Buffer const *pBuffer;
    Format format;
    uint64_t offset;
    uint64_t range;
};
struct MemoryBarrier {
    AccessFlagBits srcAccessMask;
    AccessFlagBits dstAccessMask;
};
struct BufferMemoryBarrier {
    AccessFlagBits srcAccessMask;
    AccessFlagBits dstAccessMask;
    uint32_t srcQueueFamilyIndex;  // canbe ST_QUEUE_FAMILY_IGNORED
    uint32_t dstQueueFamilyIndex;  // canbe ST_QUEUE_FAMILY_IGNORED
    Buffer const *pBuffer;
    uint64_t offset;
    uint64_t size;
};
struct ImageMemoryBarrier {
    AccessFlagBits srcAccessMask;
    AccessFlagBits dstAccessMask;
    ImageLayout oldImageLayout;
    ImageLayout newImageLayout;
    uint32_t srcQueueFamilyIndex;  // canbe ST_QUEUE_FAMILY_IGNORED
    uint32_t dstQueueFamilyIndex;  // canbe ST_QUEUE_FAMILY_IGNORED
    Image const *pImage;
    ImageSubresourceRange subresourceRange;
};
struct PipelineBarrierInfo {
    PipelineStageFlagBits srcStageMask;
    PipelineStageFlagBits dstStageMask;
    DependencyFlagBits dependencyFlags;
    uint32_t memoryBarrierCount;
    MemoryBarrier const *pMemoryBarriers;
    uint32_t bufferMemoryBarrierCount;
    BufferMemoryBarrier const *pBufferMemoryBarriers;
    uint32_t imageMemoryBarrierCount;
    ImageMemoryBarrier const *pImageMemoryBarriers;
};
struct PushConstantInfo {
    PipelineLayout const *pPipelineLayout;
    ShaderStageFlagBits stageFlags;  // for vulkan
    uint32_t offset;
    uint32_t size;
    void const *pValues;  // an array of size bytes
};
struct DispatchInfo {
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;
};
struct DispatchIndirectCommand {
    // num of local workgroups
    uint32_t x;
    uint32_t y;
    uint32_t z;
};
struct DispatchIndirectInfo {
    Buffer const *pBuffer;
    uint64_t offset;
};

struct BindTransformFeedbackBuffersInfo {
    uint32_t firstBinding;
    uint32_t bindingCount;
    Buffer const *const *ppBuffers;
    uint64_t const *pOffsets;
    uint64_t const *pSizes;
};
struct BeginTransformFeedbackInfo {
    uint32_t firstCounterBuffer;
    uint32_t counterBufferCount;
    Buffer const *const *ppCounterBuffers;
    uint64_t const *pCounterBufferOffsets;
};
using EndTransformFeedbackInfo = BeginTransformFeedbackInfo;

struct SetViewPortInfo {
    uint32_t firstViewport;
    uint32_t viewportCount;
    Viewport const *pViewports;
};
struct SetScissorInfo {
    uint32_t firstScissor;
    uint32_t scissorCount;
    Rect2D const *pScissors;
};
// TODO: create a memory class instead??
struct MappedMemoryRange {
    std::variant<Buffer const *, Image const *> memory;
    uint64_t offset;
    uint64_t size;
};
struct MemoryAllocateInfo {
    uint64_t size;
    uint32_t memoryTypeIndex;
};
struct FillBufferInfo {
    Buffer const *buffer;
    uint64_t offset;
    uint64_t size;  // must be multiply of 4 or ST_WHOLE_SIZE
    uint32_t data;
};
struct ClearColorImageInfo {
    Image const *image;
    ImageLayout imageLayout;
    ClearColorValue clearColor;
    uint32_t rangeCount;
    ImageSubresourceRange const *pRanges;
};
struct ClearDepthStencilImageInfo {
    Image const *image;
    ImageLayout imageLayout;
    ClearDepthStencilValue depthStencil;
    uint32_t rangeCount;
    ImageSubresourceRange const *pRanges;
};
struct ClearRect {
    Rect2D rect;
    uint32_t baseArrayLayer;
    uint32_t layerCount;
};
struct ClearAttachment {
    ImageAspectFlagBits aspectMask;
    uint32_t colorAttachment;
    ClearValue clearValue;
};
struct ClearAttachmentsInfo {
    uint32_t attachmentCount;
    ClearAttachment const *pAttachments;
    uint32_t rectCount;
    ClearRect const *pRects;
};
struct UpdateBufferInfo {
    Buffer *dstBuffer;
    uint64_t dstOffset;
    uint64_t dataSize;
    const void *pData;
};
struct SetDepthBiasInfo {
    float depthBiasConstantFactor;
    float depthBiasClamp;
    float depthBiasSlopFactor;
};
struct SetDepthBoundsInfo {
    float minDepthBounds;
    float maxDepthBounds;
};
struct StencilMaskInfo {
    StencilFaceFlagBits faceMask;
    uint32_t mask;
};
using SetStencilCompareMaskInfo = StencilMaskInfo;
using SetStencilWriteMaskInfo = StencilMaskInfo;
using SetStencilReferenceInfo = StencilMaskInfo;

struct GenerateMipmapInfo {
    Image const *pImage;
    Filter filter;
    ImageLayout srcImageLayout;
    ImageLayout dstImageLayout;
};
struct PushDescriptorSetInfo {
    PipelineBindPoint pipelineBindPoint;
    PipelineLayout const *pLayout;
    uint32_t set;
    uint32_t descriptorWriteCount;
    WriteDescriptorSet const *pDescriptorWrites;
};
struct ImageResolve {
    ImageSubresourceLayers srcSubresource;
    Offset3D srcOffset;
    ImageSubresourceLayers dstSubresource;
    Offset3D dstOffset;
    Extent3D extent;
};
struct ResolveImageInfo {
    Image const *pSrcImage;
    ImageLayout srcImageLayout;
    Image const *pDstImage;
    ImageLayout dstImageLayout;
    uint32_t regionCount;
    ImageResolve const *pRegions;
};
struct SurfaceCapabilities {
    uint32_t minImageCount;
    uint32_t maxImageCount;
    Extent2D currentExtent;
    Extent2D minImageExtent;
    Extent2D maxImageExtent;
    uint32_t maxImageArrayLayers;
};
}  // namespace Shit
