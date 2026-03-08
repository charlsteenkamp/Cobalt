#pragma once
#include "VulkanUtils.hpp"

#include <functional>
#include <vector>

namespace Cobalt
{

	inline void HashCombine(size_t& seed, size_t value)
	{
		seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	using DescriptorBindingHash = size_t;
	using DescriptorSignature = size_t;

	struct DescriptorBinding
	{
		uint32_t         Binding        = 0;
		uint32_t         Element        = 0;
		VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;

		VkDeviceAddress  Address        = 0;
		VkDeviceSize     Range          = 0;

		VkSampler        Sampler        = VK_NULL_HANDLE;
		VkImageView      ImageView      = VK_NULL_HANDLE;
		VkImageLayout    ImageLayout    = VK_IMAGE_LAYOUT_UNDEFINED;

		DescriptorBindingHash Hash() const
		{
			size_t hash = 0;

			HashCombine(hash, std::hash<uint32_t>{}(Binding));
			HashCombine(hash, std::hash<uint32_t>{}(Element));
			HashCombine(hash, std::hash<VkDescriptorType>{}(DescriptorType));
			HashCombine(hash, std::hash<VkDeviceAddress>{}(Address));
			HashCombine(hash, std::hash<VkDeviceSize>{}(Range));
			HashCombine(hash, std::hash<VkSampler>{}(Sampler));
			HashCombine(hash, std::hash<VkImageView>{}(ImageView));
			HashCombine(hash, std::hash<VkImageLayout>{}(ImageLayout));
			
			return hash;
		}
	};

	struct DescriptorBindings
	{
		std::vector<DescriptorBinding> Bindings;

		DescriptorSignature Hash() const
		{
			size_t hash = 0;

			for (const auto& binding : Bindings)
				hash += binding.Hash();

			return hash;
		}
	};

}
