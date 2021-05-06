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
void saveImage(const char *imagePath, int width, int height, int component, const void *data)
{
	if (!stbi_write_jpg(imagePath, width, height, component, data, 90))
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
#if 1
		//copy to buffer
		//		 cannot change format

		std::vector<Buffer *> dst(levels);
		uint32_t tempWidth = width, tempHeight = height, tempDepth = depth;
		for (uint32_t i = 0; i < levels; ++i,
					  tempWidth = std::max(tempWidth / 2, 1u),
					  tempHeight = std::max(tempHeight / 2, 1u),
					  tempDepth = std::max(tempDepth / 2, 1u))
		{
			auto size = tempWidth * tempHeight * tempDepth * component * layers;
			dst[i] = pDevice->Create(
				BufferCreateInfo{
					{},
					size,
					BufferUsageFlagBits::TRANSFER_DST_BIT,
					MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
				nullptr);
		}

		pDevice->ExecuteOneTimeCommands([&](CommandBuffer *cmdBuffer) {
			ImageMemoryBarrier imageBarrier0{
				AccessFlagBits::MEMORY_READ_BIT,
				AccessFlagBits::TRANSFER_READ_BIT,
				imageLayout,
				ImageLayout::TRANSFER_SRC_OPTIMAL,
				pImage,
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
				&imageBarrier0});

			uint32_t tempWidth = width, tempHeight = height, tempDepth = depth;
			for (uint32_t i = 0; i < levels; ++i,
						  tempWidth = std::max(tempWidth / 2, 1u),
						  tempHeight = std::max(tempHeight / 2, 1u),
						  tempDepth = std::max(tempDepth / 2, 1u))
			{
				auto size = tempWidth * tempHeight * tempDepth * component * layers;
				BufferImageCopy copyRegion{
					.imageSubresource = {i, 0, layers},
					.imageExtent = {tempWidth, tempHeight, tempDepth}};
				cmdBuffer->CopyImageToBuffer(CopyImageToBufferInfo{
					pImage,
					dst[i],
					1,
					&copyRegion});
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
				PipelineStageFlagBits::TRANSFER_BIT,
				{},
				0,
				nullptr,
				0,
				nullptr,
				1,
				&imageBarrier1});
		});

