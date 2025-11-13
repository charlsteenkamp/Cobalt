#pragma once
#include <imgui.h>
#include <vulkan/vulkan.h>
#include <vector>

namespace Cobalt
{

	class ImGuiBackend
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginFrame();
		static void EndFrame();    // called by Application
		static void RenderFrame(); // called by GraphicsContext

		static void OnResize();

	public:
		static VkCommandBuffer GetActiveCommandBuffer() { return sData->ActiveCommandBuffer; }

	private:
		static void CreateOrRecreateFramebuffers();

	private:
		struct ImGuiBackendData
		{
			std::vector<VkCommandPool> CommandPools; // per frame
			std::vector<VkCommandBuffer> CommandBuffers; // per frame
			VkCommandBuffer ActiveCommandBuffer = VK_NULL_HANDLE;

			std::vector<VkFramebuffer> Framebuffers; // per backbuffer

			VkRenderPass ImGuiRenderPass = VK_NULL_HANDLE;
		};

		inline static ImGuiBackendData* sData = nullptr;
	};

}