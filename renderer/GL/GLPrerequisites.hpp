/**
 * @file GLPrerequisites.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once

#include <GL/glew.h>

#include <renderer/ShitRendererPrerequisites.hpp>

#define glIsExtensionSupported(x) glewIsExtensionSupported(x)

// #define CLIP_ORIGIN_UPPER_LEFT
#define CLIP_ORIGIN_LOWER_LEFT

#define BUFFER_TARGET_NUM 15

#define TEXTURE_TARGET_NUM 11

#define MAX_DRAW_BUFFERS 8

#define MAX_VIEWPORTS 16
#define MAX_VERTEX_ATTRIB_BINDINGS 16

// min value of vertex shader limist
#define MAX_VERTEX_ATTRIBS 16
#define MAX_VERTEX_UNIFORM_VECTORS 256
#define MAX_VERTEX_UNIFORM_BLOCKS 14
#define MAX_VERTEX_TEXTURE_IMAGE_UNITS 16

// min value of tesselation shader limist

// min value of geometry shader limist
#define MAX_GEOMETRY_UNIFORM_BLOCKS 14
#define MAX_GEOMETRY_TEXTURE_IMAGE_UNITS 16
#define MAX_GEOMETRY_SHADER_INVOCATIONS 32
#define MAX_VERTEX_STREAMS 4

// min value of fragment shader limist
#define MAX_FRAGMENT_UNIFORM_BLOCKS 14
#define MAX_TEXTURE_IMAGE_UNITS 16
#define MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS 1
#define MAX_FRAGMENT_ATOMIC_COUNTERS 8
#define MAX_FRAGMENT_SHADER_STORAGE_BLOCKS 8

// min value of compute shader limist
#define MAX_COMPUTE_UNIFORM_BLOCKS 14
#define MAX_COMPUTE_TEXTURE_IMAGE_UNITS 16
#define MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS 8
#define MAX_COMPUTE_ATOMIC_COUNTERS 8
#define MAX_COMPUTE_IMAGE_UNIFORMS 8
#define MAX_COMPUTE_SHADER_STORAGE_BLOCKS 8

// min value of aggregate shader limits
#define MAX_UNIFORM_BUFFER_BINDING 84
#define MAX_COMBINED_UNIFORM_BLOCKS 70
#define MAX_COMBINED_TEXTURE_IMAGE_UNITS 80
#define MAX_ATOMIC_COUNTER_BUFFER_BINDINGS 1
#define MAX_COMBINED_ATOMIC_COUNTER_BUFFERS 1
#define MAX_COMBINED_ATOMIC_COUNTERS 8
#define MAX_SHADER_STORAGE_BUFFER_BINDINGS 8
#define MAX_COMBINED_SHADER_STORAGE_BLOCKS 8
#define MAX_IMAGE_UNITS 8
#define MAX_COMBINED_SHADER_OUTPUT_RESOURCES 8
#define MAX_FRAGMENT_IMAGE_UNIFORMS 8
#define MAX_COMBINED_IMAGE_UNIFORMS 8

#define MAX_VERTX_INPUT_BINDINGS 16
#define MAX_VERTX_INPUT_ATTRIBUTES 16

//
#define MAX_TRANSFORM_FEEDBACK_BUFFERS 4

#define MAX_COLOR_ATTACHMENTS 8

// self defined var
#define MAX_UNIFORM_BLOCKS 32
#define MAX_IMAGE_UNIFORMS 8

namespace Shit {
enum class GLImageFlag {
    TEXTURE,
    RENDERBUFFER,
    FRONT_LEFT_FRAMEBUFFER,
    BACK_LEFT_FRAMEBUFFER,
    FRONT_RIGHT_FRAMEBUFFER,
    BACK_RIGHT_FRAMEBUFFER,
    DEFAULT_DEPTH_STENCIL,
};

class GLDevice;
class GLFramebuffer;
class GLRenderPass;
class GLBuffer;
class GLImage;
class GLImageView;
class GLPipeline;
class GLStateManager;
class GLRenderSystem;
class GLSurface;

struct ShitGLVersion {
    int major;
    int minor;
};

extern ShitGLVersion g_GLVersion;

extern bool SHIT_GL_110;
extern bool SHIT_GL_120;
extern bool SHIT_GL_121;
extern bool SHIT_GL_130;
extern bool SHIT_GL_140;
extern bool SHIT_GL_150;
extern bool SHIT_GL_200;
extern bool SHIT_GL_210;
extern bool SHIT_GL_300;
extern bool SHIT_GL_310;
extern bool SHIT_GL_320;
extern bool SHIT_GL_330;
extern bool SHIT_GL_400;
extern bool SHIT_GL_410;
extern bool SHIT_GL_420;
extern bool SHIT_GL_430;
extern bool SHIT_GL_440;
extern bool SHIT_GL_450;
extern bool SHIT_GL_460;

extern GLRenderSystem *g_RenderSystem;

namespace GL {
void queryGLExtensionNames(std::vector<const GLubyte *> &extensionNames);

void querySupportedShaderBinaryFormat(std::vector<GLint> &shaderBinaryFormats);
void querySupportedProgramBinaryFormat(std::vector<GLint> &programBinaryFormats);
bool isSupportShaderBinaryFormat(GLenum format);
void listGLInfo();
}  // namespace GL

GLenum Map(BlendOp op);
GLenum Map(BlendFactor factor);
GLenum Map(LogicOp op);
GLenum Map(StencilOp op);
GLenum Map(FrontFace face);
GLenum Map(CullMode mode);
GLenum Map(PolygonMode mode);
GLenum Map(PrimitiveTopology topology);
GLenum Map(ComponentSwizzle swizzle);
GLenum MapInternalFormat(Format format);
GLenum MapBaseFormat(Format format);
GLenum MapPixelFormat(Format format);
GLenum Map(ShaderStageFlagBits flag);
GLbitfield MapShaderStageFlags(ShaderStageFlagBits flags);
GLenum Map(BufferUsageFlagBits flag);
GLenum Map(BufferMutableStorageUsage usage);
GLbitfield Map(BufferMapFlagBits flag);
GLbitfield Map(BufferStorageFlagBits flag);
GLenum Map(SamplerWrapMode wrapMode);
GLenum Map(CompareOp op);
GLenum Map(Filter filter);
GLenum Map(ImageType imageType, SampleCountFlagBits sampleCountFlag);
GLenum Map(ImageViewType viewType, SampleCountFlagBits sampleCountFlag);
GLenum Map(DataType dataType);
GLenum Map(IndexType type);
std::variant<std::array<float, 4>, std::array<int, 4>> Map(BorderColor color);

GLenum GetFBOBindPoint(GLImageFlag flag);
}  // namespace Shit
