#pragma once
//#include "Vulkan/ShaderStructs.hpp"
#include "Vulkan/Pipeline.hpp"

#include <glm/glm.hpp>

namespace Cobalt
{

	class Renderer;

#define CO_INVALID_MATERIAL_HANDLE -1

	using TextureHandle = uint32_t;
	using MaterialHandle = uint32_t;

#define CO_DEFAULT_TEXTURE_HANDLE 0

	struct MaterialData
	{
		TextureHandle BaseColorMapHandle                  = CO_DEFAULT_TEXTURE_HANDLE; // albedo for non-metallic materials, base color otherwise
		TextureHandle NormalMapHandle                     = CO_DEFAULT_TEXTURE_HANDLE; // defined in tangent space
		TextureHandle OcclusionRoughnessMetallicMapHandle = CO_DEFAULT_TEXTURE_HANDLE; // R - occlusion, G - roughness, B - metallic
		TextureHandle EmissiveMapHandle                   = CO_DEFAULT_TEXTURE_HANDLE; // RGB

		glm::vec4 BaseColorFactor = glm::vec4(1.0f);

		float NormalScale = 1.0f;
		float OcclusionStrength = 1.0f;
		float RoughnessFactor = 1.0f;
		float MetallicFactor = 0.0f;

		glm::vec3 EmissiveFactor = glm::vec3(0.0f);
		float __padding1;
	};


	struct MaterialInfo
	{
		const MaterialData& MaterialData;
		//const Pipeline&     Pipeline;
	};

	class Material
	{
	public:
		Material(const MaterialInfo& materialInfo);
		~Material();

	public:
		//const Pipeline& GetPipeline() const { return mPipeline; }

		      MaterialData& GetMaterialData()       { return mMaterialData; }
		const MaterialData& GetMaterialData() const { return mMaterialData; }

		MaterialHandle GetMaterialHandle() const { return mMaterialHandle; }

	private:
		MaterialData mMaterialData;
		//const Pipeline& mPipeline;

		MaterialHandle mMaterialHandle = CO_INVALID_MATERIAL_HANDLE;

		friend class Renderer;
	};

}
