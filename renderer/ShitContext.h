/**
 * @file ShitContext.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.h"
#include "ShitWindow.h"

namespace Shit
{
	class Context
	{

	protected:
		ContextCreateInfo mCreateInfo;
		Context(const ContextCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		const ContextCreateInfo *GetCreateInfo() const
		{
			return &mCreateInfo;
		}
		virtual ~Context() {}
	};

}
