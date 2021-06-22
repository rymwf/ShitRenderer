#pragma once
#include "common.hpp"
#include "node.hpp"
#include "model2.hpp"

extern glm::vec3 ambientColor;

class GUI;

struct UBOLight
{
	alignas(16) glm::vec3 pos;
	alignas(16) glm::vec4 intensity;
	alignas(16) glm::vec3 direction; //default(0,1,0) from point to light
	alignas(16) glm::vec4 points[4]; //for tube and quad light
	alignas(4) float lim_r_min;		 //the attenuation distance limit
	alignas(4) float lim_r_max;		 //the attenuation distance limit
	alignas(4) float thetaPenumbra;
	alignas(4) float thetaUmbra;
};

struct UBOFrame
{
	glm::mat4 PV;
	alignas(16) glm::vec3 eyePosition;
	UBOLight light;
	alignas(16) glm::vec3 ambientColor{};
};

class Scene
{
protected:
	std::string _name;

	CommandPool *_defaultCommandPool;
	CommandPool *_defaultResetableCommandPool;

	std::vector<CommandBuffer *> _commandBuffers; //short lived secondary commandbuffers

	//default descriptor sets
	DescriptorPool *_defaultDescriptorPool;
	std::vector<DescriptorSetLayout *> _defaultDescriptorSetLayouts;
	std::vector<DescriptorSet *> _defaultDescriptorSets; //PV
	std::vector<Buffer *> _defaultUBOBuffers;

	//pipelinelayout can recevie default descriptor sets
	PipelineLayout *_defaultPipelineLayout;

	//=============================
	std::list<std::unique_ptr<Model2>> _models;
	std::list<std::unique_ptr<Material>> _materials;

	Node *_rootNode;
	std::list<std::unique_ptr<Node>> _nodes;

	//	std::vector<std::unique_ptr<ModelView>> modelViews;
	//	std::vector<std::unique_ptr<Light>> lights;
//
	//default assets
	std::unique_ptr<Camera> _mainCamera;
	std::unique_ptr<Material> *_skyboxMaterial;

public:
	Scene();
	virtual ~Scene();

	CommandBuffer *GetCommandBuffer(uint32_t imageIndex)
	{
		return _commandBuffers[imageIndex];
	}

	Camera *GetMainCamera() const
	{
		return _mainCamera.get();
	}

	virtual void Prepare() {}

	Model2 *LoadModel(const char *pPath);

	std::unique_ptr<Material> *AddMaterial(MaterialType type);
	void ApplyMaterialToNode(std::unique_ptr<Material> *ppMaterial, Node *pNode);
	void RemoveNodeMaterial(Node* pNode);

	//template <typename T, typename = std::enable_if_t<std::is_base_of_v<Material, T>>>
	//Material *AddMaterial()
	//{
	//	_materials.emplace_back(std::make_unique<T>());
	//	return _materials.back().get();
	//}

	//template <typename T, typename = std::enable_if_t<std::is_base_of_v<Node, T>>>
	//Node *AddNode(T *parentNode)
	//{
	//	nodes.emplace_back(std::make_unique<T>());
	//	auto node = nodes.back().get();
	//	parentNode->AddChild(node);
	//	return node;
	//}
	//ModelView* AddModelView(Node* pParent);
	//Camera* AddCamera(Node* pParent);
	//Light* AddLight(Node* pParent);


	void RemoveNode(Node *pNode);

	//template <typename T, typename = std::enable_if<std::is_base_of_v<Node, T>>>
	//T *CreateNode(NodeType type, Node *pParent = nullptr)
	//{
	//	_nodes.emplace_back(std::make_unique<T>());
	//	auto pNode=_nodes.back().get();
	//	//switch (pNode->nodeType)
	//	//{
	//	//case NodeType::NONE:
	//	//default:
	//	//	break;
	//	//case NodeType::CAMERA:
	//	//	break;
	//	//case NodeType::LIGHT:
	//	//	break;
	//	//case NodeType::MODEL_VIEW:
	//	//	break;
	//	//}

	//	if (pParent)
	//		pParent->AddChild(pNode);
	//	return static_cast<T *>(pNode);
	//}
	Node *CreateNode(Node *parent = nullptr,const char* pName="new node");
	Light *CreateLight(LightType type, Node *parent = nullptr,const char* pName="new light");
	Camera *CreateCamera(CameraType type, Node *parent = nullptr,const char* pName="new camera");
	ModelView *CreateModelView(ModelType type, Node *parent = nullptr,const char* pName="new modelView");

	void Destroy(Node *pNode);
	void Destroy(Model2 *pModel);

	void UpdateDefaultUBOBuffers(Camera *camera, uint32_t imageIndex);
	void Update(uint32_t imageIndex,Framebuffer* pFramebuffer);

	//=================================
	//GUI
	friend class GUI;
};
