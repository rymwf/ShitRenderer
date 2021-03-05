/**
 * @file ShitImage.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.h"
namespace Shit
{

	class Image
	{
	protected:
		ImageCreateInfo mCreateInfo;

	public:
		Image(const ImageCreateInfo &createInfo) : mCreateInfo(createInfo) {}
		virtual ~Image() {}
	};

}
