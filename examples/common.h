/**
 * @file common.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <iostream>
#include <renderer/ShitRenderSystem.h>
#include <fstream>
#include "config.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace Shit;

#define SHADER_PATH SHIT_SOURCE_DIR "/examples/runtime/shaders"

//#ifdef NDEBUG
//#define LOG(str)
//#define LOG_VAR(str)
//#else
//#define LOG(str) \
//	std::cout << __FILE__ << " " << __LINE__ << ":  " << str << std::endl
//#define LOG_VAR(str) \
//	std::cout << __FILE__ << " " << __LINE__ << ":  " << #str << ": " << str << std::endl
//#endif

std::string readFile(const char *filename)
{
	std::fstream file(filename, std::ios::in | std::ios::ate | std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("failed to open file");
	size_t size0 = static_cast<size_t>(file.tellg());
	std::string ret;
	ret.resize(size0);
	file.seekg(0);
	file.read(const_cast<char *>(ret.data()), size0);
	file.close();
	return ret;
}

std::string buildShaderPath(const char *shaderName, RendererVersion renderVersion)
{
	renderVersion &= RendererVersion::TypeBitmask;
	std::string subdir;
	switch (renderVersion)
	{
	case RendererVersion::GL:
		subdir = "GL";
		break;
	case RendererVersion::VULKAN:
		subdir = "Vulkan";
		break;
	case RendererVersion::D3D11:
		break;
	case RendererVersion::D3D12:
		break;
	case RendererVersion::METAL:
		break;
	}
	return SHADER_PATH "/" + subdir + "/" + shaderName;
}

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	static VertexBindingDescription getVertexBindingDescription(uint32_t binding)
	{
		return {
			binding,
			sizeof(Vertex),
			0,
		};
	}
	static std::vector<VertexAttributeDescription> getVertexAttributeDescription(uint32_t startLocation, uint32_t binding)
	{
		return {
			{startLocation + 0,
			 binding,
			 3,
			 DataType::FLOAT,
			 false,
			 offsetof(Vertex, pos)},
			{startLocation + 1,
			 binding,
			 3,
			 DataType::FLOAT,
			 true,
			 offsetof(Vertex, color)},
			{startLocation + 2,
			 binding,
			 2,
			 DataType::FLOAT,
			 false,
			 offsetof(Vertex, texCoord)},
		};
	}
	static uint32_t getLocationsNum()
	{
		return 3;
	}
};
