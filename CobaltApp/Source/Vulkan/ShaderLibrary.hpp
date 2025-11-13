#pragma once
#include "Shader.hpp"

#include <memory>
#include <filesystem>

namespace Cobalt
{

	using ShaderHandle = uint32_t;

	// Class for shader ownership and retrieval

	class ShaderLibrary
	{
	public:
		ShaderLibrary(const std::filesystem::path& shaderDirectory);
		~ShaderLibrary();

	public:
		ShaderHandle RegisterShader(const std::filesystem::path& relativePath);

		bool HasShader(ShaderHandle shaderHandle) const { return shaderHandle < mRegisteredShaders.size(); }

		      Shader* GetShader(ShaderHandle shaderHandle);
		const Shader* GetShader(ShaderHandle shaderHandle) const;

		      Shader* operator[](ShaderHandle shaderHandle)       { return GetShader(shaderHandle); }
		const Shader* operator[](ShaderHandle shaderHandle) const { return GetShader(shaderHandle); }

	private:
		std::filesystem::path mShaderDirectory;
		std::vector<std::unique_ptr<Shader>> mRegisteredShaders;
	};

}
