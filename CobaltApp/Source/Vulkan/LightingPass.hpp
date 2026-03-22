#pragma once
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "Pipeline.hpp"
#include "ShaderCursor.hpp"
#include "Texture.hpp"

#include <vector>

namespace Cobalt
{

	class LightingPass : public RenderPass
	{
	public:
		LightingPass();
		~LightingPass();

	public:
		void SetSkybox(const Cubemap* skybox, const Mesh* skyboxMesh);

	public:
		void Setup(RenderGraphBuilder& builder) override;
		void Execute(VkCommandBuffer commandBuffer, const RenderContext& renderContext) override;

	private:
		void ExecuteSkyboxPass(VkCommandBuffer commandBuffer, const RenderContext& renderContext);
		void ExecuteLightingPass(VkCommandBuffer commandBuffer, const RenderContext& renderContext);

	private:
		DescriptorBufferManager& mDescriptorBufferManager;

		RGResourceHandle mPositionAttachment, mBaseColorAttachment, mNormalAttachment, mOCRAttachment, mEmissiveAttachment;

		Pipeline* mSkyboxPipeline = nullptr;
		Pipeline* mLightingPipeline = nullptr;

		std::vector<DescriptorHandle> mSkyboxDescriptors;
		std::vector<DescriptorHandle> mLightingDescriptors;

		struct SkyboxUniformBuffer
		{
			glm::mat4 ProjectionMatrix;
			glm::mat4 ViewMatrix;
			VkDeviceAddress Vertices;

			float __padding[2];
		};

		std::vector<std::unique_ptr<VulkanBuffer>> mSkyboxUniformBuffers;
		std::unique_ptr<VulkanBuffer> mSkyboxCube;
		const Cubemap* mSkybox = nullptr;
		const Mesh* mSkyboxMesh = nullptr;
	};

}
