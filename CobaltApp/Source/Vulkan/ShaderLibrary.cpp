#include "copch.hpp"
#include "ShaderLibrary.hpp"

namespace Cobalt
{

	ShaderLibrary::ShaderLibrary(const std::filesystem::path& shaderDirectory)
		: mShaderDirectory(shaderDirectory)
	{
		CO_PROFILE_FN();

		RegisterAllShaders();
	}

	ShaderLibrary::~ShaderLibrary()
	{
		CO_PROFILE_FN();
	}

	void ShaderLibrary::RegisterAllShaders()
	{
		CO_PROFILE_FN();
		
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(mShaderDirectory))
		{
			if (!dirEntry.is_regular_file())
				continue;

			std::filesystem::path relPath = std::filesystem::relative(dirEntry.path(), mShaderDirectory);

			RegisterShader(relPath);
		}
	}

	void ShaderLibrary::RegisterShader(const std::filesystem::path& relativePath)
	{
		CO_PROFILE_FN();

		std::filesystem::path absPath = mShaderDirectory / relativePath;

		mRegisteredShaders[relativePath.string()] = std::make_unique<Shader>(absPath.string());
	}

}