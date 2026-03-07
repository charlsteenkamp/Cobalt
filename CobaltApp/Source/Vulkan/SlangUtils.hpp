#pragma once
#include "ShaderParameters.hpp"

#include <slang/slang.h>
#include <iostream>
#include <vulkan/vulkan.h>

namespace Cobalt::SlangUtils
{

	inline void CheckError(SlangResult result)
	{
		if (SLANG_FAILED(result))
		{
			std::cerr << "Slang error (" << result << ")\n";
		}
	}

	inline void CheckBlob(slang::IBlob* diagnosticsBlob)
	{
		if (diagnosticsBlob)
		{
			std::cerr << (const char*)diagnosticsBlob->getBufferPointer() << std::endl;
		}
	}

	inline VkShaderStageFlagBits SlangStageToVkShaderStageFlagBits(SlangStage stage)
	{
		switch (stage)
		{
			case SLANG_STAGE_VERTEX:   return VK_SHADER_STAGE_VERTEX_BIT;
			case SLANG_STAGE_FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
			case SLANG_STAGE_COMPUTE:  return VK_SHADER_STAGE_COMPUTE_BIT;
		}
	}

	inline VkDescriptorType SlangBindingTypeToVkDescriptorType(slang::BindingType bindingType)
	{
		switch (bindingType)
		{
			case slang::BindingType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
			case slang::BindingType::Texture: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			case slang::BindingType::ConstantBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			case slang::BindingType::ParameterBlock: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			case slang::BindingType::TypedBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			case slang::BindingType::RawBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			case slang::BindingType::CombinedTextureSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			case slang::BindingType::InputRenderTarget: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			case slang::BindingType::MutableTexture: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			case slang::BindingType::MutableTypedBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			case slang::BindingType::MutableRawBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		}
	}

	inline ShaderParameterKind SlangBindingTypeToShaderParameterKind(slang::BindingType bindingType)
	{
		switch (bindingType)
		{
			case slang::BindingType::Sampler: return ShaderParameterKind::Sampler;
			case slang::BindingType::Texture: return ShaderParameterKind::SampledImage;
			case slang::BindingType::ConstantBuffer: return ShaderParameterKind::UniformBuffer;
			case slang::BindingType::ParameterBlock: return ShaderParameterKind::UniformBuffer;
			case slang::BindingType::TypedBuffer: return ShaderParameterKind::StorageBuffer;
			case slang::BindingType::RawBuffer: return ShaderParameterKind::StorageBuffer;
			case slang::BindingType::CombinedTextureSampler: return ShaderParameterKind::CombinedImageSampler;
			case slang::BindingType::InputRenderTarget: return ShaderParameterKind::InputAttachment;
			case slang::BindingType::MutableTexture: return ShaderParameterKind::StorageImage;
			case slang::BindingType::MutableTypedBuffer: return ShaderParameterKind::StorageBuffer;
			case slang::BindingType::MutableRawBuffer: return ShaderParameterKind::StorageBuffer;
		}
	}

}
