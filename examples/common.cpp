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
#include <stb_image.h>
#include <stb_image_write.h>

Shit::RendererVersion rendererVersion{Shit::RendererVersion::GL};

void executeOneTimeCommands(
	Device *pDevice,
	QueueFlagBits queueFlags,
	uint32_t queueIndex,
	const std::function<void(CommandBuffer *)> &func)
{
	auto queueFamilyIndex = pDevice->GetQueueFamilyIndexByFlag(queueFlags, {});
	auto queue = pDevice->Create(QueueCreateInfo{queueFamilyIndex->index, queueIndex});
	static auto pFence = pDevice->Create(FenceCreateInfo{});
	pFence->Reset();
	auto commandPool = pDevice->Create(CommandPoolCreateInfo{
		CommandPoolCreateFlagBits::TRANSIENT_BIT,
		queueFamilyIndex->index});
	std::vector<CommandBuffer *> commandBuffers;
	commandPool->CreateCommandBuffers(CommandBufferCreateInfo{CommandBufferLevel::PRIMARY, 1}, commandBuffers);

	commandBuffers[0]->Begin(CommandBufferBeginInfo{CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT});
	func(commandBuffers[0]);
	commandBuffers[0]->End();

	queue->Submit({{{}, {commandBuffers}}}, pFence);
	pFence->WaitFor(UINT64_MAX);
	//queue->WaitIdle();
	pDevice->Destroy(commandPool);
}

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
	void *ret;
	if (stbi_is_hdr(imagePath))
		ret = stbi_loadf(imagePath, &width, &height, &components, request_components); //force load an alpha channel,even not exist
	else
		ret = stbi_load(imagePath, &width, &height, &components, request_components); //force load an alpha channel,even not exist
	if (!ret)
		THROW("failed to load texture image!")
	return ret;
}
void saveImage(const char *imagePath, int width, int height, int component, const void *data, bool hdr)
{
	int ret{};
	if (hdr)
		ret = stbi_write_hdr(imagePath, width, height, component, reinterpret_cast<const float *>(data));
	else
		ret = stbi_write_jpg(imagePath, width, height, component, data, 90);

	if (!ret)
		THROW("failed to save image");
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

void takeScreenshot(Device *pDevice, Image *pImage, ImageLayout imageLayout)
{
	std::time_t t = std::time(nullptr);
	std::tm tm;
	localtime_s(&tm, &t);
	std::ostringstream ss;
	ss << std::put_time(&tm, "%Y%m%d%H%M%S");
	saveImage((std::string(SCRRENSHOT_DIR) + ss.str()).c_str(), pDevice, pImage, imageLayout);
}
void saveImage(const char *dstPath, Device *pDevice, Image *pImage, ImageLayout imageLayout)
{
	auto width = pImage->GetCreateInfoPtr()->extent.width;
	auto height = pImage->GetCreateInfoPtr()->extent.height;
	auto depth = pImage->GetCreateInfoPtr()->extent.depth;
	auto layers = pImage->GetCreateInfoPtr()->arrayLayers;
	auto component = Shit::GetFormatComponentNum(pImage->GetCreateInfoPtr()->format);

	auto formatSize = Shit::GetFormatSize(pImage->GetCreateInfoPtr()->format);
	auto levels = pImage->GetCreateInfoPtr()->mipLevels;

	//if constexpr (0 && static_cast<bool>(pImage->GetCreateInfoPtr()->memoryPropertyFlags & MemoryPropertyFlagBits::HOST_VISIBLE_BIT))
	//{
	//	void *data;
	//	pImage->MapMemory(0, size, &data);
	//	//swizzleColor
	//	if (
	//		pImage->GetCreateInfoPtr()->format == ShitFormat::BGRA8_SRGB ||
	//		pImage->GetCreateInfoPtr()->format == ShitFormat::BGRA8_UNORM)
	//	{
	//		unsigned char *p = reinterpret_cast<unsigned char *>(data);
	//		unsigned char temp;
	//		for (uint32_t i = 0; i < size; i += 4, p += 4)
	//		{
	//			temp = *p;
	//			*(p) = *(p + 2);
	//			*(p + 2) = temp;
	//		}
	//	}
	//	saveImage(dstPath, width, height * depth * layers, component, data);
	//	pImage->UnMapMemory();
	//	if (!static_cast<bool>(pImage->GetCreateInfoPtr()->memoryPropertyFlags & MemoryPropertyFlagBits::HOST_COHERENT_BIT))
	//	{
	//		pImage->FlushMappedMemoryRange(0, size);
	//	}
	//}
	//else
	{
		//TODO: check if support blit to linear image
		//NOTE: opengl need to flip y
		uint32_t tempWidth = width, tempHeight = height, tempDepth = depth;
#if 1
		//copy to buffer
		//		 cannot change format
		std::vector<Buffer *> dst(levels);
		std::vector<BufferMemoryBarrier> bufferBarriers(levels);
		for (uint32_t i = 0; i < levels; ++i,
					  tempWidth = std::max(tempWidth / 2, 1u),
					  tempHeight = std::max(tempHeight / 2, 1u),
					  tempDepth = std::max(tempDepth / 2, 1u))
		{
			auto size = tempWidth * tempHeight * tempDepth * layers * formatSize;
			dst[i] = pDevice->Create(
				BufferCreateInfo{
					{},
					size,
					BufferUsageFlagBits::TRANSFER_DST_BIT,
					MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
				nullptr);
		}

		executeOneTimeCommands(pDevice, QueueFlagBits::TRANSFER_BIT, 0, [&](CommandBuffer *cmdBuffer) {
			for (uint32_t i = 0; i < levels; ++i)
			{
				bufferBarriers[i] = BufferMemoryBarrier{
					{},
					AccessFlagBits::TRANSFER_WRITE_BIT,
					dst[i],
					0,
					dst[i]->GetCreateInfoPtr()->size};
			}

			ImageMemoryBarrier imageBarrier0{
				AccessFlagBits::SHADER_WRITE_BIT | AccessFlagBits::TRANSFER_WRITE_BIT,
				AccessFlagBits::TRANSFER_READ_BIT,
				imageLayout,
				ImageLayout::TRANSFER_SRC_OPTIMAL,
				pImage,
				ImageSubresourceRange{0, levels, 0, layers}};
			cmdBuffer->PipeplineBarrier(PipelineBarrierInfo{
				PipelineStageFlagBits::ALL_COMMANDS_BIT,
				PipelineStageFlagBits::TRANSFER_BIT,
				{},
				0,
				nullptr,
				static_cast<uint32_t>(bufferBarriers.size()),
				bufferBarriers.data(),
				1,
				&imageBarrier0});

			tempWidth = width, tempHeight = height, tempDepth = depth;
			for (uint32_t i = 0; i < levels; ++i,
						  tempWidth = std::max(tempWidth / 2, 1u),
						  tempHeight = std::max(tempHeight / 2, 1u),
						  tempDepth = std::max(tempDepth / 2, 1u))
			{
				BufferImageCopy copyRegion{
					.imageSubresource = {i, 0, layers},
					.imageExtent = {tempWidth, tempHeight, tempDepth}};
				cmdBuffer->CopyImageToBuffer(CopyImageToBufferInfo{
					pImage,
					dst[i],
					1,
					&copyRegion});
			}
			for (uint32_t i = 0; i < levels; ++i)
			{
				bufferBarriers[i] = BufferMemoryBarrier{
					AccessFlagBits::MEMORY_WRITE_BIT,
					AccessFlagBits::MEMORY_READ_BIT,
					dst[i],
					0,
					dst[i]->GetCreateInfoPtr()->size};
			}
			ImageMemoryBarrier imageBarrier1{
				AccessFlagBits::TRANSFER_READ_BIT,
				AccessFlagBits::MEMORY_READ_BIT,
				ImageLayout::TRANSFER_SRC_OPTIMAL,
				imageLayout,
				pImage,
				ImageSubresourceRange{0, levels, 0, layers}};

			cmdBuffer->PipeplineBarrier(PipelineBarrierInfo{
				PipelineStageFlagBits::TRANSFER_BIT,
				PipelineStageFlagBits::ALL_COMMANDS_BIT,
				{},
				0,
				nullptr,
				static_cast<uint32_t>(bufferBarriers.size()),
				bufferBarriers.data(),
				1,
				&imageBarrier1});
		});

#else
		//TODO: need fix
		//copy to image
		Image *dst = pDevice->Create(
			ImageCreateInfo{
				ImageCreateFlagBits::CUBE_COMPATIBLE_BIT,
				ImageType::TYPE_2D,
				pImage->GetCreateInfoPtr()->format,
				{width, height, depth},
				levels,
				layers,
				SampleCountFlagBits::BIT_1,
				ImageTiling::LINEAR,
				ImageUsageFlagBits::TRANSFER_DST_BIT,
				MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT,
			},
			nullptr);

		std::vector<ImageCopy> copyRegions(levels);
		for (uint32_t i = 0; i < levels; ++i)
		{
			copyRegions[i] = {
				{i, 0, layers},
				{},
				{i, 0, layers},
				{},
				{width, height, depth}};
		}
		pDevice->ExecuteOneTimeCommands([&](CommandBuffer *cmdBuffer) {
			ImageMemoryBarrier pImageBarrier0{
				AccessFlagBits::MEMORY_READ_BIT,
				AccessFlagBits::TRANSFER_READ_BIT,
				imageLayout,
				ImageLayout::TRANSFER_SRC_OPTIMAL,
				pImage,
				ImageSubresourceRange{0, levels, 0, layers}};
			ImageMemoryBarrier dstImageBarrier0{
				{},
				AccessFlagBits::TRANSFER_WRITE_BIT,
				ImageLayout::UNDEFINED,
				ImageLayout::TRANSFER_DST_OPTIMAL,
				dst,
				ImageSubresourceRange{0, levels, 0, layers}};
			cmdBuffer->PipeplineBarrier(PipelineBarrierInfo{
				PipelineStageFlagBits::TRANSFER_BIT,
				PipelineStageFlagBits::TRANSFER_BIT,
				{},
				0,
				nullptr,
				0,
				nullptr,
				1,
				&pImageBarrier0});
			cmdBuffer->PipeplineBarrier(PipelineBarrierInfo{
				PipelineStageFlagBits::TRANSFER_BIT,
				PipelineStageFlagBits::TRANSFER_BIT,
				{},
				0,
				nullptr,
				0,
				nullptr,
				1,
				&dstImageBarrier0});
			cmdBuffer->CopyImage(CopyImageInfo{
				pImage,
				dst,
				static_cast<uint32_t>(copyRegions.size()),
				copyRegions.data()});
			ImageMemoryBarrier pImageBarrier1{
				AccessFlagBits::TRANSFER_READ_BIT,
				AccessFlagBits::MEMORY_READ_BIT,
				ImageLayout::TRANSFER_SRC_OPTIMAL,
				pImage->GetCreateInfoPtr()->initialLayout,
				pImage,
				ImageSubresourceRange{0, levels, 0, layers}};
			ImageMemoryBarrier dstImageBarrier1{
				AccessFlagBits::TRANSFER_WRITE_BIT,
				AccessFlagBits::MEMORY_READ_BIT,
				ImageLayout::TRANSFER_DST_OPTIMAL,
				ImageLayout::GENERAL,
				dst,
				ImageSubresourceRange{0, levels, 0, layers}};
			cmdBuffer->PipeplineBarrier(PipelineBarrierInfo{
				PipelineStageFlagBits::TRANSFER_BIT,
				PipelineStageFlagBits::TRANSFER_BIT,
				{},
				0,
				nullptr,
				0,
				nullptr,
				1,
				&pImageBarrier1});
			cmdBuffer->PipeplineBarrier(PipelineBarrierInfo{
				PipelineStageFlagBits::TRANSFER_BIT,
				PipelineStageFlagBits::TRANSFER_BIT,
				{},
				0,
				nullptr,
				0,
				nullptr,
				1,
				&dstImageBarrier1});
		});
#endif
		tempWidth = width, tempHeight = height, tempDepth = depth;
		for (uint32_t j = 0; j < levels; ++j,
					  tempWidth = std::max(tempWidth / 2, 1u),
					  tempHeight = std::max(tempHeight / 2, 1u),
					  tempDepth = std::max(tempDepth / 2, 1u))
		{
			auto size = tempWidth * tempHeight * tempDepth * layers * formatSize;
			void *data;
			dst[j]->MapMemory(0, size, &data);
			//swizzleColor
			if (
				pImage->GetCreateInfoPtr()->format == ShitFormat::BGRA8_SRGB ||
				pImage->GetCreateInfoPtr()->format == ShitFormat::BGRA8_UNORM)
			{
				unsigned char temp;
				unsigned char *p = reinterpret_cast<unsigned char *>(data);
				for (uint32_t i = 0; i < size; i += 4, p += 4)
				{
					temp = *p;
					*(p) = *(p + 2);
					*(p + 2) = temp;
				}
			}
			std::stringstream ss;
			ss << dstPath;
			ss << "-";
			ss << j;
			if (formatSize > 4)
				ss << ".hdr";
			else
				ss << ".jpg";
			saveImage(ss.str().c_str(), tempWidth, tempHeight * tempDepth * layers, component, data, formatSize > 4);
			dst[j]->UnMapMemory();
		}
		for (auto &&e : dst)
		{
			pDevice->Destroy(e);
		}
	}
}
//void parseArgument(int ac, char **av)
//{
//
//	namespace po = boost::program_options;
//	po::options_description desc("Allowed options");
//	desc.add_options()("help", "produce help message")("T", po::value<std::string>()->default_value("GL"), "set renderer version\n"
//																										   "option values:\n"
//																										   "GL:\t opengl rendereer"
//																										   "VK:\t vulkan renderer");
//
//	// Declare an options description instance which will include
//	// all the options
//	po::options_description all("Allowed options");
//	all.add(desc);
//
//	po::variables_map vm;
//	po::store(po::parse_command_line(ac, av, all), vm);
//
//	if (vm.count("help"))
//	{
//		std::cout << desc << "\n";
//		exit(0);
//	}
//
//	if (vm.count("T"))
//	{
//		const std::string &s = vm["T"].as<std::string>();
//		if (s == "GL")
//		{
//			rendererVersion = RendererVersion::GL;
//		}
//		else if (s == "VK")
//		{
//			rendererVersion = RendererVersion::VULKAN;
//		}
//		else
//		{
//			std::cout << "invalid renderer version value";
//			exit(0);
//		}
//	}
//}
void parseArgument(int ac, char **av)
{
	for (int i = 0; i < ac; ++i)
	{
		LOG(av[i]);
	}
	if (ac > 1)
	{
		if (strcmp(av[1], "GL") == 0)
		{
			rendererVersion = Shit::RendererVersion::GL;
		}
		else if (strcmp(av[1], "VK") == 0)
		{
			rendererVersion = Shit::RendererVersion::VULKAN;
		}
	}
}

void convert2DToCubemap(
	Device *pDevice,
	Image *pSrcImage2D,
	ImageLayout srcInitialLayout,
	ImageLayout srcFinalLayout,
	Image *pDstImageCube,
	ImageLayout dstInitialLayout,
	ImageLayout dstFinalLayout)
{
	auto cubemapWidth = pDstImageCube->GetCreateInfoPtr()->extent.width;
	//==
	//create sampler
	Sampler *linearSampler = pDevice->Create(SamplerCreateInfo{
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

	//create image view
	ImageView *pSrcImageView2D = pDevice->Create(ImageViewCreateInfo{
		pSrcImage2D,
		ImageViewType::TYPE_2D,
		pSrcImage2D->GetCreateInfoPtr()->format,
		{},
		{0, pSrcImage2D->GetCreateInfoPtr()->mipLevels, 0, 1}});
	ImageView *pDstImageViewCube = pDevice->Create(ImageViewCreateInfo{
		pDstImageCube,
		ImageViewType::TYPE_CUBE,
		pDstImageCube->GetCreateInfoPtr()->format,
		{},
		{0, 1, 0, 6}});

	//============
	std::vector<DescriptorSetLayoutBinding> cubemapComputeBindings{
		DescriptorSetLayoutBinding{0, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::COMPUTE_BIT},
		DescriptorSetLayoutBinding{1, DescriptorType::STORAGE_IMAGE, 1, ShaderStageFlagBits::COMPUTE_BIT},
	};
	//cubemap rendering descriptors
	std::vector<DescriptorSetLayout *> setLayouts = {
		pDevice->Create(DescriptorSetLayoutCreateInfo{cubemapComputeBindings}),
	};
	//create pipeline layout
	PipelineLayout *pipelineLayoutGenerateCubemap = pDevice->Create(PipelineLayoutCreateInfo{{setLayouts[0]}});

	//setup descriptor pool
	std::vector<DescriptorPoolSize> poolSizes{
		{cubemapComputeBindings[0].descriptorType, cubemapComputeBindings[0].descriptorCount},
		{cubemapComputeBindings[1].descriptorType, cubemapComputeBindings[1].descriptorCount},
	};
	DescriptorPoolCreateInfo descriptorPoolCreateInfo{
		1u,
		poolSizes};
	DescriptorPool *descriptorPool = pDevice->Create(descriptorPoolCreateInfo);

	//allocate compute descriptor set
	DescriptorSetAllocateInfo allocInfo{{setLayouts[0]}};
	std::vector<DescriptorSet *> compDescriptorSets;
	descriptorPool->Allocate(allocInfo, compDescriptorSets);

	//update descriptor set
	std::vector<WriteDescriptorSet> writes;
	writes.emplace_back(
		WriteDescriptorSet{
			compDescriptorSets[0],
			0,
			0,
			DescriptorType::COMBINED_IMAGE_SAMPLER,
			std::vector<DescriptorImageInfo>{{linearSampler,
											  pSrcImageView2D,
											  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
	writes.emplace_back(
		WriteDescriptorSet{
			compDescriptorSets[0],
			1,
			0,
			DescriptorType::STORAGE_IMAGE,
			std::vector<DescriptorImageInfo>{{nullptr,
											  pDstImageViewCube,
											  ImageLayout::GENERAL}}});
	pDevice->UpdateDescriptorSets(writes, {});

	//=======================================================
	//equirectangular2cube shader
	static const char *compShaderNameGenerateCubemap = "equirectangular2cube.comp.spv";
	std::string compShaderPath = buildShaderPath(compShaderNameGenerateCubemap, rendererVersion);
	Shader *compShaderGenerateCubemap = pDevice->Create(ShaderCreateInfo{readFile(compShaderPath.c_str())});

	//generate cubemap pipeline
	std::vector<uint32_t> constantIDs = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X};
	std::vector<uint32_t> constantValues = {cubemapWidth};

	Pipeline *pipelineGenerateCubemap = pDevice->Create(ComputePipelineCreateInfo{
		PipelineShaderStageCreateInfo{
			PipelineShaderStageCreateInfo{
				ShaderStageFlagBits::COMPUTE_BIT,
				compShaderGenerateCubemap,
				"main",
				{constantIDs,
				 constantValues}},
		},
		pipelineLayoutGenerateCubemap});

	//commandbuffer
	std::vector<ImageMemoryBarrier> imageBarriers;

	executeOneTimeCommands(pDevice, QueueFlagBits::COMPUTE_BIT, 0, [&](CommandBuffer *cmdBuffer) {
		if (srcInitialLayout != ImageLayout::SHADER_READ_ONLY_OPTIMAL)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{
					{},
					AccessFlagBits::SHADER_READ_BIT,
					srcInitialLayout,
					ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					pSrcImage2D,
					{0, 1, 0, 6}});
		}
		if (dstInitialLayout != ImageLayout::GENERAL)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{
					{},
					AccessFlagBits::SHADER_WRITE_BIT,
					dstInitialLayout,
					ImageLayout::GENERAL,
					pDstImageCube,
					{0, 1, 0, 6}});
		}
		//change image layout to general
		cmdBuffer->PipeplineBarrier(PipelineBarrierInfo{
			PipelineStageFlagBits::TOP_OF_PIPE_BIT,
			PipelineStageFlagBits::COMPUTE_SHADER_BIT,
			{},
			0,
			nullptr,
			0,
			nullptr,
			static_cast<uint32_t>(imageBarriers.size()),
			imageBarriers.data()});

		cmdBuffer->BindPipeline(BindPipelineInfo{
			PipelineBindPoint::COMPUTE,
			pipelineGenerateCubemap});
		cmdBuffer->BindDescriptorSets(BindDescriptorSetsInfo{
			PipelineBindPoint::COMPUTE,
			pipelineLayoutGenerateCubemap,
			0,
			1,
			&compDescriptorSets[0],
		});
		cmdBuffer->Dispatch({cubemapWidth, 6, 1});

		//convert to Shader read only
		imageBarriers.clear();
		if (srcFinalLayout != ImageLayout::SHADER_READ_ONLY_OPTIMAL)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::SHADER_READ_BIT,
					AccessFlagBits::SHADER_READ_BIT,
					ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					srcFinalLayout,
					pSrcImage2D,
					{0, 1, 0, 6}});
		}
		if (dstFinalLayout != ImageLayout::GENERAL)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::SHADER_WRITE_BIT,
					AccessFlagBits::SHADER_READ_BIT,
					ImageLayout::GENERAL,
					dstFinalLayout,
					pDstImageCube,
					{0, 1, 0, 6}});
		}
		if (pSrcImage2D->GetCreateInfoPtr()->mipLevels > 1 && srcInitialLayout != srcFinalLayout)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::SHADER_READ_BIT,
					AccessFlagBits::SHADER_READ_BIT,
					srcInitialLayout,
					srcFinalLayout,
					pSrcImage2D,
					{1, pSrcImage2D->GetCreateInfoPtr()->mipLevels - 1, 0, 6}});
		}
		if (pDstImageCube->GetCreateInfoPtr()->mipLevels > 1 && dstInitialLayout != dstFinalLayout)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::SHADER_WRITE_BIT,
					AccessFlagBits::SHADER_READ_BIT,
					dstInitialLayout,
					dstFinalLayout,
					pDstImageCube,
					{1, pDstImageCube->GetCreateInfoPtr()->mipLevels - 1, 0, 6}});
		}
		//change image layout to general
		cmdBuffer->PipeplineBarrier(PipelineBarrierInfo{
			PipelineStageFlagBits::COMPUTE_SHADER_BIT,
			PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
			{},
			0,
			nullptr,
			0,
			nullptr,
			static_cast<uint32_t>(imageBarriers.size()),
			imageBarriers.data()});
	});

	pDevice->Destroy(pSrcImageView2D);
	pDevice->Destroy(pDstImageViewCube);
	pDevice->Destroy(linearSampler);
	pDevice->Destroy(pipelineLayoutGenerateCubemap);
	//pDevice->Destroy(descriptorPool);
	pDevice->Destroy(compShaderGenerateCubemap);

	pDstImageCube->GenerateMipmaps(Filter::LINEAR, dstFinalLayout, dstFinalLayout);
}
void generateIrradianceMap(
	Device *pDevice,
	Image *pSrcImage2D,
	ImageLayout srcInitialLayout,
	ImageLayout srcFinalLayout,
	Image *pDstImageCube,
	ImageLayout dstInitialLayout,
	ImageLayout dstFinalLayout)
{
	auto dstImageWidth = pDstImageCube->GetCreateInfoPtr()->extent.width;
	//==
	//create sampler
	Sampler *linearSampler = pDevice->Create(SamplerCreateInfo{
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
	//create image view
	ImageView *pSrcImageView2D = pDevice->Create(ImageViewCreateInfo{
		pSrcImage2D,
		ImageViewType::TYPE_2D,
		pSrcImage2D->GetCreateInfoPtr()->format,
		{},
		{0, pSrcImage2D->GetCreateInfoPtr()->mipLevels, 0, 1}});
	ImageView *pDstImageViewCube = pDevice->Create(ImageViewCreateInfo{
		pDstImageCube,
		ImageViewType::TYPE_CUBE,
		pDstImageCube->GetCreateInfoPtr()->format,
		{},
		{0, 1, 0, 6}});

	//===============================================
	//descriptor sets
	std::vector<DescriptorSetLayoutBinding> cubemapComputeBindings{
		DescriptorSetLayoutBinding{0, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::COMPUTE_BIT},
		DescriptorSetLayoutBinding{1, DescriptorType::STORAGE_IMAGE, 1, ShaderStageFlagBits::COMPUTE_BIT},
	};
	std::vector<DescriptorSetLayout *> setLayouts = {
		pDevice->Create(DescriptorSetLayoutCreateInfo{cubemapComputeBindings}),
	};
	//create pipeline layout
	PipelineLayout *pipelineLayoutGenerateCubemap = pDevice->Create(PipelineLayoutCreateInfo{{setLayouts[0]}});

	//setup descriptor pool
	std::vector<DescriptorPoolSize> poolSizes{
		{cubemapComputeBindings[0].descriptorType, cubemapComputeBindings[0].descriptorCount},
		{cubemapComputeBindings[1].descriptorType, cubemapComputeBindings[1].descriptorCount},
	};
	DescriptorPoolCreateInfo descriptorPoolCreateInfo{
		1u,
		poolSizes};
	DescriptorPool *descriptorPool = pDevice->Create(descriptorPoolCreateInfo);

	//allocate compute descriptor set
	DescriptorSetAllocateInfo allocInfo{{setLayouts[0]}};
	std::vector<DescriptorSet *> compDescriptorSets;
	descriptorPool->Allocate(allocInfo, compDescriptorSets);

	//update descriptor set
	std::vector<WriteDescriptorSet> writes;
	writes.emplace_back(
		WriteDescriptorSet{
			compDescriptorSets[0],
			0,
			0,
			DescriptorType::COMBINED_IMAGE_SAMPLER,
			std::vector<DescriptorImageInfo>{{linearSampler,
											  pSrcImageView2D,
											  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
	writes.emplace_back(
		WriteDescriptorSet{
			compDescriptorSets[0],
			1,
			0,
			DescriptorType::STORAGE_IMAGE,
			std::vector<DescriptorImageInfo>{{nullptr,
											  pDstImageViewCube,
											  ImageLayout::GENERAL}}});
	pDevice->UpdateDescriptorSets(writes, {});
	//=============================
	//shader
	const char *compShaderNameGenerateCubemap = "irradiance.comp.spv";
	std::string compShaderPath = buildShaderPath(compShaderNameGenerateCubemap, rendererVersion);
	Shader *compShaderGenerateCubemap = pDevice->Create(ShaderCreateInfo{readFile(compShaderPath.c_str())});

	//generate cubemap pipeline
	std::vector<uint32_t> constantIDs = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X, 3};
	std::vector<uint32_t> constantValues = {dstImageWidth, pDstImageCube->GetCreateInfoPtr()->extent.width};

	Pipeline *pipelineGenerateCubemap = pDevice->Create(ComputePipelineCreateInfo{
		PipelineShaderStageCreateInfo{
			PipelineShaderStageCreateInfo{
				ShaderStageFlagBits::COMPUTE_BIT,
				compShaderGenerateCubemap,
				"main",
				{constantIDs, constantValues}},
		},
		pipelineLayoutGenerateCubemap});

	//commandbuffer
	executeOneTimeCommands(pDevice, QueueFlagBits::TRANSFER_BIT, 0, [&](CommandBuffer *cmdBuffer) {
		//convert dst image layout to general
		std::vector<ImageMemoryBarrier> imageBarriers;
		if (srcInitialLayout != ImageLayout::SHADER_READ_ONLY_OPTIMAL)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{{},
								   AccessFlagBits::SHADER_READ_BIT,
								   srcInitialLayout,
								   ImageLayout::SHADER_READ_ONLY_OPTIMAL,
								   pSrcImage2D,
								   ImageSubresourceRange{0, pSrcImage2D->GetCreateInfoPtr()->mipLevels, 0, 6}});
		}
		if (dstInitialLayout != ImageLayout::GENERAL)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{{},
								   AccessFlagBits::SHADER_WRITE_BIT,
								   dstInitialLayout,
								   ImageLayout::GENERAL,
								   pDstImageCube,
								   ImageSubresourceRange{0, 1, 0, 6}});
		}

		//change image layout to general
		cmdBuffer->PipeplineBarrier(PipelineBarrierInfo{
			PipelineStageFlagBits::TOP_OF_PIPE_BIT,
			PipelineStageFlagBits::COMPUTE_SHADER_BIT,
			{},
			0,
			nullptr,
			0,
			nullptr,
			static_cast<uint32_t>(imageBarriers.size()),
			imageBarriers.data()});
		//
		cmdBuffer->BindPipeline(BindPipelineInfo{
			PipelineBindPoint::COMPUTE,
			pipelineGenerateCubemap});
		cmdBuffer->BindDescriptorSets(BindDescriptorSetsInfo{
			PipelineBindPoint::COMPUTE,
			pipelineLayoutGenerateCubemap,
			0,
			1,
			&compDescriptorSets[0],
		});
		cmdBuffer->Dispatch({dstImageWidth, 6, 1});

		imageBarriers.clear();
		if (srcFinalLayout != ImageLayout::SHADER_READ_ONLY_OPTIMAL)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{AccessFlagBits::SHADER_READ_BIT,
								   AccessFlagBits::SHADER_READ_BIT,
								   ImageLayout::SHADER_READ_ONLY_OPTIMAL,
								   srcFinalLayout,
								   pSrcImage2D,
								   ImageSubresourceRange{0, pSrcImage2D->GetCreateInfoPtr()->mipLevels, 0, 6}});
		}
		if (dstFinalLayout != ImageLayout::GENERAL)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::SHADER_WRITE_BIT,
					AccessFlagBits::SHADER_READ_BIT,
					ImageLayout::GENERAL,
					dstFinalLayout,
					pDstImageCube,
					ImageSubresourceRange{0, 1, 0, 6}});
		}
		if (dstFinalLayout != dstInitialLayout && pDstImageCube->GetCreateInfoPtr()->mipLevels > 1)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{
					{},
					AccessFlagBits::SHADER_READ_BIT,
					dstInitialLayout,
					dstFinalLayout,
					pDstImageCube,
					ImageSubresourceRange{1, pDstImageCube->GetCreateInfoPtr()->mipLevels - 1, 0, 6}});
		}
		//change image layout to general
		cmdBuffer->PipeplineBarrier(PipelineBarrierInfo{
			PipelineStageFlagBits::COMPUTE_SHADER_BIT,
			PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
			{},
			0,
			nullptr,
			0,
			nullptr,
			static_cast<uint32_t>(imageBarriers.size()),
			imageBarriers.data()});
	});
	pDstImageCube->GenerateMipmaps(Filter::LINEAR, dstFinalLayout, dstFinalLayout);
	pDevice->Destroy(pSrcImageView2D);
	pDevice->Destroy(pDstImageViewCube);
	pDevice->Destroy(linearSampler);
	pDevice->Destroy(pipelineLayoutGenerateCubemap);
	pDevice->Destroy(descriptorPool);
	pDevice->Destroy(compShaderGenerateCubemap);
}
void generateIrradianceMapSH(
	Device *pDevice,
	Image *pSrcImage2D,
	ImageLayout srcInitialLayout,
	ImageLayout srcFinalLayout,
	Image *pDstImageCube,
	ImageLayout dstInitialLayout,
	ImageLayout dstFinalLayout)
{
	//create sampler
	Sampler *linearSampler = pDevice->Create(SamplerCreateInfo{
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
	//create image view
	ImageView *pSrcImageView2D = pDevice->Create(ImageViewCreateInfo{
		pSrcImage2D,
		ImageViewType::TYPE_2D,
		pSrcImage2D->GetCreateInfoPtr()->format,
		{},
		{0, pSrcImage2D->GetCreateInfoPtr()->mipLevels, 0, 1}});
	ImageView *pDstImageViewCube = pDevice->Create(ImageViewCreateInfo{
		pDstImageCube,
		ImageViewType::TYPE_CUBE,
		pDstImageCube->GetCreateInfoPtr()->format,
		{},
		{0, 1, 0, 6}});
	//prepare llm image
	Image *llmImage = pDevice->Create(
		ImageCreateInfo{
			{},
			ImageType::TYPE_1D,
			pSrcImage2D->GetCreateInfoPtr()->format,
			{32, 1, 1},
			1,
			1,
			SampleCountFlagBits::BIT_1,
			ImageTiling::OPTIMAL,
			ImageUsageFlagBits::STORAGE_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			ImageLayout::GENERAL},
		nullptr);
	ImageView *llmImageView = pDevice->Create(
		ImageViewCreateInfo{
			llmImage,
			ImageViewType::TYPE_1D,
			pSrcImage2D->GetCreateInfoPtr()->format,
			{},
			{0, 1, 0, 1}});

	//===============================================
	//descriptor sets
	std::vector<DescriptorSetLayoutBinding> llmBindinds{
		DescriptorSetLayoutBinding{0, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::COMPUTE_BIT},
		DescriptorSetLayoutBinding{1, DescriptorType::STORAGE_IMAGE, 1, ShaderStageFlagBits::COMPUTE_BIT},
	};
	std::vector<DescriptorSetLayoutBinding> shBindinds{
		DescriptorSetLayoutBinding{0, DescriptorType::STORAGE_IMAGE, 1, ShaderStageFlagBits::COMPUTE_BIT},
		DescriptorSetLayoutBinding{1, DescriptorType::STORAGE_IMAGE, 1, ShaderStageFlagBits::COMPUTE_BIT},
	};
	std::vector<DescriptorSetLayout *> setLayouts = {
		pDevice->Create(DescriptorSetLayoutCreateInfo{llmBindinds}),
		pDevice->Create(DescriptorSetLayoutCreateInfo{shBindinds}),
	};
	//create pipeline layout
	PipelineLayout *llmPipelineLayout = pDevice->Create(PipelineLayoutCreateInfo{{setLayouts[0]}});
	PipelineLayout *shPipelineLayout = pDevice->Create(PipelineLayoutCreateInfo{{setLayouts[1]}});

	//setup descriptor pool
	std::vector<DescriptorPoolSize> poolSizes{
		{DescriptorType::COMBINED_IMAGE_SAMPLER, 1},
		{DescriptorType::STORAGE_IMAGE, 3},
	};
	DescriptorPoolCreateInfo descriptorPoolCreateInfo{
		2u,
		poolSizes};
	DescriptorPool *descriptorPool = pDevice->Create(descriptorPoolCreateInfo);

	//allocate compute descriptor set
	DescriptorSetAllocateInfo allocInfo{{setLayouts}};
	std::vector<DescriptorSet *> compDescriptorSets;
	descriptorPool->Allocate(allocInfo, compDescriptorSets);

	//update descriptor set
	std::vector<WriteDescriptorSet> writes{
		{compDescriptorSets[0],
		 0,
		 0,
		 DescriptorType::COMBINED_IMAGE_SAMPLER,
		 std::vector<DescriptorImageInfo>{{linearSampler,
										   pSrcImageView2D,
										   ImageLayout::SHADER_READ_ONLY_OPTIMAL}}},
		{compDescriptorSets[0],
		 1,
		 0,
		 DescriptorType::STORAGE_IMAGE,
		 std::vector<DescriptorImageInfo>{{nullptr,
										   llmImageView,
										   ImageLayout::GENERAL}}},
		{compDescriptorSets[1],
		 0,
		 0,
		 DescriptorType::STORAGE_IMAGE,
		 std::vector<DescriptorImageInfo>{{nullptr,
										   llmImageView,
										   ImageLayout::GENERAL}}},
		{compDescriptorSets[1],
		 1,
		 0,
		 DescriptorType::STORAGE_IMAGE,
		 std::vector<DescriptorImageInfo>{{nullptr,
										   pDstImageViewCube,
										   ImageLayout::GENERAL}}},
	};
	pDevice->UpdateDescriptorSets(writes, {});
	//=============================

	//create shader
	static const char *llmCompShaderName = "irradianceSH_Llm.comp.spv";
	static const char *shCompShaderName = "irradianceSH.comp.spv";
	std::string llmShaderPath = buildShaderPath(llmCompShaderName, rendererVersion);
	std::string shShaderPath = buildShaderPath(shCompShaderName, rendererVersion);
	Shader *llmShader = pDevice->Create(ShaderCreateInfo{readFile(llmShaderPath.c_str())});
	Shader *shShader = pDevice->Create(ShaderCreateInfo{readFile(shShaderPath.c_str())});

	//create llm pipeline
	Pipeline *llmPipeline = pDevice->Create(ComputePipelineCreateInfo{
		PipelineShaderStageCreateInfo{
			PipelineShaderStageCreateInfo{
				ShaderStageFlagBits::COMPUTE_BIT,
				llmShader,
				"main",
				{{3}, {pDstImageCube->GetCreateInfoPtr()->extent.width}}}},
		llmPipelineLayout});

	//create sh pipeline
	std::vector<uint32_t> constantIDs = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X};
	std::vector<uint32_t> constantValues = {pDstImageCube->GetCreateInfoPtr()->extent.width};

	Pipeline *shPipeline = pDevice->Create(ComputePipelineCreateInfo{
		PipelineShaderStageCreateInfo{
			PipelineShaderStageCreateInfo{
				ShaderStageFlagBits::COMPUTE_BIT,
				shShader,
				"main",
				{constantIDs,
				 constantValues}},
		},
		shPipelineLayout});

	executeOneTimeCommands(pDevice, QueueFlagBits::COMPUTE_BIT, 0, [&](CommandBuffer *pCommandBuffer) {
		std::vector<ImageMemoryBarrier> barriers;
		if (srcInitialLayout != ImageLayout::SHADER_READ_ONLY_OPTIMAL)
		{
			barriers.emplace_back(ImageMemoryBarrier{
				AccessFlagBits::MEMORY_WRITE_BIT,
				AccessFlagBits::MEMORY_READ_BIT,
				srcInitialLayout,
				ImageLayout::SHADER_READ_ONLY_OPTIMAL,
				pSrcImage2D,
				{0, 1, 0, 1}});
		}
		if (dstInitialLayout != ImageLayout::GENERAL)
		{
			barriers.emplace_back(ImageMemoryBarrier{
				{},
				AccessFlagBits::MEMORY_READ_BIT,
				dstInitialLayout,
				ImageLayout::GENERAL,
				pDstImageCube,
				{0, 1, 0, 6}});
		}
		pCommandBuffer->PipeplineBarrier(PipelineBarrierInfo{
			PipelineStageFlagBits::TOP_OF_PIPE_BIT,
			PipelineStageFlagBits::COMPUTE_SHADER_BIT,
			{},
			0,
			nullptr,
			0,
			nullptr,
			static_cast<uint32_t>(barriers.size()),
			barriers.data()});
		//create llm image
		pCommandBuffer->BindDescriptorSets(BindDescriptorSetsInfo{
			PipelineBindPoint::COMPUTE,
			llmPipelineLayout,
			0,
			1,
			&compDescriptorSets[0],
		});
		pCommandBuffer->BindPipeline(BindPipelineInfo{
			PipelineBindPoint::COMPUTE,
			llmPipeline});
		pCommandBuffer->Dispatch(DispatchInfo{1, 1, 1});
		//create irradiance image

		barriers = {{AccessFlagBits::MEMORY_WRITE_BIT,
					 AccessFlagBits::MEMORY_READ_BIT,
					 ImageLayout::GENERAL,
					 ImageLayout::GENERAL,
					 llmImage,
					 {0, 1, 0, 1}}};
		pCommandBuffer->PipeplineBarrier(
			PipelineBarrierInfo{
				PipelineStageFlagBits::COMPUTE_SHADER_BIT,
				PipelineStageFlagBits::COMPUTE_SHADER_BIT,
				{},
				0,
				nullptr,
				0,
				nullptr,
				static_cast<uint32_t>(barriers.size()),
				barriers.data()});
		pCommandBuffer->BindDescriptorSets(BindDescriptorSetsInfo{
			PipelineBindPoint::COMPUTE,
			shPipelineLayout,
			0,
			1,
			&compDescriptorSets[1],
		});
		pCommandBuffer->BindPipeline(BindPipelineInfo{
			PipelineBindPoint::COMPUTE,
			shPipeline});
		pCommandBuffer->Dispatch(DispatchInfo{pDstImageCube->GetCreateInfoPtr()->extent.width, 6, 1});

		barriers.clear();
		if (srcFinalLayout != ImageLayout::SHADER_READ_ONLY_OPTIMAL)
		{
			barriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::MEMORY_READ_BIT,
					AccessFlagBits::MEMORY_READ_BIT,
					ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					srcFinalLayout,
					pSrcImage2D,
					{0, 1, 0, 1}});
		}
		if (srcInitialLayout != srcFinalLayout)
		{
			barriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::MEMORY_READ_BIT,
					AccessFlagBits::MEMORY_READ_BIT,
					srcInitialLayout,
					srcFinalLayout,
					pSrcImageView2D->GetCreateInfoPtr()->pImage,
					{1, pSrcImageView2D->GetCreateInfoPtr()->pImage->GetCreateInfoPtr()->mipLevels - 1, 0, 1}});
		}
		if (dstFinalLayout != ImageLayout::GENERAL)
		{
			barriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::MEMORY_READ_BIT,
					AccessFlagBits::MEMORY_READ_BIT,
					ImageLayout::GENERAL,
					dstFinalLayout,
					pDstImageViewCube->GetCreateInfoPtr()->pImage,
					{0, 1, 0, 6}});
		}
		if (dstInitialLayout != dstFinalLayout)
		{
			barriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::MEMORY_READ_BIT,
					AccessFlagBits::MEMORY_READ_BIT,
					dstInitialLayout,
					dstFinalLayout,
					pDstImageCube,
					{1, pDstImageCube->GetCreateInfoPtr()->mipLevels - 1, 0, 6}});
		}
		pCommandBuffer->PipeplineBarrier(PipelineBarrierInfo{
			PipelineStageFlagBits::COMPUTE_SHADER_BIT,
			PipelineStageFlagBits::ALL_COMMANDS_BIT,
			{},
			0,
			nullptr,
			0,
			nullptr,
			static_cast<uint32_t>(barriers.size()),
			barriers.data()});
	});

	takeScreenshot(pDevice, llmImage, ImageLayout::GENERAL);
	std::this_thread::sleep_for(std::chrono::seconds(1));

	pDstImageCube->GenerateMipmaps(Filter::LINEAR, dstFinalLayout, dstFinalLayout);
	//release resouce
	pDevice->Destroy(pSrcImageView2D);
	pDevice->Destroy(pDstImageViewCube);
	pDevice->Destroy(llmImageView);
	pDevice->Destroy(llmImage);
	pDevice->Destroy(llmPipelineLayout);
	pDevice->Destroy(llmPipeline);
	pDevice->Destroy(shPipelineLayout);
	pDevice->Destroy(shPipeline);
}
void generatePrefilteredEnvMap(
	Device *pDevice,
	uint32_t maxRoughnessLevelNum,
	Image *pSrcImage2D,
	ImageLayout srcInitialLayout,
	ImageLayout srcFinalLayout,
	Image *pDstImageCube,
	ImageLayout dstInitialLayout,
	ImageLayout dstFinalLayout)
{
	//==
	//create sampler
	Sampler *linearSampler = pDevice->Create(SamplerCreateInfo{
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
	//=============================================
	auto maxLevelNum = (std::min)(maxRoughnessLevelNum + 1, pDstImageCube->GetCreateInfoPtr()->mipLevels);

	ImageViewCreateInfo imageViewCreateInfo{
		pSrcImage2D,
		ImageViewType::TYPE_2D,
		pSrcImage2D->GetCreateInfoPtr()->format,
		{},
		{0, pSrcImage2D->GetCreateInfoPtr()->mipLevels, 0, 1}};
	ImageView *pSrcImageView2D = pDevice->Create(imageViewCreateInfo);

	//create descriptor set
	std::vector<DescriptorSetLayoutBinding> cubemapComputeBindings{
		DescriptorSetLayoutBinding{0, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::COMPUTE_BIT},
		DescriptorSetLayoutBinding{1, DescriptorType::STORAGE_IMAGE, 1, ShaderStageFlagBits::COMPUTE_BIT},
	};
	DescriptorSetLayout *setLayout = pDevice->Create(DescriptorSetLayoutCreateInfo{cubemapComputeBindings});
	//create pipeline layout
	PipelineLayout *pipelineLayout = pDevice->Create(PipelineLayoutCreateInfo{{setLayout}});

	//setup descriptor pool
	std::vector<DescriptorPoolSize> poolSizes{
		{cubemapComputeBindings[0].descriptorType, cubemapComputeBindings[0].descriptorCount},
		{cubemapComputeBindings[1].descriptorType, cubemapComputeBindings[1].descriptorCount},
	};
	DescriptorPoolCreateInfo descriptorPoolCreateInfo{
		maxLevelNum,
		poolSizes};
	DescriptorPool *descriptorPool = pDevice->Create(descriptorPoolCreateInfo);

	//allocate compute descriptor set
	DescriptorSetAllocateInfo allocInfo{std::vector(maxLevelNum, setLayout)};
	std::vector<DescriptorSet *> descriptorSets;
	descriptorPool->Allocate(allocInfo, descriptorSets);

	std::vector<ImageView *> pDstImageViews(maxLevelNum);
	imageViewCreateInfo.viewType = ImageViewType::TYPE_CUBE;
	imageViewCreateInfo.pImage = pDstImageCube;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.layerCount = 6;

	//=================================================================================
	// create shader module
	const char *prefilterShaderName = "prefilterEnvMap.comp.spv";
	std::string prefilterShaderPath = buildShaderPath(prefilterShaderName, rendererVersion);
	Shader *prefilterShader = pDevice->Create(ShaderCreateInfo{readFile(prefilterShaderPath.c_str())});

	std::vector<Pipeline *> pipelines(maxLevelNum);
	uint32_t width = pDstImageCube->GetCreateInfoPtr()->extent.width;

	std::vector<WriteDescriptorSet> writes;

	for (int i = 0; i < maxLevelNum; ++i, width /= 2)
	{
		//update descriptor set
		imageViewCreateInfo.subresourceRange.baseMipLevel = i;
		pDstImageViews[i] = pDevice->Create(imageViewCreateInfo);
		if (i == 0)
		{
			//equirectangular2cube shader
			static const char *compShaderNameGenerateCubemap = "equirectangular2cube.comp.spv";
			std::string compShaderPath = buildShaderPath(compShaderNameGenerateCubemap, rendererVersion);
			Shader *compShaderGenerateCubemap = pDevice->Create(ShaderCreateInfo{readFile(compShaderPath.c_str())});

			//generate cubemap pipeline
			std::vector<uint32_t> constantIDs0 = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X};
			std::vector<uint32_t> constantValues0 = {width};

			pipelines[i] = pDevice->Create(ComputePipelineCreateInfo{
				PipelineShaderStageCreateInfo{
					PipelineShaderStageCreateInfo{
						ShaderStageFlagBits::COMPUTE_BIT,
						compShaderGenerateCubemap,
						"main",
						{constantIDs0, constantValues0}},
				},
				pipelineLayout});
		}
		else
		{
			//create pipeline
			std::vector<uint32_t> constantIDs = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X, 3, 4};
			std::vector<uint32_t> constantValues = {width, maxLevelNum - 1, pDstImageCube->GetCreateInfoPtr()->extent.width};

			pipelines[i] = pDevice->Create(ComputePipelineCreateInfo{
				PipelineShaderStageCreateInfo{
					PipelineShaderStageCreateInfo{
						ShaderStageFlagBits::COMPUTE_BIT,
						prefilterShader,
						"main",
						{constantIDs, constantValues}},
				},
				pipelineLayout});
		}
		writes.emplace_back(
			WriteDescriptorSet{descriptorSets[i],
							   0,
							   0,
							   DescriptorType::COMBINED_IMAGE_SAMPLER,
							   std::vector<DescriptorImageInfo>{{linearSampler,
																 pSrcImageView2D,
																 ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
		writes.emplace_back(WriteDescriptorSet{
			descriptorSets[i],
			1,
			0,
			DescriptorType::STORAGE_IMAGE,
			std::vector<DescriptorImageInfo>{{nullptr,
											  pDstImageViews[i],
											  ImageLayout::GENERAL}}});
	}
	pDevice->UpdateDescriptorSets(writes, {});

	//process other levels
	executeOneTimeCommands(pDevice, QueueFlagBits::COMPUTE_BIT, 0, [&](CommandBuffer *pCommandBuffer) {
		std::vector<ImageMemoryBarrier> barriers;

		//===================================
		if (srcInitialLayout != ImageLayout::SHADER_READ_ONLY_OPTIMAL)
		{
			barriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::MEMORY_WRITE_BIT,
					AccessFlagBits::SHADER_READ_BIT,
					srcInitialLayout,
					ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					pSrcImage2D,
					{0, pSrcImage2D->GetCreateInfoPtr()->mipLevels, 0, 1}});
		}
		if (dstInitialLayout != ImageLayout::GENERAL)
		{
			barriers.emplace_back(
				ImageMemoryBarrier{{},
								   AccessFlagBits::SHADER_WRITE_BIT,
								   dstInitialLayout,
								   ImageLayout::GENERAL,
								   pDstImageCube,
								   {0, pDstImageCube->GetCreateInfoPtr()->mipLevels, 0, 6}});
		}
		pCommandBuffer->PipeplineBarrier(PipelineBarrierInfo{
			PipelineStageFlagBits::COMPUTE_SHADER_BIT,
			PipelineStageFlagBits::COMPUTE_SHADER_BIT,
			{},
			0,
			nullptr,
			0,
			nullptr,
			static_cast<uint32_t>(barriers.size()),
			barriers.data()});

		//process other levels
		width = pDstImageCube->GetCreateInfoPtr()->extent.width;
		for (uint32_t i = 0; i < maxLevelNum; ++i, width /= 2)
		{
			pCommandBuffer->BindDescriptorSets(BindDescriptorSetsInfo{
				PipelineBindPoint::COMPUTE,
				pipelineLayout,
				0,
				1,
				&descriptorSets[i],
			});
			pCommandBuffer->BindPipeline(BindPipelineInfo{
				PipelineBindPoint::COMPUTE,
				pipelines[i]});
			pCommandBuffer->Dispatch(DispatchInfo{width, 6, 1});
		}

		//restore image layout
		//src already changed to shader read only ,do not change again
		barriers.clear();
		if (srcFinalLayout != ImageLayout::SHADER_READ_ONLY_OPTIMAL)
		{
			barriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::SHADER_READ_BIT,
					AccessFlagBits::SHADER_READ_BIT,
					ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					srcFinalLayout,
					pSrcImage2D,
					{0, pSrcImage2D->GetCreateInfoPtr()->mipLevels, 0, 1}});
		}
		if (dstFinalLayout != ImageLayout::GENERAL)
		{
			barriers.emplace_back(
				ImageMemoryBarrier{AccessFlagBits::SHADER_WRITE_BIT,
								   AccessFlagBits::MEMORY_READ_BIT,
								   ImageLayout::GENERAL,
								   dstFinalLayout,
								   pDstImageCube,
								   {0, pDstImageCube->GetCreateInfoPtr()->mipLevels, 0, 6}});
		}
		pCommandBuffer->PipeplineBarrier(PipelineBarrierInfo{
			PipelineStageFlagBits::TRANSFER_BIT | PipelineStageFlagBits::COMPUTE_SHADER_BIT,
			PipelineStageFlagBits::ALL_COMMANDS_BIT,
			{},
			0,
			nullptr,
			0,
			nullptr,
			static_cast<uint32_t>(barriers.size()),
			barriers.data()});
	});
	pDevice->Destroy(descriptorPool);
	pDevice->Destroy(pSrcImageView2D);
	for (auto e : pDstImageViews)
		pDevice->Destroy(e);
	pDevice->Destroy(pipelineLayout);
	pDevice->Destroy(setLayout);
	for (auto e : pipelines)
		pDevice->Destroy(e);
}