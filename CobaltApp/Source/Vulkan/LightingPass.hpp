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
		void Execute(VkCommandBuffer commandBuffer, const RenderContext& renderContext) override;

	private:
		Shader* mShader = nullptr;
		Pipeline* mPipeline = nullptr;

		std::vector<DescriptorHandle> mDescriptorHandles;

		RGResourceHandle mPositionAttachment, mBaseColorAttachment, mNormalAttachment, mOCRAttachment, mEmissiveAttachment;
	};

}
