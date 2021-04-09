/**
 * @file model.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "model.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_RAPIDJSON
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include <tiny_gltf.h>

bool loadModel(tinygltf::Model &model, const char *filename)
{
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
	if (!warn.empty())
	{
		std::cout << "WARN: " << warn << std::endl;
	}

	if (!err.empty())
	{
		std::cout << "ERR: " << err << std::endl;
	}

	if (!res)
		std::cout << "Failed to load glTF: " << filename << std::endl;
	else
		std::cout << "Loaded glTF: " << filename << std::endl;

	return res;
}
DataType getComponentDataType(uint32_t componentType)
{
	if (componentType == TINYGLTF_COMPONENT_TYPE_BYTE)
	{
		return DataType::BYTE;
	}
	else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
	{
		return DataType::UNSIGNED_BYTE;
	}
	else if (componentType == TINYGLTF_COMPONENT_TYPE_SHORT)
	{
		return DataType::SHORT;
	}
	else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
	{
		return DataType::UNSIGNED_SHORT;
	}
	else if (componentType == TINYGLTF_COMPONENT_TYPE_INT)
	{
		return DataType::INT;
	}
	else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
	{
		return DataType::UNSIGNED_INT;
	}
	else if (componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
	{
		return DataType::FLOAT;
	}
	else if (componentType == TINYGLTF_COMPONENT_TYPE_DOUBLE)
	{
		return DataType::DOUBLE;
	}
	else
	{
		// Unknown componenty type
		THROW("unknown component type");
	}
}

Model::Model(const char *filePath)
{
	mModelPath = filePath;
	mpModel = std::make_unique<tinygltf::Model>();
	if (!loadModel(*mpModel, filePath))
		return;
	mLoadSucceed = true;

	//init animation state
	for (auto &&animation : mpModel->animations)
	{
		mAnimationStates.emplace_back();
		for (auto &&sampler : animation.samplers)
		{
			auto &&accessor = mpModel->accessors[sampler.input];
			if (mAnimationStates.back().endTime < accessor.maxValues[0])
				mAnimationStates.back().endTime = static_cast<float>(accessor.maxValues[0]);
			if (mAnimationStates.back().startTime > accessor.minValues[0])
				mAnimationStates.back().startTime = static_cast<float>(accessor.minValues[0]);
		}
	}

	//init bounding volume
	for (auto &&mesh : mpModel->meshes)
	{
		for (auto &&primitive : mesh.primitives)
		{
			auto &&accessor = mpModel->accessors[primitive.attributes.at("POSITION")];
			mBoundingVolume.box.aabb.minValue = glm::min(mBoundingVolume.box.aabb.minValue, glm::vec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]));
			mBoundingVolume.box.aabb.maxValue = glm::max(mBoundingVolume.box.aabb.maxValue, glm::vec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]));
		}
	}
	mBoundingVolume.box.aabb.center = (mBoundingVolume.box.aabb.maxValue + mBoundingVolume.box.aabb.minValue) / glm::vec3(2.);
	CreateVertexInputStateInfo();
}
void Model::CreateVertexInputStateInfo()
{
	//init vertexInputStateInfo
	mVertexInputStateCreateInfo.vertexAttributeDescriptions = {
		{LOCATION_POSITION, LOCATION_POSITION, 3, DataType::FLOAT, false, 0},
		{LOCATION_NORMAL, LOCATION_NORMAL, 3, DataType::FLOAT, false, 0},
		{LOCATION_TANGENT, LOCATION_TANGENT, 4, DataType::FLOAT, false, 0},
		{LOCATION_TEXCOORD0, LOCATION_TEXCOORD0, 2, DataType::FLOAT, false, 0},
		{LOCATION_TEXCOORD1, LOCATION_TEXCOORD1, 2, DataType::FLOAT, false, 0},
		{LOCATION_COLOR0, LOCATION_COLOR0, 4, DataType::FLOAT, false, 0},
		{LOCATION_JOINTS0, LOCATION_JOINTS0, 4, DataType::FLOAT, false, 0},
		{LOCATION_WEIGHTS0, LOCATION_WEIGHTS0, 4, DataType::FLOAT, false, 0},
		{LOCATION_INSTANCE_COLOR_FACTOR, LOCATION_INSTANCE_COLOR_FACTOR, 4, DataType::FLOAT, false, 0},
		{LOCATION_INSTANCE_MATRIX + 0, LOCATION_INSTANCE_COLOR_FACTOR, 4, DataType::FLOAT, false, 16},
		{LOCATION_INSTANCE_MATRIX + 1, LOCATION_INSTANCE_COLOR_FACTOR, 4, DataType::FLOAT, false, 32},
		{LOCATION_INSTANCE_MATRIX + 2, LOCATION_INSTANCE_COLOR_FACTOR, 4, DataType::FLOAT, false, 48},
		{LOCATION_INSTANCE_MATRIX + 3, LOCATION_INSTANCE_COLOR_FACTOR, 4, DataType::FLOAT, false, 64},
	};

	mVertexInputStateCreateInfo.vertexBindingDescriptions =
		{
			{LOCATION_POSITION},
			{LOCATION_NORMAL},
			{LOCATION_TANGENT},
			{LOCATION_TEXCOORD0},
			{LOCATION_TEXCOORD1},
			{LOCATION_COLOR0},
			{LOCATION_JOINTS0},
			{LOCATION_WEIGHTS0},
			{LOCATION_INSTANCE_COLOR_FACTOR, sizeof(InstanceAttribute), 1},
		};
	for (auto &&attrib : mpModel->meshes[0].primitives[0].attributes)
	{
		int location = -1;
		if (attrib.first.compare("POSITION") == 0)
			location = LOCATION_POSITION;
		else if (attrib.first.compare("NORMAL") == 0)
			location = LOCATION_NORMAL;
		else if (attrib.first.compare("TANGENT") == 0)
			location = LOCATION_TANGENT;
		else if (attrib.first.compare("TEXCOORD_0") == 0)
			location = LOCATION_TEXCOORD0;
		else if (attrib.first.compare("TEXCOORD_1") == 0)
			location = LOCATION_TEXCOORD1;
		else if (attrib.first.compare("COLOR_0") == 0)
			location = LOCATION_COLOR0;
		else if (attrib.first.compare("JOINTS_0") == 0)
			location = LOCATION_JOINTS0;
		else if (attrib.first.compare("WEIGHTS_0") == 0)
			location = LOCATION_WEIGHTS0;
		if (location != -1)
		{
			auto &&accessor = mpModel->accessors[attrib.second];
			auto componentNum = tinygltf::GetNumComponentsInType(accessor.type);
			uint32_t stride = tinygltf::GetComponentSizeInBytes(accessor.componentType) * componentNum;

			//location equals to binding
			mVertexInputStateCreateInfo.vertexBindingDescriptions[location].stride = stride;
			mVertexInputStateCreateInfo.vertexAttributeDescriptions[location].components = componentNum;
			mVertexInputStateCreateInfo.vertexAttributeDescriptions[location].dataType = getComponentDataType(accessor.componentType);
			mVertexInputStateCreateInfo.vertexAttributeDescriptions[location].normalized = accessor.normalized;
		}
	}
}
Model::~Model()
{
	for (auto &&e : mModelAssets)
	{
		for (auto &&a : e.second.frameInstanceAttributeBuffer)
			e.first->Destroy(a);
		for (auto &&a : e.second.primitivesDrawIndirectInfo)
			e.first->Destroy(a.pBuffer);
		for (auto &&buffer : e.second.buffers)
			e.first->Destroy(buffer);
		for (auto &&a : e.second.images)
			e.first->Destroy(a);
		for (auto &&a : e.second.imageViews)
			e.first->Destroy(a);
		for (auto &&a : e.second.samplers)
			e.first->Destroy(a);
		e.first->Destroy(e.second.materialBuffer);
		for (auto &&a : e.second.frameNodeAttributeBuffers)
			e.first->Destroy(a);
		e.first->Destroy(e.second.descriptorPool);
	}
}
void Model::FreeModel(Device *pDevice)
{
	for (auto &&e : mModelAssets[pDevice].primitivesDrawIndirectInfo)
		pDevice->Destroy(e.pBuffer);
	for (auto &&buffer : mModelAssets[pDevice].buffers)
		pDevice->Destroy(buffer);
	for (auto &&e : mModelAssets[pDevice].images)
		pDevice->Destroy(e);
	for (auto &&e : mModelAssets[pDevice].imageViews)
		pDevice->Destroy(e);
	for (auto &&e : mModelAssets[pDevice].samplers)
		pDevice->Destroy(e);
	for (auto &&e : mModelAssets[pDevice].frameInstanceAttributeBuffer)
		pDevice->Destroy(e);
	pDevice->Destroy(mModelAssets[pDevice].materialBuffer);
	for (auto &&e : mModelAssets[pDevice].frameNodeAttributeBuffers)
		pDevice->Destroy(e);

	pDevice->Destroy(mModelAssets[pDevice].descriptorPool);

	mModelAssets.erase(pDevice);
}
size_t Model::GetSceneCount() const
{
	return mpModel->scenes.size();
}
void Model::DownloadModel(Device *pDevice, PipelineLayout *pipelineLayout, uint32_t imageCount)
{
	mpCurPipelineLayout = pipelineLayout;
	mpCurDevice = pDevice;

	//scene buffers
	for (auto &&buffer : mpModel->buffers)
	{
		BufferCreateInfo createInfo{
			.size = buffer.data.size(),
			.usage = BufferUsageFlagBits::INDEX_BUFFER_BIT | BufferUsageFlagBits::UNIFORM_BUFFER_BIT | BufferUsageFlagBits::VERTEX_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			.memoryPropertyFlags = MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		mModelAssets[pDevice].buffers.emplace_back(pDevice->Create(createInfo, buffer.data.data()));
	}

	CreateDescriptorSets(imageCount);

	LoadNodeAttributes();
	LoadImages();
	LoadMaterial();
}
void Model::CreateDescriptorSets(uint32_t imageCount)
{
	auto descriptorSetLayouts = mpCurPipelineLayout->GetCreateInfoPtr()->setLayouts;
	//descriptor pool
	std::vector<DescriptorPoolSize> poolSizes;
	for (auto &&e : descriptorSetLayouts[1]->GetCreateInfoPtr()->descriptorSetLayoutBindings)
	{
		poolSizes.emplace_back(e.descriptorType, e.descriptorCount);
	}
	for (auto &&e : descriptorSetLayouts[2]->GetCreateInfoPtr()->descriptorSetLayoutBindings)
	{
		poolSizes.emplace_back(e.descriptorType, e.descriptorCount);
	}
	auto materialCount = (std::max)(mpModel->materials.size(), size_t(1));
	auto nodeCount = mpModel->nodes.size();

	DescriptorPoolCreateInfo descriptorPoolCreateInfo{materialCount + nodeCount * imageCount, poolSizes};
	mModelAssets[mpCurDevice].descriptorPool = mpCurDevice->Create(descriptorPoolCreateInfo);

	std::vector<DescriptorSetLayout *> materialSetLayouts(materialCount, descriptorSetLayouts[DESCRIPTORSET_ID_MATERIAL]);
	DescriptorSetAllocateInfo allocInfo{materialSetLayouts};
	mModelAssets[mpCurDevice].descriptorPool->Allocate(allocInfo, mModelAssets[mpCurDevice].materialDescriptorSets);

	std::vector<DescriptorSetLayout *> nodeSetLayouts(nodeCount, descriptorSetLayouts[DESCRIPTORSET_ID_NODE]);
	allocInfo = DescriptorSetAllocateInfo{nodeSetLayouts};

	mModelAssets[mpCurDevice].frameNodeDescriptorSets.resize(imageCount);
	for (uint32_t k = 0; k < imageCount; ++k)
		mModelAssets[mpCurDevice].descriptorPool->Allocate(allocInfo, mModelAssets[mpCurDevice].frameNodeDescriptorSets[k]);
}
void Model::LoadNodeAttributes()
{
	auto nodeCount = mpModel->nodes.size();
	mDefaultNodeAttributes.resize(nodeCount);
	for (auto &&scene : mpModel->scenes)
	{
		for (auto &&nodeIndex : scene.nodes)
		{
			LoadNode(nodeIndex, mDefaultNodeAttributes, glm::dmat4(1));
		}
	}

	//update bounding volume
	mBoundingVolume.box.aabb.minValue = glm::mat3(mDefaultNodeAttributes[0].matrix) * mBoundingVolume.box.aabb.minValue;
	mBoundingVolume.box.aabb.maxValue = glm::mat3(mDefaultNodeAttributes[0].matrix) * mBoundingVolume.box.aabb.maxValue;
	mBoundingVolume.box.aabb.center = glm::mat3(mDefaultNodeAttributes[0].matrix) * mBoundingVolume.box.aabb.center;

	auto imageCount = mModelAssets[mpCurDevice].frameNodeDescriptorSets.size();
	mModelAssets[mpCurDevice].frameNodeAttributeBuffers.resize(imageCount);

	std::vector<WriteDescriptorSet> writes;
	for (uint32_t i = 0; i < imageCount; ++i)
	{
		mModelAssets[mpCurDevice].frameNodeAttributeBuffers[i] = mpCurDevice->Create(BufferCreateInfo{
																					{},
																					static_cast<uint32_t>(sizeof(NodeAttribute) * mDefaultNodeAttributes.size()),
																					BufferUsageFlagBits::UNIFORM_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
																					MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
																				reinterpret_cast<const void *>(mDefaultNodeAttributes.data()));
		for (size_t j = 0; j < nodeCount; ++j)
		{
			writes.emplace_back(
				WriteDescriptorSet{
					mModelAssets[mpCurDevice].frameNodeDescriptorSets[i][j],
					UNIFORM_BINDING_NODE,
					0,
					DescriptorType::UNIFORM_BUFFER,
					std::vector<DescriptorBufferInfo>{{mModelAssets[mpCurDevice].frameNodeAttributeBuffers[i],
													   sizeof(NodeAttribute) * j,
													   sizeof(NodeAttribute)}}});
		}
	}
	mpCurDevice->UpdateDescriptorSets(writes, {});
}
void Model::LoadImages()
{
	//load images
	auto curDir = mModelPath;
	curDir.remove_filename();

	ImageCreateInfo imageCreateInfo{
		{},
		ImageType::TYPE_2D,
		ShitFormat::RGBA8_UNORM,
		{},
		1,
		1,
		SampleCountFlagBits::BIT_1,
		ImageTiling::OPTIMAL,
		ImageUsageFlagBits::SAMPLED_BIT,
		MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
		true,
		Filter::LINEAR};

	ImageViewCreateInfo imageViewCreateInfo{
		nullptr,
		ImageViewType::TYPE_2D,
		ShitFormat::RGBA8_UNORM,
		{},
		{0, 1, 0, 1}};
	for (auto &&tex : mpModel->textures)
	{
		auto imagePath = curDir.string() + mpModel->images[tex.source].uri;
		int width, height, components;
		auto pixels = loadImage(imagePath.c_str(), width, height, components, 4);
		imageCreateInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2((std::max)(width, height))) + 1);
		imageCreateInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
		mModelAssets[mpCurDevice].images.emplace_back(mpCurDevice->Create(imageCreateInfo, pixels));

		//imageview
		imageViewCreateInfo.pImage = mModelAssets[mpCurDevice].images.back();
		imageViewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;
		mModelAssets[mpCurDevice].imageViews.emplace_back(mpCurDevice->Create(imageViewCreateInfo));
		freeImage(pixels);
	}
	//add default texture
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.extent = {1, 1, 1};

	//create default white image
	mModelAssets[mpCurDevice].images.emplace_back(mpCurDevice->Create(imageCreateInfo, COLOR_WHITE));
	imageViewCreateInfo.pImage = mModelAssets[mpCurDevice].images.back();
	imageViewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;
	mModelAssets[mpCurDevice].imageViews.emplace_back(mpCurDevice->Create(imageViewCreateInfo));

	//create default black image
	mModelAssets[mpCurDevice].images.emplace_back(mpCurDevice->Create(imageCreateInfo, COLOR_BLACK));
	imageViewCreateInfo.pImage = mModelAssets[mpCurDevice].images.back();
	imageViewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;
	mModelAssets[mpCurDevice].imageViews.emplace_back(mpCurDevice->Create(imageViewCreateInfo));

	//samplers
	SamplerCreateInfo samplerCreateInfo{
		Filter::LINEAR,
		Filter::LINEAR,
		SamplerMipmapMode::LINEAR,
		SamplerWrapMode::REPEAT,
		SamplerWrapMode::REPEAT,
		SamplerWrapMode::REPEAT,
		0,	   //load bias
		false, //anistropy
		false, //compare
		CompareOp::ALWAYS,
		0,
		30,
		BorderColor::FLOAT_OPAQUE_BLACK,
	};
	for (auto &&sampler : mpModel->samplers)
	{
		//TODO: edit sampler createInfo
		mModelAssets[mpCurDevice].samplers.emplace_back(mpCurDevice->Create(samplerCreateInfo));
	}
	if (mModelAssets[mpCurDevice].samplers.empty())
		mModelAssets[mpCurDevice].samplers.emplace_back(mpCurDevice->Create(samplerCreateInfo));
}
void Model::CreateImageInstanceAttributeBuffers(Device *pDevice, const std::vector<InstanceAttribute> &instanceAttributes, bool immutable)
{
	mModelAssets[pDevice].instanceCount = (std::max)(instanceAttributes.size(), size_t(1));
	//create Instance Attribute buffer
	BufferCreateInfo instanceAttributeCreateInfo{
		{},
		mModelAssets[pDevice].instanceCount * sizeof(InstanceAttribute),
		BufferUsageFlagBits::VERTEX_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
		immutable ? MemoryPropertyFlagBits::DEVICE_LOCAL_BIT : MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT};
	if (instanceAttributes.size() == 0)
	{
		InstanceAttribute attribute{glm::vec4{1}, glm::mat4{1}};
		for (size_t i = 0, len = mModelAssets[pDevice].frameNodeDescriptorSets.size(); i < len; ++i)
			mModelAssets[pDevice].frameInstanceAttributeBuffer.emplace_back(pDevice->Create(instanceAttributeCreateInfo, &attribute));
	}
	else
	{
		for (size_t i = 0, len = mModelAssets[pDevice].frameNodeDescriptorSets.size(); i < len; ++i)
			mModelAssets[pDevice].frameInstanceAttributeBuffer.emplace_back(pDevice->Create(instanceAttributeCreateInfo, reinterpret_cast<const void *>(instanceAttributes.data())));
	}
}
void Model::UpdateImageInstanceAttributes(Device *pDevice, uint32_t imageIndex, const std::vector<InstanceAttribute> &instanceAttributes)
{
	void *data;
	auto size = sizeof(InstanceAttribute) * mModelAssets[pDevice].instanceCount;
	mModelAssets[pDevice].frameInstanceAttributeBuffer[imageIndex]->MapBuffer(0, size, &data);
	memcpy(data, instanceAttributes.data(), size);
	mModelAssets[pDevice].frameInstanceAttributeBuffer[imageIndex]->UnMapBuffer();
}
void Model::CreateDrawCommandInfo()
{
	if (mModelAssets[mpCurDevice].frameInstanceAttributeBuffer.empty())
		CreateImageInstanceAttributeBuffers(mpCurDevice, {}, true);
	//primitive draw indexed indiect command info
	mModelAssets[mpCurDevice].primitivesDrawIndirectInfo.resize(mpModel->accessors.size(), {});
	for (auto &&mesh : mpModel->meshes)
	{
		for (auto &&primitive : mesh.primitives)
		{
			if (primitive.indices == -1)
			{
				int positionAccessorIndex = primitive.attributes.at("POSITION");
				auto &&positionAccessor = mpModel->accessors[positionAccessorIndex];
				DrawIndirectCommand cmd{
					positionAccessor.count,
					mModelAssets[mpCurDevice].instanceCount,
				};
				BufferCreateInfo createInfo{
					{},
					sizeof(DrawIndirectCommand),
					BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
					MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
				mModelAssets[mpCurDevice].primitivesDrawIndirectInfo[positionAccessorIndex] = {
					mpCurDevice->Create(createInfo, &cmd),
					0,
					1,
					sizeof(DrawIndirectInfo)};
			}
			else
			{
				auto &&indexAccessor = mpModel->accessors[primitive.indices];
				DrawIndexedIndirectCommand cmd{
					indexAccessor.count,
					mModelAssets[mpCurDevice].instanceCount,
				};
				BufferCreateInfo createInfo{
					{},
					sizeof(DrawIndexedIndirectCommand),
					BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
					MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
				mModelAssets[mpCurDevice].primitivesDrawIndirectInfo[primitive.indices] = {
					mpCurDevice->Create(createInfo, &cmd),
					0,
					1,
					sizeof(DrawIndirectInfo)};
			}
		}
	}
}
void Model::LoadMaterial()
{
	//material buffers
	auto materialCount = mpModel->materials.size();
	BufferCreateInfo bufferCreateInfo{
		{},
		sizeof(Material) * (std::max)(materialCount, size_t(1)),
		BufferUsageFlagBits::UNIFORM_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
		MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
	std::vector<Material> materials((std::max)(materialCount, size_t(1)), Material{});
	std::transform(mpModel->materials.begin(), mpModel->materials.end(), materials.begin(), [](auto &&m) {
		return Material{
			{
				static_cast<float>(m.emissiveFactor[0]),
				static_cast<float>(m.emissiveFactor[1]),
				static_cast<float>(m.emissiveFactor[2]),
			},
			static_cast<float>(m.alphaCutoff),
			{
				static_cast<float>(m.pbrMetallicRoughness.baseColorFactor[0]),
				static_cast<float>(m.pbrMetallicRoughness.baseColorFactor[1]),
				static_cast<float>(m.pbrMetallicRoughness.baseColorFactor[2]),
				static_cast<float>(m.pbrMetallicRoughness.baseColorFactor[3]),
			},
			static_cast<float>(m.pbrMetallicRoughness.metallicFactor),
			static_cast<float>(m.pbrMetallicRoughness.roughnessFactor),
		};
	});
	mModelAssets[mpCurDevice].materialBuffer = mpCurDevice->Create(bufferCreateInfo, (void *)materials.data());

	std::vector<WriteDescriptorSet> writes;
	int blackTextureIndex = mModelAssets[mpCurDevice].images.size() - 1;
	int whiteTextureIndex = mModelAssets[mpCurDevice].images.size() - 2;

	int samplerIndex = mModelAssets[mpCurDevice].samplers.size() - 1;
	if (materialCount == 0)
	{
		for (uint32_t i = 0; i < 6; ++i)
		{
			writes.emplace_back(
				WriteDescriptorSet{
					mModelAssets[mpCurDevice].materialDescriptorSets[0],
					i,
					0,
					DescriptorType::COMBINED_IMAGE_SAMPLER,
					std::vector<DescriptorImageInfo>{{mModelAssets[mpCurDevice].samplers[samplerIndex], //sampler
													  mModelAssets[mpCurDevice].imageViews[blackTextureIndex],
													  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
		}
		writes[TEXTURE_BINDING_ALBEDO].values =
			std::vector<DescriptorImageInfo>{{mModelAssets[mpCurDevice].samplers[samplerIndex], //sampler
											  mModelAssets[mpCurDevice].imageViews[whiteTextureIndex],
											  ImageLayout::SHADER_READ_ONLY_OPTIMAL}};
		writes.emplace_back(
			WriteDescriptorSet{
				mModelAssets[mpCurDevice].materialDescriptorSets[0],
				UNIFORM_BINDING_MATERIAL,
				0,
				DescriptorType::UNIFORM_BUFFER,
				std::vector<DescriptorBufferInfo>{{mModelAssets[mpCurDevice].materialBuffer,
												   0,
												   sizeof(Material)}}});
	}

	std::array<std::pair<int, int>, 6> texIndices;
	for (size_t i = 0; i < materialCount; ++i)
	{
		auto &&material = mpModel->materials[i];
		texIndices.fill({blackTextureIndex, samplerIndex});
		if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
		{
			int sampler = (std::max)(mpModel->textures[material.pbrMetallicRoughness.baseColorTexture.index].sampler, samplerIndex);
			texIndices[TEXTURE_BINDING_ALBEDO] = {material.pbrMetallicRoughness.baseColorTexture.index, sampler};
		}
		else
		{
			texIndices[TEXTURE_BINDING_ALBEDO].first = whiteTextureIndex;
		}

		if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
		{
			int sampler = (std::max)(mpModel->textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].sampler, samplerIndex);
			texIndices[TEXTURE_BINDING_METALLIC_ROUGHNESS] = {material.pbrMetallicRoughness.metallicRoughnessTexture.index, sampler};
		}
		if (material.normalTexture.index != -1)
		{
			int sampler = (std::max)(mpModel->textures[material.normalTexture.index].sampler, samplerIndex);
			texIndices[TEXTURE_BINDING_NORMAL] = {material.normalTexture.index, sampler};
		}
		if (material.occlusionTexture.index != -1)
		{
			int sampler = (std::max)(mpModel->textures[material.occlusionTexture.index].sampler, samplerIndex);
			texIndices[TEXTURE_BINDING_OCCLUSION] = {material.occlusionTexture.index, sampler};
		}
		if (material.emissiveTexture.index != -1)
		{
			int sampler = (std::max)(mpModel->textures[material.emissiveTexture.index].sampler, samplerIndex);
			texIndices[TEXTURE_BINDING_EMISSION] = {material.emissiveTexture.index, sampler};
		}

		for (uint32_t j = 0; j < 6; ++j)
		{
			writes.emplace_back(
				WriteDescriptorSet{
					mModelAssets[mpCurDevice].materialDescriptorSets[i],
					j,
					0,
					DescriptorType::COMBINED_IMAGE_SAMPLER,
					std::vector<DescriptorImageInfo>{{mModelAssets[mpCurDevice].samplers[texIndices[j].second], //sampler
													  mModelAssets[mpCurDevice].imageViews[texIndices[j].first],
													  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
		}
		//material descriptor update
		writes.emplace_back(
			WriteDescriptorSet{
				mModelAssets[mpCurDevice].materialDescriptorSets[i],
				UNIFORM_BINDING_MATERIAL,
				0,
				DescriptorType::UNIFORM_BUFFER,
				std::vector<DescriptorBufferInfo>{{mModelAssets[mpCurDevice].materialBuffer,
												   sizeof(Material) * i,
												   sizeof(Material)}}});
	}
	mpCurDevice->UpdateDescriptorSets(writes, {});
}
void Model::DrawModel(
	Device *pDevice,
	CommandBuffer *pCommandBuffer,
	uint32_t imageIndex,
	int sceneIndex)
{
	if (sceneIndex == -1)
	{
		sceneIndex = mpModel->defaultScene;
	}
	else if (sceneIndex == -2)
	{
		for (size_t i = 0, len = mpModel->scenes.size(); i < len; ++i)
		{
			DrawModel(pDevice, pCommandBuffer, i);
		}
	}
	mpCurDevice = pDevice;
	mCurSceneIndex = sceneIndex;
	mpCurCommandBuffer = pCommandBuffer;
	mCurImageIndex = imageIndex;

	if (mModelAssets[mpCurDevice].primitivesDrawIndirectInfo.empty())
		CreateDrawCommandInfo();

	LOG_VAR(mpModel->scenes[sceneIndex].name)
	for (auto &&nodeIndex : mpModel->scenes[sceneIndex].nodes)
	{
		DrawNode(nodeIndex);
	}
}
void Model::DrawNode(uint32_t nodeIndex)
{

	mpCurCommandBuffer->BindDescriptorSets(
		BindDescriptorSetsInfo{
			PipelineBindPoint::GRAPHICS,
			mpCurPipelineLayout,
			DESCRIPTORSET_ID_NODE,
			1,
			&mModelAssets[mpCurDevice].frameNodeDescriptorSets[mCurImageIndex][nodeIndex]});
	//	mpCurCommandBuffer->PushConstants(PushConstantUpdateInfo{
	//		mpCurPipelineLayout,
	//		ShaderStageFlagBits::VERTEX_BIT,
	//		0,
	//		sizeof(glm::mat4),
	//		&mAnimationNodeMatrices[nodeIndex]});

	auto &&node = mpModel->nodes[nodeIndex];
	if ((node.mesh >= 0) && (node.mesh < mpModel->meshes.size()))
	{
		DrawMesh(mpModel->meshes[node.mesh]);
	}
	for (size_t i = 0; i < node.children.size(); i++)
	{
		assert((node.children[i] >= 0) && (node.children[i] < mpModel->nodes.size()));
		DrawNode(node.children[i]);
	}
}
void Model::DrawMesh(const tinygltf::Mesh &mesh)
{
	BindVertexBufferInfo bindVertexBufferInfo{0, VERTEX_LOCATION_NUM_MAX};
	for (auto &&primitive : mesh.primitives)
	{
		//
		int materialDescriptorId = primitive.material >= 0 ? primitive.material : mModelAssets[mpCurDevice].materialDescriptorSets.size() - 1;
		mpCurCommandBuffer->BindDescriptorSets(
			BindDescriptorSetsInfo{
				PipelineBindPoint::GRAPHICS,
				mpCurPipelineLayout,
				DESCRIPTORSET_ID_MATERIAL,
				1,
				&mModelAssets[mpCurDevice].materialDescriptorSets[materialDescriptorId]});

		std::array<Buffer *, VERTEX_LOCATION_NUM_MAX> buffers{};
		buffers.fill(mModelAssets[mpCurDevice].buffers[0]); //TODO: ....
		buffers[LOCATION_INSTANCE_COLOR_FACTOR] = mModelAssets[mpCurDevice].frameInstanceAttributeBuffer[mCurImageIndex];
		std::array<uint64_t, VERTEX_LOCATION_NUM_MAX> offsets{};
		for (auto &&attrib : primitive.attributes)
		{
			auto &&accessor = mpModel->accessors[attrib.second];
			int index = -1;
			if (attrib.first.compare("POSITION") == 0)
				index = LOCATION_POSITION;
			else if (attrib.first.compare("NORMAL") == 0)
				index = LOCATION_NORMAL;
			else if (attrib.first.compare("TANGENT") == 0)
				index = LOCATION_TANGENT;
			else if (attrib.first.compare("TEXCOORD_0") == 0)
				index = LOCATION_TEXCOORD0;
			else if (attrib.first.compare("TEXCOORD_1") == 0)
				index = LOCATION_TEXCOORD1;
			else if (attrib.first.compare("COLOR_0") == 0)
				index = LOCATION_COLOR0;
			else if (attrib.first.compare("JOINTS_0") == 0)
				index = LOCATION_JOINTS0;
			else if (attrib.first.compare("WEIGHTS_0") == 0)
				index = LOCATION_WEIGHTS0;
			if (index >= 0)
			{
				buffers[index] = mModelAssets[mpCurDevice].buffers[mpModel->bufferViews[accessor.bufferView].buffer];
				offsets[index] = mpModel->bufferViews[accessor.bufferView].byteOffset + accessor.byteOffset;
			}
		}
		bindVertexBufferInfo.ppBuffers = buffers.data();
		bindVertexBufferInfo.pOffsets = offsets.data();

		mpCurCommandBuffer->BindVertexBuffer(bindVertexBufferInfo);

		if (primitive.indices == -1)
		{
			int positionAccessorIndex = primitive.attributes.at("POSITION");
			auto &&positionAccessor = mpModel->accessors[positionAccessorIndex];
			auto &&bufferView = mpModel->bufferViews[positionAccessor.bufferView];
			mpCurCommandBuffer->DrawIndirect(mModelAssets[mpCurDevice].primitivesDrawIndirectInfo[positionAccessorIndex]);
		}
		else
		{
			//bind indexbuffer
			auto &&indexAccessor = mpModel->accessors[primitive.indices];
			auto &&bufferView = mpModel->bufferViews[indexAccessor.bufferView];
			mpCurCommandBuffer->BindIndexBuffer(BindIndexBufferInfo{
				mModelAssets[mpCurDevice].buffers[bufferView.buffer],
				bufferView.byteOffset + indexAccessor.byteOffset,
				GetIndexType(tinygltf::GetComponentSizeInBytes(indexAccessor.componentType))});
			mpCurCommandBuffer->DrawIndexedIndirect(mModelAssets[mpCurDevice].primitivesDrawIndirectInfo[primitive.indices]);
		}
	}
}
IndexType Model::GetIndexType(int32_t componentSize)
{
	if (componentSize == 1)
		return IndexType::UINT8;
	else if (componentSize == 2)
		return IndexType::UINT16;
	else
		return IndexType::UINT32;
}
void Model::LoadNode(int nodeIndex, std::vector<NodeAttribute> &nodeAttributes, const glm::dmat4 &preMatrix)
{
	glm::dmat4 rotate(1);
	glm::dmat4 scale(1);
	glm::dmat4 translation(1);
	glm::dmat4 extra(1);

	auto &&node = mpModel->nodes[nodeIndex];
	if (!node.rotation.empty())
	{
		rotate = glm::mat4_cast(glm::dquat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]));
	}
	if (!node.scale.empty())
		scale = glm::scale(glm::dmat4(1), glm::dvec3(node.scale[0], node.scale[1], node.scale[2]));
	if (!node.translation.empty())
		translation = glm::translate(glm::dmat4(1), glm::dvec3(node.translation[0], node.translation[1], node.translation[2]));
	if (!node.matrix.empty())
		memcpy(&extra, node.matrix.data(), sizeof(double) * node.matrix.size());
	nodeAttributes[nodeIndex].matrix = extra * translation * rotate * scale * preMatrix;
	for (auto &&subNodeIndex : node.children)
	{
		LoadNode(subNodeIndex, nodeAttributes, nodeAttributes[nodeIndex].matrix);
	}
}
void Model::UpdateAnimation(uint32_t index, float time, uint32_t imageIndex)
{
	auto period = mAnimationStates[index].endTime - mAnimationStates[index].startTime;
	auto c = time / period;
	c -= static_cast<long long>(c);
	auto animationTime = c * period + mAnimationStates[index].startTime;

	static std::vector<glm::mat4> nodeMatrices(mpModel->nodes.size(), glm::mat4(1));
	static std::vector<bool> edited(mpModel->nodes.size());
	nodeMatrices.assign(mpModel->nodes.size(), glm::mat4(1));
	edited.assign(mpModel->nodes.size(), false);

	for (auto &&channel : mpModel->animations[index].channels)
	{
		auto &&sampler = mpModel->animations[index].samplers[channel.sampler];
		auto &&inputAccessor = mpModel->accessors[sampler.input];
		auto &&inputBufferView = mpModel->bufferViews[inputAccessor.bufferView];

		float *pPreTime = reinterpret_cast<float *>(mpModel->buffers[inputBufferView.buffer].data.data() + inputAccessor.byteOffset + inputBufferView.byteOffset);
		if (*pPreTime > animationTime)
			continue;

		auto &&outputAccessor = mpModel->accessors[sampler.output];
		auto &&outputBufferView = mpModel->bufferViews[outputAccessor.bufferView];

		float *pNextTime = pPreTime + 1;
		edited[channel.target_node] = true;
		auto outValueSize = tinygltf::GetComponentSizeInBytes(outputAccessor.componentType);
		auto outValueComponentCount = tinygltf::GetNumComponentsInType(outputAccessor.type);
		for (size_t i = 0; i < inputAccessor.count; ++i, ++pPreTime, ++pNextTime)
		{
			if (*pNextTime > animationTime)
			{
				auto factor = (animationTime - *pPreTime) / (*pNextTime - *pPreTime);
				unsigned char *pPreOutValue = mpModel->buffers[outputBufferView.buffer].data.data() + outputAccessor.byteOffset + outputBufferView.byteOffset + i * outValueSize * outValueComponentCount;
				unsigned char *pNextOutValue = pPreOutValue + outValueSize * outValueComponentCount;
				if (channel.target_path.compare("translation") == 0)
				{
					auto preVal = glm::vec3(*reinterpret_cast<float *>(pPreOutValue), *(reinterpret_cast<float *>(pPreOutValue) + 1), *(reinterpret_cast<float *>(pPreOutValue) + 2));
					auto nextVal = glm::vec3(*reinterpret_cast<float *>(pNextOutValue), *(reinterpret_cast<float *>(pNextOutValue) + 1), *(reinterpret_cast<float *>(pNextOutValue) + 2));

					glm::mat4 trans(1);
					if (sampler.interpolation.compare("LINEAR") == 0)
						trans = glm::translate(glm::mat4(1), glm::mix(preVal, nextVal, glm::vec3(factor)));
					else if (sampler.interpolation.compare("STEP") == 0)
						trans = glm::translate(glm::mat4(1), preVal);
					else if (sampler.interpolation.compare("CUBICSPLINE") == 0)
						trans = glm::translate(glm::mat4(1), glm::smoothstep(preVal, nextVal, glm::vec3(factor)));
					nodeMatrices[channel.target_node]= trans * nodeMatrices[channel.target_node];
				}
				else if (channel.target_path.compare("rotation") == 0)
				{
					glm::quat preVal;
					glm::quat nextVal;
					switch (outputAccessor.componentType)
					{
					case TINYGLTF_PARAMETER_TYPE_FLOAT:
					{
						preVal = glm::quat(
							*(reinterpret_cast<float *>(pPreOutValue) + 3),
							*reinterpret_cast<float *>(pPreOutValue),
							*(reinterpret_cast<float *>(pPreOutValue) + 1),
							*(reinterpret_cast<float *>(pPreOutValue) + 2));
						nextVal = glm::quat(
							*(reinterpret_cast<float *>(pNextOutValue) + 3),
							*reinterpret_cast<float *>(pNextOutValue),
							*(reinterpret_cast<float *>(pNextOutValue) + 1),
							*(reinterpret_cast<float *>(pNextOutValue) + 2));
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_BYTE:
					{
						preVal = glm::quat(
							intToFloat(*(reinterpret_cast<signed char *>(pPreOutValue) + 3)),
							intToFloat(*reinterpret_cast<signed char *>(pPreOutValue)),
							intToFloat(*(reinterpret_cast<signed char *>(pPreOutValue) + 1)),
							intToFloat(*(reinterpret_cast<signed char *>(pPreOutValue) + 2)));
						nextVal = glm::quat(
							intToFloat(*(reinterpret_cast<signed char *>(pNextOutValue) + 3)),
							intToFloat(*reinterpret_cast<signed char *>(pNextOutValue)),
							intToFloat(*(reinterpret_cast<signed char *>(pNextOutValue) + 1)),
							intToFloat(*(reinterpret_cast<signed char *>(pNextOutValue) + 2)));
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
						preVal = glm::quat(
							intToFloat(*(reinterpret_cast<unsigned char *>(pPreOutValue) + 3)),
							intToFloat(*reinterpret_cast<unsigned char *>(pPreOutValue)),
							intToFloat(*(reinterpret_cast<unsigned char *>(pPreOutValue) + 1)),
							intToFloat(*(reinterpret_cast<unsigned char *>(pPreOutValue) + 2)));
						nextVal = glm::quat(
							intToFloat(*(reinterpret_cast<unsigned char *>(pNextOutValue) + 3)),
							intToFloat(*reinterpret_cast<unsigned char *>(pNextOutValue)),
							intToFloat(*(reinterpret_cast<unsigned char *>(pNextOutValue) + 1)),
							intToFloat(*(reinterpret_cast<unsigned char *>(pNextOutValue) + 2)));
						break;
					case TINYGLTF_PARAMETER_TYPE_SHORT:
						preVal = glm::quat(
							intToFloat(*(reinterpret_cast<short *>(pPreOutValue) + 3)),
							intToFloat(*reinterpret_cast<short *>(pPreOutValue)),
							intToFloat(*(reinterpret_cast<short *>(pPreOutValue) + 1)),
							intToFloat(*(reinterpret_cast<short *>(pPreOutValue) + 2)));
						nextVal = glm::quat(
							intToFloat(*(reinterpret_cast<short *>(pNextOutValue) + 3)),
							intToFloat(*reinterpret_cast<short *>(pNextOutValue)),
							intToFloat(*(reinterpret_cast<short *>(pNextOutValue) + 1)),
							intToFloat(*(reinterpret_cast<short *>(pNextOutValue) + 2)));
						break;
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
						preVal = glm::quat(
							intToFloat(*(reinterpret_cast<unsigned short *>(pPreOutValue) + 3)),
							intToFloat(*reinterpret_cast<unsigned short *>(pPreOutValue)),
							intToFloat(*(reinterpret_cast<unsigned short *>(pPreOutValue) + 1)),
							intToFloat(*(reinterpret_cast<unsigned short *>(pPreOutValue) + 2)));
						nextVal = glm::quat(
							intToFloat(*(reinterpret_cast<unsigned short *>(pNextOutValue) + 3)),
							intToFloat(*reinterpret_cast<unsigned short *>(pNextOutValue)),
							intToFloat(*(reinterpret_cast<unsigned short *>(pNextOutValue) + 1)),
							intToFloat(*(reinterpret_cast<unsigned short *>(pNextOutValue) + 2)));
						break;
					}
					glm::mat4 trans(1);
					glm::quat interpolation;
					if (sampler.interpolation.compare("LINEAR") == 0)
						interpolation = glm::normalize(glm::slerp(preVal, nextVal, factor));
					else if (sampler.interpolation.compare("STEP") == 0)
						interpolation = preVal;
					else if (sampler.interpolation.compare("CUBICSPLINE") == 0)
					{
						//TODO:
					}
					auto b = glm::mat4_cast(interpolation);
					nodeMatrices[channel.target_node]*= b;
				}
				else if (channel.target_path.compare("scale") == 0)
				{
					auto preVal = glm::vec3(*reinterpret_cast<float *>(pPreOutValue), *(reinterpret_cast<float *>(pPreOutValue) + 1), *(reinterpret_cast<float *>(pPreOutValue) + 2));
					auto nextVal = glm::vec3(*reinterpret_cast<float *>(pNextOutValue), *(reinterpret_cast<float *>(pNextOutValue) + 1), *(reinterpret_cast<float *>(pNextOutValue) + 2));

					glm::mat4 trans(1);
					if (sampler.interpolation.compare("LINEAR") == 0)
						trans = glm::scale(glm::mat4(1), glm::mix(preVal, nextVal, glm::vec3(factor)));
					else if (sampler.interpolation.compare("STEP") == 0)
						trans = glm::scale(glm::mat4(1), preVal);
					else if (sampler.interpolation.compare("CUBICSPLINE") == 0)
						trans = glm::scale(glm::mat4(1), glm::smoothstep(preVal, nextVal, glm::vec3(factor)));
					nodeMatrices[channel.target_node]*= trans;
				}
				else if (channel.target_path.compare("weights") == 0)
				{
					//TODO: morph targets
				}
				break;
			}
		}
	}
	UpdateNodeAnimation(0, nodeMatrices, edited, glm::mat4(1));

	auto nodeCount = mpModel->nodes.size();
	void *pData;
	mModelAssets[mpCurDevice].frameNodeAttributeBuffers[imageIndex]->MapBuffer(0, sizeof(NodeAttribute) * nodeCount, &pData);
	NodeAttribute *pNodeAttribute = reinterpret_cast<NodeAttribute *>(pData);
	for (auto &&e : nodeMatrices)
	{
		pNodeAttribute->matrix = e;
		pNodeAttribute++;
	}
	mModelAssets[mpCurDevice].frameNodeAttributeBuffers[imageIndex]->UnMapBuffer();
}
void Model::UpdateNodeAnimation(int nodeIndex, std::vector<glm::mat4> &nodeMatrices,const std::vector<bool>& edited,const glm::mat4 &preMatrix)
{
	if (!edited[nodeIndex])
		nodeMatrices[nodeIndex] = mDefaultNodeAttributes[nodeIndex].matrix;
	nodeMatrices[nodeIndex] = preMatrix * nodeMatrices[nodeIndex];
	for (auto &&subNodeIndex : mpModel->nodes[nodeIndex].children)
		UpdateNodeAnimation(subNodeIndex, nodeMatrices, edited, nodeMatrices[nodeIndex]);
}