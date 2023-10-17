/**
 * @file GLPrerequisites.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "GLPrerequisites.hpp"

namespace Shit {
ShitGLVersion g_GLVersion;

/**
 * @brief if current gpu do not support a version, the value will be false,
 * they will be initialized after creating a device;
 */
bool SHIT_GL_110;
bool SHIT_GL_120;
bool SHIT_GL_121;
bool SHIT_GL_130;
bool SHIT_GL_140;
bool SHIT_GL_150;
bool SHIT_GL_200;
bool SHIT_GL_210;
bool SHIT_GL_300;
bool SHIT_GL_310;
bool SHIT_GL_320;
bool SHIT_GL_330;
bool SHIT_GL_400;
bool SHIT_GL_410;
bool SHIT_GL_420;
bool SHIT_GL_430;
bool SHIT_GL_440;
bool SHIT_GL_450;
bool SHIT_GL_460;

namespace GL {
void queryGLExtensionNames(std::vector<const GLubyte *> &extensionNames) {
    int extensionCount{};
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
    extensionNames.resize(extensionCount);
    ST_LOG_VAR(extensionCount);
    for (int i = 0; i < extensionCount; ++i) {
        extensionNames[i] = glGetStringi(GL_EXTENSIONS, i);
        ST_LOG_VAR(extensionNames[i]);
    }
}

void querySupportedShaderBinaryFormat(std::vector<GLint> &shaderBinaryFormats) {
    GLint count;
    glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &count);
    ST_LOG("supported shader binary formats num:", count);
    if (count) {
        shaderBinaryFormats.resize(count);
        glGetIntegerv(GL_SHADER_BINARY_FORMATS, shaderBinaryFormats.data());
    }
#ifndef NDEBUG
    ST_LOG("supported shader binary formats");
    for (auto &&e : shaderBinaryFormats) ST_LOG(e);
#endif
}

void querySupportedProgramBinaryFormat(std::vector<GLint> &programBinaryFormats) {
    GLint count;
    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &count);
    ST_LOG("supported program binary formats num:", count);
    if (count) {
        programBinaryFormats.resize(count);
        glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, programBinaryFormats.data());
    }
#ifndef NDEBUG
    ST_LOG("supported program binary formats");
    for (auto &&e : programBinaryFormats) ST_LOG(e);
#endif
}

bool isSupportShaderBinaryFormat(GLenum format) {
    std::vector<GLint> shaderBinaryFormats;
    querySupportedShaderBinaryFormat(shaderBinaryFormats);
    for (auto &shaderBinaryFormat : shaderBinaryFormats) {
        ST_LOG_VAR(shaderBinaryFormat);
        if (static_cast<GLenum>(shaderBinaryFormat) == format) return true;
    }
    return false;
}

void listGLInfo() {
    // check opengl informations
    ST_LOG_VAR(glGetString(GL_VENDOR));
    ST_LOG_VAR(glGetString(GL_RENDERER));
    ST_LOG_VAR(glGetString(GL_VERSION));
    ST_LOG_VAR(glGetString(GL_SHADING_LANGUAGE_VERSION));
    int numShadingLanguageVersions{};
    glGetIntegerv(GL_NUM_SHADING_LANGUAGE_VERSIONS, &numShadingLanguageVersions);
    ST_LOG_VAR(numShadingLanguageVersions);
    for (int i = 0; i < numShadingLanguageVersions; ++i) ST_LOG_VAR(glGetStringi(GL_SHADING_LANGUAGE_VERSION, i));

    int samples;
    glGetIntegerv(GL_SAMPLES, &samples);
    ST_LOG_VAR(samples);

    int maxFramebufferWidth{}, maxFramebufferHeight{};
    glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &maxFramebufferWidth);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &maxFramebufferHeight);
    ST_LOG_VAR(maxFramebufferWidth);
    ST_LOG_VAR(maxFramebufferHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (g_GLVersion.major * 10 + g_GLVersion.minor >= 43) {
        // framebuffer parameter
        GLenum list0[]{//		GL_FRAMEBUFFER_DEFAULT_WIDTH,
                       //		GL_FRAMEBUFFER_DEFAULT_HEIGHT,
                       //		GL_FRAMEBUFFER_DEFAULT_LAYERS,
                       //		GL_FRAMEBUFFER_DEFAULT_SAMPLES,
                       //		GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS,

                       GL_DOUBLEBUFFER,
                       GL_IMPLEMENTATION_COLOR_READ_FORMAT,
                       GL_IMPLEMENTATION_COLOR_READ_TYPE,
                       GL_SAMPLES,
                       GL_SAMPLE_BUFFERS,
                       GL_STEREO};
        const char *list1[]{"GL_DOUBLEBUFFER",
                            "GL_IMPLEMENTATION_COLOR_READ_FORMAT",
                            "GL_IMPLEMENTATION_COLOR_READ_TYPE",
                            "GL_SAMPLES",
                            "GL_SAMPLE_BUFFERS",
                            "GL_STEREO"};

        // default framebuffer
        for (int i = 0, len = sizeof list0 / sizeof list0[0]; i < len; ++i) {
            GLint a{};
            glGetFramebufferParameteriv(GL_FRAMEBUFFER, list0[i], &a);
            ST_LOG(list1[i], a);
        }
    }

    GLenum list2[]{GL_FRONT_LEFT, GL_FRONT_RIGHT, GL_BACK_LEFT, GL_BACK_RIGHT, GL_DEPTH, GL_STENCIL};
    const char *list3[]{"GL_FRONT_LEFT", "GL_FRONT_RIGHT", "GL_BACK_LEFT", "GL_BACK_RIGHT", "GL_DEPTH", "GL_STENCIL"};
    GLenum list4[]{GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE,       GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE,
                   GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE,      GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE,
                   GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,     GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
                   GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING};
    const char *list5[]{"GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE",       "GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE",
                        "GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE",      "GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE",
                        "GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE",     "GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE",
                        "GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE", "GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING"};
    for (int i = 0; i < 6; ++i) {
        GLint a{};
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, list2[i], GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &a);
        ST_LOG(list3[i], a);
        if (a != GL_NONE) {
            for (int j = 0, len = sizeof list4 / sizeof list4[0]; j < len; ++j) {
                glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, list2[i], list4[j], &a);
                ST_LOG(list5[j], a);
            }
        }
    }

    // extensions
