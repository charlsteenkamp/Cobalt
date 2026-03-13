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
					mRootShaderParam.Kind = ShaderParameterKind::UniformBuffer;
					break;
				}
				default:
				{
					foundFinalType = true;
					mRootShaderParam.Kind = ShaderParameterKind::None;
					break;
				}
			}
		}

		BindingOffset bindingOffset(globalVarLayout);

		AddDescriptorBindings(globalTypeLayout, bindingOffset);
		AddShaderParameters(globalVarLayout, mRootShaderParam, bindingOffset.Binding, 0);
	}

	void ShaderLayoutBuilder::AddEntryPoint(slang::EntryPointLayout* entryPointLayout)
	{
		slang::VariableLayoutReflection* varLayout = entryPointLayout->getVarLayout();
		BindingOffset entryPointOffset(varLayout);

		AddDescriptorBindings(entryPointLayout->getTypeLayout(), entryPointOffset);
		AddShaderParameters(varLayout, mRootShaderParam, varLayout->getOffset(SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT), varLayout->getOffset());
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

	void ShaderLayoutBuilder::AddShaderParameters(slang::VariableLayoutReflection* varLayout, ShaderParameter& shaderParameter, uint32_t bindingOffset, uint32_t uniformByteOffset)
	{
		CO_PROFILE_FN();

		slang::TypeLayoutReflection* typeLayout = varLayout->getTypeLayout();
		slang::VariableLayoutReflection* containerVarLayout = typeLayout->getContainerVarLayout();
		slang::VariableLayoutReflection* elementVarLayout = typeLayout->getElementVarLayout();
		slang::TypeLayoutReflection* elementTypeLayout = elementVarLayout->getTypeLayout();

		if (typeLayout->getKind() != slang::TypeReflection::Kind::Struct)
		{
			shaderParameter.Kind = SlangUtils::SlangTypeLayoutToShaderParameterKind(typeLayout);
			shaderParameter.Binding = bindingOffset + varLayout->getOffset(SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT);
			shaderParameter.UniformByteOffset = uniformByteOffset + varLayout->getOffset();
		}

		switch (typeLayout->getKind())
		{
			case slang::TypeReflection::Kind::Matrix:
			case slang::TypeReflection::Kind::Vector:
			case slang::TypeReflection::Kind::Scalar:
			{
				shaderParameter.UniformSize = typeLayout->getSize();
				break;
			}
			case slang::TypeReflection::Kind::Array:
			{
				shaderParameter.ElementKind = SlangUtils::SlangResourceShapeToShaderParameterKind(typeLayout->getType()->getElementType()->getResourceShape());
				shaderParameter.ElementStride = typeLayout->getElementStride(SLANG_PARAMETER_CATEGORY_UNIFORM);

				uint32_t elementCount = typeLayout->getElementCount();

				if (elementCount == 0 || elementCount == SLANG_UNBOUNDED_SIZE)
					break;

				shaderParameter.Elements.resize(elementCount);

				for (uint32_t i = 0; i < elementCount; i++)
				{
					shaderParameter.Elements[i] = ShaderParameter {
						.Kind = SlangUtils::SlangTypeLayoutToShaderParameterKind(elementTypeLayout),
						.Binding = shaderParameter.Binding,
						.UniformByteOffset = uniformByteOffset + i * shaderParameter.ElementStride,
						.Index = i,
					};
				}

				break;
			}
			case slang::TypeReflection::Kind::ConstantBuffer:
			case slang::TypeReflection::Kind::ParameterBlock:
			{
				shaderParameter.UniformSize = elementTypeLayout->getSize();

				AddShaderParameters(elementVarLayout, shaderParameter, shaderParameter.Binding, shaderParameter.UniformByteOffset);
				break;
			}
			case slang::TypeReflection::Kind::Struct:
			{
				uint32_t fieldCount = typeLayout->getFieldCount();
				uint32_t localBindingOffset = bindingOffset + shaderParameter.Binding;
				uint32_t localUniformByteOffset = uniformByteOffset + shaderParameter.UniformByteOffset;

				for (uint32_t i = 0; i < fieldCount; i++)
				{
					slang::VariableLayoutReflection* fieldVarLayout  = typeLayout->getFieldByIndex(i);
					slang::TypeLayoutReflection* fieldTypeLayout  = fieldVarLayout->getTypeLayout();

					slang::ParameterCategory parameterCategory = fieldTypeLayout->getParameterCategory();

					if (parameterCategory == SLANG_PARAMETER_CATEGORY_VARYING_INPUT ||
						parameterCategory == SLANG_PARAMETER_CATEGORY_VARYING_OUTPUT ||
						parameterCategory == SLANG_PARAMETER_CATEGORY_NONE)
						continue;

					if (fieldVarLayout->getTypeLayout()->getKind() == slang::TypeReflection::Kind::Struct)
					{
						localBindingOffset += fieldVarLayout->getOffset(SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT);
						localUniformByteOffset += fieldVarLayout->getOffset();
					}

					shaderParameter.Fields[fieldVarLayout->getName()] = {};

					AddShaderParameters(fieldVarLayout, shaderParameter.Fields[fieldVarLayout->getName()], localBindingOffset, localUniformByteOffset);
				}
				
				break;
			}
		}
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