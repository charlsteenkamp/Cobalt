#include "copch.hpp"
#include "Renderer.hpp"
#include "Application.hpp"
#include "AssetManager.hpp"
#include "ShaderCursor.hpp"
#include "DescriptorBufferManager.hpp"
#include "RenderGraph.hpp"
#include "MaterialSystem.hpp"

#include "GeometryPass.hpp"
#include "LightingPass.hpp"

#include <backends/imgui_impl_vulkan.h>

namespace Cobalt
{

	void Renderer::Init()
	{
		CO_PROFILE_FN();

		if (sData)
			return;

		sData = new RendererData();

		// Uniform buffers

		uint32_t frameCount = GraphicsContext::Get().GetFrameCount();

		sData->SceneBuffers.resize(frameCount);
		sData->ObjectBuffers.resize(frameCount);
		sData->PackedMaterialBuffers.resize(frameCount);

		for (uint32_t i = 0; i < frameCount; i++)
		{
			sData->SceneBuffers[i]          = VulkanBuffer::CreateMappedBuffer(sizeof(GPUScene), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
			sData->ObjectBuffers[i]         = VulkanBuffer::CreateMappedBuffer(sData->sMaxObjectCount * sizeof(GPUObject), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
			sData->PackedMaterialBuffers[i] = VulkanBuffer::CreateMappedBuffer(sData->sMaxMaterialCount * sizeof(GPUPackedMaterial), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
		}

		sData->mRenderContext.GPUObjects.reserve(sData->sMaxObjectCount);
		sData->mRenderContext.DrawCalls.reserve(sData->sMaxObjectCount);

		// Shader compilation

		sData->mShaderLibrary = std::make_unique<ShaderLibrary>("CobaltApp/Assets/Shaders");

		sData->mRenderGraph = std::make_unique<RenderGraph>();
		sData->mRenderGraph->AddPass<GeometryPass>();
		sData->mRenderGraph->AddPass<LightingPass>();
		sData->mRenderGraph->Compile();

		sData->mMaterialSystem = std::make_unique<MaterialSystem>(*sData->mRenderGraph, *sData->mShaderLibrary, GraphicsContext::Get().GetPipelineRegistry());
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

		// TODO:
		//sData->mRenderGraph->OnResize();
	}

	void Renderer::BeginScene(const GPUScene& scene)
	{
		CO_PROFILE_FN();

		uint32_t frameIndex = GraphicsContext::Get().GetFrameIndex();

		sData->mRenderContext.SceneBuffer = sData->SceneBuffers[frameIndex].get();
		sData->mRenderContext.ObjectBuffer = sData->ObjectBuffers[frameIndex].get();
		sData->mRenderContext.PackedMaterialBuffer = sData->PackedMaterialBuffers[frameIndex].get();

		sData->mRenderContext.ActiveScene = scene;

		sData->mRenderContext.GPUObjects.clear();
		sData->mRenderContext.DrawCalls.clear();
	}

	void Renderer::EndScene()
	{
		CO_PROFILE_FN();

		VkCommandBuffer commandBuffer = GraphicsContext::Get().GetActiveCommandBuffer();

		DescriptorBufferManager& descriptorBufferManager = GraphicsContext::Get().GetDescriptorBufferManager();
		descriptorBufferManager.BindDescriptorBuffers(commandBuffer);

		const auto& packedMaterials = sData->mMaterialSystem->GetGPUPackedMaterials();

		sData->mRenderContext.SceneBuffer->CopyData(&sData->mRenderContext.ActiveScene);
		sData->mRenderContext.ObjectBuffer->CopyData(sData->mRenderContext.GPUObjects.data(), sData->mRenderContext.GPUObjects.size() * sizeof(GPUObject));
		sData->mRenderContext.PackedMaterialBuffer->CopyData(packedMaterials.data(), packedMaterials.size() * sizeof(GPUPackedMaterial));

		sData->mRenderGraph->Execute(commandBuffer, sData->mRenderContext);
	}

	void Renderer::DrawObjects(VkCommandBuffer commandBuffer, const std::string& passName, const RenderContext& renderContext)
	{
		CO_PROFILE_FN();

		uint32_t frameIndex = GraphicsContext::Get().GetFrameIndex();
		auto& descriptorBufferManager = GraphicsContext::Get().GetDescriptorBufferManager();

		VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
		VkPipeline lastPipeline = VK_NULL_HANDLE;

		std::vector<DrawBatch> batches = BatchDrawCalls(passName, renderContext);

		for (const DrawBatch& batch : batches)
		{
			const VulkanBuffer& indexBuffer = *batch.IndexBuffer;
			const Pipeline& pipeline = *batch.Effect->PassPipelines.at(passName);
			DescriptorHandle descriptorHandle = batch.Effect->PassDescriptors.at(passName)[frameIndex];

			if (indexBuffer.GetBuffer() != lastIndexBuffer)
			{
				lastIndexBuffer = indexBuffer.GetBuffer();
				vkCmdBindIndexBuffer(commandBuffer, indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
			}

			if (pipeline.GetPipeline() != lastPipeline)
			{
				lastPipeline = pipeline.GetPipeline();
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetPipeline());
				descriptorBufferManager.SetDescriptorBufferOffsets(commandBuffer, pipeline.GetPipelineLayout(), descriptorHandle);
			}

			vkCmdDrawIndexed(commandBuffer, batch.IndexCount, batch.InstanceCount, batch.FirstIndex, 0, batch.FirstInstance);
		}
	}

	void Renderer::DrawMesh(const Transform& transform, const Mesh* mesh)
	{
		CO_PROFILE_FN();
		
		sData->mRenderContext.DrawCalls.emplace_back(mesh);
		sData->mRenderContext.GPUObjects.emplace_back(transform.GetTransform(), mesh);
	}

	std::vector<DrawBatch> Renderer::BatchDrawCalls(const std::string& passName, const RenderContext& renderContext)
	{
		CO_PROFILE_FN();

		std::vector<DrawBatch> batches;
		batches.reserve(renderContext.DrawCalls.size());

		DrawBatch firstBatch;
		firstBatch.IndexBuffer   = renderContext.DrawCalls[0].IndexBuffer;
		firstBatch.FirstIndex    = renderContext.DrawCalls[0].FirstIndex;
		firstBatch.IndexCount    = renderContext.DrawCalls[0].IndexCount;
		firstBatch.Effect        = renderContext.DrawCalls[0].Material->GetShaderEffect();
		firstBatch.FirstInstance = 0;
		firstBatch.InstanceCount = 1;

		batches.push_back(firstBatch);

		for (uint32_t i = 1; i < renderContext.DrawCalls.size(); i++)
		{
			DrawBatch&      lastBatch = batches.back();
			const DrawCall& currDraw  = renderContext.DrawCalls[i];

			VkPipeline currPipeline = currDraw.Material->GetShaderEffect()->PassPipelines.at(passName)->GetPipeline();
			VkPipeline lastPipeline = lastBatch.Effect->PassPipelines.at(passName)->GetPipeline();

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
				newBatch.Effect        = currDraw.Material->GetShaderEffect();
				newBatch.FirstInstance = i;
				newBatch.InstanceCount = 1;

				batches.push_back(newBatch);
			}
		}

		return batches;
	}

}