#if 0
	GLint a;
	glGetIntegerv(GL_NUM_EXTENSIONS, &a);
	for (GLint i = 0; i < a; ++i)
		ST_LOG(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
#endif
    constexpr GLenum glCapabilityArray[] = {
        GL_BLEND,
        // GL_CLIP_DISTANCE,
        GL_COLOR_LOGIC_OP,
        GL_CULL_FACE,
        GL_DEBUG_OUTPUT,
        GL_DEBUG_OUTPUT_SYNCHRONOUS,
        GL_DEPTH_CLAMP,
        GL_DEPTH_TEST,
        GL_DITHER,
        GL_FRAMEBUFFER_SRGB,
        GL_LINE_SMOOTH,
        GL_MULTISAMPLE,
        GL_POLYGON_OFFSET_FILL,
        GL_POLYGON_OFFSET_LINE,
        GL_POLYGON_OFFSET_POINT,
        GL_POLYGON_SMOOTH,
        GL_PRIMITIVE_RESTART,
        GL_PRIMITIVE_RESTART_FIXED_INDEX,
        GL_RASTERIZER_DISCARD,
        GL_SAMPLE_ALPHA_TO_COVERAGE,
        GL_SAMPLE_ALPHA_TO_ONE,
        GL_SAMPLE_COVERAGE,
        GL_SAMPLE_SHADING,
        GL_SAMPLE_MASK,
        GL_SCISSOR_TEST,
        GL_STENCIL_TEST,
        GL_TEXTURE_CUBE_MAP_SEAMLESS,
        GL_PROGRAM_POINT_SIZE,
    };
    const char *glCapabilityEnumNames[]{
        "GL_BLEND",
        "GL_COLOR_LOGIC_OP",
        "GL_CULL_FACE",
        "GL_DEBUG_OUTPUT",
        "GL_DEBUG_OUTPUT_SYNCHRONOUS",
        "GL_DEPTH_CLAMP",
        "GL_DEPTH_TEST",
        "GL_DITHER",
        "GL_FRAMEBUFFER_SRGB",
        "GL_LINE_SMOOTH",
        "GL_MULTISAMPLE",
        "GL_POLYGON_OFFSET_FILL",
        "GL_POLYGON_OFFSET_LINE",
        "GL_POLYGON_OFFSET_POINT",
        "GL_POLYGON_SMOOTH",
        "GL_PRIMITIVE_RESTART",
        "GL_PRIMITIVE_RESTART_FIXED_INDEX",
        "GL_RASTERIZER_DISCARD",
        "GL_SAMPLE_ALPHA_TO_COVERAGE",
        "GL_SAMPLE_ALPHA_TO_ONE",
        "GL_SAMPLE_COVERAGE",
        "GL_SAMPLE_SHADING",
        "GL_SAMPLE_MASK",
        "GL_SCISSOR_TEST",
        "GL_STENCIL_TEST",
        "GL_TEXTURE_CUBE_MAP_SEAMLESS",
        "GL_PROGRAM_POINT_SIZE",
    };

    for (int i = 0, len = sizeof(glCapabilityArray) / sizeof(glCapabilityArray[0]); i < len; ++i) {
        ST_LOG(glCapabilityEnumNames[i], int(glIsEnabled(glCapabilityArray[i])));
    }
}

}  // namespace GL
constexpr GLenum glDataTypeArray[]{
    GL_BYTE,
    GL_UNSIGNED_BYTE,
    GL_SHORT,
    GL_UNSIGNED_SHORT,
    GL_INT,
    GL_UNSIGNED_INT,
    GL_FLOAT,
    GL_HALF_FLOAT,
    GL_DOUBLE,
    // GL_INT_2_10_10_10_REV,
    // GL_UNSIGNED_INT_2_10_10_10_REV,
    // GL_UNSIGNED_INT_10F_11F_11F_REV,
    GL_UNSIGNED_INT_24_8,
};

