#pragma once
#include <renderer/ShitRenderSystem.hpp>
#include "config.hpp"
#include <imgui.h>

using namespace Shit;

struct ImGui_ImplShitRenderer_InitInfo
{
	RenderSystem *renderSystem;
	Device *device;
	std::vector<CommandBuffer *> secondaryCommandBuffers;
	uint32_t imageCount;
	SampleCountFlagBits msaaSamples;
};

// Called by user code
IMGUI_IMPL_API bool ImGui_ImplShitRenderer_Init(ImGui_ImplShitRenderer_InitInfo *info, RenderPass *render_pass,uint32_t subpass);
IMGUI_IMPL_API void ImGui_ImplShitRenderer_Shutdown();
IMGUI_IMPL_API void ImGui_ImplShitRenderer_NewFrame();
IMGUI_IMPL_API void ImGui_ImplShitRenderer_RenderDrawData(ImDrawData *draw_data, CommandBuffer *command_buffer, Pipeline *pipeline);
IMGUI_IMPL_API bool ImGui_ImplShitRenderer_CreateFontsTexture(CommandBuffer *command_buffer);
IMGUI_IMPL_API void ImGui_ImplShitRenderer_DestroyFontUploadObjects();
//IMGUI_IMPL_API void ImGui_ImplShitRenderer_SetMinImageCount(uint32_t min_image_count); // To override MinImageCount after initialization (e.g. if swap chain is recreated)
IMGUI_IMPL_API void ImGui_ImplShitRenderer_RecordCommandBuffer(uint32_t imageIndex, RenderPass *renderPass, uint32_t subpass, Framebuffer *pFrameBuffer);

