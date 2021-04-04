/**
 * @file VKCommandBuffer.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitCommandBuffer.hpp>
#include "VKPrerequisites.hpp"
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
		}
		void Destroy()
		{
			vkFreeCommandBuffers(mDevice, mCommandPool, 1, &mHandle);
		}
		void ExecuteSecondaryCommandBuffer(const ExecuteSecondaryCommandBufferInfo &secondaryCommandBufferInfo) override;

		void Reset(CommandBufferResetFlatBits flags) override;
		void Begin(const CommandBufferBeginInfo &beginInfo) override;
		void End() override;
		void BeginRenderPass(const RenderPassBeginInfo &beginInfo) override;
		void EndRenderPass() override;
		void NextSubpass(SubpassContents subpassContents) override;
		void BindPipeline(const BindPipelineInfo& info) override;

		void CopyBuffer(const CopyBufferInfo &copyInfo) override;
		void CopyImage(const CopyImageInfo &copyInfo) override;
		void CopyBufferToImage(const CopyBufferToImageInfo &copyInfo) override;
		void CopyImageToBuffer(const CopyImageToBufferInfo &copyInfo) override;
		void BlitImage(const BlitImageInfo &blitInfo) override;

		void BindVertexBuffer(const BindVertexBufferInfo& info)override;
		void BindIndexBuffer(const BindIndexBufferInfo& info)override;
		void BindDescriptorSets(const BindDescriptorSetsInfo &info) override;

		void Draw(const DrawIndirectCommand &info) override;
		void DrawIndirect(const DrawIndirectInfo &info) override;
		void DrawIndirectCount(const DrawIndirectCountInfo &info) override;
		void DrawIndexed(const DrawIndexedIndirectCommand &info) override;
		void DrawIndexedIndirect(const DrawIndirectInfo &info) override;
		void DrawIndexedIndirectCount(const DrawIndirectCountInfo &info) override;

		void PipeplineBarrier(const PipelineBarrierInfo &info) override;
	};
}
