/**
 * @file VKContext.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKContext.h"
namespace Shit
{
	VKContext::VKContext(VkInstance instance, const ContextCreateInfo &createInfo)
	{
#if _WIN32
		VkWin32SurfaceCreateInfoKHR tempInfo{
			VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			nullptr,
			0,
			static_cast<HINSTANCE>(createInfo.pWindow->GetNativeInstance()),
			static_cast<HWND>(createInfo.pWindow->GetNativeHandle())
		};
#else
		static_assert(0, "there is no surface implementation");
#endif
	}
} // namespace Shit
