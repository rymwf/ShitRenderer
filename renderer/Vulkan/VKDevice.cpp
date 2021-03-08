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
#include "VKSurface.h"

namespace Shit
{

	VKDevice::VKDevice(PhysicalDevice physicalDevice) : mPhysicalDevice(static_cast<VkPhysicalDevice>(physicalDevice))
	{
		VK::queryQueueFamilyProperties(mPhysicalDevice, mQueueFamilyProperties);

		std::vector<VkExtensionProperties> properties;
		VK::queryDeviceExtensionProperties(mPhysicalDevice, properties);

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
		vkGetPhysicalDeviceFeatures(mPhysicalDevice, &deviceFeatures);

		deviceFeatures.sampleRateShading = true;

		std::vector<VkDeviceQueueCreateInfo> queueInfos;
		std::vector<float> queuePriorities;
		for (uint32_t i = 0, len = mQueueFamilyProperties.size(); i < len; ++i)
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
			extensionNames.size(),
			extensionNames.data(),
			&deviceFeatures};
		if (vkCreateDevice(mPhysicalDevice, &deviceInfo, nullptr, &mDevice) != VK_SUCCESS)
			THROW("create logical device failed");

		//create a transfer command pool for memory transfer operation
		auto transferQueueFamilyIndex = GetQueueFamilyIndexByFlag(
			QueueFlagBits::TRANSFER_BIT | QueueFlagBits::GRAPHICS_BIT | QueueFlagBits::COMPUTE_BIT,
			{});
		mpOneTimeCommandPool = CreateCommandPool(
			{CommandPoolCreateFlagBits::TRANSIENT_BIT,
			 transferQueueFamilyIndex.value()});
		mpOneTimeCommandQueue = CreateDeviceQueue({transferQueueFamilyIndex->index, 0});
	}

	std::optional<QueueFamilyIndex> VKDevice::GetPresentQueueFamilyIndex(ShitWindow *pWindow)
	{
		auto index = VK::findQueueFamilyIndexPresent(
			mPhysicalDevice,
			static_cast<uint32_t>(mQueueFamilyProperties.size()),
			static_cast<VKSurface *>(pWindow->GetSurface())->GetHandle());
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
	Swapchain *VKDevice::CreateSwapchain(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow)
	{
		VkSurfaceKHR surface = static_cast<VKSurface *>(pWindow->GetSurface())->GetHandle();

		auto presentQueueFamilyIndex = GetPresentQueueFamilyIndex(pWindow);

		if (!presentQueueFamilyIndex.has_value())
			THROW("current device do not support present to surface");

		//set format
		std::vector<VkSurfaceFormatKHR> surfaceFormats;

		VK::querySurfaceFormats(mPhysicalDevice, surface, surfaceFormats);

		VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0]; //default rgba8unorm

		VkFormat format = Map(createInfo.format);
		VkColorSpaceKHR colorSpace = Map(createInfo.colorSpace);
		//change to srgba8
		for (auto &&e : surfaceFormats)
		{
			if (e.format == format && e.colorSpace == colorSpace)
			{
				surfaceFormat = e;
				break;
			}
		}

		std::vector<VkPresentModeKHR> presentModes;
		VK::querySurfacePresentModes(mPhysicalDevice, surface, presentModes);
		VkPresentModeKHR presentMode;
		auto dstmode = Map(createInfo.presentMode);
		for (auto &&e : presentModes)
		{
			if (dstmode == e)
			{
				presentMode = e;
				break;
			}
		}
		pWindow->SetSwapchain(std::make_shared<VKSwapchain>(
			mDevice,
			createInfo,
			surface,
			surfaceFormat,
			presentMode,
			presentQueueFamilyIndex.value()));
		return pWindow->GetSwapchain();
	}

	Shader *VKDevice::CreateShader(const ShaderCreateInfo &createInfo)
	{
		mShaders.emplace_back(std::make_unique<VKShader>(mDevice, createInfo));
		return mShaders.back().get();
	}
	GraphicsPipeline *VKDevice::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo)
	{
		return nullptr;
	}
	CommandPool *VKDevice::CreateCommandPool(const CommandPoolCreateInfo &createInfo)
	{
		mCommandPools.emplace_back(std::make_unique<VKCommandPool>(mDevice, createInfo));
		return mCommandPools.back().get();
	}
	Queue *VKDevice::CreateDeviceQueue(const QueueCreateInfo &createInfo)
	{
		mQueues.emplace_back(std::make_unique<VKQueue>(mDevice, createInfo));
		return mQueues.back().get();
	}
	Result VKDevice::WaitForFence(Fence *fence, uint64_t timeout)
	{
		return Result::SUCCESS;
	}
	Buffer *VKDevice::CreateBuffer(const BufferCreateInfo &createInfo, void *pData)
	{
		mBuffers.emplace_back(std::make_unique<VKBuffer>(mDevice, mPhysicalDevice, createInfo));

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
					MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::DEVICE_LOCAL_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT,
				};
				VKBuffer stagingbuffer{mDevice, mPhysicalDevice, stagingBufferCreateInfo};
				void *data;
				stagingbuffer.MapBuffer(0, createInfo.size, &data);
				memcpy(data, pData, static_cast<size_t>(createInfo.size));
				stagingbuffer.UnMapBuffer();

				auto commandBuffer = GetOneTimeCommandBuffer();
				commandBuffer->CopyBuffer({&stagingbuffer,
										   mBuffers.back().get(),
										   {{0,
											 0,
											 createInfo.size}}});
				ExecuteOneTimeCommandBuffer(commandBuffer);
			}
			else
			{
				THROW("buffer should be created with a BufferUsageFlagBits::TRANSFER_DST_BIT or MemoryPropertyFlagBits::HOST_VISIBLE_BIT");
			}
		}
		return mBuffers.back().get();
	}
	CommandBuffer *VKDevice::GetOneTimeCommandBuffer()
	{
		static std::vector<CommandBuffer *> cmdBuffers;
		if (cmdBuffers.empty())
			mpOneTimeCommandPool->CreateCommandBuffers({CommandBufferLevel::PRIMARY, 1}, cmdBuffers);
		return cmdBuffers.back();
	}
	void VKDevice::ExecuteOneTimeCommandBuffer(CommandBuffer *pCommandBuffer)
	{
		mpOneTimeCommandQueue->Submit({{{}, {pCommandBuffer}}}, nullptr);
	}
	Image *VKDevice::CreateImage(const ImageCreateInfo &createInfo, void *pData)
	{
		mImages.emplace_back(std::make_unique<VKImage>(mDevice, mPhysicalDevice, createInfo));
		if (pData)
		{
			uint64_t size = createInfo.extent.width * createInfo.extent.height * createInfo.extent.depth * GetFormatSize(createInfo.format);

			BufferCreateInfo stagingBufferCreateInfo{
				{},
				size,
				BufferUsageFlagBits::TRANSFER_SRC_BIT,
				MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::DEVICE_LOCAL_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT,
			};

			VKBuffer stagingbuffer{mDevice, mPhysicalDevice, stagingBufferCreateInfo};
			void *data;
			stagingbuffer.MapBuffer(0, size, &data);
			memcpy(data, pData, static_cast<size_t>(size));
			stagingbuffer.UnMapBuffer();

			auto commandBuffer = GetOneTimeCommandBuffer();

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
			vkCmdPipelineBarrier(static_cast<VKCommandBuffer *>(commandBuffer)->GetHandle(),
								 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
								 VK_PIPELINE_STAGE_TRANSFER_BIT,
								 0,
								 0, nullptr,
								 0, nullptr,
								 1, &barrier);
			commandBuffer->CopyBufferToImage({&stagingbuffer,
											  mImages.back().get(),
											  {{0,
												0,
												0,
												{0, 0, createInfo.arrayLayers},
												{},
												createInfo.extent}}});
			ExecuteOneTimeCommandBuffer(commandBuffer);
		}
		return mImages.back().get();
	}
	ImageView *VKDevice::CreateImageView(const ImageViewCreateInfo &createInfo)
	{
		mImageViews.emplace_back(std::make_unique<VKImageView>(mDevice, createInfo));
		return mImageViews.back().get();
	}
	DescriptorSetLayout *VKDevice::CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo &createInfo)
	{
		mDescriptorSetLayouts.emplace_back(std::make_unique<VKDescriptorSetLayout>(mDevice, createInfo));
		return mDescriptorSetLayouts.back().get();
	}
	PipelineLayout *VKDevice::CreatePipelineLayout(const PipelineLayoutCreateInfo &createInfo)
	{
		mPipelineLayouts.emplace_back(std::make_unique<VKPipelineLayout>(mDevice, createInfo));
		return mPipelineLayouts.back().get();
	}
	RenderPass *VKDevice::CreateRenderPass(const RenderPassCreateInfo &createInfo)
	{
		mRenderPasses.emplace_back(std::make_unique<VKRenderPass>(mDevice, createInfo));
		return mRenderPasses.back().get();
	}
	Framebuffer *VKDevice::CreateFramebuffer(const FramebufferCreateInfo &createInfo)
	{
		mFramebuffer.emplace_back(std::make_unique<VKFramebuffer>(mDevice, createInfo));
		return mFramebuffer.back().get();
	}
} // namespace Shit
