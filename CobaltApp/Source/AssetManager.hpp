#pragma once
#include "Asset.hpp"
#include "Vulkan/Texture.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

#include <filesystem>

namespace Cobalt
{

	// Owns all assets

	class AssetManager
	{
	public:
		static void Init();
		static void Shutdown();

	public:
		static Texture* GetTexture(AssetHandle assetHandle);
		static Texture* GetTexture(const std::string& name);
		static Cubemap* GetCubemap(AssetHandle assetHandle);
		static Cubemap* GetCubemap(const std::string& name);
		static Mesh* GetMesh(AssetHandle meshHandle);
		static Mesh* GetMesh(const std::string& name);

	public:
		static AssetHandle RegisterTexture(const std::string& name, const TextureInfo& textureInfo);
		static AssetHandle RegisterCubemap(const std::string& name, const CubemapInfo& cubemapInfo);
		static AssetHandle RegisterMesh(const std::string& name, const MeshInfo& meshInfo);

	private:
		struct AssetIdentifier
		{
			EAssetType AssetType;
			size_t     Index;
		};

		struct AssetManagerData
		{
			std::vector<std::unique_ptr<Texture>> Textures;
			std::vector<std::unique_ptr<Cubemap>> Cubemaps;
			std::vector<std::unique_ptr<Mesh>>    Meshes;

			std::unordered_map<AssetHandle, AssetIdentifier> AssetHandleIdMap;
			std::unordered_map<std::string, AssetHandle> AssetNameHandleMap;
		};

		inline static AssetManagerData* sData = nullptr;
	};

}
