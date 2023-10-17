/**
 * @file GLSampler.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitSampler.hpp>

#include "GLPrerequisites.hpp"
namespace Shit {
class GLSampler final : public Sampler {
    GLuint mHandle;
    GLStateManager *mpStateManager;

public:
    GLSampler(GLStateManager *pStateManager, const SamplerCreateInfo &createInfo);
    ~GLSampler() override;
    constexpr GLuint GetHandle() const { return mHandle; }
};

}  // namespace Shit
