#pragma once
#include "resourcebase.hpp"
#include "material.hpp"

#define TINYGLTF_USE_RAPIDJSON
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include <tiny_gltf.h>

enum class ModelType
{
	CUSTOM,
	CUBE,
	SPHERE,
	QUAD,
	ROCK,
	TEAPOT,
	CUBE_TEXTURE,
	TORUSKNOT,
	TUNNEL_CYLINDER,
	SKYBOX,
	AXIS,
	NUM
};

namespace tinygltf
{
	class Model;
	//	struct Accessor;
	//	struct Animation;
	//	struct Buffer;
	//	struct BufferView;
	//	struct Material;
	struct Mesh;
	//	class Node;
	//	struct Texture;
	//	struct Image;
	//	struct Skin;
	//	struct Sampler;
	//	struct Camera;
	//	struct Scene;
	//	struct Light;
}

struct PrimitiveView
{
	//uint64_t vertexBufferOffset{};
	//int64_t indexBufferOffset{-1};
	//uint32_t firstVertex;
	//int32_t firstIndex;
	int32_t material{-1};
	//DrawIndirectInfo drawIndirectInfo;

	std::variant<
		DrawIndirectCommand,
		DrawIndexedIndirectCommand>
		drawCommand;
};

struct MeshView
{
	std::vector<PrimitiveView> primitiveViews;
};

struct BoundingVolume
{
	struct Box
	{
		struct AABB
		{
			glm::dvec3 minValue{std::numeric_limits<double>::max()};
			glm::dvec3 maxValue{std::numeric_limits<double>::lowest()};
			glm::dvec3 center;
		} aabb;
		struct OBB
		{
		} obb;
	} box;
	struct Sphere
	{
	} sphere;
	struct Ellipsoid
	{
	} ellipsoid;
	struct Cylinder
	{
	} cylinder;
};


class Model2 : public ResourceBase
{
protected:
	//Buffer *_indirectDrawCmdBuffer{};
	DrawIndirectCommand _indirectDrawCommand;

	std::vector<Buffer *> _frameInstanceAttributeBuffers;
	std::vector<InstanceAttribute> _instanceAttributes;
	std::vector<uint32_t> _instanceIds;

	Material* _material;
	bool _useModelMaterial{true};

	ModelType _modelType;

	std::filesystem::path _modelPath;
	tinygltf::Model _model;

	Buffer *_indexBuffer{};
	Buffer *_vertexBuffer{};
	std::vector<MeshView> _meshViews;

	VertexInputStateCreateInfo _vertexInputStateCreateInfo;

	DescriptorPool *_descriptorPool;

	//skin info
	std::vector<std::vector<Buffer *>> _frameSkinJointMatrixBuffers;
	std::vector<std::vector<DescriptorSet *>> _frameSkinJointMatrixDescriptorSets;

	//materials  info
	std::vector<Image*> _images;
	std::vector<ImageView*> _imageViews;
	Sampler* _linearSampler;

	Buffer *_materialBuffer; //
	std::vector<DescriptorSet *> _materialDescriptorSets;

	//node animation info
	struct NodeTransformation
	{
		glm::dmat4 translate{1};
		glm::dmat4 rotate{1};
		glm::dmat4 scale{1};
		glm::dmat4 extra{1};
	};
	struct UBONodeAttribute
	{
		alignas(MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT) glm::mat4 matrix;
	};
	struct AnimationState
	{
		float startTime{std::numeric_limits<float>::max()};
		float endTime{std::numeric_limits<float>::min()};
	};
	std::vector<AnimationState> _animationStates;
	std::vector<NodeTransformation> _nodeTransformations;
	std::vector<Buffer *> _frameNodeAttributeBuffers;
	std::vector<std::vector<DescriptorSet *>> _frameNodeDescriptorSets;

	//bounding volume
	BoundingVolume _boundingVolume;

	//draw command info
	//std::vector<DrawIndirectInfo> _primitivesDrawIndirectInfo;

	//====================================
	void createDrawCommandInfo();
	//void createModelDrawCommandInfo();
	//void createSkyboxDrawCommandInfo();
	//void createAxisDrawCommandInfo();

	void drawModel(uint32_t sceneIndex, CommandBuffer *pCommandBuffer, uint32_t imageIndex);
	void drawNode(uint32_t nodeIndex);
	void drawMesh(const MeshView &meshView);

	void createDescriptorSets();
	void createSamplers();
	bool loadModel(const char *filePath);
	void createVertexInputStateInfo();
	void loadMaterials();
	void loadMeshes();
	void loadImages();
	void loadNodes();
	void loadNodesHelper(int nodeIndex, std::vector<NodeTransformation> &nodeTransformations, std::vector<UBONodeAttribute> &nodeAttributes, const glm::dmat4 &preMatrix);
	void loadSkins();

	void updateAnimation(uint32_t animationIndex, float time, uint32_t imageIndex);
	void updateAnimationHelper(int nodeIndex, std::vector<glm::mat4> &nodeMatrices, const glm::dmat4 &preMatrix);
	void updateSkins(int nodeIndex, std::vector<glm::mat4> &jointMatrices, const std::vector<glm::mat4> &nodeMatrices);

public:
	Model2(ModelType modelType);
	Model2(const char *filePath);
	~Model2();
	void draw(CommandBuffer *pCommandBuffer, uint32_t imageIndex);
	void useModelMaterial(bool flag);

	uint32_t addInstance(const InstanceAttribute &instanceAttribute);
	void addInstances(const std::vector<InstanceAttribute> &instanceAttributes,std::vector<uint32_t>& instanceIds);
	void removeInstance(int id);

	void updateInstanceAttributes(uint32_t imageIndex, uint32_t id, const InstanceAttribute &instanceAttribute);
	InstanceAttribute getInstanceAttribute(uint32_t index);
};