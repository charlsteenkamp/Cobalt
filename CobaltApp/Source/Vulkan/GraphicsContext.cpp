#include "copch.hpp"
#include "GraphicsContext.hpp"
#include "Application.hpp"
#include "VulkanUtils.hpp"
#include "Renderer.hpp"
#include "ImGuiBackend.hpp"


#include <cstdlib>
#include <iostream>

namespace Cobalt
{

	GraphicsContext::GraphicsContext(const Window& window)
		: mWindow(window)
	{
		CO_PROFILE_FN();

		if (sGraphicsContextInstance)
			return;

		sGraphicsContextInstance = this;
	}

	GraphicsContext::~GraphicsContext()
	{
		CO_PROFILE_FN();
	}

	void GraphicsContext::Init()
	{
		CO_PROFILE_FN();

		// Setup Vulkan Instance

		{
			uint32_t extensionCount = 0;
			const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
			const char** extensions;

			if (mEnableValidationLayers)
			{
				extensionCount++;
				extensions = (const char**)malloc(sizeof(const char*) * extensionCount);
				memcpy(extensions, requiredExtensions, extensionCount * sizeof(const char*));
				extensions[extensionCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
			}
			else
			{
				extensions = requiredExtensions;
			}

			const char* layers[] = { "VK_LAYER_KHRONOS_validation" };

			VkApplicationInfo applicationInfo = {
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pApplicationName = "Cobalt Application",
				.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
				.pEngineName = "Cobalt",
				.engineVersion = VK_MAKE_VERSION(1, 0, 0),
				.apiVersion = VK_API_VERSION_1_3
			};

			VkInstanceCreateInfo instanceCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				.flags = 0,
				.pApplicationInfo = &applicationInfo,
				.enabledExtensionCount = extensionCount,
				.ppEnabledExtensionNames = extensions
			};

			if (mEnableValidationLayers)
			{
				instanceCreateInfo.ppEnabledLayerNames = layers;
				instanceCreateInfo.enabledLayerCount = 1;
			}
			else
			{
				instanceCreateInfo.enabledExtensionCount = extensionCount;
				instanceCreateInfo.ppEnabledExtensionNames = extensions;
			}

			VK_CALL(vkCreateInstance(&instanceCreateInfo, nullptr, &mInstance));

			if (mEnableValidationLayers)
			{
				free(extensions);
			}
		}

		// Create debug utils messenger

		if (mEnableValidationLayers)
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {
					.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
					.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
					.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
					.pfnUserCallback = VulkanDebugCallback
			};

			auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");
			VK_CALL(fn(mInstance, &createInfo, nullptr, &mDebugUtilsMessenger));
		}

		// Select physical device

		{
			uint32_t count = 0;
			VK_CALL(vkEnumeratePhysicalDevices(mInstance, &count, nullptr));

			VkPhysicalDevice* physicalDevices = (VkPhysicalDevice*)malloc(count * sizeof(VkPhysicalDevice));
			VK_CALL(vkEnumeratePhysicalDevices(mInstance, &count, physicalDevices));

			uint32_t selectedIndex = 0;

			for (uint32_t i = 0; i < count; i++)
			{
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);

				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					selectedIndex = i;
					break;
				}
			}

			mPhysicalDevice = physicalDevices[selectedIndex];

			free(physicalDevices);
		}

		// Select graphics queue family

		{
			uint32_t count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &count, nullptr);

			VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*)malloc(count * sizeof(VkQueueFamilyProperties));
			vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &count, queues);

			for (int32_t i = 0; i < count; i++)
			{
				if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					mQueueFamily = i;
					break;
				}
			}
			
			free(queues);
		}

		// Create logical device

		{
			const char* deviceExtensions[] = { "VK_KHR_swapchain" };
			const float queuePriority[] = { 1.0f };

			VkPhysicalDeviceVariablePointerFeatures variablePointerFeatures = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTER_FEATURES,
				.variablePointersStorageBuffer = VK_TRUE,
				.variablePointers = VK_TRUE,
			};

			VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
				.pNext = (void*)&variablePointerFeatures,
				.bufferDeviceAddress = VK_TRUE,
			};

			VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
				.pNext = (void*)&bufferDeviceAddressFeatures,
				.shaderSampledImageArrayNonUniformIndexing = true,
				.shaderStorageBufferArrayNonUniformIndexing = true,
				.shaderStorageImageArrayNonUniformIndexing = true,
				.descriptorBindingSampledImageUpdateAfterBind = true,
				.descriptorBindingStorageImageUpdateAfterBind = true,
				.descriptorBindingStorageBufferUpdateAfterBind = true,
				.descriptorBindingPartiallyBound = true,
				.runtimeDescriptorArray = true,
			};

			VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES,
				.pNext = (void*)&descriptorIndexingFeatures,
				.shaderDrawParameters = VK_TRUE
			};

			VkDeviceQueueCreateInfo queueCreateInfo[1] = {
				{
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueFamilyIndex = (uint32_t)mQueueFamily,
					.queueCount = 1,
					.pQueuePriorities = queuePriority
				}
			};

			VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
			physicalDeviceFeatures.shaderInt64 = VK_TRUE;
			physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

			VkDeviceCreateInfo createInfo = {
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.pNext = &shaderDrawParametersFeatures,
				.queueCreateInfoCount = 1,
				.pQueueCreateInfos = queueCreateInfo,
				.enabledExtensionCount = 1,
				.ppEnabledExtensionNames = deviceExtensions,
				.pEnabledFeatures = &physicalDeviceFeatures
			};

			VK_CALL(vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice));

			vkGetDeviceQueue(mDevice, mQueueFamily, 0, &mQueue);
		}

		// Create descriptor pool
		
		{
			VkDescriptorPoolSize poolSizes[] = {
//				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 50000 },
//				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
//				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
//				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
//				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10000 },
//				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
//				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
//				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};

			VkDescriptorPoolCreateInfo createInfo = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
				.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
				.maxSets = 1000 * (sizeof(poolSizes) / sizeof(poolSizes[0])),
				.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]),
				.pPoolSizes = poolSizes
			};

			VK_CALL(vkCreateDescriptorPool(mDevice, &createInfo, nullptr, &mDescriptorPool));
		}

		// Create window surface

		{
			VK_CALL(glfwCreateWindowSurface(mInstance, mWindow.GetWindow(), nullptr, &mSurface));
		}

		// Create window swapchain

		{
			mSwapchain = std::make_unique<Swapchain>(mWindow, mDevice, mPhysicalDevice, mSurface);
		}

		// Create transient command pool

		{
#if 0
			VkCommandPoolCreateInfo commandPoolCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
				.queueFamilyIndex = (uint32_t)mQueueFamily,
			};

			VK_CALL(vkCreateCommandPool(mDevice, &commandPoolCreateInfo, nullptr, &mTransientCommandPool));
