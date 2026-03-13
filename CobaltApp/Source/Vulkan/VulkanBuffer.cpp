#include "copch.hpp"
#include "VulkanBuffer.hpp"
#include "GraphicsContext.hpp"
#include "VulkanCommands.hpp"


namespace Cobalt
{

	std::unique_ptr<VulkanBuffer> VulkanBuffer::CreateGPUBufferFromCPUData(const void* data, uint32_t size, VkBufferUsageFlags usage)
	{
		CO_PROFILE_FN();

		std::unique_ptr<VulkanBuffer> buffer = std::make_unique<VulkanBuffer>(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0);

		// Upload data to stagingBuffer

		std::unique_ptr<VulkanBuffer> stagingBuffer = CreateMappedBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		stagingBuffer->CopyData(data);

		// Copy stagingBuffer to buffer

		GraphicsContext::Get().SubmitSingleTimeCommands(GraphicsContext::Get().GetQueue(), [&](VkCommandBuffer commandBuffer)
		{
			VulkanCommands::CopyBuffer(commandBuffer, *stagingBuffer, *buffer);
		});

		return buffer;
	}

	std::unique_ptr<VulkanBuffer> VulkanBuffer::CreateMappedBuffer(uint32_t size, VkBufferUsageFlags usage)
	{
		CO_PROFILE_FN();

		return std::make_unique<VulkanBuffer>(size, usage, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
	}

	VulkanBuffer::VulkanBuffer(uint32_t size, VkBufferUsageFlags usage, VmaAllocationCreateFlags allocationFlags)
		: mUsage(usage)
	{
		CO_PROFILE_FN();

		VkBufferCreateInfo bufferCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.flags = 0,
			.size = size,
			.usage = mUsage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};

		VmaAllocationCreateInfo allocCreateInfo = {
			.flags = allocationFlags,
			.usage = VMA_MEMORY_USAGE_AUTO,
		};

		VK_CALL(vmaCreateBuffer(GraphicsContext::Get().GetAllocator(), &bufferCreateInfo, &allocCreateInfo, &mBuffer, &mAllocation, &mAllocationInfo));
	}

	VulkanBuffer::~VulkanBuffer()
	{
		CO_PROFILE_FN();

		vmaDestroyBuffer(GraphicsContext::Get().GetAllocator(), mBuffer, mAllocation);
	}

	void VulkanBuffer::Map(void** data)
	{
		CO_PROFILE_FN();

		VK_CALL(vmaMapMemory(GraphicsContext::Get().GetAllocator(), mAllocation, data));
	}

	void VulkanBuffer::Unmap()
	{
		CO_PROFILE_FN();

		vmaUnmapMemory(GraphicsContext::Get().GetAllocator(), mAllocation);
	}

	void VulkanBuffer::CopyData(const void* src, uint32_t size)
	{
		CO_PROFILE_FN();

		memcpy(mAllocationInfo.pMappedData, src, size == 0 ? mAllocationInfo.size : size);
	}

	VkDeviceAddress VulkanBuffer::GetDeviceAddress() const
	{
		CO_PROFILE_FN();

		VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = mBuffer
		};

		return vkGetBufferDeviceAddress(GraphicsContext::Get().GetDevice(), &bufferDeviceAddressInfo);
	}

}