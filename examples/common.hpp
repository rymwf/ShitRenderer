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

#define DEFAULT_WINDOW_X 80
#define DEFAULT_WINDOW_Y 40
#define DEFAULT_WINDOW_WIDTH 1280
#define DEFAULT_WINDOW_HEIGHT 720

#define ASSET_PATH SHIT_SOURCE_DIR "/examples/assets/"
#define SHADER_PATH SHIT_SOURCE_DIR "/examples/runtime/shaders/"
#define IMAGE_PATH SHIT_SOURCE_DIR "/examples/assets/images/"
#define SCRRENSHOT_DIR SHIT_SOURCE_DIR "/build/screenshot/"

#define MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT 0x100
#define MAX_VERTX_LOCATION

#define VERTEX_LOCATION_NUM_MAX 16

//vertex locations 
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

//uniform bindings
#define TEXTURE_BINDING_ALBEDO 0
#define TEXTURE_BINDING_NORMAL 1
#define TEXTURE_BINDING_METALLIC_ROUGHNESS 2
#define TEXTURE_BINDING_OCCLUSION 3
#define TEXTURE_BINDING_EMISSION 4
#define TEXTURE_BINDING_TRANSPARENCY 5

#define TEXTURE_BINDING_PREFILTERD_CUBEMAP 6
#define TEXTURE_BINDING_IRRADIANCE_CUBEMAP 7

#define UNIFORM_BINDING_FRAME 12
#define UNIFORM_BINDING_NODE 13
#define UNIFORM_BINDING_MATERIAL 14
#define UNIFORM_BINDING_JOINTMATRIX 15
//#define UNIFORM_BINDING_PUSH_CONSTANT 15

//descriptor sets
#define DESCRIPTORSET_ID_FRAME 0
#define DESCRIPTORSET_ID_NODE 1
#define DESCRIPTORSET_ID_MATERIAL 2
#define DESCRIPTORSET_ID_JOINTMATRIX 3
#define DESCRIPTORSET_ID_OTHER 4

#define DESCRIPTORSET_ID_COMPUTE_IMAGE_IN 0
#define DESCRIPTORSET_ID_COMPUTE_IMAGE_OUT 1


//constant id
#define CONSTANT_ID_JOINTNUM 1

#define COSTANT_ID_COMPUTE_LOCAL_SIZE_X 0
#define COSTANT_ID_COMPUTE_LOCAL_SIZE_Y 1
#define COSTANT_ID_COMPUTE_LOCAL_SIZE_Z 2
#define COSTANT_ID_COMPUTE_OPERATOR_DIM 3
#define COSTANT_ID_COMPUTE_VERTICAL 4

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

/**
 * @brief 
 * 
 * @param pDevice 
 * @param queueFlags 
 * @param queueIndex 
 * @param func arguments are commandbuffer and queue family index
 */
void executeOneTimeCommands(
	Device *pDevice,
	QueueFlagBits queueFlags,
	uint32_t queueIndex,
	const std::function<void(CommandBuffer *)> &func);

void *loadImage(const char *imagePath, int &width, int &height, int &components, int request_components);
void saveImage(const char *imagePath, int width, int height, int component, const void *data, bool hdr = false);
void freeImage(void *pData);

template <typename T>
float intToFloat(T value)
{
	return (std::max)(float(value) / std::numeric_limits<T>::max(), -1.f);
}

//image layout cannot be undefined or preinitialized
void takeScreenshot(Device *pDevice, Image *pImage, ImageLayout imageLayout);
void saveImage(const char *dstPath, Device *pDevice, Image *pImage, ImageLayout imageLayout);

std::string readFile(const char *filename);
std::string buildShaderPath(const char *shaderName, RendererVersion renderVersion);
WindowPixelFormat chooseSwapchainFormat(const std::vector<WindowPixelFormat> &candidates, Device *pDevice, ShitWindow *pWindow);
PresentMode choosePresentMode(const std::vector<PresentMode> &candidates, Device *pDevice, ShitWindow *window);

void convert2DToCubemap(
	Device *pDevice,
	Image *pSrcImage2D,
	ImageLayout srcInitialLayout,
	ImageLayout srcFinalLayout,
	Image *pDstImageCube,
	ImageLayout dstInitialLayout,
	ImageLayout dstFinalLayout);

void generateIrradianceMap(
	Device *pDevice,
	Image *pSrcImageView2D,
	ImageLayout srcInitialLayout,
	ImageLayout srcFinalLayout,
	Image *pDstImageViewCube,
	ImageLayout dstInitialLayout,
	ImageLayout dstFinalLayout);

void generateIrradianceMapSH(
	Device *pDevice,
	Image *pSrcImage2D,
	ImageLayout srcInitialLayout,
	ImageLayout srcFinalLayout,
	Image *pDstImageCube,
	ImageLayout dstInitialLayout,
	ImageLayout dstFinalLayout);

void generatePrefilteredEnvMap(
	Device *pDevice,
	uint32_t maxRoughnessLevelNum,
	Image *pSrcImage2D,
	ImageLayout srcInitialLayout,
	ImageLayout srcFinalLayout,
	Image *pDstImageCube,
	ImageLayout dstInitialLayout,
	ImageLayout dstFinalLayout);

void parseArgument(int ac, char **av);

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
			{startLocation + 0, binding, 4, DataType::FLOAT, false, 0},
			{startLocation + 1, binding, 4, DataType::FLOAT, false, 16},
			{startLocation + 2, binding, 4, DataType::FLOAT, false, 32},
			{startLocation + 3, binding, 4, DataType::FLOAT, false, 48},
			{startLocation + 4, binding, 4, DataType::FLOAT, false, 64},
		};
	}
	static uint32_t getLocationCount()
	{
		return 5;
	}
};

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
struct FrustumDescription
{
	double near;
	double far; //if far==0, means infinite perspective, orthogonal cannot be 0
	std::variant<PerspectiveProjectionDescription, OrthogonalProjectionDescription> extraDesc;
};

struct Frustum
{
	FrustumDescription projectionDescription;
	glm::dmat4 projectionMatrix;
	bool isUpdated{true};

	Frustum() = default;
	Frustum(const FrustumDescription &projDesc) : projectionDescription(projDesc)
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