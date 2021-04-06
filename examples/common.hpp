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
#define LOCATION_JOINTS0 4
#define LOCATION_WEIGHTS0 5
//#define LOCATION_COLOR 4
//#define LOCATION_TEXCOORD1 6
//#define LOCATION_TEXCOORD2 7
//#define LOCATION_BITANGENT 8

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
					projectionMatrix = glm::perspective(arg.fovy, arg.aspect, arg.near, arg.far);
				}
				else if constexpr (std::is_same_v<T, OrthogonalProjectionDescription>)
				{
					projectionMatrix = glm::ortho(arg.left, arg.right, arg.bottom, arg.top, arg.near, arg.far);
				}
				projectionMatrix[1][1] *= -1;
			},
			projectionDescription);
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

struct Material
{
	alignas(MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT) float emissiveFactor[3];
	float alphaCutoff;
	alignas(16) float baseColorFactor[4];
	float metallic;
	float roughness;
};
namespace tinygltf
{
	class Model;
	struct Accessor;
	struct Animation;
	struct Buffer;
	struct BufferView;
	struct Material;
	struct Mesh;
	class Node;
	struct Texture;
	struct Image;
	struct Skin;
	struct Sampler;
	struct Camera;
	struct Scene;
	struct Light;
}
class Model
{
public:
	Model(const char *filePath);
	~Model();

	void DownloadModel(Device *pDevice, PipelineLayout *pipelineLayout, const std::vector<InstanceAttribute> &instanceAttributes = {});
	void FreeModel(Device *pDevice);

	/**
	 * @brief 
	 * 
	 * @param pDevice 
	 * @param sceneIndex -1 :default scene. -2: all scenes
	 */
	void DrawModel(
		Device *pDevice,
		CommandBuffer *pCommandBuffer,
		int sceneIndex = -1);

	size_t GetSceneCount() const;

	constexpr const VertexInputStateCreateInfo *GetVertexInputStateCreateInfoPtr() const
	{
		return &mVertexInputStateCreateInfo;
	}
	constexpr bool LoadSucceed() const
	{
		return mLoadSucceed;
	}

private:
	std::unique_ptr<tinygltf::Model> mpModel;
	bool mLoadSucceed{};

	CommandBuffer *mpCurCommandBuffer;
	Device *mpCurDevice;
	int mCurSceneIndex;
	PipelineLayout *mpCurPipelineLayout;

	struct NodeAttribute
	{
		alignas(MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT) glm::mat4 matrix;
	};

	struct ModelAsset
	{
		std::vector<Buffer *> sceneBuffers;
		std::vector<DrawIndirectInfo> primitivesDrawIndirectInfo;
		std::vector<Image *> images;
		std::vector<ImageView *> imageViews;
		std::vector<Sampler *> samplers;
		Buffer *instanceAttributeBuffer{};

		Buffer *nodeAttributeBuffer;
		std::vector<DescriptorSet *> nodeDescriptorSets;

		Buffer *materialBuffer; //
		std::vector<DescriptorSet *> materialDescriptorSets;

		DescriptorPool *descriptorPool;
	};
	std::unordered_map<Device *, ModelAsset> mModelAssets;

	VertexInputStateCreateInfo mVertexInputStateCreateInfo;

	std::filesystem::path mModelPath;

	void DrawNode(const tinygltf::Node &node);
	void DrawMesh(const tinygltf::Mesh &mesh);
	IndexType GetIndexType(int32_t componentSize);

	void LoadNode(int nodeIndex, std::vector<NodeAttribute> &nodeAttributes, const glm::dmat4 &preMatrix);
};
