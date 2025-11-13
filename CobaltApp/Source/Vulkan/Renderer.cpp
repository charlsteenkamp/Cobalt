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

			VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment};
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

			sData->Objects.reserve(sData->sMaxObjectCount);
			sData->Materials.reserve(sData->sMaxMaterialCount);
		}

		// Shader compilation & pipeline creation

		sData->Shaders = std::make_unique<ShaderLibrary>("CobaltApp/Assets/Shaders");
		sData->PBRShaderHandle = sData->Shaders->RegisterShader("PBRShader.slang");

		PipelineInfo mainPBRPipelineInfo = {
			.Shader = *sData->Shaders->GetShader(sData->PBRShaderHandle),
			.PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.EnableDepthTesting = true
		};

		sData->PBRPipeline = CreatePipeline(mainPBRPipelineInfo, sData->MainRenderPass);
	}

	void Renderer::Shutdown()
	{
		CO_PROFILE_FN();

		vkDestroyRenderPass(GraphicsContext::Get().GetDevice(), sData->MainRenderPass, nullptr);

		for (VkFramebuffer framebuffer : sData->Framebuffers)
			vkDestroyFramebuffer(GraphicsContext::Get().GetDevice(), framebuffer, nullptr);
		
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

			descriptorSet->SetImageBinding(texture, sData->sDescriptorSetTextureBinding, textureIndex);
			descriptorSet->Update();
		}
	}

	void Renderer::UploadMaterial(AssetHandle assetHandle, const Material& material)
	{
		CO_PROFILE_FN();

		if (sData->AssetMaterialHandleMap.contains(assetHandle))
		{
			// Modify the material

			MaterialHandle materialHandle = sData->AssetMaterialHandleMap.at(assetHandle);
			sData->Materials[materialHandle] = material.GetMaterialData();
		}
		else
		{
			// Insert the material

			sData->Materials.push_back(material.GetMaterialData());
			sData->AssetMaterialHandleMap[assetHandle] = sData->Materials.size() - 1;
		}

		uint32_t frameCount = GraphicsContext::Get().GetFrameCount();

		for (uint32_t i = 0; i < frameCount; i++)
		{
			sData->MaterialDataStorageBuffers[i]->CopyData(sData->Materials.data(), sData->Materials.size() * sizeof(MaterialData));
		}
	}

#if 0
	TextureHandle Renderer::CreateTexture(const TextureInfo& textureInfo)
	{
		CO_PROFILE_FN();

		TextureHandle textureHandle = sData->Textures.size();
		sData->Textures.push_back(std::make_unique<Texture>(textureInfo));

		return textureHandle;
	}

	Texture& Renderer::GetTexture(TextureHandle textureHandle)
	{
		CO_PROFILE_FN();

		return *sData->Textures[textureHandle];
	}
#endif

#if 0
	std::unique_ptr<Material> CreateMaterial(const MaterialData& materialData)
	{
		CO_PROFILE_FN();

		size_t materialHash = std::hash<MaterialData>{}(materialData);

		if (sData->MaterialHandleMap.contains(materialHash))
		{
			MaterialHandle materialHandle = sData->MaterialHandleMap.at(materialHash);
			return sData->MaterialPtrs[materialHandle];
		}

		MaterialHandle materialHandle = sData->Materials.size();

		sData->Materials.push_back(materialData);

		//std::shared_ptr<Material> material = std::make_shared<Material>(materialHandle, &sData->Materials[materialHandle]);

		sData->MaterialPtrs.push_back(material);
		sData->MaterialHandleMap[materialHash] = materialHandle;

		for (uint32_t i = 0; i < GraphicsContext::Get().GetFrameCount(); i++)
		{
			// Copy material data to ssbo

			sData->MaterialDataStorageBuffers[i]->CopyData(sData->Materials.data(), sData->Materials.size() * sizeof(MaterialData));

			// Update descriptor bindings

			VulkanDescriptorSet* globalDescriptorSet = material->GetGlobalDescriptorSet(i);
			globalDescriptorSet->SetBufferBinding(sData->SceneDataUniformBuffers[i].get(), 0);
			globalDescriptorSet->SetBufferBinding(sData->ObjectStorageBuffers[i].get(), 1);
			globalDescriptorSet->SetBufferBinding(sData->MaterialDataStorageBuffers[i].get(), 2);

			for (uint32_t j = 0; j < sData->Textures.size(); j++)
				globalDescriptorSet->SetImageBinding(sData->Textures[j].get(), 3, j);

			globalDescriptorSet->Update();
		}

		return material;
	}
