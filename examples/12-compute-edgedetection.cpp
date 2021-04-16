#include "common.hpp"
#include "model.hpp"

#define MODEL_SIZE 1.
#define PERSPECTIVE 1

uint32_t WIDTH = 800, HEIGHT = 600;

int animationIndex = 0;

const char *compShaderNameGrayScale = "11-grayscale.comp.spv";
const char *compShaderNameX = "11-x.comp.spv";
const char *compShaderNameY = "11-y.comp.spv";

const char *testImagePath = IMAGE_PATH "Lenna_test.jpg";

std::vector<float> SobelOperator{1, 2, 1};
//std::vector<float> SobelOperator{-1, 0, 1};

class Hello
{
	RenderSystem *renderSystem;
	ShitWindow *window;
	Device *device;

	Shader *compShaderGrayscale;
	Shader *compShaderX;
	Shader *compShaderY;

	DescriptorPool *descriptorPool;

	PipelineLayout *pipelineLayout0;
	PipelineLayout *pipelineLayout1;
	Pipeline *pipelineGrayScale;
	Pipeline *pipelineX;
	Pipeline *pipelineY;

	std::vector<DescriptorSetLayout *> descriptorSetLayouts;
	std::vector<DescriptorSet *> descriptorSets;

	Buffer *filterBuffer;

	Image *testImage;
	ImageView *testImageView;

