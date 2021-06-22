#include "material.hpp"
#include "node.hpp"
#include "appbase.hpp"

extern Material *g_DefaultMaterial;

#define CUBEMAP_WIDTH 1024

VertexInputStateCreateInfo Vertex::getVertexInputStateCreateInfo()
{
	return VertexInputStateCreateInfo{
		std::vector<VertexBindingDescription>{
			{0, sizeof(Vertex), 0},
			{1, sizeof(InstanceAttribute), 1},
		},
		std::vector<VertexAttributeDescription>{
			{LOCATION_POSITION, 0, ShitFormat::RGB32_SFLOAT, offsetof(Vertex, position)},
			{LOCATION_NORMAL, 0, ShitFormat::RGB32_SFLOAT, offsetof(Vertex, normal)},
			{LOCATION_TANGENT, 0, ShitFormat::RGBA32_SFLOAT, offsetof(Vertex, tangent)},
			{LOCATION_TEXCOORD0, 0, ShitFormat::RG32_SFLOAT, offsetof(Vertex, texCoord)},
			{LOCATION_COLOR0, 0, ShitFormat::RGBA32_SFLOAT, offsetof(Vertex, color)},
			{LOCATION_JOINTS0, 0, ShitFormat::RGBA32_SFLOAT, offsetof(Vertex, joints)},
			{LOCATION_WEIGHTS0, 0, ShitFormat::RGBA32_SFLOAT, offsetof(Vertex, weights)},
			{LOCATION_INSTANCE_COLOR_FACTOR, 1, ShitFormat::RGBA32_SFLOAT, 0},
			{LOCATION_INSTANCE_MATRIX + 0, 1, ShitFormat::RGBA32_SFLOAT, 16},
			{LOCATION_INSTANCE_MATRIX + 1, 1, ShitFormat::RGBA32_SFLOAT, 32},
			{LOCATION_INSTANCE_MATRIX + 2, 1, ShitFormat::RGBA32_SFLOAT, 48},
			{LOCATION_INSTANCE_MATRIX + 3, 1, ShitFormat::RGBA32_SFLOAT, 64},
		}};
}

