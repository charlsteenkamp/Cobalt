#pragma once
#include "Vulkan/ShaderStructs.hpp"
#include "Vulkan/Pipeline.hpp"

namespace Cobalt
{

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

	private:
		MaterialData mMaterialData;
		const Pipeline& mPipeline;
	};

}
