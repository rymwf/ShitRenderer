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
		auto buffer = static_cast<const GLBuffer *>(createInfo.pBuffer)->GetHandle();
		glGenTextures(1, &mHandle);
		mpStateManager->BindTextureUnit(0, GL_TEXTURE_BUFFER, mHandle);
		glTexBufferRange(GL_TEXTURE_BUFFER, MapInternalFormat(mCreateInfo.format), buffer, mCreateInfo.offset, mCreateInfo.range);
		mpStateManager->PopBuffer();
	}
}
