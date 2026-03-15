#pragma once
#include "RenderGraph.hpp"
#include "Renderer.hpp"

namespace Cobalt
{

	class RenderPass
	{
	public:
		RenderPass(const std::string& name, bool sideAffect)
			: mName(name), mHasSideAffect(sideAffect)
		{
		}

		virtual ~RenderPass() = default;

	public:
		virtual void Setup(RenderGraphBuilder& builder) = 0;
		virtual void Execute(VkCommandBuffer commandBuffer, RenderFrameContext renderContext) = 0;

	public:
		const std::string& GetName() const { return mName; }
		bool HasSideAffect() const { return mHasSideAffect; }

	protected:
		std::string mName;
		bool mHasSideAffect = true;
	};

}
