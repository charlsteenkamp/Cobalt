#include "copch.hpp"
#include "Pipeline.hpp"
#include "GraphicsContext.hpp"
#include "Application.hpp"

namespace Cobalt
{

	Pipeline::Pipeline(const PipelineInfo& info)
		: mInfo(info)
	{
		CO_PROFILE_FN();

		Invalidate();
	}

	Pipeline::~Pipeline()
	{
		CO_PROFILE_FN();

		if (mPipelineLayout)
			vkDestroyPipelineLayout(GraphicsContext::Get().GetDevice(), mPipelineLayout, nullptr);

		if (mPipeline)
			vkDestroyPipeline(GraphicsContext::Get().GetDevice(), mPipeline, nullptr);
	}
	
	void Pipeline::Invalidate()
	{
		CO_PROFILE_FN();

		const std::vector<ShaderStage> shaderStages = mInfo.Shader->GetShaderStages();

		std::vector<VkShaderModule> shaderModuleHandles(shaderStages.size());
		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos(shaderStages.size());

		for (int32_t i = 0; i < shaderStages.size(); i++)
		{
			const ShaderStage& stage = shaderStages[i];

			VkShaderModuleCreateInfo shaderModuleCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = stage.SPIRV.size(),
				.pCode = (uint32_t*)stage.SPIRV.data()
			};

			VK_CALL(vkCreateShaderModule(GraphicsContext::Get().GetDevice(), &shaderModuleCreateInfo, nullptr, &shaderModuleHandles[i]));

			shaderStageCreateInfos[i] = VkPipelineShaderStageCreateInfo {
				.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.flags  = 0,
				.stage  = stage.Stage,
				.module = shaderModuleHandles[i],
				.pName  = stage.EntryPointName.c_str()
			};
		}

		//const auto& vertexInputBindingDescription = mInfo.Shader->GetVertexInputBindingDescription();
		//const auto& vertexInputAttributeDescriptions = mInfo.Shader->GetVertexInputAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.flags = 0
		};

#if 0
		if (!vertexInputAttributeDescriptions.empty())
		{
			vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
			vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
			vertexInputStateCreateInfo.vertexAttributeDescriptionCount = (uint32_t)vertexInputAttributeDescriptions.size();
			vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
		}
#endif

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.flags = 0,
			.topology = mInfo.PrimitiveTopology,
			.primitiveRestartEnable = VK_FALSE,
		};

		VkRect2D scissor = {
			.offset = {},
			.extent = { Application::Get()->GetWindow().GetWidth(), Application::Get()->GetWindow().GetHeight() }
		};

		VkViewport viewport = {
			.x = 0,
			.y = (float)scissor.extent.height,
			.width  = (float)scissor.extent.width,
			.height = -(float)scissor.extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.flags = 0,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor
		};

		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.flags = 0,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = mInfo.CullMode,
			.frontFace = mInfo.FrontFace,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.0f
		};

		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.flags = 0,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 1.0f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE
		};

		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.flags = 0,
			.depthTestEnable = mInfo.EnableDepthTesting ? VK_TRUE : VK_FALSE,
			.depthWriteEnable = mInfo.EnableDepthTesting ? VK_TRUE : VK_FALSE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f
		};

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates(mInfo.ColorAttachments.size());

		for (uint32_t i = 0; i < mInfo.ColorAttachments.size(); i++)
		{
			if (mInfo.ColorAttachments[i].Blend)
			{
				colorBlendAttachmentStates[i] = {
					.blendEnable = VK_TRUE,
					.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
					.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
					.colorBlendOp = VK_BLEND_OP_ADD,
					.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
					.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
					.alphaBlendOp = VK_BLEND_OP_ADD,
					.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
				};
			}
			else
			{
				colorBlendAttachmentStates[i] = {
					.blendEnable = VK_FALSE,
					.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT

				};
			}
		}

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.flags = 0,
			.logicOpEnable = VK_FALSE,
			.logicOp = {},
			.attachmentCount = (uint32_t)colorBlendAttachmentStates.size(),
			.pAttachments = colorBlendAttachmentStates.data(),
		};

		VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]),
			.pDynamicStates = dynamicStates
		};

		const auto& descriptorSetLayouts = mInfo.Shader->GetDescriptorSetLayouts();

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.flags = 0,
			.setLayoutCount = (uint32_t)descriptorSetLayouts.size(),
			.pSetLayouts = descriptorSetLayouts.data(),
		};

		VK_CALL(vkCreatePipelineLayout(GraphicsContext::Get().GetDevice(), &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout));

		std::vector<VkFormat> colorAttachmentFormats;
		colorAttachmentFormats.reserve(mInfo.ColorAttachments.size());

		for (auto [_, format] : mInfo.ColorAttachments)
			colorAttachmentFormats.push_back(format);

		VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
			.colorAttachmentCount = (uint32_t)mInfo.ColorAttachments.size(),
			.pColorAttachmentFormats = colorAttachmentFormats.data(),
			.depthAttachmentFormat = mInfo.DepthAttachmentFormat,
			.stencilAttachmentFormat = mInfo.StencilAttachmentFormat,
		};

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = &pipelineRenderingCreateInfo,
			.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
			.stageCount = (uint32_t)shaderStageCreateInfos.size(),
			.pStages = shaderStageCreateInfos.data(),
			.pVertexInputState = &vertexInputStateCreateInfo,
			.pInputAssemblyState = &inputAssemblyStateCreateInfo,
			.pViewportState = &viewportStateCreateInfo,
			.pRasterizationState = &rasterizationStateCreateInfo,
			.pMultisampleState = &multisampleStateCreateInfo,
			.pDepthStencilState = &depthStencilStateCreateInfo,
			.pColorBlendState = &colorBlendStateCreateInfo,
			.pDynamicState = &dynamicStateCreateInfo,
			.layout = mPipelineLayout,
//			.renderPass = mRenderPass,
			.renderPass = VK_NULL_HANDLE,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = 0
		};

		VK_CALL(vkCreateGraphicsPipelines(GraphicsContext::Get().GetDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mPipeline));
	}

}