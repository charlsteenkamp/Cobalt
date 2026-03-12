#include "copch.hpp"
#include "Shader.hpp"
#include "GraphicsContext.hpp"
#include "ShaderCompiler.hpp"
#include "SlangUtils.hpp"
#include "ShaderLayoutBuilder.hpp"

#include <algorithm>
#include <fstream>
#include <filesystem>

namespace Cobalt
{

	Shader::Shader(const std::string& filePath)
	{
		mLinkedProgram = ShaderCompiler::CompileShader(filePath);

		InitShaderStages();
		InitDescriptorSetLayouts();
	}

	void Shader::InitShaderStages()
	{
		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		Slang::ComPtr<slang::IBlob> spirvBlob;

		slang::ProgramLayout* programLayout = mLinkedProgram->getLayout();

		mShaderStages.resize(programLayout->getEntryPointCount());
		
		for (int32_t i = 0; i < mShaderStages.size(); i++)
		{
			slang::EntryPointReflection* entryPoint = programLayout->getEntryPointByIndex(i);

			SlangUtils::CheckError(mLinkedProgram->getEntryPointCode(i, 0, spirvBlob.writeRef(), diagnosticsBlob.writeRef()));
			SlangUtils::CheckBlob(diagnosticsBlob);
			
			mShaderStages[i].SPIRV          = std::vector<uint8_t>((uint8_t*)spirvBlob->getBufferPointer(), (uint8_t*)spirvBlob->getBufferPointer() + spirvBlob->getBufferSize());
			mShaderStages[i].Stage          = SlangUtils::SlangStageToVkShaderStageFlagBits(entryPoint->getStage());
			mShaderStages[i].EntryPointName = entryPoint->getName();
		}
	}

	void Shader::InitDescriptorSetLayouts()
	{
		ShaderLayoutBuilder layoutBuilder(mLinkedProgram->getLayout());

		mDescriptorSetLayouts = layoutBuilder.GetDescriptorSetLayouts();
		mRootShaderParam = layoutBuilder.GetRootShaderParameter();
	}

}