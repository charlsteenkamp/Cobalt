#pragma once
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <memory>

namespace Cobalt
{

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDeviceHandle);

	class VulkanBuffer
	{
	public:
		// Copies data to a staging buffer and then to a device local gpu buffer
		static std::unique_ptr<VulkanBuffer> CreateGPUBufferFromCPUData(const void* data, uint32_t size, VkBufferUsageFlags usage);
		static std::unique_ptr<VulkanBuffer> CreateMappedBuffer(uint32_t size, VkBufferUsageFlags usage);

	public:
		VulkanBuffer(uint32_t size, VkBufferUsageFlags usage, VmaAllocationCreateFlags allocationFlags);
		~VulkanBuffer();

	public:
		void Map(void** data);
		void Unmap();

		void CopyData(const void* src, size_t dstOffset = 0, uint32_t size = 0);

		VkDeviceAddress GetDeviceAddress() const;

	public:
		VkBuffer GetBuffer() const { return mBuffer; }
		//const VkMemoryRequirements& GetMemoryRequirements() const { return mMemoryRequirements; }
		const VmaAllocationInfo& GetAllocationInfo() const { return mAllocationInfo; }
		VkBufferUsageFlags GetUsageFlags() const { return mUsage; }

	private:
		VkBuffer mBuffer = VK_NULL_HANDLE;
		VkBufferUsageFlags mUsage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;

		VmaAllocation mAllocation;
		VmaAllocationInfo mAllocationInfo;
	};

}