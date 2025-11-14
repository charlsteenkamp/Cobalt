#pragma once
#include "Vulkan/ShaderStructs.hpp"
#include "Vulkan/Pipeline.hpp"

namespace Cobalt
{

	class Renderer;

	struct MaterialInfo
	{
		const MaterialData& MaterialData;
		const Pipeline&     Pipeline;
	};

	class Material
	{
	public:
		Material(const MaterialInfo& materialInfo);
		~Material();

	public:
		const Pipeline& GetPipeline() const { return mPipeline; }

		      MaterialData& GetMaterialData()       { return mMaterialData; }
		const MaterialData& GetMaterialData() const { return mMaterialData; }

		MaterialHandle GetMaterialHandle() const { return mMaterialHandle; }

	private:
		MaterialData mMaterialData;
		const Pipeline& mPipeline;

		MaterialHandle mMaterialHandle = CO_INVALID_MATERIAL_HANDLE;

		friend class Renderer;
	};

}