#endif

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

		// Begin render pass

		VkClearValue clearValues[2] = {};
		clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clearValues[1].depthStencil = {1.0f, 0};

		VkRenderPassBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = sData->MainRenderPass,
			.framebuffer = sData->Framebuffers[swapchain.GetBackBufferIndex()],
			.renderArea = { .extent = swapchain.GetExtent() },
			.clearValueCount = 2,
			.pClearValues = clearValues,
		};

		// Render objects

		uint32_t frameIndex = GraphicsContext::Get().GetFrameIndex();

		sData->SceneDataUniformBuffers[frameIndex]->CopyData(&sData->ActiveScene);
		sData->ObjectStorageBuffers[frameIndex]->CopyData(sData->Objects.data(), sData->Objects.size() * sizeof(ObjectData));

		CO_PROFILE_GPU_EVENT("Main Render Pass");

		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

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
				//batch.Material->GetGlobalDescriptorSet(frameIndex)->Bind(commandBuffer);
				pipeline.GetDescriptorSet(frameIndex)->Bind(commandBuffer);
			}

			vkCmdDrawIndexed(commandBuffer, batch.IndexCount, batch.InstanceCount, batch.FirstIndex, 0, batch.FirstInstance);
		}

		vkCmdEndRenderPass(commandBuffer);
	}

	void Renderer::DrawMesh(const Transform& transform, Mesh* mesh)
	{
		CO_PROFILE_FN();
		
		for (const MeshSurface& surface : mesh->GetSurfaces())
		{
			DrawCall draw;
			draw.IndexBuffer = mesh->GetIndexBuffer();
			draw.FirstIndex  = surface.FirstIndex;
			draw.IndexCount  = surface.IndexCount;
			draw.Material    = AssetManager::GetMaterial(surface.MaterialAssetHandle);

			sData->DrawCalls.push_back(draw);
		}

		ObjectData object;
		object.Transform       = transform.GetTransform();
		object.NormalMatrix    = glm::transpose(glm::inverse(object.Transform));
		object.VertexBufferRef = mesh->GetVertexBufferReference();

		sData->Objects.push_back(object);
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

		if (!sData->Framebuffers.empty())
		{
			for (VkFramebuffer framebuffer : sData->Framebuffers)
				vkDestroyFramebuffer(GraphicsContext::Get().GetDevice(), framebuffer, nullptr);

			sData->Framebuffers.clear();
		}

		const Swapchain& swapchain = GraphicsContext::Get().GetSwapchain();

		sData->Framebuffers.resize(swapchain.GetBackBufferCount());

		VkFramebufferCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.flags = 0,
			.renderPass = sData->MainRenderPass,
			.width = swapchain.GetExtent().width,
			.height = swapchain.GetExtent().height,
			.layers = 1,
		};

		for (uint32_t i = 0; i < swapchain.GetBackBufferCount(); i++)
		{
			VkImageView attachments[2] = { swapchain.GetBackBufferViews()[i], sData->DepthTexture->GetImageView() };
			createInfo.attachmentCount = 2;
			createInfo.pAttachments = attachments;

			VK_CALL(vkCreateFramebuffer(GraphicsContext::Get().GetDevice(), &createInfo, nullptr, &sData->Framebuffers[i]));
		}
	}

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

			// Image bindings are set in the `UploadTexture` method.

			//for (uint32_t j = 0; j < sData->Textures.size(); j++)
			//	descriptorSets[i]->SetImageBinding(sData->Textures[j].get(), 3, j);

			descriptorSets[i]->Update();
		}

		return pipeline;
	}

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
				//lastBatch.IndexCount += currDraw.IndexCount;
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