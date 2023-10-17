/**
 * @file VKDevice.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "VKDevice.hpp"
// #include <renderer/ShitWindow.hpp>
#include <unordered_set>

#include "VKBuffer.hpp"
#include "VKBufferView.hpp"
#include "VKCommandBuffer.hpp"
#include "VKCommandPool.hpp"
#include "VKDescriptor.hpp"
#include "VKDevice.hpp"
#include "VKFence.hpp"
#include "VKFramebuffer.hpp"
#include "VKImage.hpp"
#include "VKPipeline.hpp"
#include "VKQueue.hpp"
#include "VKRenderPass.hpp"
#include "VKRenderSystem.hpp"
#include "VKSampler.hpp"
#include "VKSemaphore.hpp"
#include "VKShader.hpp"
#include "VKSurface.hpp"
#include "VKSwapchain.hpp"

namespace Shit {
PFN_vkVoidFunction VKDevice::GetDeviceProcAddr(const char *pName) { return vkGetDeviceProcAddr(mDevice, pName); }
void VKDevice::LoadDeviceExtensionFunctions() {}
Result VKDevice::WaitIdle() const {
    if (vkDeviceWaitIdle(mDevice) == VK_SUCCESS) return Result::SUCCESS;
    return Result::SHIT_ERROR;
}
VKDevice::~VKDevice() {
    // vkDestroyDevice(mDevice, nullptr);
}
bool VKDevice::IsExtensionSupported(std::string_view name) const {
    return mExtensions.contains(std::string(name)) || g_RenderSystem->IsExtensionSupported(name);
}
RenderSystem *VKDevice::GetRenderSystem() const { return static_cast<RenderSystem *>(g_RenderSystem); }
VKDevice::VKDevice(const DeviceCreateInfo &createInfo) : Device(createInfo) {
    VkPhysicalDevice physicalDevice = GetPhysicalDevice();

    std::vector<VkExtensionProperties> properties;
    VK::queryDeviceExtensionProperties(physicalDevice, properties);

    ST_LOG("=========device extensions=================");

    std::vector<const char *> extensionNames;
    std::unordered_set<std::string> tempExtNames;

    std::string str;
    int k = 0;
    size_t l;
    for (auto &extensionProperty : properties) {
        ST_LOG("extension name: ", extensionProperty.extensionName, "version: ", extensionProperty.specVersion);

        l = strlen(extensionProperty.extensionName);
        if (l == 0) continue;

        auto p = extensionProperty.extensionName;
        k = 0;
        str = "";
        while (*p != '\0') {
            if (*p == '_') ++k;
            if (k == 2) {
                if (!tempExtNames.contains(p + 1)) {
                    mExtensions.emplace(extensionProperty.extensionName, extensionProperty.specVersion);
                    extensionNames.emplace_back(extensionProperty.extensionName);
                } else {
                    ST_LOG("extention ignored: ", extensionProperty.extensionName);
                }
                tempExtNames.emplace(p + 1);
                break;
            }
            ++p;
        }
    }

    auto version = g_RenderSystem->GetCreateInfo()->version;

    // TODO: set physical device  features, put this outside
    void *features1_3 = 0;
    void *features1_2 = 0;
    void *features1_1 = 0;

#if SHIT_VK_VERSION_ATLEAST(1, 3)
    VkPhysicalDeviceVulkan13Features physicalDeviceFeatures1_3{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    };
    if (version >= RendererVersion::VULKAN_130) features1_3 = &physicalDeviceFeatures1_3;
#endif

#if SHIT_VK_VERSION_ATLEAST(1, 2)
    VkPhysicalDeviceVulkan12Features physicalDeviceFeatures1_2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
                                                               features1_3};
    if (version >= RendererVersion::VULKAN_120) features1_2 = &physicalDeviceFeatures1_2;
#endif

#if SHIT_VK_VERSION_ATLEAST(1, 1)
    VkPhysicalDeviceVulkan11Features physicalDeviceFeatures1_1{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
                                                               features1_2};
    if (version >= RendererVersion::VULKAN_110) features1_1 = &physicalDeviceFeatures1_1;
#endif

    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, features1_1};
    vkGetPhysicalDeviceFeatures2(physicalDevice, &physicalDeviceFeatures2);

#ifndef NDEBUG
    ST_LOG("=========VkPhysicalDeviceFeatures 1.0 beg=============");
    ST_LOG("robustBufferAccess:", physicalDeviceFeatures2.features.robustBufferAccess);
    ST_LOG("fullDrawIndexUint32:", physicalDeviceFeatures2.features.fullDrawIndexUint32);
    ST_LOG("imageCubeArray:", physicalDeviceFeatures2.features.imageCubeArray);
    ST_LOG("independentBlend:", physicalDeviceFeatures2.features.independentBlend);
    ST_LOG("geometryShader:", physicalDeviceFeatures2.features.geometryShader);
    ST_LOG("tessellationShader:", physicalDeviceFeatures2.features.tessellationShader);
    ST_LOG("sampleRateShading:", physicalDeviceFeatures2.features.sampleRateShading);
    ST_LOG("dualSrcBlend:", physicalDeviceFeatures2.features.dualSrcBlend);
    ST_LOG("logicOp:", physicalDeviceFeatures2.features.logicOp);
    ST_LOG("multiDrawIndirect:", physicalDeviceFeatures2.features.multiDrawIndirect);
    ST_LOG("drawIndirectFirstInstance:", physicalDeviceFeatures2.features.drawIndirectFirstInstance);
    ST_LOG("depthClamp:", physicalDeviceFeatures2.features.depthClamp);
    ST_LOG("depthBiasClamp:", physicalDeviceFeatures2.features.depthBiasClamp);
    ST_LOG("fillModeNonSolid:", physicalDeviceFeatures2.features.fillModeNonSolid);
    ST_LOG("depthBounds:", physicalDeviceFeatures2.features.depthBounds);
    ST_LOG("wideLines:", physicalDeviceFeatures2.features.wideLines);
    ST_LOG("largePoints:", physicalDeviceFeatures2.features.largePoints);
    ST_LOG("alphaToOne:", physicalDeviceFeatures2.features.alphaToOne);
    ST_LOG("multiViewport:", physicalDeviceFeatures2.features.multiViewport);
    ST_LOG("samplerAnisotropy:", physicalDeviceFeatures2.features.samplerAnisotropy);
    ST_LOG("textureCompressionETC2:", physicalDeviceFeatures2.features.textureCompressionETC2);
    ST_LOG("textureCompressionASTC_LDR:", physicalDeviceFeatures2.features.textureCompressionASTC_LDR);
    ST_LOG("textureCompressionBC:", physicalDeviceFeatures2.features.textureCompressionBC);
    ST_LOG("occlusionQueryPrecise:", physicalDeviceFeatures2.features.occlusionQueryPrecise);
    ST_LOG("pipelineStatisticsQuery:", physicalDeviceFeatures2.features.pipelineStatisticsQuery);
    ST_LOG("vertexPipelineStoresAndAtomics:", physicalDeviceFeatures2.features.vertexPipelineStoresAndAtomics);
    ST_LOG("fragmentStoresAndAtomics:", physicalDeviceFeatures2.features.fragmentStoresAndAtomics);
    ST_LOG("shaderTessellationAndGeometryPointSize:",
           physicalDeviceFeatures2.features.shaderTessellationAndGeometryPointSize);
    ST_LOG("shaderImageGatherExtended:", physicalDeviceFeatures2.features.shaderImageGatherExtended);
    ST_LOG("shaderStorageImageExtendedFormats:", physicalDeviceFeatures2.features.shaderStorageImageExtendedFormats);
    ST_LOG("shaderStorageImageMultisample:", physicalDeviceFeatures2.features.shaderStorageImageMultisample);
    ST_LOG("shaderStorageImageReadWithoutFormat:",
           physicalDeviceFeatures2.features.shaderStorageImageReadWithoutFormat);
    ST_LOG("shaderStorageImageWriteWithoutFormat:",
           physicalDeviceFeatures2.features.shaderStorageImageWriteWithoutFormat);
    ST_LOG("shaderUniformBufferArrayDynamicIndexing:",
           physicalDeviceFeatures2.features.shaderUniformBufferArrayDynamicIndexing);
    ST_LOG("shaderSampledImageArrayDynamicIndexing:",
           physicalDeviceFeatures2.features.shaderSampledImageArrayDynamicIndexing);
    ST_LOG("shaderStorageBufferArrayDynamicIndexing:",
           physicalDeviceFeatures2.features.shaderStorageBufferArrayDynamicIndexing);
    ST_LOG("shaderStorageImageArrayDynamicIndexing:",
           physicalDeviceFeatures2.features.shaderStorageImageArrayDynamicIndexing);
    ST_LOG("shaderClipDistance:", physicalDeviceFeatures2.features.shaderClipDistance);
    ST_LOG("shaderCullDistance:", physicalDeviceFeatures2.features.shaderCullDistance);
    ST_LOG("shaderFloat64:", physicalDeviceFeatures2.features.shaderFloat64);
    ST_LOG("shaderInt64:", physicalDeviceFeatures2.features.shaderInt64);
    ST_LOG("shaderInt16:", physicalDeviceFeatures2.features.shaderInt16);
    ST_LOG("shaderResourceResidency:", physicalDeviceFeatures2.features.shaderResourceResidency);
    ST_LOG("shaderResourceMinLod:", physicalDeviceFeatures2.features.shaderResourceMinLod);
    ST_LOG("sparseBinding:", physicalDeviceFeatures2.features.sparseBinding);
    ST_LOG("sparseResidencyBuffer:", physicalDeviceFeatures2.features.sparseResidencyBuffer);
    ST_LOG("sparseResidencyImage2D:", physicalDeviceFeatures2.features.sparseResidencyImage2D);
    ST_LOG("sparseResidencyImage3D:", physicalDeviceFeatures2.features.sparseResidencyImage3D);
    ST_LOG("sparseResidency2Samples:", physicalDeviceFeatures2.features.sparseResidency2Samples);
    ST_LOG("sparseResidency4Samples:", physicalDeviceFeatures2.features.sparseResidency4Samples);
    ST_LOG("sparseResidency8Samples:", physicalDeviceFeatures2.features.sparseResidency8Samples);
    ST_LOG("sparseResidency16Samples:", physicalDeviceFeatures2.features.sparseResidency16Samples);
    ST_LOG("sparseResidencyAliased:", physicalDeviceFeatures2.features.sparseResidencyAliased);
    ST_LOG("variableMultisampleRate:", physicalDeviceFeatures2.features.variableMultisampleRate);
    ST_LOG("inheritedQueries:", physicalDeviceFeatures2.features.inheritedQueries);
    ST_LOG("=========VkPhysicalDeviceFeatures 1.0 end=============");

#if SHIT_VK_VERSION_ATLEAST(1, 1)
    ST_LOG("=========VkPhysicalDeviceFeatures 1.1 beg=============");
    ST_LOG("storageBuffer16BitAccess:", physicalDeviceFeatures1_1.storageBuffer16BitAccess);
    ST_LOG("uniformAndStorageBuffer16BitAccess:", physicalDeviceFeatures1_1.uniformAndStorageBuffer16BitAccess);
    ST_LOG("storagePushConstant16:", physicalDeviceFeatures1_1.storagePushConstant16);
    ST_LOG("storageInputOutput16:", physicalDeviceFeatures1_1.storageInputOutput16);
    ST_LOG("multiview:", physicalDeviceFeatures1_1.multiview);
    ST_LOG("multiviewGeometryShader:", physicalDeviceFeatures1_1.multiviewGeometryShader);
    ST_LOG("multiviewTessellationShader:", physicalDeviceFeatures1_1.multiviewTessellationShader);
    ST_LOG("variablePointersStorageBuffer:", physicalDeviceFeatures1_1.variablePointersStorageBuffer);
    ST_LOG("variablePointers:", physicalDeviceFeatures1_1.variablePointers);
    ST_LOG("protectedMemory:", physicalDeviceFeatures1_1.protectedMemory);
    ST_LOG("samplerYcbcrConversion:", physicalDeviceFeatures1_1.samplerYcbcrConversion);
    ST_LOG("shaderDrawParameters:", physicalDeviceFeatures1_1.shaderDrawParameters);
    ST_LOG("=========VkPhysicalDeviceFeatures 1.1 end=============");
#endif
#if SHIT_VK_VERSION_ATLEAST(1, 2)
    ST_LOG("=========VkPhysicalDeviceFeatures 1.2 beg=============");
    ST_LOG("samplerMirrorClampToEdge:", physicalDeviceFeatures1_2.samplerMirrorClampToEdge);
    ST_LOG("drawIndirectCount:", physicalDeviceFeatures1_2.drawIndirectCount);
    ST_LOG("storageBuffer8BitAccess:", physicalDeviceFeatures1_2.storageBuffer8BitAccess);
    ST_LOG("uniformAndStorageBuffer8BitAccess:", physicalDeviceFeatures1_2.uniformAndStorageBuffer8BitAccess);
    ST_LOG("storagePushConstant8:", physicalDeviceFeatures1_2.storagePushConstant8);
    ST_LOG("shaderBufferInt64Atomics:", physicalDeviceFeatures1_2.shaderBufferInt64Atomics);
    ST_LOG("shaderSharedInt64Atomics:", physicalDeviceFeatures1_2.shaderSharedInt64Atomics);
    ST_LOG("shaderFloat16:", physicalDeviceFeatures1_2.shaderFloat16);
    ST_LOG("shaderInt8:", physicalDeviceFeatures1_2.shaderInt8);
    ST_LOG("descriptorIndexing:", physicalDeviceFeatures1_2.descriptorIndexing);
    ST_LOG("shaderInputAttachmentArrayDynamicIndexing:",
           physicalDeviceFeatures1_2.shaderInputAttachmentArrayDynamicIndexing);
    ST_LOG("shaderUniformTexelBufferArrayDynamicIndexing:",
           physicalDeviceFeatures1_2.shaderUniformTexelBufferArrayDynamicIndexing);
    ST_LOG("shaderStorageTexelBufferArrayDynamicIndexing:",
           physicalDeviceFeatures1_2.shaderStorageTexelBufferArrayDynamicIndexing);
    ST_LOG("shaderUniformBufferArrayNonUniformIndexing:",
           physicalDeviceFeatures1_2.shaderUniformBufferArrayNonUniformIndexing);
    ST_LOG("shaderSampledImageArrayNonUniformIndexing:",
           physicalDeviceFeatures1_2.shaderSampledImageArrayNonUniformIndexing);
    ST_LOG("shaderStorageBufferArrayNonUniformIndexing:",
           physicalDeviceFeatures1_2.shaderStorageBufferArrayNonUniformIndexing);
    ST_LOG("shaderStorageImageArrayNonUniformIndexing:",
           physicalDeviceFeatures1_2.shaderStorageImageArrayNonUniformIndexing);
    ST_LOG("shaderInputAttachmentArrayNonUniformIndexing:",
           physicalDeviceFeatures1_2.shaderInputAttachmentArrayNonUniformIndexing);
    ST_LOG("shaderUniformTexelBufferArrayNonUniformIndexing:",
           physicalDeviceFeatures1_2.shaderUniformTexelBufferArrayNonUniformIndexing);
    ST_LOG("shaderStorageTexelBufferArrayNonUniformIndexing:",
           physicalDeviceFeatures1_2.shaderStorageTexelBufferArrayNonUniformIndexing);
    ST_LOG("descriptorBindingUniformBufferUpdateAfterBind:",
           physicalDeviceFeatures1_2.descriptorBindingUniformBufferUpdateAfterBind);
    ST_LOG("descriptorBindingSampledImageUpdateAfterBind:",
           physicalDeviceFeatures1_2.descriptorBindingSampledImageUpdateAfterBind);
    ST_LOG("descriptorBindingStorageImageUpdateAfterBind:",
           physicalDeviceFeatures1_2.descriptorBindingStorageImageUpdateAfterBind);
    ST_LOG("descriptorBindingStorageBufferUpdateAfterBind:",
           physicalDeviceFeatures1_2.descriptorBindingStorageBufferUpdateAfterBind);
    ST_LOG("descriptorBindingUniformTexelBufferUpdateAfterBind:",
           physicalDeviceFeatures1_2.descriptorBindingUniformTexelBufferUpdateAfterBind);
    ST_LOG("descriptorBindingStorageTexelBufferUpdateAfterBind:",
           physicalDeviceFeatures1_2.descriptorBindingStorageTexelBufferUpdateAfterBind);
    ST_LOG("descriptorBindingUpdateUnusedWhilePending:",
           physicalDeviceFeatures1_2.descriptorBindingUpdateUnusedWhilePending);
    ST_LOG("descriptorBindingPartiallyBound:", physicalDeviceFeatures1_2.descriptorBindingPartiallyBound);
    ST_LOG("descriptorBindingVariableDescriptorCount:",
           physicalDeviceFeatures1_2.descriptorBindingVariableDescriptorCount);
    ST_LOG("runtimeDescriptorArray:", physicalDeviceFeatures1_2.runtimeDescriptorArray);
    ST_LOG("samplerFilterMinmax:", physicalDeviceFeatures1_2.samplerFilterMinmax);
    ST_LOG("scalarBlockLayout:", physicalDeviceFeatures1_2.scalarBlockLayout);
    ST_LOG("imagelessFramebuffer:", physicalDeviceFeatures1_2.imagelessFramebuffer);
    ST_LOG("uniformBufferStandardLayout:", physicalDeviceFeatures1_2.uniformBufferStandardLayout);
    ST_LOG("shaderSubgroupExtendedTypes:", physicalDeviceFeatures1_2.shaderSubgroupExtendedTypes);
    ST_LOG("separateDepthStencilLayouts:", physicalDeviceFeatures1_2.separateDepthStencilLayouts);
    ST_LOG("hostQueryReset:", physicalDeviceFeatures1_2.hostQueryReset);
    ST_LOG("timelineSemaphore:", physicalDeviceFeatures1_2.timelineSemaphore);
    ST_LOG("bufferDeviceAddress:", physicalDeviceFeatures1_2.bufferDeviceAddress);
    ST_LOG("bufferDeviceAddressCaptureReplay:", physicalDeviceFeatures1_2.bufferDeviceAddressCaptureReplay);
    ST_LOG("bufferDeviceAddressMultiDevice:", physicalDeviceFeatures1_2.bufferDeviceAddressMultiDevice);
    ST_LOG("vulkanMemoryModel:", physicalDeviceFeatures1_2.vulkanMemoryModel);
    ST_LOG("vulkanMemoryModelDeviceScope:", physicalDeviceFeatures1_2.vulkanMemoryModelDeviceScope);
    ST_LOG("vulkanMemoryModelAvailabilityVisibilityChains:",
           physicalDeviceFeatures1_2.vulkanMemoryModelAvailabilityVisibilityChains);
    ST_LOG("shaderOutputViewportIndex:", physicalDeviceFeatures1_2.shaderOutputViewportIndex);
    ST_LOG("shaderOutputLayer:", physicalDeviceFeatures1_2.shaderOutputLayer);
    ST_LOG("subgroupBroadcastDynamicId:", physicalDeviceFeatures1_2.subgroupBroadcastDynamicId);

    ST_LOG("=========VkPhysicalDeviceFeatures 1.2 end=============");
#endif
#if SHIT_VK_VERSION_ATLEAST(1, 3)
    ST_LOG("=========VkPhysicalDeviceFeatures 1.3 beg=============");
    ST_LOG("robustImageAccess:", physicalDeviceFeatures1_3.robustImageAccess);
    ST_LOG("inlineUniformBlock:", physicalDeviceFeatures1_3.inlineUniformBlock);
    ST_LOG("descriptorBindingInlineUniformBlockUpdateAfterBind:",
           physicalDeviceFeatures1_3.descriptorBindingInlineUniformBlockUpdateAfterBind);
    ST_LOG("pipelineCreationCacheControl:", physicalDeviceFeatures1_3.pipelineCreationCacheControl);
    ST_LOG("privateData:", physicalDeviceFeatures1_3.privateData);
    ST_LOG("shaderDemoteToHelperInvocation:", physicalDeviceFeatures1_3.shaderDemoteToHelperInvocation);
    ST_LOG("shaderTerminateInvocation:", physicalDeviceFeatures1_3.shaderTerminateInvocation);
    ST_LOG("subgroupSizeControl:", physicalDeviceFeatures1_3.subgroupSizeControl);
    ST_LOG("computeFullSubgroups:", physicalDeviceFeatures1_3.computeFullSubgroups);
    ST_LOG("synchronization2:", physicalDeviceFeatures1_3.synchronization2);
    ST_LOG("textureCompressionASTC_HDR:", physicalDeviceFeatures1_3.textureCompressionASTC_HDR);
    ST_LOG("shaderZeroInitializeWorkgroupMemory:", physicalDeviceFeatures1_3.shaderZeroInitializeWorkgroupMemory);
    ST_LOG("dynamicRendering:", physicalDeviceFeatures1_3.dynamicRendering);
    ST_LOG("shaderIntegerDotProduct:", physicalDeviceFeatures1_3.shaderIntegerDotProduct);
    ST_LOG("maintenance4:", physicalDeviceFeatures1_3.maintenance4);
    ST_LOG("=========VkPhysicalDeviceFeatures 1.3 end=============");
#endif

#endif

    auto queueFamilyProperties = static_cast<VKRenderSystem *>(g_RenderSystem)->GetQueueFamilyProperties();

    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    std::vector<std::vector<float>> queuePriorities(queueFamilyProperties.size());
    for (uint32_t i = 0, len = static_cast<uint32_t>(queueFamilyProperties.size()); i < len; ++i) {
        // TODO: how to arrange queue priorities
        queuePriorities[i].resize(queueFamilyProperties[i].queueCount, 1.f);
        queueInfos.emplace_back(VkDeviceQueueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, NULL, 0,
                                                        i,                                    // queue family index
                                                        queueFamilyProperties[i].queueCount,  // queue count
                                                        queuePriorities[i].data()});
    }

    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                  features1_1,
                                  0,
                                  static_cast<uint32_t>(queueInfos.size()),
                                  queueInfos.data(),
                                  0,  // deprecated
                                  0,  // deprecated
                                  static_cast<uint32_t>(extensionNames.size()),
                                  extensionNames.data(),
                                  &physicalDeviceFeatures2.features};
    CHECK_VK_RESULT(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &mDevice))

    // create queues
    mQueues.resize(queueInfos.size());
    for (uint32_t i = 0; i < queueInfos.size(); ++i) {
        mQueues[i].resize(queueInfos[i].queueCount);
        for (uint32_t j = 0; j < queueInfos[i].queueCount; ++j) {
            mQueues[i][j] = std::make_unique<VKQueue>(this, i, j, queueInfos[i].pQueuePriorities[j]);
        }
    }
    // create one time commandbuffer
    Init();
}
Format VKDevice::GetSuitableImageFormat(std::span<const Format> candidates, ImageTiling tiling,
                                        FormatFeatureFlagBits flags) const {
    auto featureFlags = Map(flags);
    VkFormatProperties formatProperties;
    for (auto &&e : candidates) {
        vkGetPhysicalDeviceFormatProperties(GetPhysicalDevice(), Map(e), &formatProperties);
        ST_LOG("formatProperties.linearTilingFeatures", formatProperties.linearTilingFeatures);
        ST_LOG("formatProperties.optimalTilingFeatures", formatProperties.optimalTilingFeatures);
        ST_LOG("formatProperties.bufferFeatures", formatProperties.bufferFeatures);
        if (tiling == ImageTiling::LINEAR) {
            if (formatProperties.linearTilingFeatures & featureFlags) return e;
        } else {
            if (formatProperties.optimalTilingFeatures & featureFlags) return e;
        }
    }
    ST_LOG("failed to find supported format");
    exit(-1);
}
void VKDevice::GetSupportedShaderSourceLanguages(std::vector<ShadingLanguage> &shadingLanguage) const {
    shadingLanguage = {ShadingLanguage::SPIRV};
}
Shader *VKDevice::Create(const ShaderCreateInfo &createInfo) {
    return mShaders.emplace_back(std::make_unique<VKShader>(static_cast<VKDevice *>(this), createInfo)).get();
}
Pipeline *VKDevice::Create(const GraphicsPipelineCreateInfo &createInfo) {
    return mPipelines.emplace_back(std::make_unique<VKGraphicsPipeline>(static_cast<VKDevice *>(this), createInfo))
        .get();
}
Pipeline *VKDevice::Create(const ComputePipelineCreateInfo &createInfo) {
    return mPipelines.emplace_back(std::make_unique<VKComputePipeline>(static_cast<VKDevice *>(this), createInfo))
        .get();
}
CommandPool *VKDevice::Create(const CommandPoolCreateInfo &createInfo) {
    return mCommandPools.emplace_back(std::make_unique<VKCommandPool>(static_cast<VKDevice *>(this), createInfo)).get();
}
Buffer *VKDevice::Create(const BufferCreateInfo &createInfo, const void *pData) {
    auto ret =
        mBuffers
            .emplace_back(std::make_unique<VKBuffer>(static_cast<VKDevice *>(this), GetPhysicalDevice(), createInfo))
            .get();
    if (pData) ret->UpdateSubData(0, createInfo.size, pData);
    return ret;
}
Buffer *VKDevice::Create(const BufferCreateInfo &createInfo, int val) {
    auto ret =
        mBuffers
            .emplace_back(std::make_unique<VKBuffer>(static_cast<VKDevice *>(this), GetPhysicalDevice(), createInfo))
            .get();
    ret->UpdateSubData(0, createInfo.size, val);
    return ret;
}
BufferView *VKDevice::Create(const BufferViewCreateInfo &createInfo) {
    return mBufferViews.emplace_back(std::make_unique<VKBufferView>(static_cast<VKDevice *>(this), createInfo)).get();
}
Image *VKDevice::Create(const ImageCreateInfo &createInfo, ImageAspectFlagBits aspectMask, const void *pData) {
    return mImages.emplace_back(std::make_unique<VKImage>(this, createInfo, aspectMask, pData)).get();
}
Image *VKDevice::Create(const ImageCreateInfo &createInfo, ImageAspectFlagBits aspectMask, int val) {
    return mImages.emplace_back(std::make_unique<VKImage>(this, createInfo, aspectMask, val)).get();
}
ImageView *VKDevice::Create(const ImageViewCreateInfo &createInfo) {
    return mImageViews.emplace_back(std::make_unique<VKImageView>(static_cast<VKDevice *>(this), createInfo)).get();
}
DescriptorSetLayout *VKDevice::Create(const DescriptorSetLayoutCreateInfo &createInfo) {
    return mDescriptorSetLayouts
        .emplace_back(std::make_unique<VKDescriptorSetLayout>(static_cast<VKDevice *>(this), createInfo))
        .get();
}
PipelineLayout *VKDevice::Create(const PipelineLayoutCreateInfo &createInfo) {
    return mPipelineLayouts.emplace_back(std::make_unique<VKPipelineLayout>(static_cast<VKDevice *>(this), createInfo))
        .get();
}
RenderPass *VKDevice::Create(const RenderPassCreateInfo &createInfo) {
    return mRenderPasses.emplace_back(std::make_unique<VKRenderPass>(static_cast<VKDevice *>(this), createInfo)).get();
}
Framebuffer *VKDevice::Create(const FramebufferCreateInfo &createInfo) {
    return mFramebuffers.emplace_back(std::make_unique<VKFramebuffer>(static_cast<VKDevice *>(this), createInfo)).get();
}
Semaphore *VKDevice::Create(const SemaphoreCreateInfo &createInfo) {
    return mSemaphores.emplace_back(std::make_unique<VKSemaphore>(static_cast<VKDevice *>(this), createInfo)).get();
}
Fence *VKDevice::Create(const FenceCreateInfo &createInfo) {
    return mFences.emplace_back(std::make_unique<VKFence>(static_cast<VKDevice *>(this), createInfo)).get();
}
Sampler *VKDevice::Create(const SamplerCreateInfo &createInfo) {
    return mSamplers.emplace_back(std::make_unique<VKSampler>(static_cast<VKDevice *>(this), createInfo)).get();
}
DescriptorPool *VKDevice::Create(const DescriptorPoolCreateInfo &createInfo) {
    return mDescriptorPools.emplace_back(std::make_unique<VKDescriptorPool>(static_cast<VKDevice *>(this), createInfo))
        .get();
}
void VKDevice::UpdateDescriptorSets(std::span<const WriteDescriptorSet> descriptorWrites,
                                    std::span<const CopyDescriptorSet> descriptorCopies) {
    UpdateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
                         static_cast<uint32_t>(descriptorCopies.size()), descriptorCopies.data());
}
void VKDevice::UpdateDescriptorSets(uint32_t descriptorWriteCount, const WriteDescriptorSet *descriptorWrites,
                                    uint32_t descriptorCopyCount, const CopyDescriptorSet *descriptorCopies) {
    if (descriptorWriteCount == 0 && descriptorCopyCount == 0) return;
    struct Temp {
        std::vector<VkDescriptorImageInfo> imagesInfo;
        std::vector<VkDescriptorBufferInfo> buffersInfo;
        std::vector<VkBufferView> texelBufferViews;
    };

    // auto count = descriptorWrites.size();
    std::vector<VkWriteDescriptorSet> writes(descriptorWriteCount);
    std::vector<Temp> temp(descriptorWriteCount);

    for (uint32_t i = 0, j; i < descriptorWriteCount; ++i) {
        auto &&e = descriptorWrites[i];
        writes[i] = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            static_cast<VKDescriptorSet const *>(e.pDstSet)->GetHandle(),
            e.dstBinding,
            e.dstArrayElement,
            e.descriptorCount,
            Map(e.descriptorType),
        };

        switch (e.descriptorType) {
            case DescriptorType::SAMPLER:  // sampler (vulkan)
                ST_FALLTHROUGH;
            case DescriptorType::COMBINED_IMAGE_SAMPLER:  // sampler2D
                ST_FALLTHROUGH;
            case DescriptorType::SAMPLED_IMAGE:  // texture2D (vulkan)
                ST_FALLTHROUGH;
            case DescriptorType::STORAGE_IMAGE:  // image2D
                ST_FALLTHROUGH;
            case DescriptorType::INPUT_ATTACHMENT: {
                for (j = 0; j < e.descriptorCount; ++j) {
                    temp[i].imagesInfo.emplace_back(
                        e.pImageInfo[j].pSampler ? static_cast<VKSampler const *>(e.pImageInfo[j].pSampler)->GetHandle()
                                                 : VK_NULL_HANDLE,
                        e.pImageInfo[j].pImageView
                            ? static_cast<VKImageView const *>(e.pImageInfo[j].pImageView)->GetHandle()
                            : VK_NULL_HANDLE,
                        Map(e.pImageInfo[j].imageLayout));
                }
                writes[i].pImageInfo = temp[i].imagesInfo.data();
                break;
            }
            case DescriptorType::UNIFORM_TEXEL_BUFFER:  // samplerbuffer	(access to buffer
                                                        // texture,can only be accessed with texelFetch
                                                        // function) ,textureBuffer(vulkan)
                ST_FALLTHROUGH;
            case DescriptorType::STORAGE_TEXEL_BUFFER:  // imagebuffer (access to
                                                        // buffer texture)
            {
                for (j = 0; j < e.descriptorCount; ++j) {
                    temp[i].texelBufferViews.emplace_back(
                        static_cast<const VKBufferView *>(e.pTexelBufferView[j])->GetHandle());
                }
                writes[i].pTexelBufferView = temp[i].texelBufferViews.data();
                break;
            }
            case DescriptorType::UNIFORM_BUFFER:  // uniform block
            case DescriptorType::STORAGE_BUFFER:  // buffer block
            case DescriptorType::UNIFORM_BUFFER_DYNAMIC:
            case DescriptorType::STORAGE_BUFFER_DYNAMIC: {
                for (j = 0; j < e.descriptorCount; ++j) {
                    auto &&buffer = e.pBufferInfo[j];
                    temp[i].buffersInfo.emplace_back(
                        buffer.pBuffer ? static_cast<VKBuffer const *>(buffer.pBuffer)->GetHandle() : VK_NULL_HANDLE,
                        buffer.offset, buffer.range);
                }
                writes[i].pBufferInfo = temp[i].buffersInfo.data();
                break;
            }
        }
    }
    std::vector<VkCopyDescriptorSet> copies;
    copies.reserve(descriptorCopyCount);
    for (uint32_t i = 0; i < descriptorCopyCount; ++i) {
        auto &&e = descriptorCopies[i];
        copies.emplace_back(VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET, nullptr,
                            static_cast<VKDescriptorSet const *>(e.pSrcSet)->GetHandle(), e.srcBinding,
                            e.srcArrayElement, static_cast<VKDescriptorSet const *>(e.pDstSet)->GetHandle(),
                            e.dstBinding, e.dstArrayElement, e.descriptorCount);
    }
    vkUpdateDescriptorSets(mDevice, static_cast<uint32_t>(writes.size()), writes.data(),
                           static_cast<uint32_t>(copies.size()), copies.data());
}
void VKDevice::FlushMappedMemoryRanges(std::span<const MappedMemoryRange> ranges) {
    std::vector<VkMappedMemoryRange> vkranges;
    static auto v = std::views::transform([](const MappedMemoryRange &e) {
        if (auto p = *std::get_if<Buffer const *>(&e.memory)) {
            return VkMappedMemoryRange{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr,
                                       static_cast<VKBuffer const *>(p)->GetMemory(), e.offset, e.size};
        } else if (auto p2 = *std::get_if<Image const *>(&e.memory)) {
            return VkMappedMemoryRange{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr,
                                       static_cast<VKImage const *>(p2)->GetMemory(), e.offset, e.size};
        }
        ST_THROW("error, VKDevice FlushMappedMemoryRanges");
    });
    std::ranges::copy(ranges | v, std::back_inserter(vkranges));
    CHECK_VK_RESULT(vkFlushMappedMemoryRanges(mDevice, static_cast<uint32_t>(vkranges.size()), vkranges.data()))
}
void VKDevice::InvalidateMappedMemoryRanges(std::span<const MappedMemoryRange> ranges) {
    std::vector<VkMappedMemoryRange> vkranges;
    static auto v = std::views::transform([](const MappedMemoryRange &e) {
        if (auto p = *std::get_if<Buffer const *>(&e.memory)) {
            return VkMappedMemoryRange{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr,
                                       static_cast<VKBuffer const *>(p)->GetMemory(), e.offset, e.size};
        } else if (auto p2 = *std::get_if<Image const *>(&e.memory)) {
            return VkMappedMemoryRange{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr,
                                       static_cast<VKImage const *>(p2)->GetMemory(), e.offset, e.size};
        }
        ST_THROW("error, VKDevice InvalidateMappedMemoryRanges");
    });
    std::ranges::copy(ranges | v, std::back_inserter(vkranges));
    CHECK_VK_RESULT(vkInvalidateMappedMemoryRanges(mDevice, static_cast<uint32_t>(vkranges.size()), vkranges.data()))
}
}  // namespace Shit
