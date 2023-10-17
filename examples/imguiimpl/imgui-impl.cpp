#include "imgui-impl.hpp"
#include <stdio.h>

struct ImGui_ImplShitRendererH_FrameRenderBuffers
{
	uint64_t VertexBufferSize;
	uint64_t IndexBufferSize;
	Shit::Buffer *VertexBuffer;
	Shit::Buffer *IndexBuffer;
};

struct ImGui_ImplShitRendererH_WindowRenderBuffers
{
	uint32_t Index;
	uint32_t Count;
	ImGui_ImplShitRendererH_FrameRenderBuffers *FrameRenderBuffers;
};

static ImGui_ImplShitRenderer_InitInfo g_ShitRendererInitInfo = {};

static Shit::RenderPass *g_RenderPass;
static uint32_t g_Subpass;

static std::vector<Shit::Framebuffer *> g_Framebuffers;

static uint64_t g_BufferMemoryAlignment = 256;
static Shit::DescriptorSetLayout *g_DescriptorSetLayout;
static Shit::PipelineLayout *g_PipelineLayout;

static std::vector<std::vector<Shit::DescriptorSet *>> g_FrameDescriptorSets;

static Shit::Pipeline *g_Pipeline;
static Shit::Shader *g_ShaderModuleVert;
static Shit::Shader *g_ShaderModuleFrag;

static bool g_FunctionsLoaded = true;

static Shit::Sampler *g_FontSampler;
static Shit::Image *g_FontImage;
static Shit::ImageView *g_FontView;
static Shit::Buffer *g_UploadBuffer;

static Shit::DescriptorPool *g_DescriptorPool;

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

