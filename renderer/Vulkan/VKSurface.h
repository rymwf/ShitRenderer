/**
 * @file VKSurface.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitSurface.h>
#include "VKPrerequisites.h"
#ifdef _WIN32
#include <renderer/ShitWindowWin32.h>
#endif
namespace Shit
{
	class VKSurface final : public Surface
	{
		VkSurfaceKHR mHandle;

	public:
		VKSurface(const SurfaceCreateInfo &createInfo, ShitWindow *pWindow) : Surface(createInfo)
		{
#ifdef _WIN32
			VkWin32SurfaceCreateInfoKHR info{
				VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
				nullptr,
				0,
				static_cast<WindowWin32 *>(pWindow)->GetInstance(),
				static_cast<WindowWin32 *>(pWindow)->GetHWND(),
			};
#else
			static_assert(0, "there is no VK surface implementation");
#endif
			if (vkCreateWin32SurfaceKHR(vk_instance, &info, nullptr, &mHandle) != VK_SUCCESS)
				THROW("failed to create VK surface");
		}
		~VKSurface() override
		{
			vkDestroySurfaceKHR(vk_instance, mHandle, nullptr);
		}
		constexpr VkSurfaceKHR GetHandle() const
		{
			return mHandle;
		}
	};
}