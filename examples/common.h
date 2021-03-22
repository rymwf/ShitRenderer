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
#include <cstring>

#include "config.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//#include <boost/program_options.hpp>

using namespace Shit;

#define SHADER_PATH SHIT_SOURCE_DIR "/examples/runtime/shaders"

template <class... Ts>
struct overloaded : Ts...
{
	using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

/*
///#ifdef NDEBUG
///#define LOG(str)
///#define LOG_VAR(str)
///#else
///#define LOG(str) \
///	std::cout << __FILE__ << " " << __LINE__ << ":  " << str << std::endl
///#define LOG_VAR(str) \
///	std::cout << __FILE__ << " " << __LINE__ << ":  " << #str << ": " << str << std::endl
///#endif
*/

Shit::RendererVersion rendererVersion{Shit::RendererVersion::GL};

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
	default:
		break;
	}
	return SHADER_PATH "/" + subdir + "/" + shaderName;
}

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	static VertexBindingDescription getVertexBindingDescription()
	{
		return {
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
			 false,
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

WindowPixelFormat chooseSwapchainFormat(const std::vector<WindowPixelFormat> &candidates, Device *pDevice, ShitWindow *pWindow)
{
	std::vector<WindowPixelFormat> supportedFormats;
	pDevice->GetWindowPixelFormats(pWindow, supportedFormats);
	LOG("supported formats:");
	for (auto &&e : supportedFormats)
	{
		LOG_VAR(static_cast<int>(e.format));
		LOG_VAR(static_cast<int>(e.colorSpace));
		for (auto &&a : candidates)
		{
			if (e.format == a.format && e.colorSpace == a.colorSpace)
				return e;
		}
	}
	return supportedFormats[0];
}

//inline void parseArgument(int ac, char **av)
//{
//
//	namespace po = boost::program_options;
//	po::options_description desc("Allowed options");
//	desc.add_options()("help", "produce help message")("T", po::value<std::string>()->default_value("GL"), "set renderer version\n"
//																										   "option values:\n"
//																										   "GL:\t opengl rendereer"
//																										   "VK:\t vulkan renderer");
//
//	// Declare an options description instance which will include
//	// all the options
//	po::options_description all("Allowed options");
//	all.add(desc);
//
//	po::variables_map vm;
//	po::store(po::parse_command_line(ac, av, all), vm);
//
//	if (vm.count("help"))
//	{
//		std::cout << desc << "\n";
//		exit(0);
//	}
//
//	if (vm.count("T"))
//	{
//		const std::string &s = vm["T"].as<std::string>();
//		if (s == "GL")
//		{
//			rendererVersion = RendererVersion::GL;
//		}
//		else if (s == "VK")
//		{
//			rendererVersion = RendererVersion::VULKAN;
//		}
//		else
//		{
//			std::cout << "invalid renderer version value";
//			exit(0);
//		}
//	}
//}

inline void parseArgument(int ac,char** av)
{
	for (int i = 0; i < ac; ++i)
	{
		LOG(av[i]);
	}
	if (ac > 1)
	{
		if (strcmp(av[1], "GL") == 0)
		{
			rendererVersion = Shit::RendererVersion::GL;
		}
		else if (strcmp(av[1], "VK") == 0)
		{
			rendererVersion = Shit::RendererVersion::VULKAN;
		}
	}
}