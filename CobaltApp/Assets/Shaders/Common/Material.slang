module Material;

import BRDF;

public struct Material
{
	public float4 BaseColor;
	public float3 Normal;
	public float  Occlusion;
	public float  Roughness;
	public float  Metallic;
	public float3 Emissive;

	public float3 CalculateAmbient()
	{
		return float3(0.03) * BaseColor.xyz;
	}

	public CookTorranceBRDF PrepareBRDF()
	{
		CookTorranceBRDF brdf = CookTorranceBRDF(BaseColor.xyz, Roughness, Metallic);
		return brdf;
	}
};

public struct PackedMaterial
{
	public uint   BaseColorMapHandle;
	public uint   NormalMapHandle;
	public uint   OcclusionRoughnessMetallicMapHandle;
	public uint   EmissiveMap;

	public float4 BaseColorFactor;

	public float  NormalScale;
	public float  OcclusionStrength;
	public float  RoughnessFactor;
	public float  MetallicFactor;

	public float3 EmissiveFactor;

	public Material Unpack(in Sampler2D baseColorMap, in Sampler2D normalMap, in Sampler2D occlusionRoughnessMetallicMap, in Sampler2D emissiveMap, in float2 uv)
	{
		Material unpackedMaterial;

		unpackedMaterial.BaseColor = BaseColorFactor   * baseColorMap.Sample(uv);
		unpackedMaterial.Normal    = NormalScale       * normalMap.Sample(uv).xyz;
		unpackedMaterial.Occlusion = OcclusionStrength * occlusionRoughnessMetallicMap.Sample(uv).x;
		unpackedMaterial.Roughness = RoughnessFactor   * occlusionRoughnessMetallicMap.Sample(uv).r;
		unpackedMaterial.Metallic  = MetallicFactor    * occlusionRoughnessMetallicMap.Sample(uv).b;
		unpackedMaterial.Emissive  = EmissiveFactor    * emissiveMap.Sample(uv).xyz;

		return unpackedMaterial;
	}
};