/**
 * @file GLBufferView.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLBufferView.hpp"
#include "GLBuffer.hpp"
namespace Shit
{
	GLBufferView::GLBufferView(GLStateManager *pStateManager, const BufferViewCreateInfo &createInfo)
		: BufferView(createInfo), mpStateManager(pStateManager)
	{
		//		mHandle = static_cast<GLBuffer *>(createInfo.pBuffer)->GetHandle();
		LOG("not implemented yet");
	}
}
