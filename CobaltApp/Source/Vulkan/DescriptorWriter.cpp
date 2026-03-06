#include "copch.hpp"
#include "DescriptorWriter.hpp"
#include "GraphicsContext.hpp"

namespace Cobalt
{

	DescriptorWriter::DescriptorWriter(VkDescriptorSetLayout descriptorSetLayout)
		: mSignature(descriptorSetLayout)
	{
		CO_PROFILE_FN();
	}

	DescriptorWriter::~DescriptorWriter()
	{
		CO_PROFILE_FN();
	}

	void DescriptorWriter::WriteBuffer(const VulkanBuffer& buffer, uint32_t binding, uint32_t arrayIndex /*= 0*/)
	{
		CO_PROFILE_FN();

		VkDescriptorType descriptorType;

		switch (buffer.GetUsageFlags())
		{
			case VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT: descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
			case VK_BUFFER_USAGE_STORAGE_BUFFER_BIT: descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
		}

		mSignature.Buffers.push_back(BufferBindingKey {
			.Buffer = buffer.GetBuffer(),
			.Offset = 0,
			.Range = VK_WHOLE_SIZE,
			.DescriptorType = descriptorType,
			.Binding = binding,
			.ArrayIndex = arrayIndex
		});
	}

	void DescriptorWriter::WriteImage(const Texture& image, uint32_t binding, uint32_t arrayIndex /*= 0*/)
	{
		CO_PROFILE_FN();

		mSignature.Images.push_back(ImageBindingKey {
			.View = image.GetImageView(),
			.Sampler = image.GetSampler(),
			.Layout = image.GetImageLayout(),
			.Binding = binding,
			.ArrayIndex = arrayIndex
		});
	}

	void DescriptorWriter::Update(VkDescriptorSet descriptorSet)
	{
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		std::vector<VkDescriptorImageInfo>  imageInfos;
		std::vector<VkWriteDescriptorSet>   descriptorWrites;

		bufferInfos.reserve(mSignature.Buffers.size());
		imageInfos.reserve(mSignature.Images.size());
		descriptorWrites.reserve(mSignature.Buffers.size() + mSignature.Images.size());
		
		for (const auto& buffer : mSignature.Buffers)
		{
			bufferInfos.push_back(VkDescriptorBufferInfo {
				.buffer = buffer.Buffer,
				.offset = buffer.Offset,
				.range = buffer.Range
			});

			descriptorWrites.push_back(VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = descriptorSet,
				.dstBinding = buffer.Binding,
				.dstArrayElement = buffer.ArrayIndex,
				.descriptorCount = 1,
				.descriptorType = buffer.DescriptorType,
				.pBufferInfo = &bufferInfos.back(),
			});
		}

		for (const auto& image : mSignature.Images)
		{
			imageInfos.push_back(VkDescriptorImageInfo {
				.sampler = image.Sampler,
				.imageView = image.View,
				.imageLayout = image.Layout
			});

			descriptorWrites.push_back(VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = descriptorSet,
				.dstBinding = image.Binding,
				.dstArrayElement = image.ArrayIndex,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfos.back(),
			});
		}

		vkUpdateDescriptorSets(GraphicsContext::Get().GetDevice(), (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}

}