/**
 * @file VKDevice.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKDevice.hpp"
#include <renderer/ShitWindow.hpp>
#include "VKSwapchain.hpp"
#include "VKShader.hpp"
#include "VKCommandPool.hpp"
#include "VKCommandBuffer.hpp"
#include "VKDevice.hpp"
#include "VKQueue.hpp"
#include "VKBuffer.hpp"
#include "VKImage.hpp"
#include "VKDescriptor.hpp"
#include "VKSampler.hpp"
#include "VKPipeline.hpp"
#include "VKRenderPass.hpp"
#include "VKFramebuffer.hpp"
#include "VKSurface.hpp"
#include "VKSemaphore.hpp"
#include "VKDevice.hpp"
#include "VKFence.hpp"

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

		//TODO: set physical device  features, put this outside
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
		CreateOneTimeCommandPool();
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
	Pipeline *VKDevice::Create(const ComputePipelineCreateInfo &createInfo)
	{
		mPipelines.emplace_back(std::make_unique<VKComputePipeline>(mDevice, createInfo));
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
	Buffer *VKDevice::Create(const BufferCreateInfo &createInfo, const void *pData)
	{
		mBuffers.emplace_back(std::make_unique<VKBuffer>(mDevice, GetPhysicalDevice(), createInfo));

		if (pData)
		{
			if (static_cast<bool>(createInfo.memoryPropertyFlags & MemoryPropertyFlagBits::HOST_VISIBLE_BIT))
			{
				Buffer *buffer = mBuffers.back().get();
				void *data;
				buffer->MapMemory(0, createInfo.size, &data);
				memcpy(data, pData, static_cast<size_t>(createInfo.size));
				buffer->UnMapMemory();
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
				stagingbuffer.MapMemory(0, createInfo.size, &data);
				memcpy(data, pData, static_cast<size_t>(createInfo.size));
				stagingbuffer.UnMapMemory();

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
	Image *VKDevice::Create(const ImageCreateInfo &createInfo, const void *pData)
	{
		mImages.emplace_back(std::make_unique<VKImage>(this, createInfo, pData));
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
		struct Temp
		{
			std::vector<VkDescriptorImageInfo> imagesInfo;
			std::vector<VkDescriptorBufferInfo> buffersInfo;
			std::vector<VkBufferView> texelBufferViews;
		};
		auto count = descriptorWrites.size();
		std::vector<VkWriteDescriptorSet> writes(count);
		std::vector<Temp> temp(count);
		std::transform(std::execution::par, descriptorWrites.begin(), descriptorWrites.end(), temp.begin(), [&](auto &&e) {
			Temp a;
			std::visit(
				overloaded{
					[&a](const std::vector<DescriptorImageInfo> &val) {
						a.imagesInfo.resize(val.size());
						std::transform(std::execution::par, val.begin(), val.end(), a.imagesInfo.begin(), [](auto &&image) {
							return VkDescriptorImageInfo{
								image.pSampler ? static_cast<VKSampler *>(image.pSampler)->GetHandle() : VK_NULL_HANDLE,
								static_cast<VKImageView *>(image.pImageView)->GetHandle(),
								Map(image.imageLayout)};
						});
					},
					[&a](const std::vector<DescriptorBufferInfo> &val) {
						a.buffersInfo.resize(val.size());
						std::transform(std::execution::par, val.begin(), val.end(), a.buffersInfo.begin(), [](auto &&buffer) {
							return VkDescriptorBufferInfo{
								static_cast<VKBuffer *>(buffer.pBuffer)->GetHandle(),
								buffer.offset,
								buffer.range};
						});
					},
					[&a](const std::vector<BufferView *> &val) {
						a.texelBufferViews.resize(val.size());
						//TODO: buffer views
					},
				},
				e.values);
			return a;
		});
		while (count-- > 0)
		{
			auto descriptorCount = temp[count].imagesInfo.size();
			if (!descriptorCount)
				descriptorCount = temp[count].buffersInfo.size();
			if (!descriptorCount)
				descriptorCount = temp[count].texelBufferViews.size();
			writes[count] = {
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				nullptr,
				static_cast<VKDescriptorSet *>(descriptorWrites[count].pDstSet)->GetHandle(),
				descriptorWrites[count].dstBinding,
				0, //e.dstArrayElement,
				static_cast<uint32_t>(descriptorCount),
				Map(descriptorWrites[count].descriptorType),
				temp[count].imagesInfo.data(),
				temp[count].buffersInfo.data(),
				temp[count].texelBufferViews.data()};
		};

		//	auto descriptorCount = imagesInfo.size();
		//	if (!descriptorCount)
		//		descriptorCount = buffersInfo.size();
		//	if (!descriptorCount)
		//		descriptorCount = texelBufferViews.size();
		//	return VkWriteDescriptorSet{
		//		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		//		.pNext = nullptr,
		//		.dstSet = static_cast<VKDescriptorSet *>(e.pDstSet)->GetHandle(),
		//		.dstBinding = e.dstBinding,
		//		.dstArrayElement = 0, //e.dstArrayElement,
		//		.descriptorCount = static_cast<uint32_t>(descriptorCount),
		//		.descriptorType = Map(e.descriptorType),
		//		.pImageInfo = e.imagesInfo.data(),
		//		.pBufferInfo = e.buffersInfo.data(),
		//		.pTexelBufferView = e.texelBufferViews.data()};
		//});
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
