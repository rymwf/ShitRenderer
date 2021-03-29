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
#include <renderer/ShitRenderSystem.hpp>
#include <fstream>
#include <cstring>

#include "config.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <variant>

//#include <boost/program_options.hpp>

using namespace Shit;

#define SHADER_PATH SHIT_SOURCE_DIR "/examples/runtime/shaders/"
#define IMAGE_PATH SHIT_SOURCE_DIR "/examples/assets/images/"

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
	static uint32_t getLocationCount()
	{
		return 3;
	}
};

struct InstanceAttribute
{
	glm::mat4 translation;
	static VertexBindingDescription getVertexBindingDescription()
	{
		return {sizeof(InstanceAttribute), 1};
	}
	static std::vector<VertexAttributeDescription> getVertexAttributeDescription(uint32_t startLocation, uint32_t binding)
	{
		return {
			{startLocation + 0,
			 binding,
			 4,
			 DataType::FLOAT,
			 false,
			 0},
			{startLocation + 1,
			 binding,
			 4,
			 DataType::FLOAT,
			 false,
			 16},
			{startLocation + 2,
			 binding,
			 4,
			 DataType::FLOAT,
			 false,
			 32},
			{startLocation + 3,
			 binding,
			 4,
			 DataType::FLOAT,
			 false,
			 48},
		};
	}
	static uint32_t getLocationCount()
	{
		return 4;
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

inline void parseArgument(int ac, char **av)
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

struct PerspectiveProjectionDescription
{
	double near;
	double far;
	double fovy;
	double aspect;
};
struct OrthogonalProjectionDescription
{
	double near;
	double far;
	double left;
	double right;
	double bottom;
	double top;
};
using ProjectionDescription = std::variant<PerspectiveProjectionDescription, OrthogonalProjectionDescription>;

struct FrustumCreateInfo
{
	ProjectionDescription projectionDescription;
};

class Frustum
{
	ProjectionDescription mProjectionDescription;
	glm::mat4 mProjection;

public:
	Frustum(const FrustumCreateInfo &createInfo) : mProjectionDescription(createInfo.projectionDescription)
	{
		std::visit(
			[&](auto &&arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, PerspectiveProjectionDescription>)
				{
					mProjection = glm::perspective(arg.fovy, arg.aspect, arg.near, arg.far);
				}
				else if constexpr (std::is_same_v<T, OrthogonalProjectionDescription>)
				{
					mProjection = glm::ortho(arg.left, arg.right, arg.bottom, arg.top, arg.near, arg.far);
				}
			},
			mProjectionDescription);
	}
	void Update(const PerspectiveProjectionDescription &perspective)
	{
		mProjectionDescription = perspective;
		mProjection = glm::perspective(perspective.fovy, perspective.aspect, perspective.near, perspective.far);
	}
	void Update(const OrthogonalProjectionDescription &ortho)
	{
		mProjectionDescription = ortho;
		mProjection = glm::ortho(ortho.left, ortho.right, ortho.bottom, ortho.top, ortho.near, ortho.far);
	}
	constexpr const glm::mat4 *GetProjectionPtr() const
	{
		return &mProjection;
	}
};

struct CameraCreateInfo
{
	glm::vec3 eye;
	glm::vec3 center;
	glm::vec3 up;
};

class Camera
{
	CameraCreateInfo mCreateInfo;
	glm::mat4 mView;

public:
	Camera(const CameraCreateInfo &createInfo) : mCreateInfo(createInfo)
	{
	}
	void Update(const CameraCreateInfo &createInfo)
	{
		mCreateInfo = createInfo;
		mView = glm::lookAt(createInfo.eye, createInfo.center, createInfo.up);
	}
	constexpr const glm::mat4 *GetViewPtr() const
	{
		return &mView;
	}
};
