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

		VkDeviceSize LayoutSize = 0;
		VkDeviceSize SetOffset = 0;

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

		void BindDescriptorBuffers(VkCommandBuffer commandBuffer);
		void SetDescriptorBufferOffsets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, DescriptorHandle descriptorHandle);

	private:
		std::vector<DescriptorInfo> mDescriptorInfos;
		DescriptorBuffer mDescriptorBuffer;

		VkPhysicalDeviceDescriptorBufferPropertiesEXT mDescriptorBufferProperties{};

		PFN_vkGetDescriptorSetLayoutSizeEXT          vkGetDescriptorSetLayoutSizeEXT;
		PFN_vkGetDescriptorSetLayoutBindingOffsetEXT vkGetDescriptorSetLayoutBindingOffsetEXT;
		PFN_vkGetDescriptorEXT                       vkGetDescriptorEXT;
		PFN_vkCmdBindDescriptorBuffersEXT            vkCmdBindDescriptorBuffersEXT;
		PFN_vkCmdSetDescriptorBufferOffsetsEXT       vkCmdSetDescriptorBufferOffsetsEXT;
	};

}
