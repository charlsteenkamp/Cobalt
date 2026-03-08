#pragma once
#include <string>
#include <unordered_map>

namespace Cobalt
{

	enum class ShaderParameterKind 
	{
		None = -1,
		UniformBuffer,
		StorageBuffer,
		Sampler,
		SampledImage,
		CombinedImageSampler,
		StorageImage,
		InputAttachment
	};

	struct ShaderParameter;
	using ShaderParameterMap = std::unordered_map<std::string, ShaderParameter>;

	struct ShaderParameter
	{
		ShaderParameterKind Kind = ShaderParameterKind::None;

		uint32_t Binding = 0;

		size_t UniformByteOffset = 0;          // If it's a uniform
		size_t UniformSize       = 0;          // If it's a uniform
		size_t ElementStride     = 0;          // If it's an array
		
		uint32_t Index = 0; // If it's an element in an array

		ShaderParameterMap           Fields;   // If it's a struct
		std::vector<ShaderParameter> Elements; // If it's an array
	};

}
