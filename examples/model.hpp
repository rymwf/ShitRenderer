/**
 * @file model.hpp
 * @author yangzs
 * @brief 	simple gltf model loader, do not support morph and sparse accessor
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "common.hpp"

const unsigned char COLOR_WHITE[4]{255, 255, 255, 255};
const unsigned char COLOR_BLACK[4]{0, 0, 0, 1};

struct Material
{
	alignas(MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT) float emissiveFactor[3]{};
	float alphaCutoff{0.5};
	alignas(16) float baseColorFactor[4]{1.f, 1.f, 1.f, 1.f};
	float metallic{1.};
	float roughness{0.};
	float normalTextureFactor{0};
};
struct NodeAttribute
{
	alignas(MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT) glm::mat4 matrix;
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

class Model
{
public:
	Model(const char *filePath);
	~Model();

	void DownloadModel(Device *pDevice, PipelineLayout *pipelineLayout, uint32_t imageCount);

	void SetFrameInstanceAttribute(Device *pDevice, uint32_t imageIndex, uint32_t index, const InstanceAttribute &instanceAttribute);

	void AddInstances(Device *pDevice, const std::vector<InstanceAttribute> &instanceAttributes);
	void AssignInstances(Device *pDevice, const std::vector<InstanceAttribute> &instanceAttributes);

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
		uint32_t imageIndex,
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
	/**
	 * @brief 
	 * 
	 * @param index 
	 * @param time second
	 * @param imageIndex 
	 */
	void UpdateAnimation(uint32_t index, float time, uint32_t imageIndex);

	constexpr decltype(auto) GetModelBoundingVolumePtr() const
	{
		return &mBoundingVolume;
	}
	int GetAnimationNum() const
	{
		return mAnimationStates.size();
	}
	uint32_t GetJointMatrixMaxNum() const;

private:
	std::unique_ptr<tinygltf::Model> mpModel;
	std::filesystem::path mModelPath;
	bool mLoadSucceed{};
	uint32_t mImageCount; //device?

	//TODO: changing with animation
	BoundingVolume mBoundingVolume;

	CommandBuffer *mpCurCommandBuffer;
	Device *mpCurDevice;
	uint32_t mCurSceneIndex;
	PipelineLayout *mpCurPipelineLayout;
	uint32_t mCurImageIndex;

	std::vector<int> mNodeToJointIndex; //only skin 0
	std::vector<InstanceAttribute> mInstanceAttributes;

	struct NodeTransformation
	{
		glm::dmat4 translate{1};
		glm::dmat4 rotate{1};
		glm::dmat4 scale{1};
		glm::dmat4 extra{1};
	};

	struct AnimationState
	{
		float startTime{std::numeric_limits<float>::max()};
		float endTime{std::numeric_limits<float>::min()};
	};
	std::vector<AnimationState> mAnimationStates;
	std::vector<NodeTransformation> mDefaultNodeTransformations;

	struct ModelAsset
	{
		std::vector<Buffer *> buffers;

		std::vector<DrawIndirectInfo> primitivesDrawIndirectInfo;
		std::vector<Image *> images;
		std::vector<ImageView *> imageViews;
		std::vector<Sampler *> samplers;

		std::vector<Buffer *> frameInstanceAttributeBuffers;

		Buffer *materialBuffer; //
		std::vector<DescriptorSet *> materialDescriptorSets;

		//TODO: use texture buffer to include attributes of different instances
		std::vector<Buffer *> frameNodeAttributeBuffers;
		std::vector<std::vector<DescriptorSet *>> frameNodeDescriptorSets;

		std::vector<std::vector<Buffer *>> frameSkinJointMatrixBuffers;
		std::vector<std::vector<DescriptorSet *>> frameSkinJointMatrixDescriptorSets;

		DescriptorPool *descriptorPool;
	};
	std::unordered_map<Device *, ModelAsset> mModelAssets;

	VertexInputStateCreateInfo mVertexInputStateCreateInfo;

	uint32_t mInstanceAttributeBufferCapacity{};
	uint32_t mInstanceAttributeBufferSize{};
	void AllocateFrameInstanceAttributeBuffers(Device *pDevice, uint32_t size, void *pData);

	void DrawNode(uint32_t nodeIndex);
	void DrawMesh(const tinygltf::Mesh &mesh);
	IndexType GetIndexType(int32_t componentSize);

	void LoadNode(int nodeIndex, std::vector<NodeTransformation> &nodeTransformations, std::vector<NodeAttribute> &nodeAttributes, const glm::dmat4 &preMatrix);
	void LoadMaterial();
	void LoadImages();
	void CreateDrawCommandInfo();
	void CreateDescriptorSets(uint32_t imageCount);
	void LoadNodeAttributes();
	void CreateVertexInputStateInfo();
	void UpdateNodeAnimation(int nodeIndex, std::vector<glm::mat4> &nodeMatrices, const glm::dmat4 &preMatrix);
	void LoadSkins();
	void UpdateSkins(int nodeIndex, std::vector<glm::mat4> &jointMatrices, const std::vector<glm::mat4> &nodeMatrices);
};