constexpr GLenum glFormatArray[]{
    GL_NONE,                                       // UNDEFINED,
    GL_NONE,                                       // R4G4_UNORM_PACK8,
    GL_RGBA4,                                      // R4G4B4A4_UNORM_PACK16,// = 2,
    GL_NONE,                                       // B4G4R4A4_UNORM_PACK16, // = 3,
    GL_RGB565,                                     // R5G6B5_UNORM_PACK16,// =4,
    GL_NONE,                                       // B5G6R5_UNORM_PACK16, // = 5,
    GL_RGB5_A1,                                    // R5G5B5A1_UNORM_PACK16,// =6,
    GL_NONE,                                       // B5G5R5A1_UNORM_PACK16, // = 7,
    GL_NONE,                                       // A1R5G5B5_UNORM_PACK16, // = 8,
    GL_R8,                                         // R8_UNORM,							
    GL_R8_SNORM,                                   // R8_SNORM,							
    GL_NONE,                                       // R8_USCALED,
    GL_NONE,                                       // R8_SSCALED,
    GL_R8UI,                                       // R8_UINT,
    GL_R8I,                                        // R8_SINT,
    GL_SR8_EXT,                                    // R8_SRGB,							
    GL_RG8,                                        // R8G8_UNORM,
    GL_RG8_SNORM,                                  // R8G8_SNORM,						
    GL_NONE,                                       // R8G8_USCALED,
    GL_NONE,                                       // R8G8_SSCALED,
    GL_RG8UI,                                      // R8G8_UINT,
    GL_RG8I,                                       // R8G8_SINT,
    GL_SRG8_EXT,                                   // R8G8_SRGB,						
    GL_RGB8,                                       // R8G8B8_UNORM,
    GL_RGB8_SNORM,                                 // R8G8B8_SNORM,						
    GL_NONE,                                       // R8G8B8_USCALED,
    GL_NONE,                                       // R8G8B8_SSCALED,
    GL_RGB8UI,                                     // R8G8B8_UINT,						
    GL_RGB8I,                                      // R8G8B8_SINT,						
    GL_SRGB8,                                      // R8G8B8_SRGB,						
    GL_BGR,                                        // B8G8R8_UNORM,
    GL_NONE,                                       // B8G8R8_SNORM,
    GL_NONE,                                       // B8G8R8_USCALED,
    GL_NONE,                                       // B8G8R8_SSCALED,
    GL_NONE,                                       // B8G8R8_UINT,
    GL_BGR_INTEGER,                                // B8G8R8_SINT,
    GL_NONE,                                       // B8G8R8_SRGB,
    GL_RGBA8,                                      // R8G8B8A8_UNORM,
    GL_RGBA8_SNORM,                                // R8G8B8A8_SNORM,
    GL_NONE,                                       // R8G8B8A8_USCALED,
    GL_NONE,                                       // R8G8B8A8_SSCALED,
    GL_RGBA8UI,                                    // R8G8B8A8_UINT,					
    GL_RGBA8I,                                     // R8G8B8A8_SINT,					
    GL_SRGB8_ALPHA8,                               // R8G8B8A8_SRGB,
    GL_BGRA8_EXT,                                  // B8G8R8A8_UNORM,					
    GL_NONE,                                       // B8G8R8A8_SNORM,
    GL_NONE,                                       // B8G8R8A8_USCALED,
    GL_NONE,                                       // B8G8R8A8_SSCALED,
    GL_NONE,                                       // B8G8R8A8_UINT,
    GL_BGRA_INTEGER,                               // B8G8R8A8_SINT,
    GL_NONE,                                       // B8G8R8A8_SRGB,
    GL_NONE,                                       // A8B8G8R8_UNORM_PACK32, // = 51,
    GL_NONE,                                       // A8B8G8R8_SNORM_PACK32, // = 52,
    GL_NONE,                                       // A8B8G8R8_USCALED_PACK32,					
    GL_NONE,                                       // A8B8G8R8_SSCALED_PACK32,					
    GL_NONE,                                       // A8B8G8R8_UINT_PACK32, // = 55,
    GL_NONE,                                       // A8B8G8R8_SINT_PACK32, // = 56,
    GL_NONE,                                       // A8B8G8R8_SRGB_PACK32, // = 57,
    GL_RGB10_A2,                                   // A2R10G10B10_UNORM_PACK32,					
    GL_NONE,                                       // A2R10G10B10_SNORM_PACK32,					
    GL_NONE,                                       // A2R10G10B10_USCALED_PACK32, // = 60,
    GL_NONE,                                       // A2R10G10B10_SSCALED_PACK32, // = 61,
    GL_RGB10_A2UI,                                 // A2R10G10B10_UINT_PACK32,					
    GL_NONE,                                       // A2R10G10B10_SINT_PACK32,					
    GL_RGB10_A2,                                   // A2B10G10R10_UNORM_PACK32,					
    GL_NONE,                                       // A2B10G10R10_SNORM_PACK32,					
    GL_NONE,                                       // A2B10G10R10_USCALED_PACK32, // = 66,
    GL_NONE,                                       // A2B10G10R10_SSCALED_PACK32, // = 67,
    GL_NONE,                                       // A2B10G10R10_UINT_PACK32,					
    GL_RGB10_A2UI,                                 // A2B10G10R10_SINT_PACK32,					
    GL_R16,                                        // R16_UNORM,
    GL_R16_SNORM,                                  // R16_SNORM,								
    GL_NONE,                                       // R16_USCALED,
    GL_NONE,                                       // R16_SSCALED,
    GL_R16UI,                                      // R16_UINT,
    GL_R16I,                                       // R16_SINT,
    GL_R16F,                                       // R16_SFLOAT,
    GL_RG16,                                       // R16G16_UNORM,
    GL_RG16_SNORM,                                 // R16G16_SNORM,								
    GL_NONE,                                       // R16G16_USCALED,
    GL_NONE,                                       // R16G16_SSCALED,
    GL_RG16UI,                                     // R16G16_UINT,								
    GL_RG16I,                                      // R16G16_SINT,								
    GL_RG16F,                                      // R16G16_SFLOAT,
    GL_RGB16,                                      // R16G16B16_UNORM,							
    GL_RGB16_SNORM,                                // R16G16B16_SNORM,
    GL_NONE,                                       // R16G16B16_USCALED,
    GL_NONE,                                       // R16G16B16_SSCALED,
    GL_RGB16UI,                                    // R16G16B16_UINT,					
    GL_RGB16I,                                     // R16G16B16_SINT,					
    GL_RGB16F,                                     // R16G16B16_SFLOAT,					
    GL_RGBA16,                                     // R16G16B16A16_UNORM,				
    GL_RGBA16_SNORM,                               // R16G16B16A16_SNORM,
    GL_NONE,                                       // R16G16B16A16_USCALED, // = 93,
    GL_NONE,                                       // R16G16B16A16_SSCALED, // = 94,
    GL_RGBA16UI,                                   // R16G16B16A16_UINT,				
    GL_RGBA16I,                                    // R16G16B16A16_SINT,				
    GL_RGBA16F,                                    // R16G16B16A16_SFLOAT,				
    GL_R32UI,                                      // R32_UINT,
    GL_R32I,                                       // R32_SINT,
    GL_R32F,                                       // R32_SFLOAT,
    GL_RG32UI,                                     // R32G32_UINT,//=101,
    GL_RG32I,                                      // R32G32_SINT,//= 102,
    GL_RG32F,                                      // R32G32_SFLOAT,
    GL_RGB32UI,                                    // R32G32B32_UINT,//= 104,
    GL_RGB32I,                                     // R32G32B32_SINT,// =105,
    GL_RGB32F,                                     // R32G32B32_SFLOAT,// =106,
    GL_RGBA32UI,                                   // R32G32B32A32_UINT,// =107,
    GL_RGBA32I,                                    // R32G32B32A32_SINT,// =108,
    GL_RGBA32F,                                    // R32G32B32A32_SFLOAT,				
    GL_NONE,                                       // R64_UINT,
    GL_NONE,                                       // R64_SINT,
    GL_NONE,                                       // R64_SFLOAT,
    GL_NONE,                                       // R64G64_UINT,
    GL_NONE,                                       // R64G64_SINT,
    GL_NONE,                                       // R64G64_SFLOAT,
    GL_NONE,                                       // R64G64B64_UINT,
    GL_NONE,                                       // R64G64B64_SINT,
    GL_NONE,                                       // R64G64B64_SFLOAT,
    GL_NONE,                                       // R64G64B64A64_UINT,
    GL_NONE,                                       // R64G64B64A64_SINT,
    GL_NONE,                                       // R64G64B64A64_SFLOAT, // = 121,
    GL_NONE,                                       // B10G11R11_UFLOAT_PACK32,			
    GL_NONE,                                       // E5B9G9R9_UFLOAT_PACK32,
    GL_DEPTH_COMPONENT16,                          // D16_UNORM,						
    GL_DEPTH_COMPONENT24,                          // X8_D24_UNORM_PACK32,				
    GL_DEPTH_COMPONENT32F,                         // D32_SFLOAT,// =126,
    GL_STENCIL_INDEX8,                             // S8_UINT,// = 127,
    GL_NONE,                                       // D16_UNORM_S8_UINT,// = 128,
    GL_DEPTH24_STENCIL8,                           // D24_UNORM_S8_UINT,kkkkkkk// =
    GL_DEPTH32F_STENCIL8,                          // D32_SFLOAT_S8_UINT,// =130,
    GL_NONE,                                       // BC1_RGB_UNORM_BLOCK, // = 131,
    GL_NONE,                                       // BC1_RGB_SRGB_BLOCK,// = 132,
    GL_NONE,                                       // BC1_RGBA_UNORM_BLOCK, // = 133,
    GL_NONE,                                       // BC1_RGBA_SRGB_BLOCK, // = 134,
    GL_NONE,                                       // BC2_UNORM_BLOCK,// = 135,
    GL_NONE,                                       // BC2_SRGB_BLOCK,// = 136,
    GL_NONE,                                       // BC3_UNORM_BLOCK,// = 137,
    GL_NONE,                                       // BC3_SRGB_BLOCK,// = 138,
    GL_COMPRESSED_RED_RGTC1,                       // BC4_UNORM_BLOCK,// = 139,
    GL_COMPRESSED_SIGNED_RED_RGTC1_EXT,            // BC4_SNORM_BLOCK,// = 140,
    GL_COMPRESSED_RG_RGTC2,                        // BC5_UNORM_BLOCK,// = 141,
    GL_COMPRESSED_SIGNED_RG_RGTC2,                 // BC5_SNORM_BLOCK,// = 142,
    GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,         // BC6H_UFLOAT_BLOCK, // = 143,
    GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,           // BC6H_SFLOAT_BLOCK,// = 144,
    GL_COMPRESSED_RGBA_BPTC_UNORM,                 // BC7_UNORM_BLOCK,// =145,
    GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,           // BC7_SRGB_BLOCK,
    GL_COMPRESSED_RGB8_ETC2,                       // ETC2_R8G8B8_UNORM_BLOCK,
    GL_COMPRESSED_SRGB8_ETC2,                      // ETC2_R8G8B8_SRGB_BLOCK,
    GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,   // ETC2_R8G8B8A1_UNORM_BLOCK,
    GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,  // ETC2_R8G8B8A1_SRGB_BLOCK,
    GL_COMPRESSED_RGBA8_ETC2_EAC,                  // ETC2_R8G8B8A8_UNORM_BLOCK,		
    GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,           // ETC2_R8G8B8A8_SRGB_BLOCK,
    GL_COMPRESSED_R11_EAC,                         // EAC_R11_UNORM_BLOCK,				
    GL_COMPRESSED_SIGNED_R11_EAC,                  // EAC_R11_SNORM_BLOCK,				
    GL_COMPRESSED_RG11_EAC,                        // EAC_R11G11_UNORM_BLOCK,
    GL_COMPRESSED_SIGNED_RG11_EAC,                 // EAC_R11G11_SNORM_BLOCK,			
    GL_NONE,                                       // ASTC_4x4_UNORM_BLOCK, // = 157,
    GL_NONE,                                       // ASTC_4x4_SRGB_BLOCK, // = 158,
    GL_NONE,                                       // ASTC_5x4_UNORM_BLOCK, // = 159,
    GL_NONE,                                       // ASTC_5x4_SRGB_BLOCK, // = 160,
    GL_NONE,                                       // ASTC_5x5_UNORM_BLOCK, // = 161,
    GL_NONE,                                       // ASTC_5x5_SRGB_BLOCK, // = 162,
    GL_NONE,                                       // ASTC_6x5_UNORM_BLOCK, // = 163,
    GL_NONE,                                       // ASTC_6x5_SRGB_BLOCK, // = 164,
    GL_NONE,                                       // ASTC_6x6_UNORM_BLOCK, // = 165,
    GL_NONE,                                       // ASTC_6x6_SRGB_BLOCK, // = 166,
    GL_NONE,                                       // ASTC_8x5_UNORM_BLOCK, // = 167,
    GL_NONE,                                       // ASTC_8x5_SRGB_BLOCK, // = 168,
    GL_NONE,                                       // ASTC_8x6_UNORM_BLOCK, // = 169,
    GL_NONE,                                       // ASTC_8x6_SRGB_BLOCK, // = 170,
    GL_NONE,                                       // ASTC_8x8_UNORM_BLOCK, // = 171,
    GL_NONE,                                       // ASTC_8x8_SRGB_BLOCK, // = 172,
    GL_NONE,                                       // ASTC_10x5_UNORM_BLOCK, // = 173,
    GL_NONE,                                       // ASTC_10x5_SRGB_BLOCK, // = 174,
    GL_NONE,                                       // ASTC_10x6_UNORM_BLOCK, // = 175,
    GL_NONE,                                       // ASTC_10x6_SRGB_BLOCK, // = 176,
    GL_NONE,                                       // ASTC_10x8_UNORM_BLOCK, // = 177,
    GL_NONE,                                       // ASTC_10x8_SRGB_BLOCK, // = 178,
    GL_NONE,                                       // ASTC_10x10_UNORM_BLOCK,
    GL_NONE,                                       // ASTC_10x10_SRGB_BLOCK, // = 180,
    GL_NONE,                                       // ASTC_12x10_UNORM_BLOCK,
    GL_NONE,                                       // ASTC_12x10_SRGB_BLOCK, // = 182,
    GL_NONE,                                       // ASTC_12x12_UNORM_BLOCK,
    GL_NONE,                                       // ASTC_12x12_SRGB_BLOCK, // = 184,
    GL_NONE,                                       // G8B8G8R8_422_UNORM,
    GL_NONE,                                       // B8G8R8G8_422_UNORM,
    GL_NONE,                                       // G8_B8_R8_3PLANE_420_UNORM,					//
    GL_NONE,                                       // G8_B8R8_2PLANE_420_UNORM,					//
    GL_NONE,                                       // G8_B8_R8_3PLANE_422_UNORM,					//
    GL_NONE,                                       // G8_B8R8_2PLANE_422_UNORM,					//
    GL_NONE,                                       // G8_B8_R8_3PLANE_444_UNORM,					//
    GL_NONE,                                       // R10X6_UNORM_PACK16,
    GL_NONE,                                       // R10X6G10X6_UNORM_2PACK16,					//
    GL_NONE,                                       // R10X6G10X6B10X6A10X6_UNORM_4PACK16,			// =
    GL_NONE,                                       // G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,		// =
    GL_NONE,                                       // B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,		// 
    GL_NONE,                                       // G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16, // = 
    GL_NONE,                                       // G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,	// 
    GL_NONE,                                       // G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16, // = 
    GL_NONE,                                       // G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,	// 
    GL_NONE,                                       // G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16, // = 
    GL_NONE,                                       // R12X4_UNORM_PACK16,
    GL_NONE,                                       // R12X4G12X4_UNORM_2PACK16,					//
    GL_NONE,                                       // R12X4G12X4B12X4A12X4_UNORM_4PACK16,			// =
    GL_NONE,                                       // G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,		// =
    GL_NONE,                                       // B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,		
    GL_NONE,                                       // G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16, //
    GL_NONE,                                       // G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,	/
    GL_NONE,                                       // G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16, //
    GL_NONE,                                       // G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,	/
    GL_NONE,                                       // G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16, //
    GL_NONE,                                       // G16B16G16R16_422_UNORM,
    GL_NONE,                                       // B16G16R16G16_422_UNORM,
    GL_NONE,                                       // G16_B16_R16_3PLANE_420_UNORM,				// =
    GL_NONE,                                       // G16_B16R16_2PLANE_420_UNORM,				// =
    GL_NONE,                                       // G16_B16_R16_3PLANE_422_UNORM,				// =
    GL_NONE,                                       // G16_B16R16_2PLANE_422_UNORM,				// =
    GL_NONE,                                       // G16_B16_R16_3PLANE_444_UNORM,				// =
    GL_NONE,                                       // PVRTC1_2BPP_UNORM_BLOCK_IMG,				// =
    GL_NONE,                                       // PVRTC1_4BPP_UNORM_BLOCK_IMG,				// =
    GL_NONE,                                       // PVRTC2_2BPP_UNORM_BLOCK_IMG,				// =
    GL_NONE,                                       // PVRTC2_4BPP_UNORM_BLOCK_IMG,				// =
    GL_NONE,                                       // PVRTC1_2BPP_SRGB_BLOCK_IMG, // = 1000054004,
    GL_NONE,                                       // PVRTC1_4BPP_SRGB_BLOCK_IMG, // = 1000054005,
    GL_NONE,                                       // PVRTC2_2BPP_SRGB_BLOCK_IMG, // = 1000054006,
    GL_NONE,                                       // PVRTC2_4BPP_SRGB_BLOCK_IMG, // = 1000054007,
    GL_NONE,                                       // ASTC_4x4_SFLOAT_BLOCK_EXT,					//
    GL_NONE,                                       // ASTC_5x4_SFLOAT_BLOCK_EXT,					//
    GL_NONE,                                       // ASTC_5x5_SFLOAT_BLOCK_EXT,					//
    GL_NONE,                                       // ASTC_6x5_SFLOAT_BLOCK_EXT,					//
    GL_NONE,                                       // ASTC_6x6_SFLOAT_BLOCK_EXT,					//
    GL_NONE,                                       // ASTC_8x5_SFLOAT_BLOCK_EXT,					//
    GL_NONE,                                       // ASTC_8x6_SFLOAT_BLOCK_EXT,					//
    GL_NONE,                                       // ASTC_8x8_SFLOAT_BLOCK_EXT,					//
    GL_NONE,                                       // ASTC_10x5_SFLOAT_BLOCK_EXT, // = 1000066008,
    GL_NONE,                                       // ASTC_10x6_SFLOAT_BLOCK_EXT, // = 1000066009,
    GL_NONE,                                       // ASTC_10x8_SFLOAT_BLOCK_EXT, // = 1000066010,
    GL_NONE,                                       // ASTC_10x10_SFLOAT_BLOCK_EXT,				// =
    GL_NONE,                                       // ASTC_12x10_SFLOAT_BLOCK_EXT,				// =
    GL_NONE,                                       // ASTC_12x12_SFLOAT_BLOCK_EXT,				// =
    GL_NONE,                                       // A4R4G4B4_UNORM_PACK16_EXT,					//
    GL_NONE,                                       // A4B4G4R4_UNORM_PACK16_EXT,					//
};
constexpr GLenum glShaderStageArray[]{GL_VERTEX_SHADER,
                                      GL_FRAGMENT_SHADER,
                                      GL_GEOMETRY_SHADER,
                                      GL_TESS_CONTROL_SHADER,
                                      GL_TESS_EVALUATION_SHADER,
                                      GL_COMPUTE_SHADER,
                                      GL_NONE};
