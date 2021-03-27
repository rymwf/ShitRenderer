/**
 * @file VKDevice.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKDevice.h"
#include <renderer/ShitWindow.h>
#include "VKSwapchain.h"
#include "VKShader.h"
#include "VKCommandPool.h"
#include "VKCommandBuffer.h"
#include "VKDevice.h"
#include "VKQueue.h"
#include "VKBuffer.h"
#include "VKImage.h"
#include "VKDescriptor.h"
#include "VKSampler.h"
#include "VKPipeline.h"
#include "VKRenderPass.h"
#include "VKFramebuffer.h"
#include "VKSurface.h"
#include "VKSemaphore.h"
#include "VKDevice.h"
#include "VKFence.h"

namespace Shit
{

	VKDevice::VKDevice(const DeviceCreateInfo &createInfo) : Device(createInfo)
	{
		VkPhysicalDevice physicalDevice = GetPhysicalDevice();

		VK::queryQueueFamilyProperties(physicalDevice, mQueueFamilyProperties);

		std::vector<VkExtensionProperties> properties;
		VK::queryDeviceExtensionProperties(physicalDevice, properties);

		std::vector<const char *> extensionNames;
		extensionNames.reserve(properties.size());
		for (auto &extensionProperty : properties)
		{
			LOG(extensionProperty.extensionName);
			LOG(extensionProperty.specVersion);
			extensionNames.emplace_back(extensionProperty.extensionName);
		}

		//physical device  features
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		deviceFeatures.sampleRateShading = true;

		std::vector<VkDeviceQueueCreateInfo> queueInfos;
		std::vector<float> queuePriorities;
		for (uint32_t i = 0, len = static_cast<uint32_t>(mQueueFamilyProperties.size()); i < len; ++i)
		{
			queuePriorities.clear();
			//TODO: how to arrange queue priorities
			queuePriorities.resize(mQueueFamilyProperties[i].queueCount, 1.f);
			queueInfos.emplace_back(
				VkDeviceQueueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
										NULL,
										0,
										i,									  //queue family index
										mQueueFamilyProperties[i].queueCount, //queue count
										queuePriorities.data()});
		}

		VkDeviceCreateInfo deviceInfo{
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			NULL,
			0,
			static_cast<uint32_t>(queueInfos.size()),
			queueInfos.data(),
			0, //deprecated
			0, //deprecated
			static_cast<uint32_t>(extensionNames.size()),
			extensionNames.data(),
			&deviceFeatures};
		if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &mDevice) != VK_SUCCESS)
			THROW("create logical device failed");

		//create a transfer command pool for memory transfer operation
		auto transferQueueFamilyIndex = GetQueueFamilyIndexByFlag(
			QueueFlagBits::TRANSFER_BIT | QueueFlagBits::GRAPHICS_BIT | QueueFlagBits::COMPUTE_BIT,
			{});
		mpOneTimeCommandPool = Create(
			//{CommandPoolCreateFlagBits::TRANSIENT_BIT,
			{CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT,
			 transferQueueFamilyIndex.value()});
		mpOneTimeCommandQueue = Create({transferQueueFamilyIndex->index, 0});
	}

	std::optional<QueueFamilyIndex> VKDevice::GetPresentQueueFamilyIndex(ShitWindow *pWindow)
	{
		auto index = VK::findQueueFamilyIndexPresent(
			GetPhysicalDevice(),
			static_cast<uint32_t>(mQueueFamilyProperties.size()),
			static_cast<const VKSurface *>(pWindow->GetSurfacePtr())->GetHandle());
		if (index.has_value())
			return std::optional<QueueFamilyIndex>{{*index, mQueueFamilyProperties[*index].queueCount}};
		else
			return std::nullopt;
	}

	std::optional<QueueFamilyIndex> VKDevice::GetQueueFamilyIndexByFlag(QueueFlagBits flag, const std::unordered_set<uint32_t> &skipIndices)
	{
		auto index = VK::findQueueFamilyIndexByFlag(mQueueFamilyProperties, Map(flag), skipIndices);
		if (index.has_value())
			return std::optional<QueueFamilyIndex>{{*index, mQueueFamilyProperties[*index].queueCount}};
		else
			return std::nullopt;
	}
	void VKDevice::GetWindowPixelFormats(const ShitWindow *pWindow, std::vector<WindowPixelFormat> &formats)
	{
		auto surface = static_cast<const VKSurface *>(pWindow->GetSurfacePtr())->GetHandle();
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		VK::querySurfaceFormats(GetPhysicalDevice(), surface, surfaceFormats);
		formats.resize(surfaceFormats.size());
		std::transform(std::execution::par, surfaceFormats.begin(), surfaceFormats.end(), formats.begin(), [](auto &&e) {
			return WindowPixelFormat{Map(e.format), Map(e.colorSpace)};
		});
	}
	void VKDevice::GetPresentModes(const ShitWindow *pWindow, std::vector<PresentMode> &presentModes)
	{
		auto surface = static_cast<const VKSurface *>(pWindow->GetSurfacePtr())->GetHandle();
		std::vector<VkPresentModeKHR> modes;
		VK::querySurfacePresentModes(GetPhysicalDevice(), surface, modes);
		presentModes.resize(modes.size());
		std::transform(std::execution::par, modes.begin(), modes.end(), presentModes.begin(), [](auto &&e) {
			return Map(e);
		});
	}
	Swapchain *VKDevice::Create(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow)
	{
		mSwapchains.emplace_back(std::make_unique<VKSwapchain>(this, pWindow, createInfo));
		pWindow->SetSwapchain(mSwapchains.back().get());
		return mSwapchains.back().get();
	}

	Shader *VKDevice::Create(const ShaderCreateInfo &createInfo)
	{
		mShaders.emplace_back(std::make_unique<VKShader>(mDevice, createInfo));
		return mShaders.back().get();
	}
	Pipeline *VKDevice::Create(const GraphicsPipelineCreateInfo &createInfo)
	{
		mPipelines.emplace_back(std::make_unique<VKGraphicsPipeline>(mDevice, createInfo));
		return mPipelines.back().get();
	}
	CommandPool *VKDevice::Create(const CommandPoolCreateInfo &createInfo)
	{
		mCommandPools.emplace_back(std::make_unique<VKCommandPool>(mDevice, createInfo));
		return mCommandPools.back().get();
	}
	Queue *VKDevice::Create(const QueueCreateInfo &createInfo)
	{
		mQueues.emplace_back(std::make_unique<VKQueue>(mDevice, createInfo));
		return mQueues.back().get();
	}
	Buffer *VKDevice::Create(const BufferCreateInfo &createInfo, void *pData)
	{
		mBuffers.emplace_back(std::make_unique<VKBuffer>(mDevice, GetPhysicalDevice(), createInfo));

		if (pData)
		{
			if (static_cast<bool>(createInfo.memoryPropertyFlags & MemoryPropertyFlagBits::HOST_VISIBLE_BIT))
			{
				Buffer *buffer = mBuffers.back().get();
				void *data;
				buffer->MapBuffer(0, createInfo.size, &data);
				memcpy(data, pData, static_cast<size_t>(createInfo.size));
				buffer->UnMapBuffer();
			}
			else if (static_cast<bool>(createInfo.usage & BufferUsageFlagBits::TRANSFER_DST_BIT))
			{
				BufferCreateInfo stagingBufferCreateInfo{
					{},
					createInfo.size,
					BufferUsageFlagBits::TRANSFER_SRC_BIT,
					MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT,
				};
				VKBuffer stagingbuffer{mDevice, GetPhysicalDevice(), stagingBufferCreateInfo};
				void *data;
				stagingbuffer.MapBuffer(0, createInfo.size, &data);
				memcpy(data, pData, static_cast<size_t>(createInfo.size));
				stagingbuffer.UnMapBuffer();

				ExecuteOneTimeCommands([&](CommandBuffer *pCommandBuffer) {
					BufferCopy bufferCopy{0, 0, createInfo.size};
					pCommandBuffer->CopyBuffer({&stagingbuffer,
												mBuffers.back().get(),
												1,
												&bufferCopy});
				});
			}
			else
			{
				THROW("buffer should be created with a BufferUsageFlagBits::TRANSFER_DST_BIT or MemoryPropertyFlagBits::HOST_VISIBLE_BIT");
			}
		}
		return mBuffers.back().get();
	}
	void VKDevice::ExecuteOneTimeCommands(const std::function<void(CommandBuffer *)> &func)
	{
		static CommandBuffer *pOneTimeCommandBuffer;
		if (!pOneTimeCommandBuffer)
		{
			std::vector<CommandBuffer *> cmdBuffers;
			if (cmdBuffers.empty())
				mpOneTimeCommandPool->CreateCommandBuffers({CommandBufferLevel::PRIMARY, 1}, cmdBuffers);
			pOneTimeCommandBuffer = cmdBuffers[0];
		}
		CommandBufferBeginInfo beginInfo{
			CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT};
		pOneTimeCommandBuffer->Begin(beginInfo);
		func(pOneTimeCommandBuffer);
		pOneTimeCommandBuffer->End();
		mpOneTimeCommandQueue->Submit({{{}, {pOneTimeCommandBuffer}}}, nullptr);
		mpOneTimeCommandQueue->WaitIdle();
		pOneTimeCommandBuffer->Reset(CommandBufferResetFlatBits::RELEASE_RESOURCES_BIT);
	}
	Image *VKDevice::Create(const ImageCreateInfo &createInfo, void *pData)
	{
		mImages.emplace_back(std::make_unique<VKImage>(mDevice, GetPhysicalDevice(), createInfo));
		if (pData)
		{
			uint64_t size = createInfo.extent.width * createInfo.extent.height * createInfo.extent.depth * GetFormatSize(createInfo.format);

			BufferCreateInfo stagingBufferCreateInfo{
				{},
				size,
				BufferUsageFlagBits::TRANSFER_SRC_BIT,
				MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT,
			};

			VKBuffer stagingbuffer{mDevice, GetPhysicalDevice(), stagingBufferCreateInfo};
			void *data;
			stagingbuffer.MapBuffer(0, size, &data);
			memcpy(data, pData, static_cast<size_t>(size));
			stagingbuffer.UnMapBuffer();

			ExecuteOneTimeCommands([&](CommandBuffer *pCommandBuffer) {
				Image *image = mImages.back().get();
				VkImageAspectFlags aspectFlags = GetImageAspectFromFormat(createInfo.format);

				//1. transfer layout from undefined to transder destination
				VkImageMemoryBarrier barrier{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					nullptr,
					0,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_QUEUE_FAMILY_IGNORED,
					VK_QUEUE_FAMILY_IGNORED,
					static_cast<VKImage *>(image)->GetHandle(),
					{aspectFlags,
					 0,
					 createInfo.mipLevels,
					 0,
					 createInfo.arrayLayers}};
				vkCmdPipelineBarrier(static_cast<VKCommandBuffer *>(pCommandBuffer)->GetHandle(),
									 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //src stage mask
									 VK_PIPELINE_STAGE_TRANSFER_BIT,	//dst stage mask
									 0,									//dependency flags
									 0, nullptr,						//memory barriers
									 0, nullptr,						//buffer memory barriers
									 1, &barrier);						//image memory barriers
				//2. copy image
				BufferImageCopy bufferImageCopy{
					0,
					0,
					0,
					{0, 0, createInfo.arrayLayers},
					{},
					createInfo.extent};
				pCommandBuffer->CopyBufferToImage({&stagingbuffer,
												   image,
												   1,
												   &bufferImageCopy});
				//3. transfer layout from trander destiation to shader reading
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				vkCmdPipelineBarrier(static_cast<VKCommandBuffer *>(pCommandBuffer)->GetHandle(),
									 VK_PIPELINE_STAGE_TRANSFER_BIT,
									 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
									 0,
									 0, nullptr,
									 0, nullptr,
									 1, &barrier);
			});
		}
		return mImages.back().get();
	}
	ImageView *VKDevice::Create(const ImageViewCreateInfo &createInfo)
	{
		mImageViews.emplace_back(std::make_unique<VKImageView>(mDevice, createInfo));
		return mImageViews.back().get();
	}
	DescriptorSetLayout *VKDevice::Create(const DescriptorSetLayoutCreateInfo &createInfo)
	{
		mDescriptorSetLayouts.emplace_back(std::make_unique<VKDescriptorSetLayout>(mDevice, createInfo));
		return mDescriptorSetLayouts.back().get();
	}
	PipelineLayout *VKDevice::Create(const PipelineLayoutCreateInfo &createInfo)
	{
		mPipelineLayouts.emplace_back(std::make_unique<VKPipelineLayout>(mDevice, createInfo));
		return mPipelineLayouts.back().get();
	}
	RenderPass *VKDevice::Create(const RenderPassCreateInfo &createInfo)
	{
		mRenderPasses.emplace_back(std::make_unique<VKRenderPass>(mDevice, createInfo));
		return mRenderPasses.back().get();
	}
	Framebuffer *VKDevice::Create(const FramebufferCreateInfo &createInfo)
	{
		mFramebuffers.emplace_back(std::make_unique<VKFramebuffer>(mDevice, createInfo));
		return mFramebuffers.back().get();
	}
	Semaphore *VKDevice::Create(const SemaphoreCreateInfo &createInfo)
	{
		mSemaphores.emplace_back(std::make_unique<VKSemaphore>(mDevice, createInfo));
		return mSemaphores.back().get();
	}
	Fence *VKDevice::Create(const FenceCreateInfo &createInfo)
	{
		mFences.emplace_back(std::make_unique<VKFence>(mDevice, createInfo));
		return mFences.back().get();
	}
	Sampler *VKDevice::Create(const SamplerCreateInfo &createInfo)
	{
		mSamplers.emplace_back(std::make_unique<VKSampler>(mDevice, createInfo));
		return mSamplers.back().get();
	}
	DescriptorPool *VKDevice::Create(const DescriptorPoolCreateInfo &createInfo)
	{
		mDescriptorPools.emplace_back(std::make_unique<VKDescriptorPool>(mDevice, createInfo));
		return mDescriptorPools.back().get();
	}
	void VKDevice::UpdateDescriptorSets(const std::vector<WriteDescriptorSet> &descriptorWrites, const std::vector<CopyDescriptorSet> &descriptorCopies)
	{
		std::vector<VkWriteDescriptorSet> writes(descriptorWrites.size());

		std::vector<VkDescriptorImageInfo> imagesInfo;
		std::vector<VkDescriptorBufferInfo> buffersInfo;
		std::vector<VkBufferView> texelBufferViews;

		std::transform(descriptorWrites.begin(), descriptorWrites.end(), writes.begin(), [&](auto &&e) {
			imagesInfo.clear();
			buffersInfo.clear();
			texelBufferViews.clear();
			std::visit(
				overloaded{
					[&imagesInfo](const std::vector<DescriptorImageInfo> &val) {
						imagesInfo.resize(val.size());
						std::transform(std::execution::par, val.begin(), val.end(), imagesInfo.begin(), [](auto &&image) {
							return VkDescriptorImageInfo{
								image.pSampler ? static_cast<VKSampler *>(image.pSampler)->GetHandle() : VK_NULL_HANDLE,
								static_cast<VKImageView *>(image.pImageView)->GetHandle(),
								Map(image.imageLayout)};
						});
					},
					[&buffersInfo](const std::vector<DescriptorBufferInfo> &val) {
						buffersInfo.resize(val.size());
						std::transform(std::execution::par, val.begin(), val.end(), buffersInfo.begin(), [](auto &&buffer) {
							return VkDescriptorBufferInfo{
								static_cast<VKBuffer *>(buffer.pBuffer)->GetHandle(),
								buffer.offset,
								buffer.range};
						});
					},
					[&texelBufferViews](const std::vector<BufferView *> &val) {
						texelBufferViews.resize(val.size());
						//TODO: buffer views
					},
				},
				e.values);

			auto descriptorCount = imagesInfo.size();
			if (!descriptorCount)
				descriptorCount = buffersInfo.size();
			if (!descriptorCount)
				descriptorCount = texelBufferViews.size();
			return VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = static_cast<VKDescriptorSet *>(e.pDstSet)->GetHandle(),
				.dstBinding = e.dstBinding,
				.dstArrayElement = 0, //e.dstArrayElement,
				.descriptorCount = static_cast<uint32_t>(descriptorCount),
				.descriptorType = Map(e.descriptorType),
				.pImageInfo = imagesInfo.data(),
				.pBufferInfo = buffersInfo.data(),
				.pTexelBufferView = texelBufferViews.data()};
		});
		std::vector<VkCopyDescriptorSet> copies(descriptorCopies.size());
		std::transform(std::execution::par, descriptorCopies.begin(), descriptorCopies.end(), copies.begin(), [](auto &&e) {
			return VkCopyDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
				.pNext = nullptr,
				.srcSet = static_cast<VKDescriptorSet *>(e.pSrcSet)->GetHandle(),
				.srcBinding = e.srcBinding,
				.srcArrayElement = e.srcArrayElement,
				.dstSet = static_cast<VKDescriptorSet *>(e.pDstSet)->GetHandle(),
				.dstBinding = e.dstBinding,
				.dstArrayElement = e.dstArrayElement,
				.descriptorCount = e.descriptorCount};
		});
		vkUpdateDescriptorSets(mDevice,
							   static_cast<uint32_t>(writes.size()),
							   writes.data(),
							   static_cast<uint32_t>(copies.size()),
							   copies.data());
	}
} // namespace Shit
