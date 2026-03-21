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

		mMaterials.reserve(sMaxMaterialCount);
		mGPUPackedMaterials.reserve(sMaxMaterialCount);

		// Build pipelines from mesh passes

		for (const auto& pass : mRenderGraph.GetPasses())
		{
			if (!pass->HasFlag(RenderPassFlagBits::MeshPass))
				continue;

			mMeshPassNames.push_back(pass->GetName());

			PipelineInfo pipelineInfo = {
				.Shader = (Shader*)mShaderLibrary.GetShader(pass->GetShaderPath().string()),
			};

			const std::vector<Texture*> outputAttachments = mRenderGraph.GetPassOutputAttachments(pass->GetName());
			pipelineInfo.ColorAttachments.reserve(outputAttachments.size());

			for (Texture* outputAttachment : outputAttachments)
			{
				if (outputAttachment->GetImageAspectFlags() & VK_IMAGE_ASPECT_DEPTH_BIT)
					pipelineInfo.DepthAttachmentFormat = outputAttachment->GetFormat();
				else
					pipelineInfo.ColorAttachments.push_back({ false, outputAttachment->GetFormat() });
			}

			mPipelineRegistry.BuildPipeline(pass->GetName(), pipelineInfo);
		}

		// Register default opaque effect

		RegisterShaderEffect("Opaque", TransparencyMode::Opaque);

		// Register default texture

		uint32_t defaultTextureData = 0xFFFFFFFF;

		AssetHandle defaultTextureAsset = AssetManager::RegisterDefaultTexture(TextureInfo(1, 1, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT));
		Texture* defaultTexture = AssetManager::GetTexture(defaultTextureAsset);
		defaultTexture->CopyData(&defaultTextureData);

		// Register default material

		MaterialInfo materialInfo;
		materialInfo.ShaderEffectName = "Opaque";
		materialInfo.SampledTextures.push_back({ defaultTexture->GetSampler(), defaultTexture->GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

		BuildMaterial("PBR", materialInfo);
	}

	MaterialSystem::~MaterialSystem()
	{
		CO_PROFILE_FN();
	}

	ShaderEffect* MaterialSystem::RegisterShaderEffect(const std::string& shaderEffectName, TransparencyMode transparency)
	{
		CO_PROFILE_FN();

		auto& shaderEffect = mShaderEffects[shaderEffectName];
		shaderEffect.Transparency = transparency;

		// Allocate descriptors

		auto& descriptorBufferManager = GraphicsContext::Get().GetDescriptorBufferManager();

		for (const std::string& passName : mMeshPassNames)
		{
			auto& pipeline = shaderEffect.PassPipelines[passName];
			auto& descriptors = shaderEffect.PassDescriptors[passName];

			pipeline = mPipelineRegistry.GetPipeline(passName);

			VkDescriptorSetLayout descriptorSetLayout = pipeline->GetInfo().Shader->GetDescriptorSetLayouts()[0];

			for (uint32_t i = 0; i < GraphicsContext::Get().GetFrameCount(); i++)
				descriptors.push_back(descriptorBufferManager.AllocateDescriptor(descriptorSetLayout, true, true));
		}

		return &mShaderEffects[shaderEffectName];
	}

	Material* MaterialSystem::BuildMaterial(const std::string& materialName, const MaterialInfo& materialInfo)
	{
		CO_PROFILE_FN();

		auto it = mMaterialInfoMaterialMap.find(materialInfo);

		if (it != mMaterialInfoMaterialMap.end())
		if (true)
		{
			Material* material = (*it).second;
			mNameMaterialMap[materialName] = material;
			return material;
		}
		
		if (!mShaderEffects.contains(materialInfo.ShaderEffectName))
			return nullptr;

		const ShaderEffect* shaderEffect = &mShaderEffects.at(materialInfo.ShaderEffectName);

		mMaterials.emplace_back(materialInfo, shaderEffect, mMaterials.size());
		mMaterialInfoMaterialMap[materialInfo] = &mMaterials.back();
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

		return mNameMaterialMap.at(materialName);
	}

	Material* MaterialSystem::GetMaterial(const MaterialInfo& materialInfo) const
	{
		CO_PROFILE_FN();

		if (!mMaterialInfoMaterialMap.contains(materialInfo))
			return nullptr;

		return mMaterialInfoMaterialMap.at(materialInfo);
	}

}
