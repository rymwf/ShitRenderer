/**
 * @file ShitRenderSystem.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "ShitRenderSystem.h"
#include "ShitModule.h"

#if _WIN32
#include "Platform/Windows/ShitWin32Window.h"
#endif

namespace Shit
{
	const char *GetRendererName(const RendererVersion &type)
	{
		switch (type)
		{
		case RendererVersion::GL:
			return SHIT_RENDERER_GL_NAME;
		case RendererVersion::VULKAN:
			return SHIT_RENDERER_VULKAN_NAME;
		case RendererVersion::D3D11:
			return SHIT_RENDERER_D3D11_NAME;
		case RendererVersion::D3D12:
			return SHIT_RENDERER_D3D12_NAME;
		case RendererVersion::METAL:
			return SHIT_RENDERER_METAL_NAME;
		default:
			THROW("renderer type do not exist");
		}
	}

	RenderSystem *LoadRenderSystem(const RenderSystemCreateInfo &createInfo)
	{
		using RENDERER_LOAD_FUNC = Shit::RenderSystem *(*)(const RenderSystemCreateInfo &);
		auto module = ModuleManager::Get()->GetModule(GetRendererName(createInfo.version & RendererVersion::TypeBitmask));
		auto f = (RENDERER_LOAD_FUNC)(module->LoadProc(SHIT_RENDERER_LOAD_FUNC));
		if (!f)
			THROW("failed to find rendersystem load function");
		static std::unique_ptr<RenderSystem, decltype(&DeleteRenderSystem)> sRenderSystem(f(createInfo), &DeleteRenderSystem);
		return sRenderSystem.get();
	}

	void DeleteRenderSystem(RenderSystem *pRenderSystem)
	{
		using RENDERER_DELETE_FUNC = void (*)(const RenderSystem *);
		auto module = ModuleManager::Get()->GetModule(GetRendererName(pRenderSystem->GetCreateInfo()->version));
		auto f = (RENDERER_DELETE_FUNC)(module->LoadProc(SHIT_RENDERER_DELETE_FUNC));
		if (!f)
			THROW("failed to find rendersystem delele function");
		f(pRenderSystem);
	}

	ShitWindow *RenderSystem::CreateRenderWindow(const WindowCreateInfo &createInfo)
	{
#if _WIN32
		mWindowContexts.emplace_back(WindowContext{std::make_unique<Win32Window>(createInfo)});
#else
		static_assert(0, "create render Window method not implemented");
#endif
		mWindowContexts.back().window.get()->AttachEventHandle(std::bind(&RenderSystem::ProcessWindowEvent, this, std::placeholders::_1));
		return mWindowContexts.back().window.get();
	}

	void RenderSystem::DestroyWindow(const ShitWindow *pWindow)
	{
		for (auto it = mWindowContexts.begin(), end = mWindowContexts.end(); it != end; ++it)
		{
			if (it->window.get() == pWindow)
			{
				mWindowContexts.erase(it);
				break;
			}
		}
	}
	void RenderSystem::ProcessWindowEvent(const Event &ev)
	{
		switch (ev.type)
		{
		case EventType::WINDOW_CLOSE:
			DestroyWindow(ev.windowClose.pWindow);
			break;
		}
	}

} // namespace Shit
