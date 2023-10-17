/**
 * @file VKRenderSystem.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitRenderSystem.hpp>

#include "VKDevice.hpp"
#include "VKPrerequisites.hpp"
#include "VKSurface.hpp"

namespace Shit {
class VKRenderSystem final : public RenderSystem {
    std::vector<VkLayerProperties> mInstanceLayerProperties;

    // instance extensions, value is version
    std::unordered_map<std::string, uint32_t> mExtensions;
    std::unordered_map<std::string, uint32_t> mPhysicalDeviceExtensions;

    VkInstance mInstance;

    VkPhysicalDevice mPhysicalDevice;

    std::vector<VkQueueFamilyProperties> mQueueFamilyProperties;

private:
    bool CheckLayerSupport(const char *layerName);

    void EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices) override;

    void LoadInstantceExtensionFunctions();

public:
    VKRenderSystem(const RenderSystemCreateInfo &createInfo);

    ~VKRenderSystem() override;

    // VkPhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }

    constexpr VkInstance GetInstance() const { return mInstance; }

    static void QueryQueueFamilyProperties(VkPhysicalDevice physicalDevice,
                                           std::vector<VkQueueFamilyProperties> &queueFamilyProperties);

    std::optional<QueueFamily> GetQueueFamily(QueueFlagBits flag) const override;

    std::vector<VkQueueFamilyProperties> GetQueueFamilyProperties() const { return mQueueFamilyProperties; }

    Device *CreateDevice(const DeviceCreateInfo &createInfo) override;

    PFN_vkVoidFunction GetInstanceProcAddr(const char *pName);

    constexpr const std::vector<VkLayerProperties> &GetLayerProperties() const { return mInstanceLayerProperties; }

    constexpr const std::unordered_map<std::string, uint32_t> &GetExtensions() const { return mExtensions; }

    /**
     * @brief instance extensions only
     *
     * @param name
     * @return true
     * @return false
     */
    bool IsExtensionSupported(std::string_view name) const { return mExtensions.contains(std::string(name)); }

#ifdef _WIN32
    Surface *CreateSurface(const SurfaceCreateInfoWin32 &createInfo) override;
#endif
};
}  // namespace Shit