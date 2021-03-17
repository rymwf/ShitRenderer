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
	class GLPipelineLayout final : public PipelineLayout
	{
	public:
		GLPipelineLayout(const PipelineLayoutCreateInfo &createInfo) : PipelineLayout(createInfo) {}
	};

	class GLPipeline : public virtual Pipeline
	{
	protected:
		GLuint mHandle;
		std::vector<GLuint> mPrograms;
		GLStateManager *mpStateManager;

	public:
		GLPipeline(GLStateManager *pStateManager) : mpStateManager(pStateManager) {}

		~GLPipeline() override
		{
			for (auto &&e : mPrograms)
				glDeleteProgram(e);
			glDeleteProgramPipelines(1, &mHandle);
		}
		GLuint CreateProgram(const std::vector<GLuint> &shaders, bool separable, bool retrievable);

		constexpr GLuint GetHandle() const
		{
			return mHandle;
		}
	};

	class GLGraphicsPipeline final : public GraphicsPipeline, public GLPipeline
	{
		GLuint mVAO{};
		void CreateVertexArray();
	public:
		GLGraphicsPipeline(GLStateManager *stateManager, const GraphicsPipelineCreateInfo &createInfo);
		constexpr GLuint GetVertexArray() const
		{
			return mVAO;
		}
		~GLGraphicsPipeline() override
		{
			glDeleteVertexArrays(1, &mVAO);
		}
	};

	class GLComputePipeline final : public ComputePipeline, public GLPipeline
	{
	public:
		GLComputePipeline(GLStateManager *stateManager, const ComputePipelineCreateInfo &createInfo);
	};
}
