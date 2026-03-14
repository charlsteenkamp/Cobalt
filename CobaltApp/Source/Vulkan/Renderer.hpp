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
#include "RenderGraph.h"

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

		VulkanBuffer* IndexBuffer = nullptr;
		uint32_t FirstIndex = 0;
		uint32_t IndexCount = 0;
		Material* Material = nullptr;
	};

	struct DrawBatch
	{
		VulkanBuffer* IndexBuffer = nullptr;
		uint32_t FirstIndex = 0;
		uint32_t IndexCount = 0;
		Material* Material = nullptr;
		uint32_t FirstInstance = 0;
		uint32_t InstanceCount = 0;
	};

	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnResize();

	public:
		//static VkRenderPass GetMainRenderPass() { return sData->MainRenderPass; }
		static const Pipeline& GetPBRPipeline() { return *sData->GeometryPassPipeline; }

	public:
		// Called by Assetmanager
		static void UploadTexture(const Texture& texture, const Pipeline& pipeline);

		// Called by AssetManager whenever a new material is registered, or somewhere else when an existing material's data changes
		static void UploadMaterial(Material& material);

	public:
		static void BeginScene(const SceneData& scene);
		static void EndScene();

		static void DrawMesh(const Transform& transform, const Mesh* mesh);

	private:
		static void CreateOrRecreateAttachments();
		static void CreateOrRecreateFramebuffers();

		static std::vector<DrawBatch> BatchDrawCalls();

	private:
		struct RendererData
		{
			//VkRenderPass MainRenderPass;
			VkRenderPass GeometryRenderPass, LightingRenderPass;

			std::unique_ptr<Texture> PositionTexture, BaseColorTexture, NormalTexture, OcclusionRoughnessMetallicTexture, EmissiveTexture;
			std::unique_ptr<Texture> DepthTexture;

			std::vector<VkFramebuffer> GeometryPassFramebuffers, LightingPassFramebuffers; // by backbuffer index

			SceneData ActiveScene;

			std::unique_ptr<ShaderLibrary> Shaders;
			ShaderHandle PBRShaderHandle;
			ShaderHandle GeometryPassShaderHandle;
			ShaderHandle LightingPassShaderHandle;

			//std::vector<std::unique_ptr<Pipeline>> Pipelines;
			//Pipeline* PBRPipeline = nullptr;
			std::unique_ptr<Pipeline> GeometryPassPipeline;
			std::unique_ptr<Pipeline> LightingPassPipeline;

			// per frame-in-flight
			std::vector<std::unique_ptr<VulkanBuffer>> SceneDataUniformBuffers;
			std::vector<std::unique_ptr<VulkanBuffer>> ObjectStorageBuffers;
			std::vector<std::unique_ptr<VulkanBuffer>> MaterialDataStorageBuffers;

			std::vector<ObjectData>   Objects;
			std::vector<MaterialData> Materials;

			std::vector<DescriptorHandle> GeometryPassDescriptorHandles;
			std::vector<DescriptorHandle> LightingPassDescriptorHandles;

			std::vector<Image> BindlessImages;

			static constexpr uint32_t sDescriptorSetGlobalBinding   = 0;
			static constexpr uint32_t sDescriptorSetObjectBinding   = 1;
			static constexpr uint32_t sDescriptorSetMaterialBinding = 2;
			static constexpr uint32_t sDescriptorSetTextureBinding  = 3;

			static constexpr uint32_t sMaxMaterialCount = 100;
			static constexpr uint32_t sMaxObjectCount   = 10000;
			
			//std::unordered_map<AssetHandle, MaterialHandle> AssetMaterialHandleMap;
			std::unique_ptr<RenderGraph> RenderGraph;

			std::vector<DrawCall> DrawCalls;
		};

		inline static RendererData* sData = nullptr;
	};

}
