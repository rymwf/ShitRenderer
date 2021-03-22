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

extern Shit::RendererVersion rendererVersion;

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

std::string readFile(const char *filename);
std::string buildShaderPath(const char *shaderName, RendererVersion renderVersion);
WindowPixelFormat chooseSwapchainFormat(const std::vector<WindowPixelFormat> &candidates, Device *pDevice, ShitWindow *pWindow);
PresentMode choosePresentMode(const std::vector<PresentMode> &candidates, Device *pDevice, ShitWindow *window);

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