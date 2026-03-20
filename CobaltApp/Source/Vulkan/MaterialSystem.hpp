#pragma once
#include "Material.hpp"
#include "RenderGraph.hpp"
#include "ShaderLibrary.hpp"
#include "PipelineRegistry.hpp"

namespace Cobalt
{

	class MaterialSystem
	{
	public:
		MaterialSystem(const RenderGraph& renderGraph, const ShaderLibrary& shaderLibrary, PipelineRegistry& pipelineRegistry);
		~MaterialSystem();

	public:
		ShaderEffect* RegisterShaderEffect(const std::string& shaderEffectName, TransparencyMode transparency);

		Material* BuildMaterial(const std::string& materialName, const MaterialInfo& materialInfo);
		Material* GetMaterial(const std::string& materialName) const;

	public:
		const std::vector<GPUPackedMaterial>& GetGPUPackedMaterials() const { return mGPUPackedMaterials; }

	private:
		struct MaterialInfoHash
		{
			size_t operator()(const MaterialInfo& materialInfo) const
			{
				return materialInfo.Hash();
			}
		};

		const RenderGraph& mRenderGraph;
		const ShaderLibrary& mShaderLibrary;
		PipelineRegistry& mPipelineRegistry;

		std::vector<std::string> mMeshPassNames;

		std::unordered_map<std::string, ShaderEffect> mShaderEffects;

		std::unordered_map<std::string, Material*> mNameMaterialMap;
		std::unordered_map<std::string, MaterialHandle> mNameMaterialHandleMap;
		std::unordered_map<MaterialInfo, Material*, MaterialInfoHash> mMaterialInfoMaterialMap;

		std::vector<Material> mMaterials;
		std::vector<GPUPackedMaterial> mGPUPackedMaterials;
	};

}
