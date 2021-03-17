/**
 * @file GLPipeline.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLPipeline.h"
namespace Shit
{
	void GLGraphicsPipeline::CreateVertexArray()
	{
		glGenVertexArrays(1, &mVAO);
		if (mVAO == 0)
			THROW("failed to create vertex array object");
	}
	GLGraphicsPipeline::GLGraphicsPipeline(GLStateManager *pStateManager, const GraphicsPipelineCreateInfo &createInfo)
		: GraphicsPipeline(createInfo), GLPipeline(pStateManager)
	{
		if (SHIT_GL_410)
			glGenProgramPipelines(1, &mHandle);
		else
			THROW("failed to create pipelines");
		mpStateManager->BindPipeline(mHandle);

		for (auto &&stageCreateInfo : createInfo.stages)
		{
			auto shader = static_cast<GLShader *>(stageCreateInfo.pShader)->GetHandle();
			//equal to compilation
			glSpecializeShader(
				shader,
				stageCreateInfo.entryName,
				stageCreateInfo.specializationInfo.constantIDs.size(),
				stageCreateInfo.specializationInfo.constantIDs.data(),
				stageCreateInfo.specializationInfo.constantValues.data());

			// Specialization is equivalent to compilation.
			GLint isCompiled = 0;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

				// The maxLength includes the NULL character
				std::string infoLog;
				infoLog.resize(maxLength);
				glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog.data());

				LOG(infoLog);
				// We don't need the shader anymore.
				glDeleteShader(shader);

				// Use the infoLog as you see fit.
				THROW("failed to specialize shader");
			}
			auto program = CreateProgram({shader}, true, false);
			mPrograms.emplace_back(program);
			glUseProgramStages(mHandle, MapShaderStageFlags(stageCreateInfo.stage), program);
		}
		CreateVertexArray();
	}
	GLuint GLPipeline::CreateProgram(const std::vector<GLuint> &shaders, bool separable, bool retrievable)
	{
		GLuint ret = glCreateProgram();
		if (ret)
		{
			glProgramParameteri(ret, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, retrievable);
			glProgramParameteri(ret, GL_PROGRAM_SEPARABLE, separable);
			for (auto &&shader : shaders)
				glAttachShader(ret, shader);
			glLinkProgram(ret);
#ifndef NODEBUG
			GLint success;
			glGetProgramiv(ret, GL_LINK_STATUS, &success);
			if (!success)
			{
				GLint len;
				glGetProgramiv(ret, GL_INFO_LOG_LENGTH, &len);
				std::string log;
				log.resize(static_cast<size_t>(len));
				glGetProgramInfoLog(ret, len, NULL, &log[0]);
				LOG(log);
				glDeleteProgram(ret);
				THROW("failed to link program");
			}
#endif
		}
		else
			THROW("failed to create program");
		return ret;
	}

	//======================================

	GLComputePipeline::GLComputePipeline(GLStateManager *pStateManager, const ComputePipelineCreateInfo &createInfo)
		: ComputePipeline(createInfo), GLPipeline(pStateManager)
	{
	}

} // namespace Shit