Material::Material()
{
	_name="new material";
	_type = ResourceType::MATERIAL;
	_materialType = MATERIAL_CUSTOM;

	//=================
	_linearSampler = g_App->getDevice()->Create(SamplerCreateInfo{
		Filter::LINEAR,
		Filter::LINEAR,
		SamplerMipmapMode::LINEAR,
		SamplerWrapMode::CLAMP_TO_EDGE,
		SamplerWrapMode::CLAMP_TO_EDGE,
		SamplerWrapMode::CLAMP_TO_EDGE,
		0,
		false,
		1.,
		false,
		{},
		-30.f,
		30.f,
	});
}
void Material::bind(CommandBuffer *pCommandBuffer, uint32_t imageIndex)
{
	auto pipelineBindPoint = dynamic_cast<GraphicsPipeline *>(_pipeline) ? PipelineBindPoint::GRAPHICS : PipelineBindPoint::COMPUTE;

	pCommandBuffer->BindPipeline(BindPipelineInfo{pipelineBindPoint, _pipeline});
	for (size_t i = 0; i < _descriptorSets.size(); ++i)
	{
		pCommandBuffer->BindDescriptorSets(
			BindDescriptorSetsInfo{
				PipelineBindPoint::GRAPHICS,
				_pipelineLayout,
				_descriptorSetIDs[i],
				1,
				&_descriptorSets[i]});
	}
}
MaterialStandard::MaterialStandard()
{
	_type = ResourceType::MATERIAL;
	_materialType = MATERIAL_STANDARD;

	_vertShaderName = "standard.vert.spv";
	_fragShaderName = "standard.frag.spv";

	//set id 0 UBO pv
	static std::vector<DescriptorSetLayoutBinding> PVBindings{
		DescriptorSetLayoutBinding{UNIFORM_BINDING_FRAME, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT | ShaderStageFlagBits::FRAGMENT_BIT}, //UBO PV
	};

	//set id 1
	static std::vector<DescriptorSetLayoutBinding> nodeBindings{
		DescriptorSetLayoutBinding{UNIFORM_BINDING_NODE, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT}, //UBO M
	};
	//set id 2
	//create texture descriptorSets
	static std::vector<DescriptorSetLayoutBinding> materialBindings{
		DescriptorSetLayoutBinding{TEXTURE_BINDING_ALBEDO, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT},			  //albedo
		DescriptorSetLayoutBinding{TEXTURE_BINDING_NORMAL, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT},			  //normal
		DescriptorSetLayoutBinding{TEXTURE_BINDING_METALLIC_ROUGHNESS, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT}, //metallic and roughness
		DescriptorSetLayoutBinding{TEXTURE_BINDING_OCCLUSION, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT},		  //occlusion
		DescriptorSetLayoutBinding{TEXTURE_BINDING_EMISSION, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT},			  //emission
		DescriptorSetLayoutBinding{TEXTURE_BINDING_TRANSPARENCY, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT},		  //transparency
		DescriptorSetLayoutBinding{UNIFORM_BINDING_MATERIAL, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::FRAGMENT_BIT},					  //UBO material
	};
	//set id 3
	static std::vector<DescriptorSetLayoutBinding> jointMatrixBindings{
		DescriptorSetLayoutBinding{UNIFORM_BINDING_JOINTMATRIX, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT},
	};

	static DescriptorSetLayout *PVDescriptorSetLayout = g_App->getDevice()->Create(DescriptorSetLayoutCreateInfo{PVBindings});
	static DescriptorSetLayout *nodeDescriptorSetLayout = g_App->getDevice()->Create(DescriptorSetLayoutCreateInfo{nodeBindings});
	static DescriptorSetLayout *materialDescriptorSetLayout = g_App->getDevice()->Create(DescriptorSetLayoutCreateInfo{materialBindings});
	static DescriptorSetLayout *jointMatricDescriptorSetLayout = g_App->getDevice()->Create(DescriptorSetLayoutCreateInfo{jointMatrixBindings});

	//creaet pipeline layout
	_pipelineLayout = g_App->getDevice()->Create(
		PipelineLayoutCreateInfo{
			{
				PVDescriptorSetLayout,
				nodeDescriptorSetLayout,
				materialDescriptorSetLayout,
				jointMatricDescriptorSetLayout,
			}});

	std::vector<DescriptorPoolSize> poolSizes;
	for (auto &&e : materialBindings)
		poolSizes.emplace_back(e.descriptorType, e.descriptorCount);

	//create descriptor pool
	DescriptorPoolCreateInfo descriptorPoolCreateInfo{1, poolSizes};
	auto descriptorPool = g_App->getDevice()->Create(descriptorPoolCreateInfo);

	//material descriptor
	DescriptorSetAllocateInfo allocInfo{{materialDescriptorSetLayout}};
	descriptorPool->Allocate(allocInfo, _descriptorSets);

	_descriptorSetIDs = {DESCRIPTORSET_ID_MATERIAL};

	//=======================================================
	//create default white and black image
	ImageCreateInfo imageCreateInfo{
		{},
		ImageType::TYPE_2D,
		ShitFormat::RGBA8_UNORM,
		{1, 1, 1},
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

	static const unsigned char COLOR_WHITE[4]{255, 255, 255, 255};
	static const unsigned char COLOR_BLACK[4]{0, 0, 0, 1};
	_whiteImage = g_App->getDevice()->Create(imageCreateInfo, COLOR_WHITE);
	imageViewCreateInfo.pImage = _whiteImage;
	_whiteImageView = g_App->getDevice()->Create(imageViewCreateInfo);
	_blackImage = g_App->getDevice()->Create(imageCreateInfo, COLOR_BLACK);
	imageViewCreateInfo.pImage = _blackImage;
	_blackImageView = g_App->getDevice()->Create(imageViewCreateInfo);

	//default UBO material buffer
	UBOMaterial uboMaterial{};
	_materialBuffer = g_App->getDevice()->Create(
		BufferCreateInfo{
			{},
			sizeof(UBOMaterial),
			BufferUsageFlagBits::UNIFORM_BUFFER_BIT,
			MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
		&uboMaterial);
	//update descriptor
	std::vector<WriteDescriptorSet> writes{
		{_descriptorSets[0],
		 UNIFORM_BINDING_MATERIAL,
		 1,
		 DescriptorType::UNIFORM_BUFFER,
		 std::vector<DescriptorBufferInfo>{
			 {_materialBuffer,
			  0,
			  sizeof(UBOMaterial)},
		 }},
	};
	for (uint32_t i = 0; i < 6; ++i)
	{
		writes.emplace_back(
			WriteDescriptorSet{
				_descriptorSets[0],
				i,
				0,
				DescriptorType::COMBINED_IMAGE_SAMPLER,
				std::vector<DescriptorImageInfo>{{_linearSampler, //sampler
												  _whiteImageView,
												  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
	}

	g_App->getDevice()->UpdateDescriptorSets(writes, {});

	//==================================
	//create pipeline
	std::string vertShaderPath = buildShaderPath(_vertShaderName.c_str(), g_RendererVersion);
	std::string fragShaderPath = buildShaderPath(_fragShaderName.c_str(), g_RendererVersion);

	std::string axisVertCode = readFile(vertShaderPath.c_str());
	std::string axisFragCode = readFile(fragShaderPath.c_str());

	auto vertShader = g_App->getDevice()->Create(ShaderCreateInfo{axisVertCode.size(), axisVertCode.data()});
	auto fragShader = g_App->getDevice()->Create(ShaderCreateInfo{axisFragCode.size(), axisFragCode.data()});

	std::vector<PipelineShaderStageCreateInfo>
		shaderStageCreateInfos{
			PipelineShaderStageCreateInfo{
				ShaderStageFlagBits::VERTEX_BIT,
				vertShader,
				"main",
			},
			PipelineShaderStageCreateInfo{
				ShaderStageFlagBits::FRAGMENT_BIT,
				fragShader,
				"main",
			},
		};
	PipelineInputAssemblyStateCreateInfo inputAssemblyState{
		PrimitiveTopology::TRIANGLE_LIST,
	};
	uint32_t frameWidth, frameHeight;
	g_App->getWindow()->GetFramebufferSize(frameWidth, frameHeight);
	PipelineViewportStateCreateInfo viewportStateCreateInfo{
		{{0, 0, static_cast<float>(frameWidth), static_cast<float>(frameHeight), 0, 1}},
		{{{0, 0}, {frameWidth, frameHeight}}}};
	PipelineTessellationStateCreateInfo tessellationState{};
	PipelineRasterizationStateCreateInfo rasterizationState{
		false,
		false,
		PolygonMode::FILL,
		CullMode::BACK,
		FrontFace::COUNTER_CLOCKWISE,
		false,
	};
	rasterizationState.lineWidth = 1.f;

	PipelineMultisampleStateCreateInfo multisampleState{
		SAMPLE_COUNT,
		true,	 //sample shading
		1.f,	 //min sample shading
		nullptr, //mask
		false,	 //alpha to coverage
		false	 //alpha to one
	};
	PipelineDepthStencilStateCreateInfo depthStencilState{
		.depthTestEnable = true,
		.depthWriteEnable = true,
		.depthCompareOp = CompareOp::LESS,
	};

	PipelineColorBlendAttachmentState colorBlendAttachmentstate{
		false,
		BlendFactor::SRC_ALPHA,
		BlendFactor::ONE_MINUS_SRC_ALPHA,
		BlendOp::ADD,
		BlendFactor::SRC_ALPHA,
		BlendFactor::ONE_MINUS_SRC_ALPHA,
		BlendOp::ADD,
		ColorComponentFlagBits::R_BIT | ColorComponentFlagBits::G_BIT | ColorComponentFlagBits::B_BIT | ColorComponentFlagBits::A_BIT};
	PipelineColorBlendStateCreateInfo colorBlendState{
		false,
		LogicOp::COPY,
		{colorBlendAttachmentstate},
	};
	PipelineDynamicStateCreateInfo dynamicStates{
		{
			DynamicState::VIEWPORT,
			DynamicState::SCISSOR,
		},
	};
	GraphicsPipelineCreateInfo pipelineCreateInfo{
		shaderStageCreateInfos,
		Vertex::getVertexInputStateCreateInfo(),
		inputAssemblyState,
		viewportStateCreateInfo,
		tessellationState,
		rasterizationState,
		multisampleState,
		depthStencilState,
		colorBlendState,
		dynamicStates,
		_pipelineLayout,
		g_App->getDefaultRenderPass(),
		g_App->getDefaultSubpass()};
	_pipeline = g_App->getDevice()->Create(pipelineCreateInfo);
};

MaterialSkybox::MaterialSkybox()
{
	_vertShaderName = "cubemap.vert.spv";
	_fragShaderName = "cubemap.frag.spv";

	//create cubmap
	imageCube = g_App->getDevice()->Create(
		ImageCreateInfo{
			ImageCreateFlagBits::CUBE_COMPATIBLE_BIT,
			ImageType::TYPE_2D,
			ShitFormat::RGBA32_SFLOAT,
			{CUBEMAP_WIDTH, CUBEMAP_WIDTH, 1},
			0,
			6,
			SampleCountFlagBits::BIT_1,
			ImageTiling::OPTIMAL,
			ImageUsageFlagBits::SAMPLED_BIT | ImageUsageFlagBits::STORAGE_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			ImageLayout::SHADER_READ_ONLY_OPTIMAL},
		nullptr);

	ImageViewCreateInfo imageViewCreateInfo = {
		imageCube,
		ImageViewType::TYPE_CUBE,
		ShitFormat::RGBA32_SFLOAT,
		{},
		{0, imageCube->GetCreateInfoPtr()->mipLevels, 0, 6}};
	imageViewCube = g_App->getDevice()->Create(imageViewCreateInfo);

	//create ubo buffer
	uboBuffer = g_App->getDevice()->Create(
		BufferCreateInfo{
			{},
			sizeof(UBOSkybox),
			BufferUsageFlagBits::UNIFORM_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
		&uboSkybox);

	//descriptor sets laout
	std::vector<DescriptorSetLayoutBinding> bindings{
		DescriptorSetLayoutBinding{UNIFORM_BINDING_FRAME, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT | ShaderStageFlagBits::FRAGMENT_BIT},
		DescriptorSetLayoutBinding{0, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT}, //
		DescriptorSetLayoutBinding{0, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::FRAGMENT_BIT},		 //

		DescriptorSetLayoutBinding{TEXTURE_BINDING_REFLECTION_ENV_CUBEMAP, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT}, //
		DescriptorSetLayoutBinding{TEXTURE_BINDING_IRRADIANCE_ENV_CUBEMAP, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT}, //
	};
	std::vector<DescriptorSetLayout *> descriptorSetLayouts{
		g_App->getDevice()->Create(DescriptorSetLayoutCreateInfo{{bindings[0]}}),
		g_App->getDevice()->Create(DescriptorSetLayoutCreateInfo{{bindings[1]}}),
		g_App->getDevice()->Create(DescriptorSetLayoutCreateInfo{{bindings[2]}}),
		g_App->getDevice()->Create(DescriptorSetLayoutCreateInfo{{bindings[3], bindings[4]}}),
	};

	//create _pipeline layout
	_pipelineLayout = g_App->getDevice()->Create(PipelineLayoutCreateInfo{descriptorSetLayouts});

	//setup descriptor pool
	std::vector<DescriptorPoolSize> poolSizes{
		{bindings[1].descriptorType, 3},
		{bindings[2].descriptorType, bindings[2].descriptorCount},
	};
	DescriptorPoolCreateInfo descriptorPoolCreateInfo{
		3u,
		poolSizes};
	DescriptorPool *descriptorPool = g_App->getDevice()->Create(descriptorPoolCreateInfo);

	//allocate cubmap descriptor sets
	DescriptorSetAllocateInfo allocInfo = {{descriptorSetLayouts[1], descriptorSetLayouts[2], descriptorSetLayouts[3]}};
	descriptorPool->Allocate(allocInfo, _descriptorSets);
	_descriptorSetIDs = {1, 2, 3};

	//
	std::vector<WriteDescriptorSet> writes{
		{_descriptorSets[0],
		 0,
		 0,
		 DescriptorType::COMBINED_IMAGE_SAMPLER,
		 std::vector<DescriptorImageInfo>{{_linearSampler,
										   imageViewCube,
										   ImageLayout::SHADER_READ_ONLY_OPTIMAL}}},
		{_descriptorSets[1],
		 0,
		 0,
		 DescriptorType::UNIFORM_BUFFER,
		 std::vector<DescriptorBufferInfo>{{uboBuffer,
											0,
											sizeof(UBOSkybox)}}},
		{_descriptorSets[2],
		 TEXTURE_BINDING_REFLECTION_ENV_CUBEMAP,
		 0,
		 DescriptorType::COMBINED_IMAGE_SAMPLER,
		 std::vector<DescriptorImageInfo>{{_linearSampler,
										   imageViewCube,
										   ImageLayout::SHADER_READ_ONLY_OPTIMAL}}},
		{_descriptorSets[2],
		 TEXTURE_BINDING_IRRADIANCE_ENV_CUBEMAP,
		 0,
		 DescriptorType::COMBINED_IMAGE_SAMPLER,
		 std::vector<DescriptorImageInfo>{{_linearSampler,
										   imageViewCube,
										   ImageLayout::SHADER_READ_ONLY_OPTIMAL}}},
	};
	g_App->getDevice()->UpdateDescriptorSets(writes, {});
	//=========================================================
	//create shaders

	std::string vertShaderPath = buildShaderPath(_vertShaderName.c_str(), g_RendererVersion);
	std::string fragShaderPath = buildShaderPath(_fragShaderName.c_str(), g_RendererVersion);

	auto vertSource = readFile(vertShaderPath.c_str());
	auto fragSource = readFile(fragShaderPath.c_str());

	Shader *cubemapVertShader = g_App->getDevice()->Create(ShaderCreateInfo{vertSource.size(), vertSource.data()});
	Shader *cubemapFragShader = g_App->getDevice()->Create(ShaderCreateInfo{fragSource.size(), fragSource.data()});

	//cubemap _pipeline
	std::vector<PipelineShaderStageCreateInfo> cubemapShaderStageCreateInfos{
		PipelineShaderStageCreateInfo{
			ShaderStageFlagBits::VERTEX_BIT,
			cubemapVertShader,
			"main",
		},
		PipelineShaderStageCreateInfo{
			ShaderStageFlagBits::FRAGMENT_BIT,
			cubemapFragShader,
			"main",
		},
	};
	PipelineTessellationStateCreateInfo tessellationState{};
	PipelineRasterizationStateCreateInfo rasterizationState{
		false,
		false,
		PolygonMode::FILL,
		CullMode::BACK,
		FrontFace::COUNTER_CLOCKWISE,
		false,
	};
	rasterizationState.lineWidth = 1.f;

	PipelineMultisampleStateCreateInfo multisampleState{
		SAMPLE_COUNT, true, //sample shading
		1.f,				//min sample shading
		nullptr,			//mask
		false,				//alpha to coverage
		false				//alpha to one
	};
	PipelineDepthStencilStateCreateInfo depthStencilState{
		.depthTestEnable = true,
		.depthWriteEnable = true,
		.depthCompareOp = CompareOp::LESS_OR_EQUAL,
	};

	PipelineColorBlendAttachmentState colorBlendAttachmentstate{
		false,
		BlendFactor::SRC_ALPHA,
		BlendFactor::ONE_MINUS_SRC_ALPHA,
		BlendOp::ADD,
		BlendFactor::SRC_ALPHA,
		BlendFactor::ONE_MINUS_SRC_ALPHA,
		BlendOp::ADD,
		ColorComponentFlagBits::R_BIT | ColorComponentFlagBits::G_BIT | ColorComponentFlagBits::B_BIT | ColorComponentFlagBits::A_BIT};
	PipelineColorBlendStateCreateInfo colorBlendState{
		false,
		LogicOp::COPY,
		{colorBlendAttachmentstate},
	};
	PipelineDynamicStateCreateInfo dynamicStateInfo{
		{
			DynamicState::VIEWPORT,
			DynamicState::SCISSOR,
		}};
	VertexInputStateCreateInfo vertexInputStateCreateInfo{};
	_pipeline = g_App->getDevice()->Create(GraphicsPipelineCreateInfo{
	    cubemapShaderStageCreateInfos,
	    {},
	    PipelineInputAssemblyStateCreateInfo{PrimitiveTopology::TRIANGLE_LIST},
	    {{{}}, {{}}},
	    tessellationState,
	    rasterizationState,
	    multisampleState,
	    depthStencilState,
	    colorBlendState,
	    dynamicStateInfo,
	    _pipelineLayout,
		g_App->getDefaultRenderPass(),
		g_App->getDefaultSubpass()});
	//create images
}
MaterialSkybox::~MaterialSkybox()
{
	g_App->getDevice()->Destroy(sampler);
	g_App->getDevice()->Destroy(uboBuffer);
	g_App->getDevice()->Destroy(image2D);
	g_App->getDevice()->Destroy(imageView2D);
	g_App->getDevice()->Destroy(imageCube);
	g_App->getDevice()->Destroy(imageViewCube);
	g_App->getDevice()->Destroy(irradianceImageCube);
	g_App->getDevice()->Destroy(irradianceImageViewCube);
	g_App->getDevice()->Destroy(reflectionEnvMap);
	g_App->getDevice()->Destroy(reflectionEnvMapView);
}
void MaterialSkybox::setParameters(const std::array<float, 4> tintColor, float exposure)
{
	memcpy(&uboSkybox.tintColor[0], tintColor.data(), sizeof(float) * 4);
	uboSkybox.exposure = exposure;
	void *data;
	uboBuffer->MapMemory(0, sizeof(UBOSkybox), &data);
	memcpy(data, &uboSkybox, sizeof(UBOSkybox));
	uboBuffer->UnMapMemory();
}
//=====================================
MaterialSkyboxProcedural::MaterialSkyboxProcedural()
{
	_materialType = MATERIAL_SKYBOX_PROCEDURAL;
	image2D = generateEqirectangularProceduralSkybox(g_App->getDevice(), {0, -1, 0}, 1024, 512, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
	imageView2D = g_App->getDevice()->Create(ImageViewCreateInfo{
		image2D,
		ImageViewType::TYPE_2D,
		ShitFormat::RGBA32_SFLOAT,
		{},
		{0, image2D->GetCreateInfoPtr()->mipLevels, 0, 1}});

//	takeScreenshot(g_App->getDevice(), image2D, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
	//equirectangular2cube shader
	convert2DToCubemap(
		g_App->getDevice(),
		image2D,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		imageCube,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL);
}
//==================================================
MaterialSkyboxEquirectangular::MaterialSkyboxEquirectangular(const char *pImagePath)
{
	_materialType = MATERIAL_SKYBOX_EQUIRECTANGULAR;
	setImage(pImagePath);
}
void MaterialSkyboxEquirectangular::setImage(const char *pImagePath)
{
	g_App->getDevice()->Destroy(image2D);
	g_App->getDevice()->Destroy(imageView2D);

	if (pImagePath)
	{
		//load image
		int width, height, components;
		auto pixels = loadImage(pImagePath, width, height, components, 4); //force load an alpha channel,even not exist
		//create equirectangular image
		image2D = g_App->getDevice()->Create(
			ImageCreateInfo{
				{},
				ImageType::TYPE_2D,
				ShitFormat::RGBA32_SFLOAT, //TODO: check if format support storage image
				{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
				0,
				1,
				SampleCountFlagBits::BIT_1,
				ImageTiling::OPTIMAL,
				ImageUsageFlagBits::SAMPLED_BIT | ImageUsageFlagBits::TRANSFER_SRC_BIT,
				MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
				ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			},
			pixels);
		freeImage(pixels);
		image2D->GenerateMipmaps(Filter::LINEAR, ImageLayout::SHADER_READ_ONLY_OPTIMAL, ImageLayout::SHADER_READ_ONLY_OPTIMAL);

		imageView2D = g_App->getDevice()->Create(ImageViewCreateInfo{
			image2D,
			ImageViewType::TYPE_2D,
			ShitFormat::RGBA32_SFLOAT,
			{},
			{0, image2D->GetCreateInfoPtr()->mipLevels, 0, 1}});

		//equirectangular2cube shader
		convert2DToCubemap(
			g_App->getDevice(),
			image2D,
			ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			imageCube,
			ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			ImageLayout::SHADER_READ_ONLY_OPTIMAL);
	}
}

MaterialAxis::MaterialAxis()
{
	_materialType = MATERIAL_AXIS;
	_name = "axis material";

	_vertShaderName = "axis.vert.spv";
	_fragShaderName = "axis.frag.spv";

	std::string axisVertShaderPath = buildShaderPath(_vertShaderName.c_str(), g_RendererVersion);
	std::string axisFragShaderPath = buildShaderPath(_fragShaderName.c_str(), g_RendererVersion);

	auto vertSource = readFile(axisVertShaderPath.c_str());
	auto fragSource = readFile(axisFragShaderPath.c_str());

	Shader *axisVertShader = g_App->getDevice()->Create(ShaderCreateInfo{vertSource.size(), vertSource.data()});
	Shader *axisFragShader = g_App->getDevice()->Create(ShaderCreateInfo{fragSource.size(), fragSource.data()});
	//=============================================================

	//descriptor sets
	std::vector<DescriptorSetLayoutBinding> PVBindings{
		DescriptorSetLayoutBinding{UNIFORM_BINDING_FRAME, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT | ShaderStageFlagBits::FRAGMENT_BIT},
	};
	auto descriptorSetLayout = {g_App->getDevice()->Create(DescriptorSetLayoutCreateInfo{PVBindings})};

	//create pipeline layout
	_pipelineLayout = g_App->getDevice()->Create(PipelineLayoutCreateInfo{descriptorSetLayout});

	//create pipeline
	std::vector<PipelineShaderStageCreateInfo> axisShaderStageCreateInfos{
		PipelineShaderStageCreateInfo{
			ShaderStageFlagBits::VERTEX_BIT,
			axisVertShader,
			"main",
		},
		PipelineShaderStageCreateInfo{
			ShaderStageFlagBits::FRAGMENT_BIT,
			axisFragShader,
			"main",
		},
	};
	PipelineInputAssemblyStateCreateInfo inputAssemblyState{
		PrimitiveTopology::LINE_LIST,
	};
	PipelineTessellationStateCreateInfo tessellationState{};
	PipelineRasterizationStateCreateInfo rasterizationState{
		false,
		false,
		PolygonMode::LINE,
		CullMode::BACK,
		FrontFace::COUNTER_CLOCKWISE,
		false,
	};
	rasterizationState.lineWidth = 1.f;

	PipelineMultisampleStateCreateInfo multisampleState{
		SAMPLE_COUNT,
		true,	 //sample shading
		1.f,	 //min sample shading
		nullptr, //mask
		false,	 //alpha to coverage
		false	 //alpha to one
	};
	PipelineDepthStencilStateCreateInfo depthStencilState{
		.depthTestEnable = true,
		.depthWriteEnable = true,
		.depthCompareOp = CompareOp::LESS_OR_EQUAL,
	};

	PipelineColorBlendAttachmentState colorBlendAttachmentstate{
		false,
		BlendFactor::SRC_ALPHA,
		BlendFactor::ONE_MINUS_SRC_ALPHA,
		BlendOp::ADD,
		BlendFactor::SRC_ALPHA,
		BlendFactor::ONE_MINUS_SRC_ALPHA,
		BlendOp::ADD,
		ColorComponentFlagBits::R_BIT | ColorComponentFlagBits::G_BIT | ColorComponentFlagBits::B_BIT | ColorComponentFlagBits::A_BIT};
	PipelineColorBlendStateCreateInfo colorBlendState{
		false,
		LogicOp::COPY,
		{colorBlendAttachmentstate},
	};
	PipelineDynamicStateCreateInfo dynamicStateInfo{
		{
			DynamicState::VIEWPORT,
			DynamicState::SCISSOR,
		}};

	VertexInputStateCreateInfo vertexInputStateCreateInfo{};
	_pipeline = g_App->getDevice()->Create(GraphicsPipelineCreateInfo{
	    axisShaderStageCreateInfos,
	    {},
	    PipelineInputAssemblyStateCreateInfo{PrimitiveTopology::LINE_LIST},
	    {{{}}, {{}}},
	    tessellationState,
	    rasterizationState,
	    multisampleState,
	    depthStencilState,
	    colorBlendState,
	    dynamicStateInfo,
	    _pipelineLayout,
		g_App->getDefaultRenderPass(),
		g_App->getDefaultSubpass()});
}