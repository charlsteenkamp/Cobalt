#include "copch.hpp"
#include "ShaderCursor.hpp"
#include "DescriptorCache.hpp"
#include "GraphicsContext.hpp"

namespace Cobalt
{

	ShaderCursor::ShaderCursor(const ShaderParameter& shaderParameter, DescriptorBindings& descriptorBindings, DescriptorHandle descriptorHandle)
		: mShaderParameter(shaderParameter), mDescriptorBindings(descriptorBindings), mDescriptorHandle(descriptorHandle)
	{
		CO_PROFILE_FN();
	}

	ShaderCursor::~ShaderCursor()
	{
		CO_PROFILE_FN();
	}

	void ShaderCursor::Write(const VulkanBuffer& buffer)
	{
		CO_PROFILE_FN();

		VkDescriptorType descriptorType;

		if (buffer.GetUsageFlags() & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
		{
			descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		}
		else if (buffer.GetUsageFlags() & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
		{
			descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		}

		mDescriptorBindings.Bindings.push_back(DescriptorBinding {
			.Binding = mShaderParameter.Binding,
			.Element = mShaderParameter.Index,
			.DescriptorType = descriptorType,
			.Address = buffer.GetDeviceAddress(),
			.Range = buffer.GetAllocationInfo().size
		});
	}

	void ShaderCursor::Write(const Texture& texture)
	{
		CO_PROFILE_FN();

		Write(Image {
			texture.GetSampler(), texture.GetImageView(), texture.GetImageLayout()
		});
	}

	void ShaderCursor::Write(const Image& image)
	{
		CO_PROFILE_FN();

		VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		mDescriptorBindings.Bindings.push_back(DescriptorBinding {
			.Binding = mShaderParameter.Binding,
			.Element = mShaderParameter.Index,
			.DescriptorType = descriptorType,
			.Sampler = image.Sampler,
			.ImageView = image.ImageView,
			.ImageLayout = image.ImageLayout 
		});
	}

	void ShaderCursor::Write(const std::vector<Image>& images)
	{
		CO_PROFILE_FN();

		for (uint32_t i = 0; i < images.size(); i++)
		{
			Element(i).Write(images[i]);
		}
	}

	void ShaderCursor::Finalize()
	{
		CO_PROFILE_FN();

		auto& descriptorCache = GraphicsContext::Get().GetDescriptorCache();
		descriptorCache.WriteDescriptorBindingsIfNeeded(mDescriptorHandle, mDescriptorBindings);
	}

}