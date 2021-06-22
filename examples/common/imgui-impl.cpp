#include "imgui-impl.hpp"
#include <stdio.h>

struct ImGui_ImplShitRendererH_FrameRenderBuffers
{
	uint64_t VertexBufferSize;
	uint64_t IndexBufferSize;
	Buffer *VertexBuffer;
	Buffer *IndexBuffer;
};

struct ImGui_ImplShitRendererH_WindowRenderBuffers
{
	uint32_t Index;
	uint32_t Count;
	ImGui_ImplShitRendererH_FrameRenderBuffers *FrameRenderBuffers;
};

static ImGui_ImplShitRenderer_InitInfo g_ShitRendererInitInfo = {};

static RenderPass *g_RenderPass;
static uint32_t g_Subpass;

static uint64_t g_BufferMemoryAlignment = 256;
static DescriptorSetLayout *g_DescriptorSetLayout;
static PipelineLayout *g_PipelineLayout;

static std::vector<std::vector<DescriptorSet *>> g_FrameDescriptorSets;

static Pipeline *g_Pipeline;
static Shader *g_ShaderModuleVert;
static Shader *g_ShaderModuleFrag;

static bool g_FunctionsLoaded = true;

static Sampler *g_FontSampler;
static Image *g_FontImage;
static ImageView *g_FontView;
static Buffer *g_UploadBuffer;

static DescriptorPool *g_DescriptorPool;

static ImGui_ImplShitRendererH_WindowRenderBuffers g_MainWindowRenderBuffers;

/*
#version 450 core
#extension GL_EXT_scalar_block_layout : enable
#ifdef VULKAN
#define PUSH_CONSTANT push_constant
#else
#define PUSH_CONSTANT std430,binding = 0
#endif

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(PUSH_CONSTANT) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

void main()
{
    Out.Color = aColor;
    Out.UV = aUV;
    gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
}
*/

static uint32_t __glsl_shader_vert_spv_vk[] = {
	0x07230203, 0x00010300, 0x0008000a, 0x0000002e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
	0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
	0x000a000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000b, 0x0000000f, 0x00000015,
	0x0000001b, 0x0000001c, 0x00030003, 0x00000002, 0x000001c2, 0x00080004, 0x455f4c47, 0x735f5458,
	0x616c6163, 0x6c625f72, 0x5f6b636f, 0x6f79616c, 0x00007475, 0x00040005, 0x00000004, 0x6e69616d,
	0x00000000, 0x00030005, 0x00000009, 0x00000000, 0x00050006, 0x00000009, 0x00000000, 0x6f6c6f43,
	0x00000072, 0x00040006, 0x00000009, 0x00000001, 0x00005655, 0x00030005, 0x0000000b, 0x0074754f,
	0x00040005, 0x0000000f, 0x6c6f4361, 0x0000726f, 0x00030005, 0x00000015, 0x00565561, 0x00060005,
	0x00000019, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x00000019, 0x00000000,
	0x505f6c67, 0x7469736f, 0x006e6f69, 0x00030005, 0x0000001b, 0x00000000, 0x00040005, 0x0000001c,
	0x736f5061, 0x00000000, 0x00060005, 0x0000001e, 0x73755075, 0x6e6f4368, 0x6e617473, 0x00000074,
	0x00050006, 0x0000001e, 0x00000000, 0x61635375, 0x0000656c, 0x00060006, 0x0000001e, 0x00000001,
	0x61725475, 0x616c736e, 0x00006574, 0x00030005, 0x00000020, 0x00006370, 0x00040047, 0x0000000b,
	0x0000001e, 0x00000000, 0x00040047, 0x0000000f, 0x0000001e, 0x00000002, 0x00040047, 0x00000015,
	0x0000001e, 0x00000001, 0x00050048, 0x00000019, 0x00000000, 0x0000000b, 0x00000000, 0x00030047,
	0x00000019, 0x00000002, 0x00040047, 0x0000001c, 0x0000001e, 0x00000000, 0x00050048, 0x0000001e,
	0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x0000001e, 0x00000001, 0x00000023, 0x00000008,
	0x00030047, 0x0000001e, 0x00000002, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002,
	0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040017,
	0x00000008, 0x00000006, 0x00000002, 0x0004001e, 0x00000009, 0x00000007, 0x00000008, 0x00040020,
	0x0000000a, 0x00000003, 0x00000009, 0x0004003b, 0x0000000a, 0x0000000b, 0x00000003, 0x00040015,
	0x0000000c, 0x00000020, 0x00000001, 0x0004002b, 0x0000000c, 0x0000000d, 0x00000000, 0x00040020,
	0x0000000e, 0x00000001, 0x00000007, 0x0004003b, 0x0000000e, 0x0000000f, 0x00000001, 0x00040020,
	0x00000011, 0x00000003, 0x00000007, 0x0004002b, 0x0000000c, 0x00000013, 0x00000001, 0x00040020,
	0x00000014, 0x00000001, 0x00000008, 0x0004003b, 0x00000014, 0x00000015, 0x00000001, 0x00040020,
	0x00000017, 0x00000003, 0x00000008, 0x0003001e, 0x00000019, 0x00000007, 0x00040020, 0x0000001a,
	0x00000003, 0x00000019, 0x0004003b, 0x0000001a, 0x0000001b, 0x00000003, 0x0004003b, 0x00000014,
	0x0000001c, 0x00000001, 0x0004001e, 0x0000001e, 0x00000008, 0x00000008, 0x00040020, 0x0000001f,
	0x00000009, 0x0000001e, 0x0004003b, 0x0000001f, 0x00000020, 0x00000009, 0x00040020, 0x00000021,
	0x00000009, 0x00000008, 0x0004002b, 0x00000006, 0x00000028, 0x00000000, 0x0004002b, 0x00000006,
	0x00000029, 0x3f800000, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8,
	0x00000005, 0x0004003d, 0x00000007, 0x00000010, 0x0000000f, 0x00050041, 0x00000011, 0x00000012,
	0x0000000b, 0x0000000d, 0x0003003e, 0x00000012, 0x00000010, 0x0004003d, 0x00000008, 0x00000016,
	0x00000015, 0x00050041, 0x00000017, 0x00000018, 0x0000000b, 0x00000013, 0x0003003e, 0x00000018,
	0x00000016, 0x0004003d, 0x00000008, 0x0000001d, 0x0000001c, 0x00050041, 0x00000021, 0x00000022,
	0x00000020, 0x0000000d, 0x0004003d, 0x00000008, 0x00000023, 0x00000022, 0x00050085, 0x00000008,
	0x00000024, 0x0000001d, 0x00000023, 0x00050041, 0x00000021, 0x00000025, 0x00000020, 0x00000013,
	0x0004003d, 0x00000008, 0x00000026, 0x00000025, 0x00050081, 0x00000008, 0x00000027, 0x00000024,
	0x00000026, 0x00050051, 0x00000006, 0x0000002a, 0x00000027, 0x00000000, 0x00050051, 0x00000006,
	0x0000002b, 0x00000027, 0x00000001, 0x00070050, 0x00000007, 0x0000002c, 0x0000002a, 0x0000002b,
	0x00000028, 0x00000029, 0x00050041, 0x00000011, 0x0000002d, 0x0000001b, 0x0000000d, 0x0003003e,
	0x0000002d, 0x0000002c, 0x000100fd, 0x00010038};