#else
		//copy to image

		Image *dst = pDevice->Create(
			ImageCreateInfo{
				{},
				ImageType::TYPE_2D,
				ShitFormat::RGBA8_UNORM,
				{width, height, depth},
				1,
				layers,
				SampleCountFlagBits::BIT_1,
				ImageTiling::LINEAR,
				ImageUsageFlagBits::TRANSFER_DST_BIT,
				MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT,
			},
			nullptr);
		ImageCopy copyRegion{
			.srcSubresource = {0, 0, layers},
			.dstSubresource = {0, 0, layers},
			.extent = {width, height, depth}};
		pDevice->ExecuteOneTimeCommands([&](CommandBuffer *cmdBuffer) {
			ImageMemoryBarrier pImageBarrier0{
				AccessFlagBits::MEMORY_READ_BIT,
				AccessFlagBits::TRANSFER_READ_BIT,
				pImage->GetCreateInfoPtr()->initialLayout,
				ImageLayout::TRANSFER_SRC_OPTIMAL,
				pImage,
				ImageSubresourceRange{0, 1, 0, layers}};
			ImageMemoryBarrier dstImageBarrier0{
				{},
				AccessFlagBits::TRANSFER_WRITE_BIT,
				ImageLayout::UNDEFINED,
				ImageLayout::TRANSFER_DST_OPTIMAL,
				dst,
				ImageSubresourceRange{0, 1, 0, layers}};
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
				1,
				&copyRegion});
			ImageMemoryBarrier pImageBarrier1{
				AccessFlagBits::TRANSFER_READ_BIT,
				AccessFlagBits::MEMORY_READ_BIT,
				ImageLayout::TRANSFER_SRC_OPTIMAL,
				pImage->GetCreateInfoPtr()->initialLayout,
				pImage,
				ImageSubresourceRange{0, 1, 0, layers}};
			ImageMemoryBarrier dstImageBarrier1{
				AccessFlagBits::TRANSFER_WRITE_BIT,
				AccessFlagBits::MEMORY_READ_BIT,
				ImageLayout::TRANSFER_DST_OPTIMAL,
				ImageLayout::GENERAL,
				dst,
				ImageSubresourceRange{0, 1, 0, layersj}};
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
			auto size = tempWidth * tempHeight * tempDepth * component * layers;
			void *data;
			dst[j]->MapMemory(0, size, &data);
			//swizzleColor
			if (
				pImage->GetCreateInfoPtr()->format == ShitFormat::BGRA8_SRGB ||
				pImage->GetCreateInfoPtr()->format == ShitFormat::BGRA8_UNORM)
			{
				unsigned char *p = reinterpret_cast<unsigned char *>(data);
				unsigned char temp;
				for (uint32_t i = 0; i < dst[j]->GetCreateInfoPtr()->size; i += 4, p += 4)
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
			ss << ".jpg";
			saveImage(ss.str().c_str(), tempWidth, tempHeight * tempDepth * layers, component, data);
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
	ImageView *pSrcImageView2D,
	ImageLayout srcInitialLayout,
	ImageLayout srcFinalLayout,
	ImageView *pDstImageViewCube,
	ImageLayout dstInitialLayout,
	ImageLayout dstFinalLayout)
{
	auto cubemapWidth = pDstImageViewCube->GetCreateInfoPtr()->pImage->GetCreateInfoPtr()->extent.width;
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
	const char *compShaderNameGenerateCubemap = "equirectangular2cube.comp.spv";
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

	pDevice->ExecuteOneTimeCommands([&](CommandBuffer *cmdBuffer) {
		if (srcInitialLayout != ImageLayout::SHADER_READ_ONLY_OPTIMAL)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{
					{},
					AccessFlagBits::SHADER_READ_BIT,
					srcInitialLayout,
					ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					pSrcImageView2D->GetCreateInfoPtr()->pImage,
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
					pDstImageViewCube->GetCreateInfoPtr()->pImage,
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
					pSrcImageView2D->GetCreateInfoPtr()->pImage,
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
					pDstImageViewCube->GetCreateInfoPtr()->pImage,
					{0, 1, 0, 6}});
		}
		if (pSrcImageView2D->GetCreateInfoPtr()->pImage->GetCreateInfoPtr()->mipLevels > 1 && srcInitialLayout != srcFinalLayout)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::SHADER_READ_BIT,
					AccessFlagBits::SHADER_READ_BIT,
					srcInitialLayout,
					srcFinalLayout,
					pSrcImageView2D->GetCreateInfoPtr()->pImage,
					{1, pSrcImageView2D->GetCreateInfoPtr()->pImage->GetCreateInfoPtr()->mipLevels - 1, 0, 6}});
		}
		if (pDstImageViewCube->GetCreateInfoPtr()->pImage->GetCreateInfoPtr()->mipLevels > 1 && dstInitialLayout != dstFinalLayout)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::SHADER_WRITE_BIT,
					AccessFlagBits::SHADER_READ_BIT,
					dstInitialLayout,
					dstFinalLayout,
					pDstImageViewCube->GetCreateInfoPtr()->pImage,
					{1, pDstImageViewCube->GetCreateInfoPtr()->pImage->GetCreateInfoPtr()->mipLevels - 1, 0, 6}});
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

	pDevice->Destroy(linearSampler);
	pDevice->Destroy(pipelineLayoutGenerateCubemap);
	//pDevice->Destroy(descriptorPool);
	pDevice->Destroy(compShaderGenerateCubemap);
}
void generateIrradianceMap(
	Device *pDevice,
	ImageView *pSrcImageViewCube,
	ImageLayout srcInitialLayout,
	ImageLayout srcFinalLayout,
	ImageView *pDstImageViewCube,
	ImageLayout dstInitialLayout,
	ImageLayout dstFinalLayout)
{
	auto dstImageWidth = pDstImageViewCube->GetCreateInfoPtr()->pImage->GetCreateInfoPtr()->extent.width;
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
											  pSrcImageViewCube,
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
	std::vector<uint32_t> constantIDs = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X};
	std::vector<uint32_t> constantValues = {dstImageWidth};

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
	pDevice->ExecuteOneTimeCommands([&](CommandBuffer *cmdBuffer) {
		//convert dst image layout to general
		std::vector<ImageMemoryBarrier> imageBarriers;
		if (srcInitialLayout != ImageLayout::SHADER_READ_ONLY_OPTIMAL)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{{},
								   AccessFlagBits::SHADER_READ_BIT,
								   srcInitialLayout,
								   ImageLayout::SHADER_READ_ONLY_OPTIMAL,
								   pSrcImageViewCube->GetCreateInfoPtr()->pImage,
								   ImageSubresourceRange{0, pSrcImageViewCube->GetCreateInfoPtr()->pImage->GetCreateInfoPtr()->mipLevels, 0, 6}});
		}
		if(dstInitialLayout!=ImageLayout::GENERAL)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{{},
								   AccessFlagBits::SHADER_WRITE_BIT,
								   dstInitialLayout,
								   ImageLayout::GENERAL,
								   pDstImageViewCube->GetCreateInfoPtr()->pImage,
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
								   pSrcImageViewCube->GetCreateInfoPtr()->pImage,
								   ImageSubresourceRange{0, pSrcImageViewCube->GetCreateInfoPtr()->pImage->GetCreateInfoPtr()->mipLevels, 0, 6}});
		}
		if (dstFinalLayout != ImageLayout::GENERAL)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::SHADER_WRITE_BIT,
					AccessFlagBits::SHADER_READ_BIT,
					ImageLayout::GENERAL,
					dstFinalLayout,
					pDstImageViewCube->GetCreateInfoPtr()->pImage,
					ImageSubresourceRange{0, 1, 0, 6}});
		}
		if (dstFinalLayout != dstInitialLayout && pDstImageViewCube->GetCreateInfoPtr()->pImage->GetCreateInfoPtr()->mipLevels > 1)
		{
			imageBarriers.emplace_back(
				ImageMemoryBarrier{
					{},
					AccessFlagBits::SHADER_READ_BIT,
					dstInitialLayout,
					dstFinalLayout,
					pDstImageViewCube->GetCreateInfoPtr()->pImage,
					ImageSubresourceRange{1, pDstImageViewCube->GetCreateInfoPtr()->pImage->GetCreateInfoPtr()->mipLevels - 1, 0, 6}});
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
	pDevice->Destroy(linearSampler);
	pDevice->Destroy(pipelineLayoutGenerateCubemap);
	pDevice->Destroy(descriptorPool);
	pDevice->Destroy(compShaderGenerateCubemap);
}
void generatePrefilteredEnvMap(
	Device *pDevice,
	Image *pSrcImageCube,
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
	auto maxLevelNum = (std::min)(8u, pSrcImageCube->GetCreateInfoPtr()->mipLevels);

	ImageViewCreateInfo imageViewCreateInfo{
		pSrcImageCube,
		ImageViewType::TYPE_CUBE,
		ShitFormat::RGBA8_UNORM,
		{},
		{0, pSrcImageCube->GetCreateInfoPtr()->mipLevels, 0, 6}};
	ImageView *pSrcImageViewCube = pDevice->Create(imageViewCreateInfo);

	//transfer dst image level 0 layout to transfer_dst, other level to GENERRAL
	std::vector<ImageMemoryBarrier> barriers{
		{{},
		 AccessFlagBits::TRANSFER_READ_BIT,
		 srcInitialLayout,
		 ImageLayout::TRANSFER_SRC_OPTIMAL,
		 pSrcImageCube,
		 {0, 1, 0, 6}},
		{{},
		 AccessFlagBits::TRANSFER_WRITE_BIT,
		 dstInitialLayout,
		 ImageLayout::TRANSFER_DST_OPTIMAL,
		 pDstImageCube,
		 {0, 1, 0, 6}}};

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
	imageViewCreateInfo.pImage = pDstImageCube;
	imageViewCreateInfo.subresourceRange.levelCount = 1;

	// create shader module
	const char *compShaderName = "prefilterEnvMap.comp.spv";
	std::string compShaderPath = buildShaderPath(compShaderName, rendererVersion);
	Shader *compShaderGenerateCubemap = pDevice->Create(ShaderCreateInfo{readFile(compShaderPath.c_str())});

	std::vector<Pipeline *> pipelines(maxLevelNum);
	uint32_t width = pDstImageCube->GetCreateInfoPtr()->extent.width;

	std::vector<WriteDescriptorSet> writes;
	for (int i = 1; i < maxLevelNum; ++i)
	{
		//update descriptor set
		imageViewCreateInfo.subresourceRange.baseMipLevel = i;
		pDstImageViews[i] = pDevice->Create(imageViewCreateInfo);

		width /= 2;
		//create pipeline
		std::vector<uint32_t> constantIDs = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X, 3};
		std::vector<uint32_t> constantValues = {width, maxLevelNum - 1};

		pipelines[i] = pDevice->Create(ComputePipelineCreateInfo{
			PipelineShaderStageCreateInfo{
				PipelineShaderStageCreateInfo{
					ShaderStageFlagBits::COMPUTE_BIT,
					compShaderGenerateCubemap,
					"main",
					{constantIDs,
					 constantValues}},
			},
			pipelineLayout});

		writes.emplace_back(
			WriteDescriptorSet{descriptorSets[i],
							   0,
							   0,
							   DescriptorType::COMBINED_IMAGE_SAMPLER,
							   std::vector<DescriptorImageInfo>{{linearSampler,
																 pSrcImageViewCube,
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

	pDevice->ExecuteOneTimeCommands([&](CommandBuffer *pCommandBuffer) {
		//change layout
		pCommandBuffer->PipeplineBarrier(PipelineBarrierInfo{
			PipelineStageFlagBits::TOP_OF_PIPE_BIT,
			PipelineStageFlagBits::TRANSFER_BIT | PipelineStageFlagBits::COMPUTE_SHADER_BIT,
			{},
			0,
			nullptr,
			0,
			nullptr,
			static_cast<uint32_t>(barriers.size()),
			barriers.data()});
		//transfer level 0
		ImageCopy copyRegion{
			{0, 0, 6},
			{},
			{0, 0, 6},
			{},
			pSrcImageCube->GetCreateInfoPtr()->extent};
		pCommandBuffer->CopyImage(CopyImageInfo{
			pSrcImageCube,
			pDstImageCube,
			1,
			&copyRegion});
		
		//transfer level 0 to shader read only and general
		barriers = {
			{AccessFlagBits::TRANSFER_READ_BIT,
			 AccessFlagBits::SHADER_READ_BIT,
			 ImageLayout::TRANSFER_SRC_OPTIMAL,
			 ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			 pSrcImageCube,
			 {0, 1, 0, 6}},
			{AccessFlagBits::TRANSFER_WRITE_BIT,
			 AccessFlagBits::SHADER_WRITE_BIT,
			 ImageLayout::TRANSFER_DST_OPTIMAL,
			 ImageLayout::GENERAL,
			 pDstImageCube,
			 {0, 1, 0, 6}}};
		if (srcInitialLayout != ImageLayout::SHADER_READ_ONLY_OPTIMAL)
		{
			barriers.emplace_back(
				ImageMemoryBarrier{{},
								   AccessFlagBits::SHADER_READ_BIT,
								   srcInitialLayout,
								   ImageLayout::SHADER_READ_ONLY_OPTIMAL,
								   pSrcImageCube,
								   {1, pSrcImageCube->GetCreateInfoPtr()->mipLevels - 1, 0, 6}});
		}
		if (dstInitialLayout != ImageLayout::GENERAL)
		{
			barriers.emplace_back(
				ImageMemoryBarrier{{},
								   AccessFlagBits::SHADER_WRITE_BIT,
								   dstInitialLayout,
								   ImageLayout::GENERAL,
								   pDstImageCube,
								   {1, pDstImageCube->GetCreateInfoPtr()->mipLevels - 1, 0, 6}});
		}
		pCommandBuffer->PipeplineBarrier(PipelineBarrierInfo{
			PipelineStageFlagBits::TRANSFER_BIT,
			PipelineStageFlagBits::COMPUTE_SHADER_BIT,
			{},
			0,
			nullptr,
			0,
			nullptr,
			static_cast<uint32_t>(barriers.size()),
			barriers.data()});

		//process other levels
		uint32_t width = pDstImageCube->GetCreateInfoPtr()->extent.width;
		for (uint32_t i = 1; i < maxLevelNum; ++i)
		{
			width /= 2;
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
		if(srcFinalLayout!=ImageLayout::SHADER_READ_ONLY_OPTIMAL)
		{
			barriers.emplace_back(
				ImageMemoryBarrier{
					AccessFlagBits::SHADER_READ_BIT,
					AccessFlagBits::SHADER_READ_BIT,
					ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					srcFinalLayout,
					pSrcImageCube,
					{0, pSrcImageCube->GetCreateInfoPtr()->mipLevels, 0, 6}});
		}
		if(dstFinalLayout!=ImageLayout::GENERAL)
		{
			barriers.emplace_back(
				ImageMemoryBarrier{AccessFlagBits::SHADER_WRITE_BIT,
								   AccessFlagBits::SHADER_READ_BIT,
								   ImageLayout::GENERAL,
								   dstFinalLayout,
								   pDstImageCube,
								   {0, pDstImageCube->GetCreateInfoPtr()->mipLevels, 0, 6}});
		}
		pCommandBuffer->PipeplineBarrier(PipelineBarrierInfo{
			PipelineStageFlagBits::TRANSFER_BIT | PipelineStageFlagBits::COMPUTE_SHADER_BIT,
			PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
			{},
			0,
			nullptr,
			0,
			nullptr,
			static_cast<uint32_t>(barriers.size()),
			barriers.data()});
	});
	pDevice->Destroy(pSrcImageViewCube);
	for (auto e : pDstImageViews)
		pDevice->Destroy(e);
	pDevice->Destroy(descriptorPool);
	pDevice->Destroy(pipelineLayout);
	for (auto e : pipelines)
		pDevice->Destroy(e);
}