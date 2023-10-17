/**
 * @file VKSwapchain.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
// #include <renderer/ShitWindow.hpp>
#include "VKSwapchain.hpp"

#include "VKDevice.hpp"
#include "VKFence.hpp"
#include "VKImage.hpp"
#include "VKSemaphore.hpp"
#include "VKSurface.hpp"

namespace Shit {
VKSwapchain::~VKSwapchain() {
    DestroyImages();
    vkDestroySwapchainKHR(static_cast<VKDevice *>(mCreateInfo.pDevice)->GetHandle(), mHandle, nullptr);
}
VKSwapchain::VKSwapchain(Surface *pSurface, const SwapchainCreateInfo &createInfo) : Swapchain(pSurface, createInfo) {
    CreateSwapchain();
}
void VKSwapchain::CreateSwapchain() {
    auto pDevice = static_cast<VKDevice *>(mCreateInfo.pDevice)->GetHandle();

    // auto presentQueueFamilyProperty =
    // surface->GetPresentQueueFamilyProperty(mCreateInfo.pWindow);

    // if (!presentQueueFamilyProperty.has_value())
    // 	ST_THROW("current device do not support present to surface");

    // set format
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    VK::querySurfaceFormats((VkPhysicalDevice)mSurface->GetPhysicalDevice(),
                            static_cast<VKSurface *>(mSurface)->GetHandle(), surfaceFormats);

    VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];

    VkFormat format = Map(mCreateInfo.format);
    VkColorSpaceKHR colorSpace = Map(mCreateInfo.colorSpace);
    // change to srgba8
    for (auto &&e : surfaceFormats) {
        if (e.format == format && e.colorSpace == colorSpace) {
            surfaceFormat = e;
            break;
        }
        ST_LOG("swapchain format do not support:", format);
    }
    ST_LOG_VAR(surfaceFormat.format);
    ST_LOG_VAR(surfaceFormat.colorSpace);

    std::vector<VkPresentModeKHR> presentModes;
    VK::querySurfacePresentModes((VkPhysicalDevice)mSurface->GetPhysicalDevice(),
                                 static_cast<VKSurface *>(mSurface)->GetHandle(), presentModes);
    VkPresentModeKHR presentMode{VK_PRESENT_MODE_IMMEDIATE_KHR};
    auto dstmode = Map(mCreateInfo.presentMode);
    for (auto &&e : presentModes) {
        if (dstmode == e) {
            presentMode = e;
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapchainInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                                           NULL,
                                           0,
                                           static_cast<VKSurface *>(mSurface)->GetHandle(),
                                           mCreateInfo.minImageCount,
                                           surfaceFormat.format,
                                           surfaceFormat.colorSpace,
                                           {mCreateInfo.imageExtent.width, mCreateInfo.imageExtent.height},
                                           1,
                                           Map(mCreateInfo.imageUsage),
                                           VK_SHARING_MODE_EXCLUSIVE,
                                           0,
                                           nullptr,
                                           VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                                           VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                           presentMode,
                                           VK_TRUE,
                                           VK_NULL_HANDLE};

    CHECK_VK_RESULT(vkCreateSwapchainKHR(pDevice, &swapchainInfo, nullptr, &mHandle))

    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(pDevice, mHandle, &swapchainImageCount, nullptr);
    ST_LOG_VAR(swapchainImageCount);
    std::vector<VkImage> swapchainImages;
    swapchainImages.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(pDevice, mHandle, &swapchainImageCount, swapchainImages.data());

    ImageCreateInfo imageCreateInfo{{},
                                    ImageType::TYPE_2D,
                                    mCreateInfo.format,
                                    {mCreateInfo.imageExtent.width, mCreateInfo.imageExtent.height, 1},
                                    1,
                                    1,
                                    SampleCountFlagBits::BIT_1,
                                    ImageTiling::OPTIMAL,
                                    mCreateInfo.imageUsage,
                                    MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                    ImageLayout::PRESENT_SRC};

    mImages.resize(swapchainImageCount);
    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        mImages[i] =
            new VKImage(static_cast<VKDevice *>(mCreateInfo.pDevice), imageCreateInfo, swapchainImages[i], true);
    }

    auto formatAttrib = GetFormatAttribute(mCreateInfo.format);
    ImageAspectFlagBits imageAspect = ImageAspectFlagBits::COLOR_BIT;
    if (formatAttrib.baseFormat == BaseFormat::DEPTH) {
        imageAspect = ImageAspectFlagBits::DEPTH_BIT;
    } else if (formatAttrib.baseFormat == BaseFormat::STENCIL) {
        imageAspect = ImageAspectFlagBits::STENCIL_BIT;
    } else if (formatAttrib.baseFormat == BaseFormat::DEPTH_STENCIL) {
        imageAspect = ImageAspectFlagBits::DEPTH_BIT | ImageAspectFlagBits::STENCIL_BIT;
    }

    // create imageviews
    Shit::ImageViewCreateInfo imageViewCreateInfo{
        nullptr, Shit::ImageViewType::TYPE_2D, mCreateInfo.format, {}, {imageAspect, 0, 1, 0, 1}};
    mImageViews.resize(mImages.size());
    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        imageViewCreateInfo.pImage = mImages[i];
        mImageViews[i] = new VKImageView(static_cast<VKDevice *>(mCreateInfo.pDevice), imageViewCreateInfo);
    }
}
Result VKSwapchain::GetNextImage(const GetNextImageInfo &info, uint32_t &index) {
    return Map(vkAcquireNextImageKHR(
        static_cast<VKDevice *>(mCreateInfo.pDevice)->GetHandle(), mHandle, info.timeout,
        info.pSemaphore ? static_cast<VKSemaphore const *>(info.pSemaphore)->GetHandle() : VK_NULL_HANDLE,
        info.pFence ? static_cast<VKFence const *>(info.pFence)->GetHandle() : VK_NULL_HANDLE, &index));
}
void VKSwapchain::Resize(uint32_t width, uint32_t height) {
    // destroy
    DestroyImages();
    vkDestroySwapchainKHR(static_cast<VKDevice *>(mCreateInfo.pDevice)->GetHandle(), mHandle, nullptr);

    //
    mCreateInfo.imageExtent = {width, height};
    CreateSwapchain();
}
}  // namespace Shit
