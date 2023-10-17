#include "environment.h"

#include "appbase.h"

static auto eq2CubeShaderPath = "common/equirectangular2cube.comp";
static auto skyboxVertShaderPath = "skybox/skybox.vert";
static auto skyboxFragShaderPath = "skybox/skybox.frag";
static auto prefilteredEnvCompShaderName = "common/prefilteredEnv2D.comp";

#define SKYBOX_WIDTH 1024

Environment::PipelineInfo Environment::_eq2cubePipelineInfo{};
Environment::PipelineInfo Environment::_skyboxPipelineInfo{};
Environment::PipelineInfo Environment::_prefilteredEnvPipelineInfo{};
Shit::RenderPass *Environment::_renderPass{};  // compatible renderpass

Environment::Environment(Image const *pEqImage) : _envEquirectangularImage(pEqImage) {
    if (!_renderPass) {
        // createRenderPass(Shit::Format::R8G8B8A8_SRGB);
        createRenderPass(Shit::Format::B8G8R8A8_SRGB);
        createEq2CubePipeline();
        createSkyboxPipeline();
        createPrefilteredEnvPipelineInfo();
    }
    _envTex = g_App->getTextureManager()->createTexture(
        Shit::ImageCreateInfo{Shit::ImageCreateFlagBits::CUBE_COMPATIBLE_BIT,
                              Shit::ImageType::TYPE_2D,
                              Shit::Format::R32G32B32A32_SFLOAT,
                              {SKYBOX_WIDTH, SKYBOX_WIDTH, 1},
                              0,
                              6,
                              Shit::SampleCountFlagBits::BIT_1,
                              Shit::ImageTiling::OPTIMAL,
                              Shit::ImageUsageFlagBits::SAMPLED_BIT | Shit::ImageUsageFlagBits::STORAGE_BIT |
                                  Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
                              Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                              Shit::ImageLayout::GENERAL},
        Shit::ImageViewType::TYPE_CUBE, 0);
    _envTex->prepare();
}
Environment::~Environment() {
    if (_envEquirectangularTex) g_App->getTextureManager()->removeTextureById(_envEquirectangularTex->getId());
    if (_envTex) g_App->getTextureManager()->removeTextureById(_envTex->getId());
}
void Environment::createRenderPass(Shit::Format format) {
    std::vector<Shit::AttachmentDescription> attachmentDesc{
        // pass 1
        // color
        {format, Shit::SampleCountFlagBits::BIT_1, Shit::AttachmentLoadOp::CLEAR, Shit::AttachmentStoreOp::STORE,
         Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE, Shit::ImageLayout::UNDEFINED,
         Shit::ImageLayout::PRESENT_SRC},
    };

    Shit::AttachmentReference colorAttachment{0, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

    Shit::SubpassDescription subpassDesc[]{
        {Shit::PipelineBindPoint::GRAPHICS, 0, 0, 1, &colorAttachment},
    };

    _renderPass = g_App->getDevice()->Create(Shit::RenderPassCreateInfo{
        (uint32_t)attachmentDesc.size(), attachmentDesc.data(), (uint32_t)std::size(subpassDesc), subpassDesc});
}
void Environment::createEq2CubePipeline() {
    //==============
    // pipeline layout
    Shit::DescriptorSetLayoutBinding bindings[]{
        //
        {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
        {1, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
    };

    auto descriptorSetLayout =
        g_App->getDevice()->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), &bindings[0]});

    // allocate eq2cubeDescriptorSet
    _eq2cubePipelineInfo.descriptorSets.resize(1);
    g_App->getDescriptorPool()->Allocate({1, &descriptorSetLayout}, _eq2cubePipelineInfo.descriptorSets.data());

    _eq2cubePipelineInfo.pipelineLayout =
        g_App->getDevice()->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});

    //==============
    _eq2cubePipelineInfo.shaders = {static_cast<Shader *>(g_App->getShaderManager()->create(
        buildShaderPath(g_App->getDevice(), eq2CubeShaderPath, g_App->getRendererVersion()),
        ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "COMP"}}))};

    _eq2cubePipelineInfo.shaders[0]->load();

    // generate cubemap pipeline
    uint32_t constantIDs0[] = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X};
    uint32_t constantValues0[] = {SKYBOX_WIDTH};  // cubemap width

    auto shaderStageCIs = _eq2cubePipelineInfo.getStageCIs();
    shaderStageCIs[0].specializationInfo = {1, constantIDs0, constantValues0};

    _eq2cubePipelineInfo.pipeline = g_App->getDevice()->Create(
        Shit::ComputePipelineCreateInfo{shaderStageCIs[0], _eq2cubePipelineInfo.pipelineLayout});
}
void Environment::createSkyboxPipeline() {
    // skybox pipeline
    Shit::DescriptorSetLayoutBinding bindings[]{
        {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
    };

    Shit::DescriptorSetLayout *descriptorSetLayouts[]{
        g_App->getDevice()->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings}),
        g_App->getCameraDescriptorSetLayout(),
    };

    // allocate descriptorsets
    _skyboxPipelineInfo.descriptorSets.resize(1);
    g_App->getDescriptorPool()->Allocate({1u, &descriptorSetLayouts[0]}, _skyboxPipelineInfo.descriptorSets.data());

    _skyboxPipelineInfo.pipelineLayout = g_App->getDevice()->Create(
        Shit::PipelineLayoutCreateInfo{std::size(descriptorSetLayouts), descriptorSetLayouts});

    //==============
    _skyboxPipelineInfo.shaders = {
        static_cast<Shader *>(g_App->getShaderManager()->create(
            buildShaderPath(g_App->getDevice(), skyboxVertShaderPath, g_App->getRendererVersion()),
            ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
        static_cast<Shader *>(g_App->getShaderManager()->create(
            buildShaderPath(g_App->getDevice(), skyboxFragShaderPath, g_App->getRendererVersion()),
            ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}})),
    };

    auto shaderStageCIs = _skyboxPipelineInfo.getStageCIs();

    Shit::PipelineColorBlendAttachmentState colorBlendAttachmentState{
        true,
        Shit::BlendFactor::ONE_MINUS_DST_ALPHA,
        Shit::BlendFactor::DST_ALPHA,
        Shit::BlendOp::ADD,
        Shit::BlendFactor::ONE_MINUS_DST_ALPHA,
        Shit::BlendFactor::ONE,
        Shit::BlendOp::ADD,
    };
    colorBlendAttachmentState.colorWriteMask =
        Shit::ColorComponentFlagBits::R_BIT | Shit::ColorComponentFlagBits::G_BIT |
        Shit::ColorComponentFlagBits::B_BIT | Shit::ColorComponentFlagBits::A_BIT;

    Shit::DynamicState dynamicStates[]{Shit::DynamicState::VIEWPORT, Shit::DynamicState::SCISSOR};
    Shit::Viewport viewport{};
    Shit::Rect2D scissor{};

    _skyboxPipelineInfo.pipeline = g_App->getDevice()->Create(Shit::GraphicsPipelineCreateInfo{
        (uint32_t)shaderStageCIs.size(),
        shaderStageCIs.data(),
        {},
        {Shit::PrimitiveTopology::TRIANGLE_LIST},  // PipelineInputAssemblyStateCreateInfo
        {1, &viewport, 1, &scissor},               // PipelineViewportStateCreateInfo
        {},                                        // PipelineTessellationStateCreateInfo
        {false, false, Shit::PolygonMode::FILL, Shit::CullMode::BACK, Shit::FrontFace::COUNTER_CLOCKWISE, false, 0, 0,
         0, 1.f},  // PipelineRasterizationStateCreateInfo
        {
            Shit::SampleCountFlagBits::BIT_1,
        },  // PipelineMultisampleStateCreateInfo
        {
            false,
        },  // PipelineDepthStencilStateCreateInfo
        {
            false,
            {},
            1,
            &colorBlendAttachmentState,
        },  // PipelineColorBlendStateCreateInfo
        {
            (uint32_t)std::size(dynamicStates),
            dynamicStates,
        },  // PipelineDynamicStateCreateInfo
        _skyboxPipelineInfo.pipelineLayout,
        _renderPass,
        0});
}
void Environment::createPrefilteredEnvPipelineInfo() {
    Shit::DescriptorSetLayoutBinding bindings[]{
        //
        {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
        {1, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
    };
    Shit::PushConstantRange pushConstantRanges[]{
        {Shit::ShaderStageFlagBits::COMPUTE_BIT, 2, 0, sizeof(float) * 2},
    };

    auto descriptorSetLayout =
        g_App->getDevice()->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});
    std::vector<Shit::DescriptorSetLayout const *> setLayouts(_roughness1Level + 1, descriptorSetLayout);
    _prefilteredEnvPipelineInfo.descriptorSets.resize(setLayouts.size());
    g_App->getDescriptorPool()->Allocate({(uint32_t)setLayouts.size(), setLayouts.data()},
                                         _prefilteredEnvPipelineInfo.descriptorSets.data());

    _prefilteredEnvPipelineInfo.pipelineLayout = g_App->getDevice()->Create(
        Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout, std::size(pushConstantRanges), pushConstantRanges});

    //==============
    _prefilteredEnvPipelineInfo.shaders = {static_cast<Shader *>(g_App->getShaderManager()->create(
        buildShaderPath(g_App->getDevice(), prefilteredEnvCompShaderName, g_App->getRendererVersion()),
        ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "COMP"}}))};

    auto shaderStageCIs = _prefilteredEnvPipelineInfo.getStageCIs();

    _prefilteredEnvPipelineInfo.pipeline = g_App->getDevice()->Create(
        Shit::ComputePipelineCreateInfo{shaderStageCIs[0], _prefilteredEnvPipelineInfo.pipelineLayout});
}

