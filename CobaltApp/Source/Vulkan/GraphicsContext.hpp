#pragma once
#include "../Window.hpp"
#include "Swapchain.hpp"
#include "DescriptorBufferManager.hpp"
#include "Module.hpp"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <memory>

// temporary
#define CO_BINDLESS_DESCRIPTOR_COUNT 1000

namespace Cobalt
{

	struct FrameData
	{
		VkCommandPool CommandPool;
		VkCommandBuffer CommandBuffer;

		VkFence     AcquireNextImageFence;
		VkSemaphore ImageAcquiredSemaphore;
		VkSemaphore RenderFinishedSemaphore;
	};

	class GraphicsContext
	{
	public:
		static GraphicsContext& Get() { return *sGraphicsContextInstance; }

		VkInstance       GetInstance()       const { return mInstance; }
		VkDevice         GetDevice()         const { return mDevice;   }
		VkPhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }
		VkQueue          GetQueue()          const { return mQueue;    }
		int32_t GetQueueFamily() const { return mQueueFamily; }

		const Swapchain& GetSwapchain() const { return *mSwapchain; }
		const DescriptorBufferManager& GetDescriptorBufferManager() const { return *mDescriptorBufferManager; }

		VmaAllocator GetAllocator() const { return mAllocator; }

		bool ShouldRecreateSwapchain() const { return mRecreateSwapchain; }

		uint32_t GetFrameCount() const { return mFrameCount; }
		uint32_t GetFrameIndex() const { return mFrameIndex; }

		const FrameData& GetFrameData(uint32_t index) const { return mFrames[index]; }
		      FrameData& GetFrameData(uint32_t index)       { return mFrames[index]; }

	public:
		VkCommandBuffer AllocateCommandBuffer(VkCommandPool commandPool);
		void SubmitSingleTimeCommands(VkQueue queue, std::function<void(VkCommandBuffer)> fn);

	public:
		GraphicsContext(const Window& window);
		~GraphicsContext();

	public:
		void Init();
		void Shutdown();

		void RenderFrame(const std::vector<Module*> modules);
		void PresentFrame();

		void OnResize();

	public:
		VkCommandBuffer GetActiveCommandBuffer() const { return mActiveCommandBuffer; }

	private:
		const Window& mWindow;

		uint32_t mFrameCount = 3;
		uint32_t mFrameIndex = 0;
		std::vector<FrameData> mFrames;

		VkCommandBuffer mActiveCommandBuffer;

	private:
		inline static GraphicsContext* sGraphicsContextInstance = nullptr;

		VkInstance               mInstance;
		VkDebugUtilsMessengerEXT mDebugUtilsMessenger;
		VkPhysicalDevice         mPhysicalDevice;
		int32_t                  mQueueFamily;
		VkQueue                  mQueue;
		VkDevice                 mDevice;
		VkSurfaceKHR             mSurface;

		std::unique_ptr<Swapchain> mSwapchain;
		std::unique_ptr<DescriptorBufferManager> mDescriptorBufferManager;

		bool mRecreateSwapchain = false;

#ifdef CO_DEBUG
		bool mEnableValidationLayers = true;
#else
		bool mEnableValidationLayers = false;
#endif

		VmaAllocator mAllocator;
	};

}