constexpr GLenum glShaderStageFlagBitsArray[]{
    GL_VERTEX_SHADER_BIT,           // 0x00000001
    GL_FRAGMENT_SHADER_BIT,         // 0x00000002
    GL_GEOMETRY_SHADER_BIT,         // 0x00000004
    GL_TESS_CONTROL_SHADER_BIT,     // 0x00000008
    GL_TESS_EVALUATION_SHADER_BIT,  // 0x00000010
    GL_COMPUTE_SHADER_BIT,          // 0x00000020
    GL_ALL_SHADER_BITS,             // 0xFFFFFFFF
};
constexpr GLenum glBufferBindTargetArray[]{
    GL_COPY_READ_BUFFER,
    GL_COPY_WRITE_BUFFER,
    GL_TEXTURE_BUFFER,
    // 0, //storage texel
    GL_UNIFORM_BUFFER,
    GL_SHADER_STORAGE_BUFFER,
    GL_ELEMENT_ARRAY_BUFFER,
    GL_ARRAY_BUFFER,
    GL_DRAW_INDIRECT_BUFFER,
    GL_TRANSFORM_FEEDBACK_BUFFER,
};
constexpr GLbitfield glBufferMapFlagBitArray[]{
    GL_MAP_READ_BIT,               // 0x0001
    GL_MAP_WRITE_BIT,              // 0x0002
    GL_MAP_INVALIDATE_RANGE_BIT,   // 0x0004
    GL_MAP_INVALIDATE_BUFFER_BIT,  // 0x0008
    GL_MAP_FLUSH_EXPLICIT_BIT,     // 0x0010
    GL_MAP_UNSYNCHRONIZED_BIT,     // 0x0020
    GL_MAP_PERSISTENT_BIT,         // 0x0040
    GL_MAP_COHERENT_BIT,           // 0x0080
};
constexpr GLbitfield glBufferStorageFlagBitArray[]{
    GL_MAP_READ_BIT,         // 0x0001
    GL_MAP_WRITE_BIT,        // 0x0002
    GL_MAP_PERSISTENT_BIT,   // 0x0040
    GL_MAP_COHERENT_BIT,     // 0x0080
    GL_DYNAMIC_STORAGE_BIT,  // 0x0100
    GL_CLIENT_STORAGE_BIT,   // 0x0200
};
constexpr GLenum glMutableStorageUsageArray[]{
    GL_STREAM_DRAW,  GL_STREAM_READ, GL_STREAM_COPY, GL_DYNAMIC_DRAW, GL_DYNAMIC_READ,
    GL_DYNAMIC_COPY, GL_STATIC_DRAW, GL_STATIC_READ, GL_STATIC_COPY,
};
constexpr GLenum glImageTypeArray[]{
    GL_TEXTURE_1D_ARRAY,
    GL_TEXTURE_2D_ARRAY,
    GL_TEXTURE_3D,
};
constexpr GLenum glImageViewTypeArray[]{
    GL_TEXTURE_1D,
    GL_TEXTURE_2D,
    GL_TEXTURE_3D,
    GL_TEXTURE_CUBE_MAP,
    GL_TEXTURE_1D_ARRAY,
    GL_TEXTURE_2D_ARRAY,
    GL_TEXTURE_CUBE_MAP_ARRAY,
};
constexpr GLenum glFilterArray[]{
    GL_NEAREST,
    GL_LINEAR,
};

