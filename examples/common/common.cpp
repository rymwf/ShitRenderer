/**
 * @file common.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "common.h"

#include <shaderc/shaderc.hpp>

#include "stb_image.h"
#include "stb_image_write.h"

Shit::RendererVersion g_RendererVersion{Shit::RendererVersion::VULKAN};
Shit::ShadingLanguage g_shaderSourceLanguage;
static Shit::Sampler *g_LinearSampler;

std::string readFile(const char *filename) {
    std::fstream file(filename, std::ios::in | std::ios::ate | std::ios::binary);
    if (!file.is_open()) THROW("failed to open file:", filename)
    file.seekg(0, std::ios::end);
    size_t size0 = static_cast<size_t>(file.tellg());
    std::string ret;
    ret.resize(size0);
    file.seekg(0);
    file.read(const_cast<char *>(ret.data()), size0);
    file.close();
    return ret;
}
void *loadImage(const char *imagePath, int &width, int &height, int &components, int request_components,
                int &componentSize) {
    void *ret{};
    if (stbi_is_hdr(imagePath)) {
        ret = stbi_loadf(imagePath, &width, &height, &components,
                         request_components);  // force load an alpha channel,even not exist
        componentSize = 4;
    } else {
        ret = stbi_load(imagePath, &width, &height, &components,
                        request_components);  // force load an alpha channel,even not exist
        componentSize = 1;
    }
    if (!ret) LOG("failed to load texture image!")
    return ret;
}
void saveImage(const char *imagePath, int width, int height, int component, const void *data, bool hdr) {
    int ret{};
    if (hdr)
        ret = stbi_write_hdr(imagePath, width, height, component, reinterpret_cast<const float *>(data));
    else
        ret = stbi_write_jpg(imagePath, width, height, component, data, 90);

    if (!ret) LOG("failed to save image", ret);
}
void freeImage(void *pData) { stbi_image_free(pData); }
std::tuple<shaderc_target_env, shaderc_env_version, shaderc_spirv_version> getTargetEnvironment(
    Shit::RendererVersion rendererVersion) {
    auto type = rendererVersion & Shit::RendererVersion::TypeBitmask;
    // auto version = rendererVersion & Shit::RendererVersion::VersionBitmask;
    shaderc_target_env res_type;
    shaderc_env_version res_version;
    shaderc_spirv_version res_spv_version;
    switch (type) {
        case Shit::RendererVersion::GL: {
            res_type = shaderc_target_env_opengl;
            res_version = shaderc_env_version_opengl_4_5;
            res_spv_version = shaderc_spirv_version_1_0;
            break;
        }
        case Shit::RendererVersion::VULKAN:
        default: {
            res_type = shaderc_target_env_vulkan;
            switch (rendererVersion) {
                case Shit::RendererVersion::VULKAN:
                case Shit::RendererVersion::VULKAN_100:
                    res_version = shaderc_env_version_vulkan_1_0;
                    res_spv_version = shaderc_spirv_version_1_0;
                    break;
                case Shit::RendererVersion::VULKAN_110:
                    res_version = shaderc_env_version_vulkan_1_1;
                    res_spv_version = shaderc_spirv_version_1_3;
                    break;
                case Shit::RendererVersion::VULKAN_120:
                    res_version = shaderc_env_version_vulkan_1_2;
                    res_spv_version = shaderc_spirv_version_1_5;
                    break;
                case Shit::RendererVersion::VULKAN_130:
                    res_version = shaderc_env_version_vulkan_1_3;
                    res_spv_version = shaderc_spirv_version_1_6;
                    break;
            }
            break;
        }
    }
    return {res_type, res_version, res_spv_version};
}
shaderc_shader_kind getStage(Shit::ShaderStageFlagBits stage) {
    switch (stage) {
        case Shit::ShaderStageFlagBits::VERTEX_BIT:
            return shaderc_vertex_shader;
        case Shit::ShaderStageFlagBits::TESSELLATION_CONTROL_BIT:
            return shaderc_tess_control_shader;
        case Shit::ShaderStageFlagBits::TESSELLATION_EVALUATION_BIT:
            return shaderc_tess_evaluation_shader;
        case Shit::ShaderStageFlagBits::GEOMETRY_BIT:
            return shaderc_geometry_shader;
        case Shit::ShaderStageFlagBits::FRAGMENT_BIT:
            return shaderc_fragment_shader;
        case Shit::ShaderStageFlagBits::COMPUTE_BIT:
            return shaderc_compute_shader;
        case Shit::ShaderStageFlagBits::MESH_BIT:
            return shaderc_mesh_shader;
        case Shit::ShaderStageFlagBits::TASK_BIT:
            return shaderc_task_shader;
        case Shit::ShaderStageFlagBits::RAYGEN_BIT:
            return shaderc_raygen_shader;
        default:
            break;
    }
    THROW("unknow shader stage", (int)stage);
}

std::string compileGlslToSpirv(std::string_view glslcode, Shit::ShaderStageFlagBits stage,
                               Shit::RendererVersion renderVersion) {
    static shaderc::Compiler compiler;
    static shaderc::CompileOptions compileOptions;

    compileOptions.SetSourceLanguage(shaderc_source_language_glsl);
    auto targetEnv = getTargetEnvironment(renderVersion);
    compileOptions.SetTargetEnvironment(std::get<0>(targetEnv), std::get<1>(targetEnv));
    compileOptions.SetTargetSpirv(std::get<2>(targetEnv));
    compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

    auto retCode =
        compiler.CompileGlslToSpv(glslcode.data(), glslcode.size(), getStage(stage), "shader.glsl", compileOptions);
    if (retCode.GetCompilationStatus() != shaderc_compilation_status_success) {
        THROW("failed to compile shader, error message", retCode.GetErrorMessage());
    }
    auto l = std::distance(retCode.cbegin(), retCode.cend()) * sizeof(uint32_t);

    return std::string((const char *)retCode.cbegin(), l);
}

static bool flag = true;
std::string buildShaderPathGlsl(Shit::Device *pDevice, const char *shaderName, Shit::RendererVersion renderVersion) {
    if (flag) {
        flag = false;
        g_shaderSourceLanguage =
            chooseShaderShourceLanguage({Shit::ShadingLanguage::SPIRV, Shit::ShadingLanguage::GLSL}, pDevice);
    }
    return std::string(SHIT_SOURCE_DIR "/assets/shaders/glsl/") + shaderName;
}
std::string buildShaderPath(Shit::Device *pDevice, const char *shaderName, Shit::RendererVersion renderVersion) {
    if (flag) {
        flag = false;
        g_shaderSourceLanguage =
            chooseShaderShourceLanguage({Shit::ShadingLanguage::SPIRV, Shit::ShadingLanguage::GLSL}, pDevice);
    }

    renderVersion &= Shit::RendererVersion::TypeBitmask;
    std::string ss = SHIT_SOURCE_DIR;
    std::string posFix;
    switch (g_shaderSourceLanguage) {
        case Shit::ShadingLanguage::GLSL:
            ss += "/assets/shaders/glsl";
            break;
        case Shit::ShadingLanguage::SPIRV:
        default:
            ss += "/runtime/shaders/";
            posFix = ".spv";
            switch (renderVersion) {
                case Shit::RendererVersion::GL:
                    ss += "GL";
                    break;
                case Shit::RendererVersion::VULKAN:
                    ss += "Vulkan";
                    break;
                case Shit::RendererVersion::D3D11:
                    break;
                case Shit::RendererVersion::D3D12:
                    break;
                case Shit::RendererVersion::METAL:
                    break;
                default:
                    break;
            }
            break;
    }
    ss += "/";
    ss += shaderName;
    ss += posFix;
    return ss;
}
Shit::SurfacePixelFormat chooseSwapchainFormat(const std::vector<Shit::SurfacePixelFormat> &candidates,
                                               Shit::Surface *surface) {
    std::vector<Shit::SurfacePixelFormat> supportedFormats;
    surface->GetPixelFormats(supportedFormats);
    LOG("supported formats:");
    for (auto &&e : supportedFormats) {
        LOG_VAR(static_cast<int>(e.format));
        LOG_VAR(static_cast<int>(e.colorSpace));
        for (auto &&a : candidates) {
            if (e.format == a.format && e.colorSpace == a.colorSpace) return e;
        }
    }
    return supportedFormats[0];
}
Shit::PresentMode choosePresentMode(const std::vector<Shit::PresentMode> &candidates, Shit::Surface *surface) {
    std::vector<Shit::PresentMode> modes;
    surface->GetPresentModes(modes);
    for (auto &&a : candidates) {
        for (auto &&e : modes) {
            if (static_cast<int>(e) == static_cast<int>(a)) return e;
        }
    }
    return modes[0];
}
Shit::ShadingLanguage chooseShaderShourceLanguage(const std::vector<Shit::ShadingLanguage> &candidates,
                                                  Shit::Device *pDevice) {
    std::vector<Shit::ShadingLanguage> shadingLanguages;
    pDevice->GetSupportedShaderSourceLanguages(shadingLanguages);
    for (auto &&e : candidates) {
        auto it = std::find(shadingLanguages.begin(), shadingLanguages.end(), e);
        if (it != shadingLanguages.end()) return e;
    }
    THROW("failed to find supported shading language");
}

void takeScreenshot(Shit::Device *pDevice, Shit::Image *pImage, Shit::ImageLayout imageLayout) {
    constexpr auto SCRRENSHOT_DIR = SHIT_SOURCE_DIR "/screenshot/";
    if (!std::filesystem::exists(SCRRENSHOT_DIR)) {
        std::filesystem::create_directory(SCRRENSHOT_DIR);
    }
    std::time_t t = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y%m%d%H%M%S");
    saveImage((std::string(SCRRENSHOT_DIR) + ss.str()).c_str(), pDevice, pImage, imageLayout);
}
void saveImage(const char *dstPath, Shit::Device *pDevice, Shit::Image *pImage, Shit::ImageLayout imageLayout) {
    auto width = pImage->GetCreateInfoPtr()->extent.width;
    auto height = pImage->GetCreateInfoPtr()->extent.height;
    auto depth = pImage->GetCreateInfoPtr()->extent.depth;
    auto layers = pImage->GetCreateInfoPtr()->arrayLayers;
    auto component = Shit::GetFormatComponentNum(pImage->GetCreateInfoPtr()->format);

    auto format = pImage->GetCreateInfoPtr()->format;
    auto formatSize = Shit::GetFormatSize(format);
    auto dataType = Shit::GetFormatDataType(format);
    auto levels = pImage->GetCreateInfoPtr()->mipLevels;

    // if constexpr (0 &&
    // static_cast<bool>(pImage->GetCreateInfoPtr()->memoryPropertyFlags &
    // Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT))
    //{
    //	void *data;
    //	pImage->MapMemory(0, size, &data);
    //	//swizzleColor
    //	if (
    //		pImage->GetCreateInfoPtr()->format ==
    // Shit::ShitFormat::B8G8R8A8_SRGB || 		pImage->GetCreateInfoPtr()->format
    // == Shit::ShitFormat::B8G8R8A8_UNORM)
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
    //	if (!static_cast<bool>(pImage->GetCreateInfoPtr()->memoryPropertyFlags &
    // Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT))
    //	{
    //		pImage->FlushMappedMemoryRange(0, size);
    //	}
    // }
    // else
    {
        // TODO: check if support blit to linear image
        // NOTE: opengl need to flip y
        uint32_t tempWidth = width, tempHeight = height, tempDepth = depth;
#if 1
        // copy to buffer
        //		 cannot change format
        std::vector<Shit::Buffer *> dst(levels);
        std::vector<Shit::BufferMemoryBarrier> bufferBarriers(levels);
        for (uint32_t i = 0; i < levels; ++i, tempWidth = (std::max)(tempWidth / 2, 1u),
                      tempHeight = (std::max)(tempHeight / 2, 1u), tempDepth = (std::max)(tempDepth / 2, 1u)) {
            auto size = tempWidth * tempHeight * tempDepth * layers * formatSize;
            dst[i] = pDevice->Create(Shit::BufferCreateInfo{{},
                                                            size,
                                                            Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
                                                            Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT |
                                                                Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
                                     nullptr);
        }

        pDevice->ExecuteOneTimeCommand([&](Shit::CommandBuffer *cmdBuffer) {
            for (uint32_t i = 0; i < levels; ++i) {
                bufferBarriers[i] = Shit::BufferMemoryBarrier{{},
                                                              Shit::AccessFlagBits::TRANSFER_WRITE_BIT,
                                                              ST_QUEUE_FAMILY_IGNORED,
                                                              ST_QUEUE_FAMILY_IGNORED,
                                                              dst[i],
                                                              0,
                                                              dst[i]->GetCreateInfoPtr()->size};
            }

            Shit::ImageMemoryBarrier imageBarrier0{
                Shit::AccessFlagBits::SHADER_WRITE_BIT | Shit::AccessFlagBits::TRANSFER_WRITE_BIT,
                Shit::AccessFlagBits::TRANSFER_READ_BIT,
                imageLayout,
                Shit::ImageLayout::TRANSFER_SRC_OPTIMAL,
                ST_QUEUE_FAMILY_IGNORED,
                ST_QUEUE_FAMILY_IGNORED,
                pImage,
                Shit::ImageSubresourceRange{Shit::ImageAspectFlagBits::COLOR_BIT, 0, levels, 0, layers}};
            cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::ALL_COMMANDS_BIT,
                                                                 Shit::PipelineStageFlagBits::TRANSFER_BIT,
                                                                 {},
                                                                 0,
                                                                 nullptr,
                                                                 static_cast<uint32_t>(bufferBarriers.size()),
                                                                 bufferBarriers.data(),
                                                                 1,
                                                                 &imageBarrier0});

            tempWidth = width, tempHeight = height, tempDepth = depth;
            for (uint32_t i = 0; i < levels; ++i, tempWidth = (std::max)(tempWidth / 2, 1u),
                          tempHeight = (std::max)(tempHeight / 2, 1u), tempDepth = (std::max)(tempDepth / 2, 1u)) {
                Shit::BufferImageCopy copyRegion{0,  0,
                                                 0,  {Shit::ImageAspectFlagBits::COLOR_BIT, i, 0, layers},
                                                 {}, {tempWidth, tempHeight, tempDepth}};
                cmdBuffer->CopyImageToBuffer(Shit::CopyImageToBufferInfo{pImage, dst[i], 1, &copyRegion});
            }
            for (uint32_t i = 0; i < levels; ++i) {
                bufferBarriers[i] = Shit::BufferMemoryBarrier{Shit::AccessFlagBits::MEMORY_WRITE_BIT,
                                                              Shit::AccessFlagBits::MEMORY_READ_BIT,
                                                              ST_QUEUE_FAMILY_IGNORED,
                                                              ST_QUEUE_FAMILY_IGNORED,
                                                              dst[i],
                                                              0,
                                                              dst[i]->GetCreateInfoPtr()->size};
            }
            Shit::ImageMemoryBarrier imageBarrier1{
                Shit::AccessFlagBits::TRANSFER_READ_BIT,
                Shit::AccessFlagBits::MEMORY_READ_BIT,
                Shit::ImageLayout::TRANSFER_SRC_OPTIMAL,
                imageLayout,
                ST_QUEUE_FAMILY_IGNORED,
                ST_QUEUE_FAMILY_IGNORED,
                pImage,
                Shit::ImageSubresourceRange{Shit::ImageAspectFlagBits::COLOR_BIT, 0, levels, 0, layers}};

            cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::TRANSFER_BIT,
                                                                 Shit::PipelineStageFlagBits::ALL_COMMANDS_BIT,
                                                                 {},
                                                                 0,
                                                                 nullptr,
                                                                 static_cast<uint32_t>(bufferBarriers.size()),
                                                                 bufferBarriers.data(),
                                                                 1,
                                                                 &imageBarrier1});
        });

#else
        // TODO: need fix
        // copy to image
        Shit::Image *dst = pDevice->Create(
            Shit::ImageCreateInfo{
                Shit::ImageCreateFlagBits::CUBE_COMPATIBLE_BIT,
                Shit::ImageType::TYPE_2D,
                pImage->GetCreateInfoPtr()->format,
                {width, height, depth},
                levels,
                layers,
                Shit::SampleCountFlagBits::BIT_1,
                Shit::ImageTiling::LINEAR,
                Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
                Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT | Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT,
            },
            nullptr);

        std::vector<Shit::ImageCopy> copyRegions(levels);
        for (uint32_t i = 0; i < levels; ++i) {
            copyRegions[i] = {{i, 0, layers}, {}, {i, 0, layers}, {}, {width, height, depth}};
        }
        pDevice->ExecuteOneTimeCommands([&](Shit::CommandBuffer *cmdBuffer) {
            Shit::ImageMemoryBarrier pImageBarrier0{Shit::AccessFlagBits::MEMORY_READ_BIT,
                                                    Shit::AccessFlagBits::TRANSFER_READ_BIT,
                                                    imageLayout,
                                                    Shit::ImageLayout::TRANSFER_SRC_OPTIMAL,
                                                    pImage,
                                                    Shit::ImageSubresourceRange{0, levels, 0, layers}};
            Shit::ImageMemoryBarrier dstImageBarrier0{{},
                                                      Shit::AccessFlagBits::TRANSFER_WRITE_BIT,
                                                      Shit::ImageLayout::UNDEFINED,
                                                      Shit::ImageLayout::TRANSFER_DST_OPTIMAL,
                                                      dst,
                                                      Shit::ImageSubresourceRange{0, levels, 0, layers}};
            cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::TRANSFER_BIT,
                                                                 Shit::PipelineStageFlagBits::TRANSFER_BIT,
                                                                 {},
                                                                 0,
                                                                 nullptr,
                                                                 0,
                                                                 nullptr,
                                                                 1,
                                                                 &pImageBarrier0});
            cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::TRANSFER_BIT,
                                                                 Shit::PipelineStageFlagBits::TRANSFER_BIT,
                                                                 {},
                                                                 0,
                                                                 nullptr,
                                                                 0,
                                                                 nullptr,
                                                                 1,
                                                                 &dstImageBarrier0});
            cmdBuffer->CopyImage(
                CopyImageInfo{pImage, dst, static_cast<uint32_t>(copyRegions.size()), copyRegions.data()});
            Shit::ImageMemoryBarrier pImageBarrier1{Shit::AccessFlagBits::TRANSFER_READ_BIT,
                                                    Shit::AccessFlagBits::MEMORY_READ_BIT,
                                                    Shit::ImageLayout::TRANSFER_SRC_OPTIMAL,
                                                    pImage->GetCreateInfoPtr()->initialLayout,
                                                    pImage,
                                                    Shit::ImageSubresourceRange{0, levels, 0, layers}};
            Shit::ImageMemoryBarrier dstImageBarrier1{Shit::AccessFlagBits::TRANSFER_WRITE_BIT,
                                                      Shit::AccessFlagBits::MEMORY_READ_BIT,
                                                      Shit::ImageLayout::TRANSFER_DST_OPTIMAL,
                                                      Shit::ImageLayout::GENERAL,
                                                      dst,
                                                      Shit::ImageSubresourceRange{0, levels, 0, layers}};
            cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::TRANSFER_BIT,
                                                                 Shit::PipelineStageFlagBits::TRANSFER_BIT,
                                                                 {},
                                                                 0,
                                                                 nullptr,
                                                                 0,
                                                                 nullptr,
                                                                 1,
                                                                 &pImageBarrier1});
            cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::TRANSFER_BIT,
                                                                 Shit::PipelineStageFlagBits::TRANSFER_BIT,
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
        for (uint32_t j = 0; j < levels; ++j, tempWidth = (std::max)(tempWidth / 2, 1u),
                      tempHeight = (std::max)(tempHeight / 2, 1u), tempDepth = (std::max)(tempDepth / 2, 1u)) {
            auto size = tempWidth * tempHeight * tempDepth * layers * formatSize;
            void *data;
            dst[j]->MapMemory(0, size, &data);
            // swizzleColor
            if (pImage->GetCreateInfoPtr()->format == Shit::Format::B8G8R8A8_SRGB ||
                pImage->GetCreateInfoPtr()->format == Shit::Format::B8G8R8A8_UNORM) {
                unsigned char temp;
                unsigned char *p = reinterpret_cast<unsigned char *>(data);
                for (uint32_t i = 0; i < size; i += 4, p += 4) {
                    temp = *p;
                    *(p) = *(p + 2);
                    *(p + 2) = temp;
                }
            }
            std::string ss = dstPath;
            ss += "-";
            ss += std::to_string(j);
            if (dataType == Shit::DataType::FLOAT)
                ss += ".hdr";
            else
                ss += ".jpg";
            saveImage(ss.c_str(), tempWidth, tempHeight * tempDepth * layers, component, data,
                      dataType == Shit::DataType::FLOAT);
            dst[j]->UnMapMemory();
        }
        for (auto &&e : dst) {
            pDevice->Destroy(e);
        }
    }
}
// void parseArgument(int ac, char **av)
//{
//
//	namespace po = boost::program_options;
//	po::options_description desc("Allowed options");
//	desc.add_options()("help", "produce help message")("T",
// po::value<std::string>()->default_value("GL"), "set renderer version\n"
//																										   "option
// values:\n"
// "GL:\t opengl rendereer"
// "VK:\t vulkan renderer");
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
//			g_RendererVersion = Shit::RendererVersion::GL;
//		}
//		else if (s == "VK")
//		{
//			g_RendererVersion = Shit::RendererVersion::VULKAN;
//		}
//		else
//		{
//			std::cout << "invalid renderer version value";
//			exit(0);
//		}
//	}
// }

void generateIrradianceMap(Shit::Device *pDevice, Shit::Image *pSrcImage2D, Shit::ImageLayout srcInitialLayout,
                           Shit::ImageLayout srcFinalLayout, Shit::Image *pDstImageCube,
                           Shit::ImageLayout dstInitialLayout, Shit::ImageLayout dstFinalLayout) {
    auto dstImageWidth = pDstImageCube->GetCreateInfoPtr()->extent.width;
    //==
    // create sampler
    if (!g_LinearSampler)
        g_LinearSampler = pDevice->Create(Shit::SamplerCreateInfo{
            Shit::Filter::LINEAR,
            Shit::Filter::LINEAR,
            Shit::SamplerMipmapMode::LINEAR,
            Shit::SamplerWrapMode::CLAMP_TO_EDGE,
            Shit::SamplerWrapMode::CLAMP_TO_EDGE,
            Shit::SamplerWrapMode::CLAMP_TO_EDGE,
            0,
            false,
            1.f,
            false,
            {},
            -30.f,
            30.f,
        });
    // create image view
    Shit::ImageView *pSrcImageView2D = pDevice->Create(Shit::ImageViewCreateInfo{
        pSrcImage2D,
        Shit::ImageViewType::TYPE_2D,
        pSrcImage2D->GetCreateInfoPtr()->format,
        {},
        {Shit::ImageAspectFlagBits::COLOR_BIT, 0, pSrcImage2D->GetCreateInfoPtr()->mipLevels, 0, 1}});
    Shit::ImageView *pDstImageViewCube =
        pDevice->Create(Shit::ImageViewCreateInfo{pDstImageCube,
                                                  Shit::ImageViewType::TYPE_CUBE,
                                                  pDstImageCube->GetCreateInfoPtr()->format,
                                                  {},
                                                  {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 6}});

    //===============================================
    // descriptor sets
    std::vector<Shit::DescriptorSetLayoutBinding> cubemapComputeBindings{
        Shit::DescriptorSetLayoutBinding{0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1,
                                         Shit::ShaderStageFlagBits::COMPUTE_BIT},
        Shit::DescriptorSetLayoutBinding{1, Shit::DescriptorType::STORAGE_IMAGE, 1,
                                         Shit::ShaderStageFlagBits::COMPUTE_BIT},
    };
    std::vector<Shit::DescriptorSetLayout *> setLayouts = {
        pDevice->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)cubemapComputeBindings.size(),
                                                            cubemapComputeBindings.data()}),
    };
    // create pipeline layout
    Shit::PipelineLayout *pipelineLayoutGenerateCubemap =
        pDevice->Create(Shit::PipelineLayoutCreateInfo{1, &setLayouts[0]});

    // setup descriptor pool
    std::vector<Shit::DescriptorPoolSize> poolSizes{
        {cubemapComputeBindings[0].descriptorType, cubemapComputeBindings[0].descriptorCount},
        {cubemapComputeBindings[1].descriptorType, cubemapComputeBindings[1].descriptorCount},
    };
    Shit::DescriptorPoolCreateInfo descriptorPoolCreateInfo{1u, (uint32_t)poolSizes.size(), poolSizes.data()};
    Shit::DescriptorPool *descriptorPool = pDevice->Create(descriptorPoolCreateInfo);

    // allocate compute descriptor set
    Shit::DescriptorSetAllocateInfo allocInfo{1, &setLayouts[0]};
    std::vector<Shit::DescriptorSet *> compDescriptorSets;
    descriptorPool->Allocate(allocInfo, compDescriptorSets);

    // update descriptor set
    std::vector<Shit::WriteDescriptorSet> writes;
    Shit::DescriptorImageInfo imagesInfo[]{
        {g_LinearSampler, pSrcImageView2D, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        {nullptr, pDstImageViewCube, Shit::ImageLayout::GENERAL}

    };
    writes.emplace_back(Shit::WriteDescriptorSet{compDescriptorSets[0], 0, 0, 1,
                                                 Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo[0]});
    writes.emplace_back(
        Shit::WriteDescriptorSet{compDescriptorSets[0], 1, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[1]});
    pDevice->UpdateDescriptorSets(writes, {});
    //=============================
    // shader
    const char *compShaderNameGenerateCubemap = "irradiance.comp";
    std::string compShaderPath = buildShaderPath(pDevice, compShaderNameGenerateCubemap, g_RendererVersion);
    auto compSource = readFile(compShaderPath.c_str());
    // auto compSourceSpv = compileGlslToSpirv(compSource,
    // Shit::ShaderStageFlagBits::COMPUTE_BIT, g_RendererVersion);
    Shit::Shader *compShaderGenerateCubemap =
        pDevice->Create(Shit::ShaderCreateInfo{g_shaderSourceLanguage, compSource.size(), compSource.data()});

    // generate cubemap pipeline
    uint32_t constantIDs[] = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X, 3};
    uint32_t constantValues[] = {dstImageWidth, pDstImageCube->GetCreateInfoPtr()->extent.width};

    Shit::Pipeline *pipelineGenerateCubemap = pDevice->Create(Shit::ComputePipelineCreateInfo{
        Shit::PipelineShaderStageCreateInfo{
            Shit::PipelineShaderStageCreateInfo{Shit::ShaderStageFlagBits::COMPUTE_BIT,
                                                compShaderGenerateCubemap,
                                                "main",
                                                {ARR_SIZE(constantIDs), constantIDs, constantValues}},
        },
        pipelineLayoutGenerateCubemap});

    // commandbuffer
    pDevice->ExecuteOneTimeCommand([&](Shit::CommandBuffer *cmdBuffer) {
        // convert dst image layout to general
        std::vector<Shit::ImageMemoryBarrier> imageBarriers;
        if (srcInitialLayout != Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL) {
            imageBarriers.emplace_back(Shit::ImageMemoryBarrier{
                {},
                Shit::AccessFlagBits::SHADER_READ_BIT,
                srcInitialLayout,
                Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                ST_QUEUE_FAMILY_IGNORED,
                ST_QUEUE_FAMILY_IGNORED,
                pSrcImage2D,
                Shit::ImageSubresourceRange{Shit::ImageAspectFlagBits::COLOR_BIT, 0,
                                            pSrcImage2D->GetCreateInfoPtr()->mipLevels, 0, 6}});
        }
        if (dstInitialLayout != Shit::ImageLayout::GENERAL) {
            imageBarriers.emplace_back(Shit::ImageMemoryBarrier{
                {},
                Shit::AccessFlagBits::SHADER_WRITE_BIT,
                dstInitialLayout,
                Shit::ImageLayout::GENERAL,
                ST_QUEUE_FAMILY_IGNORED,
                ST_QUEUE_FAMILY_IGNORED,
                pDstImageCube,
                Shit::ImageSubresourceRange{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 6}});
        }

        // change image layout to general
        cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::TOP_OF_PIPE_BIT,
                                                             Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                             {},
                                                             0,
                                                             nullptr,
                                                             0,
                                                             nullptr,
                                                             static_cast<uint32_t>(imageBarriers.size()),
                                                             imageBarriers.data()});
        //
        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, pipelineGenerateCubemap});
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::COMPUTE,
            pipelineLayoutGenerateCubemap,
            0,
            1,
            &compDescriptorSets[0],
        });
        cmdBuffer->Dispatch({dstImageWidth, 6, 1});

        imageBarriers.clear();
        if (srcFinalLayout != Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL) {
            imageBarriers.emplace_back(Shit::ImageMemoryBarrier{
                Shit::AccessFlagBits::SHADER_READ_BIT, Shit::AccessFlagBits::SHADER_READ_BIT,
                Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL, srcFinalLayout, ST_QUEUE_FAMILY_IGNORED,
                ST_QUEUE_FAMILY_IGNORED, pSrcImage2D,
                Shit::ImageSubresourceRange{Shit::ImageAspectFlagBits::COLOR_BIT, 0,
                                            pSrcImage2D->GetCreateInfoPtr()->mipLevels, 0, 6}});
        }
        if (dstFinalLayout != Shit::ImageLayout::GENERAL) {
            imageBarriers.emplace_back(Shit::ImageMemoryBarrier{
                Shit::AccessFlagBits::SHADER_WRITE_BIT, Shit::AccessFlagBits::SHADER_READ_BIT,
                Shit::ImageLayout::GENERAL, dstFinalLayout, ST_QUEUE_FAMILY_IGNORED, ST_QUEUE_FAMILY_IGNORED,
                pDstImageCube, Shit::ImageSubresourceRange{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 6}});
        }
        if (dstFinalLayout != dstInitialLayout && pDstImageCube->GetCreateInfoPtr()->mipLevels > 1) {
            imageBarriers.emplace_back(Shit::ImageMemoryBarrier{
                {},
                Shit::AccessFlagBits::SHADER_READ_BIT,
                dstInitialLayout,
                dstFinalLayout,
                ST_QUEUE_FAMILY_IGNORED,
                ST_QUEUE_FAMILY_IGNORED,
                pDstImageCube,
                Shit::ImageSubresourceRange{Shit::ImageAspectFlagBits::COLOR_BIT, 1,
                                            pDstImageCube->GetCreateInfoPtr()->mipLevels - 1, 0, 6}});
        }
        // change image layout to general
        cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                             Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
                                                             {},
                                                             0,
                                                             nullptr,
                                                             0,
                                                             nullptr,
                                                             static_cast<uint32_t>(imageBarriers.size()),
                                                             imageBarriers.data()});
    });
    pDstImageCube->GenerateMipmap(Shit::Filter::LINEAR, dstFinalLayout, dstFinalLayout);
    pDevice->Destroy(pSrcImageView2D);
    pDevice->Destroy(pDstImageViewCube);
    pDevice->Destroy(pipelineLayoutGenerateCubemap);
    pDevice->Destroy(descriptorPool);
    pDevice->Destroy(compShaderGenerateCubemap);
    pDevice->Destroy(pipelineGenerateCubemap);
}
void generateIrradianceMapSH(Shit::Device *pDevice, Shit::Image *pSrcImage2D, Shit::ImageLayout srcInitialLayout,
                             Shit::ImageLayout srcFinalLayout, Shit::Image *pDstImageCube,
                             Shit::ImageLayout dstInitialLayout, Shit::ImageLayout dstFinalLayout) {
    // create sampler
    if (!g_LinearSampler)
        g_LinearSampler = pDevice->Create(Shit::SamplerCreateInfo{
            Shit::Filter::LINEAR,
            Shit::Filter::LINEAR,
            Shit::SamplerMipmapMode::LINEAR,
            Shit::SamplerWrapMode::CLAMP_TO_EDGE,
            Shit::SamplerWrapMode::CLAMP_TO_EDGE,
            Shit::SamplerWrapMode::CLAMP_TO_EDGE,
            0,
            false,
            1.f,
            false,
            {},
            -30.f,
            30.f,
        });
    // create image view
    Shit::ImageView *pSrcImageView2D = pDevice->Create(Shit::ImageViewCreateInfo{
        pSrcImage2D,
        Shit::ImageViewType::TYPE_2D,
        pSrcImage2D->GetCreateInfoPtr()->format,
        {},
        {Shit::ImageAspectFlagBits::COLOR_BIT, 0, pSrcImage2D->GetCreateInfoPtr()->mipLevels, 0, 1}});
    Shit::ImageView *pDstImageViewCube =
        pDevice->Create(Shit::ImageViewCreateInfo{pDstImageCube,
                                                  Shit::ImageViewType::TYPE_CUBE,
                                                  pDstImageCube->GetCreateInfoPtr()->format,
                                                  {},
                                                  {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 6}});
    // prepare llm image
    Shit::Image *llmImage = pDevice->Create(Shit::ImageCreateInfo{{},
                                                                  Shit::ImageType::TYPE_1D,
                                                                  pSrcImage2D->GetCreateInfoPtr()->format,
                                                                  {32, 1, 1},
                                                                  1,
                                                                  1,
                                                                  Shit::SampleCountFlagBits::BIT_1,
                                                                  Shit::ImageTiling::OPTIMAL,
                                                                  Shit::ImageUsageFlagBits::STORAGE_BIT,
                                                                  Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                                                  Shit::ImageLayout::GENERAL});
    Shit::ImageView *llmImageView =
        pDevice->Create(Shit::ImageViewCreateInfo{llmImage,
                                                  Shit::ImageViewType::TYPE_1D,
                                                  pSrcImage2D->GetCreateInfoPtr()->format,
                                                  {},
                                                  {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}});

    //===============================================
    // descriptor sets
    std::vector<Shit::DescriptorSetLayoutBinding> llmBindinds{
        Shit::DescriptorSetLayoutBinding{0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1,
                                         Shit::ShaderStageFlagBits::COMPUTE_BIT},
        Shit::DescriptorSetLayoutBinding{1, Shit::DescriptorType::STORAGE_IMAGE, 1,
                                         Shit::ShaderStageFlagBits::COMPUTE_BIT},
    };
    std::vector<Shit::DescriptorSetLayoutBinding> shBindinds{
        Shit::DescriptorSetLayoutBinding{0, Shit::DescriptorType::STORAGE_IMAGE, 1,
                                         Shit::ShaderStageFlagBits::COMPUTE_BIT},
        Shit::DescriptorSetLayoutBinding{1, Shit::DescriptorType::STORAGE_IMAGE, 1,
                                         Shit::ShaderStageFlagBits::COMPUTE_BIT},
    };
    std::vector<Shit::DescriptorSetLayout *> setLayouts = {
        pDevice->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)llmBindinds.size(), llmBindinds.data()}),
        pDevice->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)shBindinds.size(), shBindinds.data()}),
    };
    // create pipeline layout
    Shit::PipelineLayout *llmPipelineLayout = pDevice->Create(Shit::PipelineLayoutCreateInfo{1, &setLayouts[0]});
    Shit::PipelineLayout *shPipelineLayout = pDevice->Create(Shit::PipelineLayoutCreateInfo{1, &setLayouts[1]});

    // setup descriptor pool
    std::vector<Shit::DescriptorPoolSize> poolSizes{
        {Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1},
        {Shit::DescriptorType::STORAGE_IMAGE, 3},
    };
    Shit::DescriptorPoolCreateInfo descriptorPoolCreateInfo{2u, (uint32_t)poolSizes.size(), poolSizes.data()};
    Shit::DescriptorPool *descriptorPool = pDevice->Create(descriptorPoolCreateInfo);

    // allocate compute descriptor set
    Shit::DescriptorSetAllocateInfo allocInfo{(uint32_t)setLayouts.size(), setLayouts.data()};
    std::vector<Shit::DescriptorSet *> compDescriptorSets;
    descriptorPool->Allocate(allocInfo, compDescriptorSets);

    // update descriptor set
    Shit::DescriptorImageInfo imagesInfo[]{
        {g_LinearSampler, pSrcImageView2D, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        {nullptr, llmImageView, Shit::ImageLayout::GENERAL},
        {nullptr, llmImageView, Shit::ImageLayout::GENERAL},
        {nullptr, pDstImageViewCube, Shit::ImageLayout::GENERAL}};

    std::vector<Shit::WriteDescriptorSet> writes{
        {compDescriptorSets[0], 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo[0]},
        {compDescriptorSets[0], 1, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[1]},
        {compDescriptorSets[1], 0, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[2]},
        {compDescriptorSets[1], 1, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[3]},
    };
    pDevice->UpdateDescriptorSets(writes, {});
    //=============================

    // create shader
    static const char *llmCompShaderName = "irradianceSH_Llm.comp";
    static const char *shCompShaderName = "irradianceSH.comp";
    std::string llmShaderPath = buildShaderPath(pDevice, llmCompShaderName, g_RendererVersion);
    std::string shShaderPath = buildShaderPath(pDevice, shCompShaderName, g_RendererVersion);
    auto llmSource = readFile(llmShaderPath.c_str());
    auto shSource = readFile(shShaderPath.c_str());
    // auto llmSource = compileGlslToSpirv(readFile(llmShaderPath.c_str()),
    // Shit::ShaderStageFlagBits::COMPUTE_BIT, g_RendererVersion); auto shSource =
    // compileGlslToSpirv(readFile(shShaderPath.c_str()),
    // Shit::ShaderStageFlagBits::COMPUTE_BIT, g_RendererVersion);
    Shit::Shader *llmShader =
        pDevice->Create(Shit::ShaderCreateInfo{g_shaderSourceLanguage, llmSource.size(), llmSource.data()});
    Shit::Shader *shShader =
        pDevice->Create(Shit::ShaderCreateInfo{g_shaderSourceLanguage, shSource.size(), shSource.data()});

    uint32_t constantId = 3;
    uint32_t constantValue = pDstImageCube->GetCreateInfoPtr()->extent.width;

    // create llm pipeline
    Shit::Pipeline *llmPipeline = pDevice->Create(Shit::ComputePipelineCreateInfo{
        Shit::PipelineShaderStageCreateInfo{Shit::PipelineShaderStageCreateInfo{
            Shit::ShaderStageFlagBits::COMPUTE_BIT, llmShader, "main", {1, &constantId, &constantValue}}},
        llmPipelineLayout});

    // create sh pipeline
    uint32_t constantIDs[] = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X};
    uint32_t constantValues[] = {pDstImageCube->GetCreateInfoPtr()->extent.width};

    Shit::Pipeline *shPipeline = pDevice->Create(Shit::ComputePipelineCreateInfo{
        Shit::PipelineShaderStageCreateInfo{
            Shit::PipelineShaderStageCreateInfo{
                Shit::ShaderStageFlagBits::COMPUTE_BIT, shShader, "main", {1, constantIDs, constantValues}},
        },
        shPipelineLayout});

    pDevice->ExecuteOneTimeCommand([&](Shit::CommandBuffer *pCommandBuffer) {
        std::vector<Shit::ImageMemoryBarrier> barriers;
        if (srcInitialLayout != Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL) {
            barriers.emplace_back(Shit::ImageMemoryBarrier{Shit::AccessFlagBits::MEMORY_WRITE_BIT,
                                                           Shit::AccessFlagBits::MEMORY_READ_BIT,
                                                           srcInitialLayout,
                                                           Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                                                           ST_QUEUE_FAMILY_IGNORED,
                                                           ST_QUEUE_FAMILY_IGNORED,
                                                           pSrcImage2D,
                                                           {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}});
        }
        if (dstInitialLayout != Shit::ImageLayout::GENERAL) {
            barriers.emplace_back(Shit::ImageMemoryBarrier{{},
                                                           Shit::AccessFlagBits::MEMORY_READ_BIT,
                                                           dstInitialLayout,
                                                           Shit::ImageLayout::GENERAL,
                                                           ST_QUEUE_FAMILY_IGNORED,
                                                           ST_QUEUE_FAMILY_IGNORED,
                                                           pDstImageCube,
                                                           {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 6}});
        }
        pCommandBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::TOP_OF_PIPE_BIT,
                                                                  Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                  {},
                                                                  0,
                                                                  nullptr,
                                                                  0,
                                                                  nullptr,
                                                                  static_cast<uint32_t>(barriers.size()),
                                                                  barriers.data()});
        // create llm image
        pCommandBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::COMPUTE,
            llmPipelineLayout,
            0,
            1,
            &compDescriptorSets[0],
        });
        pCommandBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, llmPipeline});
        pCommandBuffer->Dispatch(Shit::DispatchInfo{1, 1, 1});
        // create irradiance image

        barriers = {{Shit::AccessFlagBits::MEMORY_WRITE_BIT,
                     Shit::AccessFlagBits::MEMORY_READ_BIT,
                     Shit::ImageLayout::GENERAL,
                     Shit::ImageLayout::GENERAL,
                     ST_QUEUE_FAMILY_IGNORED,
                     ST_QUEUE_FAMILY_IGNORED,
                     llmImage,
                     {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}}};
        pCommandBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                  Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                  {},
                                                                  0,
                                                                  nullptr,
                                                                  0,
                                                                  nullptr,
                                                                  static_cast<uint32_t>(barriers.size()),
                                                                  barriers.data()});
        pCommandBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::COMPUTE,
            shPipelineLayout,
            0,
            1,
            &compDescriptorSets[1],
        });
        pCommandBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, shPipeline});
        pCommandBuffer->Dispatch(Shit::DispatchInfo{pDstImageCube->GetCreateInfoPtr()->extent.width, 6, 1});

        barriers.clear();
        if (srcFinalLayout != Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL) {
            barriers.emplace_back(Shit::ImageMemoryBarrier{Shit::AccessFlagBits::MEMORY_READ_BIT,
                                                           Shit::AccessFlagBits::MEMORY_READ_BIT,
                                                           Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                                                           srcFinalLayout,
                                                           ST_QUEUE_FAMILY_IGNORED,
                                                           ST_QUEUE_FAMILY_IGNORED,
                                                           pSrcImage2D,
                                                           {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}});
        }
        if (srcInitialLayout != srcFinalLayout) {
            barriers.emplace_back(Shit::ImageMemoryBarrier{
                Shit::AccessFlagBits::MEMORY_READ_BIT,
                Shit::AccessFlagBits::MEMORY_READ_BIT,
                srcInitialLayout,
                srcFinalLayout,
                ST_QUEUE_FAMILY_IGNORED,
                ST_QUEUE_FAMILY_IGNORED,
                pSrcImageView2D->GetCreateInfoPtr()->pImage,
                {Shit::ImageAspectFlagBits::COLOR_BIT, 1,
                 pSrcImageView2D->GetCreateInfoPtr()->pImage->GetCreateInfoPtr()->mipLevels - 1, 0, 1}});
        }
        if (dstFinalLayout != Shit::ImageLayout::GENERAL) {
            barriers.emplace_back(Shit::ImageMemoryBarrier{Shit::AccessFlagBits::MEMORY_READ_BIT,
                                                           Shit::AccessFlagBits::MEMORY_READ_BIT,
                                                           Shit::ImageLayout::GENERAL,
                                                           dstFinalLayout,
                                                           ST_QUEUE_FAMILY_IGNORED,
                                                           ST_QUEUE_FAMILY_IGNORED,
                                                           pDstImageViewCube->GetCreateInfoPtr()->pImage,
                                                           {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 6}});
        }
        if (dstInitialLayout != dstFinalLayout) {
            barriers.emplace_back(Shit::ImageMemoryBarrier{
                Shit::AccessFlagBits::MEMORY_READ_BIT,
                Shit::AccessFlagBits::MEMORY_READ_BIT,
                dstInitialLayout,
                dstFinalLayout,
                ST_QUEUE_FAMILY_IGNORED,
                ST_QUEUE_FAMILY_IGNORED,
                pDstImageCube,
                {Shit::ImageAspectFlagBits::COLOR_BIT, 1, pDstImageCube->GetCreateInfoPtr()->mipLevels - 1, 0, 6}});
        }
        pCommandBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                  Shit::PipelineStageFlagBits::ALL_COMMANDS_BIT,
                                                                  {},
                                                                  0,
                                                                  nullptr,
                                                                  0,
                                                                  nullptr,
                                                                  static_cast<uint32_t>(barriers.size()),
                                                                  barriers.data()});
    });

    // takeScreenshot(pDevice, llmImage, Shit::ImageLayout::GENERAL);
    // std::this_thread::sleep_for(std::chrono::seconds(1));

    pDstImageCube->GenerateMipmap(Shit::Filter::LINEAR, dstFinalLayout, dstFinalLayout);
    // release resouce
    pDevice->Destroy(pSrcImageView2D);
    pDevice->Destroy(pDstImageViewCube);
    pDevice->Destroy(llmImageView);
    pDevice->Destroy(llmImage);
    pDevice->Destroy(llmPipelineLayout);
    pDevice->Destroy(llmPipeline);
    pDevice->Destroy(shPipelineLayout);
    pDevice->Destroy(shPipeline);
    pDevice->Destroy(llmShader);
    pDevice->Destroy(shShader);
    pDevice->Destroy(descriptorPool);
}
void generateImage2D(Shit::Device *pDevice, const char *spirvShaderName, Shit::Image *pDstImage2D,
                     Shit::ImageLayout srcLayout, Shit::ImageLayout dstLayout) {
    Shit::ImageView *pOutImageView =
        pDevice->Create(Shit::ImageViewCreateInfo{pDstImage2D,
                                                  Shit::ImageViewType::TYPE_2D,
                                                  pDstImage2D->GetCreateInfoPtr()->format,
                                                  {},
                                                  {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}});
    //===================================
    // config descriptor
    std::vector<Shit::DescriptorSetLayoutBinding> bindings{
        {0, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
    };
    auto descriptorSetLayout =
        pDevice->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)bindings.size(), bindings.data()});
    std::vector<Shit::DescriptorPoolSize> poolSize{
        {Shit::DescriptorType::STORAGE_IMAGE, 1},
    };
    auto descriptorPool =
        pDevice->Create(Shit::DescriptorPoolCreateInfo{1, (uint32_t)poolSize.size(), poolSize.data()});

    std::vector<Shit::DescriptorSet *> descriptorSets;
    descriptorPool->Allocate(Shit::DescriptorSetAllocateInfo{1, &descriptorSetLayout}, descriptorSets);

    // create pipeline layout
    auto pipelineLayout = pDevice->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});

    // create pipeline
    auto shaderPath = buildShaderPath(pDevice, spirvShaderName, g_RendererVersion);
    auto shaderSource = readFile(shaderPath.c_str());
    // auto shaderSourceSpv = compileGlslToSpirv(shaderSource,
    // Shit::ShaderStageFlagBits::COMPUTE_BIT, g_RendererVersion);
    auto shader =
        pDevice->Create(Shit::ShaderCreateInfo{g_shaderSourceLanguage, shaderSource.size(), shaderSource.data()});

    uint32_t constantIDs[] = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X};
    uint32_t constantValues[] = {pDstImage2D->GetCreateInfoPtr()->extent.width};
    auto pipeline = pDevice->Create(Shit::ComputePipelineCreateInfo{
        Shit::PipelineShaderStageCreateInfo{Shit::ShaderStageFlagBits::COMPUTE_BIT, shader, "main",
                                            Shit::SpecializationInfo{1, constantIDs, constantValues}},
        pipelineLayout});

    Shit::DescriptorImageInfo imageInfo{nullptr, pOutImageView, Shit::ImageLayout::GENERAL};
    std::vector<Shit::WriteDescriptorSet> writes{
        {descriptorSets[0], 0, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imageInfo},
    };

    pDevice->UpdateDescriptorSets(writes, {});

    // run pipeline
    pDevice->ExecuteOneTimeCommand([&](Shit::CommandBuffer *pCommandBuffer) {
        // imageLayout tranfser
        if (srcLayout != Shit::ImageLayout::GENERAL) {
            Shit::ImageMemoryBarrier barrier{{},
                                             Shit::AccessFlagBits::MEMORY_WRITE_BIT,
                                             srcLayout,
                                             Shit::ImageLayout::GENERAL,
                                             ST_QUEUE_FAMILY_IGNORED,
                                             ST_QUEUE_FAMILY_IGNORED,
                                             pDstImage2D,
                                             {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}};
            pCommandBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                      Shit::PipelineStageFlagBits::ALL_COMMANDS_BIT,
                                                                      {},
                                                                      0,
                                                                      nullptr,
                                                                      0,
                                                                      nullptr,
                                                                      1,
                                                                      &barrier});
        }

        // process image
        pCommandBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::COMPUTE,
            pipelineLayout,
            0,
            1,
            &descriptorSets[0],
        });
        pCommandBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, pipeline});
        pCommandBuffer->Dispatch(Shit::DispatchInfo{pDstImage2D->GetCreateInfoPtr()->extent.height, 1, 1});
        // convert image to dst image layout
        if (dstLayout != Shit::ImageLayout::GENERAL) {
            Shit::ImageMemoryBarrier barrier{Shit::AccessFlagBits::SHADER_WRITE_BIT,
                                             Shit::AccessFlagBits::MEMORY_READ_BIT,
                                             Shit::ImageLayout::GENERAL,
                                             dstLayout,
                                             ST_QUEUE_FAMILY_IGNORED,
                                             ST_QUEUE_FAMILY_IGNORED,
                                             pDstImage2D,
                                             {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}};
            pCommandBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                      Shit::PipelineStageFlagBits::ALL_COMMANDS_BIT,
                                                                      {},
                                                                      0,
                                                                      nullptr,
                                                                      0,
                                                                      nullptr,
                                                                      1,
                                                                      &barrier});
        }
    });

    pDevice->Destroy(pOutImageView);
    pDevice->Destroy(descriptorPool);
    pDevice->Destroy(descriptorSetLayout);
    pDevice->Destroy(shader);
    pDevice->Destroy(pipeline);
    pDevice->Destroy(pipelineLayout);
}

void generateEquirectangularProceduralSkybox(Shit::Device *pDevice, float atmosphereThickness, float lightDirection[3],
                                             Shit::Image *pDstImage2D, Shit::ImageLayout srcLayout,
                                             Shit::ImageLayout dstLayout) {
    // create sampler
    if (!g_LinearSampler)
        g_LinearSampler = pDevice->Create(Shit::SamplerCreateInfo{
            Shit::Filter::LINEAR,
            Shit::Filter::LINEAR,
            Shit::SamplerMipmapMode::LINEAR,
            Shit::SamplerWrapMode::CLAMP_TO_EDGE,
            Shit::SamplerWrapMode::CLAMP_TO_EDGE,
            Shit::SamplerWrapMode::CLAMP_TO_EDGE,
            0,
            false,
            1.f,
            false,
            {},
            -30.f,
            30.f,
        });

    //=========================
    // create optical depth image
    static Shit::ImageView *pMoleculeOpticalDepthImageView{};
    static Shit::ImageView *pAerosolsOpticalDepthImageView{};
    if (!pMoleculeOpticalDepthImageView) {
        Shit::Image *pMoleculeOpticalDepthImage = pDevice->Create(
            Shit::ImageCreateInfo{{},
                                  Shit::ImageType::TYPE_2D,
                                  Shit::Format::R32G32B32A32_SFLOAT,
                                  {64, 64, 1},
                                  1,
                                  1,
                                  Shit::SampleCountFlagBits::BIT_1,
                                  Shit::ImageTiling::OPTIMAL,
                                  Shit::ImageUsageFlagBits::STORAGE_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT,
                                  Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                  Shit::ImageLayout::UNDEFINED});
        generateOpticalDepthImage(pDevice, 8, pMoleculeOpticalDepthImage, Shit::ImageLayout::UNDEFINED,
                                  Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
        pMoleculeOpticalDepthImageView =
            pDevice->Create(Shit::ImageViewCreateInfo{pMoleculeOpticalDepthImage,
                                                      Shit::ImageViewType::TYPE_2D,
                                                      pMoleculeOpticalDepthImage->GetCreateInfoPtr()->format,
                                                      {},
                                                      {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}});

        Shit::Image *pAerosolsOpticalDepthImage = pDevice->Create(
            Shit::ImageCreateInfo{{},
                                  Shit::ImageType::TYPE_2D,
                                  Shit::Format::R32G32B32A32_SFLOAT,
                                  {64, 64, 1},
                                  1,
                                  1,
                                  Shit::SampleCountFlagBits::BIT_1,
                                  Shit::ImageTiling::OPTIMAL,
                                  Shit::ImageUsageFlagBits::STORAGE_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT,
                                  Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                  Shit::ImageLayout::UNDEFINED});
        generateOpticalDepthImage(pDevice, 1.2, pAerosolsOpticalDepthImage, Shit::ImageLayout::UNDEFINED,
                                  Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
        pAerosolsOpticalDepthImageView =
            pDevice->Create(Shit::ImageViewCreateInfo{pAerosolsOpticalDepthImage,
                                                      Shit::ImageViewType::TYPE_2D,
                                                      pAerosolsOpticalDepthImage->GetCreateInfoPtr()->format,
                                                      {},
                                                      {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}});
    }
    // takeScreenshot(pDevice, pMoleculeOpticalDepthImage,
    // Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // takeScreenshot(pDevice, pAerosolsOpticalDepthImage,
    // Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    // std::this_thread::sleep_for(std::chrono::seconds(1));

    //===============
    Shit::ImageView *pDstImageView =
        pDevice->Create(Shit::ImageViewCreateInfo{pDstImage2D,
                                                  Shit::ImageViewType::TYPE_2D,
                                                  pDstImage2D->GetCreateInfoPtr()->format,
                                                  {},
                                                  {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}});

    std::vector<Shit::DescriptorSetLayoutBinding> cubemapComputeBindings{
        Shit::DescriptorSetLayoutBinding{0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1,
                                         Shit::ShaderStageFlagBits::COMPUTE_BIT},
        Shit::DescriptorSetLayoutBinding{1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1,
                                         Shit::ShaderStageFlagBits::COMPUTE_BIT},
        Shit::DescriptorSetLayoutBinding{2, Shit::DescriptorType::STORAGE_IMAGE, 1,
                                         Shit::ShaderStageFlagBits::COMPUTE_BIT},
        Shit::DescriptorSetLayoutBinding{3, Shit::DescriptorType::UNIFORM_BUFFER, 1,
                                         Shit::ShaderStageFlagBits::COMPUTE_BIT},
    };
    Shit::DescriptorSetLayout *setLayout = pDevice->Create(
        Shit::DescriptorSetLayoutCreateInfo{(uint32_t)cubemapComputeBindings.size(), cubemapComputeBindings.data()});

    // setup descriptor pool
    std::vector<Shit::DescriptorPoolSize> poolSizes{
        {cubemapComputeBindings[0].descriptorType, cubemapComputeBindings[0].descriptorCount},
        {cubemapComputeBindings[1].descriptorType, cubemapComputeBindings[1].descriptorCount},
        {cubemapComputeBindings[2].descriptorType, cubemapComputeBindings[2].descriptorCount},
        {cubemapComputeBindings[3].descriptorType, cubemapComputeBindings[3].descriptorCount},
    };
    Shit::DescriptorPoolCreateInfo descriptorPoolCreateInfo{1, poolSizes.size(), poolSizes.data()};
    Shit::DescriptorPool *descriptorPool = pDevice->Create(descriptorPoolCreateInfo);

    // allocate compute descriptor set
    Shit::DescriptorSetAllocateInfo allocInfo{1, &setLayout};
    std::vector<Shit::DescriptorSet *> descriptorSets;
    descriptorPool->Allocate(allocInfo, descriptorSets);

    // create pipeline layout
    Shit::PipelineLayout *pipelineLayout = pDevice->Create(Shit::PipelineLayoutCreateInfo{1, &setLayout});
    // create pipeline
    auto shaderPath = buildShaderPath(pDevice, "skyboxProcedural2D.comp", g_RendererVersion);
    auto shaderSource = readFile(shaderPath.c_str());
    // auto shaderSource = compileGlslToSpirv(readFile(shaderPath.c_str()),
    // Shit::ShaderStageFlagBits::COMPUTE_BIT, g_RendererVersion);
    auto shader =
        pDevice->Create(Shit::ShaderCreateInfo{g_shaderSourceLanguage, shaderSource.size(), shaderSource.data()});

    uint32_t constantIDs[] = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X};
    uint32_t constantValues[] = {pDstImage2D->GetCreateInfoPtr()->extent.width};
    auto pipeline = pDevice->Create(Shit::ComputePipelineCreateInfo{
        Shit::PipelineShaderStageCreateInfo{Shit::ShaderStageFlagBits::COMPUTE_BIT, shader, "main",
                                            Shit::SpecializationInfo{1, constantIDs, constantValues}},
        pipelineLayout});

    // update desccriptor sets
    struct UBOAA {
        float lightDirection[3];
        float atmosphereThickness;
    } uboaa{{lightDirection[0], lightDirection[1], lightDirection[2]}, atmosphereThickness};
    Shit::Buffer *ubobuffer = pDevice->Create(Shit::BufferCreateInfo{{},
                                                                     sizeof(UBOAA),
                                                                     Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT |
                                                                         Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
                                                                     Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
                                              &uboaa);

    Shit::DescriptorImageInfo imagesInfo[]{
        {g_LinearSampler, pMoleculeOpticalDepthImageView, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        {g_LinearSampler, pAerosolsOpticalDepthImageView, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        {nullptr, pDstImageView, Shit::ImageLayout::GENERAL},
    };
    Shit::DescriptorBufferInfo bufferInfo{ubobuffer, 0, sizeof(UBOAA)};

    std::vector<Shit::WriteDescriptorSet> writes{
        {descriptorSets[0], 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo[0]},
        {descriptorSets[0], 1, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo[1]},
        {descriptorSets[0], 2, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[2]},
        {descriptorSets[0], 3, 0, 1, Shit::DescriptorType::UNIFORM_BUFFER, nullptr, &bufferInfo},
    };

    pDevice->UpdateDescriptorSets(writes, {});
    ///=========================
    pDevice->ExecuteOneTimeCommand([&](Shit::CommandBuffer *pCommandBuffer) {
        // imageLayout tranfser
        if (srcLayout != Shit::ImageLayout::GENERAL) {
            Shit::ImageMemoryBarrier barrier{
                {},
                Shit::AccessFlagBits::MEMORY_WRITE_BIT,
                srcLayout,
                Shit::ImageLayout::GENERAL,
                ST_QUEUE_FAMILY_IGNORED,
                ST_QUEUE_FAMILY_IGNORED,
                pDstImage2D,
                {Shit::ImageAspectFlagBits::COLOR_BIT, 0, pDstImage2D->GetCreateInfoPtr()->mipLevels, 0, 1}};
            pCommandBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                      Shit::PipelineStageFlagBits::ALL_COMMANDS_BIT,
                                                                      {},
                                                                      0,
                                                                      nullptr,
                                                                      0,
                                                                      nullptr,
                                                                      1,
                                                                      &barrier});
        }
        pCommandBuffer->BindDescriptorSets(
            Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::COMPUTE, pipelineLayout, 0, 1, &descriptorSets[0]});
        pCommandBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, pipeline});
        pCommandBuffer->Dispatch(Shit::DispatchInfo{pDstImage2D->GetCreateInfoPtr()->extent.height, 1, 1});

        Shit::ImageMemoryBarrier barrier{
            Shit::AccessFlagBits::MEMORY_WRITE_BIT,
            Shit::AccessFlagBits::MEMORY_READ_BIT,
            Shit::ImageLayout::GENERAL,
            Shit::ImageLayout::GENERAL,
            ST_QUEUE_FAMILY_IGNORED,
            ST_QUEUE_FAMILY_IGNORED,
            pDstImage2D,
            {Shit::ImageAspectFlagBits::COLOR_BIT, 0, pDstImage2D->GetCreateInfoPtr()->mipLevels, 0, 1}};
        pCommandBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                  Shit::PipelineStageFlagBits::ALL_COMMANDS_BIT,
                                                                  {},
                                                                  0,
                                                                  nullptr,
                                                                  0,
                                                                  nullptr,
                                                                  1,
                                                                  &barrier});
    });
    pDevice->Destroy(ubobuffer);
    pDevice->Destroy(descriptorPool);
    pDevice->Destroy(setLayout);
    pDevice->Destroy(pDstImageView);
    pDevice->Destroy(pipeline);
    pDevice->Destroy(shader);
    pDevice->WaitIdle();
    pDstImage2D->GenerateMipmap(Shit::Filter::LINEAR, Shit::ImageLayout::GENERAL, dstLayout);
}
void generateOpticalDepthImage(Shit::Device *pDevice, float H0, Shit::Image *pDstImage2D, Shit::ImageLayout srcLayout,
                               Shit::ImageLayout dstLayout) {
    static Shit::Buffer *H0Buffer = pDevice->Create(
        Shit::BufferCreateInfo{
            {},
            sizeof(float),
            Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
            Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT | Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
        nullptr);
    void *data;
    H0Buffer->MapMemory(0, sizeof(float), &data);
    memcpy(data, &H0, sizeof(float));
    H0Buffer->UnMapMemory();

    Shit::ImageView *pOutImageView =
        pDevice->Create(Shit::ImageViewCreateInfo{pDstImage2D,
                                                  Shit::ImageViewType::TYPE_2D,
                                                  pDstImage2D->GetCreateInfoPtr()->format,
                                                  {},
                                                  {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}});
    //===================================
    // config descriptor
    std::vector<Shit::DescriptorSetLayoutBinding> bindings{
        {0, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
        {1, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
    };
    auto descriptorSetLayout =
        pDevice->Create(Shit::DescriptorSetLayoutCreateInfo{static_cast<uint32_t>(bindings.size()), bindings.data()});
    std::vector<Shit::DescriptorPoolSize> poolSize{
        {Shit::DescriptorType::STORAGE_IMAGE, 1},
        {Shit::DescriptorType::UNIFORM_BUFFER, 1},
    };
    auto descriptorPool =
        pDevice->Create(Shit::DescriptorPoolCreateInfo{1, static_cast<uint32_t>(poolSize.size()), poolSize.data()});

    std::vector<Shit::DescriptorSet *> descriptorSets;
    descriptorPool->Allocate(Shit::DescriptorSetAllocateInfo{1, &descriptorSetLayout}, descriptorSets);

    // create pipeline layout
    auto pipelineLayout = pDevice->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});

    // create pipeline
    const char *spirvShaderName = "opticalDepth.comp";
    auto shaderPath = buildShaderPath(pDevice, spirvShaderName, g_RendererVersion);
    auto shaderSource = readFile(shaderPath.c_str());
    // auto shaderSource = compileGlslToSpirv(readFile(shaderPath.c_str()),
    // Shit::ShaderStageFlagBits::COMPUTE_BIT, g_RendererVersion);
    auto shader =
        pDevice->Create(Shit::ShaderCreateInfo{g_shaderSourceLanguage, shaderSource.size(), shaderSource.data()});

    uint32_t constantIDs[] = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X};
    uint32_t constantValues[] = {pDstImage2D->GetCreateInfoPtr()->extent.width};
    auto pipeline = pDevice->Create(Shit::ComputePipelineCreateInfo{
        Shit::PipelineShaderStageCreateInfo{Shit::ShaderStageFlagBits::COMPUTE_BIT, shader, "main",
                                            Shit::SpecializationInfo{1, constantIDs, constantValues}},
        pipelineLayout});

    Shit::DescriptorImageInfo imageInfo{nullptr, pOutImageView, Shit::ImageLayout::GENERAL};
    Shit::DescriptorBufferInfo bufferInfo{H0Buffer, 0, sizeof(float)};
    std::vector<Shit::WriteDescriptorSet> writes{
        {descriptorSets[0], 0, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imageInfo},
        {descriptorSets[0], 1, 0, 1, Shit::DescriptorType::UNIFORM_BUFFER, nullptr, &bufferInfo},
    };

    pDevice->UpdateDescriptorSets(writes, {});

    // run pipeline
    pDevice->ExecuteOneTimeCommand([&](Shit::CommandBuffer *pCommandBuffer) {
        // imageLayout tranfser
        if (srcLayout != Shit::ImageLayout::GENERAL) {
            Shit::ImageMemoryBarrier barrier{{},
                                             Shit::AccessFlagBits::MEMORY_WRITE_BIT,
                                             srcLayout,
                                             Shit::ImageLayout::GENERAL,
                                             ST_QUEUE_FAMILY_IGNORED,
                                             ST_QUEUE_FAMILY_IGNORED,
                                             pDstImage2D,
                                             {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}};
            pCommandBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                      Shit::PipelineStageFlagBits::ALL_COMMANDS_BIT,
                                                                      {},
                                                                      0,
                                                                      nullptr,
                                                                      0,
                                                                      nullptr,
                                                                      1,
                                                                      &barrier});
        }

        // process image
        pCommandBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::COMPUTE,
            pipelineLayout,
            0,
            1,
            &descriptorSets[0],
        });
        pCommandBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, pipeline});
        pCommandBuffer->Dispatch(Shit::DispatchInfo{pDstImage2D->GetCreateInfoPtr()->extent.height, 1, 1});
        // convert image to dst image layout
        if (dstLayout != Shit::ImageLayout::GENERAL) {
            Shit::ImageMemoryBarrier barrier{Shit::AccessFlagBits::SHADER_WRITE_BIT,
                                             Shit::AccessFlagBits::MEMORY_READ_BIT,
                                             Shit::ImageLayout::GENERAL,
                                             dstLayout,
                                             ST_QUEUE_FAMILY_IGNORED,
                                             ST_QUEUE_FAMILY_IGNORED,
                                             pDstImage2D,
                                             {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}};
            pCommandBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                      Shit::PipelineStageFlagBits::ALL_COMMANDS_BIT,
                                                                      {},
                                                                      0,
                                                                      nullptr,
                                                                      0,
                                                                      nullptr,
                                                                      1,
                                                                      &barrier});
        }
    });

    pDevice->Destroy(pOutImageView);
    pDevice->Destroy(descriptorPool);
    pDevice->Destroy(descriptorSetLayout);
    pDevice->Destroy(shader);
    pDevice->Destroy(pipeline);
    pDevice->Destroy(pipelineLayout);
}
// void initImgui(Shit::Device *pDevice,
//			   Shit::Window *pWindow,
//			   uint32_t imageCount,
//			//   Shit::SampleCountFlagBits sampleCount,
//			//   Shit::RenderPass *pRenderPass,
//			//   uint32_t subpass,
//			   std::vector<Shit::CommandBuffer *> &commandBuffers)
//{
//	auto graphicsQueueFamilyIndex =
// pDevice->GetQueueFamilyIndexByFlag(Shit::QueueFlagBits::GRAPHICS_BIT, {});
//	auto longLiveCommandPool = pDevice->Create(
//		Shit::CommandPoolCreateInfo{{},
// graphicsQueueFamilyIndex->index});
//	//auto _shortLiveCommandPool = pDevice->Create(
//	//
// Shit::CommandPoolCreateInfo{Shit::CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT,
// graphicsQueueFamilyIndex->index});

//	// Setup Dear ImGui context
//	IMGUI_CHECKVERSION();
//	ImGui::CreateContext();
//	ImGuiIO &io = ImGui::GetIO();
//	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable
// Keyboard Controls
//	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable
// Gamepad Controls 	uint32_t windowWidth, windowHeight;
//	pWindow->GetFramebufferSize(windowWidth, windowHeight);
//	io.DisplaySize = ImVec2{float(windowWidth), float(windowHeight)};

//	// Setup Dear ImGui style
//	ImGui::StyleColorsDark();
//	//ImGui::StyleColorsClassic();

//	longLiveCommandPool->CreateCommandBuffers(
//		Shit::CommandBufferCreateInfo{
//			Shit::CommandBufferLevel::PRIMARY,
//			static_cast<uint32_t>(imageCount)},
//		commandBuffers);

//	//_shortLiveCommandPool->CreateCommandBuffers(
//	//	Shit::CommandBufferCreateInfo{
//	//		Shit::CommandBufferLevel::SECONDARY,
//	//		static_cast<uint32_t>(imageCount)},
//	//	commandBuffers);

//	ImGui_ImplShitRenderer_InitInfo imguiInitInfo{
//		g_RendererVersion,
//		pDevice,
//		commandBuffers,
//		imageCount,
//		sampleCount};
//	ImGui_ImplShitRenderer_Init(&imguiInitInfo);

//	executeOneTimeCommands(pDevice, [](Shit::CommandBuffer *commandBuffer)
//						   {
// ImGui_ImplShitRenderer_CreateFontsTexture(commandBuffer); });
//	ImGui_ImplShitRenderer_DestroyFontUploadObjects();
//}
//=======================================
FilterKernel::FilterKernel(FilterPattern filterType, uint32_t radius) {
    switch (filterType) {
        case FilterPattern::DELTA:
            delta();
            break;
        case FilterPattern::BOX:
            boxKernel(radius);
            break;
        case FilterPattern::GAUSSIAN:
            gaussianKernel(radius);
            break;
        case FilterPattern::SECONDORDER_DERIVATIVE:
            secondorder_derivative();
            break;
    }
}
void FilterKernel::delta() { _kernel2D.resize(1, 1); }
void FilterKernel::gaussianKernel(uint32_t radius) {
    uint32_t W = radius * 2 + 1;
    float sigma = float(W) / 6;
    std::vector<float> a(W);
    _kernel2D.resize(W * W);
    float normalFactor = 0;

    for (int i = -radius; i <= int(radius); ++i) {
        normalFactor += a[i + radius] = gaussian(i, sigma);
    }
    for (auto &e : a) e /= normalFactor;
    for (int i = -radius; i <= (int)radius; ++i) {
        for (int j = -radius; j <= (int)radius; ++j) {
            _kernel2D[(i + radius) * W + j + radius] = a[i + radius] * a[j + radius];
        }
    }
}
void FilterKernel::boxKernel(uint32_t radius) {
    uint32_t W = radius * 2 + 1;
    uint32_t l = W * W;
    _kernel2D.resize(l, 1.f / l);
}
void FilterKernel::secondorder_derivative() {
    _kernel2D.resize(9, -1);
    _kernel2D[4] = 8;
}
FilterKernel FilterKernel::operator+(FilterKernel const &other) const {
    FilterKernel ret;
    auto R1 = getRadius();
    auto R2 = other.getRadius();
    FilterKernel const *pLarge;
    FilterKernel const *pSmall;
    if (R1 > R2) {
        pLarge = this;
        pSmall = &other;
    } else {
        pLarge = &other;
        pSmall = this;
    }
    ret = *pLarge;
    auto Rl = pLarge->getRadius();
    auto Rs = pSmall->getRadius();
    auto Wl = Rl * 2 + 1;
    auto Ws = Rs * 2 + 1;
    int dR = Rl - Rs;
    for (int i = 0; i < Ws; ++i) {
        for (int j = 0; j < Ws; ++j) {
            ret._kernel2D[(i + dR) * Wl + j + dR] += pSmall->_kernel2D[i * Ws + j];
        }
    }
    return ret;
}