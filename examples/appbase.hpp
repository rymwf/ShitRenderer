/**
 * @file appbase.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "common.hpp"
#include "model.hpp"

#define PERSPECTIVE 1

#define IRRADIANCE_MAP_WIDTH 32

#define CREATE_APP(x)               \
	int main(int argc, char **argv) \
	{                               \
		parseArgument(argc, argv);  \
		x app(1280, 720);           \
		try                         \
		{                           \
			app.run();              \
		}                           \
		catch (std::exception & e)  \
		{                           \
			LOG_VAR(e.what());      \
		}                           \
	}

constexpr SampleCountFlagBits SAMPLE_COUNT = SampleCountFlagBits::BIT_4;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct UBOLight
{
	alignas(16) glm::vec3 pos;
	alignas(16) glm::vec4 intensity;
	alignas(16) glm::vec3 direction; //default(0,1,0) from point to light
	alignas(16) glm::vec3 points[4]; //for tube and quad light
	alignas(4) float lim_r_min;		 //the attenuation distance limit
	alignas(4) float lim_r_max;		 //the attenuation distance limit
	alignas(4) float thetaP;
	alignas(4) float thetaU;
};

struct UBOFrame
{
	glm::mat4 PV;
	alignas(16) glm::vec3 eyePosition;
	UBOLight light;
	alignas(16) glm::vec3 ambientColor;
};

enum class NodeType
{
	NODE,
	CAMERA,
	LIGHT,
	MODEL_VIEW
};

struct Node
{
	std::string name;
	Node *pParent{};
	std::vector<Node *> children;

	glm::dquat rotation{1, 0, 0, 0};
	glm::dvec3 translation{};
	glm::dvec3 scale{1};

	glm::mat4 globalMatrix{1};
	bool updated{};

	void Update();
	void AddChild(Node *pNode);
	void RemoveChild(Node *pNode);
};
struct Camera : public Node
{
	Node eye;
	FrustumDescription frustumDescription;
	glm::mat4 frustumMatrix;
	bool frustumUpdated;
	void UpdateFrustum();
	glm::mat4 GetProjectionMatrix();
	Camera();
	~Camera();
};

enum class LightType
{
	DIRECTIONAL,
	POINT,
	SPHERE,
	SPOT,
	QUAD,
	TUBE,
};

struct Light : public Node
{
	LightType lightType;
	glm::vec4 intensity;
	glm::vec3 points[4];
	float lim_r_min; //the attenuation distance limit
	float lim_r_max; //the attenuation distance limit
	float thetaP;
	float thetaU;
};
struct ModelView : public Node
{
	Model *pModel;
//	uint32_t instanceIndex{};
};
class Scene
{
	std::vector<std::unique_ptr<Model>> models;
	std::vector<std::unique_ptr<Node>> nodes;

public:
	~Scene() {}

	Model *LoadModel(const char *pPath);

	template <typename T>
	T *CreateNode(Node *pParent = nullptr)
	{
		nodes.emplace_back(std::make_unique<T>());
		if (pParent)
			pParent->AddChild(nodes.back().get());
		return static_cast<T *>(nodes.back().get());
	}
	void Destroy(Node *pNode);
	void Destroy(Model *pModel);
};

/**
 * @brief cannot resize swapchain
 * 
 */
class AppBase
{
protected:
	RenderSystem *renderSystem;
	ShitWindow *window;
	Device *device;

	std::optional<QueueFamilyIndex> graphicsQueueFamilyIndex;
	QueueFamilyIndex presentQueueFamilyIndex;
	std::optional<QueueFamilyIndex> transferQueueFamilyIndex;

	SwapchainCreateInfo swapchainCreateInfo;
	Swapchain *swapchain;
	std::vector<ImageView *> swapchainImageViews;
	std::vector<Image *> swapchainImages;
	std::vector<Framebuffer *> framebuffers;

	Queue *graphicsQueue;
	Queue *presentQueue;

	Semaphore *imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	Semaphore *renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
	Fence *inFlightFences[MAX_FRAMES_IN_FLIGHT];

	RenderPass *renderPass;

	Image *depthImage;
	ImageView *depthImageView;

	Image *colorImage;
	ImageView *colorImageView;

	uint32_t FPS;
	float frameTimeInterval_ms;
	std::chrono::system_clock::time_point curTime;
	std::chrono::system_clock::time_point startTime;
	//std::vector<Scene> scenes;
	//int sceneIndex{};

	bool startScreenshot{};

	uint32_t _width;
	uint32_t _height;

	Scene scene;

	Node *rootNode;
	Camera *mainCamera;
	Light *directionalLight;

	CommandPool *defaultCommandPool;
	std::vector<CommandBuffer *> defaultCommandBuffers;
	std::vector<CommandBuffer *> backgroundSecondaryCommandBuffers;
	std::vector<std::vector<CommandBuffer *>> frameSecondaryCommandBuffers;

	std::vector<Buffer *> uboFrameBuffers;

	//skybox
	Image *skyboxImage2D;
	ImageView *skyboxImageView2D;

	Image *skyboxImageCube;
	ImageView *skyboxImageViewCube;

	PipelineLayout *skyboxPipelineLayout;
	Pipeline *skyboxPipeline;
	Buffer *skyboxIndirectDrawCmdBuffer;

	//sample
	Sampler *linearSampler;

	//descriptor
	DescriptorPool *defaultDescriptorPool;
	std::vector<DescriptorSetLayout *> defaultDescriptorSetLayouts;
	std::vector<DescriptorSet *> defaultDescriptorSetsUboFrame;

	//=================
	PipelineLayout *pipelineLayoutAxis;
	Pipeline *pipelineAxis;
	std::vector<DescriptorSet *> skyboxDescriptorSets;
	Buffer *drawIndirectCmdBufferAxis;

	//==============
	void initRenderSystem();
	void createSwapchains();
	void createDepthResources();
	void createColorResources();
	void createRenderPasses();
	void createSyncObjects();
	void createFramebuffers();

	void createSampler();

	void createUBOFrameBuffers();
	void createDefaultDescriptorSets();

	void prepareBackground();
	void prepareAxis();
	void prepareSkybox();

	void createDefaultCommandPool();
	void createDefaultCommandBuffers();

	void updateDefaultBuffers(uint32_t index);

	void mainloop();

	void processBaseEvent(const Event &ev);

	virtual void processEvent(const Event &ev) {}
	virtual void prepare() {}

	virtual void drawFrame(uint32_t imageIndex, std::vector<CommandBuffer *> &primaryCommandBuffers) = 0;

	void addSecondaryCommandBuffers(uint32_t imageIndex, const std::vector<CommandBuffer *> &commandBuffers);
	void removeSecondaryCommandBuffer(uint32_t imageIndex, const CommandBuffer *pCommandBuffer);

public:
	AppBase(uint32_t width, uint32_t height);
	void run();
};
