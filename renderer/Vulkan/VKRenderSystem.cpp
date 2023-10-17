/**
 * @file VKRenderSystem.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "VKRenderSystem.hpp"

// #ifdef _WIN32
// #include <renderer/ShitWindowWin32.hpp>
// #endif

namespace Shit {
VKRenderSystem *g_RenderSystem;

extern "C" [[nodiscard]] SHIT_API Shit::RenderSystem *ShitLoadRenderSystem(
    const Shit::RenderSystemCreateInfo &createInfo) {
    g_RenderSystem = new VKRenderSystem(createInfo);
    return g_RenderSystem;
}
extern "C" SHIT_API void ShitDeleteRenderSystem(const Shit::RenderSystem *pRenderSystem) { delete pRenderSystem; }

// static VkBool32 vkDebugReportCallbackEXT(
//	VkDebugReportFlagsEXT flags,
//	VkDebugReportObjectTypeEXT objectType,
//	uint64_t object,
//	size_t location,
//	int32_t messageCode,
//	const char *pLayerPrefix,
//	const char *pMessage,
//	void *pUserData)
//{
//	ST_LOG("=========================")
//	ST_LOG("VkDebugReportFlagsEXT:", flags)
//	ST_LOG("VkDebugReportObjectTypeEXT:", objectType, "object", object)
//	ST_LOG("location:", location)
//	ST_LOG("messageCode:", messageCode)
//	ST_LOG("pLayerPrefix:", pLayerPrefix)
//	ST_LOG("pMessage:", pMessage)
//	ST_LOG("pUserData:", (char *)pUserData)
// }

// static VkBool32 vkDebugUtilsMessengerCallbackEXT(
//	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
//	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
//	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
//	void *pUserData)
//{
//	ST_LOG("=========================")
//	ST_LOG("VkDebugUtilsMessageSeverityFlagBitsEXT:", messageSeverity)
//	ST_LOG("VkDebugUtilsMessageTypeFlagsEXT:", messageTypes)
//	ST_LOG("pUserData:", (char *)pUserData)
// }

//

PFN_vkVoidFunction VKRenderSystem::GetInstanceProcAddr(const char *pName) {
    return vkGetInstanceProcAddr(mInstance, pName);
}
void VKRenderSystem::LoadInstantceExtensionFunctions() {}
VKRenderSystem::~VKRenderSystem() {
    mSurfaces.clear();

    VkDevice device;
    for (auto it = mDevices.begin(); it != mDevices.end();) {
        device = static_cast<VKDevice *>(it->get())->GetHandle();
        it = mDevices.erase(it);
        vkDestroyDevice(device, nullptr);
    }

    vkDestroyInstance(mInstance, nullptr);
}

bool VKRenderSystem::CheckLayerSupport(const char *layerName) {
    if (mInstanceLayerProperties.empty()) {
        VK::queryInstanceLayerProperties(mInstanceLayerProperties);
#ifndef NDEBUG
        for (auto &&layer : mInstanceLayerProperties) {
            ST_LOG("============================================");
            ST_LOG_VAR(layer.layerName);
            ST_LOG_VAR(layer.specVersion);
            ST_LOG_VAR(layer.implementationVersion);
            ST_LOG_VAR(layer.description);

            std::vector<VkExtensionProperties> layerExtensionProperties;
            VK::queryInstanceExtensionProperties(layer.layerName, layerExtensionProperties);
            for (auto &&layerExtensionProp : layerExtensionProperties) {
                ST_LOG_VAR(layerExtensionProp.specVersion);
                ST_LOG_VAR(layerExtensionProp.extensionName);
            }
        }
#endif
    }
    for (auto &&layerProp : mInstanceLayerProperties)
        if (strcmp(layerProp.layerName, layerName) == 0) return true;
    return false;
}

VKRenderSystem::VKRenderSystem(const RenderSystemCreateInfo &createInfo) : RenderSystem(createInfo) {
    uint32_t a = static_cast<uint32_t>(mCreateInfo.version & RendererVersion::VersionBitmask);
    uint32_t apiversion;
    if (a)
        apiversion = VK_MAKE_VERSION((a >> 8) & 0xf, (a >> 4) & 0xf, a & 0xf);
    else {
        vkEnumerateInstanceVersion(&apiversion);
        auto b = VK_VERSION_MINOR(apiversion);
        mCreateInfo.version = Shit::RendererVersion((uint32_t)Shit::RendererVersion::VULKAN_100 | (b << 4));
    }

    ST_LOG_VAR(apiversion);

    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO,
                              NULL,
                              "application name",
                              VK_MAKE_VERSION(1, 0, 0),  // integer, app version
                              "engine name",
                              VK_MAKE_VERSION(0, 0, 0),  // engine version
                              apiversion};

    // add extensions
    std::vector<VkExtensionProperties> instanceExtensionProperties;
    VK::queryInstanceExtensionProperties(nullptr, instanceExtensionProperties);
    std::vector<const char *> extensionNames;
    extensionNames.reserve(instanceExtensionProperties.size());
    ST_LOG("=========== instance extenions============");
    for (auto &e : instanceExtensionProperties) {
        extensionNames.emplace_back(e.extensionName);
        mExtensions.emplace(e.extensionName, e.specVersion);
        ST_LOG_VAR(e.extensionName);
    }

    std::vector<const char *> layers;

    if (static_cast<bool>(createInfo.flags & RenderSystemCreateFlagBits::CONTEXT_DEBUG_BIT)) {
        // enable validation layer
        if (CheckLayerSupport(VK_LAYER_KHRONOS_validation)) {
            layers.emplace_back(VK_LAYER_KHRONOS_validation);
            // add extensions
        } else
            ST_LOG("VK_LAYER_KHRONOS_validation layer is not supported");
        // enable validation layer
        if (CheckLayerSupport(VK_LAYER_LUNARG_monitor)) {
            layers.emplace_back(VK_LAYER_LUNARG_monitor);
            // add extensions
        } else
            ST_LOG("VK_LAYER_LUNARG_monitor layer is not supported");
        //
        if (mExtensions.contains("VK_EXT_debug_utils")) {
            // PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT =
            //	(PFN_vkCreateDebugUtilsMessengerEXT)vkGetDeviceProcAddr(device,
            //															"vkCreateDebugUtilsMessengerEXT");

            // VkDebugUtilsMessengerCreateInfoEXT debugUtilsMsgCreateInfo{
            //	VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            //	NULL,
            //// pNext 	0,
            //// flags 	VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | //
            // messageSeverity 			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
            //	VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | // messageType
            //		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
            //	myOutputDebugString, //pfnUserCallback
            //		NULL // pUserData
            //  };
        } else if (mExtensions.contains("VK_EXT_debug_report")) {
            VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo{
                VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,

            };
        }

        if (mExtensions.contains("VK_EXT_validation_feature")) {
            VkValidationFeaturesEXT validationFeature{
                VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,

            };
        }
        // else if (mExtensions.contains("VK_EXT_validation_flags"))
        //{
        //	VkValidationFlagsEXT validationFlags{
        //		VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT,

        //	};
        //}
    }

    VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,  // must
                                      NULL,
                                      0,  // must
                                      &appInfo,
                                      static_cast<uint32_t>(layers.size()),
                                      layers.data(),
                                      static_cast<uint32_t>(extensionNames.size()),
                                      extensionNames.data()};
    if (vkCreateInstance(&instanceInfo, 0, &mInstance) != VK_SUCCESS) ST_THROW("create instance failed");

    mPhysicalDevice = VK::pickPhysicalDevice(mInstance);

    QueryQueueFamilyProperties(mPhysicalDevice, mQueueFamilyProperties);
}
void VKRenderSystem::QueryQueueFamilyProperties(VkPhysicalDevice physicalDevice,
                                                std::vector<VkQueueFamilyProperties> &queueFamilyProperties) {
    uint32_t queueFamilyPropertyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);
    queueFamilyProperties.resize(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

#ifndef NDEBUG
    ST_LOG_VAR(queueFamilyPropertyCount);
    for (uint32_t i = 0; i < queueFamilyPropertyCount; ++i) {
        ST_LOG_VAR(i);
        ST_LOG_VAR(queueFamilyProperties[i].queueFlags);
        ST_LOG_VAR(queueFamilyProperties[i].queueCount);
        ST_LOG_VAR(queueFamilyProperties[i].timestampValidBits);
        ST_LOG_VAR(queueFamilyProperties[i].minImageTransferGranularity.width);
        ST_LOG_VAR(queueFamilyProperties[i].minImageTransferGranularity.height);
        ST_LOG_VAR(queueFamilyProperties[i].minImageTransferGranularity.depth);
    }
#endif
}
std::optional<QueueFamily> VKRenderSystem::GetQueueFamily(QueueFlagBits flag) const {
    auto index = VK::findQueueFamilyIndexByFlag(mQueueFamilyProperties, Map(flag));
    if (index.has_value())
        return std::optional<QueueFamily>{{Map(mQueueFamilyProperties[*index].queueFlags), index.value(),
                                           mQueueFamilyProperties.at(static_cast<size_t>(index.value())).queueCount}};
    return {};
}
void VKRenderSystem::EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices) {
    VK::queryPhysicalDevices(mInstance, physicalDevices);
}

Device *VKRenderSystem::CreateDevice(const DeviceCreateInfo &) {
    auto info = DeviceCreateInfo{mPhysicalDevice};
    mDevices.emplace_back(std::make_unique<VKDevice>(info));
    return mDevices.back().get();
}

#ifdef _WIN32
Surface *VKRenderSystem::CreateSurface(const SurfaceCreateInfoWin32 &createInfo) {
    return mSurfaces.emplace_back(std::make_unique<VKSurfaceWin32>(mPhysicalDevice, createInfo)).get();
}
#endif
}  // namespace Shit