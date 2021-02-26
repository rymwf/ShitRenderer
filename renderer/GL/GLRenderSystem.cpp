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

	Context *GLRenderSystem::CreateContext(const ContextCreateInfo &createInfo)
	{
		for (auto &&windowContext : mWindowContexts)
		{
			if (windowContext.window.get() == createInfo.pWindow)
			{
#ifdef _WIN32
				windowContext.context= std::move(std::make_unique<GLContextWin32>(&mCreateInfo, createInfo));
				return windowContext.context.get();
#endif
			}
		}
		THROW("failed to create surface");
	}
	void GLRenderSystem::EnumeratePhysicalDevice([[maybe_unused]] std::vector<PhysicalDevice> &physicalDevices)
	{
		LOG("currently GL do not support select gpus");
	}

} // namespace Shit
