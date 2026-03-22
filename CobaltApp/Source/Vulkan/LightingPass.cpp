#include "copch.hpp"
#include "LightingPass.hpp"
#include "GraphicsContext.hpp"
#include "Renderer.hpp"
#include "RenderGraph.hpp"

namespace Cobalt
{

	LightingPass::LightingPass()
		: RenderPass("Lighting Pass", "Deferred\\LightingPass.slang", (RenderPassFlags)RenderPassFlagBits::SideAffect)
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

		auto& shaderLibrary = Renderer::GetShaderLibrary();
		auto& pipelineRegistry = GraphicsContext::Get().GetPipelineRegistry();
		auto& descriptorBufferManager = GraphicsContext::Get().GetDescriptorBufferManager();

		// Build pipelines

#if CO_ENABLE_LIGHTING_PASS_SKYBOX
		PipelineInfo skyboxPipelineInfo = {
			.Shader = shaderLibrary.GetShader("Deferred\\Skybox.slang"),
			.EnableDepthTesting = false,
			.ColorAttachments = {
				{ true, GraphicsContext::Get().GetSwapchain().GetSurfaceFormat().format }
			}
		};

		mSkyboxPipeline = pipelineRegistry.BuildPipeline("Skybox", skyboxPipelineInfo);
#endif

		PipelineInfo lightingPassPipelineInfo = {
			.Shader = Renderer::GetShaderLibrary().GetShader("Deferred\\LightingPass.slang"),
			.PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.CullMode = VK_CULL_MODE_NONE,
			.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.EnableDepthTesting = false,
			.ColorAttachments = {
				{ true, GraphicsContext::Get().GetSwapchain().GetSurfaceFormat().format }
			}
		};

		mLightingPipeline = pipelineRegistry.BuildPipeline(mName, lightingPassPipelineInfo);

		// Allocate descriptors

		uint32_t frameCount = GraphicsContext::Get().GetFrameCount();
#if CO_ENABLE_LIGHTING_PASS_SKYBOX
		mSkyboxDescriptors.resize(frameCount);
#endif
		mLightingDescriptors.resize(frameCount);

		for (uint32_t i = 0; i < frameCount; i++)
		{
#if CO_ENABLE_LIGHTING_PASS_SKYBOX
			mSkyboxDescriptors[i]   = descriptorBufferManager.AllocateDescriptor(skyboxPipelineInfo.Shader->GetDescriptorSetLayouts()[0], true, true);
#endif
			mLightingDescriptors[i] = descriptorBufferManager.AllocateDescriptor(lightingPassPipelineInfo.Shader->GetDescriptorSetLayouts()[0], true, true);
		}

		// Create skybox resources

#if CO_ENABLE_LIGHTING_PASS_SKYBOX
		mSkyboxViewProjectionBuffers.resize(GraphicsContext::Get().GetFrameCount());

		for (auto& buffer : mSkyboxViewProjectionBuffers)
			buffer = VulkanBuffer::CreateMappedBuffer(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

		CubemapFacePaths cubemapFacePaths;

		mSkybox = std::make_unique<Cubemap>(cubemapFacePaths);
#endif
	}

	void LightingPass::Execute(VkCommandBuffer commandBuffer, const RenderContext& renderContext)
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

		// Render skybox

#if CO_ENABLE_LIGHTING_PASS_SKYBOX
		ShaderCursor skyboxShaderCursor(mSkyboxPipeline->GetInfo().Shader->GetRootShaderParameter(), mSkyboxDescriptors[frameIndex]);
		skyboxShaderCursor.Field("uniforms")
			.WriteField("ViewProjection", *mSkyboxViewProjectionBuffers[frameIndex])
			.WriteField("Vertices", );
		skyboxShaderCursor.WriteField("skybox", *mSkybox);
		skyboxShaderCursor.Finalize();
#endif

		// Execute lighting pass

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