#include "copch.hpp"
#include "LightingPass.hpp"
#include "GraphicsContext.hpp"
#include "Renderer.hpp"
#include "RenderGraph.hpp"
#include "VulkanCommands.hpp"

namespace Cobalt
{

	LightingPass::LightingPass()
		: RenderPass("Lighting Pass", "Deferred\\LightingPass.slang", (RenderPassFlags)RenderPassFlagBits::SideAffect),
		mDescriptorBufferManager(GraphicsContext::Get().GetDescriptorBufferManager())
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

		// Build pipeline

		PipelineInfo lightingPassPipelineInfo = {
			.Shader = Renderer::GetShaderLibrary().GetShader("Deferred\\LightingPass.slang"),
			.EnableDepthTesting = false,
			.ColorAttachments = {
				{ true, GraphicsContext::Get().GetSwapchain().GetSurfaceFormat().format }
			}
		};

		mLightingPipeline = GraphicsContext::Get().GetPipelineRegistry().BuildPipeline("Lighting", lightingPassPipelineInfo);

		// Allocate descriptors

		uint32_t frameCount = GraphicsContext::Get().GetFrameCount();

		mLightingDescriptors.resize(frameCount);

		for (uint32_t i = 0; i < frameCount; i++)
			mLightingDescriptors[i] = mDescriptorBufferManager.AllocateDescriptor(lightingPassPipelineInfo.Shader->GetDescriptorSetLayouts()[0], true, true);
	}

	void LightingPass::SetSkybox(const Cubemap* skybox, const Mesh* skyboxMesh)
	{
		CO_PROFILE_FN();

		mSkybox = skybox;
		mSkyboxMesh = skyboxMesh;

		float vertices[36 * 4] = {
			-0.5f, -0.5f, -0.5f, 1.0f,
			 0.5f, -0.5f, -0.5f, 1.0f,
			 0.5f,  0.5f, -0.5f, 1.0f,
			 
			 0.5f,  0.5f, -0.5f, 1.0f,
			-0.5f,  0.5f, -0.5f, 1.0f,
			-0.5f, -0.5f, -0.5f, 1.0f,

			 0.5f, -0.5f, -0.5f, 1.0f,
			 0.5f, -0.5f,  0.5f, 1.0f,
			 0.5f,  0.5f,  0.5f, 1.0f,
			 
			 0.5f,  0.5f,  0.5f, 1.0f,
			 0.5f,  0.5f, -0.5f, 1.0f,
			 0.5f, -0.5f, -0.5f, 1.0f,

			 -0.5f, -0.5f,  0.5f, 1.0f,
			 -0.5f, -0.5f, -0.5f, 1.0f,
			 -0.5f,  0.5f, -0.5f, 1.0f,

			 -0.5f,  0.5f, -0.5f, 1.0f,
			 -0.5f,  0.5f,  0.5f, 1.0f,
			 -0.5f, -0.5f,  0.5f, 1.0f,

			 -0.5f, -0.5f,  0.5f, 1.0f,
			  0.5f, -0.5f,  0.5f, 1.0f,
			  0.5f,  0.5f,  0.5f, 1.0f,
			 
			  0.5f,  0.5f,  0.5f, 1.0f,
			 -0.5f,  0.5f,  0.5f, 1.0f,
			 -0.5f, -0.5f,  0.5f, 1.0f,

			 -0.5f, -0.5f,  0.5f, 1.0f,
			  0.5f, -0.5f,  0.5f, 1.0f,
			  0.5f, -0.5f, -0.5f, 1.0f,

			  0.5f, -0.5f, -0.5f, 1.0f,
			 -0.5f, -0.5f, -0.5f, 1.0f,
			 -0.5f, -0.5f,  0.5f, 1.0f,

			 -0.5f, 0.5f,  0.5f, 1.0f,
			  0.5f, 0.5f,  0.5f, 1.0f,
			  0.5f, 0.5f, -0.5f, 1.0f,

			  0.5f, 0.5f, -0.5f, 1.0f,
			 -0.5f, 0.5f, -0.5f, 1.0f,
			 -0.5f, 0.5f,  0.5f, 1.0f,
		};
		
		mSkyboxCube = VulkanBuffer::CreateGPUBufferFromCPUData(vertices, sizeof(vertices), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

		// Build skybox pipeline

		PipelineInfo skyboxPipelineInfo = {
			.Shader = Renderer::GetShaderLibrary().GetShader("Deferred\\Skybox.slang"),
			.CullMode = VK_CULL_MODE_NONE,
			.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.EnableDepthTesting = false,
			.ColorAttachments = {
				{ true, GraphicsContext::Get().GetSwapchain().GetSurfaceFormat().format }
			}
		};

		mSkyboxPipeline = GraphicsContext::Get().GetPipelineRegistry().BuildPipeline("Skybox", skyboxPipelineInfo);

		// Allocate buffers & descriptors

		uint32_t frameCount = GraphicsContext::Get().GetFrameCount();

		mSkyboxUniformBuffers.resize(frameCount);
		mSkyboxDescriptors.resize(frameCount);

		for (uint32_t i = 0; i < frameCount; i++)
		{
			mSkyboxUniformBuffers[i] = VulkanBuffer::CreateMappedBuffer(sizeof(SkyboxUniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
			mSkyboxDescriptors[i] = mDescriptorBufferManager.AllocateDescriptor(skyboxPipelineInfo.Shader->GetDescriptorSetLayouts()[0], true, true);
		}
	}

	void LightingPass::Execute(VkCommandBuffer commandBuffer, const RenderContext& renderContext)
	{
		CO_PROFILE_FN();

		VulkanCommands::SetViewport(commandBuffer, GraphicsContext::Get().GetSwapchain().GetExtent());

		if (mSkybox)
			ExecuteSkyboxPass(commandBuffer, renderContext);

		ExecuteLightingPass(commandBuffer, renderContext);
	}

	void LightingPass::ExecuteSkyboxPass(VkCommandBuffer commandBuffer, const RenderContext& renderContext)
	{
		CO_PROFILE_FN();

		uint32_t frameIndex = GraphicsContext::Get().GetFrameIndex();

		SkyboxUniformBuffer skyboxUniformBufferData{};
		skyboxUniformBufferData.ProjectionMatrix = renderContext.ProjectionMatrix;
		skyboxUniformBufferData.ViewMatrix = renderContext.ViewMatrix;
		//skyboxUniformBufferData.Vertices = mSkyboxMesh->GetVertexBuffer()->GetDeviceAddress();
		skyboxUniformBufferData.Vertices = mSkyboxCube->GetDeviceAddress();

		mSkyboxUniformBuffers[frameIndex]->CopyData(&skyboxUniformBufferData, sizeof(SkyboxUniformBuffer));

		ShaderCursor skyboxShaderCursor(mSkyboxPipeline->GetInfo().Shader->GetRootShaderParameter(), mSkyboxDescriptors[frameIndex]);
		skyboxShaderCursor.WriteField("uniforms", *mSkyboxUniformBuffers[frameIndex]);
		skyboxShaderCursor.WriteField("skybox", *mSkybox);
		skyboxShaderCursor.Finalize();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSkyboxPipeline->GetPipeline());
		mDescriptorBufferManager.SetDescriptorBufferOffsets(commandBuffer, mSkyboxPipeline->GetPipelineLayout(), mSkyboxDescriptors[frameIndex]);
		vkCmdDrawIndexed(commandBuffer, mSkyboxMesh->GetIndices().size(), 1, 0, 0, 0);
		vkCmdDraw(commandBuffer, 36, 1, 0, 0);
	}

	void LightingPass::ExecuteLightingPass(VkCommandBuffer commandBuffer, const RenderContext& renderContext)
	{
		CO_PROFILE_FN();

		uint32_t frameIndex = GraphicsContext::Get().GetFrameIndex();
		auto& descriptorBufferManager = GraphicsContext::Get().GetDescriptorBufferManager();
		auto& renderGraph = Renderer::GetRenderGraph();

		ShaderCursor lightingPassShaderCursor(mLightingPipeline->GetInfo().Shader->GetRootShaderParameter(), mLightingDescriptors[frameIndex]);
		lightingPassShaderCursor.Field("scene").Write(*renderContext.SceneBuffer);
		lightingPassShaderCursor.Field("gBuffers")
			.WriteField("SamplerPosition", renderGraph.GetResource(mPositionAttachment))
			.WriteField("SamplerBaseColor", renderGraph.GetResource(mBaseColorAttachment))
			.WriteField("SamplerNormal", renderGraph.GetResource(mNormalAttachment))
			.WriteField("SamplerOcclusionRoughnessMetallic", renderGraph.GetResource(mOCRAttachment))
			.WriteField("SamplerEmissive", renderGraph.GetResource(mEmissiveAttachment));
		lightingPassShaderCursor.Finalize();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mLightingPipeline->GetPipeline());
		descriptorBufferManager.SetDescriptorBufferOffsets(commandBuffer, mLightingPipeline->GetPipelineLayout(), mLightingDescriptors[frameIndex]);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	}

}