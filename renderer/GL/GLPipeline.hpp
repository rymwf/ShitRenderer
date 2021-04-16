/**
 * @file GLPipeline.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitPipeline.hpp>
#include "GLPrerequisites.hpp"
#include "GLShader.hpp"

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
		GLuint mProgram;
		std::vector<GLuint> mShaders;
		GLStateManager *mpStateManager;

	public:
		GLPipeline(GLStateManager *pStateManager) : mpStateManager(pStateManager) {}

		~GLPipeline() override
		{
			mpStateManager->NotifyReleasedPipeline(mHandle);
			for (auto e : mShaders)
				glDeleteShader(e);
			glDeleteProgram(mProgram);
			glDeleteProgramPipelines(1, &mHandle);
		}
		static GLuint CreateProgram(const std::vector<GLuint> &shaders, bool separable, bool retrievable);

		static GLuint CreateShader(const PipelineShaderStageCreateInfo &shaderStageCreateInfo);

		constexpr GLuint GetHandle() const
		{
			return mHandle;
		}
	};

	class GLGraphicsPipeline final : public GraphicsPipeline, public GLPipeline
	{
		GLuint mVAO{};

		void CreateVertexArray(const VertexInputStateCreateInfo &vertexInputStateCreateInfo);

	public:
		GLGraphicsPipeline(GLStateManager *stateManager, const GraphicsPipelineCreateInfo &createInfo);
		constexpr GLuint GetVertexArray() const
		{
			return mVAO;
		}
		~GLGraphicsPipeline() override
		{
			mpStateManager->NotifyReleasedVertexArray(mVAO);
			glDeleteVertexArrays(1, &mVAO);
		}
	};

	class GLComputePipeline final : public ComputePipeline, public GLPipeline
	{
	public:
		GLComputePipeline(GLStateManager *stateManager, const ComputePipelineCreateInfo &createInfo);
	};
}
