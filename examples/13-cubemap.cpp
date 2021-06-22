#include "appbase.hpp"

static const char *testModelPath;

class HelloScene : public Scene
{

	std::unique_ptr<Material> *ppMaterial;

	void createMaterials()
	{
		ppMaterial = AddMaterial(MATERIAL_CUSTOM);
		auto material = (*ppMaterial).get();
		material->_name = "cubemap";

		std::vector<DescriptorSetLayoutBinding> frameBindings{
			DescriptorSetLayoutBinding{UNIFORM_BINDING_FRAME, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT | ShaderStageFlagBits::FRAGMENT_BIT}, //UBO PV
		};
		std::vector<DescriptorSetLayoutBinding> nodeBindings{
			DescriptorSetLayoutBinding{UNIFORM_BINDING_NODE, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT}, //UBO M
		};
		std::vector<DescriptorSetLayoutBinding> materialBindings{
			DescriptorSetLayoutBinding{TEXTURE_BINDING_ALBEDO, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT},			  //albedo
			DescriptorSetLayoutBinding{TEXTURE_BINDING_NORMAL, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT},			  //normal
			DescriptorSetLayoutBinding{TEXTURE_BINDING_METALLIC_ROUGHNESS, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT}, //metallic and roughness
			DescriptorSetLayoutBinding{TEXTURE_BINDING_OCCLUSION, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT},		  //occlusion
			DescriptorSetLayoutBinding{TEXTURE_BINDING_EMISSION, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT},			  //emission
			DescriptorSetLayoutBinding{TEXTURE_BINDING_TRANSPARENCY, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT},		  //transparency
			DescriptorSetLayoutBinding{UNIFORM_BINDING_MATERIAL, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::FRAGMENT_BIT},					  //UBO material
		};
		std::vector<DescriptorSetLayoutBinding> jointMatrixBindings{
			DescriptorSetLayoutBinding{UNIFORM_BINDING_JOINTMATRIX, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT},
		};
		std::vector<DescriptorSetLayoutBinding> otherBindings{
			DescriptorSetLayoutBinding{TEXTURE_BINDING_REFLECTION_ENV_CUBEMAP, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT},
			//	DescriptorSetLayoutBinding{TEXTURE_BINDING_IRRADIANCE_ENV_CUBEMAP, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT},
		};
		auto device = g_App->getDevice();
		std::vector<DescriptorSetLayout *> setLayouts{
			device->Create(DescriptorSetLayoutCreateInfo{frameBindings}),
			device->Create(DescriptorSetLayoutCreateInfo{nodeBindings}),
			device->Create(DescriptorSetLayoutCreateInfo{materialBindings}),
			device->Create(DescriptorSetLayoutCreateInfo{jointMatrixBindings}),
			device->Create(DescriptorSetLayoutCreateInfo{otherBindings}),
		};
		//pipeline layout
		material->_pipelineLayout = device->Create(PipelineLayoutCreateInfo{setLayouts});

		std::vector<DescriptorPoolSize> poolSizes;
		for (auto &&e : otherBindings)
			poolSizes.emplace_back(e.descriptorType, e.descriptorCount);

		//create descriptor pool
		DescriptorPoolCreateInfo descriptorPoolCreateInfo{1, poolSizes};
		auto descriptorPool = device->Create(descriptorPoolCreateInfo);

		//deacriptor sets
		descriptorPool->Allocate(DescriptorSetAllocateInfo{{setLayouts[DESCRIPTORSET_ID_OTHER]}}, material->_descriptorSets);
		material->_descriptorSetIDs = {DESCRIPTORSET_ID_OTHER};

		//update descriptor sets
		std::vector<WriteDescriptorSet> writes{
			{material->_descriptorSets[0],
			 TEXTURE_BINDING_REFLECTION_ENV_CUBEMAP,
			 0,
			 DescriptorType::COMBINED_IMAGE_SAMPLER,
			 std::vector<DescriptorImageInfo>{
				 {material->_linearSampler,
				  static_cast<MaterialSkybox *>(_skyboxMaterial->get())->imageViewCube,
				  ImageLayout::SHADER_READ_ONLY_OPTIMAL},
			 }},
		};
		device->UpdateDescriptorSets(writes, {});

		//=================================
		//shaders
		material->_vertShaderName = "standard.vert.spv";
		material->_fragShaderName = "13.frag.spv";

		auto vertShaderPath = buildShaderPath(material->_vertShaderName.c_str(), g_RendererVersion);
		auto fragShaderPath = buildShaderPath(material->_fragShaderName.c_str(), g_RendererVersion);

		auto vertCode = readFile(vertShaderPath.c_str());
		auto fragCode = readFile(fragShaderPath.c_str());

		auto vertShader = device->Create(ShaderCreateInfo{vertCode.size(), vertCode.data()});
		auto fragShader = device->Create(ShaderCreateInfo{fragCode.size(), fragCode.data()});

		//pipeline
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
			material->_pipelineLayout,
			g_App->getDefaultRenderPass(),
			g_App->getDefaultSubpass()};
		material->_pipeline = g_App->getDevice()->Create(pipelineCreateInfo);
	}

public:
	HelloScene() {}
	void Prepare() override
	{
		auto sphere = CreateModelView(ModelType::SPHERE, nullptr, "sphere");
		//auto cube = CreateModelView(ModelType::CUBE, nullptr, "cube");
		//auto rock = CreateModelView(ModelType::ROCK, nullptr, "rock");
		//auto teapot = CreateModelView(ModelType::TEAPOT, nullptr, "teapot");
		//auto torusknot = CreateModelView(ModelType::TORUSKNOT, nullptr, "torusknot");
		//auto cylinder = CreateModelView(ModelType::TUNNEL_CYLINDER, nullptr, "cylinder");

		createMaterials();
		ApplyMaterialToNode(ppMaterial, sphere);
	}
};

CREATE_SCENE(HelloScene)