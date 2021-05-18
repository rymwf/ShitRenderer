/**
 * @file VKBufferView.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKBufferView.hpp"
#include "VKBuffer.hpp"

namespace Shit
{
	VKBufferView::VKBufferView(VkDevice device, const BufferViewCreateInfo &createInfo)
		: BufferView(createInfo), mDevice(device)
	{
		VkBufferViewCreateInfo info{
			VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
			nullptr, //next
			0,		 //flags
			static_cast<const VKBuffer *>(createInfo.pBuffer)->GetHandle(),
			Map(createInfo.format),
			createInfo.offset,
			createInfo.range};
		CHECK_VK_RESULT(vkCreateBufferView(device, &info, nullptr, &mHandle))
	}
}
