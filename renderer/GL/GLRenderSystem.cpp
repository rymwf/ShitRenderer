/**
 * @file GLRenderSystem.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLRenderSystem.h"
#include <gl/wglext.h>
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
} // namespace Shit
