#include "copch.hpp"
#include "Mesh.hpp"
#include "Vulkan/GraphicsContext.hpp"
#include "Vulkan/Renderer.hpp"
#include "Material.hpp"

namespace Cobalt
{

	Mesh::Mesh(const MeshInfo& meshInfo)
		: mVertices(meshInfo.Vertices), mIndices(meshInfo.Indices), mMaterialRef(meshInfo.MaterialRef)
	{
		CO_PROFILE_FN();

		mVertexBuffer = VulkanBuffer::CreateGPUBufferFromCPUData(mVertices.data(), sizeof(MeshVertex) * mVertices.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
		mIndexBuffer  = VulkanBuffer::CreateGPUBufferFromCPUData(mIndices.data(), sizeof(uint32_t) * mIndices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	}

	Mesh::~Mesh()
	{
		CO_PROFILE_FN();
	}

}