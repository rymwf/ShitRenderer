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
	void GLGraphicsPipeline::CreateVertexArray(const VertexInputStateCreateInfo &vertexInputStateCreateInfo)
	{
		glGenVertexArrays(1, &mVAO);
		if (mVAO == 0)
			THROW("failed to create vertex array object");
		mpStateManager->BindVertexArray(mVAO);
		for (auto &&attrib : vertexInputStateCreateInfo.vertexAttributeDescriptions)
		{
			glVertexAttribFormat(
				attrib.location,
				attrib.components,
				Map(attrib.dataType),
				attrib.normalized,
				attrib.offset);
			glVertexAttribBinding(attrib.location, attrib.binding);
			glVertexBindingDivisor(attrib.binding, vertexInputStateCreateInfo.vertexBindingDescriptions[attrib.binding].divisor);
			glEnableVertexAttribArray(attrib.location);
		}
	}
	GLuint GLGraphicsPipeline::CreateShader(const PipelineShaderStageCreateInfo &shaderStageCreateInfo)
	{
		auto shader = glCreateShader(Map(shaderStageCreateInfo.stage));
		if (shader)
			glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V,
						   shaderStageCreateInfo.pShader->GetCreateInfoPtr()->code.data(),
						   static_cast<GLsizei>(shaderStageCreateInfo.pShader->GetCreateInfoPtr()->code.size()));
		else
			THROW("failed to create shader");

		//equal to compilation
		glSpecializeShader(
			shader,
			shaderStageCreateInfo.entryName,
			static_cast<GLsizei>(shaderStageCreateInfo.specializationInfo.constantIDs.size()),
			shaderStageCreateInfo.specializationInfo.constantIDs.data(),
			shaderStageCreateInfo.specializationInfo.constantValues.data());

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
			// Use the infoLog as you see fit.
			THROW("failed to specialize shader");
		}
		return shader;
	}
	GLGraphicsPipeline::GLGraphicsPipeline(GLStateManager *pStateManager, const GraphicsPipelineCreateInfo &createInfo)
		: GraphicsPipeline(createInfo), GLPipeline(pStateManager)
	{
		ShaderStageFlagBits stageFlags{};
		for (auto &&e : createInfo.stages)
		{
			mShaders.emplace_back(CreateShader(e));
			stageFlags |= e.stage;
		}
		mProgram = CreateProgram(mShaders, true, false);

		glGenProgramPipelines(1, &mHandle);

		mpStateManager->BindPipeline(mHandle);

		glUseProgramStages(mHandle, MapShaderStageFlags(stageFlags), mProgram);
		CreateVertexArray(createInfo.vertexInputState);
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