static uint32_t __glsl_shader_vert_spv_gl[] = {
	0x07230203, 0x00010000, 0x0008000a, 0x00000031, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
	0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
	0x000c000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000b, 0x0000000f, 0x00000015,
	0x0000001b, 0x0000001c, 0x0000002f, 0x00000030, 0x00030003, 0x00000002, 0x000001c2, 0x00080004,
	0x455f4c47, 0x735f5458, 0x616c6163, 0x6c625f72, 0x5f6b636f, 0x6f79616c, 0x00007475, 0x00040005,
	0x00000004, 0x6e69616d, 0x00000000, 0x00030005, 0x00000009, 0x00000000, 0x00050006, 0x00000009,
	0x00000000, 0x6f6c6f43, 0x00000072, 0x00040006, 0x00000009, 0x00000001, 0x00005655, 0x00030005,
	0x0000000b, 0x0074754f, 0x00040005, 0x0000000f, 0x6c6f4361, 0x0000726f, 0x00030005, 0x00000015,
	0x00565561, 0x00060005, 0x00000019, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006,
	0x00000019, 0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00030005, 0x0000001b, 0x00000000,
	0x00040005, 0x0000001c, 0x736f5061, 0x00000000, 0x00060005, 0x0000001e, 0x73755075, 0x6e6f4368,
	0x6e617473, 0x00000074, 0x00050006, 0x0000001e, 0x00000000, 0x61635375, 0x0000656c, 0x00060006,
	0x0000001e, 0x00000001, 0x61725475, 0x616c736e, 0x00006574, 0x00030005, 0x00000020, 0x00006370,
	0x00050005, 0x0000002f, 0x565f6c67, 0x65747265, 0x00444978, 0x00060005, 0x00000030, 0x495f6c67,
	0x6174736e, 0x4965636e, 0x00000044, 0x00040047, 0x0000000b, 0x0000001e, 0x00000000, 0x00040047,
	0x0000000f, 0x0000001e, 0x00000002, 0x00040047, 0x00000015, 0x0000001e, 0x00000001, 0x00050048,
	0x00000019, 0x00000000, 0x0000000b, 0x00000000, 0x00030047, 0x00000019, 0x00000002, 0x00040047,
	0x0000001c, 0x0000001e, 0x00000000, 0x00050048, 0x0000001e, 0x00000000, 0x00000023, 0x00000000,
	0x00050048, 0x0000001e, 0x00000001, 0x00000023, 0x00000008, 0x00030047, 0x0000001e, 0x00000002,
	0x00040047, 0x00000020, 0x00000022, 0x00000000, 0x00040047, 0x00000020, 0x00000021, 0x00000000,
	0x00040047, 0x0000002f, 0x0000000b, 0x00000005, 0x00040047, 0x00000030, 0x0000000b, 0x00000006,
	0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020,
	0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040017, 0x00000008, 0x00000006, 0x00000002,
	0x0004001e, 0x00000009, 0x00000007, 0x00000008, 0x00040020, 0x0000000a, 0x00000003, 0x00000009,
	0x0004003b, 0x0000000a, 0x0000000b, 0x00000003, 0x00040015, 0x0000000c, 0x00000020, 0x00000001,
	0x0004002b, 0x0000000c, 0x0000000d, 0x00000000, 0x00040020, 0x0000000e, 0x00000001, 0x00000007,
	0x0004003b, 0x0000000e, 0x0000000f, 0x00000001, 0x00040020, 0x00000011, 0x00000003, 0x00000007,
	0x0004002b, 0x0000000c, 0x00000013, 0x00000001, 0x00040020, 0x00000014, 0x00000001, 0x00000008,
	0x0004003b, 0x00000014, 0x00000015, 0x00000001, 0x00040020, 0x00000017, 0x00000003, 0x00000008,
	0x0003001e, 0x00000019, 0x00000007, 0x00040020, 0x0000001a, 0x00000003, 0x00000019, 0x0004003b,
	0x0000001a, 0x0000001b, 0x00000003, 0x0004003b, 0x00000014, 0x0000001c, 0x00000001, 0x0004001e,
	0x0000001e, 0x00000008, 0x00000008, 0x00040020, 0x0000001f, 0x00000002, 0x0000001e, 0x0004003b,
	0x0000001f, 0x00000020, 0x00000002, 0x00040020, 0x00000021, 0x00000002, 0x00000008, 0x0004002b,
	0x00000006, 0x00000028, 0x00000000, 0x0004002b, 0x00000006, 0x00000029, 0x3f800000, 0x00040020,
	0x0000002e, 0x00000001, 0x0000000c, 0x0004003b, 0x0000002e, 0x0000002f, 0x00000001, 0x0004003b,
	0x0000002e, 0x00000030, 0x00000001, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003,
	0x000200f8, 0x00000005, 0x0004003d, 0x00000007, 0x00000010, 0x0000000f, 0x00050041, 0x00000011,
	0x00000012, 0x0000000b, 0x0000000d, 0x0003003e, 0x00000012, 0x00000010, 0x0004003d, 0x00000008,
	0x00000016, 0x00000015, 0x00050041, 0x00000017, 0x00000018, 0x0000000b, 0x00000013, 0x0003003e,
	0x00000018, 0x00000016, 0x0004003d, 0x00000008, 0x0000001d, 0x0000001c, 0x00050041, 0x00000021,
	0x00000022, 0x00000020, 0x0000000d, 0x0004003d, 0x00000008, 0x00000023, 0x00000022, 0x00050085,
	0x00000008, 0x00000024, 0x0000001d, 0x00000023, 0x00050041, 0x00000021, 0x00000025, 0x00000020,
	0x00000013, 0x0004003d, 0x00000008, 0x00000026, 0x00000025, 0x00050081, 0x00000008, 0x00000027,
	0x00000024, 0x00000026, 0x00050051, 0x00000006, 0x0000002a, 0x00000027, 0x00000000, 0x00050051,
	0x00000006, 0x0000002b, 0x00000027, 0x00000001, 0x00070050, 0x00000007, 0x0000002c, 0x0000002a,
	0x0000002b, 0x00000028, 0x00000029, 0x00050041, 0x00000011, 0x0000002d, 0x0000001b, 0x0000000d,
	0x0003003e, 0x0000002d, 0x0000002c, 0x000100fd, 0x00010038};

