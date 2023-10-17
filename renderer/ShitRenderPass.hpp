/**
 * @file ShitRenderPass.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include "ShitRendererPrerequisites.hpp"
namespace Shit {
class RenderPass {
protected:
    RenderPassCreateInfo mCreateInfo{};

    RenderPass(const RenderPassCreateInfo &createInfo);

public:
    virtual ~RenderPass();
    constexpr const RenderPassCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }
};
}  // namespace Shit
