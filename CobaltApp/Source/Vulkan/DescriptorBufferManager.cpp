#include "copch.hpp"
#include "DescriptorBufferManager.hpp"
#include "GraphicsContext.hpp"

namespace Cobalt
{

	VkDeviceSize AlignedVkSize(VkDeviceSize value, VkDeviceSize alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	DescriptorBufferManager::DescriptorBufferManager()
	{
		CO_PROFILE_FN();

		VkDevice device = GraphicsContext::Get().GetDevice();

		vkGetDescriptorSetLayoutSizeEXT          = (PFN_vkGetDescriptorSetLayoutSizeEXT)vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutSizeEXT");
		vkGetDescriptorSetLayoutBindingOffsetEXT = (PFN_vkGetDescriptorSetLayoutBindingOffsetEXT)vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutBindingOffsetEXT");
		vkGetDescriptorEXT                       = (PFN_vkGetDescriptorEXT)vkGetDeviceProcAddr(device, "vkGetDescriptorEXT");
		vkCmdBindDescriptorBuffersEXT            = (PFN_vkCmdBindDescriptorBuffersEXT)vkGetDeviceProcAddr(device, "vkCmdBindDescriptorBuffersEXT");
		vkCmdSetDescriptorBufferOffsetsEXT       = (PFN_vkCmdSetDescriptorBufferOffsetsEXT)vkGetDeviceProcAddr(device, "vkCmdSetDescriptorBufferOffsetsEXT");


		VkPhysicalDeviceProperties2KHR physicalDeviceProperties = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR,
			.pNext = (void*)&mDescriptorBufferProperties
		};
			
		auto vkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR)vkGetDeviceProcAddr(device, "vkGetPhysicalDeviceProperties2KHR");
		vkGetPhysicalDeviceProperties2KHR(GraphicsContext::Get().GetPhysicalDevice(), &physicalDeviceProperties);

		uint32_t resourceDescriptorBufferSize = 0;
		uint32_t samplerDescriptorBufferSize = 0;

		mResourceDescriptorBuffer.Buffer = VulkanBuffer::CreateMappedBuffer(resourceDescriptorBufferSize, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
		mSamplerDescriptorBuffer.Buffer  = VulkanBuffer::CreateMappedBuffer(samplerDescriptorBufferSize, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
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
			descriptorInfo.ResourceSetOffset = mResourceDescriptorBuffer.Offset;
			mResourceDescriptorBuffer.Offset += descriptorInfo.LayoutSize;
		}

		if (samplerDescriptor)
		{
			descriptorInfo.SamplerSetOffset = mSamplerDescriptorBuffer.Offset;
			mSamplerDescriptorBuffer.Offset += descriptorInfo.LayoutSize;
		}

		mDescriptorInfos.push_back(descriptorInfo);
		return mDescriptorInfos.size() - 1;
	}

	void DescriptorBufferManager::WriteDescriptor(const DescriptorBinding& descriptorBinding, DescriptorHandle descriptorHandle)
	{
		CO_PROFILE_FN();

		DescriptorInfo descriptorInfo = mDescriptorInfos[descriptorHandle];
		VkDeviceSize bindingOffset;

		if (descriptorInfo.BindingOffsets.contains(descriptorBinding.Binding))
		{
			bindingOffset = descriptorInfo.BindingOffsets.at(descriptorBinding.Binding);
		}
		else
		{
			vkGetDescriptorSetLayoutBindingOffsetEXT(GraphicsContext::Get().GetDevice(), descriptorInfo.Layout, descriptorBinding.Binding, &bindingOffset);
			descriptorInfo.BindingOffsets[descriptorBinding.Binding] = bindingOffset;
		}

		switch (descriptorBinding.DescriptorType)
		{
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:         WriteBufferDescriptor(descriptorBinding, descriptorInfo, bindingOffset, mDescriptorBufferProperties.uniformBufferDescriptorSize); break;
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:         WriteBufferDescriptor(descriptorBinding, descriptorInfo, bindingOffset, mDescriptorBufferProperties.storageBufferDescriptorSize); break;
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: WriteImageDescriptor(descriptorBinding, descriptorInfo, bindingOffset, mDescriptorBufferProperties.combinedImageSamplerDescriptorSize); break;
		}
	}

