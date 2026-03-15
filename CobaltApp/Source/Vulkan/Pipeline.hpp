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
		VkFormat Format;
	};

	struct PipelineInfo
	{
		const Shader& Shader;

		VkPrimitiveTopology PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkCullModeFlags CullMode = VK_CULL_MODE_NONE;
		VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		bool EnableDepthTesting = true;

		std::vector<ColorAttachment> ColorAttachments;
		VkFormat DepthAttachmentFormat = VK_FORMAT_UNDEFINED;
	};

	class Pipeline
	{
	public:
		Pipeline(const PipelineInfo& info);
		~Pipeline();

		// Called automatically
		void Invalidate();

	public:
		VkPipeline GetPipeline() const { return mPipeline; }
		VkPipelineLayout GetPipelineLayout() const { return mPipelineLayout; }
	
	private:
		PipelineInfo mInfo;

		VkPipeline mPipeline;
		VkPipelineLayout mPipelineLayout;
	};

}
