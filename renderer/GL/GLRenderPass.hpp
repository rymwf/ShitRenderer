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
#include <algorithm>
#include <renderer/ShitRenderPass.hpp>
namespace Shit {
class GLRenderPass final : public RenderPass {
    std::vector<ClearValue> mAttachmentsClearValues;

public:
    GLRenderPass(const RenderPassCreateInfo &createInfo) : RenderPass(createInfo) {
        mAttachmentsClearValues.resize(createInfo.attachmentCount);
    }
    void SetAttachmentClearValue(size_t index, const ClearValue &clearValue) {
        mAttachmentsClearValues[index] = clearValue;
    }
    void SetAttachmentClearValue(std::span<const ClearValue> clearValues) {
        mAttachmentsClearValues.resize(clearValues.size());
        std::ranges::copy(clearValues, mAttachmentsClearValues.begin());
    }
    constexpr std::span<ClearValue const> getClearValue() const { return mAttachmentsClearValues; }
    const ClearValue *GetAttachmentClearValuePtr(int index) const { return &mAttachmentsClearValues[index]; }
};
}  // namespace Shit