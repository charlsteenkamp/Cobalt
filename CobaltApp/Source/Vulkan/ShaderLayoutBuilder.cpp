#include "copch.hpp"
#include "ShaderLayoutBuilder.hpp"
#include "SlangUtils.hpp"
#include "GraphicsContext.hpp"

namespace Cobalt
{

	ShaderLayoutBuilder::ShaderLayoutBuilder(slang::ProgramLayout* programLayout)
	{
		AddGlobalBindings(programLayout->getGlobalParamsVarLayout());

		for (int32_t i = 0; i < programLayout->getEntryPointCount(); i++)
			AddEntryPoint(programLayout->getEntryPointByIndex(i));

		BuildDescriptorSetLayouts();
	}

	ShaderLayoutBuilder::~ShaderLayoutBuilder()
	{
	}

	void ShaderLayoutBuilder::AddGlobalBindings(slang::VariableLayoutReflection* globalVarLayout)
	{
		slang::TypeLayoutReflection* globalTypeLayout = globalVarLayout->getTypeLayout();

		bool foundFinalType = false;

		while (!foundFinalType)
		{
			if (!globalTypeLayout->getType())
			{
				if (slang::TypeLayoutReflection* elementTypeLayout = globalTypeLayout->getElementTypeLayout())
					globalTypeLayout = elementTypeLayout;
			}

			switch (globalTypeLayout->getKind())
			{
				case slang::TypeReflection::Kind::Array:
				case slang::TypeReflection::Kind::Resource:
				{
					globalTypeLayout = globalTypeLayout->getElementTypeLayout();
					foundFinalType = true;
					break;
				}
				case slang::TypeReflection::Kind::ConstantBuffer:
				case slang::TypeReflection::Kind::ParameterBlock:
				{
					globalTypeLayout = globalTypeLayout->getElementTypeLayout();
					break;
				}
				default:
				{
					foundFinalType = true;
					break;
				}
			}
		}

		BindingOffset bindingOffset(globalVarLayout);

		AddDescriptorBindings(globalTypeLayout, bindingOffset);
	}

	void ShaderLayoutBuilder::AddEntryPoint(slang::EntryPointLayout* entryPointLayout)
	{
		BindingOffset entryPointOffset(entryPointLayout->getVarLayout());

		AddDescriptorBindings(entryPointLayout->getTypeLayout(), entryPointOffset);
	}

	void ShaderLayoutBuilder::AddDescriptorBindings(slang::TypeLayoutReflection* typeLayout, BindingOffset bindingOffset)
	{
		// First find descriptor sets

		int32_t descriptorSetCount = typeLayout->getDescriptorSetCount();

		for (int32_t i = 0; i < descriptorSetCount; i++)
		{
			int32_t descriptorRangeCount = typeLayout->getDescriptorSetDescriptorRangeCount(i);

			if (descriptorRangeCount > 0)
			{
				FindOrAddDescriptorSet(bindingOffset.Set + typeLayout->getDescriptorSetSpaceOffset(i));
			}
		}

		int32_t bindingRangeCount = typeLayout->getBindingRangeCount();

		for (int32_t i = 0; i < bindingRangeCount; i++)
		{
			slang::BindingType bindingRangeType = typeLayout->getBindingRangeType(i);

			// Skip over sub-objects; they're handled later

			if (bindingRangeType == slang::BindingType::ParameterBlock   ||
				bindingRangeType == slang::BindingType::ConstantBuffer   ||
				bindingRangeType == slang::BindingType::ExistentialValue ||
				bindingRangeType == slang::BindingType::PushConstant)
			{
				continue;
			}

			int32_t descriptorRangeCount = typeLayout->getBindingRangeDescriptorRangeCount(i);

			if (descriptorRangeCount == 0)
				continue;

			int32_t slangDescriptorSetIndex = typeLayout->getBindingRangeDescriptorSetIndex(i);
			int32_t descriptorSetIndex = FindOrAddDescriptorSet(bindingOffset.Set + typeLayout->getDescriptorSetSpaceOffset(slangDescriptorSetIndex));

			DescriptorSetInfo& descriptorSetInfo = mDescriptorSetInfos.at(descriptorSetIndex);

			int32_t firstDescriptorRangeIndex = typeLayout->getBindingRangeFirstDescriptorRangeIndex(i);

			for (int32_t j = 0; j < descriptorRangeCount; j++)
			{
				int32_t descriptorRangeIndex = firstDescriptorRangeIndex + j;

				slang::BindingType slangDescriptorType = typeLayout->getDescriptorSetDescriptorRangeType(slangDescriptorSetIndex, descriptorRangeIndex);

				if (slangDescriptorType == slang::BindingType::ExistentialValue ||
					slangDescriptorType == slang::BindingType::InlineUniformData ||
					slangDescriptorType == slang::BindingType::PushConstant)
				{
					continue;
				}

				uint32_t descriptorCount = typeLayout->getDescriptorSetDescriptorRangeDescriptorCount(slangDescriptorSetIndex, descriptorRangeIndex);
				descriptorCount = descriptorCount != UINT32_MAX ? descriptorCount : CO_BINDLESS_DESCRIPTOR_COUNT;

				VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {
					.binding            = bindingOffset.Binding + (uint32_t)typeLayout->getDescriptorSetDescriptorRangeIndexOffset(slangDescriptorSetIndex, descriptorRangeIndex),
					.descriptorType     = SlangUtils::SlangBindingTypeToVkDescriptorType(slangDescriptorType),
					.descriptorCount    = descriptorCount,
					.stageFlags         = VK_SHADER_STAGE_ALL,
					.pImmutableSamplers = nullptr,
				};

				descriptorSetInfo.AddBinding(descriptorSetLayoutBinding);

				slang::VariableLayoutReflection* variableLayout = typeLayout->getContainerVarLayout();

				ShaderParameter shaderParameter;
				shaderParameter.Kind = SlangUtils::SlangBindingTypeToShaderParameterKind(slangDescriptorType);
				shaderParameter.Binding = descriptorSetLayoutBinding.binding;
				shaderParameter.UniformByteOffset = variableLayout->getOffset();
				shaderParameter.UniformSize = typeLayout->getSize();
				shaderParameter.ElementStride = typeLayout->getStride();
			}
		}

		// Iterate over sub-object ranges

		int32_t subObjectRangeCount = typeLayout->getSubObjectRangeCount();

		for (int32_t i = 0; i < subObjectRangeCount; i++)
		{
			int32_t bindingRangeIndex = typeLayout->getSubObjectRangeBindingRangeIndex(i);

			slang::BindingType bindingType = typeLayout->getBindingRangeType(bindingRangeIndex);
			slang::TypeLayoutReflection* subObjectTypeLayout = typeLayout->getBindingRangeLeafTypeLayout(bindingRangeIndex);

			BindingOffset subObjectRangeOffset = bindingOffset;
			subObjectRangeOffset += BindingOffset(typeLayout->getSubObjectRangeOffset(i));

			switch (bindingType)
			{
				default:
					break;
				case slang::BindingType::ConstantBuffer:
				{
					slang::VariableLayoutReflection* containerVarLayout = subObjectTypeLayout->getContainerVarLayout();
					slang::VariableLayoutReflection* elementVarLayout = subObjectTypeLayout->getElementVarLayout();

					BindingOffset containerOffset = subObjectRangeOffset + BindingOffset(containerVarLayout);
					BindingOffset elementOffset   = subObjectRangeOffset + BindingOffset(elementVarLayout);
					
					AddConstantBufferDescriptorBindings(elementVarLayout->getTypeLayout(), containerOffset, elementOffset);

					break;
				}
				case slang::BindingType::PushConstant:
				{
					slang::VariableLayoutReflection* containerVarLayout = subObjectTypeLayout->getContainerVarLayout();
					slang::VariableLayoutReflection* elementVarLayout = subObjectTypeLayout->getElementVarLayout();

					BindingOffset containerOffset = subObjectRangeOffset + BindingOffset(containerVarLayout);
					BindingOffset elementOffset   = subObjectRangeOffset + BindingOffset(elementVarLayout);

					AddPushConstantRange(elementVarLayout->getTypeLayout(), containerOffset, elementOffset);

					break;
				}
			}
		}
	}
	
