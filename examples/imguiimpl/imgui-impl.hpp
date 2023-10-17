#pragma once
#include "renderer/ShitRenderSystem.hpp"
#include <imgui.h>

struct ImGui_ImplShitRenderer_InitInfo
{
	uint32_t maxFramesInFlight;
	Shit::RendererVersion rendererVersion;
	Shit::Device *device;
	std::vector<Shit::Image *> images;
	std::vector<Shit::CommandBuffer *> commandBuffers; //imageCount commandbuffers, can be primary or secondary buffers
	Shit::ImageLayout initialImageLayout;
	Shit::ImageLayout finalImageLayout;
	//Shit::SampleCountFlagBits msaaSamples;
};

// Called by user code
IMGUI_IMPL_API bool ImGui_ImplShitRenderer_Init(ImGui_ImplShitRenderer_InitInfo *info);
IMGUI_IMPL_API void ImGui_ImplShitRenderer_Shutdown();
IMGUI_IMPL_API void ImGui_ImplShitRenderer_NewFrame();
IMGUI_IMPL_API void ImGui_ImplShitRenderer_RecreateFrameBuffers(const std::vector<Shit::Image *> &images);
IMGUI_IMPL_API void ImGui_ImplShitRenderer_RenderDrawData(
	uint32_t imageIndex,
	ImDrawData *draw_data,
	Shit::CommandBuffer *command_buffer,
	Shit::Pipeline *pipeline);
IMGUI_IMPL_API bool ImGui_ImplShitRenderer_CreateFontsTexture(Shit::CommandBuffer *command_buffer);
IMGUI_IMPL_API void ImGui_ImplShitRenderer_DestroyFontUploadObjects();
//IMGUI_IMPL_API void ImGui_ImplShitRenderer_SetMinImageCount(uint32_t min_image_count); // To override MinImageCount after initialization (e.g. if swap chain is recreated)
IMGUI_IMPL_API void ImGui_ImplShitRenderer_RecordCommandBuffer(uint32_t frameIndex, uint32_t imageIndex);