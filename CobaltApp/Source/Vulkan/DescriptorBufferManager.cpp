#include "copch.hpp"
#include "DescriptorBufferManager.hpp"
#include "GraphicsContext.hpp"

namespace Cobalt
{

	VkDeviceSize AlignedVkSize(VkDeviceSize value, VkDeviceSize alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	DescriptorBufferManager::DescriptorBufferManager(uint32_t frameCount)
	{
		CO_PROFILE_FN();

		VkPhysicalDeviceProperties2KHR physicalDeviceProperties = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR,
			.pNext = (void*)&mDescriptorBufferProperties
		};
			
		vkGetPhysicalDeviceProperties2KHR(GraphicsContext::Get().GetPhysicalDevice(), &physicalDeviceProperties);

		uint32_t resourceDescriptorBufferSize = 0;
		uint32_t samplerDescriptorBufferSize = 0;

		mResourceDescriptorBuffers.resize(frameCount);

		for (uint32_t i = 0; i < frameCount; i++)
		{
			mResourceDescriptorBuffers[i].Buffer = VulkanBuffer::CreateMappedBuffer(resourceDescriptorBufferSize, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		}

		mSamplerDescriptorBuffer.Buffer = VulkanBuffer::CreateMappedBuffer(samplerDescriptorBufferSize, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

	DescriptorBufferManager::~DescriptorBufferManager()
	{
		CO_PROFILE_FN();
	}

	DescriptorHandle DescriptorBufferManager::AllocateDescriptor(VkDescriptorSetLayout descriptorSetLayout, bool resourceDescriptor, bool samplerDescriptor)
	{
		CO_PROFILE_FN();

		DescriptorInfo descriptorInfo;
		descriptorInfo.Layout = descriptorSetLayout;

		vkGetDescriptorSetLayoutSizeEXT(GraphicsContext::Get().GetDevice(), descriptorSetLayout, &descriptorInfo.LayoutSize);
		descriptorInfo.LayoutSize = AlignedVkSize(descriptorInfo.LayoutSize, mDescriptorBufferProperties.descriptorBufferOffsetAlignment);
		
		if (resourceDescriptor)
		{
			for (auto& resourceDescriptorBuffer : mResourceDescriptorBuffers)
			{
				descriptorInfo.ResourceSetOffset = resourceDescriptorBuffer.Offset;
				resourceDescriptorBuffer.Offset += descriptorInfo.LayoutSize;
			}
		}

		if (samplerDescriptor)
		{
			descriptorInfo.SamplerSetOffset = mSamplerDescriptorBuffer.Offset;
			mSamplerDescriptorBuffer.Offset += descriptorInfo.LayoutSize;
		}

		mDescriptorInfos.push_back(descriptorInfo);
		return mDescriptorInfos.size() - 1;
	}

	void DescriptorBufferManager::WriteDescriptor(const VulkanBuffer& buffer, DescriptorHandle descriptorHandle, uint32_t binding, uint32_t element = 0)
	{
		CO_PROFILE_FN();

		DescriptorInfo descriptorInfo = mDescriptorInfos[descriptorHandle];

		size_t descriptorSize = 0;
		VkDescriptorType descriptorType{};

		if (buffer.GetUsageFlags() & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
		{
			descriptorSize = mDescriptorBufferProperties.uniformBufferDescriptorSize;
			descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		}
		else if (buffer.GetUsageFlags() & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
		{
			descriptorSize = mDescriptorBufferProperties.storageBufferDescriptorSize;
			descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		}

		for (auto& resourceDescriptorBuffer : mResourceDescriptorBuffers)
		{
			VkDeviceSize bindingOffset;
			vkGetDescriptorSetLayoutBindingOffsetEXT(GraphicsContext::Get().GetDevice(), descriptorInfo.Layout, binding, &bindingOffset);

			uint8_t* bindingPtr = (uint8_t*)resourceDescriptorBuffer.Buffer->GetAllocationInfo().pMappedData + descriptorInfo.ResourceSetOffset + bindingOffset;
			bindingPtr = bindingPtr + element * descriptorSize;

			VkDescriptorAddressInfoEXT descriptorAddressInfo = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
				.address = buffer.GetDeviceAddress(),
				.range = buffer.GetAllocationInfo().size,
				.format = VK_FORMAT_UNDEFINED,
			};

			VkDescriptorGetInfoEXT descriptorGetInfo = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
				.type = descriptorType,
			};

			if (descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				descriptorGetInfo.data.pUniformBuffer = &descriptorAddressInfo;
			else if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
				descriptorGetInfo.data.pStorageBuffer = &descriptorAddressInfo;

			vkGetDescriptorEXT(GraphicsContext::Get().GetDevice(), &descriptorGetInfo, descriptorSize, bindingPtr);
		}
	}

	void DescriptorBufferManager::WriteDescriptor(const Texture& image, DescriptorHandle descriptorHandle, uint32_t binding, uint32_t element = 0)
	{
		CO_PROFILE_FN();

		DescriptorInfo descriptorInfo = mDescriptorInfos[descriptorHandle];

		size_t descriptorSize = mDescriptorBufferProperties.combinedImageSamplerDescriptorSize;
		VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		VkDeviceSize bindingOffset;
		vkGetDescriptorSetLayoutBindingOffsetEXT(GraphicsContext::Get().GetDevice(), descriptorInfo.Layout, binding, &bindingOffset);

		uint8_t* bindingPtr = (uint8_t*)mSamplerDescriptorBuffer.Buffer->GetAllocationInfo().pMappedData + descriptorInfo.ResourceSetOffset + bindingOffset;
		bindingPtr = bindingPtr + element * descriptorSize;

		VkDescriptorImageInfo descriptorImageInfo = {
			.sampler = image.GetSampler(),
			.imageView = image.GetImageView(),
			.imageLayout = image.GetImageLayout()
		};

		VkDescriptorGetInfoEXT descriptorGetInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type = descriptorType,
			.data = {
				.pCombinedImageSampler = &descriptorImageInfo
			}
		};

		vkGetDescriptorEXT(GraphicsContext::Get().GetDevice(), &descriptorGetInfo, descriptorSize, bindingPtr);
	}

}