/**
 * @file VKCommandBuffer.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitCommandBuffer.h>
#include "VKPrerequisites.h"
namespace Shit
{

	class VKCommandBuffer final : public CommandBuffer
	{
		VkCommandBuffer mHandle;
		VkDevice mDevice;
		VkCommandPool mCommandPool;

	public:
		VKCommandBuffer(VkDevice device, VkCommandPool commandPool, const CommandBufferCreateInfo &createInfo);
		constexpr VkCommandBuffer GetHandle()
		{
			return mHandle;
		}
		~VKCommandBuffer() override
		{
			vkFreeCommandBuffers(mDevice, mCommandPool, 1, &mHandle);
		}
		void Begin(const CommandBufferBeginInfo &beginInfo) override;
		void End() override;
		void ExecuteSecondaryCommandBuffer(const std::vector<CommandBuffer *> &secondaryCommandBuffers) override;
		void BeginRenderPass(const RenderPassBeginInfo &beginInfo, const SubpassBeginInfo &subpassBeginInfo) override;
		void EndRenderPass() override;
		void NextSubpass(const SubpassBeginInfo &subpassBeginInfo) override;

		void CopyBuffer(const CopyBufferInfo &copyInfo) override;
		void CopyImage(const CopyImageInfo &copyInfo) override;
		void CopyBufferToImage(const CopyBufferToImageInfo &copyInfo) override;
		void CopyImageToBuffer(const CopyImageToBufferInfo &copyInfo) override;
		void BlitImage(const BlitImageInfo &blitInfo) override;
	};
}
