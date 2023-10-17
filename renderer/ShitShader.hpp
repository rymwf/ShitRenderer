/**
 * @file ShitShader.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include "ShitNonCopyable.hpp"
#include "ShitRendererPrerequisites.hpp"

namespace Shit {
class Shader : public NonCopyable {
protected:
    ShaderCreateInfo mCreateInfo{};

    virtual void Destroy() {}
    virtual void Initialize() {}

public:
    Shader(const ShaderCreateInfo &createInfo) {
        mCreateInfo.shadingLanguage = createInfo.shadingLanguage;
        SetSource(createInfo.code, createInfo.size);
    }
    virtual ~Shader() {
        delete[] mCreateInfo.code;
        mCreateInfo.code = nullptr;
    }
    constexpr const ShaderCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }
    /**
     * @brief reset shader Source, need call reinitialize after
     *
     * @param data
     * @param size
     */
    void SetSource(void const *data, size_t size) {
        delete[] mCreateInfo.code;
        mCreateInfo.size = size;
        mCreateInfo.code = new char[size];
        memcpy((void *)mCreateInfo.code, data, size);
    }
    void Reinitialize() {
        Destroy();
        Initialize();
    }
};
}  // namespace Shit
