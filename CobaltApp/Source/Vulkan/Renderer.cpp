#include "copch.hpp"
#include "Renderer.hpp"
#include "Application.hpp"
#include "AssetManager.hpp"

#include <backends/imgui_impl_vulkan.h>

namespace Cobalt
{

	void Renderer::Init()
	{
		CO_PROFILE_FN();

		if (sData)
			return;

		sData = new RendererData();

		CreateOrRecreateDepthTexture();

		// Create the Render Pass

		{
#if 0
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = GraphicsContext::Get().GetSwapchain().GetSurfaceFormat().format;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = VK_FORMAT_D32_SFLOAT;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthAttachmentRef = {};
			depthAttachmentRef.attachment = 1;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
			VkAttachmentReference attachmentRefs[2] = { colorAttachmentRef, depthAttachmentRef };

			VkSubpassDescription subpass1 = {};
			subpass1.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass1.colorAttachmentCount = 1;
			subpass1.pColorAttachments = &colorAttachmentRef;
			subpass1.pDepthStencilAttachment = &depthAttachmentRef;

			VkSubpassDependency dependency1 = {};
			dependency1.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency1.dstSubpass = 0;
			dependency1.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependency1.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependency1.srcAccessMask = 0;
			dependency1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			VkSubpassDescription subpasses[1] = { subpass1 };
			VkSubpassDependency dependencies[1] = { dependency1 };

			VkRenderPassCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			info.attachmentCount = 2;
			info.pAttachments = attachments;
			info.subpassCount = 1;
			info.pSubpasses = subpasses;
			info.dependencyCount = 1;
			info.pDependencies = dependencies;

			VK_CALL(vkCreateRenderPass(GraphicsContext::Get().GetDevice(), &info, nullptr, &sData->MainRenderPass));
#else
			VkAttachmentDescription gBufferAttachment = {
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			VkAttachmentDescription depthAttachment = {
				.format = VK_FORMAT_D32_SFLOAT,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			};

			VkAttachmentDescription attachments[5] = { gBufferAttachment, gBufferAttachment, gBufferAttachment, gBufferAttachment, depthAttachment };

			VkAttachmentReference baseColorAttachmentRef = {
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};

			VkAttachmentReference normalAttachmentRef = {
				.attachment = 1,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};

			VkAttachmentReference occlusionRoughnessMetallicAttachmentRef = {
				.attachment = 2,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};

			VkAttachmentReference emissiveAttachmentRef = {
				.attachment = 3,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};

			VkAttachmentReference depthAttachmentRef = {
				.attachment = 4,
				.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			};

			VkAttachmentReference gBufferAttachmentRefs[4] = { baseColorAttachmentRef, normalAttachmentRef, occlusionRoughnessMetallicAttachmentRef, emissiveAttachmentRef };
			VkAttachmentReference attachmentRefs[5] = { baseColorAttachmentRef, normalAttachmentRef, occlusionRoughnessMetallicAttachmentRef, emissiveAttachmentRef, depthAttachmentRef };

			VkSubpassDescription geometrySubpassDesc = {
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 4,
				.pColorAttachments = gBufferAttachmentRefs,
				.pDepthStencilAttachment = &depthAttachmentRef
			};

			VkSubpassDependency geometryReadSubpassDep = {
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
//				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
			};

			VkSubpassDependency geometryWriteSubpassDep = {
				.srcSubpass = 0,
				.dstSubpass = VK_SUBPASS_EXTERNAL,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
//				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
			};

			VkSubpassDependency depthSubpassDep = {
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
//				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
			};

			VkSubpassDependency geometrySubpassDependencies[3] = { geometryReadSubpassDep, geometryWriteSubpassDep, depthSubpassDep };

			VkRenderPassCreateInfo geometryRenderPassCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
				.attachmentCount = 5,
				.pAttachments = attachments,
				.subpassCount = 1,
				.pSubpasses = &geometrySubpassDesc,
				.dependencyCount = 3,
				.pDependencies = geometrySubpassDependencies
			};

			VK_CALL(vkCreateRenderPass(GraphicsContext::Get().GetDevice(), &geometryRenderPassCreateInfo, nullptr, &sData->GeometryRenderPass));

			VkAttachmentDescription colorAttachment = {
				.format = GraphicsContext::Get().GetSwapchain().GetSurfaceFormat().format,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			};

			VkAttachmentReference colorAttachmentRef = {
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};

			VkSubpassDescription lightingSubpassDesc = {
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &colorAttachmentRef,
			};

			VkSubpassDependency lightingSubpassDep = {
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
			};

			VkRenderPassCreateInfo lightingRenderPassCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
				.attachmentCount = 1,
				.pAttachments = &colorAttachment,
				.subpassCount = 1,
				.pSubpasses = &lightingSubpassDesc,
				.dependencyCount = 1,
				.pDependencies = &lightingSubpassDep
			};

			VK_CALL(vkCreateRenderPass(GraphicsContext::Get().GetDevice(), &lightingRenderPassCreateInfo, nullptr, &sData->LightingRenderPass));

			// Create attachments

			uint32_t width  = GraphicsContext::Get().GetSwapchain().GetExtent().width;
			uint32_t height = GraphicsContext::Get().GetSwapchain().GetExtent().height;

			sData->BaseColorTexture                  = std::make_unique<Texture>(TextureInfo(width, height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
			sData->NormalTexture                     = std::make_unique<Texture>(TextureInfo(width, height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
			sData->OcclusionRoughnessMetallicTexture = std::make_unique<Texture>(TextureInfo(width, height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
			sData->EmissiveTexture                   = std::make_unique<Texture>(TextureInfo(width, height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
#endif
		}

		CreateOrRecreateFramebuffers();

		// Create scene & material uniform buffers & descriptors

		{
			// Uniform buffers

			uint32_t frameCount = GraphicsContext::Get().GetFrameCount();

			sData->SceneDataUniformBuffers.resize(frameCount);
			sData->ObjectStorageBuffers.resize(frameCount);
			sData->MaterialDataStorageBuffers.resize(frameCount);

			for (uint32_t i = 0; i < frameCount; i++)
			{
				sData->SceneDataUniformBuffers[i]    = VulkanBuffer::CreateMappedBuffer(sizeof(SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
				sData->ObjectStorageBuffers[i]       = VulkanBuffer::CreateMappedBuffer(sData->sMaxObjectCount * sizeof(ObjectData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
				sData->MaterialDataStorageBuffers[i] = VulkanBuffer::CreateMappedBuffer(sData->sMaxMaterialCount * sizeof(MaterialData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
			}

			sData->DrawCalls.reserve(sData->sMaxObjectCount);
			sData->Objects.reserve(sData->sMaxObjectCount);
			sData->Materials.reserve(sData->sMaxMaterialCount);
		}

		// Shader compilation & pipeline creation

		sData->Shaders = std::make_unique<ShaderLibrary>("CobaltApp/Assets/Shaders");
		/*sData->PBRShaderHandle = sData->Shaders->RegisterShader("Forward/PBRShader.slang");

		PipelineInfo mainPBRPipelineInfo = {
			.Shader = *sData->Shaders->GetShader(sData->PBRShaderHandle),
			.PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.EnableDepthTesting = true
		};

		sData->PBRPipeline = CreatePipeline(mainPBRPipelineInfo, sData->MainRenderPass);*/

		sData->GeometryPassShaderHandle = sData->Shaders->RegisterShader("Deferred/GeometryPass.slang");
		sData->LightingPassShaderHandle = sData->Shaders->RegisterShader("Deferred/LightingPass.slang");

		uint32_t frameCount = GraphicsContext::Get().GetFrameCount();

		PipelineInfo geometryPassPipelineInfo = {
			.Shader = *sData->Shaders->GetShader(sData->GeometryPassShaderHandle),
			.PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.EnableDepthTesting = true,
			.ColorAttachments = {
				{ false }, { false }, { false }, { false }
			}
		};

		sData->GeometryPassPipeline = std::make_unique<Pipeline>(geometryPassPipelineInfo, sData->GeometryRenderPass);
		sData->GeometryPassPipeline->AllocateDescriptorSets(GraphicsContext::Get().GetDescriptorPool(), 0, frameCount);

		for (uint32_t i = 0; i < frameCount; i++)
		{
			VulkanDescriptorSet* descriptorSet = sData->GeometryPassPipeline->GetDescriptorSet(i);
			descriptorSet->SetBufferBinding(*sData->SceneDataUniformBuffers[i], 0);
			descriptorSet->SetBufferBinding(*sData->ObjectStorageBuffers[i], 1);
			descriptorSet->SetBufferBinding(*sData->MaterialDataStorageBuffers[i], 2);

			descriptorSet->Update();
		}

		PipelineInfo lightingPassPipelineInfo = {
			.Shader = *sData->Shaders->GetShader(sData->LightingPassShaderHandle),
			.PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.EnableDepthTesting = false,
			.ColorAttachments = {
				{ true }
			}
		};

		sData->LightingPassPipeline = std::make_unique<Pipeline>(lightingPassPipelineInfo, sData->LightingRenderPass);
		sData->LightingPassPipeline->AllocateDescriptorSets(GraphicsContext::Get().GetDescriptorPool(), 0, frameCount);

		for (uint32_t i = 0; i < frameCount; i++)
		{
			VulkanDescriptorSet* descriptorSet = sData->LightingPassPipeline->GetDescriptorSet(i);
			descriptorSet->SetBufferBinding(*sData->SceneDataUniformBuffers[i], 0);
			descriptorSet->SetImageBinding(*sData->BaseColorTexture, 1);
			descriptorSet->SetImageBinding(*sData->NormalTexture, 2);
			descriptorSet->SetImageBinding(*sData->OcclusionRoughnessMetallicTexture, 3);
			descriptorSet->SetImageBinding(*sData->EmissiveTexture, 4);
			descriptorSet->Update();
		}
	}

	void Renderer::Shutdown()
	{
		CO_PROFILE_FN();

		// TODO
		/*vkDestroyRenderPass(GraphicsContext::Get().GetDevice(), sData->MainRenderPass, nullptr);

		for (VkFramebuffer framebuffer : sData->Framebuffers)
			vkDestroyFramebuffer(GraphicsContext::Get().GetDevice(), framebuffer, nullptr);*/
		
		delete sData;
		sData = nullptr;
	}

	void Renderer::OnResize()
	{
		CO_PROFILE_FN();

		CreateOrRecreateDepthTexture();
		CreateOrRecreateFramebuffers();
	}

	void Renderer::UploadTexture(const Texture& texture, const Pipeline& pipeline)
	{
		CO_PROFILE_FN();

		uint32_t frameCount = GraphicsContext::Get().GetFrameCount();

		for (uint32_t i = 0; i < frameCount; i++)
		{
			VulkanDescriptorSet* descriptorSet = pipeline.GetDescriptorSet(i);
			uint32_t textureIndex = descriptorSet->GetDescriptorImageCount();

			descriptorSet->SetImageBinding(texture, 3, textureIndex);
			descriptorSet->Update();
		}
	}

	void Renderer::UploadMaterial(Material& material)
	{
		CO_PROFILE_FN();

		if (material.mMaterialHandle == CO_INVALID_MATERIAL_HANDLE)
		{
			// Insert the material

			sData->Materials.push_back(material.GetMaterialData());
			material.mMaterialHandle = sData->Materials.size() - 1;
		}
		else
		{
			// Modify the material

			sData->Materials[material.mMaterialHandle] = material.GetMaterialData();
		}

		uint32_t frameCount = GraphicsContext::Get().GetFrameCount();

		for (uint32_t i = 0; i < frameCount; i++)
		{
			sData->MaterialDataStorageBuffers[i]->CopyData(sData->Materials.data(), sData->Materials.size() * sizeof(MaterialData));
		}
	}

	void Renderer::BeginScene(const SceneData& scene)
	{
		CO_PROFILE_FN();

		sData->Objects.clear();
		sData->DrawCalls.clear();

		sData->ActiveScene = scene;
	}

	void Renderer::EndScene()
	{
		CO_PROFILE_FN();

		const Swapchain& swapchain = GraphicsContext::Get().GetSwapchain();

		VkCommandBuffer commandBuffer = GraphicsContext::Get().GetActiveCommandBuffer();
		
		// Viewport & scissor

		VkExtent2D extent = GraphicsContext::Get().GetSwapchain().GetExtent();

#if 1
		VkViewport viewport = {
			.x = 0,
			.y = (float)extent.height,
			.width = (float)extent.width,
			.height = -(float)extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
#else
		VkViewport viewport = {
			.x = 0,
			.y = 0,
			.width = (float)extent.width,
			.height = (float)extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
#endif

		VkRect2D scissor = {
			.extent = extent
		};

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		// Begin geometry pass

		VkClearValue geometryPassClearValues[5] = {};
		geometryPassClearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		geometryPassClearValues[1].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		geometryPassClearValues[2].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		geometryPassClearValues[3].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		geometryPassClearValues[4].depthStencil = {1.0f, 0};

		VkRenderPassBeginInfo geometryPassBeginInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = sData->GeometryRenderPass,
			.framebuffer = sData->GeometryPassFramebuffers[swapchain.GetBackBufferIndex()],
			.renderArea = { .extent = swapchain.GetExtent() },
			.clearValueCount = 5,
			.pClearValues = geometryPassClearValues,
		};

		// Render objects

		uint32_t frameIndex = GraphicsContext::Get().GetFrameIndex();

		sData->SceneDataUniformBuffers[frameIndex]->CopyData(&sData->ActiveScene);
		sData->ObjectStorageBuffers[frameIndex]->CopyData(sData->Objects.data(), sData->Objects.size() * sizeof(ObjectData));

		CO_PROFILE_GPU_EVENT("Geometry Pass");

		vkCmdBeginRenderPass(commandBuffer, &geometryPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
		VkPipeline lastPipeline = VK_NULL_HANDLE;

		for (const DrawBatch& batch : BatchDrawCalls())
		{
			const VulkanBuffer& indexBuffer = *batch.IndexBuffer;
			const Pipeline&     pipeline    = batch.Material->GetPipeline();

			if (indexBuffer.GetBuffer() != lastIndexBuffer)
			{
				lastIndexBuffer = indexBuffer.GetBuffer();
				vkCmdBindIndexBuffer(commandBuffer, indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
			}

			if (pipeline.GetPipeline() != lastPipeline)
			{
				lastPipeline = pipeline.GetPipeline();
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetPipeline());
				pipeline.GetDescriptorSet(frameIndex)->Bind(commandBuffer);
			}

			vkCmdDrawIndexed(commandBuffer, batch.IndexCount, batch.InstanceCount, batch.FirstIndex, 0, batch.FirstInstance);
		}

		vkCmdEndRenderPass(commandBuffer);

		// Begin lighting pass

		CO_PROFILE_GPU_EVENT("Lighting pass");

		VkClearValue lightingPassClearValues[1];
		lightingPassClearValues[0].color = { {0.0f, 0.0f, 0.0f, 0.0f} };

		VkRenderPassBeginInfo lightingPassBeginInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = sData->LightingRenderPass,
			.framebuffer = sData->LightingPassFramebuffers[swapchain.GetBackBufferIndex()],
			.renderArea = { .extent = swapchain.GetExtent() },
			.clearValueCount = 1,
			.pClearValues = lightingPassClearValues,
		};	

		vkCmdBeginRenderPass(commandBuffer, &lightingPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sData->LightingPassPipeline->GetPipeline());
		sData->LightingPassPipeline->GetDescriptorSet(frameIndex)->Bind(commandBuffer);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(commandBuffer);
	}

	void Renderer::DrawMesh(const Transform& transform, const Mesh* mesh)
	{
		CO_PROFILE_FN();
		
		/*for (const MeshSurface& surface : mesh->GetSurfaces())
		{
			DrawCall draw;
			draw.IndexBuffer = mesh->GetIndexBuffer();
			draw.FirstIndex  = surface.FirstIndex;
			draw.IndexCount  = surface.IndexCount;
			draw.Material    = AssetManager::GetMaterial(surface.MaterialAssetHandle);

			sData->DrawCalls.push_back(draw);
		}*/

		/*DrawCall draw;
		draw.IndexBuffer = mesh->GetIndexBuffer();
		draw.FirstIndex  = 0;
		draw.IndexCount  = mesh->GetIndices().size();
		draw.Material    = mesh->GetMaterial();

		sData->DrawCalls.push_back(draw);*/

		sData->DrawCalls.emplace_back(mesh);

		/*ObjectData object;
		object.Transform       = transform.GetTransform();
		object.NormalMatrix    = glm::transpose(glm::inverse(object.Transform));
		object.VertexBufferRef = mesh->GetVertexBufferReference();
		object.MaterialHandle  = mesh->GetMaterial()->mMaterialHandle;

		sData->Objects.push_back(object);*/

		sData->Objects.emplace_back(transform.GetTransform(), mesh);
	}

	void Renderer::CreateOrRecreateDepthTexture()
	{
		CO_PROFILE_FN();

		uint32_t width  = GraphicsContext::Get().GetSwapchain().GetExtent().width;
		uint32_t height = GraphicsContext::Get().GetSwapchain().GetExtent().height;

		if (sData->DepthTexture)
			sData->DepthTexture->Recreate(width, height);
		else
			sData->DepthTexture = std::make_unique<Texture>(TextureInfo(width, height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT));
	}

	void Renderer::CreateOrRecreateFramebuffers()
	{
		CO_PROFILE_FN();

		if (!sData->GeometryPassFramebuffers.empty())
		{
			for (VkFramebuffer framebuffer : sData->GeometryPassFramebuffers)
				vkDestroyFramebuffer(GraphicsContext::Get().GetDevice(), framebuffer, nullptr);

			sData->GeometryPassFramebuffers.clear();
		}

		if (!sData->LightingPassFramebuffers.empty())
		{
			for (VkFramebuffer framebuffer : sData->LightingPassFramebuffers)
				vkDestroyFramebuffer(GraphicsContext::Get().GetDevice(), framebuffer, nullptr);

			sData->LightingPassFramebuffers.clear();
		}

		const Swapchain& swapchain = GraphicsContext::Get().GetSwapchain();

		sData->GeometryPassFramebuffers.resize(swapchain.GetBackBufferCount());
		sData->LightingPassFramebuffers.resize(swapchain.GetBackBufferCount());

		VkFramebufferCreateInfo geometryPassFramebufferCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.flags = 0,
			.renderPass = sData->GeometryRenderPass,
			.width = swapchain.GetExtent().width,
			.height = swapchain.GetExtent().height,
			.layers = 1,
		};

		VkFramebufferCreateInfo lightingPassFramebufferCreateInfo = geometryPassFramebufferCreateInfo;
		lightingPassFramebufferCreateInfo.renderPass = sData->LightingRenderPass;

		for (uint32_t i = 0; i < swapchain.GetBackBufferCount(); i++)
		{
			VkImageView geometryPassAttachments[5] = {
				sData->BaseColorTexture->GetImageView(),
				sData->NormalTexture->GetImageView(),
				sData->OcclusionRoughnessMetallicTexture->GetImageView(),
				sData->EmissiveTexture->GetImageView(),
				sData->DepthTexture->GetImageView()
			};

			geometryPassFramebufferCreateInfo.attachmentCount = 5;
			geometryPassFramebufferCreateInfo.pAttachments = geometryPassAttachments;

			VK_CALL(vkCreateFramebuffer(GraphicsContext::Get().GetDevice(), &geometryPassFramebufferCreateInfo, nullptr, &sData->GeometryPassFramebuffers[i]));

			VkImageView lightingPassAttachments[1] = { swapchain.GetBackBufferViews()[i] };

			lightingPassFramebufferCreateInfo.attachmentCount = 1;
			lightingPassFramebufferCreateInfo.pAttachments = lightingPassAttachments;

			VK_CALL(vkCreateFramebuffer(GraphicsContext::Get().GetDevice(), &lightingPassFramebufferCreateInfo, nullptr, &sData->LightingPassFramebuffers[i]));
		}
	}

#if 0
	Pipeline* Renderer::CreatePipeline(const PipelineInfo& info, VkRenderPass renderPass)
	{
		CO_PROFILE_FN();

		if (renderPass == VK_NULL_HANDLE)
			renderPass = sData->MainRenderPass;

		sData->Pipelines.push_back(std::make_unique<Pipeline>(info, renderPass));

		// Allocate descriptor sets & set buffer bindings

		Pipeline* pipeline = sData->Pipelines.back().get();
		uint32_t frameCount = GraphicsContext::Get().GetFrameCount();

		std::vector<VulkanDescriptorSet*> descriptorSets = pipeline->AllocateDescriptorSets(GraphicsContext::Get().GetDescriptorPool(), 0, frameCount);

		for (uint32_t i = 0; i < frameCount; i++)
		{
			descriptorSets[i]->SetBufferBinding(*sData->SceneDataUniformBuffers[i], sData->sDescriptorSetGlobalBinding);
			descriptorSets[i]->SetBufferBinding(*sData->ObjectStorageBuffers[i], sData->sDescriptorSetObjectBinding);
			descriptorSets[i]->SetBufferBinding(*sData->MaterialDataStorageBuffers[i], sData->sDescriptorSetMaterialBinding);

			descriptorSets[i]->Update();
		}

		return pipeline;
	}
#endif

	std::vector<DrawCall> Renderer::CullDrawCalls(const std::vector<DrawCall>& draws)
	{
		CO_PROFILE_FN();

		// TODO
		return draws;

#if 0
		std::vector<DrawCall> culledDraws;

		for (const DrawCall& draw : draws)
		{
			const ObjectData& object = sData->Objects[draw.FirstInstance];

			glm::mat4 mvp = sData->ActiveScene.Camera.ViewProjectionMatrix * object.Transform;

		}

		return culledDraws;
#endif
	}

	std::vector<DrawBatch> Renderer::BatchDrawCalls()
	{
		CO_PROFILE_FN();

		std::vector<DrawBatch> batches;
		batches.reserve(sData->DrawCalls.size());

		DrawBatch firstBatch;
		firstBatch.IndexBuffer   = sData->DrawCalls[0].IndexBuffer;
		firstBatch.FirstIndex    = sData->DrawCalls[0].FirstIndex;
		firstBatch.IndexCount    = sData->DrawCalls[0].IndexCount;
		firstBatch.Material      = sData->DrawCalls[0].Material;
		firstBatch.FirstInstance = 0;
		firstBatch.InstanceCount = 1;

		batches.push_back(firstBatch);

		for (uint32_t i = 1; i < sData->DrawCalls.size(); i++)
		{
			DrawBatch&      lastBatch = batches.back();
			const DrawCall& currDraw  = sData->DrawCalls[i];

			VkPipeline currPipeline = currDraw.Material->GetPipeline().GetPipeline();
			VkPipeline lastPipeline = lastBatch.Material->GetPipeline().GetPipeline();

			bool sameIndexBuffer = currDraw.IndexBuffer == lastBatch.IndexBuffer;
			bool samePipeline    = lastPipeline == currPipeline;

			if (sameIndexBuffer && samePipeline)
			{
				lastBatch.InstanceCount++;
			}
			else
			{
				DrawBatch newBatch;
				newBatch.IndexBuffer   = currDraw.IndexBuffer;
				newBatch.FirstIndex    = currDraw.FirstIndex;
				newBatch.IndexCount    = currDraw.IndexCount;
				newBatch.Material      = currDraw.Material;
				newBatch.FirstInstance = i;
				newBatch.InstanceCount = 1;

				batches.push_back(newBatch);
			}
		}

		return batches;
	}

}