/*
#version 450 core
layout(location = 0) out vec4 fColor;
layout(binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
    fColor = In.Color * texture(sTexture, In.UV.st);
}
*/

static uint32_t __glsl_shader_frag_spv_vk[]{
	0x07230203, 0x00010300, 0x0008000a, 0x0000001e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
	0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
	0x0007000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000d, 0x00030010,
	0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d,
	0x00000000, 0x00040005, 0x00000009, 0x6c6f4366, 0x0000726f, 0x00030005, 0x0000000b, 0x00000000,
	0x00050006, 0x0000000b, 0x00000000, 0x6f6c6f43, 0x00000072, 0x00040006, 0x0000000b, 0x00000001,
	0x00005655, 0x00030005, 0x0000000d, 0x00006e49, 0x00050005, 0x00000016, 0x78655473, 0x65727574,
	0x00000000, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047, 0x0000000d, 0x0000001e,
	0x00000000, 0x00040047, 0x00000016, 0x00000022, 0x00000000, 0x00040047, 0x00000016, 0x00000021,
	0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006,
	0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003,
	0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00040017, 0x0000000a, 0x00000006,
	0x00000002, 0x0004001e, 0x0000000b, 0x00000007, 0x0000000a, 0x00040020, 0x0000000c, 0x00000001,
	0x0000000b, 0x0004003b, 0x0000000c, 0x0000000d, 0x00000001, 0x00040015, 0x0000000e, 0x00000020,
	0x00000001, 0x0004002b, 0x0000000e, 0x0000000f, 0x00000000, 0x00040020, 0x00000010, 0x00000001,
	0x00000007, 0x00090019, 0x00000013, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
	0x00000001, 0x00000000, 0x0003001b, 0x00000014, 0x00000013, 0x00040020, 0x00000015, 0x00000000,
	0x00000014, 0x0004003b, 0x00000015, 0x00000016, 0x00000000, 0x0004002b, 0x0000000e, 0x00000018,
	0x00000001, 0x00040020, 0x00000019, 0x00000001, 0x0000000a, 0x00050036, 0x00000002, 0x00000004,
	0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x00050041, 0x00000010, 0x00000011, 0x0000000d,
	0x0000000f, 0x0004003d, 0x00000007, 0x00000012, 0x00000011, 0x0004003d, 0x00000014, 0x00000017,
	0x00000016, 0x00050041, 0x00000019, 0x0000001a, 0x0000000d, 0x00000018, 0x0004003d, 0x0000000a,
	0x0000001b, 0x0000001a, 0x00050057, 0x00000007, 0x0000001c, 0x00000017, 0x0000001b, 0x00050085,
	0x00000007, 0x0000001d, 0x00000012, 0x0000001c, 0x0003003e, 0x00000009, 0x0000001d, 0x000100fd,
	0x00010038};
static uint32_t __glsl_shader_frag_spv_gl[]{
	0x07230203, 0x00010000, 0x0008000a, 0x0000001e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
	0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
	0x0007000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000d, 0x00030010,
	0x00000004, 0x00000008, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d,
	0x00000000, 0x00040005, 0x00000009, 0x6c6f4366, 0x0000726f, 0x00030005, 0x0000000b, 0x00000000,
	0x00050006, 0x0000000b, 0x00000000, 0x6f6c6f43, 0x00000072, 0x00040006, 0x0000000b, 0x00000001,
	0x00005655, 0x00030005, 0x0000000d, 0x00006e49, 0x00050005, 0x00000016, 0x78655473, 0x65727574,
	0x00000000, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047, 0x0000000d, 0x0000001e,
	0x00000000, 0x00040047, 0x00000016, 0x00000022, 0x00000000, 0x00040047, 0x00000016, 0x00000021,
	0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006,
	0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003,
	0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00040017, 0x0000000a, 0x00000006,
	0x00000002, 0x0004001e, 0x0000000b, 0x00000007, 0x0000000a, 0x00040020, 0x0000000c, 0x00000001,
	0x0000000b, 0x0004003b, 0x0000000c, 0x0000000d, 0x00000001, 0x00040015, 0x0000000e, 0x00000020,
	0x00000001, 0x0004002b, 0x0000000e, 0x0000000f, 0x00000000, 0x00040020, 0x00000010, 0x00000001,
	0x00000007, 0x00090019, 0x00000013, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
	0x00000001, 0x00000000, 0x0003001b, 0x00000014, 0x00000013, 0x00040020, 0x00000015, 0x00000000,
	0x00000014, 0x0004003b, 0x00000015, 0x00000016, 0x00000000, 0x0004002b, 0x0000000e, 0x00000018,
	0x00000001, 0x00040020, 0x00000019, 0x00000001, 0x0000000a, 0x00050036, 0x00000002, 0x00000004,
	0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x00050041, 0x00000010, 0x00000011, 0x0000000d,
	0x0000000f, 0x0004003d, 0x00000007, 0x00000012, 0x00000011, 0x0004003d, 0x00000014, 0x00000017,
	0x00000016, 0x00050041, 0x00000019, 0x0000001a, 0x0000000d, 0x00000018, 0x0004003d, 0x0000000a,
	0x0000001b, 0x0000001a, 0x00050057, 0x00000007, 0x0000001c, 0x00000017, 0x0000001b, 0x00050085,
	0x00000007, 0x0000001d, 0x00000012, 0x0000001c, 0x0003003e, 0x00000009, 0x0000001d, 0x000100fd,
	0x00010038};

