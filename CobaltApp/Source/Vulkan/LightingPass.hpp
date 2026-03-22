#pragma once
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "Pipeline.hpp"
#include "ShaderCursor.hpp"
#include "Texture.hpp"

#include <vector>

#define CO_ENABLE_LIGHTING_PASS_SKYBOX 0

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
		RGResourceHandle mPositionAttachment, mBaseColorAttachment, mNormalAttachment, mOCRAttachment, mEmissiveAttachment;

		Pipeline* mLightingPipeline = nullptr;

		std::vector<DescriptorHandle> mLightingDescriptors;

#if CO_ENABLE_LIGHTING_PASS_SKYBOX
		Pipeline* mSkyboxPipeline = nullptr;
		std::vector<DescriptorHandle> mSkyboxDescriptors;
		std::vector<std::unique_ptr<VulkanBuffer>> mSkyboxViewProjectionBuffers;
		std::unique_ptr<Cubemap> mSkybox;
#endif
	};

}
