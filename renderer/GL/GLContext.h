/**
 * @file GLContext.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitContext.h>
#include "GLPrerequisites.h"

#if _WIN32
#include <Windows.h>
#endif

namespace Shit
{
	class GLContext : public Context
	{
	protected:
		const RenderSystemCreateInfo *mRenderSystemCreateInfo;

		void QueryGLExtensionNames(std::vector<const GLubyte *> &extensionNames);
		GLContext(const RenderSystemCreateInfo *pRenderSystemCreateInfo, const ContextCreateInfo &createInfo)
			: Context(createInfo), mRenderSystemCreateInfo(pRenderSystemCreateInfo) {}

	public:
	};

#if _WIN32
	class GLContextWin32 final : public GLContext
	{
		HDC mHDeviceContext;
		HGLRC mHRenderContext; //false context

		const char *QueryInstanceExtensionNames();

		void CreateRenderContext();

	public:
		GLContextWin32(const RenderSystemCreateInfo *pRenderSystemCreateInfo, const ContextCreateInfo &createInfo);
	};
#endif

} // namespace Shit