#endif
		}

		// Create frames

		{
			mFrames.resize(mFrameCount);

			for (FrameData& fd : mFrames)
			{
				// Create command pool

				{
					VkCommandPoolCreateInfo commandPoolCreateInfo = {
						.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
						//.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
						.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
						.queueFamilyIndex = (uint32_t)mQueueFamily,
					};

					VK_CALL(vkCreateCommandPool(mDevice, &commandPoolCreateInfo, nullptr, &fd.CommandPool));

					fd.CommandBuffer = AllocateCommandBuffer(fd.CommandPool);
				}

				// Create synchronisation objects 

				{
					VkFenceCreateInfo fenceCreateInfo = {
						.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
						.flags = VK_FENCE_CREATE_SIGNALED_BIT
					};

					VkSemaphoreCreateInfo semaphoreCreateInfo = {
						.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
						.flags = 0
					};

					VK_CALL(vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &fd.AcquireNextImageFence));

					VK_CALL(vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &fd.ImageAcquiredSemaphore));
					VK_CALL(vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &fd.RenderFinishedSemaphore));
				}
			}
		}

		// Initialize VMA

		{

			VmaAllocatorCreateInfo allocatorCreateInfo = {
				.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
				.physicalDevice = mPhysicalDevice,
				.device = mDevice,
				.instance = mInstance,
			};

			VK_CALL(vmaCreateAllocator(&allocatorCreateInfo, &mAllocator));
		}

		OPTICK_GPU_INIT_VULKAN(&mDevice, &mPhysicalDevice, &mQueue, (uint32_t*)&mQueueFamily, 2, nullptr);
	}

	void GraphicsContext::Shutdown()
	{
		CO_PROFILE_FN();

		for (auto& fd : mFrames)
		{
			vkDestroyCommandPool(mDevice, fd.CommandPool, nullptr);

			vkDestroyFence(mDevice, fd.AcquireNextImageFence, nullptr);
			vkDestroySemaphore(mDevice, fd.ImageAcquiredSemaphore, nullptr);
			vkDestroySemaphore(mDevice, fd.RenderFinishedSemaphore, nullptr);
		}

		mFrames.clear();

		//vkDestroyCommandPool(mDevice, mTransientCommandPool, nullptr);

		mSwapchain.reset();

		vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
		vkDestroyDevice(mDevice, nullptr);

		auto pfn_vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
		pfn_vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugUtilsMessenger, nullptr);
		//vkDestroyInstance(mInstance, nullptr);
	}

	VkCommandBuffer GraphicsContext::AllocateCommandBuffer(VkCommandPool commandPool)
	{
		CO_PROFILE_FN();

		VkCommandBuffer commandBuffer;

		VkCommandBufferAllocateInfo allocInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		VK_CALL(vkAllocateCommandBuffers(mDevice, &allocInfo, &commandBuffer));

		return commandBuffer;
	}

	void GraphicsContext::SubmitSingleTimeCommands(VkQueue queue, std::function<void(VkCommandBuffer)> fn)
	{
		CO_PROFILE_FN();

		VkFenceCreateInfo fenceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = 0
		};

		VkFence fence;
		VK_CALL(vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &fence));

		VkCommandBuffer commandBuffer = AllocateCommandBuffer(mFrames[mFrameIndex].CommandPool);

		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

		VK_CALL(vkBeginCommandBuffer(commandBuffer, &beginInfo));
		fn(commandBuffer);
		VK_CALL(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer
		};

		VK_CALL(vkQueueSubmit(queue, 1, &submitInfo, fence));
		VK_CALL(vkWaitForFences(mDevice, 1, &fence, VK_TRUE, UINT64_MAX));

		vkDestroyFence(mDevice, fence, nullptr);
		vkFreeCommandBuffers(mDevice, mFrames[mFrameIndex].CommandPool, 1, &commandBuffer);
	}

	void GraphicsContext::RenderFrame(const std::vector<Module*> modules)
	{
		CO_PROFILE_CATEGORY(OPTICK_FUNC, Optick::Category::Rendering);

		VkResult result;

		const FrameData& fd = mFrames[mFrameIndex];

		bool enableImGui = Application::Get()->GetInfo().EnableImGui;

		// Wait for previous frame to finish

		{
			CO_PROFILE_CATEGORY("vkWaitForFences", Optick::Category::Wait);

			VK_CALL(vkWaitForFences(mDevice, 1, &fd.AcquireNextImageFence, VK_TRUE, UINT64_MAX));
		}

		// Acquire next image

		{
			CO_PROFILE_CATEGORY("vkAcquireNextImageKHR", Optick::Category::Wait);

			result = vkAcquireNextImageKHR(mDevice, mSwapchain->GetHandle(), UINT64_MAX, fd.ImageAcquiredSemaphore, VK_NULL_HANDLE, mSwapchain->GetBackBufferIndexPtr());

			if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR || mSwapchain->GetBackBufferIndex() == -1)
			{
				mRecreateSwapchain = true;
				return;
			}

			VK_CALL(result);

			VK_CALL(vkResetFences(mDevice, 1, &fd.AcquireNextImageFence));
		}

		// Start command buffer

		{
			CO_PROFILE_CATEGORY("vkBeginCommandBuffer", Optick::Category::Wait);

			mActiveCommandBuffer = fd.CommandBuffer;

			CO_PROFILE_COMMAND_BUFFER(mActiveCommandBuffer);

			VkCommandBufferBeginInfo beginInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
			};

			VK_CALL(vkBeginCommandBuffer(mActiveCommandBuffer, &beginInfo));
		}

		// Do rendering

		{
			CO_PROFILE_SCOPE("Rendering");

			for (Module* module : modules)
				module->OnRender();
		}

		if (enableImGui)
		{
			ImGuiBackend::RenderFrame();
		}
		
		// Submit command buffers

		{
			CO_PROFILE_GPU_EVENT("Queue Submission");

			VK_CALL(vkEndCommandBuffer(mActiveCommandBuffer));

			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			std::vector<VkCommandBuffer> commandBuffers;
			commandBuffers.push_back(mActiveCommandBuffer);

			if (enableImGui)
				commandBuffers.push_back(ImGuiBackend::GetActiveCommandBuffer());

			VkSubmitInfo submitInfo = {
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &fd.ImageAcquiredSemaphore,
				.pWaitDstStageMask = &waitStage,
				.commandBufferCount = (uint32_t)commandBuffers.size(),
				.pCommandBuffers = commandBuffers.data(),
				.signalSemaphoreCount = 1,
				.pSignalSemaphores = &fd.RenderFinishedSemaphore
			};

			VK_CALL(vkQueueSubmit(mQueue, 1, &submitInfo, fd.AcquireNextImageFence));

			mActiveCommandBuffer = VK_NULL_HANDLE;
		}
	}

	void GraphicsContext::PresentFrame()
	{
		CO_PROFILE_SET_SWAPCHAIN(mSwapchain->GetHandle());
		CO_PROFILE_CATEGORY("Swapchain Presentation", Optick::Category::Wait);
		
		if (mRecreateSwapchain)
			return;

		VkSwapchainKHR swapchain = mSwapchain->GetHandle();

		VkPresentInfoKHR presentInfo = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &mFrames[mFrameIndex].RenderFinishedSemaphore,
			.swapchainCount = 1,
			.pSwapchains = &swapchain,
			.pImageIndices = mSwapchain->GetBackBufferIndexPtr()
		};

		VkResult result = vkQueuePresentKHR(mQueue, &presentInfo);

		if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			mRecreateSwapchain = true;
			return;
		}

		VK_CALL(result);

		mFrameIndex = (mFrameIndex + 1) % mFrameCount;
	}

	void GraphicsContext::OnResize()
	{
		CO_PROFILE_FN();

		if (mRecreateSwapchain)
		{
			mSwapchain->Recreate();
			mRecreateSwapchain = false;
		}
	}

}