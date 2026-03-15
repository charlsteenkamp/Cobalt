#pragma once
#include "RenderPass.hpp"
#include "Pipeline.hpp"
#include "Shader.hpp"
#include "ShaderCursor.hpp"

#include <vector>

namespace Cobalt
{

	class GeometryPass : public RenderPass
	{
	public:
		GeometryPass();
		~GeometryPass();

	public:
		void Setup(RenderGraphBuilder& builder) override;
		void Execute(VkCommandBuffer commandBuffer, RenderFrameContext renderContext) override;

	private:
		Shader* mShader = nullptr;

		std::unique_ptr<Pipeline> mPipeline;

		std::vector<DescriptorHandle> mDescriptorHandles;

		RGResourceHandle mPositionAttachment, mBaseColorAttachment, mNormalAttachment, mOCRAttachment, mEmissiveAttachment;
		RGResourceHandle mDepthStencilAttachment;
	};

}
