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
		builder.AddDependency(mDepthStencilAttachment, RGAccessType::ReadWrite);

		auto& shaderLibrary = Renderer::GetShaderLibrary();
		mShader = shaderLibrary.GetShader("Deferred/GeometryPass.slang");

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
			}
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