// void Environment::destroyRes()
//{
//	if (_envTex)
//		g_App->getTextureManager()->removeTextureById(_envTex->getId());
//	if (_envEquirectangularTex)
//		g_App->getTextureManager()->removeTextureById(_envEquirectangularTex->getId());
// }
void Environment::createEqTex() {
    _envEquirectangularTex = g_App->getTextureManager()->createTexture(_envEquirectangularImage);
    _envEquirectangularTex->prepare();
    _envEquirectangularTex->getImage()->GenerateMipmap(
        Shit::Filter::LINEAR, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
}
void Environment::generateSkyBox() {
    auto sampler = g_App->getSampler("linear");

    // write descriptor set
    Shit::DescriptorImageInfo imagesInfo[]{
        {sampler, _envEquirectangularTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        {sampler, _envTex->getImageView(), Shit::ImageLayout::GENERAL},
    };
    std::vector<Shit::WriteDescriptorSet> writes{
        {_eq2cubePipelineInfo.descriptorSets[0], 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo[0]},
        {_eq2cubePipelineInfo.descriptorSets[0], 1, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[1]},
    };
    g_App->getDevice()->UpdateDescriptorSets(writes);

    // run pipeline
    g_App->getDevice()->ExecuteOneTimeCommand([&](Shit::CommandBuffer *cmdBuffer) {
        cmdBuffer->BindPipeline(
            Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, _eq2cubePipelineInfo.pipeline});

        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::COMPUTE,
                                                                   _eq2cubePipelineInfo.pipelineLayout, 0, 1,
                                                                   &_eq2cubePipelineInfo.descriptorSets[0]});
        cmdBuffer->Dispatch({1024, 6, 1});

        // transfer imagelayout to shder readonly
        Shit::ImageMemoryBarrier barrier{
            Shit::AccessFlagBits::SHADER_WRITE_BIT,
            Shit::AccessFlagBits::TRANSFER_READ_BIT,
            Shit::ImageLayout::GENERAL,
            Shit::ImageLayout::GENERAL,
            ST_QUEUE_FAMILY_IGNORED,
            ST_QUEUE_FAMILY_IGNORED,
            _envTex->getImage(),
            {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, _envTex->getImage()->GetCreateInfoPtr()->arrayLayers}};
        cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                             Shit::PipelineStageFlagBits::TRANSFER_BIT,
                                                             {},
                                                             0,
                                                             0,
                                                             0,
                                                             0,
                                                             1,
                                                             &barrier});
        // cmdBuffer->GenerateMipmap({_envTex->getImage(), Shit::Filter::LINEAR,
        //						   Shit::ImageLayout::GENERAL,
        // Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL});
    });
}
void Environment::generateReflectionTex() {
    auto sampler = g_App->getSampler("trilinear");
    std::vector<Shit::DescriptorImageInfo> imageInfos{
        {sampler, _envEquirectangularTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL}, {}};

    std::vector<Shit::ImageView *> cubeTexImageViews(_roughness1Level + 1, 0);
    for (uint32_t i = 0; i <= _roughness1Level; ++i) {
        cubeTexImageViews[i] = g_App->getDevice()->Create(Shit::ImageViewCreateInfo{
            _envTex->getImage(),
            Shit::ImageViewType::TYPE_CUBE,
            _envTex->getImage()->GetCreateInfoPtr()->format,
            {},
            {Shit::ImageAspectFlagBits::COLOR_BIT, i, 1, 0, _envTex->getImage()->GetCreateInfoPtr()->arrayLayers}});
        imageInfos[1] = {nullptr, cubeTexImageViews[i], Shit::ImageLayout::GENERAL};
        Shit::WriteDescriptorSet writeSets[]{
            {_prefilteredEnvPipelineInfo.descriptorSets[i], 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
             &imageInfos[0]},
            {_prefilteredEnvPipelineInfo.descriptorSets[i], 1, 0, 1, Shit::DescriptorType::STORAGE_IMAGE,
             &imageInfos[1]},
        };
        g_App->getDevice()->UpdateDescriptorSets(writeSets);
    }

    // run pipeline
    g_App->getDevice()->ExecuteOneTimeCommand([&](Shit::CommandBuffer *cmdBuffer) {
        cmdBuffer->BindPipeline(
            Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, _prefilteredEnvPipelineInfo.pipeline});

        auto pipeline = dynamic_cast<Shit::ComputePipeline *>(_prefilteredEnvPipelineInfo.pipeline);
        auto descriptorSetLayout = pipeline->GetCreateInfoPtr()->pLayout->GetCreateInfoPtr()->pSetLayouts[0];
        uint32_t width = SKYBOX_WIDTH;
        int sampleNum = 512;
        float a[2];
        memcpy(&a[1], &sampleNum, sizeof(int));
        int i = 1;
        for (; i <= _roughness1Level; ++i) {
            width /= 2;
            a[0] = float(i) / _roughness1Level;
            cmdBuffer->PushConstants(Shit::PushConstantInfo{pipeline->GetCreateInfoPtr()->pLayout,
                                                            Shit::ShaderStageFlagBits::COMPUTE_BIT, 0,
                                                            sizeof(float) * 2, a});
            cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::COMPUTE,
                                                                       pipeline->GetCreateInfoPtr()->pLayout, 0, 1,
                                                                       &_prefilteredEnvPipelineInfo.descriptorSets[i]});
            cmdBuffer->Dispatch({width, 6, 1});
        }
        //// transfer imagelayout to shder readonly
        // Shit::ImageMemoryBarrier barrier{
        //	Shit::AccessFlagBits::SHADER_WRITE_BIT,
        //	Shit::AccessFlagBits::SHADER_READ_BIT,
        //	Shit::ImageLayout::GENERAL,
        //	Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
        //	cmdBuffer->GetCommandPool()->GetCreateInfoPtr()->queueFamilyIndex,
        //	g_App->getGraphicsQueue()->GetFamilyIndex(),
        //	_envTex->getImage(),
        //	{Shit::ImageAspectFlagBits::COLOR_BIT,
        //	 0,
        //	 _envTex->getImage()->GetCreateInfoPtr()->mipLevels,
        //	 0,
        //	 _envTex->getImage()->GetCreateInfoPtr()->arrayLayers}};
        // cmdBuffer->PipelineBarrier(
        //	Shit::PipelineBarrierInfo{
        //		Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
        //		Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
        //		{},
        //		0,
        //		0,
        //		0,
        //		0,
        //		1,
        //		&barrier});
    });
}
void Environment::prepareRendering() {
    auto sampler = g_App->getSampler("linear");
    Shit::DescriptorImageInfo imageInfo{sampler, _envTex->getImageView(), Shit::ImageLayout::GENERAL};

    // update skybox descriptorsets
    std::vector<Shit::WriteDescriptorSet> writes{
        {_skyboxPipelineInfo.descriptorSets[0], 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imageInfo},
    };
    g_App->getDevice()->UpdateDescriptorSets(writes);
}
void Environment::prepare() {
    createEqTex();
    generateSkyBox();
    generateReflectionTex();
    prepareRendering();
}
void Environment::setSkyBoxEquirectangular(Image const *pEqImage) {
    _envEquirectangularImage = pEqImage;
    g_App->getTextureManager()->removeTextureById(_envEquirectangularTex->getId());
    createEqTex();
    generateSkyBox();
    generateReflectionTex();
    // prepareRendering();
}
void Environment::draw(Shit::CommandBuffer *cmdBuffer, Shit::DescriptorSet *cameraDescriptorSet) {
    cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, _skyboxPipelineInfo.pipeline});

    // Shit::Viewport viewport{0, 0,
    //						framebuffer->GetCreateInfoPtr()->extent.width,
    //						framebuffer->GetCreateInfoPtr()->extent.height,
    //						0, 1};
    // Shit::Rect2D scissor{{}, framebuffer->GetCreateInfoPtr()->extent};
    // cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
    // cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});

    // bind camera
    cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
        Shit::PipelineBindPoint::GRAPHICS, _skyboxPipelineInfo.pipelineLayout, 1, 1, &cameraDescriptorSet});

    // bind skybox tex
    cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS,
                                                               _skyboxPipelineInfo.pipelineLayout, 0, 1,
                                                               &_skyboxPipelineInfo.descriptorSets[0]});
    cmdBuffer->Draw(Shit::DrawIndirectCommand{36, 1, 0, 0});
}