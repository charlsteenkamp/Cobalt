#include "copch.hpp"
#include "ShaderCompiler.hpp"
#include "SlangUtils.hpp"

namespace Cobalt
{


	void ShaderCompiler::Init()
	{
		SlangUtils::CheckError(slang::createGlobalSession(sGlobalSession.writeRef()));

		slang::CompilerOptionEntry compilerOptions[] = {
			{
				.name  = slang::CompilerOptionName::VulkanUseEntryPointName,
				.value = slang::CompilerOptionValue { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1, .intValue1 = 1 }
			},
			{
				.name  = slang::CompilerOptionName::EmitSpirvViaGLSL,
				.value = slang::CompilerOptionValue { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1, .intValue1 = 1 }
			},
			{
				.name  = slang::CompilerOptionName::FloatingPointMode,
				.value = slang::CompilerOptionValue { .kind = slang::CompilerOptionValueKind::String, .stringValue0 = "precise" }
			}

		};

		slang::TargetDesc targetDescs[] = {
			{
				.format = SLANG_SPIRV,
//				.profile = sGlobalSession->findProfile("spirv_1_4"),
.profile = sGlobalSession->findProfile("GLSL_150"),
				.flags = 0 /* | SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY*/,
				.compilerOptionEntries = compilerOptions,
				.compilerOptionEntryCount = 2,
			} 
		};

		const char* searchPaths[] = { "CobaltApp/Assets/Shaders" };

		slang::SessionDesc sessionDesc = {
			.targets = targetDescs,
			.targetCount = sizeof(targetDescs) / sizeof(targetDescs[0]),
			.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
			.searchPaths = searchPaths,
			.searchPathCount = 1,
			.compilerOptionEntries = compilerOptions,
			.compilerOptionEntryCount = 3,
		};

		SlangUtils::CheckError(sGlobalSession->createSession(sessionDesc, sDefaultSession.writeRef()));
	}

	void ShaderCompiler::Shutdown()
	{
		sDefaultSession->release();
		sGlobalSession->release();
	}

	Slang::ComPtr<slang::IComponentType> ShaderCompiler::CompileShader(const std::string& filePath)
	{
		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		slang::IModule* slangModule = sDefaultSession->loadModule(filePath.c_str(), diagnosticsBlob.writeRef());
		SlangUtils::CheckBlob(diagnosticsBlob);

		slang::CompilerOptionEntry compilerOptions[] = {
			{
				.name  = slang::CompilerOptionName::VulkanUseEntryPointName,
				.value = slang::CompilerOptionValue { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1, .intValue1 = 1 }
			},
			{
				.name  = slang::CompilerOptionName::EmitSpirvViaGLSL,
				.value = slang::CompilerOptionValue { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1, .intValue1 = 1 }
			},
			{
				.name  = slang::CompilerOptionName::FloatingPointMode,
				.value = slang::CompilerOptionValue { .kind = slang::CompilerOptionValueKind::String, .stringValue0 = "precise" }
			}
		};

		int32_t entryPointCount = slangModule->getDefinedEntryPointCount();

		std::vector<slang::IComponentType*> componentTypes(entryPointCount + 1);
		componentTypes[0] = slangModule;
		
		std::vector<Slang::ComPtr<slang::IEntryPoint>> entryPoints(entryPointCount);

		for (int32_t i = 0; i < entryPointCount; i++)
		{
			SlangUtils::CheckError(slangModule->getDefinedEntryPoint(i, entryPoints[i].writeRef()));

			componentTypes[i + 1] = entryPoints[i];
		}

		Slang::ComPtr<slang::IComponentType> program;

		SlangUtils::CheckError(sDefaultSession->createCompositeComponentType(componentTypes.data(), componentTypes.size(), program.writeRef(), diagnosticsBlob.writeRef()));
		SlangUtils::CheckBlob(diagnosticsBlob);

		Slang::ComPtr<slang::IComponentType> linkedProgram;

		SlangUtils::CheckError(program->linkWithOptions(linkedProgram.writeRef(), 3, compilerOptions, diagnosticsBlob.writeRef()));
		SlangUtils::CheckBlob(diagnosticsBlob);

		return linkedProgram;
	}

}