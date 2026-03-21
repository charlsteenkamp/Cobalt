#include "copch.hpp"
#include "GeometryPass.hpp"
#include "Renderer.hpp"

namespace Cobalt
{

	GeometryPass::GeometryPass()
		: RenderPass("Geometry Pass", "Deferred\\GeometryPass.slang", (RenderPassFlags)RenderPassFlagBits::MeshPass)
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
	}

	void GeometryPass::Execute(VkCommandBuffer commandBuffer, const RenderContext& renderContext)
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

		const ShaderEffect* lastShaderEffect = nullptr;

		for (const auto& draw : renderContext.DrawCalls)
		{
			const ShaderEffect* shaderEffect = draw.Material->GetShaderEffect();

			if (shaderEffect == lastShaderEffect)
				continue;

			lastShaderEffect = shaderEffect;

			ShaderCursor shaderCursor = shaderEffect->GetShaderCursor(mName, frameIndex);
			shaderCursor.Field("scene").Write(*renderContext.SceneBuffer);
			shaderCursor.Field("objects").Write(*renderContext.ObjectBuffer);
			shaderCursor.Field("materials").Write(*renderContext.PackedMaterialBuffer);
			shaderCursor.Field("textures").Write(draw.Material->GetMaterialInfo().SampledTextures);
			shaderCursor.Finalize();
		}

		Renderer::DrawObjects(commandBuffer, mName, renderContext);
	}

}