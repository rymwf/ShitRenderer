/**
 * @file GLPipeline.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitPipeline.hpp>

#include "GLPrerequisites.hpp"
#include "GLShader.hpp"

namespace Shit {
class GLGraphicsPipeline;

class GLVAO {
public:
    struct VertexAttribute {
        GLuint buffer;
        GLintptr offset;
        GLsizei stride;
    };
    struct IndexBufferState {
        GLuint buffer{~0U};
        GLenum type;
        uint64_t offset;
    };
    GLVAO(GLGraphicsPipeline *pipeline);
    ~GLVAO();
    // gl 4.3
    void BindVertexBuffer(GLuint first, GLuint buffer, GLintptr offset, GLsizei stride);
    // gl 4.4
    void BindVertexBuffers(GLuint first, GLsizei count, std::span<const GLuint> buffers,
                           std::span<const GLintptr> offsets, std::span<const GLsizei> strides);
    void BindIndexBuffer(GLuint buffer, GLenum type, uint64_t offset);
    void Bind();

    GLenum GetIndexType() const;

private:
    GLGraphicsPipeline *mPipeline;
    GLuint mHandle;

    // VertexAttributeDescription mVertexAttribute
    std::array<VertexAttribute, MAX_VERTX_INPUT_ATTRIBUTES> mVertexAttributes;
    IndexBufferState mIndexBufferState;
};
class GLPipelineLayout final : public PipelineLayout {
    GLStateManager *mpStateManager;
    std::vector<GLuint> mPushConstantBuffers;

public:
    GLPipelineLayout(GLStateManager *pStateManager, const PipelineLayoutCreateInfo &createInfo);
    ~GLPipelineLayout();
    void PushConstant(ShaderStageFlagBits stageFlags, uint32_t offset, uint32_t size, void const *pValues) const;
};

class GLPipeline : public virtual Pipeline {
protected:
    GLuint mHandle;

    struct ProgrameInfo {
        GLuint program;
        GLuint shader;
        ShaderStageFlagBits stage;
        int32_t pushConstantBufferBinding{-1};
    };

    std::vector<ProgrameInfo> mProgramsInfo;

    GLStateManager *mpStateManager;

    void Destroy() override;

    GLuint GetProgram(ShaderStageFlagBits stageFlag) const;

    void InitializePipeline();

public:
    GLPipeline(GLStateManager *pStateManager);

    ~GLPipeline() override {}
    static GLuint CreateProgram(const GLuint shader, bool separable, bool retrievable);
    static GLuint CreateProgram(std::span<const GLuint> shaders, bool separable, bool retrievable);

    static GLuint CreateShader(const PipelineShaderStageCreateInfo &shaderStageCreateInfo);

    constexpr GLuint GetHandle() const { return mHandle; }
    GLint GetResourceLocation(ShaderStageFlagBits stage, GLenum programInterface, const char *name) const;

    virtual void Bind() const;
};

class GLGraphicsPipeline final : public GraphicsPipeline, public GLPipeline {
    std::unique_ptr<GLVAO> mVAO;

    void Destroy() override { GLPipeline::Destroy(); }

public:
    GLGraphicsPipeline(GLStateManager *stateManager, const GraphicsPipelineCreateInfo &createInfo);
    ~GLGraphicsPipeline() override { Destroy(); }
    void Initialize() override;

    PipelineLayout const *GetPipelineLayout() const override { return mCreateInfo.pLayout; }
    PipelineBindPoint GetBindPoint() const override { return PipelineBindPoint::GRAPHICS; }

    void Bind() const override;

    // gl 4.3
    void BindVertexBuffer(GLuint first, GLuint buffer, GLintptr offset, GLsizei stride);
    // gl 4.4
    void BindVertexBuffers(GLuint first, GLsizei count, std::span<const GLuint> buffers,
                           std::span<const GLintptr> offsets, std::span<const GLsizei> strides);
    void BindVertexBuffers(BindVertexBuffersInfo const &info);

    void BindIndexBuffer(GLuint buffer, GLenum type, uint64_t offset);
    void BindIndexBuffer(BindIndexBufferInfo const &info);

    GLenum GetIndexType() const;
};

class GLComputePipeline final : public ComputePipeline, public GLPipeline {
    void Destroy() override { GLPipeline::Destroy(); }

public:
    GLComputePipeline(GLStateManager *stateManager, const ComputePipelineCreateInfo &createInfo);
    void Initialize() override;
    PipelineLayout const *GetPipelineLayout() const override { return mCreateInfo.pLayout; }
    PipelineBindPoint GetBindPoint() const override { return PipelineBindPoint::COMPUTE; }
};
}  // namespace Shit
