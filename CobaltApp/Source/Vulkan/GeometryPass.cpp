#include "copch.hpp"
#include "GeometryPass.hpp"
#include "Renderer.hpp"

namespace Cobalt
{

	GeometryPass::GeometryPass()
		: RenderPass("Geometry Pass", false)
	{
		CO_PROFILE_FN();
	}

	GeometryPass::~GeometryPass()
	{
		CO_PROFILE_FN();
	}

	void GeometryPass::Setup(RenderGraphBuilder& builder)
	{
		CO_PROFILE_FN();

		mPositionAttachment  = builder.DeclareResource("Position Attachment", { RGResourceType::ColorAttachment });
		mBaseColorAttachment = builder.DeclareResource("Base Color Attachment", { RGResourceType::ColorAttachment });
		mNormalAttachment    = builder.DeclareResource("Normal Attachment", { RGResourceType::ColorAttachment });
		mOCRAttachment       = builder.DeclareResource("OCR Attachment", { RGResourceType::ColorAttachment });
		mEmissiveAttachment  = builder.DeclareResource("Emissive Attachment", { RGResourceType::ColorAttachment });
		mDepthStencilAttachment = builder.DeclareResource("Depth Stencil Attachment", { RGResourceType::DepthAttachment });

		builder.AddDependency(mPositionAttachment, RGAccessType::ColorAttachmentWrite);
		builder.AddDependency(mBaseColorAttachment, RGAccessType::ColorAttachmentWrite);
		builder.AddDependency(mNormalAttachment, RGAccessType::ColorAttachmentWrite);
		builder.AddDependency(mOCRAttachment, RGAccessType::ColorAttachmentWrite);
		builder.AddDependency(mEmissiveAttachment, RGAccessType::ColorAttachmentWrite);
		builder.AddDependency(mDepthStencilAttachment, RGAccessType::DepthAttachment);

		builder.SetClearColor(mPositionAttachment);
		builder.SetClearColor(mBaseColorAttachment);
		builder.SetClearColor(mNormalAttachment);
		builder.SetClearColor(mOCRAttachment);
		builder.SetClearColor(mEmissiveAttachment);

		mShader = Renderer::GetShaderLibrary().GetShader("Deferred\\GeometryPass.slang");

		uint32_t frameCount = GraphicsContext::Get().GetFrameCount();

		PipelineInfo pipelineInfo = {
			.Shader = *mShader,
			.PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.CullMode = VK_CULL_MODE_NONE,
			.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.EnableDepthTesting = true,
			.ColorAttachments = {
				{ false, VK_FORMAT_R32G32B32A32_SFLOAT },
				{ false, VK_FORMAT_R32G32B32A32_SFLOAT },
				{ false, VK_FORMAT_R32G32B32A32_SFLOAT },
				{ false, VK_FORMAT_R32G32B32A32_SFLOAT },
				{ false, VK_FORMAT_R32G32B32A32_SFLOAT }
			},
			.DepthAttachmentFormat = VK_FORMAT_D32_SFLOAT
		};

		mPipeline = std::make_unique<Pipeline>(pipelineInfo);

		auto& descriptorBufferManager = GraphicsContext::Get().GetDescriptorBufferManager();
		mDescriptorHandles.resize(frameCount);

		for (uint32_t i = 0; i < frameCount; i++)
		{
			mDescriptorHandles[i] = descriptorBufferManager.AllocateDescriptor(mShader->GetDescriptorSetLayouts()[0], true, true);
		}
	}

	void GeometryPass::Execute(VkCommandBuffer commandBuffer, RenderFrameContext renderContext)
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

		ShaderCursor shaderCursor(mShader->GetRootShaderParameter(), mDescriptorHandles[frameIndex]);
		shaderCursor.Field("scene").Write(renderContext.SceneBuffer);
		shaderCursor.Field("objects").Write(renderContext.ObjectBuffer);
		shaderCursor.Field("materials").Write(renderContext.MaterialBuffer);
		shaderCursor.Field("textures").Write(renderContext.BindlessImages);
		shaderCursor.Finalize();

		Renderer::DrawObjects(commandBuffer, mDescriptorHandles[frameIndex], *mPipeline);
	}

}