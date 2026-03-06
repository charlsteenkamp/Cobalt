#include "copch.hpp"
#include "ShaderCursor.hpp"

namespace Cobalt
{

	ShaderCursor::ShaderCursor(const ShaderParameter& shaderParameter, DescriptorWriter& descriptorWriter)
		: mShaderParameter(shaderParameter), mDescriptorWriter(descriptorWriter)
	{
		CO_PROFILE_FN();
	}

	ShaderCursor::~ShaderCursor()
	{
		CO_PROFILE_FN();
	}

	void ShaderCursor::Write(const Texture& texture)
	{
		CO_PROFILE_FN();

	}

	void ShaderCursor::Write(const VulkanBuffer& buffer)
	{
		CO_PROFILE_FN();

	}

}