/**
 * @file GLRenderPass.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitRenderPass.hpp>
namespace Shit
{
	class GLRenderPass final : public RenderPass
	{
		std::vector<ClearValue> mAttachmentsClearValues;

	public:
		GLRenderPass(const RenderPassCreateInfo &createInfo)
			: RenderPass(createInfo)
		{
			mAttachmentsClearValues.resize(createInfo.attachments.size());
		}
		void SetAttachmentClearValue(size_t index, const ClearValue &clearValue)
		{
			mAttachmentsClearValues[index] = clearValue;
		}
		void SetAttachmentClearValue(const std::vector<ClearValue> &clearValues)
		{
			mAttachmentsClearValues = clearValues;
		}
		const ClearValue *GetAttachmentClearValuePtr(int index) const
		{
			return &mAttachmentsClearValues[index];
		}
	};
};