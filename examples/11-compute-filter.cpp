#include "common.hpp"
#include "model.hpp"

const char *compShaderName = "11.comp.spv";

const char *testImagePath = IMAGE_PATH "Lenna_test.jpg";

std::vector<float> filter_linear_4{0.25, 0.5, 0.25};
std::vector<float> filter_binominal{0.0625, 0.25, 0.375, 0.25, 0.0625};
std::vector<float> filter_sobel{-0.5, 0, 0.5};
std::vector<float> filter_corner{-0.5, 1, -0.5};
std::vector<float> filter_gaussian05{0.10798, 0.79788, 0.10798}; //0.5
std::vector<float> filter_gaussian1{
	0.00013383022576488537,
	0.0044318484119380075,
	0.05399096651318806,
	0.24197072451914337,
	0.3989422804014327,
	0.24197072451914337,
	0.05399096651318806,
	0.0044318484119380075,
	0.00013383022576488537,
}; //1

//std::vector<float> filter = filter_linear_4;
//std::vector<float> filter=filter_binominal;
//std::vector<float> filter = filter_sobel;
//std::vector<float> filter = filter_corner;
//std::vector<float> filter = filter_gaussian05;
std::vector<float> filter = filter_gaussian1;
//std::vector<float> filter = filter_sharp;
float filter_radius = 10.f;

class Hello
{
	RenderSystem *renderSystem;
	ShitWindow *window;
	Device *device;

	Shader *compShader;

	DescriptorPool *descriptorPool;

	PipelineLayout *pipelineLayout;
	Pipeline *pipeline;

	std::vector<DescriptorSetLayout *> descriptorSetLayouts;
	std::vector<DescriptorSet *> descriptorSets;

	Buffer *filterBuffer;

	Image *testImage;
	ImageView *testImageView;

	Image *outputImage;
	ImageView *outputImageView;

