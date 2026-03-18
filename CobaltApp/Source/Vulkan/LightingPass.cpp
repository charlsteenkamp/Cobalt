#include "copch.hpp"
#include "LightingPass.hpp"
#include "GraphicsContext.hpp"
#include "Renderer.hpp"
#include "RenderGraph.hpp"

namespace Cobalt
{

	LightingPass::LightingPass()
		: RenderPass("Lighting Pass", true)
	{
		CO_PROFILE_FN();
	}

	LightingPass::~LightingPass()
	{
		CO_PROFILE_FN();
	}

	void LightingPass::Setup(RenderGraphBuilder& builder)
	{
		CO_PROFILE_FN();

		mPositionAttachment  = builder.GetResource("Position Attachment");
		mBaseColorAttachment = builder.GetResource("Base Color Attachment");
		mNormalAttachment    = builder.GetResource("Normal Attachment");
		mOCRAttachment       = builder.GetResource("OCR Attachment");
		mEmissiveAttachment  = builder.GetResource("Emissive Attachment");

		auto backbufferAttachment = builder.GetResource("BackBuffer Attachment");

		builder.AddDependency(mPositionAttachment, RGAccessType::ShaderRead);
		builder.AddDependency(mBaseColorAttachment, RGAccessType::ShaderRead);
		builder.AddDependency(mNormalAttachment, RGAccessType::ShaderRead);
		builder.AddDependency(mOCRAttachment, RGAccessType::ShaderRead);
		builder.AddDependency(mEmissiveAttachment, RGAccessType::ShaderRead);
		builder.AddDependency(backbufferAttachment, RGAccessType::Present);

		mShader = Renderer::GetShaderLibrary().GetShader("Deferred\\LightingPass.slang");

		PipelineInfo lightingPassPipelineInfo = {
			.Shader = *mShader,
			.PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.CullMode = VK_CULL_MODE_NONE,
			.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.EnableDepthTesting = true,
			.ColorAttachments = {
				{ true, GraphicsContext::Get().GetSwapchain().GetSurfaceFormat().format }
			}
		};

		mPipeline = std::make_unique<Pipeline>(lightingPassPipelineInfo);

		auto& descriptorBufferManager = GraphicsContext::Get().GetDescriptorBufferManager();

		uint32_t frameCount = GraphicsContext::Get().GetFrameCount();
		mDescriptorHandles.resize(frameCount);

		for (uint32_t i = 0; i < frameCount; i++)
		{
			mDescriptorHandles[i] = descriptorBufferManager.AllocateDescriptor(mShader->GetDescriptorSetLayouts()[0], true, true);
		}
	}

	void LightingPass::Execute(VkCommandBuffer commandBuffer, RenderFrameContext renderContext)
	{
		CO_PROFILE_FN();

		VkExtent2D extent = GraphicsContext::Get().GetSwapchain().GetExtent();

		VkViewport viewport = {
			.x = 0,
			.y = (float)extent.height,
			.width = (float)extent.width,
			.height = -(float)extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		VkRect2D scissor = {
			.extent = extent
		};

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		uint32_t frameIndex = GraphicsContext::Get().GetFrameIndex();
		auto& descriptorBufferManager = GraphicsContext::Get().GetDescriptorBufferManager();
		auto& renderGraph = Renderer::GetRenderGraph();

		ShaderCursor lightingPassShaderCursor(mShader->GetRootShaderParameter(), mDescriptorHandles[frameIndex]);
		lightingPassShaderCursor.Field("scene").Write(renderContext.SceneBuffer);
		lightingPassShaderCursor.Field("gBuffers")
			.WriteField("SamplerPosition", renderGraph.GetResource(mPositionAttachment))
			.WriteField("SamplerBaseColor", renderGraph.GetResource(mBaseColorAttachment))
			.WriteField("SamplerNormal", renderGraph.GetResource(mNormalAttachment))
			.WriteField("SamplerOcclusionRoughnessMetallic", renderGraph.GetResource(mOCRAttachment))
			.WriteField("SamplerEmissive", renderGraph.GetResource(mEmissiveAttachment));
		lightingPassShaderCursor.Finalize();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline->GetPipeline());
		descriptorBufferManager.SetDescriptorBufferOffsets(commandBuffer, mPipeline->GetPipelineLayout(), mDescriptorHandles[frameIndex]);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	}

}