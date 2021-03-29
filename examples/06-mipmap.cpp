#include "common.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

uint32_t WIDTH = 800, HEIGHT = 600;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const char *vertShaderName = "06.vert.spv";
const char *fragShaderName = "06.frag.spv";
const char *testImagePath = IMAGE_PATH "Lenna_test.jpg";

std::string vertShaderPath;
std::string fragShaderPath;

std::vector<Vertex> vertices{
	{{-0.5, -0.5, 0}, {1, 0, 0}, {0, 0}},
	{{-0.5, 0.5, 0}, {0, 0, 1}, {0, 1}},
	{{0.5, -0.5, 0}, {0, 1, 0}, {1, 0}},
	{{0.5, 0.5, 0}, {1, 1, 0}, {1, 1}},
};
std::vector<uint16_t> indices{0, 1, 2, 3};

class Hello
{
	RenderSystem *renderSystem;
	ShitWindow *window;
	Device *device;

	std::optional<QueueFamilyIndex> graphicsQueueFamilyIndex;
	QueueFamilyIndex presentQueueFamilyIndex;
	std::optional<QueueFamilyIndex> transferQueueFamilyIndex;

	SwapchainCreateInfo swapchainCreateInfo;
	Swapchain *swapchain;
	std::vector<ImageView *> swapchainImageViews;
	std::vector<Image *> swapchainImages;
	std::vector<Framebuffer *> framebuffers;

	CommandPool *commandPool;
	std::vector<CommandBuffer *> commandBuffers;
	Queue *graphicsQueue;
	Queue *presentQueue;

	Semaphore *imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	Semaphore *renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
	Fence *inFlightFences[MAX_FRAMES_IN_FLIGHT];
	//std::vector<Fence *> inFlightImages;

	Shader *vertShader;
	Shader *fragShader;

	DescriptorPool *descriptorPool;
	std::vector<DescriptorSetLayout *> descriptorSetLayouts;
	std::vector<DescriptorSet *> descriptorSets;

	PipelineLayout *pipelineLayout;
	RenderPass *renderPass;
	Pipeline *pipeline;

	Buffer *drawIndirectCmdBuffer;
	Buffer *drawCountBuffer;
	Buffer *drawIndexedIndirectCmdBuffer;
	Buffer *indexBuffer;
	Buffer *vertexBuffer;

	Image *testImage;
	ImageView *testImageView;
	Sampler *sampler;

public:
	void initRenderSystem()
	{
		RenderSystemCreateInfo renderSystemCreateInfo{
			.version = rendererVersion,
			.flags = RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT,
		};

		renderSystem = LoadRenderSystem(renderSystemCreateInfo);
		//1. create window
		WindowCreateInfo windowCreateInfo{
			__FILE__,
			{{SHIT_DEFAULT_WINDOW_X, SHIT_DEFAULT_WINDOW_Y},
			 {SHIT_DEFAULT_WINDOW_WIDTH, SHIT_DEFAULT_WINDOW_HEIGHT}},
			std::make_shared<std::function<void(const Event &)>>(std::bind(&Hello::ProcessEvent, this, std::placeholders::_1))};
		window = renderSystem->CreateRenderWindow(windowCreateInfo);
		//1.5 choose phyiscal device
		//2. create device of a physical device
		DeviceCreateInfo deviceCreateInfo{};
		if ((rendererVersion & RendererVersion::TypeBitmask) == RendererVersion::GL)
			deviceCreateInfo = {window};

		device = renderSystem->CreateDevice(deviceCreateInfo);

		//3
		graphicsQueueFamilyIndex = device->GetQueueFamilyIndexByFlag(QueueFlagBits::GRAPHICS_BIT, {});
		if (!graphicsQueueFamilyIndex.has_value())
			THROW("failed to find a graphic queue");

		LOG_VAR(graphicsQueueFamilyIndex->index);
		LOG_VAR(graphicsQueueFamilyIndex->count);

		//4. command pool
		CommandPoolCreateInfo commandPoolCreateInfo{
			{},
			graphicsQueueFamilyIndex->index};
		commandPool = device->Create(commandPoolCreateInfo);

		//5
		QueueCreateInfo queueCreateInfo{
			graphicsQueueFamilyIndex->index,
			0,
		};
		graphicsQueue = device->Create(queueCreateInfo);

		createSwapchains();

		createShaders();
		createDescriptorSets();
		createRenderPasses();
		createPipeline();
		createFramebuffers();

		createDrawCommandBuffers();
		createIndexBuffer();
		createVertexBuffer();
		createImages();
		createSamplers();
		createSyncObjects();
		updateDescriptorSets();

		createCommandBuffers();

		transferQueueFamilyIndex = device->GetQueueFamilyIndexByFlag(QueueFlagBits::TRANSFER_BIT, {graphicsQueueFamilyIndex->index, presentQueueFamilyIndex.index});
		if (!transferQueueFamilyIndex.has_value())
			THROW("failed to find a transfer queue");

		LOG_VAR(transferQueueFamilyIndex->index);
		LOG_VAR(transferQueueFamilyIndex->count);
	}
	/**
	 * @brief process window event, do not write render code here
	 * 
	 * @param ev 
	 */
	void ProcessEvent(const Event &ev)
	{
		std::visit(overloaded{
					   [&ev](const KeyEvent &value) {
						   if (value.keyCode == KeyCode::KEY_ESCAPE)
							   ev.pWindow->Close();
					   },
					   [](auto &&) {},
					   [&ev]([[maybe_unused]] const WindowResizeEvent &value) {
					   },
				   },
				   ev.value);
	}

