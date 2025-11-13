#pragma once
#include "VulkanUtils.hpp"
#include "VulkanBuffer.hpp"
#include "Texture.hpp"

namespace Cobalt
{

	class VulkanDescriptorSet
	{
	public:
		VulkanDescriptorSet(uint32_t setIndex, VkDescriptorSet descriptorSet, VkPipelineLayout pipelineLayout);
		~VulkanDescriptorSet();

	public:
		void SetBufferBinding(const VulkanBuffer& buffer, uint32_t binding, uint32_t arrayIndex = 0);
		void SetImageBinding(const Texture& image, uint32_t binding, uint32_t arrayIndex = 0);
		void Update();

		void Bind(VkCommandBuffer commandBuffer);

	public:
		uint32_t GetDescriptorBufferBindingCount() const { return mDescriptorBufferInfos.size(); }
		uint32_t GetDescriptorImageCount()         const { return mDescriptorImageInfos.size();  }

	public:
		VkDescriptorSet GetDescriptorSet() const { return mDescriptorSet; }
		VkDescriptorSet* GetDescriptorSetPtr() { return &mDescriptorSet; }

	private:
		uint32_t mSetIndex = 0;

		VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
		VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;

		std::vector<VkDescriptorBufferInfo> mDescriptorBufferInfos;
		std::vector<VkDescriptorImageInfo> mDescriptorImageInfos;
		std::vector<VkWriteDescriptorSet> mDescriptorWrites;
	};

}
