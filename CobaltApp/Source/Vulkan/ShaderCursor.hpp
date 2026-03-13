#pragma once
#include "Texture.hpp"
#include "VulkanBuffer.hpp"
#include "ShaderParameters.hpp"
#include "DescriptorBindings.hpp"
#include "DescriptorBufferManager.hpp"

#include <string>
#include <cassert>

namespace Cobalt
{

	class ShaderCursor
	{
	public:
		ShaderCursor(ShaderParameter& shaderParameter, DescriptorHandle descriptorHandle);
		~ShaderCursor();

	private:
		ShaderCursor(ShaderParameter& shaderParameter, DescriptorBindings& descriptorBindings, DescriptorHandle descriptorHandle);

	public:
		void Write(const VulkanBuffer& buffer);
		void Write(const Texture& texture);
		void Write(const Image& image);
		void Write(const std::vector<Image>& images);

		ShaderCursor Field(const std::string& name) const
		{
			assert(mShaderParameter.Fields.contains(name));
			return { mShaderParameter.Fields.at(name), mDescriptorBindingsRef, mDescriptorHandle };
		}

		ShaderCursor Element(uint32_t index) const
		{
			assert(mShaderParameter.Kind == ShaderParameterKind::Array);

			if (index >= mShaderParameter.Elements.size())
			{
				mShaderParameter.Elements.push_back(ShaderParameter{
					.Kind = mShaderParameter.ElementKind,
					.Binding = mShaderParameter.Binding,
					.Index = index,
				});
			}

			return { mShaderParameter.Elements[index], mDescriptorBindingsRef, mDescriptorHandle};
		}

		ShaderCursor WriteField(const std::string& name, const VulkanBuffer& buffer) const
		{
			Field(name).Write(buffer);
			return *this;
		}

		ShaderCursor WriteField(const std::string& name, const Texture& texture) const
		{
			Field(name).Write(texture);
			return *this;
		}

		ShaderCursor WriteField(const std::string& name, const std::vector<Image>& images) const
		{
			Field(name).Write(images);
			return *this;
		}

		// Updates descriptor bindings if needed
		void Finalize();

	private:
		ShaderParameter& mShaderParameter;
		DescriptorBindings mDescriptorBindings;
		DescriptorBindings& mDescriptorBindingsRef;
		DescriptorHandle mDescriptorHandle;
	};

}
