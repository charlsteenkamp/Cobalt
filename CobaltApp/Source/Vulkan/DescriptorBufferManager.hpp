#pragma once
#include "VulkanUtils.hpp"
#include "VulkanBuffer.hpp"
#include "Texture.hpp"
#include "DescriptorBindings.hpp"

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

		void WriteDescriptor(const DescriptorBinding& descriptorBinding, DescriptorHandle descriptorHandle);

	private:
		void WriteBufferDescriptor(DescriptorHandle descriptorHandle, uint32_t binding, uint32_t element, VkDescriptorType descriptorType, size_t descriptorSize, VkDeviceAddress address, VkDeviceSize range);
		void WriteImageDescriptor(DescriptorHandle descriptorHandle, uint32_t binding, uint32_t element, VkDescriptorType descriptorType, size_t descriptorSize, VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout);

	private:
		std::vector<DescriptorInfo> mDescriptorInfos;

		std::vector<DescriptorBuffer> mResourceDescriptorBuffers; // per frame-in-flight
		DescriptorBuffer mSamplerDescriptorBuffer;

		VkPhysicalDeviceDescriptorBufferPropertiesEXT mDescriptorBufferProperties;
	};

}

