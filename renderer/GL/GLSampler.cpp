/**
 * @file GLSampler.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "GLSampler.hpp"

#include "GLState.hpp"
namespace Shit {
GLSampler::GLSampler(GLStateManager *pStateManager, const SamplerCreateInfo &createInfo)
    : Sampler(createInfo), mpStateManager(pStateManager) {
    glGenSamplers(1, &mHandle);
    glSamplerParameteri(mHandle, GL_TEXTURE_MAG_FILTER, Map(createInfo.magFilter));
    if (createInfo.mipmapMode == SamplerMipmapMode::LINEAR) {
        if (createInfo.minFilter == Filter::LINEAR)
            glSamplerParameteri(mHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        else
            glSamplerParameteri(mHandle, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    } else {
        if (createInfo.minFilter == Filter::LINEAR)
            glSamplerParameteri(mHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        else
            glSamplerParameteri(mHandle, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    }
    glSamplerParameterf(mHandle, GL_TEXTURE_LOD_BIAS, createInfo.mipLodBias);
    glSamplerParameterf(mHandle, GL_TEXTURE_MIN_LOD, createInfo.minLod);
    glSamplerParameterf(mHandle, GL_TEXTURE_MAX_LOD, createInfo.maxLod);
    glSamplerParameteri(mHandle, GL_TEXTURE_WRAP_S, Map(createInfo.wrapModeU));
    glSamplerParameteri(mHandle, GL_TEXTURE_WRAP_T, Map(createInfo.wrapModeV));
    glSamplerParameteri(mHandle, GL_TEXTURE_WRAP_R, Map(createInfo.wrapModeW));
    if (createInfo.anisotropyEnable) {
        glSamplerParameterf(mHandle, GL_TEXTURE_MAX_ANISOTROPY, createInfo.maxAnisotropy);
    }
    if (createInfo.compareEnable) {
        glSamplerParameteri(mHandle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(mHandle, GL_TEXTURE_COMPARE_FUNC, Map(createInfo.compareOp));
    }
    auto borderColor = Map(createInfo.borderColor);
    std::visit(
        [this](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::array<float, 4>>)
                glSamplerParameterfv(mHandle, GL_TEXTURE_BORDER_COLOR, arg.data());
            else if constexpr (std::is_same_v<T, std::array<int32_t, 4>>)
                glSamplerParameterIiv(mHandle, GL_TEXTURE_BORDER_COLOR, arg.data());
        },
        borderColor);
}
GLSampler::~GLSampler() {
    mpStateManager->NotifyReleasedSampler(mHandle);
    glDeleteSamplers(1, &mHandle);
}
}  // namespace Shit