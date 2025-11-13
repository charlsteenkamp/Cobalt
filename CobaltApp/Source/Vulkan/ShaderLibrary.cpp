#include "copch.hpp"
#include "ShaderLibrary.hpp"

namespace Cobalt
{

	ShaderLibrary::ShaderLibrary(const std::filesystem::path& shaderDirectory)
		: mShaderDirectory(shaderDirectory)
	{
		CO_PROFILE_FN();
	}

	ShaderLibrary::~ShaderLibrary()
	{
		CO_PROFILE_FN();
	}

	ShaderHandle ShaderLibrary::RegisterShader(const std::filesystem::path& relativePath)
	{
		CO_PROFILE_FN();

		mRegisteredShaders.push_back(std::make_unique<Shader>((mShaderDirectory / relativePath).string()));
		return mRegisteredShaders.size() - 1;
	}
	
	Shader* ShaderLibrary::GetShader(ShaderHandle shaderHandle)
	{
		CO_PROFILE_FN();

		if (!HasShader(shaderHandle))
			return nullptr;

		return mRegisteredShaders[shaderHandle].get();
	}

	const Shader* ShaderLibrary::GetShader(ShaderHandle shaderHandle) const
	{
		CO_PROFILE_FN();

		if (!HasShader(shaderHandle))
			return nullptr;

		return mRegisteredShaders[shaderHandle].get();
	}

}