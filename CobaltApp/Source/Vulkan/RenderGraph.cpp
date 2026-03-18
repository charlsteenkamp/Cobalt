#include "copch.hpp"
#include "RenderGraph.hpp"
#include "RenderGraphBuilder.hpp"
#include "RenderPass.hpp"

#include <algorithm>
#include <ranges>
#include <queue>

namespace Cobalt
{

	bool IsWriteRGAccessType(RGAccessType accessType)
	{
		CO_PROFILE_FN();

		switch (accessType)
		{
		case RGAccessType::ColorAttachmentWrite: return true;
		case RGAccessType::ShaderRead: return false;
		case RGAccessType::DepthAttachment: return true;
		case RGAccessType::Present: return true;
		}

		return false;
	}

	bool IsReadRGAccessType(RGAccessType accessType)
	{
		CO_PROFILE_FN();

		switch (accessType)
		{
		case RGAccessType::ColorAttachmentWrite: return false;
		case RGAccessType::ShaderRead: return true;
		case RGAccessType::DepthAttachment: return true;
		case RGAccessType::Present: return false;
		}

		return false;
	}

	VkImageUsageFlags RGAccessTypeToVkImageUsage(RGAccessType accessType)
	{
		CO_PROFILE_FN();

		switch (accessType)
		{
		case RGAccessType::ColorAttachmentWrite: return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		case RGAccessType::ShaderRead: return VK_IMAGE_USAGE_SAMPLED_BIT;
		case RGAccessType::DepthAttachment: return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		case RGAccessType::Present: return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
	}

	struct ResourceAccessInfo
	{
		VkPipelineStageFlags2 StageFlags;
		VkAccessFlags2 AccessMask;
		VkImageLayout ImageLayout;
	};

	ResourceAccessInfo GetResourceAccessInfo(RGAccessType accessType)
	{
		CO_PROFILE_FN();

		ResourceAccessInfo accessInfo;

		switch (accessType)
		{
			case RGAccessType::None:
			{
				accessInfo.StageFlags = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
				accessInfo.AccessMask = 0;
				accessInfo.ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

				return accessInfo;
			}
			case RGAccessType::ColorAttachmentWrite:
			{
				accessInfo.StageFlags = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
				accessInfo.AccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
				accessInfo.ImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				return accessInfo;
			}
			case RGAccessType::ShaderRead:
			{
				accessInfo.StageFlags = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
				accessInfo.AccessMask = VK_ACCESS_2_SHADER_READ_BIT;
				accessInfo.ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				return accessInfo;
			}
			case RGAccessType::DepthAttachment:
			{
				accessInfo.StageFlags = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
				accessInfo.StageFlags = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				accessInfo.ImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

				return accessInfo;
			}
			case RGAccessType::Present:
			{
				accessInfo.StageFlags = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
				accessInfo.AccessMask = 0;
				accessInfo.ImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

				return accessInfo;
			}
		}

		return accessInfo;
	}

	bool IsDepthStencilFormat(VkFormat format)
	{
		CO_PROFILE_FN();

		switch (format)
		{
			case VK_FORMAT_D16_UNORM: return false;
			case VK_FORMAT_D32_SFLOAT: return false;
			case VK_FORMAT_D16_UNORM_S8_UINT: return true;
			case VK_FORMAT_D24_UNORM_S8_UINT: return true;
			case VK_FORMAT_D32_SFLOAT_S8_UINT: return true;
		}

		return false;
	}


	VkImageMemoryBarrier2 MakeBarrierForResourceTransition(VkImage image, VkImageAspectFlags imageAspect, RGAccessType prevAccessType, RGAccessType currAccessType)
	{
		CO_PROFILE_FN();

		ResourceAccessInfo prevAccess = GetResourceAccessInfo(prevAccessType);
		ResourceAccessInfo currAccess = GetResourceAccessInfo(currAccessType);

		VkImageMemoryBarrier2 imageMemoryBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = prevAccess.StageFlags,
			.srcAccessMask = prevAccess.AccessMask,
			.dstStageMask = currAccess.StageFlags,
			.dstAccessMask = currAccess.AccessMask,
			.oldLayout = prevAccess.ImageLayout,
			.newLayout = currAccess.ImageLayout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = {
				.aspectMask = imageAspect,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		return imageMemoryBarrier;
	}

	RenderGraph::RenderGraph()
	{
		CO_PROFILE_FN();

		VkDevice device = GraphicsContext::Get().GetDevice();

		m_vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(device, "vkCmdBeginRenderingKHR"));
		m_vkCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(device, "vkCmdEndRenderingKHR"));
	}

	RenderGraph::~RenderGraph()
	{
		CO_PROFILE_FN();
	}

	Texture& RenderGraph::GetResource(RGResourceHandle handle)
	{
		CO_PROFILE_FN();

		return *mResources[handle];
	}

