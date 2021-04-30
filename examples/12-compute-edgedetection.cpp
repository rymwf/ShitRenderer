#include "common.hpp"
#include "model.hpp"

const char *compShaderNameGrayScale = "12-grayscale.comp.spv";
const char *compShaderName = "12.comp.spv";
const char *compShaderNameFilter = "12-filter.comp.spv";

const char *testImagePath = IMAGE_PATH "Lenna_test.jpg";

//std::vector<float> filter_gaussian1{
//	0.00013383022576488537,
//	0.0044318484119380075,
//	0.05399096651318806,
//	0.24197072451914337,
//	0.3989422804014327,
//	0.24197072451914337,
//	0.05399096651318806,
//	0.0044318484119380075,
//	0.00013383022576488537,
//}; //1
//std::vector<float> filter_gaussian05{0.10798, 0.79788, 0.10798}; //0.5
std::vector<float> filter_binominal{0.0625, 0.25, 0.375, 0.25, 0.0625};
std::vector<float> filter = filter_binominal;

class Hello
{
	RenderSystem *renderSystem;
	ShitWindow *window;
	Device *device;

	Shader *compShaderGrayscale;
	Shader *compShader;
	Shader *compShaderFilter;

	DescriptorPool *descriptorPool;

	PipelineLayout *pipelineLayout;
	PipelineLayout *pipelineLayoutFilter;

	Pipeline *pipelineGrayScale;
	Pipeline *pipeline;
	Pipeline *pipelineFilter;

	std::vector<DescriptorSetLayout *> descriptorSetLayouts;
	std::vector<DescriptorSet *> descriptorSets;

	Buffer *filterBuffer;

	Image *testImage;
	ImageView *testImageView;

	Image *outputImage0;
	ImageView *outputImageView0;

	Image *outputImage1;
	ImageView *outputImageView1;

	//==============
	std::chrono::system_clock::time_point curTime;
	std::chrono::system_clock::time_point startTime;

public:
	void initRenderSystem()
	{
		startTime = std::chrono::system_clock::now();
		RenderSystemCreateInfo renderSystemCreateInfo{
			.version = rendererVersion,
			.flags = RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT,
		};

		renderSystem = LoadRenderSystem(renderSystemCreateInfo);
		//1. create window needed when using opengl
		WindowCreateInfo windowCreateInfo{
			{WindowCreateFlagBits::INVISIBLE},
			__FILE__,
			{{DEFAULT_WINDOW_X, DEFAULT_WINDOW_Y},
			 {DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT}}};
		window = renderSystem->CreateRenderWindow(windowCreateInfo);
		//1.5 choose phyiscal device
		//2. create device of a physical device
		DeviceCreateInfo deviceCreateInfo{};
		if ((rendererVersion & RendererVersion::TypeBitmask) == RendererVersion::GL)
			deviceCreateInfo = {window};

		device = renderSystem->CreateDevice(deviceCreateInfo);

		createImages();
		createUBOBuffers();

		createShaders();
		createDescriptorSets();
		updateDescriptorSets();

		createPipeline();
		processImage();

		saveImages();
	}

	void cleanUp()
	{
	}
	void run()
	{
		initRenderSystem();
	}

	void createShaders()
	{
		std::string compShaderPath = buildShaderPath(compShaderNameGrayScale, rendererVersion);
		compShaderGrayscale = device->Create(ShaderCreateInfo{readFile(compShaderPath.c_str())});

		compShaderPath = buildShaderPath(compShaderName, rendererVersion);
		compShader = device->Create(ShaderCreateInfo{readFile(compShaderPath.c_str())});

		compShaderPath = buildShaderPath(compShaderNameFilter, rendererVersion);
		compShaderFilter = device->Create(ShaderCreateInfo{readFile(compShaderPath.c_str())});
	}

