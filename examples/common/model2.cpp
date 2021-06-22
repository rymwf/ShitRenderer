#include "model2.hpp"
#include "appBase.hpp"

static const unsigned char COLOR_WHITE[4]{255, 255, 255, 255};
static const unsigned char COLOR_BLACK[4]{0, 0, 0, 1};

bool loadModelGLTF(tinygltf::Model &model, const char *filename)
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

Model2::Model2(ModelType modelType) : _modelType(modelType)
{
	_name = "new model";
	static const char *filePath;
	switch (modelType)
	{
	case ModelType::SPHERE:
	default:
		filePath = ASSET_PATH "models/geosphere.gltf";
		break;
	case ModelType::CUBE:
		filePath = ASSET_PATH "models/cube.gltf";
		break;
	case ModelType::QUAD:
		LOG("model quad not implemented yet");
		return;
	case ModelType::ROCK:
		filePath = ASSET_PATH "models/rock.gltf";
		break;
	case ModelType::TEAPOT:
		filePath = ASSET_PATH "models/teapot.gltf";
		break;
	case ModelType::CUBE_TEXTURE:
		filePath = ASSET_PATH "models/textured_unit_cube.gltf";
		break;
	case ModelType::TORUSKNOT:
		filePath = ASSET_PATH "models/torusknot.gltf";
		break;
	case ModelType::TUNNEL_CYLINDER:
		filePath = ASSET_PATH "models/tunnel_cylinder.gltf";
		break;
	case ModelType::SKYBOX:
	case ModelType::AXIS:
		return;
	}
	_modelPath = filePath;
	loadModel(filePath);
}
Model2::Model2(const char *filePath) : _modelPath(filePath)
{
	_name = "new model";
	loadModel(filePath);
}
Model2::~Model2()
{
}
uint32_t Model2::addInstance(const InstanceAttribute& instanceAttribute)
{
	auto cap = _instanceAttributes.capacity();
	_instanceIds.emplace_back(_instanceAttributes.empty() ? 0 : _instanceIds.back() + 1);
	_instanceAttributes.emplace_back(instanceAttribute);
	_frameInstanceAttributeBuffers.resize(g_App->getImageCount());

	for (auto &&e : _frameInstanceAttributeBuffers)
	{
		if (cap < _instanceAttributes.capacity())
		{
			g_App->getDevice()->Destroy(e);
			e = g_App->getDevice()->Create(
				BufferCreateInfo{
					{},
					_instanceAttributes.capacity() * sizeof(InstanceAttribute),
					BufferUsageFlagBits::VERTEX_BUFFER_BIT,
					MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
				_instanceAttributes.data());
		}
		else
		{
			void *data;
			auto size = sizeof(InstanceAttribute) * _instanceAttributes.size();
			e->MapMemory(0, size, &data);
			memcpy(&data, _instanceAttributes.data(), size);
			e->UnMapMemory();
		}
	}
	//createDrawCommandInfo();
	return _instanceIds.back();
}
void Model2::addInstances(const std::vector<InstanceAttribute> &instanceAttributes,std::vector<uint32_t>& instanceIds)
{
	auto cap=_instanceAttributes.capacity();
	auto instanceCount=_instanceAttributes.size();

	_instanceIds.resize(_instanceIds.size() + instanceAttributes.size());
	std::iota(std::next(_instanceIds.begin(), instanceCount), _instanceIds.end(), instanceCount ? _instanceIds.back() + 1 : 0);
	instanceIds.resize(instanceAttributes.size());
	memcpy(instanceIds.data(),&_instanceIds[instanceCount],sizeof(uint32_t)*instanceAttributes.size());

	_instanceAttributes.insert(_instanceAttributes.end(), instanceAttributes.begin(), instanceAttributes.end());

	for (auto &&e : _frameInstanceAttributeBuffers)
	{
		if (cap < _instanceAttributes.capacity())
		{
			g_App->getDevice()->Destroy(e);
			e = g_App->getDevice()->Create(
				BufferCreateInfo{
					{},
					_instanceAttributes.capacity() * sizeof(InstanceAttribute),
					BufferUsageFlagBits::VERTEX_BUFFER_BIT,
					MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
				_instanceAttributes.data());
		}
		else
		{
			void *data;
			auto size = sizeof(InstanceAttribute) * _instanceAttributes.size();
			e->MapMemory(0, size, &data);
			memcpy(&data, _instanceAttributes.data(), size);
			e->UnMapMemory();
		}
	}
}
void Model2::removeInstance(int id)
{
	auto it = std::lower_bound(_instanceIds.begin(), _instanceIds.end(), id);
	if (*it == id)
	{
		_instanceIds.erase(it);
		auto pos = std::distance(_instanceIds.begin(), it);
		_instanceAttributes.erase(std::next(_instanceAttributes.begin(), pos));

		for (auto &&e : _frameInstanceAttributeBuffers)
		{
			void *data;
			auto size = sizeof(InstanceAttribute) * _instanceAttributes.size();
			e->MapMemory(0, size, &data);
			memcpy(&data, _instanceAttributes.data(), size);
			e->UnMapMemory();
		}
	}
}

void Model2::updateInstanceAttributes(uint32_t imageIndex, uint32_t id, const InstanceAttribute &instanceAttribute)
{
	auto it = std::lower_bound(_instanceIds.begin(), _instanceIds.end(), id);
	if (*it == id)
	{
		auto pos = std::distance(_instanceIds.begin(), it);
		_instanceAttributes[pos] = instanceAttribute;

		void *data;
		auto size = sizeof(InstanceAttribute);
		_frameInstanceAttributeBuffers[imageIndex]->MapMemory(sizeof(InstanceAttribute) * pos, size, &data);
		memcpy(data, &instanceAttribute, size);
		_frameInstanceAttributeBuffers[imageIndex]->UnMapMemory();
	}
}
InstanceAttribute Model2::getInstanceAttribute(uint32_t index)
{
	return _instanceAttributes[index];
}
bool Model2::loadModel(const char *filePath)
{
	if (!loadModelGLTF(_model, filePath))
	{
		LOG(std::string("failed to load model:") + filePath)
		return false;
	}
	createDescriptorSets();
	createSamplers();
	loadMeshes();
	loadImages();
	loadSkins();
	loadMaterials();
	loadNodes();
	return true;
}
void Model2::loadMeshes()
{
	std::vector<Vertex> vertexBuffer;
	std::vector<uint32_t> indexBuffer;

	_meshViews.reserve(_model.meshes.size());
	std::vector<PrimitiveView> primitiveViews;

	//load meshes
	for (auto &&mesh : _model.meshes)
	{
		primitiveViews.clear();
		for (auto &&primitive : mesh.primitives)
		{
			tinygltf::Accessor accessor;
			tinygltf::BufferView bufferView;
			tinygltf::Buffer buffer;
			//================================
			primitiveViews.emplace_back(
				PrimitiveView{
					primitive.material,
					DrawIndirectCommand{
						(uint32_t)accessor.count,
						1u,
						(uint32_t)vertexBuffer.size(),
						0u,
					}});
			//===================
			auto vertexOffset = vertexBuffer.size();
			//load vertices
			auto it = primitive.attributes.find("POSITION");
			if (it != primitive.attributes.end())
			{
				accessor = _model.accessors[it->second];
				bufferView = _model.bufferViews[accessor.bufferView];
				buffer = _model.buffers[bufferView.buffer];
				vertexBuffer.resize(vertexOffset + accessor.count);
				float *p = reinterpret_cast<float *>(buffer.data.data() + accessor.byteOffset + bufferView.byteOffset);
				for (size_t i = 0; i < accessor.count; ++i)
				{
					vertexBuffer[vertexOffset + i].position[0] = *p++;
					vertexBuffer[vertexOffset + i].position[1] = *p++;
					vertexBuffer[vertexOffset + i].position[2] = *p++;
				}

			}
			it = primitive.attributes.find("NORMAL");
			if (it != primitive.attributes.end())
			{
				accessor = _model.accessors[it->second];
				bufferView = _model.bufferViews[accessor.bufferView];
				buffer = _model.buffers[bufferView.buffer];
				float *p = reinterpret_cast<float *>(buffer.data.data() + accessor.byteOffset + bufferView.byteOffset);
				for (size_t i = 0; i < accessor.count; ++i)
				{
					vertexBuffer[vertexOffset + i].normal[0] = *p++;
					vertexBuffer[vertexOffset + i].normal[1] = *p++;
					vertexBuffer[vertexOffset + i].normal[2] = *p++;
				}
			}
			it = primitive.attributes.find("TANGENT");
			if (it != primitive.attributes.end())
			{
				accessor = _model.accessors[it->second];
				bufferView = _model.bufferViews[accessor.bufferView];
				buffer = _model.buffers[bufferView.buffer];
				float *p = reinterpret_cast<float *>(buffer.data.data() + accessor.byteOffset + bufferView.byteOffset);
				for (size_t i = 0; i < accessor.count; ++i)
				{
					vertexBuffer[vertexOffset + i].tangent[0] = *p++;
					vertexBuffer[vertexOffset + i].tangent[1] = *p++;
					vertexBuffer[vertexOffset + i].tangent[2] = *p++;
				}
			}
			it = primitive.attributes.find("TEXCOORD_0");
			if (it != primitive.attributes.end())
			{
				accessor = _model.accessors[it->second];
				bufferView = _model.bufferViews[accessor.bufferView];
				buffer = _model.buffers[bufferView.buffer];
				auto p = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;
				switch (accessor.componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_FLOAT:
				{
					float *p2 = reinterpret_cast<float *>(p);
					for (size_t i = 0; i < accessor.count; ++i)
					{
						vertexBuffer[vertexOffset + i].texCoord[0] = *p2++;
						vertexBuffer[vertexOffset + i].texCoord[1] = *p2++;
					}
				}
				break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					for (size_t i = 0; i < accessor.count; ++i)
					{
						vertexBuffer[vertexOffset + i].texCoord[0] = intToFloat(*p++);
						vertexBuffer[vertexOffset + i].texCoord[1] = intToFloat(*p++);
					}
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				{
					uint16_t *p2 = reinterpret_cast<uint16_t *>(p);
					for (size_t i = 0; i < accessor.count; ++i)
					{
						vertexBuffer[vertexOffset + i].texCoord[0] = intToFloat(*p2++);
						vertexBuffer[vertexOffset + i].texCoord[1] = intToFloat(*p2++);
					}
				}
				break;
				}
			}
			it = primitive.attributes.find("CORLOR_0");
			if (it != primitive.attributes.end())
			{
				accessor = _model.accessors[it->second];
				bufferView = _model.bufferViews[accessor.bufferView];
				buffer = _model.buffers[bufferView.buffer];
				auto p = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

				switch (accessor.componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_FLOAT:
				{
					float *p2 = reinterpret_cast<float *>(p);
					for (size_t i = 0; i < accessor.count; ++i)
					{
						vertexBuffer[vertexOffset + i].color[0] = *p2++;
						vertexBuffer[vertexOffset + i].color[1] = *p2++;
						vertexBuffer[vertexOffset + i].color[2] = *p2++;
						vertexBuffer[vertexOffset + i].color[3] = *p2++;
					}
				}
				break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				{
					bool isVec4 = accessor.type == TINYGLTF_TYPE_VEC4;
					for (size_t i = 0; i < accessor.count; ++i)
					{
						vertexBuffer[vertexOffset + i].color[0] = intToFloat(*p++);
						vertexBuffer[vertexOffset + i].color[1] = intToFloat(*p++);
						vertexBuffer[vertexOffset + i].color[2] = intToFloat(*p++);
						if (isVec4)
							vertexBuffer[vertexOffset + i].color[3] = intToFloat(*p++);
					}
				}
				break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				{
					uint16_t *p2 = reinterpret_cast<uint16_t *>(p);
					for (size_t i = 0; i < accessor.count; ++i)
					{
						vertexBuffer[vertexOffset + i].color[0] = intToFloat(*p2++);
						vertexBuffer[vertexOffset + i].color[1] = intToFloat(*p2++);
						vertexBuffer[vertexOffset + i].color[2] = intToFloat(*p2++);
						vertexBuffer[vertexOffset + i].color[3] = intToFloat(*p2++);
					}
				}
				break;
				}
			}
			it = primitive.attributes.find("JOINTS_0");
			if (it != primitive.attributes.end())
			{
				accessor = _model.accessors[it->second];
				bufferView = _model.bufferViews[accessor.bufferView];
				buffer = _model.buffers[bufferView.buffer];
				auto p = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

				switch (accessor.componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				{
					for (size_t i = 0; i < accessor.count; ++i)
					{
						vertexBuffer[vertexOffset + i].joints[0] = (*p++);
						vertexBuffer[vertexOffset + i].joints[1] = (*p++);
						vertexBuffer[vertexOffset + i].joints[2] = (*p++);
						vertexBuffer[vertexOffset + i].joints[3] = (*p++);
					}
				}
				break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				{
					uint16_t *p2 = reinterpret_cast<uint16_t *>(p);
					for (size_t i = 0; i < accessor.count; ++i)
					{
						vertexBuffer[vertexOffset + i].joints[0] = (*p2++);
						vertexBuffer[vertexOffset + i].joints[1] = (*p2++);
						vertexBuffer[vertexOffset + i].joints[2] = (*p2++);
						vertexBuffer[vertexOffset + i].joints[3] = (*p2++);
					}
				}
				break;
				}
			}
			it = primitive.attributes.find("WEIGHTS_0");
			if (it != primitive.attributes.end())
			{
				accessor = _model.accessors[it->second];
				bufferView = _model.bufferViews[accessor.bufferView];
				buffer = _model.buffers[bufferView.buffer];
				auto p = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

				switch (accessor.componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_FLOAT:
				{
					float *p2 = reinterpret_cast<float *>(p);
					for (size_t i = 0; i < accessor.count; ++i)
					{
						vertexBuffer[vertexOffset + i].weights[0] = *p2++;
						vertexBuffer[vertexOffset + i].weights[1] = *p2++;
						vertexBuffer[vertexOffset + i].weights[2] = *p2++;
						vertexBuffer[vertexOffset + i].weights[3] = *p2++;
					}
				}
				break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				{
					for (size_t i = 0; i < accessor.count; ++i)
					{
						vertexBuffer[vertexOffset + i].weights[0] = intToFloat(*p++);
						vertexBuffer[vertexOffset + i].weights[1] = intToFloat(*p++);
						vertexBuffer[vertexOffset + i].weights[2] = intToFloat(*p++);
						vertexBuffer[vertexOffset + i].weights[3] = intToFloat(*p++);
					}
				}
				break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				{
					uint16_t *p2 = reinterpret_cast<uint16_t *>(p);
					for (size_t i = 0; i < accessor.count; ++i)
					{
						vertexBuffer[vertexOffset + i].weights[0] = intToFloat(*p2++);
						vertexBuffer[vertexOffset + i].weights[1] = intToFloat(*p2++);
						vertexBuffer[vertexOffset + i].weights[2] = intToFloat(*p2++);
						vertexBuffer[vertexOffset + i].weights[3] = intToFloat(*p2++);
					}
				}
				break;
				}
			}

			//load indices
			if (primitive.indices >= 0)
			{
				accessor = _model.accessors[primitive.indices];
				bufferView = _model.bufferViews[accessor.bufferView];
				buffer = _model.buffers[bufferView.buffer];

				//primitiveViews.back().indexBufferOffset = indexBuffer.size() * sizeof(uint32_t);
				//primitiveViews.back().firstIndex = indexBuffer.size();
				primitiveViews.back() =
					PrimitiveView{
						primitive.material,
						DrawIndexedIndirectCommand{
							static_cast<uint32_t>(accessor.count),
							1u,
							static_cast<uint32_t>(indexBuffer.size()),
							static_cast<int32_t>(std::get<DrawIndirectCommand>(primitiveViews.back().drawCommand).firstVertex * sizeof(Vertex)),
							0u}};

				unsigned char *p = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;
				switch (accessor.componentType)
				{
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
					for (size_t i = 0; i < accessor.count; ++i)
						indexBuffer.emplace_back(*p++);
					break;
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
				{
					uint16_t *p2 = reinterpret_cast<uint16_t *>(p);
					for (size_t i = 0; i < accessor.count; ++i, ++p2)
						indexBuffer.emplace_back(*p2);
				}
				break;
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
				{
					uint32_t *p2 = reinterpret_cast<uint32_t *>(p);
					for (size_t i = 0; i < accessor.count; ++i, ++p2)
						indexBuffer.emplace_back(*p2);
				}
				break;
				default:
					LOG("wrong index type");
					break;
				}
			}
		}
		_meshViews.emplace_back(primitiveViews);
	}

	_indexBuffer = g_App->getDevice()->Create(BufferCreateInfo{
												  {},
												  indexBuffer.size() * sizeof(uint32_t),
												  BufferUsageFlagBits::INDEX_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
												  MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
											  indexBuffer.data());
	_vertexBuffer = g_App->getDevice()->Create(BufferCreateInfo{
												   {},
												   vertexBuffer.size() * sizeof(Vertex),
												   BufferUsageFlagBits::VERTEX_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
												   MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
											   vertexBuffer.data());
}
void Model2::createDescriptorSets()
{
	//set id 1
	static std::vector<DescriptorSetLayoutBinding> nodeBindings{
		DescriptorSetLayoutBinding{UNIFORM_BINDING_NODE, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT}, //UBO M
	};
	//set id 2
	//create texture descriptorSets
	static std::vector<DescriptorSetLayoutBinding> materialBindings{
		DescriptorSetLayoutBinding{TEXTURE_BINDING_ALBEDO, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT | ShaderStageFlagBits::VERTEX_BIT},				//albedo
		DescriptorSetLayoutBinding{TEXTURE_BINDING_NORMAL, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT | ShaderStageFlagBits::VERTEX_BIT},				//normal
		DescriptorSetLayoutBinding{TEXTURE_BINDING_METALLIC_ROUGHNESS, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT | ShaderStageFlagBits::VERTEX_BIT}, //metallic and roughness
		DescriptorSetLayoutBinding{TEXTURE_BINDING_OCCLUSION, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT | ShaderStageFlagBits::VERTEX_BIT},			//occlusion
		DescriptorSetLayoutBinding{TEXTURE_BINDING_EMISSION, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT | ShaderStageFlagBits::VERTEX_BIT},			//emission
		DescriptorSetLayoutBinding{TEXTURE_BINDING_TRANSPARENCY, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT | ShaderStageFlagBits::VERTEX_BIT},		//transparency
		DescriptorSetLayoutBinding{UNIFORM_BINDING_MATERIAL, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::FRAGMENT_BIT | ShaderStageFlagBits::VERTEX_BIT},					//UBO material
	};
	//set id 3
	static std::vector<DescriptorSetLayoutBinding> jointMatrixBindings{
		DescriptorSetLayoutBinding{UNIFORM_BINDING_JOINTMATRIX, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT},
	};

	static DescriptorSetLayout *nodeDescriptorSetLayout = g_App->getDevice()->Create(DescriptorSetLayoutCreateInfo{nodeBindings});
	static DescriptorSetLayout *materialDescriptorSetLayout = g_App->getDevice()->Create(DescriptorSetLayoutCreateInfo{materialBindings});
	static DescriptorSetLayout *jointMatricDescriptorSetLayout = g_App->getDevice()->Create(DescriptorSetLayoutCreateInfo{jointMatrixBindings});

	auto materialCount = (std::max)(_model.materials.size(), size_t(1));
	auto nodeCount = _model.nodes.size();
	auto skinCount = (std::max)(_model.skins.size(), size_t(1));

	std::vector<DescriptorPoolSize> poolSizes;
	for (auto &&e : nodeBindings)
		poolSizes.emplace_back(e.descriptorType, e.descriptorCount * 2);
	for (auto &&e : materialBindings)
		poolSizes.emplace_back(e.descriptorType, e.descriptorCount);

	//create descriptor pool
	DescriptorPoolCreateInfo descriptorPoolCreateInfo{materialCount + (nodeCount + skinCount) * g_App->getImageCount(), poolSizes};
	_descriptorPool = g_App->getDevice()->Create(descriptorPoolCreateInfo);

	//material descriptor
	std::vector<DescriptorSetLayout *> materialSetLayouts(materialCount, materialDescriptorSetLayout);
	DescriptorSetAllocateInfo allocInfo{materialSetLayouts};
	_descriptorPool->Allocate(allocInfo, _materialDescriptorSets);

	//node attribute descriptor
	std::vector<DescriptorSetLayout *> nodeSetLayouts(nodeCount, nodeDescriptorSetLayout);
	allocInfo = DescriptorSetAllocateInfo{nodeSetLayouts};
	_frameNodeDescriptorSets.resize(g_App->getImageCount());
	for (uint32_t k = 0; k < g_App->getImageCount(); ++k)
		_descriptorPool->Allocate(allocInfo, _frameNodeDescriptorSets[k]);

	//joint matrix descriptor
	std::vector<DescriptorSetLayout *> jointMatrixSetLayouts(skinCount, jointMatricDescriptorSetLayout);
	allocInfo = DescriptorSetAllocateInfo{jointMatrixSetLayouts};
	_frameSkinJointMatrixDescriptorSets.resize(g_App->getImageCount());
	for (uint32_t k = 0; k < g_App->getImageCount(); ++k)
		_descriptorPool->Allocate(allocInfo, {_frameSkinJointMatrixDescriptorSets[k]});
}
void Model2::loadMaterials()
{
	//material buffers
	auto materialCount = _model.materials.size();
	BufferCreateInfo bufferCreateInfo{
		{},
		sizeof(UBOMaterial) * (std::max)(materialCount, size_t(1)),
		BufferUsageFlagBits::UNIFORM_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
		MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
	std::vector<UBOMaterial> materials((std::max)(materialCount, size_t(1)), UBOMaterial{});
	std::transform(_model.materials.begin(), _model.materials.end(), materials.begin(), [](auto &&m) {
		float normalScale = m.normalTexture.index == -1 ? 0 : m.normalTexture.scale;
		return UBOMaterial{
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
			normalScale};
	});
	_materialBuffer = g_App->getDevice()->Create(bufferCreateInfo, (void *)materials.data());

	std::vector<WriteDescriptorSet> writes;
	int blackTextureIndex = _images.size() - 1;
	int whiteTextureIndex = _images.size() - 2;

	if (materialCount == 0)
	{
		for (uint32_t i = 0; i < 6; ++i)
		{
			writes.emplace_back(
				WriteDescriptorSet{
					_materialDescriptorSets[0],
					i,
					0,
					DescriptorType::COMBINED_IMAGE_SAMPLER,
					std::vector<DescriptorImageInfo>{{_linearSampler, //sampler
													  _imageViews[whiteTextureIndex],
													  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
		}
		writes[TEXTURE_BINDING_EMISSION].values =
			std::vector<DescriptorImageInfo>{{_linearSampler, //sampler
											  _imageViews[blackTextureIndex],
											  ImageLayout::SHADER_READ_ONLY_OPTIMAL}};
		writes.emplace_back(
			WriteDescriptorSet{
				_materialDescriptorSets[0],
				UNIFORM_BINDING_MATERIAL,
				0,
				DescriptorType::UNIFORM_BUFFER,
				std::vector<DescriptorBufferInfo>{{_materialBuffer,
												   0,
												   sizeof(UBOMaterial)}}});
	}

	std::array<int, 6> textures;
	for (size_t i = 0; i < materialCount; ++i)
	{
		auto &&material = _model.materials[i];
		textures.fill(whiteTextureIndex);
		if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
		{
			textures[TEXTURE_BINDING_ALBEDO] = material.pbrMetallicRoughness.baseColorTexture.index;
		}
		if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
		{
			textures[TEXTURE_BINDING_METALLIC_ROUGHNESS] = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
		}
		if (material.normalTexture.index != -1)
		{
			textures[TEXTURE_BINDING_NORMAL] = material.normalTexture.index;
		}
		if (material.occlusionTexture.index != -1)
		{
			textures[TEXTURE_BINDING_OCCLUSION] = material.occlusionTexture.index;
		}
		if (material.emissiveTexture.index != -1)
		{
			textures[TEXTURE_BINDING_EMISSION] = material.emissiveTexture.index;
		}

		for (uint32_t j = 0; j < 6; ++j)
		{
			writes.emplace_back(
				WriteDescriptorSet{
					_materialDescriptorSets[i],
					j,
					0,
					DescriptorType::COMBINED_IMAGE_SAMPLER,
					std::vector<DescriptorImageInfo>{{_linearSampler,
													  _imageViews[textures[j]],
													  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
		}
		//material descriptor update
		writes.emplace_back(
			WriteDescriptorSet{
				_materialDescriptorSets[i],
				UNIFORM_BINDING_MATERIAL,
				0,
				DescriptorType::UNIFORM_BUFFER,
				std::vector<DescriptorBufferInfo>{{_materialBuffer,
												   sizeof(UBOMaterial) * i,
												   sizeof(UBOMaterial)}}});
	}
	g_App->getDevice()->UpdateDescriptorSets(writes, {});
}
//void Model2::createSkyboxDrawCommandInfo()
//{
//	//create draw command buffer
//	std::vector<DrawIndirectCommand> drawIndirectCmds{{36, 1, 0, 0}};
//	_indirectDrawCmdBuffer = g_App->getDevice()->Create(
//		BufferCreateInfo{
//			{},
//			sizeof(DrawIndirectCommand) * drawIndirectCmds.size(),
//			BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
//			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
//		drawIndirectCmds.data());
//}
//void Model2::createAxisDrawCommandInfo()
//{
//	//axis draw command buffer
//	std::vector<DrawIndirectCommand> drawIndirectCmds{{6, 1, 0, 0}};
//	BufferCreateInfo bufferCreateInfo{
//		{},
//		sizeof(DrawIndirectCommand) * drawIndirectCmds.size(),
//		BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
//		MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
//	_indirectDrawCmdBuffer = g_App->getDevice()->Create(bufferCreateInfo, drawIndirectCmds.data());
//}
//void Model2::createDrawCommandInfo()
//{
//	switch (_modelType)
//	{
//	case ModelType::SKYBOX:
//		createSkyboxDrawCommandInfo();
//		break;
//	case ModelType::AXIS:
//		createAxisDrawCommandInfo();
//		break;
//	default:
//		//createModelDrawCommandInfo();
//		break;
//	}
//}
//void Model2::createModelDrawCommandInfo()
//{
//	//primitive draw indexed indiect command info
//	//_primitivesDrawIndirectInfo.reserve(_model.accessors.size(), {});
//	for (size_t i = 0; i < _model.meshes.size(); ++i)
//	{
//		for (size_t j = 0; j < _model.meshes[i].primitives.size(); ++j)
//		{
//			auto &&primitive = _model.meshes[i].primitives[j];
//
//			if (primitive.indices == -1)
//			{
//				int positionAccessorIndex = primitive.attributes.at("POSITION");
//				auto &&positionAccessor = _model.accessors[positionAccessorIndex];
//				//DrawIndirectCommand cmd{
//				//	positionAccessor.count,
//				//	_instanceAttributes.size(),
//				//};
//				//BufferCreateInfo createInfo{
//				//	{},
//				//	sizeof(DrawIndirectCommand),
//				//	BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
//				//	MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
//				//_meshViews[i].primitiveViews[j].drawIndirectInfo = {g_App->getDevice()->Create(createInfo, &cmd),
//				//													0,
//				//													1,
//				//													sizeof(DrawIndirectInfo)};
//				_meshViews[i].primitiveViews[j].drawIndirectCommand = DrawIndirectCommand{
//					positionAccessor.count,
//					1,
//				};
//			}
//			else
//			{
//				auto &&indexAccessor = _model.accessors[primitive.indices];
//				DrawIndexedIndirectCommand cmd{
//					indexAccessor.count,
//					_instanceAttributes.size(),
//				};
//				BufferCreateInfo createInfo{
//					{},
//					sizeof(DrawIndexedIndirectCommand),
//					BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
//					MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
//				_meshViews[i].primitiveViews[j].drawIndirectInfo = {g_App->getDevice()->Create(createInfo, &cmd),
//																	0,
//																	1,
//																	sizeof(DrawIndirectInfo)};
//			}
//		}
//	}
//}
void Model2::draw(CommandBuffer *pCommandBuffer, uint32_t imageIndex)
{
	switch (_modelType)
	{
	case ModelType::SKYBOX:
		pCommandBuffer->Draw(DrawIndirectCommand{36, 1});
		break;
	case ModelType::AXIS:
	{
		pCommandBuffer->Draw(DrawIndirectCommand{6, 1});
		//pCommandBuffer->DrawIndirect({_indirectDrawCmdBuffer,
		//							  0,
		//							  1,
		//							  sizeof(DrawIndirectCommand)});
	}
	break;
	default:
		drawModel(0, pCommandBuffer, imageIndex);
		break;
	}
}

static uint32_t curSceneIndex = 0;
static uint32_t curImageIndex = 0;
static CommandBuffer *pCurCommandBuffer = 0;
void Model2::drawModel(uint32_t sceneIndex, CommandBuffer *pCommandBuffer, uint32_t imageIndex)
{
	curSceneIndex = sceneIndex;
	curImageIndex = imageIndex;
	pCurCommandBuffer = pCommandBuffer;

	//always use skin 0
	//if (!_model.skins.empty())
	{
		pCurCommandBuffer->BindDescriptorSets(BindDescriptorSetsInfo{
			PipelineBindPoint::GRAPHICS,
			(*g_ppDefaultMaterial)->_pipelineLayout,
			DESCRIPTORSET_ID_JOINTMATRIX,
			1,
			&_frameSkinJointMatrixDescriptorSets[imageIndex][0]});
	}

	Buffer *buffers[]{_vertexBuffer, _frameInstanceAttributeBuffers[imageIndex]};
	uint64_t offsets[]{0, 0};
	pCurCommandBuffer->BindVertexBuffer(BindVertexBufferInfo{
		0,
		2,
		&buffers[0],
		&offsets[0]});

	if (_indexBuffer)
	{
		pCurCommandBuffer->BindIndexBuffer(BindIndexBufferInfo{
			_indexBuffer,
			0,
			IndexType::UINT32});
	}
	//LOG_VAR(_model.scenes[sceneIndex].name)
	for (auto &&nodeIndex : _model.scenes[sceneIndex].nodes)
	{
		drawNode(nodeIndex);
	}
}
void Model2::drawNode(uint32_t nodeIndex)
{
	auto &&node = _model.nodes[nodeIndex];
	if ((node.mesh >= 0) && (node.mesh < _model.meshes.size()))
	{
		pCurCommandBuffer->BindDescriptorSets(
			BindDescriptorSetsInfo{
				PipelineBindPoint::GRAPHICS,
				(*g_ppDefaultMaterial)->_pipelineLayout,
				DESCRIPTORSET_ID_NODE,
				1,
				&_frameNodeDescriptorSets[curImageIndex][nodeIndex]});

		drawMesh(_meshViews[node.mesh]);
	}
	for (size_t i = 0; i < node.children.size(); i++)
	{
		assert((node.children[i] >= 0) && (node.children[i] < _model.nodes.size()));
		drawNode(node.children[i]);
	}
}

void Model2::drawMesh(const MeshView &meshView)
{
	//BindVertexBufferInfo bindVertexBufferInfo{0, VERTEX_LOCATION_NUM_MAX};
	//Buffer *buffers[2]{_vertexBuffer, _frameInstanceAttributeBuffers[curImageIndex]};
	//uint64_t offsets[2]{};
	for (auto &&primitiveView : meshView.primitiveViews)
	{
		if (_useModelMaterial)
		{
			auto material = primitiveView.material == -1 ? 0 : primitiveView.material;
			//bind material
			pCurCommandBuffer->BindDescriptorSets(
				BindDescriptorSetsInfo{
					PipelineBindPoint::GRAPHICS,
					(*g_ppDefaultMaterial)->_pipelineLayout,
					DESCRIPTORSET_ID_MATERIAL,
					1,
					&_materialDescriptorSets[material]});
		}

		//offsets[0] = primitiveView.vertexBufferOffset;
		//pCurCommandBuffer->BindVertexBuffer(BindVertexBufferInfo{0, 2, buffers, offsets});

		if (auto pDrawCmd = std::get_if<DrawIndirectCommand>(&primitiveView.drawCommand))
		{
			pCurCommandBuffer->Draw(*pDrawCmd);
		}
		else if (auto pDrawIndexCmd = std::get_if<DrawIndexedIndirectCommand>(&primitiveView.drawCommand))
		{
			pCurCommandBuffer->DrawIndexed(*pDrawIndexCmd);
		}

		//if (primitiveView.indexBufferOffset == -1)
		//{
		//	//pCurCommandBuffer->DrawIndirect(primitiveView.drawIndirectInfo);
		//	auto drawCmd = std::get<DrawIndirectCommand>(primitiveView.drawCommand);
		//	pCurCommandBuffer->Draw(DrawIndirectCommand{

		//	});
		//}
		//else
		//{
		//	//bind indexbuffer
		//	pCurCommandBuffer->BindIndexBuffer(BindIndexBufferInfo{
		//		_indexBuffer,
		//		static_cast<uint64_t>(primitiveView.indexBufferOffset),
		//		IndexType::UINT32});
		//	//pCurCommandBuffer->DrawIndexedIndirect(primitiveView.drawIndirectInfo);
		//	pCurCommandBuffer->DrawIndexed(DrawIndexedIndirectCommand{

		//	});
		//}
	}
}

void Model2::createSamplers()
{
	_linearSampler = g_App->getDevice()->Create(SamplerCreateInfo{
		Filter::LINEAR,
		Filter::LINEAR,
		SamplerMipmapMode::LINEAR,
		SamplerWrapMode::REPEAT,
		SamplerWrapMode::REPEAT,
		SamplerWrapMode::REPEAT,
		0,	   //load bias
		false, //anistropy
		1.f,
		false, //compare
		CompareOp::ALWAYS,
		-100,
		100,
		BorderColor::FLOAT_OPAQUE_BLACK,
	});
}
void Model2::loadImages()
{
	//load images
	auto curDir = _modelPath;
	curDir.remove_filename();

	ImageCreateInfo imageCreateInfo{
		{},
		ImageType::TYPE_2D,
		ShitFormat::RGBA8_UNORM,
		{},
		0,
		1,
		SampleCountFlagBits::BIT_1,
		ImageTiling::OPTIMAL,
		ImageUsageFlagBits::SAMPLED_BIT,
		MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL};

	ImageViewCreateInfo imageViewCreateInfo{
		nullptr,
		ImageViewType::TYPE_2D,
		ShitFormat::RGBA8_UNORM,
		{},
		{0, 1, 0, 1}};

	for (auto &&tex : _model.textures)
	{
		auto imagePath = curDir.string() + _model.images[tex.source].uri;
		int width, height, components;
		auto pixels = loadImage(imagePath.c_str(), width, height, components, 4);
		imageCreateInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
		_images.emplace_back(g_App->getDevice()->Create(imageCreateInfo, pixels));
		_images.back()->GenerateMipmaps(Filter::LINEAR, ImageLayout::SHADER_READ_ONLY_OPTIMAL, ImageLayout::SHADER_READ_ONLY_OPTIMAL);

		//imageview
		imageViewCreateInfo.pImage = _images.back();
		imageViewCreateInfo.subresourceRange.levelCount = imageViewCreateInfo.pImage->GetCreateInfoPtr()->mipLevels;
		_imageViews.emplace_back(g_App->getDevice()->Create(imageViewCreateInfo));
		freeImage(pixels);
	}
	//add default texture
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.extent = {1, 1, 1};

	//create default white image
	_images.emplace_back(g_App->getDevice()->Create(imageCreateInfo, COLOR_WHITE));
	imageViewCreateInfo.pImage = _images.back();
	imageViewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;
	_imageViews.emplace_back(g_App->getDevice()->Create(imageViewCreateInfo));

	//create default black image
	_images.emplace_back(g_App->getDevice()->Create(imageCreateInfo, COLOR_BLACK));
	imageViewCreateInfo.pImage = _images.back();
	imageViewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;
	_imageViews.emplace_back(g_App->getDevice()->Create(imageViewCreateInfo));
}
void Model2::loadNodesHelper(int nodeIndex, std::vector<NodeTransformation> &nodeTransformations, std::vector<UBONodeAttribute> &nodeAttributes, const glm::dmat4 &preMatrix)
{
	auto &&node = _model.nodes[nodeIndex];
	if (!node.rotation.empty())
		nodeTransformations[nodeIndex].rotate = glm::mat4_cast(glm::dquat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]));
	if (!node.scale.empty())
		nodeTransformations[nodeIndex].scale = glm::scale(glm::dmat4(1), glm::dvec3(node.scale[0], node.scale[1], node.scale[2]));
	if (!node.translation.empty())
		nodeTransformations[nodeIndex].translate = glm::translate(glm::dmat4(1), glm::dvec3(node.translation[0], node.translation[1], node.translation[2]));
	if (!node.matrix.empty())
		memcpy(&nodeTransformations[nodeIndex].extra, node.matrix.data(), sizeof(double) * node.matrix.size());

	nodeAttributes[nodeIndex].matrix = preMatrix *
									   nodeTransformations[nodeIndex].extra *
									   nodeTransformations[nodeIndex].translate *
									   nodeTransformations[nodeIndex].rotate *
									   nodeTransformations[nodeIndex].scale;

	//update bounding volume
	if (node.mesh != -1)
	{
		for (auto &&primitive : _model.meshes[node.mesh].primitives)
		{
			auto &&accessor = _model.accessors[primitive.attributes["POSITION"]];
			auto a = nodeAttributes[nodeIndex].matrix * glm::dvec4(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2], 1);
			auto b = nodeAttributes[nodeIndex].matrix * glm::dvec4(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2], 1);
			_boundingVolume.box.aabb.maxValue = glm::max(glm::max(_boundingVolume.box.aabb.maxValue, glm::dvec3(a)), glm::dvec3(b));
			_boundingVolume.box.aabb.minValue = glm::min(glm::min(_boundingVolume.box.aabb.minValue, glm::dvec3(a)), glm::dvec3(b));
		}
	}

	for (auto &&subNodeIndex : node.children)
	{
		loadNodesHelper(subNodeIndex, nodeTransformations, nodeAttributes, nodeAttributes[nodeIndex].matrix);
	}
}
void Model2::loadNodes()
{
	auto nodeCount = _model.nodes.size();
	_nodeTransformations.resize(nodeCount);
	std::vector<UBONodeAttribute> nodeAttributes(nodeCount);
	for (auto &&scene : _model.scenes)
	{
		for (auto &&nodeIndex : scene.nodes)
		{
			loadNodesHelper(nodeIndex, _nodeTransformations, nodeAttributes, glm::dmat4(1));
		}
	}
	_boundingVolume.box.aabb.center = (_boundingVolume.box.aabb.minValue + _boundingVolume.box.aabb.maxValue) / glm::dvec3(2);

	_frameNodeAttributeBuffers.resize(g_App->getImageCount());

	std::vector<WriteDescriptorSet> writes;
	for (uint32_t i = 0; i < g_App->getImageCount(); ++i)
	{
		_frameNodeAttributeBuffers[i] = g_App->getDevice()->Create(BufferCreateInfo{
																	   {},
																	   static_cast<uint32_t>(sizeof(UBONodeAttribute) * nodeAttributes.size()),
																	   BufferUsageFlagBits::UNIFORM_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
																	   MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
																   reinterpret_cast<const void *>(nodeAttributes.data()));
		for (size_t j = 0; j < nodeCount; ++j)
		{
			writes.emplace_back(
				WriteDescriptorSet{
					_frameNodeDescriptorSets[i][j],
					UNIFORM_BINDING_NODE,
					0,
					DescriptorType::UNIFORM_BUFFER,
					std::vector<DescriptorBufferInfo>{{_frameNodeAttributeBuffers[i],
													   sizeof(UBONodeAttribute) * j,
													   sizeof(UBONodeAttribute)}}});
		}
	}
	g_App->getDevice()->UpdateDescriptorSets(writes, {});
}
void Model2::loadSkins()
{
	_frameSkinJointMatrixBuffers.resize(g_App->getImageCount());

	auto skinCount = _model.skins.size();

	std::vector<WriteDescriptorSet> writes;
	if (skinCount == 0)
	{
		static Buffer *jointMatrixBuffer = g_App->getDevice()->Create(BufferCreateInfo{
																		  {},
																		  sizeof(glm::mat4),
																		  BufferUsageFlagBits::UNIFORM_BUFFER_BIT,
																		  MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
																	  nullptr);
		for (size_t k = 0; k < g_App->getImageCount(); ++k)
		{
			writes.emplace_back(
				WriteDescriptorSet{
					_frameSkinJointMatrixDescriptorSets[k][0],
					UNIFORM_BINDING_JOINTMATRIX,
					0,
					DescriptorType::UNIFORM_BUFFER,
					std::vector<DescriptorBufferInfo>{
						DescriptorBufferInfo{jointMatrixBuffer,
											 0,
											 1}}});
		}
	}
	else
	{

		for (size_t i = 0; i < skinCount; ++i)
		{
			auto &&accessor = _model.accessors[_model.skins[i].inverseBindMatrices];
			auto &&bufferView = _model.bufferViews[accessor.bufferView];

			struct AA
			{
				int hasSkin;
				std::vector<glm::mat4> matrices;
			} uboJointMatrix;
			auto buffersize = sizeof(int) + sizeof(glm::mat4) * accessor.count;
			uboJointMatrix.hasSkin = 1;
			uboJointMatrix.matrices.resize(accessor.count);
			memcpy(uboJointMatrix.matrices.data(), _model.buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset, accessor.count * sizeof(glm::mat4));

			for (size_t k = 0; k < g_App->getImageCount(); ++k)
			{
				_frameSkinJointMatrixBuffers[k].resize(skinCount);
				_frameSkinJointMatrixBuffers[k][i] = g_App->getDevice()->Create(
					BufferCreateInfo{
						{},
						sizeof(glm::mat4) * accessor.count + sizeof(int),
						BufferUsageFlagBits::UNIFORM_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
						MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
					&uboJointMatrix);
				writes.emplace_back(
					WriteDescriptorSet{
						_frameSkinJointMatrixDescriptorSets[k][i],
						UNIFORM_BINDING_JOINTMATRIX,
						0,
						DescriptorType::UNIFORM_BUFFER,
						std::vector<DescriptorBufferInfo>{
							DescriptorBufferInfo{_frameSkinJointMatrixBuffers[k][i],
												 0,
												 buffersize}}});
			}
		}
	}
	g_App->getDevice()->UpdateDescriptorSets(writes, {});
}

void Model2::updateAnimation(uint32_t animationIndex, float time, uint32_t imageIndex)
{
	auto period = _animationStates[animationIndex].endTime - _animationStates[animationIndex].startTime;
	auto c = time / period;
	c -= static_cast<long long>(c);
	auto animationTime = c * period + _animationStates[animationIndex].startTime;

	for (auto &&channel : _model.animations[animationIndex].channels)
	{
		auto &&sampler = _model.animations[animationIndex].samplers[channel.sampler];
		auto &&inputAccessor = _model.accessors[sampler.input];
		auto &&inputBufferView = _model.bufferViews[inputAccessor.bufferView];

		float *pPreTime = reinterpret_cast<float *>(_model.buffers[inputBufferView.buffer].data.data() + inputAccessor.byteOffset + inputBufferView.byteOffset);
		if (*pPreTime > animationTime)
			continue;

		auto &&outputAccessor = _model.accessors[sampler.output];
		auto &&outputBufferView = _model.bufferViews[outputAccessor.bufferView];

		float *pNextTime = pPreTime + 1;
		auto outValueSize = tinygltf::GetComponentSizeInBytes(outputAccessor.componentType);
		auto outValueComponentCount = tinygltf::GetNumComponentsInType(outputAccessor.type);
		for (size_t i = 0; i < inputAccessor.count; ++i, ++pPreTime, ++pNextTime)
		{
			if (*pNextTime > animationTime)
			{
				auto factor = (animationTime - *pPreTime) / (*pNextTime - *pPreTime);
				unsigned char *pPreOutValue = _model.buffers[outputBufferView.buffer].data.data() + outputAccessor.byteOffset + outputBufferView.byteOffset + i * outValueSize * outValueComponentCount;
				unsigned char *pNextOutValue = pPreOutValue + outValueSize * outValueComponentCount;
				if (channel.target_path.compare("translation") == 0)
				{
					auto preVal = glm::vec3(*reinterpret_cast<float *>(pPreOutValue), *(reinterpret_cast<float *>(pPreOutValue) + 1), *(reinterpret_cast<float *>(pPreOutValue) + 2));
					auto nextVal = glm::vec3(*reinterpret_cast<float *>(pNextOutValue), *(reinterpret_cast<float *>(pNextOutValue) + 1), *(reinterpret_cast<float *>(pNextOutValue) + 2));

					if (sampler.interpolation.compare("LINEAR") == 0)
						_nodeTransformations[channel.target_node].translate = glm::translate(glm::mat4(1), glm::mix(preVal, nextVal, glm::vec3(factor)));
					else if (sampler.interpolation.compare("STEP") == 0)
						_nodeTransformations[channel.target_node].translate = glm::translate(glm::mat4(1), preVal);
					else if (sampler.interpolation.compare("CUBICSPLINE") == 0)
						_nodeTransformations[channel.target_node].translate = glm::translate(glm::mat4(1), glm::smoothstep(preVal, nextVal, glm::vec3(factor)));
				}
				else if (channel.target_path.compare("rotation") == 0)
				{
					glm::quat preVal{};
					glm::quat nextVal{};
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
					_nodeTransformations[channel.target_node].rotate = glm::mat4_cast(interpolation);
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
					_nodeTransformations[channel.target_node].scale = trans;
				}
				else if (channel.target_path.compare("weights") == 0)
				{
					//TODO: morph targets
				}
				break;
			}
		}
	}

	//TODO:: set scene index
	std::vector<glm::mat4> nodeMatrices(_model.nodes.size());
	for (auto nodeIndex : _model.scenes[curSceneIndex].nodes)
	{
		updateAnimationHelper(nodeIndex, nodeMatrices, glm::mat4(1));
	}
	if (!_model.skins.empty())
	{
		std::vector<glm::mat4> jointMatrices(_model.skins[0].joints.size());
		for (auto &&nodeIndex : _model.scenes[curSceneIndex].nodes)
			updateSkins(nodeIndex, jointMatrices, nodeMatrices);

		//always use skin 0
		if (!jointMatrices.empty())
		{
			void *data;
			auto size = sizeof(glm::mat4) * jointMatrices.size();
			_frameSkinJointMatrixBuffers[imageIndex][0]->MapMemory(0, size, &data);
			memcpy(data, jointMatrices.data(), size);
			_frameSkinJointMatrixBuffers[imageIndex][0]->UnMapMemory();
		}
	}

	//copy node
	auto nodeCount = _model.nodes.size();
	void *pData;
	_frameNodeAttributeBuffers[imageIndex]->MapMemory(0, sizeof(UBONodeAttribute) * nodeCount, &pData);
	UBONodeAttribute *pUBONodeAttribute = reinterpret_cast<UBONodeAttribute *>(pData);
	for (auto &&e : nodeMatrices)
	{
		pUBONodeAttribute->matrix = e;
		pUBONodeAttribute++;
	}
	_frameNodeAttributeBuffers[imageIndex]->UnMapMemory();
}
void Model2::updateAnimationHelper(int nodeIndex, std::vector<glm::mat4> &nodeMatrices, const glm::dmat4 &preMatrix)
{
	nodeMatrices[nodeIndex] = preMatrix *
							  _nodeTransformations[nodeIndex].extra *
							  _nodeTransformations[nodeIndex].translate *
							  _nodeTransformations[nodeIndex].rotate *
							  _nodeTransformations[nodeIndex].scale;
	for (auto &&subNodeIndex : _model.nodes[nodeIndex].children)
		updateAnimationHelper(subNodeIndex, nodeMatrices, nodeMatrices[nodeIndex]);
}
void Model2::updateSkins(int nodeIndex, std::vector<glm::mat4> &jointMatrices, const std::vector<glm::mat4> &nodeMatrices)
{
	if (_model.nodes[nodeIndex].skin != -1)
	{
		auto &&skin = _model.skins[0];
		auto &&accessor = _model.accessors[skin.inverseBindMatrices];
		auto &&bufferView = _model.bufferViews[accessor.bufferView];
		auto p = reinterpret_cast<glm::mat4 *>(_model.buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset);

		auto jointCount = skin.joints.size();
		jointMatrices.assign(jointCount, glm::mat4(1));
		auto invM = glm::inverse(nodeMatrices[nodeIndex]);
		for (size_t i = 0; i < jointCount; ++i)
		{
			auto m = nodeMatrices[skin.joints[i]];
			jointMatrices[i] = invM * m * (*(p + i));
		}
		return;
	}
	for (auto &&child : _model.nodes[nodeIndex].children)
	{
		updateSkins(child, jointMatrices, nodeMatrices);
	}
}
void Model2::useModelMaterial(bool flag)
{
	_useModelMaterial = flag;
}