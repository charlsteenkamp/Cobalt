#include "copch.hpp"
#include "RenderGraph.hpp"
#include "RenderPass.hpp"

#include <algorithm>
#include <ranges>
#include <queue>

namespace Cobalt
{

	bool IsWriteRGAccessType(RGAccessType accessType)
	{
		switch (accessType)
		{
			case RGAccessType::ColorAttachmentWrite: return true;
			case RGAccessType::ShaderRead: return false;
			case RGAccessType::ReadWrite: return true;
		}

		return false;
	}


	RenderGraph::RenderGraph()
	{

	}

	RenderGraph::~RenderGraph()
	{

	}

	Texture& RenderGraph::GetResource(RGResourceHandle handle)
	{
		Texture texture(TextureInfo(""));
		return texture;
	}

	void RenderGraph::SetupPassesAndRecordDependencies(RGResourceTouchList& resourceTouchList)
	{
		CO_PROFILE_FN();

		for (RGPassHandle passHandle = 0; passHandle < mPasses.size(); passHandle++)
		{
			RenderGraphBuilder builder(mResourceNameHandleMap, mResourceInfos);

			mPasses[passHandle]->Setup(builder);

			for (const auto& [resourceHandle, accessType] : builder.GetResourceDependencies())
			{
				if (resourceHandle >= resourceTouchList.size())
					resourceTouchList.resize(resourceHandle + 1);

				resourceTouchList[resourceHandle].push_back({ passHandle, accessType });
			}
		}
	}

	void RenderGraph::BuildAdjacencyGraph(const RGResourceTouchList& resourceTouchList, RGPassAdjacencyGraph& passAdjacencyGraph, std::vector<RGPassHandle> passInDegree)
	{
		CO_PROFILE_FN();

		for (RGResourceHandle resourceHandle = 0; resourceHandle < resourceTouchList.size(); resourceHandle++)
		{
			const auto& touches = resourceTouchList[resourceHandle];

			if (touches.size() < 2)
				continue;

			std::sort(touches.begin(), touches.end(), [](auto& a, auto& b) { return a.PassHandle < b.PassHandle; });

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

		neededPasses.resize(mPasses.size());
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

	void RenderGraph::Compile()
	{
		CO_PROFILE_FN();

		// Setup passes & record deps

		RGResourceTouchList resourceTouchList;

		SetupPassesAndRecordDependencies(resourceTouchList);

		// Build adjacency graph

		RGPassAdjacencyGraph passAdjacencyGraph;
		std::vector<RGPassHandle> passInDegree;

		BuildAdjacencyGraph(resourceTouchList, passAdjacencyGraph, passInDegree);

		// Record needed passes

		std::vector<bool> neededPasses;

		CullPasses(resourceTouchList, neededPasses);

		// Sort passes

		SortPasses(neededPasses, passAdjacencyGraph, passInDegree);
		
		BuildResources();
		BuildBarriers();
		BuildCompiledPasses();
	}

	void RenderGraph::Execute(VkCommandBuffer commandBuffer, RenderFrameContext renderFrameContext)
	{
		CO_PROFILE_FN();

		for (const auto& compiledPass : mCompiledPasses)
		{
			if (!compiledPass.ImageBarriers.empty())
			{
				VkDependencyInfo dependencyInfo = {
					.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
					.imageMemoryBarrierCount = (uint32_t)compiledPass.ImageBarriers.size(),
					.pImageMemoryBarriers = compiledPass.ImageBarriers.data(),
				};

				vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
			}

			VkRenderingInfo renderingInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
				.renderArea = compiledPass.RenderArea,
				.layerCount = 1,
				.colorAttachmentCount = (uint32_t)compiledPass.ColorAttachments.size(),
				.pColorAttachments = compiledPass.ColorAttachments.data(),
				.pDepthAttachment = &compiledPass.DepthStencilAttachment,
				.pStencilAttachment = &compiledPass.DepthStencilAttachment,
			};

			vkCmdBeginRenderingKHR(commandBuffer, &renderingInfo);
			compiledPass.Pass->Execute(commandBuffer, renderFrameContext);
			vkCmdEndRenderingKHR(commandBuffer);
		}
	}

}