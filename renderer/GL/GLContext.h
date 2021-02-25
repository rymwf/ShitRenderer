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
	class GLContext final : public Context
	{
#if _WIN32
		HDC mHDeviceContext;
 	 	HGLRC mHRenderContext;	//false context
#endif
		const char *QueryInstanceExtensionNames();

		void QueryGLExtensionNames(std::vector<const GLubyte *> &extensionNames);

	public:
		GLContext(const ContextCreateInfo &createInfo);
	};
} // namespace Shit
