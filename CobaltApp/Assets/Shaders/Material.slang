module Material;

public struct UnpackedMaterial
{
	public float3 Albedo;
	public float3 Metallic;
	public float3 Roughness;
};

public struct Material
{
	public uint AlbedoMapHandle;
	public uint MetallicMapHandle;
	public uint RoughnessMapHandle;

	public UnpackedMaterial Unpack(in Sampler2D albedoTexture, in Sampler2D metallicTexture, in Sampler2D roughnessTexture, in float2 uv)
	{
		UnpackedMaterial unpackedMaterial;
		unpackedMaterial.Albedo  = albedoTexture.Sample(uv).xyz;
		unpackedMaterial.Metallic = metallicTexture.Sample(uv).xyz;
		unpackedMaterial.Roughness = roughnessTexture.Sample(uv).xyz;

		return unpackedMaterial;
	}
};