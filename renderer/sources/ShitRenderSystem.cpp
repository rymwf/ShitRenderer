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

#ifdef _WIN32
#include "ShitWindowWin32.h"
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

	void DeleteRenderSystem(RenderSystem *pRenderSystem)
	{
		using RENDERER_DELETE_FUNC = void (*)(const RenderSystem *);
		auto module = ModuleManager::Get()->GetModule(GetRendererName(pRenderSystem->GetCreateInfo()->version));
		auto f = (RENDERER_DELETE_FUNC)(module->LoadProc(SHIT_RENDERER_DELETE_FUNC));
		if (!f)
			THROW("failed to find rendersystem delele function");
		f(pRenderSystem);
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

	ShitWindow *RenderSystem::CreateRenderWindow(const WindowCreateInfo &createInfo)
	{
#ifdef _WIN32
		mWindows.emplace_back(std::make_unique<WindowWin32>(createInfo));
#else
		static_assert(0, "create render Window method not implemented");
#endif
		auto window = mWindows.back().get();
		window->AttachEventHandle(std::bind(&RenderSystem::ProcessWindowEvent, this, std::placeholders::_1));
		window->AttachEventHandle(createInfo.eventHandle);
		CreateSurface({mWindows.back().get()});
		return mWindows.back().get();
	}

	void RenderSystem::DestroyDevice(const Device *pDevice)
	{
		for (auto it = mDevices.begin(), end = mDevices.end(); it != end; ++it)
		{
			if (it->get() == pDevice)
			{
				mDevices.erase(it);
				break;
			}
		}
	}
} // namespace Shit