	void DescriptorBufferManager::WriteBufferDescriptor(const DescriptorBinding& descriptorBinding, const DescriptorInfo& descriptorInfo, VkDeviceSize bindingOffset, size_t descriptorSize)
	{
		CO_PROFILE_FN();

		uint8_t* bindingPtr = (uint8_t*)mResourceDescriptorBuffer.Buffer->GetAllocationInfo().pMappedData + descriptorInfo.ResourceSetOffset + bindingOffset;
		bindingPtr = bindingPtr + descriptorBinding.Element * descriptorSize;

		VkDescriptorAddressInfoEXT descriptorAddressInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
			.address = descriptorBinding.Address,
			.range = descriptorBinding.Range,
			.format = VK_FORMAT_UNDEFINED,
		};

		VkDescriptorGetInfoEXT descriptorGetInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type = descriptorBinding.DescriptorType,
		};

		switch (descriptorBinding.DescriptorType)
		{
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: descriptorGetInfo.data.pUniformBuffer = &descriptorAddressInfo; break;
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: descriptorGetInfo.data.pStorageBuffer = &descriptorAddressInfo; break;
		}

		vkGetDescriptorEXT(GraphicsContext::Get().GetDevice(), &descriptorGetInfo, descriptorSize, bindingPtr);
	}

	void DescriptorBufferManager::WriteImageDescriptor(const DescriptorBinding& descriptorBinding, const DescriptorInfo& descriptorInfo, VkDeviceSize bindingOffset, size_t descriptorSize)
	{
		CO_PROFILE_FN();

		uint8_t* bindingPtr = (uint8_t*)mSamplerDescriptorBuffer.Buffer->GetAllocationInfo().pMappedData + descriptorInfo.SamplerSetOffset + bindingOffset;
		bindingPtr = bindingPtr + descriptorBinding.Element * descriptorSize;

		VkDescriptorImageInfo descriptorImageInfo = {
			.sampler = descriptorBinding.Sampler,
			.imageView = descriptorBinding.ImageView,
			.imageLayout = descriptorBinding.ImageLayout
		};

		VkDescriptorGetInfoEXT descriptorGetInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type = descriptorBinding.DescriptorType,
			.data = {
				.pCombinedImageSampler = &descriptorImageInfo
			}
		};

		vkGetDescriptorEXT(GraphicsContext::Get().GetDevice(), &descriptorGetInfo, descriptorSize, bindingPtr);
	}

	void DescriptorBufferManager::BindDescriptorBuffers(VkCommandBuffer commandBuffer)
	{
		CO_PROFILE_FN();

		VkDescriptorBufferBindingInfoEXT descriptorBufferBindingInfos[2];

		descriptorBufferBindingInfos[0] = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
			.address = mResourceDescriptorBuffer.Buffer->GetDeviceAddress(),
			.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
		};
		descriptorBufferBindingInfos[1] = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
			.address = mSamplerDescriptorBuffer.Buffer->GetDeviceAddress(),
			.usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
		};

		vkCmdBindDescriptorBuffersEXT(commandBuffer, 2, descriptorBufferBindingInfos);
	}

	void DescriptorBufferManager::SetDescriptorBufferOffsets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, DescriptorHandle descriptorHandle)
	{
		CO_PROFILE_FN();

		DescriptorInfo descriptorInfo = mDescriptorInfos[descriptorHandle];

		uint32_t     bufferIndices[2];
		VkDeviceSize bufferOffsets[2];

		uint32_t count = 0;

		if (descriptorInfo.IsResourceDescriptor())
		{
			bufferIndices[count] = 0;
			bufferOffsets[count] = descriptorInfo.ResourceSetOffset;
			count++;
		}

		if (descriptorInfo.IsSamplerDescriptor())
		{
			bufferIndices[count] = 1;
			bufferIndices[count] = descriptorInfo.SamplerSetOffset;
			count++;
		}

		vkCmdSetDescriptorBufferOffsetsEXT(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, count, bufferIndices, bufferOffsets);
	}
	
}