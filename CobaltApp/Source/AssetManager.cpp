#include "copch.hpp"
#include "AssetManager.hpp"
#include "Vulkan/Renderer.hpp"

#include <random>

namespace Cobalt
{

	static AssetHandle GenerateAssetHandle()
	{	
		static std::random_device rd;
		static std::mt19937 generator(rd());

		std::uniform_int_distribution<uint64_t> dist(
			std::numeric_limits<uint64_t>::min(),
			std::numeric_limits<uint64_t>::max()
		);

		return dist(generator);
	}

	void AssetManager::Init()
	{
		CO_PROFILE_FN();
		if (sData)
			return;

		sData = new AssetManagerData();
	}

	void AssetManager::Shutdown()
	{
		CO_PROFILE_FN();

		delete sData;
	}

	Texture* AssetManager::GetTexture(AssetHandle textureHandle)
	{
		CO_PROFILE_FN();

		if (!sData->AssetHandleIdMap.contains(textureHandle))
			return nullptr;

		size_t index = sData->AssetHandleIdMap.at(textureHandle).Index;
		return sData->Textures[index].get();
	}

	Mesh* AssetManager::GetMesh(AssetHandle meshHandle)
	{
		CO_PROFILE_FN();

		if (!sData->AssetHandleIdMap.contains(meshHandle))
			return nullptr;

		size_t index = sData->AssetHandleIdMap.at(meshHandle).Index;
		return sData->Meshes[index].get();
	}

	AssetHandle AssetManager::RegisterDefaultTexture(const TextureInfo& textureInfo)
	{
		CO_PROFILE_FN();

		AssetHandle assetHandle = CO_DEFAULT_TEXTURE_HANDLE;

		sData->Textures.push_back(std::make_unique<Texture>(textureInfo));
		sData->AssetHandleIdMap[assetHandle] = { EAssetType::Texture, sData->Textures.size() - 1 };

		return assetHandle;

	}

	AssetHandle AssetManager::RegisterTexture(const TextureInfo& textureInfo)
	{
		CO_PROFILE_FN();

		AssetHandle assetHandle = GenerateAssetHandle();

		sData->Textures.push_back(std::make_unique<Texture>(textureInfo));
		sData->AssetHandleIdMap[assetHandle] = { EAssetType::Texture, sData->Textures.size() - 1 };

		return assetHandle;
	}

	AssetHandle AssetManager::RegisterMesh(const MeshInfo& meshInfo)
	{
		CO_PROFILE_FN();

		AssetHandle assetHandle = GenerateAssetHandle();

		sData->Meshes.push_back(std::make_unique<Mesh>(meshInfo));
		sData->AssetHandleIdMap[assetHandle] = { EAssetType::Mesh, sData->Meshes.size() - 1 };

		return assetHandle;
	}

}