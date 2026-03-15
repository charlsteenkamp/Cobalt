#include "copch.hpp"
#include "Material.hpp"
#include "Vulkan/Renderer.hpp"
#include "Vulkan/GraphicsContext.hpp"

namespace Cobalt
{

	Material::Material(const MaterialInfo& materialInfo)
		: mMaterialData(materialInfo.MaterialData)/*, mPipeline(materialInfo.Pipeline)*/
	{
		CO_PROFILE_FN();

		//mPipeline = std::make_unique<Pipeline>(
		//	PipelineInfo {
		//		.Shader = std::make_unique<Shader>("CobaltApp/Assets/Shaders/DefaultShader.slang"),
		//		.PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		//		.EnableDepthTesting = true
		//	},
		//	Renderer::GetMainRenderPass()
		//);

		//uint32_t frameCount = GraphicsContext::Get().GetFrameCount();

		//mGlobalDescriptorSets = mPipeline.AllocateDescriptorSets(GraphicsContext::Get().GetDescriptorPool(), 0, frameCount);

		//mMaterialHandle = std::hash<MaterialData>{}(materialData);
	}

	Material::~Material()
	{
		CO_PROFILE_FN();
	}

}