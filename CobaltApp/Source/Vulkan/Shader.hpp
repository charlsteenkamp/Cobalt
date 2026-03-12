#pragma once
#include "ShaderParameters.hpp"

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang-com-helper.h>

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace Cobalt
{

	class ShaderCompiler;

	struct ShaderStage
	{
		std::vector<uint8_t> SPIRV;
		VkShaderStageFlagBits Stage;
		std::string EntryPointName;
	};

	class Shader
	{
	public:
		Shader(const std::string& filePath);

	public:
		const std::vector<ShaderStage>& GetShaderStages() const { return mShaderStages; }
		const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return mDescriptorSetLayouts; }

		ShaderParameter& GetRootShaderParameter() { return mRootShaderParam; }

	private:
		void InitShaderStages();
		void InitDescriptorSetLayouts();

	private:
		Slang::ComPtr<slang::IComponentType> mLinkedProgram;

		std::vector<ShaderStage> mShaderStages;
		std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
		ShaderParameter mRootShaderParam;

		friend class ShaderCompiler;
	};

}