#pragma once
//#include "Vulkan/ShaderStructs.hpp"
#include "Vulkan/Pipeline.hpp"
#include "Vulkan/HashUtils.hpp"
#include "Vulkan/DescriptorBufferManager.hpp"

#include <glm/glm.hpp>

namespace Cobalt
{

	class Renderer;

#define CO_INVALID_MATERIAL_HANDLE -1

	using TextureHandle = uint32_t;
	using MaterialHandle = uint32_t;

#define CO_DEFAULT_TEXTURE_HANDLE 0

	struct GPUPackedMaterial
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

		size_t Hash() const
		{
			size_t h = 0;

			HashCombine(h, std::hash<TextureHandle>{}(BaseColorMapHandle));
			HashCombine(h, std::hash<TextureHandle>{}(NormalMapHandle));
			HashCombine(h, std::hash<TextureHandle>{}(OcclusionRoughnessMetallicMapHandle));
			HashCombine(h, std::hash<TextureHandle>{}(EmissiveMapHandle));

			HashCombine(h, std::hash<float>{}(BaseColorFactor.x));
			HashCombine(h, std::hash<float>{}(BaseColorFactor.y));
			HashCombine(h, std::hash<float>{}(BaseColorFactor.z));
			HashCombine(h, std::hash<float>{}(BaseColorFactor.w));

			HashCombine(h, std::hash<float>{}(NormalScale));
			HashCombine(h, std::hash<float>{}(OcclusionStrength));
			HashCombine(h, std::hash<float>{}(RoughnessFactor));
			HashCombine(h, std::hash<float>{}(MetallicFactor));

			HashCombine(h, std::hash<float>{}(EmissiveFactor.x));
			HashCombine(h, std::hash<float>{}(EmissiveFactor.y));
			HashCombine(h, std::hash<float>{}(EmissiveFactor.z));

			return h;
		}
	};

	struct SampledTexture
	{
		VkImageView ImageView = VK_NULL_HANDLE;
		VkSampler Sampler = VK_NULL_HANDLE;
	};

	enum class TransparencyMode
	{
		Opaque,
		Transparent
	};

	using MaterialHash = uint64_t;

	// Indexed by mesh pass -> per-frame descriptor handles
	using PassDescriptorHandles = std::unordered_map<std::string, std::vector<DescriptorHandle>>;

	struct ShaderEffect
	{
		std::unordered_map<std::string, Pipeline*> PassPipelines;
		TransparencyMode Transparency;
		PassDescriptorHandles PassDescriptors;
	};

	struct MaterialInfo
	{
		GPUPackedMaterial PackedMaterial;
		std::vector<SampledTexture> SampledTextures;
		std::string ShaderEffectName;

		MaterialHash Hash() const
		{
			size_t h = 0;

			// SampledTextures aren't hashed

			HashCombine(h, std::hash<size_t>{}(PackedMaterial.Hash()));
			HashCombine(h, std::hash<std::string>{}(ShaderEffectName));

			return h;
		}

		bool operator==(const MaterialInfo& other) const
		{
			return Hash() == other.Hash();
		}
	};

	class Material
	{
	public:
		Material() = default;
		Material(const MaterialInfo& materialInfo, const ShaderEffect* shaderEffect, MaterialHandle materialHandle)
			: mMaterialInfo(materialInfo), mShaderEffect(shaderEffect), mMaterialHandle(materialHandle)
		{
		}

		~Material() = default;

	public:
		const MaterialInfo& GetMaterialInfo() const { return mMaterialInfo; }
		const ShaderEffect* GetShaderEffect() const { return mShaderEffect; }

		MaterialHandle GetMaterialHandle() const { return mMaterialHandle; }

	private:
		const MaterialInfo& mMaterialInfo;
		const ShaderEffect* mShaderEffect = nullptr;

		MaterialHandle mMaterialHandle = -1;
	};

}
