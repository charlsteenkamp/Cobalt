#pragma once
#include "Shader.hpp"

#include <memory>
#include <string>
#include <filesystem>

namespace Cobalt
{

	// Class for shader ownership and retrieval

	class ShaderLibrary
	{
	public:
		ShaderLibrary(const std::filesystem::path& shaderDirectory);
		~ShaderLibrary();

	public:
		bool HasShader(const std::string& name) const { return mRegisteredShaders.contains(name); }

		Shader* GetShader(const std::string& name)
		{
			if (HasShader(name))
				return mRegisteredShaders.at(name).get();

			return nullptr;
		}

		const Shader* GetShader(const std::string& name) const
		{
			if (HasShader(name))
				return mRegisteredShaders.at(name).get();

			return nullptr;
		}

	private:
		void RegisterAllShaders();
		void RegisterShader(const std::filesystem::path& relativePath);

	private:
		std::filesystem::path mShaderDirectory;
		std::unordered_map<std::string, std::unique_ptr<Shader>> mRegisteredShaders;
	};

}
