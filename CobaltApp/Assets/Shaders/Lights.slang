module Lights;

import Material;
import PBRShading;

public float3 CalculateAmbient(in UnpackedMaterial material)
{
	return float3(0.03) * material.BaseColor.xyz;
}

public float3 GammaCorrection(in float3 color)
{
	float3 result = color / (color + 1.0);
	result = pow(color, float3(1.0 / 2.2));
	return result;
}

public struct DirectionalLight
{
	public float3 Direction;

	public float3 Ambient;
	public float3 Diffuse;
	public float3 Specular;

	public float3 CalculateLuminance(in UnpackedMaterial material, in float3 normal, in float3 viewDir)
	{
		float3 f0 = float3(0.04);
		f0 = lerp(f0, material.BaseColor.xyz, material.Metallic);

		float3 lightDir = normalize(-Direction);
		float3 halfway = normalize(lightDir + viewDir);

		float nDotL = saturate(dot(normal, lightDir));
		float nDotV = saturate(dot(normal, viewDir));
		float hDotV = saturate(dot(halfway, viewDir));
		float hDotL = saturate(dot(halfway, lightDir));

		float  NDF = DistributionGGX(normal, halfway, material.Roughness);
		float  G   = GeometrySmith(normal, viewDir, lightDir, material.Roughness);
		float3 F   = FresnelSchlick(hDotV, f0);

		float3 kS = F;
		float3 kD = float3(1.0) - kS;
		kD *= 1.0 - material.Metallic;

		float3 diffuse = kD * material.BaseColor.xyz / float.getPi();
		float3 specular = NDF * G * F / (4.0 * nDotV * nDotL + 0.0001);

		return (diffuse + specular) * nDotL;
	}
};

public struct PointLight
{
	public float3 Position;

	public float3 Ambient;
	public float3 Diffuse;
	public float3 Specular;

	private float Constant;
	private float Linear;
	private float Quadratic;

	public float3 CalculateRadiance()
	{

	}
};