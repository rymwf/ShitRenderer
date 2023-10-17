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
#include "GLState.hpp"
namespace Shit {
GLBufferView::GLBufferView(GLStateManager *pStateManager, const BufferViewCreateInfo &createInfo)
    : BufferView(createInfo), mpStateManager(pStateManager) {
    auto buffer = static_cast<const GLBuffer *>(createInfo.pBuffer)->GetHandle();
    glGenTextures(1, &mHandle);
    mpStateManager->PushTextureUnit(0, GL_TEXTURE_BUFFER, mHandle);
    glTexBuffer(GL_TEXTURE_BUFFER, MapInternalFormat(mCreateInfo.format), buffer);
    // glTexBufferRange(GL_TEXTURE_BUFFER, MapInternalFormat(mCreateInfo.format),
    // buffer, static_cast<GLintptr>(mCreateInfo.offset),
    // static_cast<GLsizeiptr>(mCreateInfo.range));
    mpStateManager->PopTextureUnit();
}
}  // namespace Shit
