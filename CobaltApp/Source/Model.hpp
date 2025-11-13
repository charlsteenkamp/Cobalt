#pragma once
#include "Mesh.hpp"
#include "Asset.hpp"

#include <filesystem>
#include <memory>
#include <fastgltf/core.hpp>

namespace Cobalt
{

	class Model
	{
	public:
		Model(const std::filesystem::path& filePath);
		~Model();

	private:
		void Load();
		void LoadTextures(const fastgltf::Asset& gltf);
		void LoadMaterials(const fastgltf::Asset& gltf);
		void LoadMeshes(const fastgltf::Asset& gltf);

	public:
		const std::vector<AssetHandle>& GetTextureAssetHandles()  const { return mTextureAssetHandles;  }
		const std::vector<AssetHandle>& GetMaterialAssetHandles() const { return mMaterialAssetHandles; }
		const std::vector<AssetHandle>& GetMeshAssetHandles()     const { return mMeshAssetHandles;     }

		AssetHandle GetTextureAssetHandle(uint32_t index = 0)  const { return mTextureAssetHandles[index];  }
		AssetHandle GetMaterialAssetHandle(uint32_t index = 0) const { return mMaterialAssetHandles[index]; }
		AssetHandle GetMeshAssetHandle(uint32_t index = 0)     const { return mMeshAssetHandles[index];     }

	private:
		std::filesystem::path mFilePath;

		std::vector<AssetHandle> mTextureAssetHandles;
		std::vector<AssetHandle> mMaterialAssetHandles;
		std::vector<AssetHandle> mMeshAssetHandles;
	};

}
