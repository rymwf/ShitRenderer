#pragma once
#include "glm/glm.hpp"
#include "image.h"
#include "prerequisites.h"
#include "shader.h"
#include "texture.h"

class Environment {
public:
    struct PipelineInfo {
        std::vector<Shader *> shaders;
        std::vector<Shit::DescriptorSet *> descriptorSets;
        Shit::PipelineLayout *pipelineLayout;
        Shit::Pipeline *pipeline;
        std::vector<Shit::PipelineShaderStageCreateInfo> getStageCIs() {
            std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
            for (auto p : shaders) {
                p->load();
                shaderStageCIs.emplace_back(p->getStage(), p->getHandle(), p->getEntryName().data());
            }
            return shaderStageCIs;
        }
    };

    Environment(Image const *pEqImage);
    ~Environment();

    void prepare();

    void setSkyBoxEquirectangular(Image const *pEqImage);

    void draw(Shit::CommandBuffer *cmdBuffer, Shit::DescriptorSet *cameraDescriptorSet);

    constexpr Shit::RenderPass *getRenderPass() const { return _renderPass; }

    constexpr int getRoughness1Level() const { return _roughness1Level; }

    constexpr Texture *getEnvTex() const { return _envTex; }

private:
    static PipelineInfo _eq2cubePipelineInfo;
    static PipelineInfo _skyboxPipelineInfo;
    static PipelineInfo _prefilteredEnvPipelineInfo;

    static Shit::RenderPass *_renderPass;  // compatible renderpass

    std::vector<Shit::ImageView *> _renderImageViews;
    std::vector<Shit::Framebuffer *> _framebuffers;

    Shit::ClearValue _clearValue{Shit::ClearColorValueFloat{}};

    // std::vector<Shit::DescriptorSet *> descriptorSets;
    Image const *_envEquirectangularImage{};
    Texture *_envEquirectangularTex{};

    int _roughness1Level{7};

    Texture *_envTex{};  // cubemap
    // Texture *_envIrradianceTex{}; // cubemap

    // void destroyRes();
    void createEqTex();
    void generateSkyBox();
    void generateReflectionTex();
    void prepareRendering();

    void createRenderPass(Shit::Format format);
    void createEq2CubePipeline();
    void createSkyboxPipeline();
    void createPrefilteredEnvPipelineInfo();
};