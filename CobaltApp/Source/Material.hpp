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

	struct ShaderEffect
	{
		std::unordered_map<std::string, Pipeline*> PassPipelines;
		TransparencyMode Transparency;
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

			HashCombine(h, std::hash<TextureHandle>{}(PackedMaterial.BaseColorMapHandle));
			HashCombine(h, std::hash<TextureHandle>{}(PackedMaterial.NormalMapHandle));
			HashCombine(h, std::hash<TextureHandle>{}(PackedMaterial.OcclusionRoughnessMetallicMapHandle));
			HashCombine(h, std::hash<TextureHandle>{}(PackedMaterial.EmissiveMapHandle));

			HashCombine(h, std::hash<float>{}(PackedMaterial.BaseColorFactor.x));
			HashCombine(h, std::hash<float>{}(PackedMaterial.BaseColorFactor.y));
			HashCombine(h, std::hash<float>{}(PackedMaterial.BaseColorFactor.z));
			HashCombine(h, std::hash<float>{}(PackedMaterial.BaseColorFactor.w));

			HashCombine(h, std::hash<float>{}(PackedMaterial.NormalScale));
			HashCombine(h, std::hash<float>{}(PackedMaterial.OcclusionStrength));
			HashCombine(h, std::hash<float>{}(PackedMaterial.RoughnessFactor));
			HashCombine(h, std::hash<float>{}(PackedMaterial.MetallicFactor));

			HashCombine(h, std::hash<float>{}(PackedMaterial.EmissiveFactor.x));
			HashCombine(h, std::hash<float>{}(PackedMaterial.EmissiveFactor.y));
			HashCombine(h, std::hash<float>{}(PackedMaterial.EmissiveFactor.z));

			HashCombine(h, std::hash<std::string>{}(ShaderEffectName));

			return h;
		}
	};

	// Indexed by mesh pass -> per-frame descriptor handles
	using PassDescriptorHandles = std::unordered_map<std::string, std::vector<DescriptorHandle>>;

	class Material
	{
	public:
		Material(const MaterialInfo& materialInfo, const ShaderEffect& shaderEffect, const PassDescriptorHandles& passDescriptorHandles)
			: mMaterialInfo(materialInfo), mShaderEffect(shaderEffect), mPassDescriptorHandles(passDescriptorHandles)
		{
		}

		~Material() = default;

	public:
		      MaterialInfo& GetMaterialInfo()       { return mMaterialInfo; }
		const MaterialInfo& GetMaterialInfo() const { return mMaterialInfo; }

		const ShaderEffect& GetShaderEffect() const { return mShaderEffect; }

		DescriptorHandle GetDescriptorHandle(const std::string& passName, uint32_t frameIndex) const
		{
			if (!mPassDescriptorHandles.contains(passName))
				return -1;

			const auto& descriptorHandles = mPassDescriptorHandles.at(passName);

			if (frameIndex >= descriptorHandles.size())
				return -1;

			return descriptorHandles[frameIndex];
		}

	private:
		MaterialInfo mMaterialInfo;
		const ShaderEffect& mShaderEffect;

		PassDescriptorHandles mPassDescriptorHandles;
	};

}