constexpr GLenum glSamplerWrapModeArray[]{
    GL_REPEAT,
    GL_MIRRORED_REPEAT,
    GL_CLAMP_TO_EDGE,
    GL_CLAMP_TO_BORDER,
};
constexpr GLenum glCompareOpArray[]{GL_NEVER,   GL_LESS,     GL_EQUAL,  GL_LEQUAL,
                                    GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS};
constexpr GLenum glComponentSwizzleArray[]{GL_NONE, GL_ZERO, GL_ONE, GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
constexpr GLenum glPrimitiveTopologyArray[]{
    GL_POINTS,       GL_LINES,           GL_LINE_STRIP,           GL_TRIANGLES,           GL_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN, GL_LINES_ADJACENCY, GL_LINE_STRIP_ADJACENCY, GL_TRIANGLES_ADJACENCY, GL_TRIANGLE_STRIP_ADJACENCY,
    GL_PATCHES};
constexpr GLenum glPolygonModeArray[]{
    GL_FILL,
    GL_LINE,
    GL_POINT,
};
constexpr GLenum glCullModeArray[]{GL_NONE, GL_FRONT, GL_BACK, GL_FRONT_AND_BACK};

// set clipcontrol origin
constexpr GLenum glFrontFaceArray[]{
#ifdef CLIP_ORIGIN_UPPER_LEFT
    GL_CW,
    GL_CCW,
#else
    GL_CCW,
    GL_CW,
#endif
};
constexpr GLenum glStencilOpArray[]{GL_KEEP, GL_ZERO,   GL_REPLACE,   GL_INCR,
                                    GL_DECR, GL_INVERT, GL_INCR_WRAP, GL_DECR_WRAP};
constexpr GLenum glLogicOpArray[]{
    GL_CLEAR, GL_AND,   GL_AND_REVERSE, GL_COPY,       GL_AND_INVERTED,  GL_NOOP,        GL_XOR,  GL_OR,
    GL_NOR,   GL_EQUIV, GL_INVERT,      GL_OR_REVERSE, GL_COPY_INVERTED, GL_OR_INVERTED, GL_NAND, GL_SET};
constexpr GLenum glBlendFactorArray[]{
    GL_ZERO,
    GL_ONE,
    GL_SRC_COLOR,
    GL_ONE_MINUS_SRC_COLOR,
    GL_DST_COLOR,
    GL_ONE_MINUS_DST_COLOR,
    GL_SRC_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_ALPHA,
    GL_ONE_MINUS_DST_ALPHA,
    GL_CONSTANT_COLOR,
    GL_ONE_MINUS_CONSTANT_COLOR,
    GL_CONSTANT_ALPHA,
    GL_ONE_MINUS_CONSTANT_ALPHA,
    GL_SRC_ALPHA_SATURATE,
    GL_SRC1_COLOR,
    GL_ONE_MINUS_SRC1_COLOR,
    GL_SRC1_ALPHA,
    GL_ONE_MINUS_SRC1_ALPHA,
};
constexpr GLenum glBlendOpArray[]{
    GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX,
};
constexpr GLenum glBaseFormatArray[]{
    GL_RED, GL_RG, GL_RGB, GL_RGBA, GL_DEPTH_COMPONENT, GL_STENCIL_INDEX, GL_DEPTH_STENCIL, GL_BGR, GL_BGRA,
};
constexpr GLenum glPixelFormatArray[]{
    GL_STENCIL_INDEX,
    GL_DEPTH_COMPONENT,
    GL_DEPTH_STENCIL,
    GL_RED,
    GL_GREEN,
    GL_BLUE,
    GL_RG,
    GL_RGB,
    GL_RGBA,
    GL_BGR,
    GL_BGRA,
    GL_RED_INTEGER,
    GL_GREEN_INTEGER,
    GL_BLUE_INTEGER,
    GL_RG_INTEGER,
    GL_RGB_INTEGER,
    GL_RGBA_INTEGER,
    GL_BGR_INTEGER,
    GL_BGRA_INTEGER,
};
GLenum Map(BlendOp op) { return glBlendOpArray[static_cast<size_t>(op)]; }
GLenum Map(BlendFactor factor) { return glBlendFactorArray[static_cast<size_t>(factor)]; }
GLenum Map(LogicOp op) { return glLogicOpArray[static_cast<size_t>(op)]; }
GLenum Map(StencilOp op) { return glStencilOpArray[static_cast<size_t>(op)]; }
GLenum Map(FrontFace face) { return glFrontFaceArray[static_cast<size_t>(face)]; }
GLenum Map(CullMode mode) { return glCullModeArray[static_cast<size_t>(mode)]; }
GLenum Map(PolygonMode mode) { return glPolygonModeArray[static_cast<size_t>(mode)]; }
GLenum Map(PrimitiveTopology topology) { return glPrimitiveTopologyArray[static_cast<size_t>(topology)]; }
GLenum Map(ComponentSwizzle swizzle) { return glComponentSwizzleArray[static_cast<size_t>(swizzle)]; }
GLenum Map(ShaderStageFlagBits flag) {
    switch (flag) {
        case ShaderStageFlagBits::VERTEX_BIT:
            return GL_VERTEX_SHADER;
        case ShaderStageFlagBits::FRAGMENT_BIT:
            return GL_FRAGMENT_SHADER;
        case ShaderStageFlagBits::GEOMETRY_BIT:
            return GL_GEOMETRY_SHADER;
        case ShaderStageFlagBits::TESSELLATION_CONTROL_BIT:
            return GL_TESS_CONTROL_SHADER;
        case ShaderStageFlagBits::TESSELLATION_EVALUATION_BIT:
            return GL_TESS_EVALUATION_SHADER;
        case ShaderStageFlagBits::COMPUTE_BIT:
            return GL_COMPUTE_SHADER;
        default:
            ST_THROW("GL do not contain shader stage: %d", int(flag));
    }
}
GLbitfield MapShaderStageFlags(ShaderStageFlagBits flags) {
    if (static_cast<bool>(flags == ShaderStageFlagBits::ALL)) return GL_ALL_SHADER_BITS;
    if (static_cast<bool>(flags == ShaderStageFlagBits::ALL_GRAPHICS)) return 0x1F;
    GLbitfield ret{};
    if (static_cast<bool>(flags & ShaderStageFlagBits::VERTEX_BIT)) ret |= GL_VERTEX_SHADER_BIT;
    if (static_cast<bool>(flags & ShaderStageFlagBits::FRAGMENT_BIT)) ret |= GL_FRAGMENT_SHADER_BIT;
    if (static_cast<bool>(flags & ShaderStageFlagBits::TESSELLATION_CONTROL_BIT)) ret |= GL_TESS_CONTROL_SHADER_BIT;
    if (static_cast<bool>(flags & ShaderStageFlagBits::TESSELLATION_EVALUATION_BIT))
        ret |= GL_TESS_EVALUATION_SHADER_BIT;
    if (static_cast<bool>(flags & ShaderStageFlagBits::GEOMETRY_BIT)) ret |= GL_GEOMETRY_SHADER_BIT;
    if (static_cast<bool>(flags & ShaderStageFlagBits::COMPUTE_BIT)) ret |= GL_COMPUTE_SHADER_BIT;
    return ret;
}
GLenum MapInternalFormat(Format format) {
    auto ret = glFormatArray[static_cast<size_t>(format)];
    if (ret == GL_NONE) ST_THROW("failed to find corresponding GL fomat", (int)format);
    return ret;
}
GLenum MapBaseFormat(Format format) {
    auto &&a = GetFormatAttribute(format);
    return glBaseFormatArray[static_cast<size_t>(a.baseFormat)];
}
GLenum MapPixelFormat(Format format) {
    auto &&a = GetFormatAttribute(format);
    switch (a.formatNumeric) {
        case FormatNumeric::SINT:
        case FormatNumeric::UINT:
            switch (a.baseFormat) {
                case BaseFormat::R:
                    return GL_RED_INTEGER;
                    break;
                case BaseFormat::RG:
                    return GL_RG_INTEGER;
                    break;
                case BaseFormat::RGB:
                    return GL_RGB_INTEGER;
                    break;
                case BaseFormat::RGBA:
                    return GL_RGBA_INTEGER;
                    break;
                case BaseFormat::DEPTH:
                    return GL_DEPTH_COMPONENT;
                    break;
                case BaseFormat::STENCIL:
                    return GL_STENCIL_INDEX;
                    break;
                case BaseFormat::DEPTH_STENCIL:
                    return GL_DEPTH_STENCIL;
                    break;
                case BaseFormat::BGR:
                    return GL_BGR_INTEGER;
                    break;
                case BaseFormat::BGRA:
                    return GL_BGRA_INTEGER;
                    break;
            }
            break;
        default:
            switch (a.baseFormat) {
                case BaseFormat::R:
                    return GL_RED;
                    break;
                case BaseFormat::RG:
                    return GL_RG;
                    break;
                case BaseFormat::RGB:
                    return GL_RGB;
                    break;
                case BaseFormat::RGBA:
                    return GL_RGBA;
                    break;
                case BaseFormat::DEPTH:
                    return GL_DEPTH_COMPONENT;
                    break;
                case BaseFormat::STENCIL:
                    return GL_STENCIL_INDEX;
                    break;
                case BaseFormat::DEPTH_STENCIL:
                    return GL_DEPTH_STENCIL;
                    break;
                case BaseFormat::BGR:
                    return GL_BGR;
                    break;
                case BaseFormat::BGRA:
                    return GL_BGRA;
                    break;
            }
            break;
    }
    return GL_RGBA;
}
GLenum Map(BufferMutableStorageUsage usage) { return glMutableStorageUsageArray[static_cast<size_t>(usage)]; }
GLbitfield Map(BufferMapFlagBits flag) { return static_cast<GLbitfield>(flag); }
GLbitfield Map(BufferStorageFlagBits flag) { return static_cast<GLbitfield>(flag); }
GLenum Map(SamplerWrapMode wrapMode) { return glSamplerWrapModeArray[static_cast<size_t>(wrapMode)]; }
GLenum Map(CompareOp op) { return glCompareOpArray[static_cast<size_t>(op)]; }
GLenum Map(Filter filter) { return glFilterArray[static_cast<size_t>(filter)]; }
GLenum Map(ImageType imageType, SampleCountFlagBits sampleCountFlag) {
    if (sampleCountFlag > SampleCountFlagBits::BIT_1)
        return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
    else
        return glImageTypeArray[static_cast<size_t>(imageType)];
}
GLenum Map(ImageViewType viewType, SampleCountFlagBits sampleCountFlag) {
    if (sampleCountFlag > SampleCountFlagBits::BIT_1) {
        if (viewType == ImageViewType::TYPE_2D)
            return GL_TEXTURE_2D_MULTISAMPLE;
        else if (viewType == ImageViewType::TYPE_2D_ARRAY)
            return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
        else
            ST_THROW("wrong image view type");
    } else
        return glImageViewTypeArray[static_cast<size_t>(viewType)];
}
GLenum Map(DataType dataType) { return glDataTypeArray[static_cast<size_t>(dataType)]; }
GLenum Map(IndexType type) {
    switch (type) {
        case IndexType::UINT8:
            return GL_UNSIGNED_BYTE;
        case IndexType::UINT16:
            return GL_UNSIGNED_SHORT;
        case IndexType::UINT32:
            return GL_UNSIGNED_INT;
        default:
            ST_THROW("wrong index type");
    }
}
std::variant<std::array<float, 4>, std::array<int32_t, 4>> Map(BorderColor color) {
    switch (color) {
        case BorderColor::FLOAT_OPAQUE_BLACK:
            return std::array<float, 4>{0.f, 0.f, 0.f, 1.f};
        case BorderColor::FLOAT_OPAQUE_WHITE:
            return std::array<float, 4>{1.f, 1.f, 1.f, 1.f};
        case BorderColor::FLOAT_TRANSPARENT_BLACK:
        default:
            return std::array<float, 4>{0.f, 0.f, 0.f, 0.f};
        case BorderColor::INT_OPAQUE_BLACK:
            return std::array<int32_t, 4>{0, 0, 0, 1};
        case BorderColor::INT_OPAQUE_WHITE:
            return std::array<int32_t, 4>{1, 1, 1, 1};
        case BorderColor::INT_TRANSPARENT_BLACK:
            return std::array<int32_t, 4>{0, 0, 0, 0};
    }
}
GLenum GetFBOBindPoint(GLImageFlag flag) {
    switch (flag) {
        case GLImageFlag::BACK_LEFT_FRAMEBUFFER:
            return GL_BACK_LEFT;
        case GLImageFlag::BACK_RIGHT_FRAMEBUFFER:
            return GL_BACK_RIGHT;
        case GLImageFlag::FRONT_LEFT_FRAMEBUFFER:
            return GL_FRONT_LEFT;
        case GLImageFlag::FRONT_RIGHT_FRAMEBUFFER:
            return GL_FRONT_RIGHT;
        default:
            break;
    }
    ST_THROW("image is not framebuffer");
}
}  // namespace Shit
