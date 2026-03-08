#pragma once
#include "Shader.hpp"
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
		VkPipeline GetPipeline() const { return mPipeline; }
		VkPipelineLayout GetPipelineLayout() const { return mPipelineLayout; }
	
	private:
		PipelineInfo mInfo;
		VkRenderPass mRenderPass;

		VkPipeline mPipeline;
		VkPipelineLayout mPipelineLayout;
	};

}
