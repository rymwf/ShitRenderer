#pragma once
#include "common.hpp"
#include "material.hpp"
#include "model2.hpp"

extern std::unique_ptr<Material> *g_ppDefaultMaterial;

enum class NodeType
{
	NONE,
	CAMERA,
	LIGHT,
	MODEL_VIEW,
};

struct Icon
{
};

struct Node : ResourceBase
{
	inline static NodeType nodeType = NodeType::NONE;
	Node *pParent{};
	std::vector<Node *> children;

	glm::quat rotation{1, 0, 0, 0};
	glm::vec3 translation{};
	glm::vec3 scale{1};

	glm::mat4 globalMatrix{1};
	bool updated{};

	static inline std::array<std::unique_ptr<Model2>, size_t(ModelType::NUM)> models;

	std::unique_ptr<Material> *ppMaterial{};
	Model2 *pModel{};
	uint32_t instanceId{};

	bool enable{true};

	void Update();
	void AddChild(Node *pNode);
	void RemoveChild(Node *pNode);
	Node();
	virtual ~Node() {}
};

enum class CameraType
{
	PERSPECTIVE,
	ORTHO,
};

struct Camera : public Node
{
	CameraType cameraType;
	Node eye;
	FrustumDescription frustumDescription;
	glm::mat4 frustumMatrix;
	bool frustumUpdated;
	void UpdateFrustum();
	glm::mat4 GetProjectionMatrix();
	Camera(CameraType type,const char* pName="new camera");
	~Camera();
};

enum class LightType
{
	DIRECTIONAL,
	//POINT,
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
	Light(LightType lightType,const char* pName="new light");
};

struct ModelView : public Node
{
	ModelView(ModelType modelType,const char* pName="new model");
	ModelView(Model2 *model,const char* pName="new model")
	{
		_name = pName;
		pModel = model;
		nodeType = NodeType::MODEL_VIEW;
	}
	~ModelView()
	{
	}
};