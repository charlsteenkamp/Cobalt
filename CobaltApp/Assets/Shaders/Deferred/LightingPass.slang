module LightingPass;

import Common.Lights;
import Common.BRDF;

struct CoarseVertex
{
	float2 UV;
	float3 FragPosition;
};

struct VSOutput
{
	float4       Position:     SV_Position;
	CoarseVertex CoarseVertex: CoarseVertex;
};

[shader("vertex")]
VSOutput VSMain(uint vertexID: SV_VertexID)
{
	VSOutput output;
	output.CoarseVertex.UV = float2((vertexID << 1) & 2, vertexID & 2);
	output.Position        = float4(output.CoarseVertex.UV * 2.0 - 1.0, 0.0, 1.0);
	output.CoarseVertex.FragPosition = output.Position.xyz;

	return output;
}

struct GBuffers
{
	Sampler2D SamplerBaseColor;
	Sampler2D SamplerNormal;
	Sampler2D SamplerOcclusionRoughnessMetallic;
	Sampler2D SamplerEmissive;
};

FSOutput FSMain(CoarseVertex coarseVertex: CoarseVertex, uniform ConstantBuffer<Scene> scene, uniform GBuffers gBuffers): SV_Target
{
	float3 occlusionRoughnessMetallic = gBuffers.SamplerOcclusionRoughnessMetallic.Sample(coarseVertex.UV).xyz;

	Material material;
	material.BaseColor = gBuffers.SamplerBaseColor.Sample(coarseVertex.UV);
	material.Normal    = gBuffers.SamplerNormal.Sample(coarseVertex.UV).xyz;
	material.Occlusion = occlusionRoughnessMetallic.x;
	material.Roughness = occlusionRoughnessMetallic.y;
	material.Metallic  = occlusionRoughnessMetallic.z;
	material.Emissive  = gBuffers.SamplerEmissive.Sample(coarseVertex.UV).xyz;

	CookTorranceBRDF brdf = material.PrepareBRDF();

	float3 luminance = 0.0;
	float3 viewDir  = normalize(scene.Camera.Translation - coarseVertex.FragPosition);

	luminance += scene.DirectionalLight.Illuminate(brdf, material.Normal, viewDir);

	float3 color = luminance + material.CalculateAmbient();

	color = color / (color + float3(1.0)); // HDR tonemapping
	color = pow(color, float3(1.0 / 2.2)); // Gamma correction
	
	return float4(color, 1.0);

}

