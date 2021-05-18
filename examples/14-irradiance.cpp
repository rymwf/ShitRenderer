//TODO: use product sampling, not done yet
#include "appbase.hpp"

#define MODEL_SIZE 1.

const char *vertShaderName = "14.vert.spv";
const char *fragShaderName = "14.frag.spv";

static const char *testModelPath = ASSET_PATH "models/geosphere.gltf";
//static const char *testModelPath = ASSET_PATH "models/teapot.gltf";

int animationIndex = 0;

class HelloApp : public AppBase
{
	Model *testModel;
	Model *tempModel{};
	std::vector<ModelView *> testModelViews;

	PipelineLayout *pipelineLayout;
	Pipeline *pipeline;
	std::vector<DescriptorSet *> descriptorSets;

	std::vector<CommandBuffer *> commandBuffers;

	Image *irradianceImageCube;
	ImageView *irradianceImageViewCube;

public:
	HelloApp(uint32_t width, uint32_t height) : AppBase(width, height) {}

	void processEvent(const Event &ev) override
	{
		std::visit(overloaded{
					   [&](const DropEvent &value) {
						   LOG(value.paths[0]);
						   tempModel = scene.LoadModel(value.paths[0]);
					   },
					   [](auto &&) {}},
				   ev.value);
	}
	void createDescriptorSets()
	{
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
		std::vector<DescriptorSetLayoutBinding> cubemapBindings{
			DescriptorSetLayoutBinding{0, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT},
		};
		std::vector<DescriptorSetLayoutBinding> otherBindings{
			DescriptorSetLayoutBinding{TEXTURE_BINDING_REFLECTION_ENV_CUBEMAP, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT}, //transparency
		};
		std::vector<DescriptorSetLayout *> setLayouts(5);
		setLayouts[DESCRIPTORSET_ID_FRAME] = (device->Create(DescriptorSetLayoutCreateInfo{frameBindings}));
		setLayouts[DESCRIPTORSET_ID_NODE] = (device->Create(DescriptorSetLayoutCreateInfo{nodeBindings}));
		setLayouts[DESCRIPTORSET_ID_MATERIAL] = (device->Create(DescriptorSetLayoutCreateInfo{materialBindings}));
		setLayouts[DESCRIPTORSET_ID_JOINTMATRIX] = (device->Create(DescriptorSetLayoutCreateInfo{jointMatrixBindings}));
		setLayouts[DESCRIPTORSET_ID_OTHER] = (device->Create(DescriptorSetLayoutCreateInfo{otherBindings}));

		pipelineLayout = device->Create(PipelineLayoutCreateInfo{setLayouts});

		std::vector<DescriptorPoolSize> poolsizes{
			{DescriptorType::COMBINED_IMAGE_SAMPLER, 1},
		};

		DescriptorPool *descriptorPool = device->Create(DescriptorPoolCreateInfo{1, poolsizes});
		DescriptorSetAllocateInfo allocInfo{{setLayouts[DESCRIPTORSET_ID_OTHER]}};
		descriptorPool->Allocate(allocInfo, descriptorSets);

		//update descriptor sets
		std::vector<WriteDescriptorSet> writes{
			{descriptorSets[0],
			 TEXTURE_BINDING_REFLECTION_ENV_CUBEMAP,
			 0,
			 DescriptorType::COMBINED_IMAGE_SAMPLER,
			 std::vector<DescriptorImageInfo>{
				 {linearSampler,
				  irradianceImageViewCube,
				  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}}};
		device->UpdateDescriptorSets(writes, {});
	}
	void createPipeline()
	{
		//load shaders
		std::string vertShaderPath = buildShaderPath(vertShaderName, rendererVersion);
		std::string fragShaderPath = buildShaderPath(fragShaderName, rendererVersion);

		auto vertSource=readFile(vertShaderPath.c_str());
		auto fragSource=readFile(fragShaderPath.c_str());

		Shader *vertShader = device->Create(ShaderCreateInfo{vertSource.size(), vertSource.data()});
		Shader *fragShader = device->Create(ShaderCreateInfo{fragSource.size(), fragSource.data()});

		//create pipeline

		uint32_t jointMatrixNum = testModel->GetJointMatrixMaxNum();
		std::vector<uint32_t> constantIDs{CONSTANT_ID_JOINTNUM};
		std::vector<uint32_t> constantValues{jointMatrixNum};

		std::vector<PipelineShaderStageCreateInfo>
			shaderStageCreateInfos{
				PipelineShaderStageCreateInfo{
					ShaderStageFlagBits::VERTEX_BIT,
					vertShader,
					"main",
					{constantIDs, constantValues}},
				PipelineShaderStageCreateInfo{
					ShaderStageFlagBits::FRAGMENT_BIT,
					fragShader,
					"main",
				},
			};
		PipelineInputAssemblyStateCreateInfo inputAssemblyState{
			PrimitiveTopology::TRIANGLE_LIST,
		};
		PipelineViewportStateCreateInfo viewportStateCreateInfo{
			{{0, 0, static_cast<float>(swapchain->GetCreateInfoPtr()->imageExtent.width), static_cast<float>(swapchain->GetCreateInfoPtr()->imageExtent.height), 0, 1}},
			{{{0, 0}, swapchain->GetCreateInfoPtr()->imageExtent}}};
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

		GraphicsPipelineCreateInfo pipelineCreateInfo{
			shaderStageCreateInfos,
			*testModel->GetVertexInputStateCreateInfoPtr(),
			inputAssemblyState,
			viewportStateCreateInfo,
			tessellationState,
			rasterizationState,
			multisampleState,
			depthStencilState,
			colorBlendState,
			dynamicStateInfo,
			pipelineLayout,
			defaultRenderPass,
			0};

		pipeline = device->Create(pipelineCreateInfo);
	};
	void createCommandBuffers()
	{
		uint32_t count = swapchainImages.size();
		defaultShortLiveCommandPool->CreateCommandBuffers(CommandBufferCreateInfo{CommandBufferLevel::SECONDARY, count},
														  commandBuffers);

		for (size_t i = 0; i < count; ++i)
		{
			addSecondaryCommandBuffers(i, {commandBuffers[i]});
		}
	}
	void recordCommandBuffers(uint32_t imageIndex)
	{
		commandBuffers[imageIndex]->Begin({CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT,
										   CommandBufferInheritanceInfo{
											   defaultRenderPass,
											   0,
											   framebuffers[imageIndex]}});

		Viewport viewport{0, 0, swapchain->GetCreateInfoPtr()->imageExtent.width, swapchain->GetCreateInfoPtr()->imageExtent.height, 0, 1};
		commandBuffers[imageIndex]->SetViewport(SetViewPortInfo{0, 1, &viewport});
		Rect2D scissor{{}, swapchain->GetCreateInfoPtr()->imageExtent};
		commandBuffers[imageIndex]->SetScissor(SetScissorInfo{0, 1, &scissor});

		commandBuffers[imageIndex]->BindDescriptorSets(
			BindDescriptorSetsInfo{
				PipelineBindPoint::GRAPHICS,
				pipelineLayout,
				DESCRIPTORSET_ID_FRAME,
				1,
				&defaultDescriptorSetsUboFrame[imageIndex]});
		commandBuffers[imageIndex]->BindDescriptorSets(
			BindDescriptorSetsInfo{
				PipelineBindPoint::GRAPHICS,
				pipelineLayout,
				DESCRIPTORSET_ID_OTHER,
				1,
				&descriptorSets[0]});
		commandBuffers[imageIndex]->BindPipeline({PipelineBindPoint::GRAPHICS, pipeline});
		testModel->DrawModel(device, commandBuffers[imageIndex], imageIndex);
		commandBuffers[imageIndex]->End();
	}
	void setScene()
	{
		testModel->DownloadModel(device, pipelineLayout, swapchainImages.size());

		auto boundingBox = testModel->GetModelBoundingVolumePtr()->box;
		glm::dvec3 scaleFactor{1. / (boundingBox.aabb.maxValue.y - boundingBox.aabb.center.y)};
		glm::dvec3 translation = glm::dvec3(0., -boundingBox.aabb.minValue.y, 0.) * scaleFactor;

		//////create ModelView
		testModelViews[0]->pModel = testModel;
		testModelViews[0]->translation = translation;
		testModelViews[0]->scale = scaleFactor;

		testModelViews[1]->pModel = testModel;
		testModelViews[1]->translation = translation + glm::dvec3(3, 0, 0);
		testModelViews[1]->scale = scaleFactor;

		mainCamera->translation = translation;
		rootNode->Update();
		testModel->AssignInstances(device, {
											   {{}, testModelViews[0]->globalMatrix},
											   {{}, testModelViews[1]->globalMatrix},
										   });

		createPipeline();
	}
	void prepare() override
	{
		createIrradianceMap();

		createUniformBuffers();
		createDescriptorSets();

		//load assets
		testModel = scene.LoadModel(testModelPath);
		testModelViews.resize(2);
		testModelViews[0] = scene.CreateNode<ModelView>(rootNode);
		testModelViews[1] = scene.CreateNode<ModelView>(rootNode);
		setScene();
		createCommandBuffers();
	}
	void createUniformBuffers()
	{
	}
	void drawFrame(uint32_t imageIndex, std::vector<CommandBuffer *> &primaryCommandBuffers) override
	{
		//========================================
		//std::this_thread::sleep_for(std::chrono::milliseconds(5));
		if (tempModel)
		{
			presentQueue->WaitIdle();
			//destroy old model
			scene.Destroy(testModel);
			device->Destroy(pipeline);

			//download new model
			testModel = tempModel;
			setScene();
			tempModel = nullptr;
		}
		if (testModel->GetAnimationNum() > 0)
		{
			//update model animation
			static auto animationStartTime = curTime;
			static float cycleTime = -1; //ms
			auto elapsedTime = std::chrono::duration<float, std::chrono::seconds::period>(curTime - animationStartTime).count();
			testModel->UpdateAnimation(animationIndex, elapsedTime, imageIndex);
		}
		recordCommandBuffers(imageIndex);
		drawImGui();
	}
	void createIrradianceMap()
	{
		irradianceImageCube = device->Create(
			ImageCreateInfo{
				.flags = ImageCreateFlagBits::CUBE_COMPATIBLE_BIT,
				.imageType = ImageType::TYPE_2D,
				.format = ShitFormat::RGBA32_SFLOAT, //TODO: check if format support storage image
				.extent = {IRRADIANCE_MAP_WIDTH, IRRADIANCE_MAP_WIDTH, 1},
				.mipLevels = 0,
				.arrayLayers = 6,
				.samples = SampleCountFlagBits::BIT_1,
				.tiling = ImageTiling::OPTIMAL,
				.usageFlags = ImageUsageFlagBits::SAMPLED_BIT | ImageUsageFlagBits::STORAGE_BIT,
				.memoryPropertyFlags = MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			},
			nullptr);

		irradianceImageViewCube = device->Create(
			ImageViewCreateInfo{
				irradianceImageCube,
				ImageViewType::TYPE_CUBE,
				ShitFormat::RGBA32_SFLOAT,
				{},
				{0, irradianceImageCube->GetCreateInfoPtr()->mipLevels, 0, 6},
			});
		generateIrradianceMap(
			device,
			skyboxImage2D,
			ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			irradianceImageCube,
			ImageLayout::UNDEFINED,
			ImageLayout::SHADER_READ_ONLY_OPTIMAL);
	}
	void drawImGui() override
	{
	}
};

CREATE_APP(HelloApp)
