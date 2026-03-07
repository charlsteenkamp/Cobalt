#pragma once
#include "ShaderParameters.hpp"

#include <slang/slang.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>

namespace Cobalt
{

	struct BindingOffset
	{
		BindingOffset() = default;
		BindingOffset(slang::VariableLayoutReflection* varLayout)
		{
			if (varLayout)
			{
				Set = (uint32_t)varLayout->getBindingSpace(SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT);
				Binding = (uint32_t)varLayout->getOffset(SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT);

				for (int32_t i = 0; i < varLayout->getCategoryCount(); i++)
				{
					slang::ParameterCategory category = varLayout->getCategoryByIndex(i);
					uint32_t offset = varLayout->getOffset((SlangParameterCategory)category);

					int b = 0;
				}
			}
		}

		void operator+=(const BindingOffset& other)
		{
			Set += other.Set;
			Binding += other.Binding;
		}

		uint32_t Set     = 0;
		uint32_t Binding = 0;
	};

	inline BindingOffset operator+(const BindingOffset& lhs, const BindingOffset& rhs)
	{
		BindingOffset result;
		result.Set = lhs.Set + rhs.Set;
		result.Binding = lhs.Binding + rhs.Binding;

		return result;
	}

	struct DescriptorSetInfo
	{
		std::vector<VkDescriptorSetLayoutBinding> Bindings;
		uint32_t Space = -1;

		void AddBinding(const VkDescriptorSetLayoutBinding& descriptorSetLayoutBinding)
		{
			auto it = std::find_if(Bindings.begin(), Bindings.end(), [=](const VkDescriptorSetLayoutBinding& descriptorSetLayoutBindingIter) -> bool
			{
				return descriptorSetLayoutBindingIter.binding == descriptorSetLayoutBinding.binding;
			});

			if (it == Bindings.end())
			{
				Bindings.push_back(descriptorSetLayoutBinding);
			}
		}
	};

	class ShaderLayoutBuilder
	{
	public:
		ShaderLayoutBuilder(slang::ProgramLayout* programLayout);
		~ShaderLayoutBuilder();

	public:
		const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return mDescriptorSetLayouts; }
		const std::vector<VkPushConstantRange>& GetPushConstantRanges() const { return mPushConstantRanges; }

		const ShaderParameterMap& GetShaderParameters() const { return mShaderParameters; }

	private:
		void AddGlobalBindings(slang::VariableLayoutReflection* globalVarLayout);
		void AddEntryPoint(slang::EntryPointLayout* entryPointLayout);
		void BuildDescriptorSetLayouts();

		void AddDescriptorBindings(slang::TypeLayoutReflection* typeLayout, BindingOffset bindingOffset);
		void AddConstantBufferDescriptorBindings(slang::TypeLayoutReflection* elementTypeLayout, BindingOffset containerOffset, BindingOffset elementOffset);
		void AddPushConstantRange(slang::TypeLayoutReflection* elementTypeLayout, BindingOffset containerOffset, BindingOffset elementOffset);

		int32_t FindOrAddDescriptorSet(int32_t space);

	private:
		std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
		std::vector<VkPushConstantRange> mPushConstantRanges;
		std::vector<DescriptorSetInfo> mDescriptorSetInfos;
		std::unordered_map<int32_t, int32_t> mDescriptorSpaceIndexMap; // space - index into mDescriptorSetInfos
		
		ShaderParameterMap mShaderParameters;
	};

}