	void createDescriptorSets()
	{
		std::vector<DescriptorSetLayoutBinding> bindings0{
			DescriptorSetLayoutBinding{0, DescriptorType::STORAGE_IMAGE, 1, ShaderStageFlagBits::COMPUTE_BIT},
			DescriptorSetLayoutBinding{1, DescriptorType::STORAGE_IMAGE, 1, ShaderStageFlagBits::COMPUTE_BIT},
		};
		std::vector<DescriptorSetLayoutBinding> bindings1{
			DescriptorSetLayoutBinding{0, DescriptorType::STORAGE_IMAGE, 1, ShaderStageFlagBits::COMPUTE_BIT},
			DescriptorSetLayoutBinding{1, DescriptorType::STORAGE_IMAGE, 1, ShaderStageFlagBits::COMPUTE_BIT},
			DescriptorSetLayoutBinding{2, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::COMPUTE_BIT},
		};

		descriptorSetLayouts.resize(2);
		descriptorSetLayouts[0] = (device->Create(DescriptorSetLayoutCreateInfo{bindings0}));
		descriptorSetLayouts[1] = (device->Create(DescriptorSetLayoutCreateInfo{bindings1}));

		//pipelineLayout
		pipelineLayout = device->Create(PipelineLayoutCreateInfo{{1, descriptorSetLayouts[0]}});

		pipelineLayoutFilter = device->Create(PipelineLayoutCreateInfo{{1, descriptorSetLayouts[1]}});

		//frame binding descriptors
		std::vector<DescriptorSetLayout *> setLayouts{descriptorSetLayouts[0], descriptorSetLayouts[0], descriptorSetLayouts[1]};

		std::vector<DescriptorPoolSize> poolSizes(2);
		poolSizes[0] = DescriptorPoolSize{bindings0[0].descriptorType, 6};
		poolSizes[1] = DescriptorPoolSize{bindings1[2].descriptorType, 1};

		DescriptorPoolCreateInfo descriptorPoolCreateInfo{
			.maxSets = static_cast<uint32_t>(setLayouts.size()),
			.poolSizes = poolSizes};
		descriptorPool = device->Create(descriptorPoolCreateInfo);

		DescriptorSetAllocateInfo allocInfo{setLayouts};
		descriptorPool->Allocate(allocInfo, descriptorSets);
	}
	void updateDescriptorSets()
	{
		std::vector<WriteDescriptorSet> writes;
		//descriptor set 0
		writes.emplace_back(
			WriteDescriptorSet{
				descriptorSets[0],
				0,
				0,
				DescriptorType::STORAGE_IMAGE,
				std::vector<DescriptorImageInfo>{{nullptr,
												  testImageView,
												  ImageLayout::GENERAL}}});
		writes.emplace_back(
			WriteDescriptorSet{
				descriptorSets[0],
				1,
				0,
				DescriptorType::STORAGE_IMAGE,
				std::vector<DescriptorImageInfo>{{nullptr,
												  outputImageView0,
												  ImageLayout::GENERAL}}});
		//set 1
		writes.emplace_back(
			WriteDescriptorSet{
				descriptorSets[1],
				0,
				0,
				DescriptorType::STORAGE_IMAGE,
				std::vector<DescriptorImageInfo>{{nullptr,
												  outputImageView0,
												  ImageLayout::GENERAL}}});
		writes.emplace_back(
			WriteDescriptorSet{
				descriptorSets[1],
				1,
				0,
				DescriptorType::STORAGE_IMAGE,
				std::vector<DescriptorImageInfo>{{nullptr,
												  outputImageView1,
												  ImageLayout::GENERAL}}});
		//filter descriptor
		writes.emplace_back(
			WriteDescriptorSet{
				descriptorSets[2],
				0,
				0,
				DescriptorType::STORAGE_IMAGE,
				std::vector<DescriptorImageInfo>{{nullptr,
												  outputImageView0,
												  ImageLayout::GENERAL}}});
		writes.emplace_back(
			WriteDescriptorSet{
				descriptorSets[2],
				1,
				0,
				DescriptorType::STORAGE_IMAGE,
				std::vector<DescriptorImageInfo>{{nullptr,
												  outputImageView0,
												  ImageLayout::GENERAL}}});
		writes.emplace_back(
			WriteDescriptorSet{
				descriptorSets[2],
				2,
				0,
				DescriptorType::UNIFORM_BUFFER,
				std::vector<DescriptorBufferInfo>{{filterBuffer,
												   0,
												   sizeof(float) * filter.size() * 4}}});
		device->UpdateDescriptorSets(writes, {});
	}
	void createPipeline()
	{
		//pipeline grayscale
		std::vector<uint32_t> constantIDs{COSTANT_ID_COMPUTE_LOCAL_SIZE_X};
		std::vector<uint32_t> constantValues{testImage->GetCreateInfoPtr()->extent.width};

		pipelineGrayScale = device->Create(ComputePipelineCreateInfo{
			PipelineShaderStageCreateInfo{
				PipelineShaderStageCreateInfo{
					ShaderStageFlagBits::COMPUTE_BIT,
					compShaderGrayscale,
					"main",
					{constantIDs,
					 constantValues}},
			},
			pipelineLayout});
		// filter pipelin
		constantIDs = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X, COSTANT_ID_COMPUTE_OPERATOR_DIM};
		constantValues = {testImage->GetCreateInfoPtr()->extent.width, static_cast<uint32_t>(filter.size())};

		pipelineFilter = device->Create(ComputePipelineCreateInfo{
			PipelineShaderStageCreateInfo{
				PipelineShaderStageCreateInfo{
					ShaderStageFlagBits::COMPUTE_BIT,
					compShaderFilter,
					"main",
					{constantIDs,
					 constantValues}},
			},
			pipelineLayoutFilter});