	void RenderGraph::SetupPassesAndRecordDependencies(RGResourceTouchList& resourceTouchList, RGPassTouchList& passTouchList)
	{
		CO_PROFILE_FN();

		passTouchList.resize(mPasses.size());

		for (RGPassHandle passHandle = 0; passHandle < mPasses.size(); passHandle++)
		{
			RenderGraphBuilder builder(passHandle, mResourceNameHandleMap, mResourceInfos, mClearColorMap);

			mPasses[passHandle]->Setup(builder);

			for (const auto& [resourceHandle, accessType] : builder.GetResourceDependencies())
			{
				if (resourceHandle >= resourceTouchList.size())
					resourceTouchList.resize(resourceHandle + 1);

				resourceTouchList[resourceHandle].push_back({ passHandle, accessType });
				passTouchList[passHandle].push_back(RGResourceDependency{ resourceHandle, accessType });
			}
		}
	}

	void RenderGraph::BuildAdjacencyGraph(const RGResourceTouchList& resourceTouchList, RGPassAdjacencyGraph& passAdjacencyGraph, std::vector<RGPassHandle>& passInDegree)
	{
		CO_PROFILE_FN();

		passAdjacencyGraph.resize(mPasses.size());
		passInDegree.resize(mPasses.size());

		for (RGResourceHandle resourceHandle = 0; resourceHandle < resourceTouchList.size(); resourceHandle++)
		{
			const auto& touches = resourceTouchList[resourceHandle];

			if (touches.size() < 2)
				continue;

			//std::sort(touches.begin(), touches.end(), [](auto& a, auto& b) { return a.PassHandle < b.PassHandle; });

			bool isTransient = mResourceInfos[resourceHandle].Transient;

			for (uint32_t touchA = 0; touchA < touches.size(); touchA++)
			{
				for (uint32_t touchB = touchA + 1; touchB < touches.size(); touchB++)
				{
					auto [passA, accessA] = touches[touchA];
					auto [passB, accessB] = touches[touchB];

					bool aWrites = IsWriteRGAccessType(accessA);
					bool bWrites = IsWriteRGAccessType(accessB);

					// No dependency for Read -> Read

					if (!aWrites && !bWrites)
						continue;

					// 1. Read -> Write: invalid if it is a transient resource; it needs to be written first before being read,
					//                   and is therefore swapped.
					// 2. Write -> Read
					// 3. Write -> Write

					RGPassHandle srcPass = passA;
					RGPassHandle dstPass = passB;

					if (isTransient && !aWrites && bWrites)
					{
						srcPass = passB;
						dstPass = passA;
					}

					// Add an edge from srcPass -> dstPass, and increase the passInDegree if it was inserted

					if (passAdjacencyGraph[srcPass].insert(dstPass).second);
					{
						passInDegree[dstPass]++;
					}
				}
			}
		}
	}

	void RenderGraph::CullPasses(const RGResourceTouchList& resourceTouchList, std::vector<bool>& neededPasses)
	{
		CO_PROFILE_FN();

		// TODO

		neededPasses.resize(mPasses.size());

		for (RGPassHandle passHandle = 0; passHandle < mPasses.size(); passHandle++)
		{
			neededPasses[passHandle] = true;
		}
	}

	void RenderGraph::SortPasses(const std::vector<bool>& neededPasses, const RGPassAdjacencyGraph& passAdjacencyGraph, std::vector<RGPassHandle> passInDegree)
	{
		CO_PROFILE_FN();

		std::queue<RGPassHandle> leafPasses;

		for (RGPassHandle passHandle = 0; passHandle < mPasses.size(); passHandle++)
		{
			if (neededPasses[passHandle] && passInDegree[passHandle] == 0)
			{
				leafPasses.push(passHandle);
			}
		}

		mPassOrder.clear();

		while (!leafPasses.empty())
		{
			RGPassHandle leafPassHandle = leafPasses.front();
			leafPasses.pop();

			mPassOrder.push_back(leafPassHandle);

			for (RGPassHandle dstPassHandle : passAdjacencyGraph[leafPassHandle])
			{
				if (neededPasses[dstPassHandle])
				{
					passInDegree[dstPassHandle]--;

					if (passInDegree[dstPassHandle] == 0)
						leafPasses.push(dstPassHandle);
				}
			}
		}
	}

