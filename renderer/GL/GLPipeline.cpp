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
	GLGraphicsPipeline::GLGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo) : GraphicsPipeline(createInfo)
	{
		if (SHIT_GL_410)
			glGenProgramPipelines(1, &mPipeline);
		else
			THROW("failed to create pipelines");
		glBindProgramPipeline(mPipeline);

		for (auto &&stageCreateInfo : *createInfo.pStages)
		{
			auto shader = static_cast<GLShader *>(stageCreateInfo.pShader)->GetHandle();
			GLuint specSize{};
			GLuint *idData{}, *valueData{};
			if (stageCreateInfo.pSpecializationInfo)
			{
				specSize = stageCreateInfo.pSpecializationInfo->constantIDs.size();
				idData = stageCreateInfo.pSpecializationInfo->constantIDs.data();
				valueData = stageCreateInfo.pSpecializationInfo->constantValues.data();
			}
			//equal to compilation
			glSpecializeShader(
				shader,
				stageCreateInfo.entryName,
				specSize,
				idData,
				valueData);

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
				throw std::runtime_error("failed to specialize shader");
			}
			auto program = CreateProgram({shader}, true, false);
			mPrograms.emplace_back(program);
			glUseProgramStages(mPipeline, Map(stageCreateInfo.stage), program);
		}
	}
	GLuint GLGraphicsPipeline::CreateProgram(const std::vector<GLuint> &shaders, bool separable, bool retrievable)
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
} // namespace Shit
