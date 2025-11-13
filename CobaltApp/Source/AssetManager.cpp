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

		RegisterDefaultTexture();
		RegisterDefaultMaterial();
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

	Material* AssetManager::GetMaterial(AssetHandle materialHandle)
	{
		CO_PROFILE_FN();

		if (!sData->AssetHandleIdMap.contains(materialHandle))
			return nullptr;

		size_t index = sData->AssetHandleIdMap.at(materialHandle).Index;
		return sData->Materials[index].get();
	}

	AssetHandle AssetManager::RegisterTexture(const TextureInfo& textureInfo)
	{
		CO_PROFILE_FN();

		AssetHandle assetHandle = GenerateAssetHandle();

		sData->Textures.push_back(std::make_unique<Texture>(textureInfo));
		sData->AssetHandleIdMap[assetHandle] = { EAssetType::Texture, sData->Textures.size() - 1 };

		Renderer::UploadTexture(*sData->Textures.back(), Renderer::GetPBRPipeline());

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

	AssetHandle AssetManager::RegisterMaterial(const MaterialInfo& materialInfo)
	{
		CO_PROFILE_FN();

		AssetHandle assetHandle = GenerateAssetHandle();
	
		sData->Materials.push_back(std::make_unique<Material>(materialInfo));
		sData->AssetHandleIdMap[assetHandle] = { EAssetType::Material, sData->Materials.size() - 1};

		Renderer::UploadMaterial(assetHandle, *sData->Materials.back());

		return assetHandle;
	}

	void AssetManager::RegisterDefaultTexture()
	{
		CO_PROFILE_FN();

		uint32_t textureData = 0xFFFFFFFF;

		sData->Textures.push_back(std::make_unique<Texture>(TextureInfo(1, 1, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)));
		sData->Textures.back()->CopyData(&textureData);
		sData->AssetHandleIdMap[CO_DEFAULT_TEXTURE_ASSET] = { EAssetType::Texture, 0 };

		Renderer::UploadTexture(*sData->Textures.back(), Renderer::GetPBRPipeline());
	}

	void AssetManager::RegisterDefaultMaterial()
	{
		CO_PROFILE_FN();

		sData->Materials.push_back(std::make_unique<Material>(MaterialInfo(MaterialData(), Renderer::GetPBRPipeline())));
		sData->AssetHandleIdMap[CO_DEFAULT_MATERIAL_ASSET] = { EAssetType::Material, 0 };

		Renderer::UploadMaterial(CO_DEFAULT_MATERIAL_ASSET, *sData->Materials.back());
	}


}