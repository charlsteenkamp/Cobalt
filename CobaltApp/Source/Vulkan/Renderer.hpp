#pragma once
#include "GraphicsContext.hpp"
#include "ShaderStructs.hpp"
#include "ShaderLibrary.hpp"
#include "Pipeline.hpp"
#include "VulkanBuffer.hpp"
#include "Texture.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "Asset.hpp"
#include "DescriptorBindings.hpp"
#include "RenderGraphBuilder.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <array>
#include <memory>

namespace Cobalt
{

	struct Transform
	{
		glm::vec3 Translation = glm::vec3(0.0f);
		glm::vec3 Rotation = glm::vec3(0.0f);
		glm::vec3 Scale = glm::vec3(1.0f);

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

			return glm::translate(glm::mat4(1.0f), Translation) * rotation * glm::scale(glm::mat4(1.0f), Scale);
		}
	};

	struct DrawCall
	{
		DrawCall(const Mesh* mesh)
			: IndexBuffer(mesh->GetIndexBuffer()), IndexCount(mesh->GetIndices().size()), Material(mesh->GetMaterial())
		{
		}

		const VulkanBuffer* IndexBuffer = nullptr;
		uint32_t FirstIndex = 0;
		uint32_t IndexCount = 0;
		const Material* Material = nullptr;
	};

	struct DrawBatch
	{
		const VulkanBuffer* IndexBuffer = nullptr;
		uint32_t FirstIndex = 0;
		uint32_t IndexCount = 0;
		const ShaderEffect* Effect = nullptr;
		uint32_t FirstInstance = 0;
		uint32_t InstanceCount = 0;
	};

	struct RenderContext
	{
		VulkanBuffer* SceneBuffer;
		VulkanBuffer* ObjectBuffer;
		VulkanBuffer* PackedMaterialBuffer;

		GPUScene ActiveScene;
		std::vector<GPUObject> GPUObjects;
		std::vector<DrawCall> DrawCalls;

		glm::mat4 ProjectionMatrix;
		glm::mat4 ViewMatrix;
	};

	class RenderGraph;
	class MaterialSystem;

	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnResize();

	public:
		static ShaderLibrary& GetShaderLibrary() { return *sData->mShaderLibrary; }
		static MaterialSystem& GetMaterialSystem() { return *sData->mMaterialSystem; }
		static RenderGraph& GetRenderGraph() { return *sData->mRenderGraph; }

		static const RenderContext& GetRenderContext() { return sData->mRenderContext; }

	public:
		static void BeginScene(const GPUScene& scene, const glm::mat4& projectionMat, const glm::mat4& viewMat);
		static void EndScene();

		static void DrawMesh(const Transform& transform, const Mesh* mesh);

	public:
		static void DrawObjects(VkCommandBuffer commandBuffer, const std::string& passName, const RenderContext& renderContext);

	private:
		static std::vector<DrawBatch> BatchDrawCalls(const std::string& passName, const RenderContext& renderContext);

	private:
		struct RendererData
		{
			// per frame-in-flight
			std::vector<std::unique_ptr<VulkanBuffer>> SceneBuffers;
			std::vector<std::unique_ptr<VulkanBuffer>> ObjectBuffers;
			std::vector<std::unique_ptr<VulkanBuffer>> PackedMaterialBuffers;

			static constexpr uint32_t sMaxObjectCount = 10000;
			
			std::unique_ptr<ShaderLibrary> mShaderLibrary;
			std::unique_ptr<RenderGraph> mRenderGraph;
			std::unique_ptr<MaterialSystem> mMaterialSystem;

			RenderContext mRenderContext;
		};

		inline static RendererData* sData = nullptr;
	};

}