//-----------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------

static void CreateOrResizeBuffer(Buffer *&buffer, uint64_t &p_buffer_size, size_t new_size, BufferUsageFlagBits usage)
{
	auto v = &g_ShitRendererInitInfo;
	v->device->Destroy(buffer);
	uint64_t vertex_buffer_size_aligned = ((new_size - 1) / g_BufferMemoryAlignment + 1) * g_BufferMemoryAlignment;
	buffer = v->device->Create(BufferCreateInfo{
								   {},
								   vertex_buffer_size_aligned,
								   usage,
								   MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
							   nullptr);
	p_buffer_size = buffer->GetCreateInfoPtr()->size;
}
static void ImGui_ImplShitRenderer_SetupRenderState(uint32_t imageIndex, ImDrawData *draw_data, Pipeline *pipeline, CommandBuffer *command_buffer, ImGui_ImplShitRendererH_FrameRenderBuffers *rb, int fb_width, int fb_height)
{

	command_buffer->BindPipeline(BindPipelineInfo{
		PipelineBindPoint::GRAPHICS,
		pipeline});

	std::vector<WriteDescriptorSet> writes;
	writes.reserve(draw_data->CmdListsCount * 10);
	for (int i = 0; i < draw_data->CmdListsCount; ++i)
	{
		for (size_t j = 0; j < draw_data->CmdLists[i]->CmdBuffer.size(); ++j)
		{
			writes.emplace_back(WriteDescriptorSet{
				g_FrameDescriptorSets[imageIndex][writes.size()],
				0,
				0,
				DescriptorType::COMBINED_IMAGE_SAMPLER,
				std::vector<DescriptorImageInfo>{
					{g_FontSampler, reinterpret_cast<ImageView *>(draw_data->CmdLists[i]->CmdBuffer[j].TextureId), ImageLayout::SHADER_READ_ONLY_OPTIMAL},
				}});
		}
	}
	g_ShitRendererInitInfo.device->UpdateDescriptorSets(writes, {});

	// Bind Vertex And Index Buffer:
	if (draw_data->TotalVtxCount > 0)
	{
		std::vector<Buffer *> vertexBuffers{rb->VertexBuffer};
		std::vector<uint64_t> vertexOffsets{0};
		command_buffer->BindVertexBuffer(BindVertexBufferInfo{
			0,
			static_cast<uint32_t>(vertexBuffers.size()),
			vertexBuffers.data(),
			vertexOffsets.data()});
		command_buffer->BindIndexBuffer(BindIndexBufferInfo{
			rb->IndexBuffer,
			0,
			sizeof(ImDrawIdx) == 2 ? IndexType::UINT16 : IndexType::UINT32});
	}

	// Setup viewport:
	{
		Viewport viewport{
			0, 0,
			(float)fb_width, (float)fb_height,
			0.f, 1.f};
		command_buffer->SetViewport(SetViewPortInfo{0, 1, &viewport});
	}

	// Setup scale and translation:
	// Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
	{
		//float scale[2];
		//scale[0] = 2.0f / draw_data->DisplaySize.x;
		//scale[1] = 2.0f / draw_data->DisplaySize.y;
		//float translate[2];
		//translate[0] = -1.0f - draw_data->DisplayPos.x * scale[0];
		//translate[1] = -1.0f - draw_data->DisplayPos.y * scale[1];
		//command_buffer->PushConstants(PushConstantInfo{
		//	g_PipelineLayout,
		//	ShaderStageFlagBits::VERTEX_BIT,
		//	0,
		//	0,
		//	sizeof(float) * 2,
		//	scale,
		//});
		//command_buffer->PushConstants(PushConstantInfo{
		//	g_PipelineLayout,
		//	ShaderStageFlagBits::VERTEX_BIT,
		//	0,
		//	sizeof(float) * 2,
		//	sizeof(float) * 2,
		//	translate,
		//});
		//flag = false;

		float pc[4];
		pc[0] = 2.0f / draw_data->DisplaySize.x;
		pc[1] = 2.0f / draw_data->DisplaySize.y;
		pc[2] = -1.0f - draw_data->DisplayPos.x * pc[0];
		pc[3] = -1.0f - draw_data->DisplayPos.y * pc[1];
		command_buffer->PushConstants(PushConstantInfo{
			g_PipelineLayout,
			ShaderStageFlagBits::VERTEX_BIT,
			0,
			0,
			sizeof(float) * 4,
			pc,
		});
	}
}

void ImGui_ImplShitRenderer_RenderDrawData(uint32_t imageIndex, ImDrawData *draw_data, CommandBuffer *command_buffer, Pipeline *pipeline)
{
	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
	int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
	if (fb_width <= 0 || fb_height <= 0)
		return;
	auto v = &g_ShitRendererInitInfo;
	if (pipeline)
		pipeline = g_Pipeline;

	// Allocate array to store enough vertex/index buffers
	ImGui_ImplShitRendererH_WindowRenderBuffers *wrb = &g_MainWindowRenderBuffers;
	if (wrb->FrameRenderBuffers == NULL)
	{
		wrb->Index = 0;
		wrb->Count = v->imageCount;
		wrb->FrameRenderBuffers = (ImGui_ImplShitRendererH_FrameRenderBuffers *)IM_ALLOC(sizeof(ImGui_ImplShitRendererH_FrameRenderBuffers) * wrb->Count);
		memset(wrb->FrameRenderBuffers, 0, sizeof(ImGui_ImplShitRendererH_FrameRenderBuffers) * wrb->Count);
	}
	IM_ASSERT(wrb->Count == v->imageCount);
	wrb->Index = (wrb->Index + 1) % wrb->Count;
	ImGui_ImplShitRendererH_FrameRenderBuffers *rb = &wrb->FrameRenderBuffers[wrb->Index];

	if (draw_data->TotalVtxCount > 0)
	{
		// Create or resize the vertex/index buffers
		size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
		size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

		// Upload vertex/index data into a single contiguous GPU buffer
		if (!rb->VertexBuffer || rb->VertexBufferSize < vertex_size)
			CreateOrResizeBuffer(rb->VertexBuffer, rb->VertexBufferSize, vertex_size, BufferUsageFlagBits::VERTEX_BUFFER_BIT);
		if (!rb->IndexBuffer || rb->IndexBufferSize < index_size)
			CreateOrResizeBuffer(rb->IndexBuffer, rb->IndexBufferSize, index_size, BufferUsageFlagBits::INDEX_BUFFER_BIT);

		ImDrawVert *vtx_dst;
		ImDrawIdx *idx_dst;
		rb->IndexBuffer->MapMemory(0, rb->IndexBufferSize, (void **)&idx_dst);
		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList *cmd_list = draw_data->CmdLists[n];
			memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			idx_dst += cmd_list->IdxBuffer.Size;
		}
		rb->IndexBuffer->FlushMappedMemoryRange(0, rb->IndexBufferSize);
		rb->IndexBuffer->UnMapMemory();

		rb->VertexBuffer->MapMemory(0, rb->VertexBufferSize, (void **)&vtx_dst);
		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList *cmd_list = draw_data->CmdLists[n];
			memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			vtx_dst += cmd_list->VtxBuffer.Size;
		}
		rb->VertexBuffer->FlushMappedMemoryRange(0, rb->VertexBufferSize);
		rb->VertexBuffer->UnMapMemory();
	}

	// Setup desired ShitRenderer state
	ImGui_ImplShitRenderer_SetupRenderState(imageIndex, draw_data, pipeline, command_buffer, rb, fb_width, fb_height);

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = draw_data->DisplayPos;		 // (0,0) unless using multi-viewports
	ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	int global_vtx_offset = 0;
	int global_idx_offset = 0;
	int descriptorIndex = 0;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList *cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++, descriptorIndex++)
		{
			const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback != NULL)
			{
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					ImGui_ImplShitRenderer_SetupRenderState(imageIndex, draw_data, pipeline, command_buffer, rb, fb_width, fb_height);
				else
					pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				// Project scissor/clipping rectangles into framebuffer space
				ImVec4 clip_rect;
				clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
				clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
				clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
				clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

				if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
				{
					// Negative offsets are illegal for vkCmdSetScissor
					if (clip_rect.x < 0.0f)
						clip_rect.x = 0.0f;
					if (clip_rect.y < 0.0f)
						clip_rect.y = 0.0f;

					// Apply scissor/clipping rectangle
					Rect2D scissor;
					scissor.offset.x = (int32_t)(clip_rect.x);
					scissor.offset.y = (int32_t)(clip_rect.y);
					scissor.extent.width = (uint32_t)(clip_rect.z - clip_rect.x);
					scissor.extent.height = (uint32_t)(clip_rect.w - clip_rect.y);
					command_buffer->SetScissor({0, 1, &scissor});

					command_buffer->BindDescriptorSets(BindDescriptorSetsInfo{
						PipelineBindPoint::GRAPHICS,
						g_PipelineLayout,
						0,
						1,
						&g_FrameDescriptorSets[imageIndex][descriptorIndex],
					});
					// Draw
					command_buffer->DrawIndexed({pcmd->ElemCount, 1u, pcmd->IdxOffset + global_idx_offset, static_cast<int32_t>(pcmd->VtxOffset + global_vtx_offset), 0u});
				}
			}
		}
		global_idx_offset += cmd_list->IdxBuffer.Size;
		global_vtx_offset += cmd_list->VtxBuffer.Size;
	}
}
bool ImGui_ImplShitRenderer_CreateFontsTexture(CommandBuffer *command_buffer)
{
	auto v = &g_ShitRendererInitInfo;
	ImGuiIO &io = ImGui::GetIO();

	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	size_t upload_size = width * height * 4 * sizeof(char);

	// Create the Image:
	{
		g_FontImage = v->device->Create(
			ImageCreateInfo{
				{},
				ImageType::TYPE_2D,
				ShitFormat::RGBA8_UNORM,
				{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1u},
				1,
				1,
				SampleCountFlagBits::BIT_1,
				ImageTiling::OPTIMAL,
				ImageUsageFlagBits::SAMPLED_BIT | ImageUsageFlagBits::TRANSFER_DST_BIT,
				MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			},
			nullptr);
	}

	// Create the Image View:
	{
		g_FontView = v->device->Create(
			ImageViewCreateInfo{
				g_FontImage,
				ImageViewType::TYPE_2D,
				ShitFormat::RGBA8_UNORM,
				{},
				{0, 1, 0, 1}});
	}

	// Update the Descriptor Set:
	{
		std::vector<WriteDescriptorSet> writes;
		writes.reserve(g_FrameDescriptorSets.size() * g_FrameDescriptorSets[0].size());
		for (uint32_t i = 0; i < v->imageCount; ++i)
			for (auto &&e : g_FrameDescriptorSets)
			{
				for (auto &&e2 : e)
				{
					writes.emplace_back(WriteDescriptorSet{
						e2,
						0,
						0,
						DescriptorType::COMBINED_IMAGE_SAMPLER,
						std::vector<DescriptorImageInfo>{
							{g_FontSampler, g_FontView, ImageLayout::SHADER_READ_ONLY_OPTIMAL},
						}});
				}
			}
		v->device->UpdateDescriptorSets(writes, {});
	}
	// Create the Upload Buffer:
	{
		g_UploadBuffer = v->device->Create(
			BufferCreateInfo{
				{},
				upload_size,
				BufferUsageFlagBits::TRANSFER_SRC_BIT,
				MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
			nullptr);
	}

	// Upload to Buffer:
	{
		char *map;
		g_UploadBuffer->MapMemory(0, upload_size, (void **)(&map));
		memcpy(map, pixels, upload_size);
		g_UploadBuffer->FlushMappedMemoryRange(0, upload_size);
		g_UploadBuffer->UnMapMemory();
	}

	// Copy to Image:
	{
		ImageMemoryBarrier copy_barrier[1]{{{},
											AccessFlagBits::TRANSFER_WRITE_BIT,
											ImageLayout::UNDEFINED,
											ImageLayout::TRANSFER_DST_OPTIMAL,
											g_FontImage,
											{0, 1, 0, 1}}};
		command_buffer->PipeplineBarrier(
			{PipelineStageFlagBits::HOST_BIT,
			 PipelineStageFlagBits::TRANSFER_BIT,
			 {},
			 0,
			 nullptr,
			 0,
			 nullptr,
			 1,
			 copy_barrier});

		BufferImageCopy region{
			0,
			0,
			0,
			{0, 0, 1},
			{},
			{width, height, 1u}};
		command_buffer->CopyBufferToImage({g_UploadBuffer, g_FontImage, 1, &region});

		ImageMemoryBarrier use_barrier[1] = {
			{AccessFlagBits::TRANSFER_WRITE_BIT,
			 AccessFlagBits::SHADER_READ_BIT,
			 ImageLayout::TRANSFER_DST_OPTIMAL,
			 ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			 g_FontImage,
			 {0, 1, 0, 1}}};
		command_buffer->PipeplineBarrier({PipelineStageFlagBits::TRANSFER_BIT,
										  PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
										  {},
										  0,
										  nullptr,
										  0,
										  nullptr,
										  1,
										  use_barrier});
	}

	// Store our identifier
	io.Fonts->SetTexID((ImTextureID)g_FontView);

	return true;
}

void ImGui_ImplShitRenderer_DestroyFontUploadObjects()
{
	auto v = &g_ShitRendererInitInfo;
	if (g_UploadBuffer)
	{
		v->device->Destroy(g_UploadBuffer);
		g_UploadBuffer = nullptr;
	}
}

// Forward Declarations
bool ImGui_ImplShitRenderer_CreateDeviceObjects();
void ImGui_ImplShitRenderer_DestroyDeviceObjects();

static void ImGui_ImplShitRenderer_CreateShaderModules(Device *device)
{
	auto rendererType = g_ShitRendererInitInfo.rendererVersion & RendererVersion::TypeBitmask;

	switch (rendererType)
	{
	case RendererVersion::GL:
		if (!g_ShaderModuleVert)
		{
			g_ShaderModuleVert = device->Create(ShaderCreateInfo{sizeof(__glsl_shader_vert_spv_gl), __glsl_shader_vert_spv_gl});
		}
		if (!g_ShaderModuleFrag)
		{
			g_ShaderModuleFrag = device->Create(ShaderCreateInfo{sizeof(__glsl_shader_frag_spv_gl), __glsl_shader_frag_spv_gl});
		}
		return;
	case RendererVersion::VULKAN:
		if (!g_ShaderModuleVert)
		{
			g_ShaderModuleVert = device->Create(ShaderCreateInfo{sizeof(__glsl_shader_vert_spv_vk), __glsl_shader_vert_spv_vk});
		}
		if (!g_ShaderModuleFrag)
		{
			g_ShaderModuleFrag = device->Create(ShaderCreateInfo{sizeof(__glsl_shader_frag_spv_vk), __glsl_shader_frag_spv_vk});
		}
		return;
	}
}
static void ImGui_ImplShitRenderer_CreateFontSampler(Device *device)
{
	if (g_FontSampler)
		return;
	g_FontSampler = device->Create(
		SamplerCreateInfo{
			Filter::LINEAR,
			Filter::LINEAR,
			SamplerMipmapMode::LINEAR,
			SamplerWrapMode::REPEAT,
			SamplerWrapMode::REPEAT,
			SamplerWrapMode::REPEAT,
			0,
			false,
			1.f,
			false,
			{},
			-100.f,
			100.f,
		});
}
static void ImGui_ImplShitRenderer_CreateDescriptorSetLayout(Device *device)
{
	if (g_DescriptorSetLayout)
		return;
	ImGui_ImplShitRenderer_CreateFontSampler(device);
	std::vector<DescriptorSetLayoutBinding> bindings{
		{0, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT, {g_FontSampler}},
	};
	g_DescriptorSetLayout = device->Create(DescriptorSetLayoutCreateInfo{bindings});
}
static void ImGui_ImplShitRenderer_CreatePipelineLayout(Device *device)
{
	if (g_PipelineLayout)
		return;
	// Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
	ImGui_ImplShitRenderer_CreateDescriptorSetLayout(device);
	std::vector<PushConstantRange> pushConstantRanges{
		{ShaderStageFlagBits::VERTEX_BIT, 0, 0, sizeof(float) * 4},
	};
	g_PipelineLayout = device->Create(PipelineLayoutCreateInfo{
		{g_DescriptorSetLayout},
		pushConstantRanges});
}

static void ImGui_ImplShitRenderer_CreatePipeline(Device *device, RenderPass *renderPass, SampleCountFlagBits MSAASamples, Pipeline *&pipeline, uint32_t subpass)
{
	ImGui_ImplShitRenderer_CreateShaderModules(device);

	std::vector<PipelineShaderStageCreateInfo> stages{
		{
			ShaderStageFlagBits::VERTEX_BIT,
			g_ShaderModuleVert,
			"main",
		},
		{
			ShaderStageFlagBits::FRAGMENT_BIT,
			g_ShaderModuleFrag,
			"main",
		},
	};

	std::vector<VertexBindingDescription> vertexBindingInfo{
		{0, sizeof(ImDrawVert), 0}};

	std::vector<VertexAttributeDescription> vertexAttributeInfo{
		{0, vertexBindingInfo[0].binding, ShitFormat::RG32_SFLOAT, IM_OFFSETOF(ImDrawVert, pos)},
		{1, vertexBindingInfo[0].binding, ShitFormat::RG32_SFLOAT, IM_OFFSETOF(ImDrawVert, uv)},
		{2, vertexBindingInfo[0].binding, ShitFormat::RGBA8_UNORM, IM_OFFSETOF(ImDrawVert, col)},
	};

	VertexInputStateCreateInfo vertexInputInfo{
		vertexBindingInfo,
		vertexAttributeInfo};

	PipelineInputAssemblyStateCreateInfo assemblyInfo{PrimitiveTopology::TRIANGLE_LIST};

	PipelineViewportStateCreateInfo viewPortInfo{{{}}, {{}}};

	PipelineRasterizationStateCreateInfo rasterizationInfo{};
	rasterizationInfo.polygonMode = PolygonMode::FILL;
	rasterizationInfo.cullMode = CullMode::NONE;
	rasterizationInfo.frontFace = FrontFace::COUNTER_CLOCKWISE;
	rasterizationInfo.lineWidth = 1.f;

	PipelineMultisampleStateCreateInfo multisampleInfo{MSAASamples};

	PipelineColorBlendStateCreateInfo blendInfo{
		false,
		{},
		{
			PipelineColorBlendAttachmentState{
				true,
				BlendFactor::SRC_ALPHA,
				BlendFactor::ONE_MINUS_SRC_ALPHA,
				BlendOp::ADD,
				BlendFactor::ONE,
				BlendFactor::ONE_MINUS_SRC_ALPHA,
				BlendOp::ADD,
				ColorComponentFlagBits::R_BIT |
					ColorComponentFlagBits::G_BIT |
					ColorComponentFlagBits::B_BIT |
					ColorComponentFlagBits::A_BIT},
		}};

	PipelineDepthStencilStateCreateInfo depthInfo{};
	PipelineDynamicStateCreateInfo dynamicInfo{{DynamicState::VIEWPORT, DynamicState::SCISSOR}};

	ImGui_ImplShitRenderer_CreatePipelineLayout(device);

	pipeline = device->Create(GraphicsPipelineCreateInfo{
		stages,
		vertexInputInfo,
		assemblyInfo,
		viewPortInfo,
		{},
		rasterizationInfo,
		multisampleInfo,
		depthInfo,
		blendInfo,
		dynamicInfo,
		g_PipelineLayout,
		renderPass,
		subpass});
}

bool ImGui_ImplShitRenderer_CreateDeviceObjects()
{
	auto v = &g_ShitRendererInitInfo;

	if (!g_FontSampler)
	{
		g_FontSampler = v->device->Create(SamplerCreateInfo{
			Filter::LINEAR,
			Filter::LINEAR,
			SamplerMipmapMode::LINEAR,
			SamplerWrapMode::REPEAT,
			SamplerWrapMode::REPEAT,
			SamplerWrapMode::REPEAT,
			0,
			false,
			1.f,
			false,
			{},
			-100.f,
			100.f,
		});
	}
	if (!g_DescriptorSetLayout)
	{
		g_DescriptorSetLayout = v->device->Create(
			DescriptorSetLayoutCreateInfo{
				{
					{0,
					 DescriptorType::COMBINED_IMAGE_SAMPLER,
					 1,
					 ShaderStageFlagBits::FRAGMENT_BIT},
				}});
	}
	{
		g_FrameDescriptorSets.resize(v->imageCount);
		std::vector<DescriptorSetLayout *> setLayouts(100, g_DescriptorSetLayout);
		for (int i = 0; i < v->imageCount; ++i)
		{
			g_DescriptorPool->Allocate(
				DescriptorSetAllocateInfo{setLayouts},
				g_FrameDescriptorSets[i]);
		}
	}

	ImGui_ImplShitRenderer_CreatePipelineLayout(v->device);
	ImGui_ImplShitRenderer_CreatePipeline(v->device, g_RenderPass, v->msaaSamples, g_Pipeline, g_Subpass);
	return true;
}

void ImGui_ImplShitRendererH_CrreateDescriptorPool()
{
	std::vector<DescriptorPoolSize> poolSizes{
		{DescriptorType::UNIFORM_BUFFER, 1000},
		{DescriptorType::COMBINED_IMAGE_SAMPLER, 1000},
	};
	g_DescriptorPool = g_ShitRendererInitInfo.device->Create(DescriptorPoolCreateInfo{1000, poolSizes});
}

bool ImGui_ImplShitRenderer_Init(ImGui_ImplShitRenderer_InitInfo *info, RenderPass *render_pass, uint32_t subpass)
{
	IM_ASSERT(g_FunctionsLoaded && "Need to call ImGui_ImplShitRenderer_LoadFunctions() if IMGUI_IMPL_VULKAN_NO_PROTOTYPES or VK_NO_PROTOTYPES are set!");

	// Setup backend capabilities flags
	ImGuiIO &io = ImGui::GetIO();
	io.BackendRendererName = "imgui_impl_vulkan";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

	IM_ASSERT(info->device);
	//IM_ASSERT(info->minImageCount >= 2);
	//IM_ASSERT(info->imageCount >= info->minImageCount);
	IM_ASSERT(render_pass);
	g_ShitRendererInitInfo = *info;
	g_RenderPass = render_pass;
	g_Subpass = subpass;

	ImGui_ImplShitRendererH_CrreateDescriptorPool();
	ImGui_ImplShitRenderer_CreateDeviceObjects();
	return true;
}
void ImGui_ImplShitRenderer_Shutdown()
{
}
void ImGui_ImplShitRenderer_NewFrame()
{
}
//void ImGui_ImplShitRenderer_SetMinImageCount(uint32_t min_image_count)
//{
//    IM_ASSERT(min_image_count >= 2);
//    if (g_ShitRendererInitInfo.minImageCount== min_image_count)
//        return;
//
//    auto v = &g_ShitRendererInitInfo;
//	//ImGui_ImplShitRendererH_DestroyWindowRenderBuffers(v->device, &g_MainWindowRenderBuffers);
//	g_ShitRendererInitInfo.minImageCount = min_image_count;
//}
void ImGui_ImplShitRenderer_RecordCommandBuffer(uint32_t imageIndex, RenderPass *renderPass, uint32_t subpass, Framebuffer *pFrameBuffer)
{
	auto cmdBuffer = g_ShitRendererInitInfo.secondaryCommandBuffers[imageIndex];
	cmdBuffer->Begin(CommandBufferBeginInfo{
		CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT,
		{renderPass, subpass, pFrameBuffer}});

	ImDrawData *draw_data = ImGui::GetDrawData();
	ImGui_ImplShitRenderer_RenderDrawData(imageIndex, draw_data, cmdBuffer, g_Pipeline);
	cmdBuffer->End();
}
// Create or resize window
//void ImGui_ImplShitRendererH_CreateOrResizeWindow(Instance* instance, PhysicalDevice physical_device, VkDevice device, ImGui_ImplShitRendererH_Window* wd, uint32_t queue_family, const VkAllocationCallbacks* allocator, int width, int height, uint32_t min_image_count)
//{
//    IM_ASSERT(g_FunctionsLoaded && "Need to call ImGui_ImplShitRenderer_LoadFunctions() if IMGUI_IMPL_VULKAN_NO_PROTOTYPES or VK_NO_PROTOTYPES are set!");
//    (void)instance;
//    ImGui_ImplShitRendererH_CreateWindowSwapChain(physical_device, device, wd, allocator, width, height, min_image_count);
//    ImGui_ImplShitRendererH_CreateWindowCommandBuffers(physical_device, device, wd, queue_family, allocator);
//}

//void ImGui_ImplShitRendererH_DestroyWindow(VkInstance instance, VkDevice device, ImGui_ImplShitRendererH_Window* wd, const VkAllocationCallbacks* allocator)
//{
//	 vkDeviceWaitIdle(device); // FIXME: We could wait on the Queue if we had the queue in wd-> (otherwise ShitRendererH functions can't use globals)
//    //vkQueueWaitIdle(g_Queue);
//
//    for (uint32_t i = 0; i < wd->ImageCount; i++)
//    {
//        ImGui_ImplShitRendererH_DestroyFrame(device, &wd->Frames[i], allocator);
//        ImGui_ImplShitRendererH_DestroyFrameSemaphores(device, &wd->FrameSemaphores[i], allocator);
//    }
//    IM_FREE(wd->Frames);
//    IM_FREE(wd->FrameSemaphores);
//    wd->Frames = NULL;
//    wd->FrameSemaphores = NULL;
//    vkDestroyPipeline(device, wd->Pipeline, allocator);
//    vkDestroyRenderPass(device, wd->RenderPass, allocator);
//    vkDestroySwapchainKHR(device, wd->Swapchain, allocator);
//    vkDestroySurfaceKHR(instance, wd->Surface, allocator);
//
//    *wd = ImGui_ImplShitRendererH_Window();
//}

//void ImGui_ImplShitRendererH_DestroyFrame(VkDevice device, ImGui_ImplShitRendererH_Frame* fd, const VkAllocationCallbacks* allocator)
//{
//    vkDestroyFence(device, fd->Fence, allocator);
//    vkFreeCommandBuffers(device, fd->CommandPool, 1, &fd->CommandBuffer);
//    vkDestroyCommandPool(device, fd->CommandPool, allocator);
//    fd->Fence = VK_NULL_HANDLE;
//    fd->CommandBuffer = VK_NULL_HANDLE;
//    fd->CommandPool = VK_NULL_HANDLE;
//
//    vkDestroyImageView(device, fd->BackbufferView, allocator);
//    vkDestroyFramebuffer(device, fd->Framebuffer, allocator);
//}
//
//void ImGui_ImplShitRendererH_DestroyFrameSemaphores(VkDevice device, ImGui_ImplShitRendererH_FrameSemaphores* fsd, const VkAllocationCallbacks* allocator)
//{
//    vkDestroySemaphore(device, fsd->ImageAcquiredSemaphore, allocator);
//    vkDestroySemaphore(device, fsd->RenderCompleteSemaphore, allocator);
//    fsd->ImageAcquiredSemaphore = fsd->RenderCompleteSemaphore = VK_NULL_HANDLE;
//}
//
//void ImGui_ImplShitRendererH_DestroyFrameRenderBuffers(VkDevice device, ImGui_ImplShitRendererH_FrameRenderBuffers* buffers, const VkAllocationCallbacks* allocator)
//{
//    if (buffers->VertexBuffer) { vkDestroyBuffer(device, buffers->VertexBuffer, allocator); buffers->VertexBuffer = VK_NULL_HANDLE; }
//    if (buffers->VertexBufferMemory) { vkFreeMemory(device, buffers->VertexBufferMemory, allocator); buffers->VertexBufferMemory = VK_NULL_HANDLE; }
//    if (buffers->IndexBuffer) { vkDestroyBuffer(device, buffers->IndexBuffer, allocator); buffers->IndexBuffer = VK_NULL_HANDLE; }
//    if (buffers->IndexBufferMemory) { vkFreeMemory(device, buffers->IndexBufferMemory, allocator); buffers->IndexBufferMemory = VK_NULL_HANDLE; }
//    buffers->VertexBufferSize = 0;
//    buffers->IndexBufferSize = 0;
//}
//
//void ImGui_ImplShitRendererH_DestroyWindowRenderBuffers(VkDevice device, ImGui_ImplShitRendererH_WindowRenderBuffers* buffers, const VkAllocationCallbacks* allocator)
//{
//    for (uint32_t n = 0; n < buffers->Count; n++)
//        ImGui_ImplShitRendererH_DestroyFrameRenderBuffers(device, &buffers->FrameRenderBuffers[n], allocator);
//    IM_FREE(buffers->FrameRenderBuffers);
//    buffers->FrameRenderBuffers = NULL;
//    buffers->Index = 0;
//    buffers->Count = 0;
//}