	void RenderGraph::AllocateResources(const RGResourceTouchList& resourceTouchList)
	{
		CO_PROFILE_FN();

		// TODO: work out resource lifetimes

		const auto& swapchain = GraphicsContext::Get().GetSwapchain();
		uint32_t swapchainWidth = swapchain.GetExtent().width;
		uint32_t swapchainHeight = swapchain.GetExtent().height;
		VkFormat swapchainFormat = swapchain.GetSurfaceFormat().format;
		VkFormat defaultDepthFormat = VK_FORMAT_D32_SFLOAT;

		VkFormat defaultColorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

		mResources.resize(mResourceInfos.size());

		for (RGResourceHandle resourceHandle = 0; resourceHandle < mResourceInfos.size(); resourceHandle++)
		{
			const auto& resourceInfo = mResourceInfos[resourceHandle];

			if (resourceInfo.SwapchainTarget)
				continue;

			TextureInfo textureInfo;

			if (resourceInfo.ResourceSizeFlags == RGResourceSizeFlags::SwapchainRelative)
			{
				textureInfo.Width = resourceInfo.Width * swapchainWidth;
				textureInfo.Height = resourceInfo.Height * swapchainHeight;
			}
			else
			{
				textureInfo.Width = resourceInfo.Width;
				textureInfo.Height = resourceInfo.Height;
			}

			switch (resourceInfo.ResourceType)
			{
				case RGResourceType::ColorAttachment:
				{
					textureInfo.Format = defaultColorFormat;

					for (auto [_, accessType] : resourceTouchList[resourceHandle])
						textureInfo.Usage |= RGAccessTypeToVkImageUsage(accessType);

					break;
				}
				case RGResourceType::DepthAttachment:
				{
					textureInfo.Format = defaultDepthFormat;
					textureInfo.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
					break;
				}
			}

			mResources[resourceHandle] = std::make_unique<Texture>(textureInfo);
		}
	}

	void RenderGraph::BuildCompiledPasses(const RGResourceTouchList& resourceTouchList, const RGPassTouchList& passTouchList, const RGPassAdjacencyGraph& passAdjacencyGraph, const std::vector<bool>& neededPasses)
	{
		CO_PROFILE_FN();

		const Swapchain& swapchain = GraphicsContext::Get().GetSwapchain();

		for (RGPassHandle passHandle = 0; passHandle < mPasses.size(); passHandle++)
		{
			if (!neededPasses[passHandle])
				continue;

			RGPassHandle sortedPassHandle = mPassOrder[passHandle];

			RGCompiledPass compiledPass;
			compiledPass.Pass = mPasses[sortedPassHandle].get();
			compiledPass.RenderArea = {};

			for (auto [resourceHandle, accessType] : passTouchList[sortedPassHandle])
			{
				if (IsReadRGAccessType(accessType) && accessType != RGAccessType::DepthAttachment)
					continue;

				// Fill in attachment info

				VkRenderingAttachmentInfo renderingAttachmentInfo{};
				renderingAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

				if (resourceHandle == RGResourceHandle_BackBufferAttachment)
				{
					compiledPass.BackbufferAttachmentIndex = compiledPass.ColorAttachments.size();
					renderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				}
				else
				{
					renderingAttachmentInfo.imageView = mResources[resourceHandle]->GetImageView();
					renderingAttachmentInfo.imageLayout = GetResourceAccessInfo(accessType).ImageLayout;
				}

				renderingAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				renderingAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

				if (mClearColorMap.contains({ sortedPassHandle, resourceHandle }))
				{
					renderingAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					renderingAttachmentInfo.clearValue.color = mClearColorMap.at({ sortedPassHandle, resourceHandle });
				}
				else if (accessType == RGAccessType::DepthAttachment)
				{
					renderingAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					renderingAttachmentInfo.clearValue = { .depthStencil = { 1.0f, 0 } };

					compiledPass.HasDepthAttachment = true;
					compiledPass.HasStencilAttachment = IsDepthStencilFormat(mResources[resourceHandle]->GetFormat());
				}
				else if (IsReadRGAccessType(accessType))
				{
					renderingAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
				}

				if (IsWriteRGAccessType(accessType))
				{
					renderingAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

					if (resourceHandle == RGResourceHandle_BackBufferAttachment)
					{
						compiledPass.RenderArea.extent = {
							.width = swapchain.GetExtent().width,
							.height = swapchain.GetExtent().height
						};
					}
					else
					{
						compiledPass.RenderArea.extent = {
							.width = mResources[resourceHandle]->GetWidth(),
							.height = mResources[resourceHandle]->GetHeight()
						};
					}
				}

				if (mResourceInfos[resourceHandle].ResourceType == RGResourceType::DepthAttachment)
					compiledPass.DepthStencilAttachment = renderingAttachmentInfo;
				else
					compiledPass.ColorAttachments.push_back(renderingAttachmentInfo);

				// Fill in image barrier info

				uint32_t minDistance = mPasses.size();
				RGPassHandle lastPassHandle = 0; // sorted
				RGAccessType lastAccessType;

				bool shouldInsertBarrier = false;

				// Find the last pass that accessed the resource by finding the minimum "distance"
				// between the current sorted pass and the previous one

				for (auto [otherPassHandle, otherAccessType] : resourceTouchList[resourceHandle])
				{
					if (passAdjacencyGraph[otherPassHandle].find(sortedPassHandle) == passAdjacencyGraph[otherPassHandle].end())
						continue;

					RGPassHandle sortedOtherPassHandle = mPassOrder[otherPassHandle];
					uint32_t distance = sortedPassHandle - otherPassHandle;

					if (distance < minDistance)
					{
						minDistance = distance;
						lastPassHandle = sortedOtherPassHandle;
						lastAccessType = otherAccessType;

						shouldInsertBarrier = true;
					}
				}

				// A barrier should also be inserted if it's the back buffer

				if (shouldInsertBarrier)
				{
					compiledPass.ImageBarriers.push_back(MakeBarrierForResourceTransition(mResources[resourceHandle]->GetImage(), mResources[resourceHandle]->GetImageAspectFlags(), lastAccessType, accessType));
				}
				else if (compiledPass.BackbufferAttachmentIndex == compiledPass.ColorAttachments.size() - 1)
				{
					compiledPass.ImageBarriers.push_back(MakeBarrierForResourceTransition(VK_NULL_HANDLE, VK_IMAGE_ASPECT_COLOR_BIT, RGAccessType::None, RGAccessType::ColorAttachmentWrite));
					compiledPass.ImageBarriers.push_back(MakeBarrierForResourceTransition(VK_NULL_HANDLE, VK_IMAGE_ASPECT_COLOR_BIT, RGAccessType::ColorAttachmentWrite, RGAccessType::Present));
					compiledPass.BackBufferBarrierIndices = { (int32_t)compiledPass.ImageBarriers.size() - 1, (int32_t)compiledPass.ImageBarriers.size() - 2 };
				}
			}

			mCompiledPasses.push_back(compiledPass);
		}
	}

