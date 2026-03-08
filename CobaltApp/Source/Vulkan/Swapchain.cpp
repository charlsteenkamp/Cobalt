#include "copch.hpp"
#include "Swapchain.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>

namespace Cobalt
{

	namespace Utils
	{

		struct SwapchainSupportDetails
		{
			VkSurfaceCapabilitiesKHR        Capabilities;
			std::vector<VkSurfaceFormatKHR> SurfaceFormats;
			std::vector<VkPresentModeKHR>   PresentModes;
		};

		static SwapchainSupportDetails QuerySwapchainSupportDetails(VkSurfaceKHR surfaceHandle, VkPhysicalDevice physicalDeviceHandle)
		{
			SwapchainSupportDetails supportDetails;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDeviceHandle, surfaceHandle, &supportDetails.Capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDeviceHandle, surfaceHandle, &formatCount, nullptr);

			if (formatCount > 0)
			{
				supportDetails.SurfaceFormats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDeviceHandle, surfaceHandle, &formatCount, supportDetails.SurfaceFormats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDeviceHandle, surfaceHandle, &presentModeCount, nullptr);

			if (presentModeCount > 0)
			{
				supportDetails.PresentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDeviceHandle, surfaceHandle, &presentModeCount, supportDetails.PresentModes.data());
			}

			return supportDetails;
		}

		static VkSurfaceFormatKHR ChooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
		{
			for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
			{
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					return availableFormat;
				}
			}

			return availableFormats[0];
		}

		static VkPresentModeKHR ChooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
		{
			return VK_PRESENT_MODE_IMMEDIATE_KHR;

			for (const VkPresentModeKHR& availablePresentMode : availablePresentModes)
			{
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					return availablePresentMode;
				}
				else if (availablePresentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
				{
					return availablePresentMode;
				}
			}

			return VK_PRESENT_MODE_FIFO_KHR;
		}

		static VkExtent2D ChooseSwapchainExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities, GLFWwindow* windowHandle)
		{
			if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
				return surfaceCapabilities.currentExtent;

			int32_t framebufferSize[2];
			glfwGetFramebufferSize(windowHandle, &framebufferSize[0], &framebufferSize[1]);

			VkExtent2D actualExtent = {
				std::clamp((uint32_t)framebufferSize[0], surfaceCapabilities.minImageExtent.width,  surfaceCapabilities.minImageExtent.width),
				std::clamp((uint32_t)framebufferSize[1], surfaceCapabilities.minImageExtent.height, surfaceCapabilities.minImageExtent.height)
			};

			return actualExtent;
		}


	}

	Swapchain::Swapchain(const Window& window, VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
		: mWindow(window), mDevice(device), mPhysicalDevice(physicalDevice), mSurface(surface)
	{
		CO_PROFILE_FN();

		CreateOrRecreateSwapchain();
		CreateOrRecreateBackbuffers();
	}

	Swapchain::~Swapchain()
	{
		CO_PROFILE_FN();

		vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);

		for (uint32_t i = 0; i < mBackBufferCount; i++)
		{
			vkDestroyImageView(mDevice, mBackBufferViews[i], nullptr);
		}

		free(mBackBuffers);
		free(mBackBufferViews);
	}

	void Swapchain::Recreate()
	{
		CO_PROFILE_FN();

		CreateOrRecreateSwapchain();
		CreateOrRecreateBackbuffers();
	}

	void Swapchain::CreateOrRecreateSwapchain()
	{
		CO_PROFILE_FN();

		VkSwapchainKHR oldSwapchain = mSwapchain;

		// Choose swapchain details

		Utils::SwapchainSupportDetails supportDetails = Utils::QuerySwapchainSupportDetails(mSurface, mPhysicalDevice);

		mSurfaceFormat = Utils::ChooseSwapchainSurfaceFormat(supportDetails.SurfaceFormats);
		mPresentMode   = Utils::ChooseSwapchainPresentMode(supportDetails.PresentModes);
		mExtent        = Utils::ChooseSwapchainExtent(supportDetails.Capabilities, mWindow.GetWindow());
		
		// Choose back buffer count

		mBackBufferCount = 3;

		if (supportDetails.Capabilities.maxImageCount > 0 && mBackBufferCount > supportDetails.Capabilities.maxImageCount)
			mBackBufferCount = supportDetails.Capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR swapchainCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.flags = 0,
			.surface = mSurface,
			.minImageCount = mBackBufferCount,
			.imageFormat = mSurfaceFormat.format,
			.imageColorSpace = mSurfaceFormat.colorSpace,
			.imageExtent = mExtent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = mPresentMode,
			.clipped = VK_TRUE,
			.oldSwapchain = mSwapchain
		};

		VK_CALL(vkCreateSwapchainKHR(mDevice, &swapchainCreateInfo, nullptr, &mSwapchain));

		if (oldSwapchain)
			vkDestroySwapchainKHR(mDevice, oldSwapchain, nullptr);
	}

	void Swapchain::CreateOrRecreateBackbuffers()
	{
		CO_PROFILE_FN();

		if (mBackBuffers)
			free(mBackBuffers);

		if (mBackBufferViews)
		{
			for (uint32_t i = 0; i < mBackBufferCount; i++)
				vkDestroyImageView(mDevice, mBackBufferViews[i], nullptr);

			free(mBackBufferViews);
		}

		mBackBuffers     = (VkImage*)    malloc(mBackBufferCount * sizeof(VkImage));
		mBackBufferViews = (VkImageView*)malloc(mBackBufferCount * sizeof(VkImageView));

		VK_CALL(vkGetSwapchainImagesKHR(mDevice, mSwapchain, &mBackBufferCount, mBackBuffers));

		VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.flags = 0,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = mSurfaceFormat.format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		for (uint32_t i = 0; i < mBackBufferCount; i++)
		{
			imageViewCreateInfo.image = mBackBuffers[i];

			VK_CALL(vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mBackBufferViews[i]));
		}
	}

}