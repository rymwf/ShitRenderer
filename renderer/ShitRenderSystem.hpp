/**
 * @file ShitRenderer.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include "ShitDevice.hpp"
#include "ShitListener.hpp"
#include "ShitNonCopyable.hpp"
#include "ShitRendererPrerequisites.hpp"

namespace Shit {
class RenderSystem : public NonCopyable {
protected:
    RenderSystemCreateInfo mCreateInfo;

    // std::vector<std::unique_ptr<Window>> mWindows;
    std::vector<std::unique_ptr<Device>> mDevices;
    std::vector<std::unique_ptr<Surface>> mSurfaces;

protected:
    RenderSystem() : mCreateInfo{RendererVersion::VULKAN_110} {}

    void DestroyDevice(const Device *pDevice);

public:
    RenderSystem(const RenderSystemCreateInfo &createInfo) : mCreateInfo(createInfo) {}
    virtual ~RenderSystem() {}

    const RenderSystemCreateInfo *GetCreateInfo() const { return &mCreateInfo; }

    virtual Surface *CreateSurface(const SurfaceCreateInfoWin32 &createInfo) = 0;

    virtual std::optional<QueueFamily> GetQueueFamily(QueueFlagBits) const {
        // for opengl
        return std::optional<QueueFamily>{{Shit::QueueFlagBits::GRAPHICS_BIT | Shit::QueueFlagBits::COMPUTE_BIT |
                                               Shit::QueueFlagBits::SPARSE_BINDING_BIT |
                                               Shit::QueueFlagBits::TRANSFER_BIT,
                                           0, 1}};
    }

    /**
     * @brief Create a Device object,
     * create a device include all queue families supported by the physical device
     * currently physical device selection is not supported, the method will use
     * the current gpu(opengl) or the best gpu(Vulkan)
     *
     * @param createInfo
     * @return Device*
     */
    virtual Device *CreateDevice(const DeviceCreateInfo &createInfo = {}) = 0;

    /**
     * @brief TODO: physical device not finished
     *
     * @param physicalDevices
     */
    virtual void EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices) = 0;
};

RenderSystem *LoadRenderSystem(const RenderSystemCreateInfo &createInfo);
void DeleteRenderSystem(RenderSystem *pRenderSystem);
}  // namespace Shit
