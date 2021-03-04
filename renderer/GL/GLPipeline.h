/**
 * @file GLPipeline.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitPipeline.h>
#include "GLPrerequisites.h"
#include "GLShader.h"

namespace Shit
{

	class GLGraphicsPipeline final : public GraphicsPipeline
	{
		GLuint mPipeline;
		std::vector<GLuint> mPrograms;

	public:
		GLGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo);

		GLuint CreateProgram(const std::vector<GLuint> &shaders, bool separable, bool retrievable);

		~GLGraphicsPipeline() override
		{
			for (auto &&e : mPrograms)
				glDeleteProgram(e);
		}
	};
}
