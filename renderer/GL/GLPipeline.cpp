/**
 * @file GLPipeline.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLPipeline.hpp"
namespace Shit
{
	void GLGraphicsPipeline::CreateVertexArray(const VertexInputStateCreateInfo &vertexInputStateCreateInfo)
	{
		glGenVertexArrays(1, &mVAO);
		if (mVAO == 0)
			THROW("failed to create vertex array object");
		mpStateManager->BindVertexArray(mVAO);

		std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> bindingTable;
		for (auto &&e : vertexInputStateCreateInfo.vertexBindingDescriptions)
			bindingTable[e.binding] = {e.stride, e.divisor};

		for (auto &&attrib : vertexInputStateCreateInfo.vertexAttributeDescriptions)
		{
			glVertexAttribFormat(
				attrib.location,
				attrib.components,
				Map(attrib.dataType),
				attrib.normalized,
				attrib.offset);
			glVertexAttribBinding(attrib.location, attrib.binding);
			glVertexBindingDivisor(attrib.binding, bindingTable[attrib.binding].second);
			glEnableVertexAttribArray(attrib.location);
		}
	}
	GLuint GLPipeline::CreateShader(const PipelineShaderStageCreateInfo &shaderStageCreateInfo)
	{
		auto shader = glCreateShader(Map(shaderStageCreateInfo.stage));
		if (shader)
		{
			glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V,
						   shaderStageCreateInfo.pShader->GetCreateInfoPtr()->code.data(),
						   static_cast<GLsizei>(shaderStageCreateInfo.pShader->GetCreateInfoPtr()->code.size()));
			GLint a;
			glGetShaderiv(shader,GL_SPIR_V_BINARY,&a);
			if (!a)
			{
				LOG("failed to load spirv module");
			}
		}
		else
			THROW("failed to create shader");
		//equal to compilation
		glSpecializeShaderARB(
			shader,
			shaderStageCreateInfo.entryName,
			static_cast<GLuint>(shaderStageCreateInfo.specializationInfo.constantIDs.size()),
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

		/*
		//reflection
		GLint activeResouces;
		glGetProgramInterfaceiv(mProgram, GL_UNIFORM, GL_ACTIVE_RESOURCES, &activeResouces);
		LOG("GL_ACTIVE_RESOURCES");
		LOG_VAR(activeResouces);
		GLint a;
		glGetProgramInterfaceiv(mProgram, GL_UNIFORM, GL_MAX_NAME_LENGTH, &a);
		LOG("GL_MAX_NAME_LENGTH");
		LOG_VAR(a);
		glGetProgramInterfaceiv(mProgram, GL_UNIFORM, GL_MAX_NUM_ACTIVE_VARIABLES, &a);
		LOG("GL_MAX_NUM_ACTIVE_VARIABLES");
		LOG_VAR(a);

		std::vector<GLenum> props{
			GL_TYPE,
			GL_ARRAY_SIZE,
			GL_OFFSET,
			GL_BLOCK_INDEX,
			GL_ARRAY_STRIDE,
			GL_MATRIX_STRIDE,
			GL_IS_ROW_MAJOR,
			GL_ATOMIC_COUNTER_BUFFER_INDEX,
			GL_REFERENCED_BY_VERTEX_SHADER,
			GL_REFERENCED_BY_TESS_CONTROL_SHADER,
			GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
			GL_REFERENCED_BY_GEOMETRY_SHADER,
			GL_REFERENCED_BY_FRAGMENT_SHADER,
			GL_REFERENCED_BY_COMPUTE_SHADER,
			GL_LOCATION,
		};
		std::vector<GLint> params;
		GLsizei length;
		GLsizei nameLength{};
		for (GLint i = 0; i < activeResouces; ++i)
		{
			LOG("=========================================");
			char name[64];
			glGetProgramResourceName(mProgram, GL_UNIFORM, i, 64, &length, name);
			LOG_VAR(nameLength);
			LOG_VAR(name);
			params.clear();
			params.resize(props.size());
			glGetProgramResourceiv(mProgram, GL_UNIFORM, i, static_cast<GLsizei>(props.size()), props.data(), static_cast<GLsizei>(props.size()), &length, params.data());
			LOG_VAR(length);
			for (auto param : params)
			{
				LOG_VAR(param);
			}
		}
*/
		//=========

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
		mShaders.emplace_back(CreateShader(createInfo.stage));
		mProgram = CreateProgram(mShaders, true, false);

		glGenProgramPipelines(1, &mHandle);

		mpStateManager->BindPipeline(mHandle);

		glUseProgramStages(mHandle, MapShaderStageFlags(createInfo.stage.stage), mProgram);
	}

} // namespace Shit
