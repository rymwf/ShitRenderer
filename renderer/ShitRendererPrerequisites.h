/**
 * @file ShitRendererPrerequisites.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once

#include "ShitUtility.h"
#include "ShitEnum.h"
#include "config.h"

#ifdef SHIT_DLL
#define SHIT_API __declspec(dllexport)
#else
#define SHIT_API
#endif // SHIT_DLL

#ifdef __GNUC__
#define LIBPREFIX "lib"
#else
#define LIBPREFIX ""
#endif

#define SHIT_DEFAULT_WINDOW_X 100
#define SHIT_DEFAULT_WINDOW_Y 60
#define SHIT_DEFAULT_WINDOW_WIDTH 800
#define SHIT_DEFAULT_WINDOW_HEIGHT 600

#define SHIT_RENDERER_LOAD_FUNC "ShitLoadRenderSystem"
#define SHIT_RENDERER_DELETE_FUNC "ShitDeleteRenderSystem"

#define SHIT_RENDERER_GL_NAME "GLRenderer"
#define SHIT_RENDERER_D3D11_NAME "D3D11Renderer"
#define SHIT_RENDERER_D3D12_NAME "D3D12Renderer"
#define SHIT_RENDERER_METAL_NAME "MetalRenderer"
#define SHIT_RENDERER_VULKAN_NAME "VKRenderer"

#ifdef NDEBUG
#define LOG(str)
#define LOG_VAR(str)
#else
#include <iostream>
#define LOG(str) \
	std::cout << __FILE__ << " " << __LINE__ << ":  " << str << std::endl
#define LOG_VAR(str) \
	std::cout << __FILE__ << " " << __LINE__ << ":  " << #str << ": " << str << std::endl
#endif

#define THROW(str) throw std::runtime_error(__FILE__ " " + std::to_string(__LINE__) + ": " + str);

namespace Shit
{
	class RenderSystem;
	class ShitWindow;
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

	//common object types
	struct Offset2D
	{
		int32_t x;
		int32_t y;
	};
	struct Offset3D
	{
		int32_t x;
		int32_t y;
		int32_t z;
	};
	struct Extent2D
	{
		uint32_t width;
		uint32_t height;
	};
	struct Extent3D
	{
		uint32_t width;
		uint32_t height;
		uint32_t depth;
	};
	struct Rect2D
	{
		Offset2D offset;
		Extent2D extent;
	};

	struct Rect3D
	{
		Offset3D offset;
		Extent3D extent;
	};

	struct RenderSystemCreateInfo
	{
		RendererVersion version;
		RenderSystemCreateFlagBits flags;
	};

	using PhysicalDevice = void *;

	struct DeviceCreateInfo
	{
		std::variant<PhysicalDevice, ShitWindow *> physicalDevice; //use shitwindow for vulkan
	};

	struct WindowCreateInfo
	{
		const char *name;
		Rect2D rect;
		std::shared_ptr<std::function<void(const Event &)>> eventListener;
	};

	struct SurfaceCreateInfo
	{
	};

	struct WindowPixelFormat
	{
		ShitFormat format;
		ColorSpace colorSpace;
	};

	struct SwapchainCreateInfo
	{
		uint32_t minImageCount;
		ShitFormat format;		   //!<opengl can choose  RGBA8_UNORM, RGBA8_SRGB
		ColorSpace colorSpace;	   //!<only sRGB
		Extent2D imageExtent;	   //!<no use for opengl
		uint32_t imageArrayLayers; //!< alway 1 unless you are developing a stereoscopic 3D applicaiton
		PresentMode presentMode;
	};

	struct ShaderCreateInfo
	{
		std::string code;
	};

	struct SpecializationInfo
	{
		std::vector<uint32_t> constantIDs;
		std::vector<uint32_t> constantValues;
	};
	struct PipelineShaderStageCreateInfo
	{
		ShaderStageFlagBits stage;
		Shader *pShader;
		const char *entryName;
		SpecializationInfo specializationInfo;
	};
	struct VertexAttributeDescription
	{
		uint32_t location;	 //location in shader
		uint32_t binding;	 //index in given buffers
		uint32_t components; //1,2,3,4
		DataType dataType;
		bool normalized;
		uint32_t offset;
	};

	struct VertexBindingDescription
	{
		//uint32_t binding; //index in given buffers, binding must be equal to the given buffer index
		uint32_t stride;
		uint32_t divisor; //attributes advance once per divior instances,when 0, advance per vertex
	};

	struct VertexInputStateCreateInfo
	{
		std::vector<VertexBindingDescription> vertexBindingDescriptions;
		std::vector<VertexAttributeDescription> vertexAttributeDescriptions;
	};
	struct Viewport
	{
		float x;
		float y;
		float width;
		float height;
		float minDepth;
		float maxDepth;
	};
	struct PipelineInputAssemblyStateCreateInfo
	{
		PrimitiveTopology topology;
		bool primitiveRestartEnable;
	};
	struct PipelineViewportStateCreateInfo
	{
		std::vector<Viewport> viewports;
		std::vector<Rect2D> scissors;
	};
	struct PipelineTessellationStateCreateInfo
	{
		uint32_t patchControlPoints;
	};
	struct PipelineRasterizationStateCreateInfo
	{
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
	struct PipelineMultisampleStateCreateInfo
	{
		SampleCountFlagBits rasterizationSamples;
		bool sampleShadingEnable;
		float minSampleShading;
		const uint32_t *pSampleMask; //array size is sampleCount
		bool alphaToCoverageEnable;
		bool alphaToOneEnable;
	};
	struct StencilOpState
	{
		StencilOp failOp;
		StencilOp passOp;
		StencilOp depthFailOp;
		CompareOp compareOp;
		uint32_t compareMask;
		uint32_t writeMask;
		uint32_t reference;
	};
	struct PipelineDepthStencilStateCreateInfo
	{
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
	struct PipelineColorBlendAttachmentState
	{
		bool blendEnable;
		BlendFactor srcColorBlendFactor;
		BlendFactor dstColorBlendFactor;
		BlendOp colorBlendOp;
		BlendFactor srcAlphaBlendFactor;
		BlendFactor dstAlphaBlendFactor;
		BlendOp alphaBlendOp;
		ColorComponentFlagBits colorWriteMask;
	};
	struct PipelineColorBlendStateCreateInfo
	{
		bool logicOpEnable;
		LogicOp logicOp;
		std::vector<PipelineColorBlendAttachmentState> attachments;
		std::array<float, 4> blendConstants;
	};
	struct PipelineDynamicStateCreateInfo
	{
		std::vector<DynamicState> dynamicStates;
	};
	struct GraphicsPipelineCreateInfo
	{
		std::vector<PipelineShaderStageCreateInfo> stages;
		VertexInputStateCreateInfo vertexInputState;
		PipelineInputAssemblyStateCreateInfo inputAssemblyState;
		PipelineViewportStateCreateInfo viewportState;
		PipelineTessellationStateCreateInfo tessellationState;
		PipelineRasterizationStateCreateInfo rasterizationState;
		PipelineMultisampleStateCreateInfo multisampleState;
		PipelineDepthStencilStateCreateInfo depthStencilState;
		PipelineColorBlendStateCreateInfo colorBlendState;
		PipelineDynamicStateCreateInfo dynamicState;
		PipelineLayout *pLayout;
		RenderPass *pRenderPass;
		uint32_t subpass;
	};
	struct ComputePipelineCreateInfo
	{
	};
	struct BufferCreateInfo
	{
		BufferCreateFlagBits flags;
		uint64_t size;
		BufferUsageFlagBits usage;
		MemoryPropertyFlagBits memoryPropertyFlags;
	};
	struct QueueFamilyIndex
	{
		uint32_t index;
		uint32_t count;
	};

	struct CommandPoolCreateInfo
	{
		CommandPoolCreateFlagBits flags;
		QueueFamilyIndex queueFamilyIndex;
	};
	struct CommandBufferCreateInfo
	{
		CommandBufferLevel level;
		uint32_t count;
	};
	struct CommandBufferBeginInfo
	{
		CommandBufferUsageFlagBits usage;
		Framebuffer *pFramebuffer; //optional, when use secondary cmdbuffer, this can be set
	};
	struct QueueCreateInfo
	{
		uint32_t queueFamilyIndex;
		uint32_t queueIndex;
	};

	struct FenceCreateInfo
	{
		FenceCreateFlagBits flags;
	};

	struct SemaphoreCreateInfo
	{
		//		SemaphoreType type;
	};

	struct ImageCreateInfo
	{
		ImageCreateFlagBits flags;
		ImageType imageType;
		ShitFormat format;
		Extent3D extent;
		uint32_t mipLevels;
		uint32_t arrayLayers;
		SampleCountFlagBits samples;
		ImageTiling tiling; //no use for opengl
		ImageUsageFlagBits usageFlags;
		MemoryPropertyFlagBits memoryPropertyFlags;
		bool generateMipmap; //if generate mipmap
		Filter mipmapFilter;
	};

	using ClearColorValue = std::variant<std::array<float, 4>, std::array<int32_t, 4>, std::array<uint32_t, 4>>;

	struct ClearDepthStencilValue
	{
		float depth;
		uint32_t stencil;
	};

	using ClearValue = std::variant<ClearColorValue, ClearDepthStencilValue>;

	struct SamplerCreateInfo
	{
		Filter magFilter;
		Filter minFilter;
		SamplerMipmapMode mipmapMode;
		SamplerWrapMode wrapModeU;
		SamplerWrapMode wrapModeV;
		SamplerWrapMode wrapModeW;
		float mipLodBias;
		bool anisotopyEnable;
		bool compareEnable;
		CompareOp compareOp;
		float minLod;
		float maxLod;
		BorderColor borderColor;
		//ClearColorValue borderColor;
	};

	struct BufferCopy
	{
		uint64_t srcOffset;
		uint64_t dstOffset;
		uint64_t size;
	};
	struct CopyBufferInfo
	{
		Buffer *pSrcBuffer;
		Buffer *pDstBuffer;
		uint32_t regionCount;
		BufferCopy *pRegions;
	};

	struct ImageSubresourceLayer
	{
		uint32_t mipLevel;
		uint32_t baseArrayLayer;
		uint32_t layerCount;
	};

	struct ImageCopy
	{
		ImageSubresourceLayer srcSubresource;
		Offset3D srcOffset;
		ImageSubresourceLayer dstSubresource;
		Offset3D dstOffset;
		Extent3D extent;
	};

	struct CopyImageInfo
	{
		Image *pSrcImage;
		Image *pDstImage;
		uint32_t regionCount;
		ImageCopy *pRegions;
	};

	struct BufferImageCopy
	{
		uint64_t bufferOffset;
		uint32_t bufferRowLength;
		uint32_t bufferImageHeight;
		ImageSubresourceLayer imageSubresource;
		Offset3D imageOffset;
		Extent3D imageExtent;
	};
	struct CopyBufferToImageInfo
	{
		Buffer *pSrcBuffer;
		Image *pDstImage;
		uint32_t regionCount;
		BufferImageCopy *pRegions;
	};
	struct CopyImageToBufferInfo
	{
		Image *pSrcImage;
		Buffer *pDstBuffer;
		uint32_t regionCount;
		BufferImageCopy *pRegions;
	};
	struct ImageBlit
	{
		ImageSubresourceLayer srcSubresource;
		std::array<Offset3D, 2> srcOffsets;
		ImageSubresourceLayer dstSubresource;
		std::array<Offset3D, 2> dstOffsets;
	};
	struct BlitImageInfo
	{
		Image *pSrcImage;
		Image *pDstImage;
		uint32_t regionCount;
		ImageBlit *pRegions;
		Filter filter;
	};

	struct DescriptorSetLayoutBinding
	{
		uint32_t binding; //shader binding
		DescriptorType descriptorType;
		uint32_t descriptorCount;		//array
		ShaderStageFlagBits stageFlags; //Vulkan
		std::vector<Sampler *> immutableSamplers;
	};

	struct DescriptorSetLayoutCreateInfo
	{
		std::vector<DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
	};

	struct ComponentMapping
	{
		ComponentSwizzle r;
		ComponentSwizzle g;
		ComponentSwizzle b;
		ComponentSwizzle a;
	};
	struct ImageSubresourceRange
	{
		uint32_t baseMipLevel;
		uint32_t levelCount;
		uint32_t baseArrayLayer;
		uint32_t layerCount;
	};

	struct ImageViewCreateInfo
	{
		Image *pImage;
		ImageViewType viewType;
		ShitFormat format;
		ComponentMapping components;
		ImageSubresourceRange subresourceRange;
	};

	struct DescriptorImageInfo
	{
		Sampler *pSampler;
		ImageView *pImageView;
		ImageLayout imageLayout;
	};
	struct DescriptorBufferInfo
	{
		Buffer *pBuffer;
		uint64_t offset;
		uint64_t range;
	};
	struct WriteDescriptorSet
	{
		DescriptorSet *pDstSet;
		uint32_t dstBinding;
		uint32_t dstArrayElement;	//start array index 
		DescriptorType descriptorType;	//must be same as type of dstset at dstbinding
		std::variant<
			std::vector<DescriptorImageInfo>,
			std::vector<DescriptorBufferInfo>,
			std::vector<BufferView *>>
			values;
	};
	struct CopyDescriptorSet
	{
		DescriptorSet *pSrcSet;
		uint32_t srcBinding;
		uint32_t srcArrayElement;
		DescriptorSet *pDstSet;
		uint32_t dstBinding;
		uint32_t dstArrayElement;
		uint32_t descriptorCount;
	};
	struct DescriptorSetAllocateInfo
	{
		std::vector<DescriptorSetLayout *> setLayouts;
	};
	struct DescriptorPoolSize
	{
		DescriptorType type;
		uint32_t count;
	};
	struct DescriptorPoolCreateInfo
	{
		uint32_t maxSets;
		std::vector<DescriptorPoolSize> poolSizes;
	};

	//vulkan only
	struct PushConstantRange
	{
		ShaderStageFlagBits stageFlags;
		uint32_t offset;
		uint32_t size;
	};

	struct PipelineLayoutCreateInfo
	{
		std::vector<DescriptorSetLayout *> setLayouts;
		std::vector<PushConstantRange> pushConstantRanges;
	};

	struct DrawIndirectCommand
	{
		uint32_t vertexCount;
		uint32_t instanceCount;
		uint32_t firstVertex;
		uint32_t firstInstance;
	};

	struct DrawIndexedIndirectCommand
	{
		uint32_t indexCount;
		uint32_t instanceCount;
		uint32_t firstIndex;
		int32_t vertexOffset;
		uint32_t firstInstance;
	};
	struct DrawIndirectInfo
	{
		Buffer *pBuffer;
		uint64_t offset;
		uint32_t drawCount;
		uint32_t stride;
	};
	struct DrawIndirectCountInfo
	{
		Buffer *pBuffer;
		uint64_t offset;
		Buffer *pCountBuffer; //!< read a single GLsizei(GL)/uint32_t(VK) type value
		uint64_t countBufferOffset;
		uint32_t maxDrawCount;
		uint32_t stride;
	};

	struct AttachmentDescription
	{
		ShitFormat format;
		SampleCountFlagBits samples;
		AttachmentLoadOp loadOp;   //glclear
		AttachmentStoreOp storeOp; //opengl default store
		AttachmentLoadOp stencilLoadOp;
		AttachmentStoreOp stencilStoreOp;
		ImageLayout initialLayout;
		ImageLayout finalLayout;
	};

	struct AttachmentReference
	{
		uint32_t attachment;
		ImageLayout layout;
	};

	struct SubpassDescription
	{
		PipelineBindPoint pipelineBindPoint;
		std::vector<AttachmentReference> colorAttachments;
		std::optional<AttachmentReference> resolveAttachment;
		std::optional<AttachmentReference> depthStencilAttachment;
	};
	struct RenderPassCreateInfo
	{
		std::vector<AttachmentDescription> attachments;
		std::vector<SubpassDescription> subpasses;
		//	std::vector<SubpassDependencies> spSubpassDependencies;	//TODO:spSubpassDependencies
	};
	struct FramebufferCreateInfo
	{
		RenderPass *pRenderPass;
		std::vector<ImageView *> attachments;
		Extent2D extent;
		uint32_t layers;
	};
	struct RenderPassBeginInfo
	{
		RenderPass *pRenderPass;
		Framebuffer *pFramebuffer;
		Rect2D renderArea;
		uint32_t clearValueCount;
		ClearValue *pClearValues;
		SubpassContents contents; //first subpass provide method
	};

	struct GetNextImageInfo
	{
		uint64_t timeout;
		Semaphore *pSemaphore; //must be unsigal, will be signaled after operation
		Fence *pFence;
	};
	struct SubmitInfo
	{
		std::vector<Semaphore *> waitSempahores; //! wait pipeline stage is color attachment output, will be unsignaled
		std::vector<CommandBuffer *> commandBuffers;
		std::vector<Semaphore *> signalSempahores;
	};
	struct PresentInfo
	{
		std::vector<Semaphore *> waitSemaphores;
		std::vector<Swapchain *> swapchains;
		std::vector<uint32_t> imageIndices;
	};
	struct BindVertexBufferInfo
	{
		uint32_t firstBinding;
		uint32_t bindingCount;
		Buffer **ppBuffers;	//buffer pointer array
		uint64_t *pOffsets; //offset array
	};
	struct BindIndexBufferInfo
	{
		Buffer *pBuffer;
		uint64_t offset;
		IndexType indexType;
	};
	struct BindPipelineInfo
	{
		PipelineBindPoint bindPoint;
		Pipeline *pPipeline;
	};
	struct ExecuteSecondaryCommandBufferInfo
	{
		uint32_t count;
		CommandBuffer *pCommandBuffers;
	};
	struct BindDescriptorSetsInfo
	{
		PipelineBindPoint pipelineBindPoint;
		PipelineLayout *pPipelineLayout;
		uint32_t firstset;
		uint32_t descriptorSetCount;
		DescriptorSet **ppDescriptorSets;
		uint32_t dynamicOffsetCount;	//dynamic uniform or storage buffer
		uint32_t *pDynamicOffsets;
	};
	struct BufferViewCreateInfo
	{
	};

	struct MemoryBarrier
	{
		AccessFlagBits srcAccessMask;	
		AccessFlagBits dstAccessMask;	
	};
	struct BufferMemoryBarrier
	{
		AccessFlagBits srcAccessMask;
		AccessFlagBits dstAccessMask;
//		uint32_t srcQueueFamilyIndex;
//		uint32_t dstQueueFamilyIndex;
		Buffer *pBuffer;
		uint64_t offset;
		uint64_t size;
	};
	struct ImageMemoryBarrier
	{
		AccessFlagBits srcAccessMask;
		AccessFlagBits dstAccessMask;
		ImageLayout oldImageLayout;
		ImageLayout newImageLayout;
	//	uint32_t srcQueueFamilyIndex;
	//	uint32_t dstQueueFamilyIndex;
		Image *pImage;
		ImageSubresourceRange subresourceRange;
	};
	struct PipelineBarrierInfo
	{
		PipelineStageFlagBits srcStageMask;
		PipelineStageFlagBits dstStageMask;
		DependencyFlagBits dependencyFlags;
		std::vector<MemoryBarrier> memoryBarriers;
		std::vector<BufferMemoryBarrier> bufferMemoryBarriers;
		std::vector<ImageMemoryBarrier> imageMemoryBarriers;
	};
} // namespace Shit