	void RenderGraph::Compile()
	{
		CO_PROFILE_FN();

		// Setup passes & record deps

		RGResourceTouchList resourceTouchList;
		RGPassTouchList passTouchList;

		SetupPassesAndRecordDependencies(resourceTouchList, passTouchList);

		// Build adjacency graph

		RGPassAdjacencyGraph passAdjacencyGraph;
		std::vector<RGPassHandle> passInDegree;

		BuildAdjacencyGraph(resourceTouchList, passAdjacencyGraph, passInDegree);

		// Record needed passes

		std::vector<bool> neededPasses;

		CullPasses(resourceTouchList, neededPasses);

		// Sort passes

		//SortPasses(neededPasses, passAdjacencyGraph, passInDegree);
		mPassOrder = { 0, 1 };
		
		AllocateResources(resourceTouchList);
		BuildCompiledPasses(resourceTouchList, passTouchList, passAdjacencyGraph, neededPasses);
	}

	void RenderGraph::Execute(VkCommandBuffer commandBuffer, RenderFrameContext renderFrameContext)
	{
		CO_PROFILE_FN();

		const Swapchain& swapchain = GraphicsContext::Get().GetSwapchain();
		VkImage backBufferImage = swapchain.GetBackBuffers()[swapchain.GetBackBufferIndex()];
		VkImageView backBufferImageView = swapchain.GetBackBufferViews()[swapchain.GetBackBufferIndex()];

		for (auto& compiledPass : mCompiledPasses)
		{
			if (!compiledPass.ImageBarriers.empty())
			{
				for (uint32_t i : compiledPass.BackBufferBarrierIndices)
				{
					compiledPass.ImageBarriers[i].image = backBufferImage;
				}

				VkDependencyInfo dependencyInfo = {
					.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
					.imageMemoryBarrierCount = (uint32_t)compiledPass.ImageBarriers.size(),
					.pImageMemoryBarriers = compiledPass.ImageBarriers.data(),
				};

				vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
			}

			if (compiledPass.BackbufferAttachmentIndex != -1)
				compiledPass.ColorAttachments[compiledPass.BackbufferAttachmentIndex].imageView = backBufferImageView;

			VkRenderingInfo renderingInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
				.renderArea = compiledPass.RenderArea,
				.layerCount = 1,
				.colorAttachmentCount = (uint32_t)compiledPass.ColorAttachments.size(),
				.pColorAttachments = compiledPass.ColorAttachments.data(),
				.pDepthAttachment = compiledPass.HasDepthAttachment ? &compiledPass.DepthStencilAttachment : nullptr,
				.pStencilAttachment = compiledPass.HasStencilAttachment ? &compiledPass.DepthStencilAttachment : nullptr,
			};

			m_vkCmdBeginRenderingKHR(commandBuffer, &renderingInfo);
			compiledPass.Pass->Execute(commandBuffer, renderFrameContext);
			m_vkCmdEndRenderingKHR(commandBuffer);
		}
	}

}