	void ShaderLayoutBuilder::BuildDescriptorSetLayouts()
	{
		mDescriptorSetLayouts.resize(mDescriptorSetInfos.size());

		for (int32_t set = 0; set < mDescriptorSetInfos.size(); set++)
		{
			const DescriptorSetInfo& descriptorSetInfo = mDescriptorSetInfos[mDescriptorSpaceIndexMap.at(set)];
			std::vector<VkDescriptorBindingFlags> descriptorBindingFlags((uint32_t)descriptorSetInfo.Bindings.size());

#if 0
			for (uint32_t i = 0; i < descriptorBindingFlags.size(); i++)
			{
				const VkDescriptorSetLayoutBinding& binding = descriptorSetInfo.Bindings[i];
				if (binding.descriptorCount == CO_BINDLESS_DESCRIPTOR_COUNT)
				{
					descriptorBindingFlags[i] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
				}
				else
				{
					descriptorBindingFlags[i] = 0;
				}
			}
#endif

			VkDescriptorSetLayoutBindingFlagsCreateInfo descriptorSetLayoutBindingFlagsCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
				.bindingCount = (uint32_t)descriptorBindingFlags.size(),
				.pBindingFlags = descriptorBindingFlags.data()
			};

			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext = &descriptorSetLayoutBindingFlagsCreateInfo,
				.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
				.bindingCount = (uint32_t)descriptorSetInfo.Bindings.size(),
				.pBindings = descriptorSetInfo.Bindings.data(),
			};

			VK_CALL(vkCreateDescriptorSetLayout(GraphicsContext::Get().GetDevice(), &descriptorSetLayoutCreateInfo, nullptr, &mDescriptorSetLayouts[set]));
		}
	}

	void ShaderLayoutBuilder::AddConstantBufferDescriptorBindings(slang::TypeLayoutReflection* elementTypeLayout, BindingOffset containerOffset, BindingOffset elementOffset)
	{
		if (elementTypeLayout->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM) != 0)
		{
			int32_t descriptorSetIndex = FindOrAddDescriptorSet(containerOffset.Set);
			DescriptorSetInfo& descriptorSetInfo = mDescriptorSetInfos[descriptorSetIndex];

			descriptorSetInfo.AddBinding(VkDescriptorSetLayoutBinding {
				.binding = containerOffset.Binding,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_ALL,
				.pImmutableSamplers = nullptr,
			});
		}

		AddDescriptorBindings(elementTypeLayout, elementOffset);
	}

	void ShaderLayoutBuilder::AddPushConstantRange(slang::TypeLayoutReflection* elementTypeLayout, BindingOffset containerOffset, BindingOffset elementOffset)
	{
		// TODO
	}

	int32_t ShaderLayoutBuilder::FindOrAddDescriptorSet(int32_t space)
	{
		if (mDescriptorSpaceIndexMap.contains(space))
			return mDescriptorSpaceIndexMap.at(space);

		DescriptorSetInfo descriptorSetInfo;
		descriptorSetInfo.Space = space;

		mDescriptorSetInfos.push_back(descriptorSetInfo);

		int32_t index = mDescriptorSetInfos.size() - 1;

		mDescriptorSpaceIndexMap[space] = index;

		return index;
	}

}