#pragma once
#include "VulkanUtils.hpp"
#include "VulkanBuffer.hpp"
#include "Texture.hpp"

#include <map>
#include <functional>

namespace Cobalt
{

	struct DescriptorInfo
	{
		VkDescriptorSetLayout Layout   = VK_NULL_HANDLE;

		VkDeviceSize LayoutSize        = 0;
		VkDeviceSize ResourceSetOffset = 0;
		VkDeviceSize SamplerSetOffset  = 0;
	};

	using DescriptorHandle = uint32_t;

	struct DescriptorBuffer
	{
		std::unique_ptr<VulkanBuffer> Buffer;
		VkDeviceSize Offset = 0;
	};

	class DescriptorBufferManager
	{
	public:
		DescriptorBufferManager();
		~DescriptorBufferManager();

	public:
		DescriptorHandle AllocateDescriptor(VkDescriptorSetLayout descriptorSetLayout, bool resourceDescriptor, bool samplerDescriptor);

		void WriteDescriptor(const VulkanBuffer& buffer, DescriptorHandle descriptorHandle, uint32_t binding, uint32_t element = 0);
		void WriteDescriptor(const Texture& image, DescriptorHandle descriptorHandle, uint32_t binding, uint32_t element = 0);

	private:
		std::vector<DescriptorInfo> mDescriptorInfos;

		std::vector<DescriptorBuffer> mResourceDescriptorBuffers; // per frame-in-flight
		DescriptorBuffer mSamplerDescriptorBuffer;

		VkPhysicalDeviceDescriptorBufferPropertiesEXT mDescriptorBufferProperties;
	};

}

