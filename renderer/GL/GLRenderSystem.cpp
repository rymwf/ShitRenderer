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
		for (auto &&windowSurface : mWindowSurfaces)
		{
			if (windowSurface.window.get() == createInfo.pWindow)
			{
#ifdef _WIN32
				windowSurface.surface = std::move(std::make_unique<GLContext>(createInfo));
				return windowSurface.surface.get();
#endif
			}
		}
		throw std::runtime_error("failed to create surface");
	}

} // namespace Shit
