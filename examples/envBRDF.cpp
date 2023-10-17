#include "common/appbase.h"
#include "common/entry.h"
auto shaderName = "common/envBRDF.comp";
// auto shaderName = "common/sheenEnvBRDF.comp";

#define WIDTH 64

class Hello {
    Shit::RenderSystem *renderSystem;
    Shit::Device *device;

    Shit::Pipeline *pipeline;
    Shit::DescriptorPool *descriptorPool;
    Shit::DescriptorSet *descriptorSet;

public:
    void initRenderSystem() {
        Shit::RenderSystemCreateInfo renderSystemCreateInfo{
            .version = g_RendererVersion,
            .flags = Shit::RenderSystemCreateFlagBits::CONTEXT_DEBUG_BIT,
        };

        renderSystem = LoadRenderSystem(renderSystemCreateInfo);
        // 1.5 choose phyiscal device
        // 2. create device of a physical device
        Shit::DeviceCreateInfo deviceCreateInfo{};

        device = renderSystem->CreateDevice(deviceCreateInfo);

        processImage();
    }

    void cleanUp() {}
    void run() { initRenderSystem(); }

    void preparePipeline() {
        Shit::DescriptorSetLayoutBinding bindings[]{
            {0, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
        };
        auto descriptorSetLayout = device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});
        // pipeline

        std::vector<Shit::DescriptorPoolSize> poolSize{
            {Shit::DescriptorType::STORAGE_IMAGE, 1},
        };
        auto descriptorPool =
            device->Create(Shit::DescriptorPoolCreateInfo{1, (uint32_t)poolSize.size(), poolSize.data()});
        descriptorPool->Allocate({1, &descriptorSetLayout}, &descriptorSet);

        auto pipelineLayout = device->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});

        auto shaderPath = buildShaderPath(device, shaderName, g_RendererVersion);
        auto shaderSource = readFile(shaderPath.c_str());
        // auto shaderSourceSpv = compileGlslToSpirv(shaderSource,
        // Shit::ShaderStageFlagBits::COMPUTE_BIT, g_RendererVersion);
        auto shader =
            device->Create(Shit::ShaderCreateInfo{g_shaderSourceLanguage, shaderSource.size(), shaderSource.data()});

        uint32_t constantIDs[] = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X};
        uint32_t constantValues[] = {WIDTH};
        auto stageCI = Shit::PipelineShaderStageCreateInfo{Shit::ShaderStageFlagBits::COMPUTE_BIT, shader, "main",
                                                           Shit::SpecializationInfo{1, constantIDs, constantValues}};
        pipeline = device->Create(Shit::ComputePipelineCreateInfo{stageCI, pipelineLayout});
    }
    void processImage() {
        preparePipeline();
        auto envBRDFImage = device->Create(Shit::ImageCreateInfo{{},
                                                                 Shit::ImageType::TYPE_2D,
                                                                 Shit::Format::R32G32B32A32_SFLOAT,
                                                                 {WIDTH, WIDTH, 1},
                                                                 1,
                                                                 1,
                                                                 Shit::SampleCountFlagBits::BIT_1,
                                                                 Shit::ImageTiling::OPTIMAL,
                                                                 Shit::ImageUsageFlagBits::STORAGE_BIT,
                                                                 Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                                                 Shit::ImageLayout::GENERAL});
        auto envBRDFImageView =
            device->Create(Shit::ImageViewCreateInfo{envBRDFImage,
                                                     Shit::ImageViewType::TYPE_2D,
                                                     Shit::Format::R32G32B32A32_SFLOAT,
                                                     {},
                                                     {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}});

        Shit::DescriptorImageInfo imagesInfo[]{{0, envBRDFImageView, Shit::ImageLayout::GENERAL}};
        Shit::WriteDescriptorSet writes[]{
            {descriptorSet, 0, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[0]}};
        device->UpdateDescriptorSets(writes);

        device->ExecuteOneTimeCommand([&](Shit::CommandBuffer *cmdBuffer) {
            auto pipelineLayout = dynamic_cast<Shit::ComputePipeline *>(pipeline)->GetCreateInfoPtr()->pLayout;
            cmdBuffer->BindPipeline({Shit::PipelineBindPoint::COMPUTE, pipeline});
            cmdBuffer->BindDescriptorSets({Shit::PipelineBindPoint::COMPUTE, pipelineLayout, 0, 1, &descriptorSet});
            cmdBuffer->Dispatch({WIDTH, 1, 1});
        });
        takeScreenshot(device, envBRDFImage, Shit::ImageLayout::GENERAL);
    }
};
EXAMPLE_MAIN(Hello)