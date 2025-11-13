#include "copch.hpp"
#include "VulkanDescriptorSet.hpp"
#include "GraphicsContext.hpp"

namespace Cobalt
{

	VulkanDescriptorSet::VulkanDescriptorSet(uint32_t setIndex, VkDescriptorSet descriptorSet, VkPipelineLayout pipelineLayout)
		: mSetIndex(setIndex), mDescriptorSet(descriptorSet), mPipelineLayout(pipelineLayout)
	{
		CO_PROFILE_FN();

		mDescriptorBufferInfos.reserve(10);
		mDescriptorImageInfos.reserve(100);
		mDescriptorWrites.reserve(100);
	}

	VulkanDescriptorSet::~VulkanDescriptorSet()
	{
		CO_PROFILE_FN();
	}

	void VulkanDescriptorSet::SetBufferBinding(const VulkanBuffer& buffer, uint32_t binding, uint32_t arrayIndex)
	{
		CO_PROFILE_FN();

		VkDescriptorBufferInfo descriptorBufferInfo = {
			.buffer = buffer.GetBuffer(),
			.offset = 0,
			.range = VK_WHOLE_SIZE
		};

		mDescriptorBufferInfos.push_back(descriptorBufferInfo);

		VkDescriptorType descriptorType;

		switch (buffer.GetUsageFlags())
		{
			case VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT: descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
			case VK_BUFFER_USAGE_STORAGE_BUFFER_BIT: descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
		}

		VkWriteDescriptorSet writeDescSet = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = mDescriptorSet,
			.dstBinding = binding,
			.dstArrayElement = arrayIndex,
			.descriptorCount = 1,
			.descriptorType = descriptorType,
			.pBufferInfo = &mDescriptorBufferInfos[mDescriptorBufferInfos.size() - 1],
		};

		mDescriptorWrites.push_back(writeDescSet);
	}

	void VulkanDescriptorSet::SetImageBinding(const Texture& image, uint32_t binding, uint32_t arrayIndex)
	{
		CO_PROFILE_FN();

		VkDescriptorImageInfo descImageInfo = {
			.sampler = image.GetSampler(),
			.imageView = image.GetImageView(),
			.imageLayout = image.GetImageLayout()
		};

		mDescriptorImageInfos.push_back(descImageInfo);

		VkWriteDescriptorSet writeDescSet = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = mDescriptorSet,
			.dstBinding = binding,
			.dstArrayElement = arrayIndex,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &mDescriptorImageInfos[mDescriptorImageInfos.size() - 1]
		};

		mDescriptorWrites.push_back(writeDescSet);
	}

	void VulkanDescriptorSet::Update()
	{
		CO_PROFILE_FN();

		vkUpdateDescriptorSets(GraphicsContext::Get().GetDevice(), (uint32_t)mDescriptorWrites.size(), mDescriptorWrites.data(), 0, nullptr);
	}

	void VulkanDescriptorSet::Bind(VkCommandBuffer commandBuffer)
	{
		CO_PROFILE_FN();

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, mSetIndex, 1, &mDescriptorSet, 0, nullptr);
	}

}