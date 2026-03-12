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

		VkInstance instance = GraphicsContext::Get().GetInstance();
		VkDevice device = GraphicsContext::Get().GetDevice();

		vkGetDescriptorSetLayoutSizeEXT          = (PFN_vkGetDescriptorSetLayoutSizeEXT)vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutSizeEXT");
		vkGetDescriptorSetLayoutBindingOffsetEXT = (PFN_vkGetDescriptorSetLayoutBindingOffsetEXT)vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutBindingOffsetEXT");
		vkGetDescriptorEXT                       = (PFN_vkGetDescriptorEXT)vkGetDeviceProcAddr(device, "vkGetDescriptorEXT");
		vkCmdBindDescriptorBuffersEXT            = (PFN_vkCmdBindDescriptorBuffersEXT)vkGetDeviceProcAddr(device, "vkCmdBindDescriptorBuffersEXT");
		vkCmdSetDescriptorBufferOffsetsEXT       = (PFN_vkCmdSetDescriptorBufferOffsetsEXT)vkGetDeviceProcAddr(device, "vkCmdSetDescriptorBufferOffsetsEXT");

		mDescriptorBufferProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;

		VkPhysicalDeviceProperties2KHR physicalDeviceProperties = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR,
			.pNext = &mDescriptorBufferProperties
		};

		auto vkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR");
		vkGetPhysicalDeviceProperties2KHR(GraphicsContext::Get().GetPhysicalDevice(), &physicalDeviceProperties);

		uint32_t descriptorBufferSize = 1000000;

		mDescriptorBuffer.Buffer = VulkanBuffer::CreateMappedBuffer(descriptorBufferSize, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
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
		descriptorInfo.SetOffset = mDescriptorBuffer.Offset;

		mDescriptorBuffer.Offset += descriptorInfo.LayoutSize;
			
		/*if (resourceDescriptor)
		{
			descriptorInfo.ResourceSetOffset = mResourceDescriptorBuffer.Offset;
			mResourceDescriptorBuffer.Offset += descriptorInfo.LayoutSize;
		}

		if (samplerDescriptor)
		{
			descriptorInfo.SamplerSetOffset = mSamplerDescriptorBuffer.Offset;
			mSamplerDescriptorBuffer.Offset += descriptorInfo.LayoutSize;
		}*/

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

		uint8_t* bindingPtr = (uint8_t*)mDescriptorBuffer.Buffer->GetAllocationInfo().pMappedData + descriptorInfo.SetOffset + bindingOffset;

		switch (descriptorBinding.DescriptorType)
		{
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			{
				size_t descriptorSize = mDescriptorBufferProperties.uniformBufferDescriptorSize;
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
					.data = {
						.pUniformBuffer = &descriptorAddressInfo
					}
				};

				vkGetDescriptorEXT(GraphicsContext::Get().GetDevice(), &descriptorGetInfo, descriptorSize, bindingPtr);

				break;
			}
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			{
				size_t descriptorSize = mDescriptorBufferProperties.storageBufferDescriptorSize;
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
					.data = {
						.pStorageBuffer = &descriptorAddressInfo
					}
				};

				vkGetDescriptorEXT(GraphicsContext::Get().GetDevice(), &descriptorGetInfo, descriptorSize, bindingPtr);

				break;
			}
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			{
				size_t descriptorSize = mDescriptorBufferProperties.combinedImageSamplerDescriptorSize;
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

				break;
			}
		}
	}

	void DescriptorBufferManager::BindDescriptorBuffers(VkCommandBuffer commandBuffer)
	{
		CO_PROFILE_FN();

		VkDescriptorBufferBindingInfoEXT descriptorBufferBindingInfos[1];

		descriptorBufferBindingInfos[0] = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
			.address = mDescriptorBuffer.Buffer->GetDeviceAddress(),
			.usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
		};

		vkCmdBindDescriptorBuffersEXT(commandBuffer, 1, descriptorBufferBindingInfos);
	}

	void DescriptorBufferManager::SetDescriptorBufferOffsets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, DescriptorHandle descriptorHandle)
	{
		CO_PROFILE_FN();

		DescriptorInfo descriptorInfo = mDescriptorInfos[descriptorHandle];

		uint32_t bufferIndices[1] = { 0 };
		VkDeviceSize bufferOffsets[1] = { descriptorInfo.SetOffset };

		vkCmdSetDescriptorBufferOffsetsEXT(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, bufferIndices, bufferOffsets);
	}
	
}