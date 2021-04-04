/**
 * @file GLBufferView.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitBufferView.hpp>
#include "GLPrerequisites.hpp"
namespace Shit
{
	class GLBufferView final : public BufferView
	{
		GLuint mHandle;
		GLStateManager *mpStateManager;

	public:
		GLBufferView(GLStateManager *pStateManager, const BufferViewCreateInfo &createInfo)
			: BufferView(createInfo), mpStateManager(pStateManager)
		{
		}
		virtual ~GLBufferView() {}
		constexpr GLuint GetHandle() const
		{
			return mHandle;
		}
	};
} // namespace Shit