	Sampler *sampler;

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
		std::string compShaderPath = buildShaderPath(compShaderName, rendererVersion);
		compShader = device->Create(ShaderCreateInfo{readFile(compShaderPath.c_str())});
	}

	void createDescriptorSets()
	{
		std::vector<DescriptorSetLayoutBinding> bindings{
			DescriptorSetLayoutBinding{0, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::COMPUTE_BIT},
			DescriptorSetLayoutBinding{1, DescriptorType::STORAGE_IMAGE, 1, ShaderStageFlagBits::COMPUTE_BIT},
			DescriptorSetLayoutBinding{2, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::COMPUTE_BIT},
		};

		descriptorSetLayouts.resize(1, device->Create(DescriptorSetLayoutCreateInfo{bindings}));

		//pipelineLayout
		pipelineLayout = device->Create(PipelineLayoutCreateInfo{{1, descriptorSetLayouts[0]}});

		//frame binding descriptors
		std::vector<DescriptorSetLayout *> setLayouts{descriptorSetLayouts[0]};

		std::vector<DescriptorPoolSize> poolSizes(bindings.size());
		std::transform(bindings.begin(), bindings.end(), poolSizes.begin(), [](auto &&e) {
			return DescriptorPoolSize{e.descriptorType, e.descriptorCount};
		});

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
		//set 0
		writes.emplace_back(
			WriteDescriptorSet{
				descriptorSets[0],
				0,
				0,
				DescriptorType::COMBINED_IMAGE_SAMPLER,
				std::vector<DescriptorImageInfo>{{sampler,
												  testImageView,
												  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
		writes.emplace_back(
			WriteDescriptorSet{
				descriptorSets[0],
				1,
				0,
				DescriptorType::STORAGE_IMAGE,
				std::vector<DescriptorImageInfo>{{nullptr,
												  outputImageView,
												  ImageLayout::GENERAL}}});
		writes.emplace_back(
			WriteDescriptorSet{
				descriptorSets[0],
				2,
				0,
				DescriptorType::UNIFORM_BUFFER,
				std::vector<DescriptorBufferInfo>{{filterBuffer,
												   0,
												   sizeof(float) * (filter.size() + 1) * 4}}});
		device->UpdateDescriptorSets(writes, {});
	}
	void createPipeline()
	{
		//pipeline x
		std::vector<uint32_t> constantIDs{COSTANT_ID_COMPUTE_LOCAL_SIZE_X, COSTANT_ID_COMPUTE_OPERATOR_DIM};
		std::vector<uint32_t> constantValues{testImage->GetCreateInfoPtr()->extent.width, static_cast<uint32_t>(filter.size())};

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
		device->ExecuteOneTimeCommands([&](CommandBuffer *commandBuffer) {
			//x convolution
			commandBuffer->BindPipeline(BindPipelineInfo{
				PipelineBindPoint::COMPUTE,
				pipeline});
			commandBuffer->BindDescriptorSets(BindDescriptorSetsInfo{
				PipelineBindPoint::COMPUTE,
				pipelineLayout,
				0,
				1,
				&descriptorSets[0],
			});
			commandBuffer->Dispatch(DispatchInfo{testImage->GetCreateInfoPtr()->extent.height, 1u, 1u});

//			ImageMemoryBarrier barrier{
//				AccessFlagBits::SHADER_WRITE_BIT,
//				AccessFlagBits::SHADER_READ_BIT,
//				ImageLayout::GENERAL,
//				ImageLayout::GENERAL,
//				outputImage,
//				ImageSubresourceRange{0, 1, 0, 1}};
		});
	}
	void createImages()
	{
		sampler = device->Create(SamplerCreateInfo{
			Filter::LINEAR,
			Filter::LINEAR,
			SamplerMipmapMode::LINEAR,
			SamplerWrapMode::CLAMP_TO_EDGE,
			SamplerWrapMode::CLAMP_TO_EDGE,
			SamplerWrapMode::CLAMP_TO_EDGE,
			0,
			false,
			false,
			{},
			0.f,
			30.f,
		});

		//=========
		int width, height, components;
		auto pixels = loadImage(testImagePath, width, height, components, 4); //force load an alpha channel,even not exist
		if (!pixels)
			throw std::runtime_error("failed to load texture image!");

		ImageCreateInfo imageCreateInfo{
			//.flags=ImageCreateFlagBits::MUTABLE_FORMAT_BIT,
			.imageType = ImageType::TYPE_2D,
			.format = ShitFormat::RGBA8_UNORM, //TODO: check if format support storage image
			.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
			.mipLevels = 0,
			.arrayLayers = 1,
			.samples = SampleCountFlagBits::BIT_1,
			.tiling = ImageTiling::OPTIMAL,
			.usageFlags = ImageUsageFlagBits::SAMPLED_BIT | ImageUsageFlagBits::STORAGE_BIT,
			.memoryPropertyFlags = MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			.initialLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		};

		testImage = device->Create(imageCreateInfo, pixels);
		testImage->GenerateMipmaps(Filter::LINEAR);

		ImageViewCreateInfo imageViewCreateInfo{
			.pImage = testImage,
			.viewType = ImageViewType::TYPE_2D,
			.format = ShitFormat::RGBA8_UNORM,
			.subresourceRange = {0, 1, 0, 1},
		};
		testImageView = device->Create(imageViewCreateInfo);
		freeImage(pixels);

		//=======
		imageCreateInfo.usageFlags = ImageUsageFlagBits::STORAGE_BIT;
		imageCreateInfo.initialLayout = ImageLayout::GENERAL;
		outputImage = device->Create(imageCreateInfo, nullptr);
		imageViewCreateInfo.pImage = outputImage;
		outputImageView = device->Create(imageViewCreateInfo);
	}
	void saveImages()
	{
		takeScreenshot(device, outputImage);
	}
	void createUBOBuffers()
	{
		std::vector<float> ubodata(4 * (filter.size() + 1));
		ubodata[0] = filter_radius;
		for (size_t i = 0; i < filter.size(); ++i)
		{
			ubodata[(i + 1) * 4] = filter[i];
		}
		filterBuffer = device->Create(BufferCreateInfo{
										  {},
										  sizeof(float) * (filter.size() + 1) * 4,
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