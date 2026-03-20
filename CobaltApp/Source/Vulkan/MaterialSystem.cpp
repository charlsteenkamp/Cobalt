#include "copch.hpp"
#include "MaterialSystem.hpp"
#include "RenderPass.hpp"

#include "AssetManager.hpp"

namespace Cobalt
{

	MaterialSystem::MaterialSystem(const RenderGraph& renderGraph, const ShaderLibrary& shaderLibrary, PipelineRegistry& pipelineRegistry)
		: mRenderGraph(renderGraph), mShaderLibrary(shaderLibrary), mPipelineRegistry(pipelineRegistry)
	{
		CO_PROFILE_FN();

		// Build pipelines from mesh passes

		for (const auto& pass : mRenderGraph.GetPasses())
		{
			if (!pass->HasFlag(RenderPassFlagBits::MeshPass))
				continue;

			mMeshPassNames.push_back(pass->GetName());

			PipelineInfo pipelineInfo = {
				.Shader = *mShaderLibrary.GetShader(pass->GetShaderPath().string()),
			};

			const std::vector<Texture*> outputAttachments = mRenderGraph.GetPassOutputAttachments(pass->GetName());
			pipelineInfo.ColorAttachments.reserve(outputAttachments.size());

			for (Texture* outputAttachment : outputAttachments)
				pipelineInfo.ColorAttachments.push_back({ false, outputAttachment->GetFormat() });

			mPipelineRegistry.BuildPipeline(pass->GetName(), pipelineInfo);
		}

		// Register default opaque effect

		RegisterShaderEffect("Opaque", TransparencyMode::Opaque);

		// Register default texture

		uint32_t defaultTextureData = 0xFFFFFFFF;

		AssetHandle defaultTextureAsset = AssetManager::RegisterTexture(TextureInfo(1, 1, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT));
		Texture* defaultTexture = AssetManager::GetTexture(defaultTextureAsset);
		defaultTexture->CopyData(&defaultTextureData);

		// Register default material

		MaterialInfo materialInfo;
		materialInfo.ShaderEffectName = "Opaque";
		materialInfo.SampledTextures.push_back({ defaultTexture->GetImageView(), defaultTexture->GetSampler() });

		BuildMaterial("PBR", materialInfo);
	}

	MaterialSystem::~MaterialSystem()
	{
		CO_PROFILE_FN();
	}

	ShaderEffect* MaterialSystem::RegisterShaderEffect(const std::string& shaderEffectName, TransparencyMode transparency)
	{
		CO_PROFILE_FN();

		mShaderEffects[shaderEffectName] = ShaderEffect{
			.Transparency = transparency
		};

		for (const std::string& passName : mMeshPassNames)
			mShaderEffects[shaderEffectName].PassPipelines[passName] = mPipelineRegistry.GetPipeline(passName);

		return &mShaderEffects[shaderEffectName];
	}

	Material* MaterialSystem::BuildMaterial(const std::string& materialName, const MaterialInfo& materialInfo)
	{
		CO_PROFILE_FN();

		auto it = mMaterialInfoMaterialMap.find(materialInfo);

		if (it != mMaterialInfoMaterialMap.end())
		{
			Material* material = (*it).second;
			mNameMaterialMap[materialName] = material;
			return material;
		}
		
		if (!mShaderEffects.contains(materialInfo.ShaderEffectName))
			return nullptr;

		auto& descriptorBufferManager = GraphicsContext::Get().GetDescriptorBufferManager();
		const ShaderEffect& shaderEffect = mShaderEffects.at(materialInfo.ShaderEffectName);

		// Allocate descriptor handles for each pass

		PassDescriptorHandles passDescriptorHandles;

		for (const auto& [passName, pipelinePtr] : shaderEffect.PassPipelines)
		{
			passDescriptorHandles[passName] = {};

			VkDescriptorSetLayout descriptorSetLayout = pipelinePtr->GetInfo().Shader.GetDescriptorSetLayouts()[0];

			for (uint32_t i = 0; i < GraphicsContext::Get().GetFrameCount(); i++)
				passDescriptorHandles[passName].push_back(descriptorBufferManager.AllocateDescriptor(descriptorSetLayout, true, true));
		}

		mMaterials.emplace_back(materialInfo, shaderEffect, passDescriptorHandles);
		mNameMaterialMap[materialName] = &mMaterials.back();
		mNameMaterialHandleMap[materialName] = mMaterials.size() - 1;

		mGPUPackedMaterials.push_back(materialInfo.PackedMaterial);
		
		return &mMaterials.back();
	}

	Material* MaterialSystem::GetMaterial(const std::string& materialName) const
	{
		CO_PROFILE_FN();

		if (!mNameMaterialMap.contains(materialName))
			return nullptr;

		return mNameMaterialMap.at(name);
	}

}
