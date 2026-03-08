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

		std::unordered_map<uint32_t, VkDeviceSize> BindingOffsets;
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
		void WriteBufferDescriptor(const DescriptorBinding& descriptorBinding, const DescriptorInfo& descriptorInfo, VkDeviceSize bindingOffset, size_t descriptorSize);
		void WriteImageDescriptor(const DescriptorBinding& descriptorBinding, const DescriptorInfo& descriptorInfo, VkDeviceSize bindingOffset, size_t descriptorSize);

	private:
		std::vector<DescriptorInfo> mDescriptorInfos;

		DescriptorBuffer mResourceDescriptorBuffer;
		DescriptorBuffer mSamplerDescriptorBuffer;

		VkPhysicalDeviceDescriptorBufferPropertiesEXT mDescriptorBufferProperties;
	};

}

