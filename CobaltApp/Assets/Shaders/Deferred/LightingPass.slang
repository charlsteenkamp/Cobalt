module LightingPass;

import Common.Common;
import Common.Material;
import Common.Lights;
import Common.BRDF;

struct VSOutput
{
	float4 Position: SV_Position;
	float2 UV;
};

[shader("vertex")]
VSOutput VSMain(uint vertexID: SV_VertexID)
{
	VSOutput output;
	output.UV       = float2((vertexID << 1) & 2, vertexID & 2);
	output.Position = float4(output.UV * 2.0 - 1.0, 0.0, 1.0);

	return output;
}

struct GBuffers
{
	Sampler2D SamplerPosition;
	Sampler2D SamplerBaseColor;
	Sampler2D SamplerNormal;
	Sampler2D SamplerOcclusionRoughnessMetallic;
	Sampler2D SamplerEmissive;
};

[shader("fragment")]
float4 FSMain(VSOutput input, uniform ConstantBuffer<Scene> scene, uniform GBuffers gBuffers): SV_Target
{
	float3 fragPosition = gBuffers.SamplerPosition.Sample(input.UV).xyz;
	float3 occlusionRoughnessMetallic = gBuffers.SamplerOcclusionRoughnessMetallic.Sample(input.UV).xyz;
	float3 fragPosition = gBuffers.SamplerPosition.Sample(input.UV).xyz;
	float3 occlusionRoughnessMetallic = gBuffers.SamplerOcclusionRoughnessMetallic.Sample(input.UV).xyz;

	Material material;
	material.BaseColor = gBuffers.SamplerBaseColor.Sample(coarseVertex.UV);
	material.Normal    = gBuffers.SamplerNormal.Sample(coarseVertex.UV).xyz;
	material.Occlusion = occlusionRoughnessMetallic.x;
	material.Roughness = occlusionRoughnessMetallic.y;
	material.Metallic  = occlusionRoughnessMetallic.z;
	material.Emissive  = gBuffers.SamplerEmissive.Sample(input.UV).xyz;

	CookTorranceBRDF brdf = material.PrepareBRDF();

	float3 luminance = 0.0;
	float3 viewDir  = normalize(scene.Camera.Translation - fragPosition);

	luminance += scene.DirectionalLight.Illuminate(brdf, material.Normal, viewDir);

	float3 color = luminance + material.CalculateAmbient();

	color = color / (color + float3(1.0)); // HDR tonemapping
	color = pow(color, float3(1.0 / 2.2)); // Gamma correction
	
	return float4(color, 1.0);
	//return float4(material.Normal.xyz, 1.0);
	//return float4(float3(0.5) + 0.5 * material.Normal.xyz, 1.0);
	//return float4(float3(0.5) + 0.5 * fragPosition, 1.0);
}

