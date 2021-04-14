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

void takeScreenshot(Device *pDevice, Swapchain *pSwapchain, int swapchainImageIndex)
{
	Image *srcImage = pSwapchain->GetImageByIndex(swapchainImageIndex);

	auto width = pSwapchain->GetCreateInfoPtr()->imageExtent.width;
	auto height = pSwapchain->GetCreateInfoPtr()->imageExtent.height;
	auto component = 4;
	auto size = width * height * component;

//TODO: check if support blit to linear image
//NOTE: opengl need to flip y
#if 1
	//copy to buffer
	//		 cannot change format
	Buffer *dst = pDevice->Create(
		BufferCreateInfo{
			{},
			size,
			BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
		nullptr);

	BufferImageCopy copyRegion{
		.imageSubresource = {0, 0, 1},
		.imageExtent = {width, height, 1}};

	pDevice->ExecuteOneTimeCommands([&](CommandBuffer *cmdBuffer) {
		ImageMemoryBarrier imageBarrier0{
			AccessFlagBits::MEMORY_READ_BIT,
			AccessFlagBits::TRANSFER_READ_BIT,
			ImageLayout::PRESENT_SRC,
			ImageLayout::TRANSFER_SRC_OPTIMAL,
			srcImage,
			ImageSubresourceRange{0, 1, 0, 1}};
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
		cmdBuffer->CopyImageToBuffer(CopyImageToBufferInfo{
			srcImage,
			dst,
			1,
			&copyRegion});

		ImageMemoryBarrier imageBarrier1{
			AccessFlagBits::TRANSFER_READ_BIT,
			AccessFlagBits::MEMORY_READ_BIT,
			ImageLayout::TRANSFER_SRC_OPTIMAL,
			ImageLayout::PRESENT_SRC,
			srcImage,
			ImageSubresourceRange{0, 1, 0, 1}};

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
			{width, height, 1},
			1,
			1,
			SampleCountFlagBits::BIT_1,
			ImageTiling::LINEAR,
			ImageUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT,
		},
		nullptr);
	ImageCopy copyRegion{
		.srcSubresource = {0, 0, 1},
		.dstSubresource = {0, 0, 1},
		.extent = {width, height, 1}};
	pDevice->ExecuteOneTimeCommands([&](CommandBuffer *cmdBuffer) {
		ImageMemoryBarrier srcImageBarrier0{
			AccessFlagBits::MEMORY_READ_BIT,
			AccessFlagBits::TRANSFER_READ_BIT,
			ImageLayout::PRESENT_SRC,
			ImageLayout::TRANSFER_SRC_OPTIMAL,
			srcImage,
			ImageSubresourceRange{0, 1, 0, 1}};
		ImageMemoryBarrier dstImageBarrier0{
			{},
			AccessFlagBits::TRANSFER_WRITE_BIT,
			ImageLayout::UNDEFINED,
			ImageLayout::TRANSFER_DST_OPTIMAL,
			dst,
			ImageSubresourceRange{0, 1, 0, 1}};
		cmdBuffer->PipeplineBarrier(PipelineBarrierInfo{
			PipelineStageFlagBits::TRANSFER_BIT,
			PipelineStageFlagBits::TRANSFER_BIT,
			{},
			0,
			nullptr,
			0,
			nullptr,
			1,
			&srcImageBarrier0});
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
			srcImage,
			dst,
			1,
			&copyRegion});
		ImageMemoryBarrier srcImageBarrier1{
			AccessFlagBits::TRANSFER_READ_BIT,
			AccessFlagBits::MEMORY_READ_BIT,
			ImageLayout::TRANSFER_SRC_OPTIMAL,
			ImageLayout::PRESENT_SRC,
			srcImage,
			ImageSubresourceRange{0, 1, 0, 1}};
		ImageMemoryBarrier dstImageBarrier1{
			AccessFlagBits::TRANSFER_WRITE_BIT,
			AccessFlagBits::MEMORY_READ_BIT,
			ImageLayout::TRANSFER_DST_OPTIMAL,
			ImageLayout::GENERAL,
			dst,
			ImageSubresourceRange{0, 1, 0, 1}};
		cmdBuffer->PipeplineBarrier(PipelineBarrierInfo{
			PipelineStageFlagBits::TRANSFER_BIT,
			PipelineStageFlagBits::TRANSFER_BIT,
			{},
			0,
			nullptr,
			0,
			nullptr,
			1,
			&srcImageBarrier1});
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

	std::time_t t = std::time(nullptr);
	std::tm tm;
	localtime_s(&tm, &t);
	std::ostringstream ss;
	ss << std::put_time(&tm, "%Y%m%d%H%M%S");

	void *data;
	dst->MapMemory(0, size, &data);

	//swizzleColor
	if (
		pSwapchain->GetCreateInfoPtr()->format == ShitFormat::BGRA8_SRGB |
		pSwapchain->GetCreateInfoPtr()->format == ShitFormat::BGRA8_UNORM)
	{
		unsigned char *p = reinterpret_cast<unsigned char *>(data);
		unsigned char temp;
		for (uint32_t i = 0; i < size; i += 4, p += 4)
		{
			temp = *p;
			*(p) = *(p + 2);
			*(p + 2) = temp;
		}
	}
	auto screenshotPath = std::string(SCRRENSHOT_DIR) + ss.str() + ".jpg";
	saveImage(screenshotPath.c_str(), width, height, component, data);
	dst->UnMapMemory();

	pDevice->Destroy(dst);
}