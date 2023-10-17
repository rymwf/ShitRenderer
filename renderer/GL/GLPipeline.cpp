/**
 * @file GLPipeline.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "GLPipeline.hpp"

#include "GLBuffer.hpp"
#include "GLState.hpp"

namespace Shit {
GLPipelineLayout::GLPipelineLayout(GLStateManager *pStateManager, const PipelineLayoutCreateInfo &createInfo)
    : PipelineLayout(createInfo), mpStateManager(pStateManager) {
    mPushConstantBuffers.resize(mCreateInfo.pushConstantRangeCount);
    glGenBuffers((GLsizei)mPushConstantBuffers.size(), mPushConstantBuffers.data());
    for (uint32_t i = 0; i < mCreateInfo.pushConstantRangeCount; ++i) {
        mpStateManager->PushBuffer(GL_UNIFORM_BUFFER, mPushConstantBuffers[i]);
        glBufferStorage(GL_UNIFORM_BUFFER, mCreateInfo.pPushConstantRanges[i].size, nullptr,
                        GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);
        mpStateManager->PopBuffer();
    }
}
GLPipelineLayout::~GLPipelineLayout() {
    glDeleteBuffers((GLsizei)mPushConstantBuffers.size(), mPushConstantBuffers.data());
}
void GLPipelineLayout::PushConstant(ShaderStageFlagBits stageFlags, uint32_t offset, uint32_t size,
                                    void const *pValues) const {
    for (uint32_t i = 0; i < mCreateInfo.pushConstantRangeCount; ++i) {
        auto &&e = mCreateInfo.pPushConstantRanges[i];
        if (bool(stageFlags & e.stageFlags)) {
            mpStateManager->BindBuffer(GL_UNIFORM_BUFFER, mPushConstantBuffers[i]);
            mpStateManager->BindBufferRange(GL_UNIFORM_BUFFER, e.binding, mPushConstantBuffers[i], 0, e.size);
            void *data = glMapBufferRange(GL_UNIFORM_BUFFER, offset, size,
                                          GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);

            memcpy(data, pValues, size);
            glUnmapBuffer(GL_UNIFORM_BUFFER);
            break;
        }
    }
}

GLVAO::GLVAO(GLGraphicsPipeline *pipeline) : mPipeline(pipeline) {
    glGenVertexArrays(1, &mHandle);
    if (mHandle == 0) ST_THROW("failed to create vertex array object")
    Bind();

    // gl>=4.3
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> bindingTable;
    for (uint32_t i = 0; i < mPipeline->GetCreateInfoPtr()->vertexInputState.vertexBindingDescriptionCount; ++i) {
        auto &&e = mPipeline->GetCreateInfoPtr()->vertexInputState.vertexBindingDescriptions[i];
        bindingTable[e.binding] = {e.stride, e.divisor};
    }
    for (uint32_t i = 0; i < mPipeline->GetCreateInfoPtr()->vertexInputState.vertexAttributeDescriptionCount; ++i) {
        auto &&attrib = mPipeline->GetCreateInfoPtr()->vertexInputState.vertexAttributeDescriptions[i];
        glVertexAttribFormat(attrib.location, GetFormatComponentNum(attrib.format),
                             Map(GetFormatDataType(attrib.format)), GetFormatNormalized(attrib.format), attrib.offset);
        glVertexAttribBinding(attrib.location, attrib.binding);
        glVertexBindingDivisor(attrib.binding, bindingTable[attrib.binding].second);
        glEnableVertexAttribArray(attrib.location);
    }
}
GLVAO::~GLVAO() { glDeleteVertexArrays(1, &mHandle); }
void GLVAO::BindVertexBuffer(GLuint first, GLuint buffer, GLintptr offset, GLsizei stride) {
    if (mVertexAttributes[first].buffer != buffer || mVertexAttributes[first].offset != offset ||
        mVertexAttributes[first].stride != stride) {
        glBindVertexBuffer(first, buffer, offset, stride);
        mVertexAttributes[first] = VertexAttribute{buffer, offset, stride};
    }
}
void GLVAO::BindVertexBuffers(GLuint first, GLsizei count, std::span<const GLuint> buffers,
                              std::span<const GLintptr> offsets, std::span<const GLsizei> strides) {
    for (GLsizei i = 0; i < count; ++i) {
        BindVertexBuffer(first + i, buffers[i], offsets[i], strides[i]);
    }
}
void GLVAO::BindIndexBuffer(GLuint buffer, GLenum type, uint64_t offset) {
    if (mIndexBufferState.buffer != buffer) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
        mIndexBufferState.buffer = buffer;
    }
    mIndexBufferState.type = type;
    mIndexBufferState.offset = offset;
}
void GLVAO::Bind() { glBindVertexArray(mHandle); }
GLenum GLVAO::GetIndexType() const { return mIndexBufferState.type; }
//=====================================
GLPipeline::GLPipeline(GLStateManager *pStateManager) : mpStateManager(pStateManager) {}
void GLPipeline::Destroy() {
    mpStateManager->NotifyReleasedPipeline(this);
    for (auto &&e : mProgramsInfo) {
        glDeleteProgram(e.program);
        glDeleteShader(e.shader);
    }
    glDeleteProgramPipelines(1, &mHandle);
}
void GLPipeline::Bind() const { glBindProgramPipeline(mHandle); }
GLuint GLPipeline::CreateProgram(const GLuint shader, bool separable, bool retrievable) {
    return CreateProgram({&shader, 1}, separable, retrievable);
}
GLuint GLPipeline::CreateProgram(std::span<const GLuint> shaders, bool separable, bool retrievable) {
    GLuint ret = glCreateProgram();
    if (ret) {
        glProgramParameteri(ret, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, retrievable);
        glProgramParameteri(ret, GL_PROGRAM_SEPARABLE, separable);
        for (auto shader : shaders) glAttachShader(ret, shader);
        glLinkProgram(ret);
#ifndef NDEBUG
        GLint success;
        glGetProgramiv(ret, GL_LINK_STATUS, &success);
        if (!success) {
            GLint len;
            glGetProgramiv(ret, GL_INFO_LOG_LENGTH, &len);
            std::string aa;
            aa.resize(static_cast<size_t>(len));
            glGetProgramInfoLog(ret, len, NULL, &aa[0]);
            ST_LOG(aa);
            glDeleteProgram(ret);
            ST_THROW("failed to link program");
        }
#endif
    } else
        ST_THROW("failed to create program");
    return ret;
}
GLuint GLPipeline::CreateShader(const PipelineShaderStageCreateInfo &shaderStageCreateInfo) {
    auto shader = glCreateShader(Map(shaderStageCreateInfo.stage));
    if (shader) {
        auto shaderCreateInfoPtr = shaderStageCreateInfo.pShader->GetCreateInfoPtr();
        if (static_cast<bool>((shaderCreateInfoPtr->shadingLanguage & ShadingLanguage::TypeBitmask) ==
                              ShadingLanguage::GLSL)) {
            auto size = static_cast<int>(shaderCreateInfoPtr->size);
            glShaderSource(shader, 1, reinterpret_cast<GLchar const *const *>(&shaderCreateInfoPtr->code), &size);
            glCompileShader(shader);
        } else if (static_cast<bool>((shaderCreateInfoPtr->shadingLanguage & ShadingLanguage::TypeBitmask) ==
                                     ShadingLanguage::SPIRV)) {
            glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, shaderCreateInfoPtr->code,
                           static_cast<GLsizei>(shaderCreateInfoPtr->size));
#ifndef NDBUG
            GLStateManager::CheckError();
#endif
            // equal to compilation
            glSpecializeShaderARB(shader, shaderStageCreateInfo.entryName,
                                  static_cast<GLuint>(shaderStageCreateInfo.specializationInfo.constantValueCount),
                                  shaderStageCreateInfo.specializationInfo.constantIds,
                                  shaderStageCreateInfo.specializationInfo.constantValues);
        } else {
            ST_THROW("unknown shading language", int(shaderCreateInfoPtr->shadingLanguage));
        }
    } else
        ST_THROW("failed to create shader");

#ifndef NDEBUG
    // Specialization is equivalent to compilation.
    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::string infoLog;
        infoLog.resize(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog.data());

        ST_LOG(infoLog);
        // Use the infoLog as you see fit.
        ST_THROW("failed to compile shader")
    }
#endif
    return shader;
}
GLuint GLPipeline::GetProgram(ShaderStageFlagBits stageFlag) const {
    for (auto &&e : mProgramsInfo) {
        if (e.stage == stageFlag) return e.program;
    }
    return mProgramsInfo[0].program;
}
GLint GLPipeline::GetResourceLocation(ShaderStageFlagBits stage, GLenum programInterface, const char *name) const {
    return glGetProgramResourceLocation(GetProgram(stage), programInterface, name);
}
void GLPipeline::InitializePipeline() {
    //// create push constant buffers
    // for (uint32_t i = 0; i <
    // mCreateInfo.pLayout->GetCreateInfoPtr()->pushConstantRangeCount; ++i)
    //{
    //	auto &&pcRange =
    // mCreateInfo.pLayout->GetCreateInfoPtr()->pPushConstantRanges[i]; 	for (auto
    //&&e : mProgramsInfo)
    //	{
    //		if (bool(e.stage & pcRange.stageFlags))
    //		{
    //			e.pushConstantBufferBinding =
    // glGetProgramResourceLocation(e.program, GL_UNIFORM, pcRange.name);
    //		}
    //	};
    // }

    glGenProgramPipelines(1, &mHandle);
    Bind();
    for (auto &&e : mProgramsInfo) {
        glUseProgramStages(mHandle, MapShaderStageFlags(e.stage), e.program);
    }
}
//=====================================
void GLGraphicsPipeline::Initialize() {
    mVAO = std::make_unique<GLVAO>(this);

    mProgramsInfo.resize(mCreateInfo.stageCount);
    for (uint32_t i = 0; i < mCreateInfo.stageCount; ++i) {
        auto &&e = mCreateInfo.stages[i];
        mProgramsInfo[i].stage = e.stage;
        mProgramsInfo[i].shader = CreateShader(e);
        mProgramsInfo[i].program = CreateProgram(mProgramsInfo[i].shader, true, false);
    }
    //========
    InitializePipeline();

    /*
    //reflection
    GLint activeResouces;
    glGetProgramInterfaceiv(mProgram, GL_UNIFORM, GL_ACTIVE_RESOURCES,
    &activeResouces); ST_LOG("GL_ACTIVE_RESOURCES"); ST_LOG_VAR(activeResouces);
    GLint a;
    glGetProgramInterfaceiv(mProgram, GL_UNIFORM, GL_MAX_NAME_LENGTH, &a);
    ST_LOG("GL_MAX_NAME_LENGTH");
    ST_LOG_VAR(a);
    glGetProgramInterfaceiv(mProgram, GL_UNIFORM, GL_MAX_NUM_ACTIVE_VARIABLES,
    &a); ST_LOG("GL_MAX_NUM_ACTIVE_VARIABLES"); ST_LOG_VAR(a);

    std::vector<GLenum> props{
            GL_TYPE,
            GL_ARRAY_SIZE,
            GL_OFFSET,
            GL_BLOCK_INDEX,
            GL_ARRAY_STRIDE,
            GL_MATRIX_STRIDE,
            GL_IS_ROW_MAJOR,
            GL_ATOMIC_COUNTER_BUFFER_INDEX,
            GL_REFERENCED_BY_VERTEX_SHADER,
            GL_REFERENCED_BY_TESS_CONTROL_SHADER,
            GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
            GL_REFERENCED_BY_GEOMETRY_SHADER,
            GL_REFERENCED_BY_FRAGMENT_SHADER,
            GL_REFERENCED_BY_COMPUTE_SHADER,
            GL_LOCATION,
    };
    std::vector<GLint> params;
    GLsizei length;
    GLsizei nameLength{};
    for (GLint i = 0; i < activeResouces; ++i)
    {
            ST_LOG("=========================================");
            char name[64];
            glGetProgramResourceName(mProgram, GL_UNIFORM, i, 64, &length, name);
            ST_LOG_VAR(nameLength);
            ST_LOG_VAR(name);
            params.clear();
            params.resize(props.size());
            glGetProgramResourceiv(mProgram, GL_UNIFORM, i,
    static_cast<GLsizei>(props.size()), props.data(),
    static_cast<GLsizei>(props.size()), &length, params.data());
            ST_LOG_VAR(length);
            for (auto param : params)
            {
                    ST_LOG_VAR(param);
            }
    }
  */
    //=========
}
GLGraphicsPipeline::GLGraphicsPipeline(GLStateManager *pStateManager, const GraphicsPipelineCreateInfo &createInfo)
    : GraphicsPipeline(createInfo), GLPipeline(pStateManager) {
    Initialize();
}
void GLGraphicsPipeline::Bind() const {
    GLPipeline::Bind();

    mVAO->Bind();
}
// gl 4.3
void GLGraphicsPipeline::BindVertexBuffer(GLuint first, GLuint buffer, GLintptr offset, GLsizei stride) {
    mVAO->BindVertexBuffer(first, buffer, offset, stride);
}
// gl 4.4
void GLGraphicsPipeline::BindVertexBuffers(GLuint first, GLsizei count, std::span<const GLuint> buffers,
                                           std::span<const GLintptr> offsets, std::span<const GLsizei> strides) {
    mVAO->BindVertexBuffers(first, count, buffers, offsets, strides);
}
void GLGraphicsPipeline::BindVertexBuffers(BindVertexBuffersInfo const &info) {
    std::vector<GLuint> vertexBuffers;
    std::vector<GLintptr> offsets;
    // std::vector<GLintptr> offsets(info.pOffsets, info.pOffsets +
    // info.bindingCount);
    std::vector<GLsizei> strides;
    auto bindingCount = (std::min)(info.bindingCount, mCreateInfo.vertexInputState.vertexBindingDescriptionCount);

    vertexBuffers.reserve(bindingCount);
    offsets.reserve(bindingCount);
    strides.reserve(bindingCount);

    std::ranges::transform(info.ppBuffers, info.ppBuffers + info.bindingCount, std::back_inserter(vertexBuffers),
                           [](auto p) { return p ? static_cast<GLBuffer const *>(p)->GetHandle() : GL_NONE; });
    std::ranges::transform(info.pOffsets, info.pOffsets + info.bindingCount, std::back_inserter(offsets),
                           [](auto p) { return static_cast<GLintptr>(p); });

    for (uint32_t i = 0; i < bindingCount; ++i) {
        auto &&bindingDesc = mCreateInfo.vertexInputState.vertexBindingDescriptions[i];
        strides.emplace_back(bindingDesc.stride);
    }

    uint32_t l = 0, r = 1;
    for (; r < bindingCount; ++r) {
        // isNull_r = vertexBuffers[r] != GL_NONE;
        if (bool(vertexBuffers[r]) != bool(vertexBuffers[r - 1])) {
            // if (bool(vertexBuffers[l]))
            //{
            BindVertexBuffers(l, r - l, std::span{&vertexBuffers[l], r - l}, std::span{&offsets[l], r - l},
                              std::span{&strides[l], r - l});
            //}
            // else
            //{
            //	BindVertexBuffers(l, r - l, {}, {}, {});
            //}
            l = r;
        }
    }
    if (r != l) {
        // if (bool(vertexBuffers[l]))
        //{
        BindVertexBuffers(l, r - l, std::span{&vertexBuffers[l], r - l}, std::span{&offsets[l], r - l},
                          std::span{&strides[l], r - l});
        //}
        // else
        //{
        //	BindVertexBuffers(l, r - l, {}, {}, {});
        //}
    }
}
void GLGraphicsPipeline::BindIndexBuffer(GLuint buffer, GLenum type, uint64_t offset) {
    mVAO->BindIndexBuffer(buffer, type, offset);
}
void GLGraphicsPipeline::BindIndexBuffer(BindIndexBufferInfo const &info) {
    BindIndexBuffer(static_cast<GLBuffer const *>(info.pBuffer)->GetHandle(), Map(info.indexType), info.offset);
}
GLenum GLGraphicsPipeline::GetIndexType() const { return mVAO->GetIndexType(); }
//======================================

GLComputePipeline::GLComputePipeline(GLStateManager *pStateManager, const ComputePipelineCreateInfo &createInfo)
    : ComputePipeline(createInfo), GLPipeline(pStateManager) {
    Initialize();
}
void GLComputePipeline::Initialize() {
    mProgramsInfo.resize(1);
    mProgramsInfo[0].stage = mCreateInfo.stage.stage;
    mProgramsInfo[0].shader = CreateShader(mCreateInfo.stage);
    mProgramsInfo[0].program = CreateProgram(mProgramsInfo[0].shader, true, false);

    InitializePipeline();
}
}  // namespace Shit
