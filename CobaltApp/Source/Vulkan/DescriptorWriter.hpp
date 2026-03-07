#pragma once
#include "VulkanUtils.hpp"
#include "VulkanBuffer.hpp"
#include "Texture.hpp"

namespace Cobalt
{

	class DescriptorWriter
	{
	public:
		DescriptorWriter() = default;
		DescriptorWriter(VkDescriptorSetLayout descriptorSetLayout);
		~DescriptorWriter();

		void SetLayout(VkDescriptorSetLayout layout) { mSignature.Layout = layout; }

	public:
		void WriteBuffer(const VulkanBuffer& buffer, uint32_t binding, uint32_t arrayIndex = 0);
		void WriteImage(const Texture& image, uint32_t binding, uint32_t arrayIndex = 0);
		void Update(VkDescriptorSet descriptorSet);

	public:
		const DescriptorSetSignature& GetSignature() const { return mSignature; }

	private:
		DescriptorSetSignature mSignature;
	};

}

