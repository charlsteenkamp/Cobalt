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
		static Texture* GetTexture(AssetHandle textureHandle);
		static Mesh* GetMesh(AssetHandle meshHandle);
		static Material* GetMaterial(AssetHandle materialHandle);

	public:
		static AssetHandle RegisterTexture(const TextureInfo& textureInfo);
		static AssetHandle RegisterMesh(const MeshInfo& meshInfo);
		static AssetHandle RegisterMaterial(const MaterialInfo& materialInfo);

	private:
		static void RegisterDefaultTexture();
		static void RegisterDefaultMaterial();

	private:
		struct AssetIdentifier
		{
			EAssetType AssetType;
			size_t     Index;
		};

		struct AssetManagerData
		{
			std::vector<std::unique_ptr<Texture>>  Textures;
			std::vector<std::unique_ptr<Mesh>>     Meshes;
			std::vector<std::unique_ptr<Material>> Materials;

			std::unordered_map<AssetHandle, AssetIdentifier> AssetHandleIdMap;
		};

		inline static AssetManagerData* sData = nullptr;
	};

}