static void CreateOrResizeBuffer(Shit::Buffer *&buffer, uint64_t &p_buffer_size, size_t new_size, Shit::BufferUsageFlagBits usage)
{
	if (buffer)
	{
		buffer->Resize(new_size);
	}
	else
	{
		auto v = &g_ShitRendererInitInfo;
		v->device->Destroy(buffer);
		buffer = v->device->Create(Shit::BufferCreateInfo{
									   {},
									   new_size,
									   usage,
									   Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
								   nullptr);
	}
	p_buffer_size = buffer->GetMemorySize();
}
static void ImGui_ImplShitRenderer_SetupRenderState(uint32_t frameIndex, ImDrawData *draw_data, Shit::Pipeline *pipeline, Shit::CommandBuffer *command_buffer, ImGui_ImplShitRendererH_FrameRenderBuffers *rb, int fb_width, int fb_height)
{
	command_buffer->BindPipeline(Shit::BindPipelineInfo{
		Shit::PipelineBindPoint::GRAPHICS,
		pipeline});

	std::vector<Shit::WriteDescriptorSet> writes;
	std::vector<Shit::DescriptorImageInfo> imageInfos;
	for (int i = 0; i < draw_data->CmdListsCount; ++i)
	{
		for (int j = 0; j < draw_data->CmdLists[i]->CmdBuffer.size(); ++j)
		{
			imageInfos.emplace_back(
				g_FontSampler,
				reinterpret_cast<Shit::ImageView *>(draw_data->CmdLists[i]->CmdBuffer[j].TextureId),
				Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
			writes.emplace_back(Shit::WriteDescriptorSet{
				g_FrameDescriptorSets[frameIndex][writes.size()],
				0,
				0,
				1,
				Shit::DescriptorType::COMBINED_IMAGE_SAMPLER});
		}
	}
	for (size_t i = 0; i < writes.size(); ++i)
		writes[i].pImageInfo = &imageInfos[i];
	g_ShitRendererInitInfo.device->UpdateDescriptorSets(writes, {});

	// Bind Vertex And Index Buffer:
	if (draw_data->TotalVtxCount > 0)
	{
		Shit::Buffer *vertexBuffers[]{rb->VertexBuffer};
		uint64_t vertexOffsets[]{0};
		command_buffer->BindVertexBuffers(Shit::BindVertexBuffersInfo{
			0,
			std::size(vertexBuffers),
			vertexBuffers,
			vertexOffsets});
		command_buffer->BindIndexBuffer(Shit::BindIndexBufferInfo{
			rb->IndexBuffer,
			0,
			sizeof(ImDrawIdx) == 2 ? Shit::IndexType::UINT16 : Shit::IndexType::UINT32});
	}

	// Setup viewport:
	{
		Shit::Viewport viewport{
			0, 0,
			(float)fb_width, (float)fb_height,
			0.f, 1.f};
		command_buffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
	}

	// Setup scale and translation:
	// Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
	{
		// float scale[2];
		// scale[0] = 2.0f / draw_data->DisplaySize.x;
		// scale[1] = 2.0f / draw_data->DisplaySize.y;
		// float translate[2];
		// translate[0] = -1.0f - draw_data->DisplayPos.x * scale[0];
		// translate[1] = -1.0f - draw_data->DisplayPos.y * scale[1];
		// command_buffer->PushConstants(PushConstantInfo{
		//	g_PipelineLayout,
		//	Shit::ShaderStageFlagBits::VERTEX_BIT,
		//	0,
		//	0,
		//	sizeof(float) * 2,
		//	scale,
		// });
		// command_buffer->PushConstants(PushConstantInfo{
		//	g_PipelineLayout,
		//	Shit::ShaderStageFlagBits::VERTEX_BIT,
		//	0,
		//	sizeof(float) * 2,
		//	sizeof(float) * 2,
		//	translate,
		// });
		// flag = false;

		float pc[4];
		pc[0] = 2.0f / draw_data->DisplaySize.x;
		pc[1] = -2.0f / draw_data->DisplaySize.y;
		pc[2] = -1.0f - draw_data->DisplayPos.x * pc[0];
		pc[3] = 1.0f - draw_data->DisplayPos.y * pc[1];
		command_buffer->PushConstants(Shit::PushConstantInfo{
			g_PipelineLayout,
			Shit::ShaderStageFlagBits::VERTEX_BIT,
			0,
			sizeof(float) * 4,
			pc,
		});
	}
}

void ImGui_ImplShitRenderer_RenderDrawData(uint32_t frameIndex, ImDrawData *draw_data, Shit::CommandBuffer *command_buffer, Shit::Pipeline *pipeline)
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
		wrb->Count = v->images.size();
		wrb->FrameRenderBuffers = (ImGui_ImplShitRendererH_FrameRenderBuffers *)IM_ALLOC(sizeof(ImGui_ImplShitRendererH_FrameRenderBuffers) * wrb->Count);
		memset(wrb->FrameRenderBuffers, 0, sizeof(ImGui_ImplShitRendererH_FrameRenderBuffers) * wrb->Count);
	}
	IM_ASSERT(wrb->Count == v->images.size());
	wrb->Index = (wrb->Index + 1) % wrb->Count;
	ImGui_ImplShitRendererH_FrameRenderBuffers *rb = &wrb->FrameRenderBuffers[wrb->Index];

	if (draw_data->TotalVtxCount > 0)
	{
		// Create or resize the vertex/index buffers
		size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
		size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

		// Upload vertex/index data into a single contiguous GPU buffer
		if (!rb->VertexBuffer || rb->VertexBuffer->GetCreateInfoPtr()->size < vertex_size)
			CreateOrResizeBuffer(rb->VertexBuffer, rb->VertexBufferSize, vertex_size, Shit::BufferUsageFlagBits::VERTEX_BUFFER_BIT);
		if (!rb->IndexBuffer || rb->IndexBuffer->GetCreateInfoPtr()->size < index_size)
			CreateOrResizeBuffer(rb->IndexBuffer, rb->IndexBufferSize, index_size, Shit::BufferUsageFlagBits::INDEX_BUFFER_BIT);

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
	ImGui_ImplShitRenderer_SetupRenderState(frameIndex, draw_data, pipeline, command_buffer, rb, fb_width, fb_height);

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
					ImGui_ImplShitRenderer_SetupRenderState(frameIndex, draw_data, pipeline, command_buffer, rb, fb_width, fb_height);
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
					Shit::Rect2D scissor;
					scissor.offset.x = (int32_t)(clip_rect.x);
					scissor.offset.y = (int32_t)(clip_rect.y);
					scissor.extent.width = (uint32_t)(clip_rect.z - clip_rect.x);
					scissor.extent.height = (uint32_t)(clip_rect.w - clip_rect.y);
					command_buffer->SetScissor({0, 1, &scissor});

					command_buffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
						Shit::PipelineBindPoint::GRAPHICS,
						g_PipelineLayout,
						0,
						1,
						&g_FrameDescriptorSets[frameIndex][descriptorIndex],
					});
					// Draw
					command_buffer->DrawIndexed({pcmd->ElemCount,
												 1u,
												 pcmd->IdxOffset + global_idx_offset,
												 static_cast<int32_t>(pcmd->VtxOffset + global_vtx_offset),
												 0u});
				}
			}
		}
		global_idx_offset += cmd_list->IdxBuffer.Size;
		global_vtx_offset += cmd_list->VtxBuffer.Size;
	}
}
bool ImGui_ImplShitRenderer_CreateFontsTexture(Shit::CommandBuffer *command_buffer)
{
	auto v = &g_ShitRendererInitInfo;
	ImGuiIO &io = ImGui::GetIO();

	unsigned char *pixels;
	int width, height, bytes_per_pixel;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);
	size_t upload_size = width * height * bytes_per_pixel;

	// Create the Image:
	{
		g_FontImage = v->device->Create(
			Shit::ImageCreateInfo{
				{},
				Shit::ImageType::TYPE_2D,
				Shit::Format::R8G8B8A8_UNORM,
				{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1u},
				1,
				1,
				Shit::SampleCountFlagBits::BIT_1,
				Shit::ImageTiling::OPTIMAL,
				Shit::ImageUsageFlagBits::SAMPLED_BIT | Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
				Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			});
	}

	// Create the Shit::Image View:
	{
		g_FontView = v->device->Create(
			Shit::ImageViewCreateInfo{
				g_FontImage,
				Shit::ImageViewType::TYPE_2D,
				Shit::Format::R8G8B8A8_UNORM,
				{},
				{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}});
	}

	// Update the Descriptor Set:
	{
		std::vector<Shit::DescriptorImageInfo> imageInfos;
		std::vector<Shit::WriteDescriptorSet> writes;
		for (uint32_t i = 0, l = v->images.size(); i < l; ++i)
		{
			for (auto &&e : g_FrameDescriptorSets)
			{
				for (auto &&e2 : e)
				{
					imageInfos.emplace_back(
						g_FontSampler,
						g_FontView,
						Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
					writes.emplace_back(Shit::WriteDescriptorSet{
						e2,
						0,
						0,
						1,
						Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
					});
				}
			}
		}
		for (size_t i = 0; i < writes.size(); ++i)
		{
			writes[i].pImageInfo = &imageInfos[i];
		}
		v->device->UpdateDescriptorSets(writes, {});
	}
	// Create the Upload Buffer:
	{
		g_UploadBuffer = v->device->Create(
			Shit::BufferCreateInfo{
				{},
				upload_size,
				Shit::BufferUsageFlagBits::TRANSFER_SRC_BIT,
				Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
			nullptr);
	}

	// Upload to Buffer:
	{
		void *map;
		g_UploadBuffer->MapMemory(0, upload_size, &map);
		memcpy(map, pixels, upload_size);
		g_UploadBuffer->FlushMappedMemoryRange(0, upload_size);
		g_UploadBuffer->UnMapMemory();
	}

	// Copy to Image:
	{
		Shit::ImageMemoryBarrier copy_barriers[]{{{},
												  Shit::AccessFlagBits::TRANSFER_WRITE_BIT,
												  Shit::ImageLayout::UNDEFINED,
												  Shit::ImageLayout::TRANSFER_DST_OPTIMAL,
												  ST_QUEUE_FAMILY_IGNORED,
												  ST_QUEUE_FAMILY_IGNORED,
												  g_FontImage,
												  {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}}};
		command_buffer->PipelineBarrier(
			{Shit::PipelineStageFlagBits::TRANSFER_BIT,
			 Shit::PipelineStageFlagBits::TRANSFER_BIT,
			 {},
			 0,
			 nullptr,
			 0,
			 nullptr,
			 1,
			 copy_barriers});

		Shit::BufferImageCopy region{
			0,
			0,
			0,
			{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 0, 1},
			{},
			{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1u}};
		command_buffer->CopyBufferToImage({g_UploadBuffer, g_FontImage, 1, &region});

		Shit::ImageMemoryBarrier use_barriers[] = {
			{Shit::AccessFlagBits::TRANSFER_WRITE_BIT,
			 Shit::AccessFlagBits::TRANSFER_READ_BIT,
			 Shit::ImageLayout::TRANSFER_DST_OPTIMAL,
			 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			 ST_QUEUE_FAMILY_IGNORED,
			 ST_QUEUE_FAMILY_IGNORED,
			 g_FontImage,
			 {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}}};
		command_buffer->PipelineBarrier({Shit::PipelineStageFlagBits::TRANSFER_BIT,
										 Shit::PipelineStageFlagBits::TRANSFER_BIT,
										 {},
										 0,
										 nullptr,
										 0,
										 nullptr,
										 1,
										 use_barriers});
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

static void ImGui_ImplShitRenderer_CreateShaderModules(Shit::Device *device)
{
	auto rendererType = g_ShitRendererInitInfo.rendererVersion & Shit::RendererVersion::TypeBitmask;
	std::vector<Shit::ShadingLanguage> languages;
	device->GetSupportedShaderSourceLanguages(languages);
	Shit::ShadingLanguage shaderSourceLanguage{Shit::ShadingLanguage::SPIRV};
	std::vector<Shit::ShadingLanguage> candidates{Shit::ShadingLanguage::SPIRV, Shit::ShadingLanguage::GLSL};
	for (auto &&e : candidates)
	{
		auto it = std::find(languages.begin(), languages.end(), e);
		if (it != languages.end())
		{
			shaderSourceLanguage = e;
			break;
		}
	}

	switch (rendererType)
	{
	case Shit::RendererVersion::GL:
		if (!g_ShaderModuleVert)
		{
			g_ShaderModuleVert = device->Create(Shit::ShaderCreateInfo{shaderSourceLanguage, sizeof(__glsl_shader_vert_spv_gl), (char *)__glsl_shader_vert_spv_gl});
		}
		if (!g_ShaderModuleFrag)
		{
			g_ShaderModuleFrag = device->Create(Shit::ShaderCreateInfo{shaderSourceLanguage, sizeof(__glsl_shader_frag_spv_gl), (char *)__glsl_shader_frag_spv_gl});
		}
		return;
	case Shit::RendererVersion::VULKAN:
		if (!g_ShaderModuleVert)
		{
			g_ShaderModuleVert = device->Create(Shit::ShaderCreateInfo{shaderSourceLanguage, sizeof(__glsl_shader_vert_spv_vk), (char *)__glsl_shader_vert_spv_vk});
		}
		if (!g_ShaderModuleFrag)
		{
			g_ShaderModuleFrag = device->Create(Shit::ShaderCreateInfo{shaderSourceLanguage, sizeof(__glsl_shader_frag_spv_vk), (char *)__glsl_shader_frag_spv_vk});
		}
		return;
	default:
		ST_THROW("wrong renderer type");
		return;
	}
}
static void ImGui_ImplShitRenderer_CreateFontSampler(Shit::Device *device)
{
	if (g_FontSampler)
		return;
	g_FontSampler = device->Create(
		Shit::SamplerCreateInfo{
			Shit::Filter::LINEAR,
			Shit::Filter::LINEAR,
			Shit::SamplerMipmapMode::LINEAR,
			Shit::SamplerWrapMode::REPEAT,
			Shit::SamplerWrapMode::REPEAT,
			Shit::SamplerWrapMode::REPEAT,
			0,
			false,
			1.f,
			false,
			{},
			-100.f,
			100.f,
		});
}
static void ImGui_ImplShitRenderer_CreateDescriptorSetLayout(Shit::Device *device)
{
	if (g_DescriptorSetLayout)
		return;
	ImGui_ImplShitRenderer_CreateFontSampler(device);
	Shit::DescriptorSetLayoutBinding bindings[]{
		{0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT, 1, &g_FontSampler},
	};
	g_DescriptorSetLayout = device->Create(Shit::DescriptorSetLayoutCreateInfo{sizeof(bindings) / sizeof(bindings[0]), bindings});
}
static void ImGui_ImplShitRenderer_CreatePipelineLayout(Shit::Device *device)
{
	if (g_PipelineLayout)
		return;
	// Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
	ImGui_ImplShitRenderer_CreateDescriptorSetLayout(device);
	Shit::PushConstantRange pushConstantRanges[]{
		{Shit::ShaderStageFlagBits::VERTEX_BIT, 0, 0, sizeof(float) * 4},
	};
	g_PipelineLayout = device->Create(Shit::PipelineLayoutCreateInfo{
		1, &g_DescriptorSetLayout,
		1, pushConstantRanges});
}

static void ImGui_ImplShitRenderer_CreatePipeline(
	Shit::Device *device,
	Shit::RenderPass *renderPass,
	// Shit::SampleCountFlagBits MSAASamples,
	Shit::Pipeline *&pipeline,
	uint32_t subpass)
{
	ImGui_ImplShitRenderer_CreateShaderModules(device);

	Shit::PipelineShaderStageCreateInfo stages[]{
		{
			Shit::ShaderStageFlagBits::VERTEX_BIT,
			g_ShaderModuleVert,
			"main",
		},
		{
			Shit::ShaderStageFlagBits::FRAGMENT_BIT,
			g_ShaderModuleFrag,
			"main",
		},
	};

	Shit::VertexBindingDescription vertexBindingInfo[]{
		{0, sizeof(ImDrawVert), 0}};

	Shit::VertexAttributeDescription vertexAttributeInfo[]{
		{0, vertexBindingInfo[0].binding, Shit::Format::R32G32_SFLOAT, IM_OFFSETOF(ImDrawVert, pos)},
		{1, vertexBindingInfo[0].binding, Shit::Format::R32G32_SFLOAT, IM_OFFSETOF(ImDrawVert, uv)},
		{2, vertexBindingInfo[0].binding, Shit::Format::R8G8B8A8_UNORM, IM_OFFSETOF(ImDrawVert, col)},
	};

	Shit::VertexInputStateCreateInfo vertexInputInfo{
		(uint32_t)std::size(vertexBindingInfo),
		vertexBindingInfo,
		(uint32_t)std::size(vertexAttributeInfo),
		vertexAttributeInfo};

	Shit::PipelineInputAssemblyStateCreateInfo assemblyInfo{Shit::PrimitiveTopology::TRIANGLE_LIST};

	Shit::PipelineViewportStateCreateInfo viewPortInfo{};

	Shit::PipelineRasterizationStateCreateInfo rasterizationInfo{};
	rasterizationInfo.polygonMode = Shit::PolygonMode::FILL;
	rasterizationInfo.cullMode = Shit::CullMode::FRONT;
	rasterizationInfo.frontFace = Shit::FrontFace::COUNTER_CLOCKWISE;
	rasterizationInfo.lineWidth = 1.f;

	Shit::PipelineMultisampleStateCreateInfo multisampleInfo{Shit::SampleCountFlagBits::BIT_1};
	Shit::PipelineColorBlendAttachmentState blendAttachments[]{
		{true,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::ColorComponentFlagBits::R_BIT |
			 Shit::ColorComponentFlagBits::G_BIT |
			 Shit::ColorComponentFlagBits::B_BIT |
			 Shit::ColorComponentFlagBits::A_BIT},
	};
	Shit::PipelineColorBlendStateCreateInfo blendInfo{
		false,
		{},
		(uint32_t)std::size(blendAttachments),
		blendAttachments};

	Shit::PipelineDepthStencilStateCreateInfo depthInfo{};
	Shit::DynamicState states[]{Shit::DynamicState::VIEWPORT, Shit::DynamicState::SCISSOR};
	Shit::PipelineDynamicStateCreateInfo dynamicInfo{(uint32_t)std::size(states), states};

	ImGui_ImplShitRenderer_CreatePipelineLayout(device);

	pipeline = device->Create(Shit::GraphicsPipelineCreateInfo{
		(uint32_t)std::size(stages),
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
		g_FontSampler = v->device->Create(Shit::SamplerCreateInfo{
			Shit::Filter::LINEAR,
			Shit::Filter::LINEAR,
			Shit::SamplerMipmapMode::LINEAR,
			Shit::SamplerWrapMode::REPEAT,
			Shit::SamplerWrapMode::REPEAT,
			Shit::SamplerWrapMode::REPEAT,
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
		Shit::DescriptorSetLayoutBinding bindings[]{
			{0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
		};
		g_DescriptorSetLayout = v->device->Create(
			Shit::DescriptorSetLayoutCreateInfo{(uint32_t)std::size(bindings), bindings});
	}
	{
		auto imageCount = v->images.size();
		g_FrameDescriptorSets.resize(imageCount);
		std::vector<Shit::DescriptorSetLayout *> setLayouts(100, g_DescriptorSetLayout);
		for (uint32_t i = 0; i < imageCount; ++i)
		{
			g_DescriptorPool->Allocate(
				Shit::DescriptorSetAllocateInfo{(uint32_t)setLayouts.size(), setLayouts.data()},
				g_FrameDescriptorSets[i]);
		}
	}

	ImGui_ImplShitRenderer_CreatePipelineLayout(v->device);
	ImGui_ImplShitRenderer_CreatePipeline(v->device, g_RenderPass, g_Pipeline, g_Subpass);
	return true;
}

void ImGui_ImplShitRenderer_CreateDescriptorPool()
{
	std::vector<Shit::DescriptorPoolSize> poolSizes{
		{Shit::DescriptorType::UNIFORM_BUFFER, 1000},
		{Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1000},
	};
	g_DescriptorPool = g_ShitRendererInitInfo.device->Create(Shit::DescriptorPoolCreateInfo{1000, (uint32_t)poolSizes.size(), poolSizes.data()});
}
static std::vector<Shit::ImageView *> imageViews;
void ImGui_ImplShitRenderer_CreateFramebuffers(
	Shit::Device *device,
	Shit::RenderPass *renderPass,
	const std::vector<Shit::Image *> &images,
	std::vector<Shit::Framebuffer *> &framebuffers)
{
	// create imageViews
	auto imageCount = images.size();
	imageViews.resize(imageCount);

	uint32_t i = 0;
	for (i = 0; i < imageCount; ++i)
	{
		imageViews[i] = device->Create(Shit::ImageViewCreateInfo{
			images[i],
			Shit::ImageViewType::TYPE_2D,
			images[i]->GetCreateInfoPtr()->format,
			{},
			{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}});
	}

	framebuffers.resize(imageCount);
	for (i = 0; i < imageCount; ++i)
	{
		auto &&extent = images[i]->GetCreateInfoPtr()->extent;
		framebuffers[i] = device->Create(Shit::FramebufferCreateInfo{
			renderPass,
			1,
			&imageViews[i],
			{extent.width, extent.height},
			1});
	}
}
void ImGui_ImplShitRenderer_RecreateFrameBuffers(const std::vector<Shit::Image *> &images)
{
	for (auto &&e : g_Framebuffers)
		g_ShitRendererInitInfo.device->Destroy(e);
	for (auto &&e : imageViews)
		g_ShitRendererInitInfo.device->Destroy(e);
	ImGui_ImplShitRenderer_CreateFramebuffers(g_ShitRendererInitInfo.device, g_RenderPass, images, g_Framebuffers);
}

Shit::RenderPass *ImGui_ImplShitRenderer_CreateRenderPass(
	Shit::Device *device, Shit::Format format,
	Shit::ImageLayout srcImageLayout,
	Shit::ImageLayout dstImageLayout)
{
	// create gui renderPass
	// gui use bit 1 renderpass
	auto sampleCount = Shit::SampleCountFlagBits::BIT_1;
	std::vector<Shit::AttachmentDescription> attachmentDescriptions{
		{format,
		 sampleCount,
		 Shit::AttachmentLoadOp::LOAD,
		 Shit::AttachmentStoreOp::STORE,
		 Shit::AttachmentLoadOp::DONT_CARE,
		 Shit::AttachmentStoreOp::DONT_CARE,
		 srcImageLayout,
		 dstImageLayout}};

	std::vector<Shit::AttachmentReference> colorAttachments{
		{0, // the index of attachment description
		 Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
	};
	std::vector<Shit::AttachmentReference> resolveAttachments;

	std::vector<Shit::SubpassDescription> subPasses{
		{
			Shit::PipelineBindPoint::GRAPHICS,
			0,
			nullptr,
			static_cast<uint32_t>(colorAttachments.size()),
			colorAttachments.data(),
			resolveAttachments.data(),
		}};
	return device->Create(
		Shit::RenderPassCreateInfo{
			static_cast<uint32_t>(attachmentDescriptions.size()),
			attachmentDescriptions.data(),
			static_cast<uint32_t>(subPasses.size()),
			subPasses.data()});
}
bool ImGui_ImplShitRenderer_Init(ImGui_ImplShitRenderer_InitInfo *info)
{
	IM_ASSERT(g_FunctionsLoaded && "Need to call ImGui_ImplShitRenderer_LoadFunctions() if IMGUI_IMPL_VULKAN_NO_PROTOTYPES or VK_NO_PROTOTYPES are set!");

	// Setup backend capabilities flags
	ImGuiIO &io = ImGui::GetIO();
	io.BackendRendererName = "imgui_impl_vulkan";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

	// Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeysDown[] array.
	IM_ASSERT(info->device);
	// IM_ASSERT(info->minImageCount >= 2);
	// IM_ASSERT(info->imageCount >= info->minImageCount);
	// IM_ASSERT(render_pass);

	g_ShitRendererInitInfo = *info;
	g_Subpass = 0;
	g_RenderPass = ImGui_ImplShitRenderer_CreateRenderPass(
		info->device,
		info->images[0]->GetCreateInfoPtr()->format,
		info->initialImageLayout,
		info->finalImageLayout);

	ImGui_ImplShitRenderer_CreateFramebuffers(info->device, g_RenderPass, info->images, g_Framebuffers);

	ImGui_ImplShitRenderer_CreateDescriptorPool();
	ImGui_ImplShitRenderer_CreateDeviceObjects();
	return true;
}
void ImGui_ImplShitRenderer_Shutdown()
{
}
void ImGui_ImplShitRenderer_NewFrame()
{
}
// void ImGui_ImplShitRenderer_SetMinImageCount(uint32_t min_image_count)
//{
//     IM_ASSERT(min_image_count >= 2);
//     if (g_ShitRendererInitInfo.minImageCount== min_image_count)
//         return;
//
//     auto v = &g_ShitRendererInitInfo;
//	//ImGui_ImplShitRendererH_DestroyWindowRenderBuffers(v->device, &g_MainWindowRenderBuffers);
//	g_ShitRendererInitInfo.minImageCount = min_image_count;
// }
void ImGui_ImplShitRenderer_RecordCommandBuffer(uint32_t frameIndex, uint32_t imageIndex)
{
	auto cmdBuffer = g_ShitRendererInitInfo.commandBuffers[frameIndex];
	auto usage = Shit::CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT;
	if (cmdBuffer->GetLevel() == Shit::CommandBufferLevel::SECONDARY)
	{
		usage |= Shit::CommandBufferUsageFlagBits::RENDER_PASS_CONTINUE_BIT; //| Shit::CommandBufferUsageFlagBits::SIMULTANEOUS_USE_BIT;

		cmdBuffer->Begin(Shit::CommandBufferBeginInfo{
			usage,
			{g_RenderPass,
			 g_Subpass,
			 g_Framebuffers[imageIndex]}});
	}
	else
	{
		cmdBuffer->Begin(Shit::CommandBufferBeginInfo{usage});

		// static Shit::ClearValue clearValues[]
		cmdBuffer->BeginRenderPass(
			Shit::BeginRenderPassInfo{
				g_RenderPass,
				g_Framebuffers[imageIndex],
				{{}, g_Framebuffers[imageIndex]->GetCreateInfoPtr()->extent},
				0,
				nullptr,
				{Shit::SubpassContents::INLINE}});
	}

	ImDrawData *draw_data = ImGui::GetDrawData();
	ImGui_ImplShitRenderer_RenderDrawData(frameIndex, draw_data, cmdBuffer, g_Pipeline);
	if (cmdBuffer->GetLevel() == Shit::CommandBufferLevel::PRIMARY)
		cmdBuffer->EndRenderPass();
	cmdBuffer->End();
}
// Create or resize window
// void ImGui_ImplShitRendererH_CreateOrResizeWindow(Instance* instance, PhysicalDevice physical_device, VkDevice device, ImGui_ImplShitRendererH_Window* wd, uint32_t queue_family, const VkAllocationCallbacks* allocator, int width, int height, uint32_t min_image_count)
//{
//    IM_ASSERT(g_FunctionsLoaded && "Need to call ImGui_ImplShitRenderer_LoadFunctions() if IMGUI_IMPL_VULKAN_NO_PROTOTYPES or VK_NO_PROTOTYPES are set!");
//    (void)instance;
//    ImGui_ImplShitRendererH_CreateWindowSwapChain(physical_device, device, wd, allocator, width, height, min_image_count);
//    ImGui_ImplShitRendererH_CreateWindowCommandBuffers(physical_device, device, wd, queue_family, allocator);
//}

// void ImGui_ImplShitRendererH_DestroyWindow(VkInstance instance, VkDevice device, ImGui_ImplShitRendererH_Window* wd, const VkAllocationCallbacks* allocator)
//{
//	 vkDeviceWaitIdle(device); // FIXME: We could wait on the Shit::Queue if we had the queue in wd-> (otherwise ShitRendererH functions can't use globals)
//     //vkQueueWaitIdle(g_Queue);
//
//     for (uint32_t i = 0; i < wd->ImageCount; i++)
//     {
//         ImGui_ImplShitRendererH_DestroyFrame(device, &wd->Frames[i], allocator);
//         ImGui_ImplShitRendererH_DestroyFrameSemaphores(device, &wd->FrameSemaphores[i], allocator);
//     }
//     IM_FREE(wd->Frames);
//     IM_FREE(wd->FrameSemaphores);
//     wd->Frames = NULL;
//     wd->FrameSemaphores = NULL;
//     vkDestroyPipeline(device, wd->Pipeline, allocator);
//     vkDestroyRenderPass(device, wd->RenderPass, allocator);
//     vkDestroySwapchainKHR(device, wd->Swapchain, allocator);
//     vkDestroySurfaceKHR(instance, wd->Surface, allocator);
//
//     *wd = ImGui_ImplShitRendererH_Window();
// }

// void ImGui_ImplShitRendererH_DestroyFrame(VkDevice device, ImGui_ImplShitRendererH_Frame* fd, const VkAllocationCallbacks* allocator)
//{
//     vkDestroyFence(device, fd->Fence, allocator);
//     vkFreeCommandBuffers(device, fd->CommandPool, 1, &fd->CommandBuffer);
//     vkDestroyCommandPool(device, fd->CommandPool, allocator);
//     fd->Shit::Fence  = VK_NULL_HANDLE;
//     fd->Shit::CommandBuffer = VK_NULL_HANDLE;
//     fd->Shit::CommandPool = VK_NULL_HANDLE;
//
//     vkDestroyImageView(device, fd->BackbufferView, allocator);
//     vkDestroyFramebuffer(device, fd->Framebuffer, allocator);
// }
//
// void ImGui_ImplShitRendererH_DestroyFrameSemaphores(VkDevice device, ImGui_ImplShitRendererH_FrameSemaphores* fsd, const VkAllocationCallbacks* allocator)
//{
//     vkDestroySemaphore(device, fsd->ImageAcquiredSemaphore, allocator);
//     vkDestroySemaphore(device, fsd->RenderCompleteSemaphore, allocator);
//     fsd->ImageAcquiredSemaphore = fsd->RenderCompleteSemaphore = VK_NULL_HANDLE;
// }
//
// void ImGui_ImplShitRendererH_DestroyFrameRenderBuffers(VkDevice device, ImGui_ImplShitRendererH_FrameRenderBuffers* buffers, const VkAllocationCallbacks* allocator)
//{
//     if (buffers->VertexBuffer) { vkDestroyBuffer(device, buffers->VertexBuffer, allocator); buffers->VertexBuffer = VK_NULL_HANDLE; }
//     if (buffers->VertexBufferMemory) { vkFreeMemory(device, buffers->VertexBufferMemory, allocator); buffers->VertexBufferMemory = VK_NULL_HANDLE; }
//     if (buffers->IndexBuffer) { vkDestroyBuffer(device, buffers->IndexBuffer, allocator); buffers->IndexBuffer = VK_NULL_HANDLE; }
//     if (buffers->IndexBufferMemory) { vkFreeMemory(device, buffers->IndexBufferMemory, allocator); buffers->IndexBufferMemory = VK_NULL_HANDLE; }
//     buffers->VertexBufferSize = 0;
//     buffers->IndexBufferSize = 0;
// }
//
// void ImGui_ImplShitRendererH_DestroyWindowRenderBuffers(VkDevice device, ImGui_ImplShitRendererH_WindowRenderBuffers* buffers, const VkAllocationCallbacks* allocator)
//{
//     for (uint32_t n = 0; n < buffers->Count; n++)
//         ImGui_ImplShitRendererH_DestroyFrameRenderBuffers(device, &buffers->FrameRenderBuffers[n], allocator);
//     IM_FREE(buffers->FrameRenderBuffers);
//     buffers->FrameRenderBuffers = NULL;
//     buffers->Index = 0;
//     buffers->Count = 0;
// }