		//pipeline edge detection
		constantIDs = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X};
		constantValues = {testImage->GetCreateInfoPtr()->extent.width};

		pipeline = device->Create(ComputePipelineCreateInfo{
			PipelineShaderStageCreateInfo{
				PipelineShaderStageCreateInfo{
					ShaderStageFlagBits::COMPUTE_BIT,
					compShader,
					"main",
					{constantIDs,
					 constantValues}},
			},
			pipelineLayout});
	}
	void processImage()
	{
		ImageMemoryBarrier barrier{
			AccessFlagBits::SHADER_WRITE_BIT,
			AccessFlagBits::SHADER_READ_BIT,
			ImageLayout::GENERAL,
			ImageLayout::GENERAL,
			outputImage0,
			ImageSubresourceRange{0, 1, 0, 1}};

		device->ExecuteOneTimeCommands([&](CommandBuffer *commandBuffer) {
			//gray scale
			commandBuffer->BindPipeline(BindPipelineInfo{
				PipelineBindPoint::COMPUTE,
				pipelineGrayScale});
			commandBuffer->BindDescriptorSets(BindDescriptorSetsInfo{
				PipelineBindPoint::COMPUTE,
				pipelineLayout,
				0,
				1,
				&descriptorSets[0],
			});
			commandBuffer->Dispatch(DispatchInfo{testImage->GetCreateInfoPtr()->extent.height, 1u, 1u});
			//smooth
			commandBuffer->PipeplineBarrier(PipelineBarrierInfo{
				PipelineStageFlagBits::COMPUTE_SHADER_BIT,
				PipelineStageFlagBits::COMPUTE_SHADER_BIT,
				{},
				0,
				nullptr,
				0,
				nullptr,
				1,
				&barrier});
			commandBuffer->BindDescriptorSets(BindDescriptorSetsInfo{
				PipelineBindPoint::COMPUTE,
				pipelineLayoutFilter,
				0,
				1,
				&descriptorSets[2],
			});
			commandBuffer->BindPipeline(BindPipelineInfo{
				PipelineBindPoint::COMPUTE,
				pipelineFilter});

			commandBuffer->Dispatch(DispatchInfo{testImage->GetCreateInfoPtr()->extent.height, 1u, 1u});

			//edge detection
			commandBuffer->PipeplineBarrier(PipelineBarrierInfo{
				PipelineStageFlagBits::COMPUTE_SHADER_BIT,
				PipelineStageFlagBits::COMPUTE_SHADER_BIT,
				{},
				0,
				nullptr,
				0,
				nullptr,
				1,
				&barrier});
			commandBuffer->BindPipeline(BindPipelineInfo{
				PipelineBindPoint::COMPUTE,
				pipeline});
			commandBuffer->BindDescriptorSets(BindDescriptorSetsInfo{
				PipelineBindPoint::COMPUTE,
				pipelineLayout,
				0,
				1,
				&descriptorSets[1],
			});
			commandBuffer->Dispatch(DispatchInfo{testImage->GetCreateInfoPtr()->extent.height, 1u, 1u});
		});
	}
	void createImages()
	{
		int width, height, components;
		auto pixels = loadImage(testImagePath, width, height, components, 4); //force load an alpha channel,even not exist
		if (!pixels)
			throw std::runtime_error("failed to load texture image!");
		//uint32_t mipmapLevels = 5;

		ImageCreateInfo imageCreateInfo{
			//.flags=ImageCreateFlagBits::MUTABLE_FORMAT_BIT,
			.imageType = ImageType::TYPE_2D,
			.format = ShitFormat::RGBA8_UNORM, //TODO: check if format support storage image
			.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = SampleCountFlagBits::BIT_1,
			.tiling = ImageTiling::OPTIMAL,
			.usageFlags = ImageUsageFlagBits::STORAGE_BIT,
			.memoryPropertyFlags = MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			.initialLayout = ImageLayout::GENERAL};

		testImage = device->Create(imageCreateInfo, pixels);

		ImageViewCreateInfo imageViewCreateInfo{
			.pImage = testImage,
			.viewType = ImageViewType::TYPE_2D,
			.format = ShitFormat::RGBA8_UNORM,
			.subresourceRange = {0, 1, 0, 1},
		};
		testImageView = device->Create(imageViewCreateInfo);
		freeImage(pixels);

		//=======
		imageCreateInfo.format = ShitFormat::R8_UNORM;
		outputImage0 = device->Create(imageCreateInfo, nullptr);

		imageViewCreateInfo.format = ShitFormat::R8_UNORM;
		imageViewCreateInfo.pImage = outputImage0;
		outputImageView0 = device->Create(imageViewCreateInfo);

		outputImage1 = device->Create(imageCreateInfo, nullptr);

		imageViewCreateInfo.pImage = outputImage1;
		outputImageView1 = device->Create(imageViewCreateInfo);
	}
	void saveImages()
	{
		takeScreenshot(device, outputImage0);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		takeScreenshot(device, outputImage1);
	}
	void createUBOBuffers()
	{
		std::vector<float> ubodata(4 * filter.size());
		for (size_t i = 0; i < filter.size(); ++i)
		{
			ubodata[i * 4] = filter[i];
		}
		filterBuffer = device->Create(BufferCreateInfo{
										  {},
										  sizeof(float) * filter.size() * 4,
										  BufferUsageFlagBits::UNIFORM_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
										  MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
									  ubodata.data());
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