	Image *outputImage;
	ImageView *outputImageView;

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
			{{SHIT_DEFAULT_WINDOW_X, SHIT_DEFAULT_WINDOW_Y},
			 {SHIT_DEFAULT_WINDOW_WIDTH, SHIT_DEFAULT_WINDOW_HEIGHT}}};
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

		compShaderPath = buildShaderPath(compShaderNameX, rendererVersion);
		compShaderX = device->Create(ShaderCreateInfo{readFile(compShaderPath.c_str())});

		compShaderPath = buildShaderPath(compShaderNameY, rendererVersion);
		compShaderY = device->Create(ShaderCreateInfo{readFile(compShaderPath.c_str())});
	}

	void createDescriptorSets()
	{
		std::vector<DescriptorSetLayoutBinding> bindings0{
			DescriptorSetLayoutBinding{0, DescriptorType::STORAGE_IMAGE, 1, ShaderStageFlagBits::COMPUTE_BIT},
			DescriptorSetLayoutBinding{1, DescriptorType::STORAGE_IMAGE, 1, ShaderStageFlagBits::COMPUTE_BIT},
		};
		std::vector<DescriptorSetLayoutBinding> bindings1{
			DescriptorSetLayoutBinding{0, DescriptorType::STORAGE_IMAGE, 1, ShaderStageFlagBits::COMPUTE_BIT},
			DescriptorSetLayoutBinding{1, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::COMPUTE_BIT},
		};

		descriptorSetLayouts.resize(2);
		descriptorSetLayouts[0] = (device->Create(DescriptorSetLayoutCreateInfo{bindings0}));
		descriptorSetLayouts[1] = (device->Create(DescriptorSetLayoutCreateInfo{bindings1}));

		//pipelineLayout
		pipelineLayout0 = device->Create(PipelineLayoutCreateInfo{{1, descriptorSetLayouts[0]}});
		pipelineLayout1 = device->Create(PipelineLayoutCreateInfo{{1, descriptorSetLayouts[1]}});

		//frame binding descriptors
		std::vector<DescriptorSetLayout *> setLayouts{descriptorSetLayouts};

		std::vector<DescriptorPoolSize> poolSizes(2);
		poolSizes[0] = DescriptorPoolSize{bindings1[0].descriptorType, 3};
		poolSizes[1] = DescriptorPoolSize{bindings1[1].descriptorType, 1};

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
												  outputImageView,
												  ImageLayout::GENERAL}}});
		//set 1
		writes.emplace_back(
			WriteDescriptorSet{
				descriptorSets[1],
				0,
				0,
				DescriptorType::STORAGE_IMAGE,
				std::vector<DescriptorImageInfo>{{nullptr,
												  outputImageView,
												  ImageLayout::GENERAL}}});
		writes.emplace_back(
			WriteDescriptorSet{
				descriptorSets[1],
				1,
				0,
				DescriptorType::UNIFORM_BUFFER,
				std::vector<DescriptorBufferInfo>{{filterBuffer,
												   0,
												   sizeof(float) * SobelOperator.size()}}});
		device->UpdateDescriptorSets(writes, {});
	}
	void createPipeline()
	{
		//pipeline grayscale
		std::vector<uint32_t> constantIDs{COSTANT_ID_COMPUTE_LOCAL_SIZE_X };
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
			pipelineLayout0});
		//pipeline x
		constantIDs = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X, COSTANT_ID_COMPUTE_OPERATOR_DIM};
		constantValues = {testImage->GetCreateInfoPtr()->extent.width, static_cast<uint32_t>(SobelOperator.size())};

		pipelineX = device->Create(ComputePipelineCreateInfo{
			PipelineShaderStageCreateInfo{
				PipelineShaderStageCreateInfo{
					ShaderStageFlagBits::COMPUTE_BIT,
					compShaderX,
					"main",
					{constantIDs,
					 constantValues}},
			},
			pipelineLayout1});
		//pipeline y
		constantIDs = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X, COSTANT_ID_COMPUTE_OPERATOR_DIM};
		constantValues = {testImage->GetCreateInfoPtr()->extent.height, static_cast<uint32_t>(SobelOperator.size())};

		pipelineY = device->Create(ComputePipelineCreateInfo{
			PipelineShaderStageCreateInfo{
				PipelineShaderStageCreateInfo{
					ShaderStageFlagBits::COMPUTE_BIT,
					compShaderY,
					"main",
					{constantIDs,
					 constantValues}},
			},
			pipelineLayout1});
	}
	void processImage()
	{
		device->ExecuteOneTimeCommands([&](CommandBuffer *commandBuffer) {
			//gray scale
			commandBuffer->BindPipeline(BindPipelineInfo{
				PipelineBindPoint::COMPUTE,
				pipelineGrayScale});
			commandBuffer->BindDescriptorSets(BindDescriptorSetsInfo{
				PipelineBindPoint::COMPUTE,
				pipelineLayout0,
				0,
				1,
				&descriptorSets[0],
			});
			commandBuffer->Dispatch(DispatchInfo{testImage->GetCreateInfoPtr()->extent.height, 1u, 1u});

			ImageMemoryBarrier barrier{
				AccessFlagBits::SHADER_WRITE_BIT,
				AccessFlagBits::SHADER_READ_BIT,
				ImageLayout::GENERAL,
				ImageLayout::GENERAL,
				outputImage,
				ImageSubresourceRange{0, 1, 0, 1}};

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

			//x convolution
			commandBuffer->BindPipeline(BindPipelineInfo{
				PipelineBindPoint::COMPUTE,
				pipelineX});
			commandBuffer->BindDescriptorSets(BindDescriptorSetsInfo{
				PipelineBindPoint::COMPUTE,
				pipelineLayout1,
				0,
				1,
				&descriptorSets[1],
			});
			commandBuffer->Dispatch(DispatchInfo{testImage->GetCreateInfoPtr()->extent.height, 1u, 1u});

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
			//y convolution
			commandBuffer->BindDescriptorSets(BindDescriptorSetsInfo{
				PipelineBindPoint::COMPUTE,
				pipelineLayout1,
				0,
				1,
				&descriptorSets[2],
			});
			commandBuffer->BindPipeline(BindPipelineInfo{
				PipelineBindPoint::COMPUTE,
				pipelineY});
			commandBuffer->Dispatch(DispatchInfo{testImage->GetCreateInfoPtr()->extent.height, 1u, 1u});
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
		filterBuffer = device->Create(BufferCreateInfo{
											 {},
											 sizeof(float) * SobelOperator.size(),
											 BufferUsageFlagBits::UNIFORM_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
											 MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
										 SobelOperator.data());
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