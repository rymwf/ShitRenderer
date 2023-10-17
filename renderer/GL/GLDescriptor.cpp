/**
 * @file GLDescriptor.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "GLDescriptor.hpp"

#include "GLBuffer.hpp"
#include "GLImage.hpp"
#include "GLSampler.hpp"
#include "GLState.hpp"

namespace Shit {
GLDescriptorSet::GLDescriptorSet(GLStateManager *pStateManager, const DescriptorSetLayout *pDescriptorSetLayout)
    : DescriptorSet(pDescriptorSetLayout), mpStateManager(pStateManager) {}
void GLDescriptorSet::Set(DescriptorType type, uint32_t binding, uint32_t count, DescriptorImageInfo const *values) {
    // the image2D need to specify it's format and it has a limited number of
    // formats, use sampler2D instead
    if (type == DescriptorType::COMBINED_IMAGE_SAMPLER || type == DescriptorType::INPUT_ATTACHMENT) {
        for (uint32_t i = 0; i < count; ++i) {
            auto &&e = values[i];
            if (e.pSampler)
                const_cast<GLImageView *>(static_cast<GLImageView const *>(e.pImageView))->SetSampler(e.pSampler);
            mTextureDescriptorsInfo[binding++] = DescriptorInfo{type, e};
        }
    } else if (type == DescriptorType::STORAGE_IMAGE) {
        for (uint32_t i = 0; i < count; ++i) {
            auto &&e = values[i];
            if (e.pSampler)
                const_cast<GLImageView *>(static_cast<GLImageView const *>(e.pImageView))->SetSampler(e.pSampler);
            mImageTextureDescriptorsInfo[binding++] = DescriptorInfo{type, e};
        }
    } else
        ST_LOG("descriptor ", (int)type, "type not supported yet");
}
void GLDescriptorSet::Set(DescriptorType type, uint32_t binding, uint32_t count, BufferView const *const *values) {
    if (type == DescriptorType::UNIFORM_TEXEL_BUFFER) {
        for (uint32_t i = 0; i < count; ++i) mTextureDescriptorsInfo[binding++] = DescriptorInfo{type, values[i]};
    } else if (type == DescriptorType::STORAGE_TEXEL_BUFFER) {
        for (uint32_t i = 0; i < count; ++i) mImageTextureDescriptorsInfo[binding++] = DescriptorInfo{type, values[i]};
    } else
        ST_LOG("descriptor ", (int)type, "type not supported yet");
}
void GLDescriptorSet::Set(DescriptorType type, uint32_t binding, uint32_t count, DescriptorBufferInfo const *values) {
    if (type == DescriptorType::UNIFORM_BUFFER || type == DescriptorType::UNIFORM_BUFFER_DYNAMIC) {
        for (uint32_t i = 0; i < count; ++i) mUniformBufferDescriptorsInfo[binding++] = DescriptorInfo{type, values[i]};
    } else if (type == DescriptorType::STORAGE_BUFFER || type == DescriptorType::STORAGE_BUFFER_DYNAMIC) {
        for (uint32_t i = 0; i < count; ++i) mStorageBufferDescriptorsInfo[binding++] = DescriptorInfo{type, values[i]};
    } else
        ST_LOG("descriptor ", (int)type, "type not supported yet");
}
void GLDescriptorSet::Bind(uint32_t const *pDynamicOffsets, uint32_t &index) {
    for (GLuint i = 0; i < IMAGE_TEXTURE_DESCRIPTOR_MAX_COUNT; ++i) {
        auto &&descriptorInfo = mImageTextureDescriptorsInfo[i];
        if (descriptorInfo.type == DescriptorType::STORAGE_IMAGE) {
            if (auto p = std::get_if<DescriptorImageInfo>(&descriptorInfo.info)) {
                GLboolean layered = GL_TRUE;
                if (p->pImageView->GetCreateInfoPtr()->viewType == ImageViewType::TYPE_1D ||
                    p->pImageView->GetCreateInfoPtr()->viewType == ImageViewType::TYPE_2D ||
                    p->pImageView->GetCreateInfoPtr()->viewType == ImageViewType::TYPE_3D)
                    layered = GL_FALSE;
                // mpStateManager->BindImageTexture(k,
                //								 static_cast<const
                // GLImage *>(pImageView->GetCreateInfoPtr()->pImage)->GetHandle(),
                //								 pImageView->GetCreateInfoPtr()->subresourceRange.baseMipLevel,
                //								 layered,
                //								 pImageView->GetCreateInfoPtr()->subresourceRange.baseArrayLayer,
                //								 GL_READ_WRITE,
                //								 MapInternalFormat(pImageView->GetCreateInfoPtr()->format));
                mpStateManager->BindImageTexture(i, static_cast<GLImageView const *>(p->pImageView)->GetHandle(), 0,
                                                 layered, 0, GL_READ_WRITE,
                                                 MapInternalFormat(p->pImageView->GetCreateInfoPtr()->format));
            } else
                ST_THROW("descriptor info is not DescriptorImageInfo");
        } else if (descriptorInfo.type == DescriptorType::STORAGE_TEXEL_BUFFER) {
            if (auto p = std::get_if<BufferView const *>(&descriptorInfo.info)) {
                mpStateManager->BindImageTexture(i, static_cast<GLBufferView const *>(*p)->GetHandle(), 0, GL_FALSE, 0,
                                                 GL_READ_WRITE, MapInternalFormat((*p)->GetCreateInfoPtr()->format));
                // mpStateManager->BindBuffer(GL_TEXTURE_BUFFER,
                // static_cast<GLBufferView const *>(*p)->GetHandle());
            } else
                ST_THROW("descriptor info is not BufferView");
        }
    }

    for (GLuint i = 0; i < TEXTURE_DESCRIPTOR_MAX_COUNT; ++i) {
        auto &&descriptorInfo = mTextureDescriptorsInfo[i];
        if (descriptorInfo.type == DescriptorType::COMBINED_IMAGE_SAMPLER ||
            descriptorInfo.type == DescriptorType::INPUT_ATTACHMENT) {
            if (auto p = std::get_if<DescriptorImageInfo>(&descriptorInfo.info)) {
                mpStateManager->BindTextureUnit(
                    i,
                    Map(p->pImageView->GetCreateInfoPtr()->viewType,
                        p->pImageView->GetCreateInfoPtr()->pImage->GetCreateInfoPtr()->samples),
                    static_cast<GLImageView const *>(p->pImageView)->GetHandle());
                if (p->pSampler)
                    mpStateManager->BindSampler(i, static_cast<GLSampler const *>(p->pSampler)->GetHandle());
                else if (auto setLayoutBinding = mpSetLayout->GetSetLayoutBinding(i)) {
                    if (setLayoutBinding->pImmutableSamplers) {
                        mpStateManager->BindSampler(
                            i, static_cast<GLSampler const *>(setLayoutBinding->pImmutableSamplers[0])->GetHandle());
                    }
                }
            } else
                ST_THROW("descriptor info is not DescriptorImageInfo");
        } else if (descriptorInfo.type == DescriptorType::UNIFORM_TEXEL_BUFFER) {
            if (auto p = std::get_if<BufferView const *>(&descriptorInfo.info)) {
                mpStateManager->BindTextureUnit(i, GL_TEXTURE_BUFFER,
                                                static_cast<GLBufferView const *>(*p)->GetHandle());
            } else
                ST_THROW("descriptor info is not BufferView");
        }
    }
    for (GLuint i = 0; i < UNIFORM_BUFFER_DESCRIPTOR_MAX_COUNT; ++i) {
        auto &&descriptorInfo = mUniformBufferDescriptorsInfo[i];
        if (descriptorInfo.type == DescriptorType::UNIFORM_BUFFER) {
            if (auto p = std::get_if<DescriptorBufferInfo>(&descriptorInfo.info)) {
                auto offset = static_cast<GLintptr>(p->offset);
                mpStateManager->BindBufferRange(GL_UNIFORM_BUFFER, i,
                                                static_cast<const GLBuffer *>(p->pBuffer)->GetHandle(), offset,
                                                static_cast<GLintptr>(p->range));
            } else
                ST_THROW("descriptor info is not DescriptorBufferInfo");
        } else if (descriptorInfo.type == DescriptorType::UNIFORM_BUFFER_DYNAMIC) {
            ST_LOG("warning, there may be some problem when using dynamic buffers")
            if (auto p = std::get_if<DescriptorBufferInfo>(&descriptorInfo.info)) {
                auto offset = static_cast<GLintptr>(p->offset);
                offset += pDynamicOffsets[index];
                ++index;
                mpStateManager->BindBufferRange(GL_UNIFORM_BUFFER, i,
                                                static_cast<const GLBuffer *>(p->pBuffer)->GetHandle(), offset,
                                                static_cast<GLintptr>(p->range));
            } else
                ST_THROW("descriptor info is not DescriptorBufferInfo");
        }
    }
    for (GLuint i = 0; i < STORAGE_BUFFER_DESCRIPTOR_MAX_COUNT; ++i) {
        auto &&descriptorInfo = mStorageBufferDescriptorsInfo[i];
        if (descriptorInfo.type == DescriptorType::STORAGE_BUFFER) {
            if (auto p = std::get_if<DescriptorBufferInfo>(&descriptorInfo.info)) {
                auto offset = static_cast<GLintptr>(p->offset);
                mpStateManager->BindBufferRange(GL_SHADER_STORAGE_BUFFER, i,
                                                static_cast<const GLBuffer *>(p->pBuffer)->GetHandle(), offset,
                                                static_cast<GLintptr>(p->range));
            } else
                ST_THROW("descriptor info is not DescriptorBufferInfo");
        } else if (descriptorInfo.type == DescriptorType::STORAGE_BUFFER_DYNAMIC) {
            ST_LOG("warning, there may be some problem when using dynamic buffers")
            if (auto p = std::get_if<DescriptorBufferInfo>(&descriptorInfo.info)) {
                auto offset = static_cast<GLintptr>(p->offset);
                offset += pDynamicOffsets[index];
                ++index;
                mpStateManager->BindBufferRange(GL_SHADER_STORAGE_BUFFER, i,
                                                static_cast<const GLBuffer *>(p->pBuffer)->GetHandle(), offset,
                                                static_cast<GLintptr>(p->range));
            } else
                ST_THROW("descriptor info is not DescriptorBufferInfo");
        }
    }
}
//===========================================================================================
void GLDescriptorPool::Allocate(const DescriptorSetAllocateInfo &allocateInfo,
                                std::vector<DescriptorSet *> &descriptorSets) {
    descriptorSets.resize(allocateInfo.setLayoutCount);
    Allocate(allocateInfo, descriptorSets.data());
}
void GLDescriptorPool::Allocate(const DescriptorSetAllocateInfo &allocateInfo, DescriptorSet **ppDescriptorSets) {
    for (size_t i = 0; i < allocateInfo.setLayoutCount; ++i)
        ppDescriptorSets[i] =
            *mDescriptorSets.emplace(new GLDescriptorSet(mpStateManager, allocateInfo.setLayouts[i])).first;
}
}  // namespace Shit
