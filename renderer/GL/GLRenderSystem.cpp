/**
 * @file GLRenderSystem.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLRenderSystem.hpp"
#include "GLDevice.hpp"

namespace Shit
{

	extern "C" [[nodiscard]] SHIT_API Shit::RenderSystem *ShitLoadRenderSystem(const Shit::RenderSystemCreateInfo &createInfo)
	{
		return new GLRenderSystem(createInfo);
	}
	extern "C" SHIT_API void ShitDeleteRenderSystem(const Shit::RenderSystem *pRenderSystem)
	{
		delete pRenderSystem;
	}

	void GLRenderSystem::EnumeratePhysicalDevice([[maybe_unused]] std::vector<PhysicalDevice> &physicalDevices)
	{
		LOG("currently GL do not support select gpus");
	}

	Device *GLRenderSystem::CreateDevice(const DeviceCreateInfo &createInfo)
	{
#ifdef _WIN32
		mDevices.emplace_back(std::make_unique<GLDeviceWin32>(createInfo, mCreateInfo));
#else
		static_assert(0, "GL CreateDevice is not implemented yet");
#endif
		return mDevices.back().get();
	};
} // namespace Shit
