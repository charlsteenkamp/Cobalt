#pragma once
#include "Shader.hpp"
#include "VulkanDescriptorSet.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace Cobalt
{

	struct ColorAttachment
	{
		bool Blend = true;
	};

	struct PipelineInfo
	{
		Shader& Shader;

		VkPrimitiveTopology PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkCullModeFlags CullMode = VK_CULL_MODE_NONE;
		VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		bool EnableDepthTesting = true;

		// TODO: add more options e.g. color format
		std::vector<ColorAttachment> ColorAttachments;
	};

	class Pipeline
	{
	public:
		Pipeline(const PipelineInfo& info, VkRenderPass renderPass);
		~Pipeline();

		// Called automatically
		void Invalidate();

	public:
		// allocate `count` descriptor sets of set `set`
		std::vector<VulkanDescriptorSet*> AllocateDescriptorSets(VkDescriptorPool descriptorPool, uint32_t set, uint32_t count = 1);

	public:
		VkPipeline GetPipeline() const { return mPipeline; }
		VkPipelineLayout GetPipelineLayout() const { return mPipelineLayout; }

		VulkanDescriptorSet* GetDescriptorSet(uint32_t frameIndex) const
		{
			if (frameIndex >= mDescriptorSets.size())
				return nullptr;

			return mDescriptorSets[frameIndex].get();
		}

	private:
		PipelineInfo mInfo;
		VkRenderPass mRenderPass;

		VkPipeline mPipeline;
		VkPipelineLayout mPipelineLayout;

		std::vector<std::unique_ptr<VulkanDescriptorSet>> mDescriptorSets;
	};

}
