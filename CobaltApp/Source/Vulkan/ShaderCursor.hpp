#pragma once
#include "Texture.hpp"
#include "VulkanBuffer.hpp"
#include "DescriptorWriter.hpp"
#include "ShaderParameters.hpp"

#include <string>
#include <unordered_map>

namespace Cobalt
{

	class ShaderCursor
	{
	public:
		ShaderCursor(const ShaderParameter& shaderParameter, DescriptorWriter& descriptorWriter);
		~ShaderCursor();

	public:
		ShaderCursor Field(const std::string& name) const { return { mShaderParameter.Fields.at(name), mDescriptorWriter }; }
		ShaderCursor Element(uint32_t index)        const { return { mShaderParameter.Elements[index], mDescriptorWriter }; }

	public:
		void Write(const Texture& texture);
		void Write(const VulkanBuffer& buffer);

	private:
		const ShaderParameter& mShaderParameter;
		DescriptorWriter& mDescriptorWriter;
	};

}
