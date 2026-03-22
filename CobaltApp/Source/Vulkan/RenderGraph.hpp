#pragma once
#include "VulkanUtils.hpp"
#include "Texture.hpp"
#include "RenderGraphBuilder.hpp"
#include "Renderer.hpp"

#include <string>
#include <vector>
#include <unordered_set>
#include <functional>

namespace Cobalt
{

	class RenderPass;

	struct RGCompiledPass
	{
		std::vector<VkImageMemoryBarrier2> ImageBarriers;
		std::vector<VkImageMemoryBarrier2> PostImageBarriers;

		VkRect2D RenderArea;
		std::vector<VkRenderingAttachmentInfo> ColorAttachments;
		VkRenderingAttachmentInfo DepthStencilAttachment;

		int32_t BackbufferAttachmentIndex = -1;
		std::vector<int32_t> BackBufferBarrierIndices, BackBufferPostBarrierIndices;
		bool HasDepthAttachment = false;
		bool HasStencilAttachment = false;

		RenderPass* Pass;
	};

	using RGResourceTouchList = std::vector<std::vector<RGPassDependency>>;
	using RGPassTouchList = std::vector<std::vector<RGResourceDependency>>;
	using RGPassAdjacencyGraph = std::vector<std::unordered_set<RGPassHandle>>; // src pass -> set of dst passes

	class RenderGraph
	{
	public:
		RenderGraph();
		~RenderGraph();

	public:
		Texture& GetResource(RGResourceHandle handle) const;
		RenderPass* GetPass(const std::string& passName) const;

		std::vector<Texture*> GetPassOutputAttachments(const std::string& passName) const;

		const std::vector<std::unique_ptr<RenderPass>>& GetPasses() const { return mPasses; }

	public:
		template<typename T>
		void AddPass()
		{
			static_assert(std::is_base_of<RenderPass, T>::value);

			mPasses.push_back(std::make_unique<T>());
			mNamePassHandleMap[mPasses.back()->GetName()] = mPasses.size() - 1;
		}

	public:
		void Compile();
		void Execute(VkCommandBuffer commandBuffer, const RenderContext& renderContext);

	private:
		void SetupPassesAndRecordDependencies(RGResourceTouchList& resourceTouchList, RGPassTouchList& passTouchList);
		void BuildAdjacencyGraph(const RGResourceTouchList& resourceTouchList, RGPassAdjacencyGraph& passAdjacencyGraph, std::vector<RGPassHandle>& passInDegree);
		void CullPasses(const RGResourceTouchList& resourceTouchList, std::vector<bool>& neededPasses);
		void SortPasses(const std::vector<bool>& neededPasses, const RGPassAdjacencyGraph& passAdjacencyGraph, std::vector<RGPassHandle> passInDegree);

		void AllocateResources(const RGResourceTouchList& resourceTouchList);
		void BuildCompiledPasses(const RGResourceTouchList& resourceTouchList, const RGPassTouchList& passTouchList, const RGPassAdjacencyGraph& passAdjacencyGraph, const std::vector<bool>& neededPasses);

	private:
		RGResourceNameHandleMap mResourceNameHandleMap;
		RGClearColorMap mClearColorMap;

		RGResourceTouchList mResourceTouchList;
		RGPassTouchList mPassTouchList;
		RGPassAdjacencyGraph mPassAdjacencyGraph;
		std::vector<bool> mNeededPasses;
		std::vector<RGPassHandle> mPassInDegree;

		std::vector<RGResourceInfo> mResourceInfos;
		std::vector<std::unique_ptr<Texture>> mResources;

		std::unordered_map<std::string, RGPassHandle> mNamePassHandleMap;
		std::vector<std::unique_ptr<RenderPass>> mPasses;
		std::vector<RGPassHandle> mPassOrder;
		std::vector<RGCompiledPass> mCompiledPasses;

		PFN_vkCmdBeginRenderingKHR m_vkCmdBeginRenderingKHR;
		PFN_vkCmdEndRenderingKHR m_vkCmdEndRenderingKHR;
	};

}
