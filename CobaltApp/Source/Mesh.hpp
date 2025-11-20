#pragma once
#include "Vulkan/VulkanBuffer.hpp"
#include "Asset.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace Cobalt
{

	struct MeshVertex
	{
		glm::vec4 Position{};
		glm::vec4 Normal{};
		glm::vec4 Tangent{};
		glm::vec4 UV{};
	};

	class Material;

	struct MeshInfo
	{
		const std::vector<MeshVertex>& Vertices;
		const std::vector<uint32_t>&   Indices;

		Material* MaterialRef; // non-owning pointer
	};

	class Mesh
	{
	public:
		Mesh(const MeshInfo& meshInfo);
		~Mesh();

	public:
		void SetMaterial(Material* material) { mMaterialRef = material; }

	public:
		const std::vector<MeshVertex>& GetVertices()  const { return mVertices; }
		const std::vector<uint32_t>&   GetIndices()   const { return mIndices;  }

		Material* GetMaterial() const { return mMaterialRef; }
		//const std::vector<MeshSurface>& GetSurfaces() const { return mSurfaces; }

		VulkanBuffer* GetVertexBuffer() const { return mVertexBuffer.get(); }
		VulkanBuffer* GetIndexBuffer()  const { return mIndexBuffer.get();  }

		VkDeviceAddress GetVertexBufferReference() const { return mVertexBuffer->GetDeviceAddress(); }

	private:
		std::vector<MeshVertex> mVertices;
		std::vector<uint32_t> mIndices;
		//std::vector<MeshSurface> mSurfaces;
		Material* mMaterialRef;

		std::unique_ptr<VulkanBuffer> mVertexBuffer;
		std::unique_ptr<VulkanBuffer> mIndexBuffer;
	};

}
