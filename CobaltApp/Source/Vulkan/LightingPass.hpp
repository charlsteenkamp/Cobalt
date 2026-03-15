#pragma once
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "Pipeline.hpp"
#include "ShaderCursor.hpp"

namespace Cobalt
{

	class LightingPass : public RenderPass
	{
	public:
		LightingPass();
		~LightingPass();

	public:
		void Setup(RenderGraphBuilder& builder) override;
		void Execute(VkCommandBuffer commandBuffer, RenderFrameContext renderContext) override;

	private:
		Shader* mShader = nullptr;

		std::unique_ptr<Pipeline> mPipeline;

		std::vector<DescriptorHandle> mDescriptorHandles;

		RGResourceHandle mPositionAttachment, mBaseColorAttachment, mNormalAttachment, mOCRAttachment, mEmissiveAttachment;
	};

}
