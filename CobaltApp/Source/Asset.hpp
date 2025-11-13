#pragma once
#include <memory>

#define CO_INVALID_ASSET_HANDLE 0
#define CO_DEFAULT_TEXTURE_ASSET 1
#define CO_DEFAULT_MATERIAL_ASSET 2

namespace Cobalt
{

	using AssetHandle = uint64_t;

	enum class EAssetType
	{
		None,
		Texture,
		Mesh,
		Material
	};

}
