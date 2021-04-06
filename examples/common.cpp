/**
 * @file common.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "common.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_RAPIDJSON
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include <tiny_gltf.h>

Shit::RendererVersion rendererVersion{Shit::RendererVersion::GL};

std::string readFile(const char *filename)
{
	std::fstream file(filename, std::ios::in | std::ios::ate | std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("failed to open file");
	size_t size0 = static_cast<size_t>(file.tellg());
	std::string ret;
	ret.resize(size0);
	file.seekg(0);
	file.read(const_cast<char *>(ret.data()), size0);
	file.close();
	return ret;
}
void *loadImage(const char *imagePath, int &width, int &height, int &components, int request_components)
{
	auto ret = stbi_load(imagePath, &width, &height, &components, request_components); //force load an alpha channel,even not exist
	if (!ret)
		THROW("failed to load texture image!")
	return ret;
}
void freeImage(void *pData)
{
	stbi_image_free(pData);
}

std::string buildShaderPath(const char *shaderName, RendererVersion renderVersion)
{
	renderVersion &= RendererVersion::TypeBitmask;
	std::string subdir;
	switch (renderVersion)
	{
	case RendererVersion::GL:
		subdir = "GL";
		break;
	case RendererVersion::VULKAN:
		subdir = "Vulkan";
		break;
	case RendererVersion::D3D11:
		break;
	case RendererVersion::D3D12:
		break;
	case RendererVersion::METAL:
		break;
	default:
		break;
	}
	return SHADER_PATH + subdir + "/" + shaderName;
}
WindowPixelFormat chooseSwapchainFormat(const std::vector<WindowPixelFormat> &candidates, Device *pDevice, ShitWindow *pWindow)
{
	std::vector<WindowPixelFormat> supportedFormats;
	pDevice->GetWindowPixelFormats(pWindow, supportedFormats);
	LOG("supported formats:");
	for (auto &&e : supportedFormats)
	{
		LOG_VAR(static_cast<int>(e.format));
		LOG_VAR(static_cast<int>(e.colorSpace));
		for (auto &&a : candidates)
		{
			if (e.format == a.format && e.colorSpace == a.colorSpace)
				return e;
		}
	}
	return supportedFormats[0];
}
PresentMode choosePresentMode(const std::vector<PresentMode> &candidates, Device *pDevice, ShitWindow *window)
{
	std::vector<PresentMode> modes;
	pDevice->GetPresentModes(window, modes);
	for (auto &&e : modes)
	{
		LOG_VAR(static_cast<int>(e));
		for (auto &&a : candidates)
		{
			if (static_cast<int>(e) == static_cast<int>(a))
				return e;
		}
	}
	return modes[0];
}

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
//==============================================================================================
Model::Model(const char *filePath)
{
	mModelPath = filePath;
	mpModel = std::make_unique<tinygltf::Model>();
	if (!loadModel(*mpModel, filePath))
		return;
	mLoadSucceed = true;

	//init vertexInputStateInfo
	mVertexInputStateCreateInfo.vertexAttributeDescriptions = {
		{LOCATION_POSITION, LOCATION_POSITION, 3, DataType::FLOAT, false, 0},
		{LOCATION_NORMAL, LOCATION_NORMAL, 3, DataType::FLOAT, false, 0},
		{LOCATION_TANGENT, LOCATION_TANGENT, 4, DataType::FLOAT, false, 0},
		{LOCATION_TEXCOORD0, LOCATION_TEXCOORD0, 2, DataType::FLOAT, false, 0},
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
			{LOCATION_INSTANCE_COLOR_FACTOR, sizeof(InstanceAttribute), 1},
		};
	for (auto &&attrib : mpModel->meshes[0].primitives[0].attributes)
	{
		int location = -1;
		if (attrib.first.compare("POSITION") == 0)
		{
			location = LOCATION_POSITION;
		}
		else if (attrib.first.compare("NORMAL") == 0)
		{
			location = LOCATION_NORMAL;
		}
		else if (attrib.first.compare("TANGENT") == 0)
		{
			location = LOCATION_TANGENT;
		}
		else if (attrib.first.compare("TEXCOORD_0") == 0)
		{
			location = LOCATION_TEXCOORD0;
		}
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
		e.first->Destroy(e.second.instanceAttributeBuffer);
		for (auto &&a : e.second.primitivesDrawIndirectInfo)
			e.first->Destroy(a.pBuffer);
		for (auto &&buffer : e.second.sceneBuffers)
			e.first->Destroy(buffer);
		for (auto &&a : e.second.images)
			e.first->Destroy(a);
		for (auto &&a : e.second.imageViews)
			e.first->Destroy(a);
		for (auto &&a : e.second.samplers)
			e.first->Destroy(a);
		e.first->Destroy(e.second.materialBuffer);
		e.first->Destroy(e.second.nodeAttributeBuffer);
		e.first->Destroy(e.second.descriptorPool);
	}
}
void Model::FreeModel(Device *pDevice)
{
	for (auto &&e : mModelAssets[pDevice].primitivesDrawIndirectInfo)
		pDevice->Destroy(e.pBuffer);
	for (auto &&buffer : mModelAssets[pDevice].sceneBuffers)
		pDevice->Destroy(buffer);
	for (auto &&e : mModelAssets[pDevice].images)
		pDevice->Destroy(e);
	for (auto &&e : mModelAssets[pDevice].imageViews)
		pDevice->Destroy(e);
	for (auto &&e : mModelAssets[pDevice].samplers)
		pDevice->Destroy(e);
	pDevice->Destroy(mModelAssets[pDevice].instanceAttributeBuffer);

	pDevice->Destroy(mModelAssets[pDevice].materialBuffer);
	pDevice->Destroy(mModelAssets[pDevice].nodeAttributeBuffer);

	pDevice->Destroy(mModelAssets[pDevice].descriptorPool);

	mModelAssets.erase(pDevice);
}
size_t Model::GetSceneCount() const
{
	return mpModel->scenes.size();
}
void Model::DownloadModel(Device *pDevice, PipelineLayout *pipelineLayout, const std::vector<InstanceAttribute> &instanceAttributes)
{
	mpCurPipelineLayout = pipelineLayout;

	//scene buffers
	for (auto &&buffer : mpModel->buffers)
	{
		BufferCreateInfo createInfo{
			.size = buffer.data.size(),
			.usage = BufferUsageFlagBits::INDEX_BUFFER_BIT | BufferUsageFlagBits::UNIFORM_BUFFER_BIT | BufferUsageFlagBits::VERTEX_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			.memoryPropertyFlags = MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		mModelAssets[pDevice].sceneBuffers.emplace_back(pDevice->Create(createInfo, buffer.data.data()));
	}

	//primitive draw indexed indiect command info
	mModelAssets[pDevice].primitivesDrawIndirectInfo.resize(mpModel->accessors.size(), {});
	for (auto &&mesh : mpModel->meshes)
	{
		for (auto &&primitive : mesh.primitives)
		{
			if (primitive.indices == -1)
			{
				int positionAccessorIndex{};
				for(auto&& e:primitive.attributes)
				{
					if (e.first.compare("POSITION") == 0)
					{
						positionAccessorIndex = e.second;
						break;
					}
				}
				auto &&positionAccessor = mpModel->accessors[positionAccessorIndex];
				DrawIndirectCommand cmd{
					positionAccessor.count,
					(std::max)(static_cast<uint32_t>(instanceAttributes.size()), 1u),
				};
				BufferCreateInfo createInfo{
					{},
					sizeof(DrawIndirectCommand),
					BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
					MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
				mModelAssets[pDevice].primitivesDrawIndirectInfo[positionAccessorIndex] = {
					pDevice->Create(createInfo, &cmd),
					0,
					1,
					sizeof(DrawIndirectInfo)};
			}
			else
			{
				auto &&indexAccessor = mpModel->accessors[primitive.indices];
				DrawIndexedIndirectCommand cmd{
					indexAccessor.count,
					(std::max)(instanceAttributes.size(), size_t(1)),
				};
				BufferCreateInfo createInfo{
					{},
					sizeof(DrawIndexedIndirectCommand),
					BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
					MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
				mModelAssets[pDevice].primitivesDrawIndirectInfo[primitive.indices] = {
					pDevice->Create(createInfo, &cmd),
					0,
					1,
					sizeof(DrawIndirectInfo)};
			}
		}
	}

	//create Instance Attribute buffer
	BufferCreateInfo instanceAttributeCreateInfo{
		{},
		(std::max)(instanceAttributes.size(), size_t(1)) * sizeof(InstanceAttribute),
		BufferUsageFlagBits::VERTEX_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
		MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
	if (instanceAttributes.size() == 0)
	{
		InstanceAttribute attribute{glm::vec4{1}, glm::mat4{1}};
		mModelAssets[pDevice].instanceAttributeBuffer = pDevice->Create(instanceAttributeCreateInfo, &attribute);
	}
	else
	{
		mModelAssets[pDevice].instanceAttributeBuffer = pDevice->Create(instanceAttributeCreateInfo, reinterpret_cast<const void *>(instanceAttributes.data()));
	}

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
		mModelAssets[pDevice].images.emplace_back(pDevice->Create(imageCreateInfo, pixels));

		//imageview
		imageViewCreateInfo.pImage = mModelAssets[pDevice].images.back();
		imageViewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;
		mModelAssets[pDevice].imageViews.emplace_back(pDevice->Create(imageViewCreateInfo));
		freeImage(pixels);
	}

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
		mModelAssets[pDevice].samplers.emplace_back(pDevice->Create(samplerCreateInfo));
	}
	if (mModelAssets[pDevice].samplers.empty())
		mModelAssets[pDevice].samplers.emplace_back(pDevice->Create(samplerCreateInfo));

	//node attribute buffer
	auto nodeCount = mpModel->nodes.size();
	std::vector<NodeAttribute> nodeAttributes;
	nodeAttributes.resize(nodeCount);
	for (auto &&scene : mpModel->scenes)
	{
		for (auto &&nodeIndex : scene.nodes)
		{
			LoadNode(nodeIndex, nodeAttributes, glm::dmat4(1));
		}
	}
	mModelAssets[pDevice].nodeAttributeBuffer = pDevice->Create(BufferCreateInfo{
																	{},
																	static_cast<uint32_t>(sizeof(NodeAttribute) * nodeAttributes.size()),
																	BufferUsageFlagBits::UNIFORM_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
																	MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
																reinterpret_cast<const void *>(nodeAttributes.data()));
	//material buffers
	auto materialCount = mpModel->materials.size();
	BufferCreateInfo bufferCreateInfo{
		{},
		sizeof(Material) * materialCount,
		BufferUsageFlagBits::UNIFORM_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
		MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
	std::vector<Material> materials(materialCount);
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
	mModelAssets[pDevice].materialBuffer = pDevice->Create(bufferCreateInfo, &materials[0]);

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

	DescriptorPoolCreateInfo descriptorPoolCreateInfo{materialCount + nodeCount, poolSizes};
	mModelAssets[pDevice].descriptorPool = pDevice->Create(descriptorPoolCreateInfo);

	std::vector<DescriptorSetLayout *> materialSetLayouts(materialCount, descriptorSetLayouts[DESCRIPTORSET_ID_MATERIAL]);
	DescriptorSetAllocateInfo allocInfo{materialSetLayouts};
	mModelAssets[pDevice].descriptorPool->Allocate(allocInfo, mModelAssets[pDevice].materialDescriptorSets);

	std::vector<DescriptorSetLayout *> nodeSetLayouts(nodeCount, descriptorSetLayouts[DESCRIPTORSET_ID_NODE]);
	allocInfo = DescriptorSetAllocateInfo{nodeSetLayouts};
	mModelAssets[pDevice].descriptorPool->Allocate(allocInfo, mModelAssets[pDevice].nodeDescriptorSets);

	//descriptorsets
	std::vector<WriteDescriptorSet> writes;
	for (size_t i = 0; i < nodeCount; ++i)
	{
		writes.emplace_back(
			WriteDescriptorSet{
				mModelAssets[pDevice].nodeDescriptorSets[i],
				UNIFORM_BINDING_NODE,
				0,
				DescriptorType::UNIFORM_BUFFER,
				std::vector<DescriptorBufferInfo>{{mModelAssets[pDevice].nodeAttributeBuffer,
												   sizeof(NodeAttribute) * i,
												   sizeof(NodeAttribute)}}});
	}

	for (size_t i = 0; i < materialCount; ++i)
	{
		auto &&material = mpModel->materials[i];

		if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
		{
			int sampler = (std::max)(mpModel->textures[material.pbrMetallicRoughness.baseColorTexture.index].sampler, 0);
			writes.emplace_back(
				WriteDescriptorSet{
					mModelAssets[pDevice].materialDescriptorSets[i],
					TEXTURE_BINDING_ALBEDO,
					0,
					DescriptorType::COMBINED_IMAGE_SAMPLER,
					std::vector<DescriptorImageInfo>{{mModelAssets[pDevice].samplers[sampler], //sampler
													  mModelAssets[pDevice].imageViews[material.pbrMetallicRoughness.baseColorTexture.index],
													  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
		}
		if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
		{
			int sampler = (std::max)(mpModel->textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].sampler, 0);
			writes.emplace_back(
				WriteDescriptorSet{
					mModelAssets[pDevice].materialDescriptorSets[i],
					TEXTURE_BINDING_METALLIC_ROUGHNESS,
					0,
					DescriptorType::COMBINED_IMAGE_SAMPLER,
					std::vector<DescriptorImageInfo>{{mModelAssets[pDevice].samplers[sampler], //sampler
													  mModelAssets[pDevice].imageViews[material.pbrMetallicRoughness.metallicRoughnessTexture.index],
													  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
		}
		if (material.normalTexture.index != -1)
		{
			int sampler = (std::max)(mpModel->textures[material.normalTexture.index].sampler, 0);
			writes.emplace_back(
				WriteDescriptorSet{
					mModelAssets[pDevice].materialDescriptorSets[i],
					TEXTURE_BINDING_NORMAL,
					0,
					DescriptorType::COMBINED_IMAGE_SAMPLER,
					std::vector<DescriptorImageInfo>{{mModelAssets[pDevice].samplers[sampler], //sampler
													  mModelAssets[pDevice].imageViews[material.normalTexture.index],
													  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
		}
		if (material.occlusionTexture.index != -1)
		{
			int sampler = (std::max)(mpModel->textures[material.occlusionTexture.index].sampler, 0);
			writes.emplace_back(
				WriteDescriptorSet{
					mModelAssets[pDevice].materialDescriptorSets[i],
					TEXTURE_BINDING_OCCLUSION,
					0,
					DescriptorType::COMBINED_IMAGE_SAMPLER,
					std::vector<DescriptorImageInfo>{{mModelAssets[pDevice].samplers[sampler], //sampler
													  mModelAssets[pDevice].imageViews[material.occlusionTexture.index],
													  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
		}
		if (material.emissiveTexture.index != -1)
		{
			int sampler = (std::max)(mpModel->textures[material.emissiveTexture.index].sampler, 0);
			writes.emplace_back(
				WriteDescriptorSet{
					mModelAssets[pDevice].materialDescriptorSets[i],
					TEXTURE_BINDING_EMISSION,
					0,
					DescriptorType::COMBINED_IMAGE_SAMPLER,
					std::vector<DescriptorImageInfo>{{mModelAssets[pDevice].samplers[sampler], //sampler
													  mModelAssets[pDevice].imageViews[material.emissiveTexture.index],
													  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
		}

		//material descriptor update
		writes.emplace_back(
			WriteDescriptorSet{
				mModelAssets[pDevice].materialDescriptorSets[i],
				UNIFORM_BINDING_MATERIAL,
				0,
				DescriptorType::UNIFORM_BUFFER,
				std::vector<DescriptorBufferInfo>{{mModelAssets[pDevice].materialBuffer,
												   sizeof(Material) * i,
												   sizeof(Material)}}});
	}
	pDevice->UpdateDescriptorSets(writes, {});
}
void Model::DrawModel(
	Device *pDevice,
	CommandBuffer *pCommandBuffer,
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

	LOG_VAR(mpModel->scenes[sceneIndex].name)
	for (auto &&nodeIndex : mpModel->scenes[sceneIndex].nodes)
	{
		mpCurCommandBuffer->BindDescriptorSets(
			BindDescriptorSetsInfo{
				PipelineBindPoint::GRAPHICS,
				mpCurPipelineLayout,
				DESCRIPTORSET_ID_NODE,
				1,
				&mModelAssets[mpCurDevice].nodeDescriptorSets[nodeIndex]});
		DrawNode(mpModel->nodes[nodeIndex]);
	}
}
void Model::DrawNode(const tinygltf::Node &node)
{
	if ((node.mesh >= 0) && (node.mesh < mpModel->meshes.size()))
	{
		DrawMesh(mpModel->meshes[node.mesh]);
	}
	for (size_t i = 0; i < node.children.size(); i++)
	{
		assert((node.children[i] >= 0) && (node.children[i] < mpModel->nodes.size()));
		mpCurCommandBuffer->BindDescriptorSets(
			BindDescriptorSetsInfo{
				PipelineBindPoint::GRAPHICS,
				mpCurPipelineLayout,
				DESCRIPTORSET_ID_NODE,
				1,
				&mModelAssets[mpCurDevice].nodeDescriptorSets[node.children[i]]});
		DrawNode(mpModel->nodes[node.children[i]]);
	}
}
void Model::DrawMesh(const tinygltf::Mesh &mesh)
{
	BindVertexBufferInfo bindVertexBufferInfo{0, VERTEX_LOCATION_NUM_MAX};
	for (auto &&primitive : mesh.primitives)
	{
		//
		mpCurCommandBuffer->BindDescriptorSets(
			BindDescriptorSetsInfo{
				PipelineBindPoint::GRAPHICS,
				mpCurPipelineLayout,
				DESCRIPTORSET_ID_MATERIAL,
				1,
				&mModelAssets[mpCurDevice].materialDescriptorSets[primitive.material]});

		std::array<Buffer *, VERTEX_LOCATION_NUM_MAX> buffers;
		buffers.fill(mModelAssets[mpCurDevice].sceneBuffers[mCurSceneIndex]);
		buffers[LOCATION_INSTANCE_COLOR_FACTOR] = mModelAssets[mpCurDevice].instanceAttributeBuffer;
		std::array<uint64_t, VERTEX_LOCATION_NUM_MAX> offsets{};
		for (auto &&attrib : primitive.attributes)
		{
			auto &&accessor = mpModel->accessors[attrib.second];
			if (attrib.first.compare("POSITION") == 0)
			{
				offsets[LOCATION_POSITION] = mpModel->bufferViews[accessor.bufferView].byteOffset + accessor.byteOffset;
			}
			else if (attrib.first.compare("NORMAL") == 0)
			{
				offsets[LOCATION_NORMAL] = mpModel->bufferViews[accessor.bufferView].byteOffset + accessor.byteOffset;
			}
			else if (attrib.first.compare("TANGENT") == 0)
			{
				offsets[LOCATION_TANGENT] = mpModel->bufferViews[accessor.bufferView].byteOffset + accessor.byteOffset;
			}
			else if (attrib.first.compare("TEXCOORD_0") == 0)
			{
				offsets[LOCATION_TEXCOORD0] = mpModel->bufferViews[accessor.bufferView].byteOffset + accessor.byteOffset;
			}
		}
		bindVertexBufferInfo.ppBuffers = buffers.data();
		bindVertexBufferInfo.pOffsets = offsets.data();

		mpCurCommandBuffer->BindVertexBuffer(bindVertexBufferInfo);

		if (primitive.indices==-1)
		{
			int positionAccessorIndex{};
			for (auto &&e : primitive.attributes)
			{
				if (e.first.compare("POSITION") == 0)
				{
					positionAccessorIndex = e.second;
					break;
				}
			}
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
				mModelAssets[mpCurDevice].sceneBuffers[bufferView.buffer],
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