	void mainLoop()
	{
		while (window->PollEvents())
		{
			drawFrame();
		}
		presentQueue->WaitIdle();
	}
	void cleanUp()
	{
	}
	void run()
	{
		initRenderSystem();
		mainLoop();
		cleanUp();
	}
	void cleanupSwapchain()
	{
		for (auto &&e : swapchainImageViews)
			device->Destroy(e);
		device->Destroy(renderPass);
		for (auto &&e : framebuffers)
			device->Destroy(e);
		device->Destroy(pipelineLayout);
		device->Destroy(pipeline);
		for (auto &&e : commandBuffers)
			commandPool->DestroyCommandBuffer(e);
		device->Destroy(swapchain);
	}
	void recreateSwapchain()
	{
		cleanupSwapchain();
		createSwapchains();
		createRenderPasses();
		createFramebuffers();
		createPipeline();
		createCommandBuffers();
	}
	void drawFrame()
	{
		static uint32_t currentFrame{};
		inFlightFences[currentFrame]->WaitFor(UINT64_MAX);

		uint32_t imageIndex{};
		GetNextImageInfo nextImageInfo{
			UINT64_MAX,
			imageAvailableSemaphores[currentFrame]};
		auto ret = swapchain->GetNextImage(nextImageInfo, imageIndex);
		if (ret == Result::SHIT_ERROR_OUT_OF_DATE)
		{
			presentQueue->WaitIdle();
			recreateSwapchain();
			return;
		}
		else if (ret != Result::SUCCESS)
		{
			THROW("failed to get next image");
		}

		inFlightFences[currentFrame]->Reset();

		std::vector<SubmitInfo> submitInfos{
			{{imageAvailableSemaphores[currentFrame]},
			 {commandBuffers[imageIndex]},
			 {renderFinishedSemaphores[currentFrame]}}};
		graphicsQueue->Submit(submitInfos, inFlightFences[currentFrame]);

		PresentInfo presentInfo{
			{renderFinishedSemaphores[currentFrame]},
			{swapchain},
			{imageIndex}};
		auto res = presentQueue->Present(presentInfo);
		if (res == Result::SHIT_ERROR_OUT_OF_DATE)
		{
			presentQueue->WaitIdle();
			recreateSwapchain();
		}
		else if (res != Result::SUCCESS)
		{
			THROW("failed to present swapchain image");
		}
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void createShaders()
	{
		vertShaderPath = buildShaderPath(vertShaderName, rendererVersion);
		fragShaderPath = buildShaderPath(fragShaderName, rendererVersion);

		std::string vertCode = readFile(vertShaderPath.c_str());
		std::string fragCode = readFile(fragShaderPath.c_str());

		ShaderCreateInfo vertShaderCreateInfo{vertCode};
		ShaderCreateInfo fragShaderCreateInfo{fragCode};

		vertShader = device->Create(vertShaderCreateInfo);
		fragShader = device->Create(fragShaderCreateInfo);
	}

	void createSwapchains()
	{
		auto swapchainFormat = chooseSwapchainFormat(
			{
				{ShitFormat::BGRA8_SRGB, ColorSpace::SRGB_NONLINEAR},
				{ShitFormat::RGBA8_SRGB, ColorSpace::SRGB_NONLINEAR},
			},
			device,
			window);
		LOG_VAR(static_cast<int>(swapchainFormat.format));
		LOG_VAR(static_cast<int>(swapchainFormat.colorSpace));

		auto presentMode = choosePresentMode({PresentMode::IMMEDIATE, PresentMode::FIFO}, device, window);

		swapchainCreateInfo = SwapchainCreateInfo{
			2,
			swapchainFormat.format,
			swapchainFormat.colorSpace,
			{800, 600},
			1,
			presentMode};
		window->GetFramebufferSize(swapchainCreateInfo.imageExtent.width, swapchainCreateInfo.imageExtent.height);
		while (swapchainCreateInfo.imageExtent.width == 0 && swapchainCreateInfo.imageExtent.height == 0)
		{
			window->WaitEvents();
			window->GetFramebufferSize(swapchainCreateInfo.imageExtent.width, swapchainCreateInfo.imageExtent.height);
		}
		swapchain = device->Create(swapchainCreateInfo, window);
		swapchain->GetImages(swapchainImages);
		ImageViewCreateInfo imageViewCreateInfo{
			nullptr,
			ImageViewType::TYPE_2D,
			swapchainCreateInfo.format,
			{},
			{0, 1, 0, 1}};
		auto swapchainImageCount = swapchainImages.size();
		swapchainImageViews.resize(swapchainImageCount);
		while (swapchainImageCount-- > 0)
		{
			imageViewCreateInfo.pImage = swapchainImages[swapchainImageCount];
			swapchainImageViews[swapchainImageCount] = device->Create(imageViewCreateInfo);
		}
		presentQueueFamilyIndex = swapchain->GetPresentQueueFamilyIndex();

		LOG_VAR(presentQueueFamilyIndex.index);
		LOG_VAR(presentQueueFamilyIndex.count);

		QueueCreateInfo queueCreateInfo{
			presentQueueFamilyIndex.index,
			0,
		};
		presentQueue = device->Create(queueCreateInfo);
	}
	void createDescriptorSets()
	{
		std::vector<DescriptorSetLayoutBinding> bindings{
			DescriptorSetLayoutBinding{
				.binding = 0,
				.descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = ShaderStageFlagBits::FRAGMENT_BIT,
			},
		};
		descriptorSetLayouts.emplace_back(device->Create(DescriptorSetLayoutCreateInfo{bindings}));

		std::vector<DescriptorPoolSize> poolSizes(bindings.size());
		std::transform(bindings.begin(), bindings.end(), poolSizes.begin(), [](auto &&e) {
			return DescriptorPoolSize{e.descriptorType, e.descriptorCount};
		});
		DescriptorPoolCreateInfo descriptorPoolCreateInfo{
			.maxSets = 1,
			.poolSizes = poolSizes};
		descriptorPool = device->Create(descriptorPoolCreateInfo);

		DescriptorSetAllocateInfo allocInfo{descriptorSetLayouts};
		descriptorPool->Allocate(allocInfo, descriptorSets);
	}
	void updateDescriptorSets()
	{
		std::vector<DescriptorImageInfo> imagesInfo{
			DescriptorImageInfo{
				.pSampler = sampler,
				.pImageView = testImageView,
				.imageLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL}};
		std::vector<WriteDescriptorSet> writes{
			WriteDescriptorSet{
				nullptr, //pDstset
				0,		 //dst binding
				0,		 //dstArrayElement
				DescriptorType::COMBINED_IMAGE_SAMPLER,
				imagesInfo,
			}};
		for (auto &&e : descriptorSets)
		{
			writes[0].pDstSet = e;
			device->UpdateDescriptorSets(writes, {});
		}
	}
	void createRenderPasses()
	{
		std::vector<AttachmentDescription> attachmentDescriptions{
			{swapchain->GetCreateInfoPtr()->format,
			 SampleCountFlagBits::BIT_1,
			 AttachmentLoadOp::CLEAR,
			 AttachmentStoreOp::DONT_CARE,
			 AttachmentLoadOp::DONT_CARE,
			 AttachmentStoreOp::DONT_CARE,
			 ImageLayout::UNDEFINED, //inital layout
			 ImageLayout::PRESENT_SRC}};

		std::vector<AttachmentReference> colorAttachments{
			{0, //the index of attachment description
			 ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
		};

		std::vector<SubpassDescription> subPasses{
			SubpassDescription{
				.pipelineBindPoint = PipelineBindPoint::GRAPHICS,
				.colorAttachments = colorAttachments,
			},
		};

		RenderPassCreateInfo renderPassCreateInfo{
			attachmentDescriptions,
			subPasses};

		renderPass = device->Create(renderPassCreateInfo);
	}
	void createPipeline()
	{
		pipelineLayout = device->Create(PipelineLayoutCreateInfo{descriptorSetLayouts});
		std::vector<PipelineShaderStageCreateInfo> shaderStageCreateInfos{
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
		auto vertexBindingDesc = Vertex::getVertexBindingDescription();
		auto vertexAttributeDesc = Vertex::getVertexAttributeDescription(0, 0);
		VertexInputStateCreateInfo vertexInputState{
			{std::move(vertexBindingDesc)},
			std::move(vertexAttributeDesc),
		};
		PipelineInputAssemblyStateCreateInfo inputAssemblyState{
			PrimitiveTopology::TRIANGLE_STRIP,
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
			SampleCountFlagBits::BIT_1,
		};
		PipelineDepthStencilStateCreateInfo depthStencilState{};

		PipelineColorBlendAttachmentState colorBlendAttachmentstate{};
		colorBlendAttachmentstate.colorWriteMask = ColorComponentFlagBits::R_BIT | ColorComponentFlagBits::G_BIT | ColorComponentFlagBits::B_BIT | ColorComponentFlagBits::A_BIT;
		PipelineColorBlendStateCreateInfo colorBlendState{
			false,
			{},
			{std::move(colorBlendAttachmentstate)},
		};

		GraphicsPipelineCreateInfo pipelineCreateInfo{
			std::move(shaderStageCreateInfos),
			std::move(vertexInputState),
			std::move(inputAssemblyState),
			std::move(viewportStateCreateInfo),
			std::move(tessellationState),
			std::move(rasterizationState),
			std::move(multisampleState),
			std::move(depthStencilState),
			std::move(colorBlendState),
			{},
			pipelineLayout,
			renderPass,
			0};

		pipeline = device->Create(pipelineCreateInfo);
	}
	void createFramebuffers()
	{
		FramebufferCreateInfo framebufferCreateInfo{
			renderPass,
			{},
			swapchain->GetCreateInfoPtr()->imageExtent,
			1};
		auto count = swapchainImageViews.size();
		framebuffers.resize(count);
		while (count-- > 0)
		{
			framebufferCreateInfo.attachments = {swapchainImageViews[count]};
			framebuffers[count] = device->Create(framebufferCreateInfo);
		}
	}
	void createCommandBuffers()
	{
		uint32_t count = static_cast<uint32_t>(swapchainImageViews.size());
		CommandBufferCreateInfo cmdBufferCreateInfo{
			CommandBufferLevel::PRIMARY, count};
		commandPool->CreateCommandBuffers(cmdBufferCreateInfo, commandBuffers);

		CommandBufferBeginInfo cmdBufferBeginInfo{};

		std::vector<ClearValue> clearValues{std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.f}};

		RenderPassBeginInfo renderPassBeginInfo{
			renderPass,
			nullptr,
			Rect2D{
				{},
				swapchain->GetCreateInfoPtr()->imageExtent},
			static_cast<uint32_t>(clearValues.size()),
			clearValues.data(),
			SubpassContents::INLINE};

		for (uint32_t i = 0; i < count; ++i)
		{
			renderPassBeginInfo.pFramebuffer = framebuffers[i];
			commandBuffers[i]->Begin(cmdBufferBeginInfo);
			commandBuffers[i]->BeginRenderPass(renderPassBeginInfo);
			commandBuffers[i]->BindPipeline({PipelineBindPoint::GRAPHICS, pipeline});
			uint64_t offsets[] = {0};
			Buffer *buffers[] = {vertexBuffer};
			commandBuffers[i]->BindVertexBuffer(
				BindVertexBufferInfo{
					0,
					1,
					buffers,
					offsets});

			BindDescriptorSetsInfo info{
				PipelineBindPoint::GRAPHICS,
				pipelineLayout,
				0,
				static_cast<uint32_t>(descriptorSets.size()),
				descriptorSets.data()};
			commandBuffers[i]->BindDescriptorSets(info);

			int drawMethod = 4;
			switch (drawMethod)
			{
			case 0:
			default:
			{
				DrawIndirectCommand drawIndirectCmd{4, 1, 0, 0};
				commandBuffers[i]->Draw(drawIndirectCmd);
			}
			break;
			case 1:
			{
				DrawIndirectInfo drawCmdInfo{
					drawIndirectCmdBuffer,
					0,
					1,
					sizeof(DrawIndirectCommand)};
				commandBuffers[i]->DrawIndirect(drawCmdInfo);
			}
			break;
			case 2:
			{
				DrawIndirectCountInfo drawCmdInfo{
					drawIndirectCmdBuffer,
					0,
					drawCountBuffer,
					0,
					1,
					sizeof(DrawIndirectCommand)};
				commandBuffers[i]->DrawIndirectCount(drawCmdInfo);
			}
			break;
			case 3:
			{
				//draw index
				commandBuffers[i]->BindIndexBuffer(BindIndexBufferInfo{
					indexBuffer,
					0,
					IndexType::UINT16});
				DrawIndexedIndirectCommand drawIndexedIndirectCmd{4, 1, 0, 0, 0};
				commandBuffers[i]->DrawIndexed(drawIndexedIndirectCmd);
			}
			break;
			case 4:
			{
				//draw index
				commandBuffers[i]->BindIndexBuffer(BindIndexBufferInfo{
					indexBuffer,
					0,
					IndexType::UINT16});
				commandBuffers[i]->DrawIndexedIndirect(
					DrawIndirectInfo{
						drawIndexedIndirectCmdBuffer,
						0,
						1,
						sizeof(DrawIndexedIndirectCommand)});
			}
			break;
			case 5:
			{
				//draw index
				commandBuffers[i]->BindIndexBuffer(BindIndexBufferInfo{
					indexBuffer,
					0,
					IndexType::UINT16});
				commandBuffers[i]->DrawIndexedIndirectCount(
					DrawIndirectCountInfo{
						drawIndexedIndirectCmdBuffer,
						0,
						drawCountBuffer,
						0,
						1,
						sizeof(DrawIndexedIndirectCommand)});
			}
			break;
			}
			commandBuffers[i]->EndRenderPass();
			commandBuffers[i]->End();
		}
	}

	void createSyncObjects()
	{
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			imageAvailableSemaphores[i] = device->Create(SemaphoreCreateInfo{});
			renderFinishedSemaphores[i] = device->Create(SemaphoreCreateInfo{});
			inFlightFences[i] = device->Create(FenceCreateInfo{FenceCreateFlagBits::SIGNALED_BIT});
		}
		//inFlightImages.resize(swapchainImages.size());
	}
	void createDrawCommandBuffers()
	{
		//create draw indirect command buffer
		std::vector<DrawIndirectCommand> drawIndirectCmds{{4, 1, 0, 0}};
		BufferCreateInfo bufferCreateInfo{
			{},
			sizeof(DrawIndirectCommand) * drawIndirectCmds.size(),
			BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		drawIndirectCmdBuffer = device->Create(bufferCreateInfo, drawIndirectCmds.data());
		bufferCreateInfo.size = sizeof(uint32_t);
		uint32_t drawCount = 1;
		drawCountBuffer = device->Create(bufferCreateInfo, &drawCount);

		std::vector<DrawIndexedIndirectCommand> drawIndexedIndirectCmds{{4, 1, 0, 0, 0}};

		//create draw indexed indirect command buffer
		BufferCreateInfo drawIndexedIndirectCmdBufferCreateInfo{
			{},
			sizeof(DrawIndexedIndirectCommand) * drawIndexedIndirectCmds.size(),
			BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		drawIndexedIndirectCmdBuffer = device->Create(drawIndexedIndirectCmdBufferCreateInfo, drawIndexedIndirectCmds.data());
	}
	void createVertexBuffer()
	{
		BufferCreateInfo createInfo{
			{},
			sizeof(vertices[0]) * vertices.size(),
			BufferUsageFlagBits::VERTEX_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};

		vertexBuffer = device->Create(createInfo, vertices.data());
	}
	void createIndexBuffer()
	{
		BufferCreateInfo indexBufferCreateInfo{
			{},
			sizeof(indices[0]) * indices.size(),
			BufferUsageFlagBits::INDEX_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		indexBuffer = device->Create(indexBufferCreateInfo, indices.data());
	}
	void createImages()
	{
		int width, height, components;
		auto pixels = stbi_load(testImagePath, &width, &height, &components, 4); //force load an alpha channel,even not exist
		if (!pixels)
			throw std::runtime_error("failed to load texture image!");
		auto mipmapLevels = static_cast<uint32_t>(std::floor(std::log2((std::max)(width, height))) + 1);
		//uint32_t mipmapLevels = 5;

		ImageCreateInfo imageCreateInfo{
			//.flags=ImageCreateFlagBits::MUTABLE_FORMAT_BIT,
			.imageType = ImageType::TYPE_2D,
			.format = ShitFormat::RGBA8_SRGB,
			.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
			.mipLevels = mipmapLevels,
			.arrayLayers = 1,
			.samples = SampleCountFlagBits::BIT_1,
			.tiling = ImageTiling::OPTIMAL,
			.usageFlags = ImageUsageFlagBits::SAMPLED_BIT,
			.memoryPropertyFlags = MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			.generateMipmap = true,
			.mipmapFilter = Filter::LINEAR};

		testImage = device->Create(imageCreateInfo, pixels);

		ImageViewCreateInfo imageViewCreateInfo{
			.pImage = testImage,
			.viewType = ImageViewType::TYPE_2D,
			//.format = ShitFormat::RGBA8_UNORM,
			.format = ShitFormat::RGBA8_SRGB,
			.subresourceRange = {2, mipmapLevels - 2, 0, 1},
		};
		testImageView = device->Create(imageViewCreateInfo);
		stbi_image_free(pixels);
	}
	void createSamplers()
	{
		SamplerCreateInfo samplerCreateInfo{
			.magFilter = Filter::LINEAR,
			.minFilter = Filter::LINEAR,
			.mipmapMode = SamplerMipmapMode::LINEAR,
			.wrapModeU = SamplerWrapMode::REPEAT,
			.wrapModeV = SamplerWrapMode::REPEAT,
			.wrapModeW = SamplerWrapMode::REPEAT,
			.minLod = 0.f,
			.maxLod = 100.f,
		};
		sampler = device->Create(samplerCreateInfo);
	}
};

int main(int ac, char **av)
{
	parseArgument(ac, av);
	Hello app;
	try
	{
		app.run();
	}
	catch (const std::exception &err)
	{
		std::cout << err.what() << std::endl;
	}
}