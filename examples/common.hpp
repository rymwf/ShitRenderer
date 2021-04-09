/**
 * @file common.hpp
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
#include <variant>
#include <filesystem>

#include "config.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

//#include <boost/program_options.hpp>

using namespace Shit;

#define ASSET_PATH SHIT_SOURCE_DIR "/examples/assets/"
#define SHADER_PATH SHIT_SOURCE_DIR "/examples/runtime/shaders/"
#define IMAGE_PATH SHIT_SOURCE_DIR "/examples/assets/images/"

#define MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT 0x100
#define MAX_VERTX_LOCATION

#define VERTEX_LOCATION_NUM_MAX 16

#define LOCATION_POSITION 0
#define LOCATION_NORMAL 1
#define LOCATION_TANGENT 2
#define LOCATION_TEXCOORD0 3
#define LOCATION_TEXCOORD1 4
#define LOCATION_COLOR0 5
#define LOCATION_JOINTS0 6
#define LOCATION_WEIGHTS0 7

#define LOCATION_INSTANCE_COLOR_FACTOR 11
#define LOCATION_INSTANCE_MATRIX 12

//#define TEXTURE_MAX_BINDING_COUNT 12

#define TEXTURE_BINDING_ALBEDO 0
#define TEXTURE_BINDING_NORMAL 1
#define TEXTURE_BINDING_METALLIC_ROUGHNESS 2
#define TEXTURE_BINDING_OCCLUSION 3
#define TEXTURE_BINDING_EMISSION 4
#define TEXTURE_BINDING_TRANSPARENCY 5

#define UNIFORM_BINDING_FRAME 12
#define UNIFORM_BINDING_NODE 13
#define UNIFORM_BINDING_MATERIAL 14
#define UNIFORM_BINDING_PUSH_CONSTANT 15

#define DESCRIPTORSET_ID_FRAME 0
#define DESCRIPTORSET_ID_NODE 1
#define DESCRIPTORSET_ID_MATERIAL 2

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

void *loadImage(const char *imagePath, int &width, int &height, int &components, int request_components);
void freeImage(void *pData);

struct Light
{
	alignas(16) glm::vec3 pos;
	alignas(16) glm::vec4 color;
	alignas(16) glm::vec4 intensity;
	alignas(8) glm::vec2 lim_r; //the attenuation distance limit,r.x: min dist(sphere light radius), r.y: max dist
	alignas(16) glm::vec3 direction;
	alignas(16) glm::vec3 tube_p0;
	alignas(16) glm::vec3 tube_p1;
};

//update max once per frame
struct UBOFrame
{
	glm::mat4 PV;
	Light light;
	alignas(16) glm::vec3 ambientColor;
};

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
	glm::vec4 colorFactor;
	glm::mat4 matrix;
	static VertexBindingDescription getVertexBindingDescription(uint32_t binding)
	{
		return {binding, sizeof(InstanceAttribute), 1};
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
			{startLocation + 4,
			 binding,
			 4,
			 DataType::FLOAT,
			 false,
			 64},
		};
	}
	static uint32_t getLocationCount()
	{
		return 5;
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
	double fovy;
	double aspect;
};
struct OrthogonalProjectionDescription
{
	double xmag;
	double ymag;
};
struct ProjectionDescription
{
	double near;
	double far; //if far==0, means infinite perspective, orthogonal cannot be 0
	std::variant<PerspectiveProjectionDescription, OrthogonalProjectionDescription> extraDesc;
};

struct Frustum
{
	ProjectionDescription projectionDescription;
	glm::dmat4 projectionMatrix;
	bool isUpdated{true};

	Frustum() = default;
	Frustum(const ProjectionDescription &projDesc) : projectionDescription(projDesc)
	{
		Update();
	}
	void Update()
	{
		std::visit(
			[&](auto &&arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, PerspectiveProjectionDescription>)
				{
					if (projectionDescription.far == 0)
						projectionMatrix = glm::infinitePerspective(arg.fovy, arg.aspect, projectionDescription.near);
					else
						projectionMatrix = glm::perspective(arg.fovy, arg.aspect, projectionDescription.near, projectionDescription.far);
				}
				else if constexpr (std::is_same_v<T, OrthogonalProjectionDescription>)
				{
					projectionMatrix = glm::ortho(-arg.xmag / 2, arg.xmag / 2, -arg.ymag / 2, arg.ymag / 2, projectionDescription.near, projectionDescription.far);
				}
				projectionMatrix[1][1] *= -1;
			},
			projectionDescription.extraDesc);
		isUpdated = true;
	}
};

struct Camera
{
	glm::dvec3 eye;
	glm::dvec3 center;
	glm::dvec3 up;
	glm::dmat4 viewMatrix;
	bool isUpdated{true};

	Camera() = default;
	Camera(
		glm::dvec3 eye_,
		glm::dvec3 center_,
		glm::dvec3 up_) : eye(eye_), center(center_), up(up_)
	{
		Update();
	}
	void Update()
	{
		viewMatrix = glm::lookAt(eye, center, up);
		isUpdated = true;
	}
};

//vector
template <typename T>
struct VertexAttribute
{
	static VertexBindingDescription GetVertexBindingDescription()
	{
		return {sizeof(T)};
	}
	static std::vector<VertexAttributeDescription> GetVertexAttributeDescription(uint32_t startLocation, uint32_t binding)
	{
		return {
			{
				startLocation + 0,
				binding,				   //buffer index
				sizeof(T) / sizeof(float), //components
				DataType::FLOAT,		   //data type
				false,					   //normalized
				0						   //offset
			},
		};
	}
};
template <>
struct VertexAttribute<glm::mat4>
{
	static VertexBindingDescription GetVertexBindingDescription()
	{
		return {sizeof(glm::mat4)};
	}
	static std::vector<VertexAttributeDescription> GetVertexAttributeDescription(uint32_t startLocation, uint32_t binding)
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
};

template<typename T>
float intToFloat(T value)
{
	return (std::max)(float(value) / std::numeric_limits<T>::max(), -1.f);
}
