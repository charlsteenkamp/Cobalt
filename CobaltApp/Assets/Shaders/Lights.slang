module Lights;

import Material;
import PBRShading;

public float3 CalculateAmbient(in UnpackedMaterial material)
{
	return float3(0.03) * material.Albedo;
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

	public float3 CalculateRadiance(in UnpackedMaterial material, in float3 normal, in float3 viewDir)
	{
		float3 f0 = float3(0.04);
		f0 = mix(f0, material.Albedo, material.Metallic);

		float3 lightDir = normalize(-Direction);
		float3 halfway = normalize(lightDir + viewDir);

		float nDotL = saturate(dot(normal, lightDir));
		float nDotV = saturate(dot(normal, viewDir));
		float hDotV = saturate(dot(halfway, viewDir));
		float hDotL = saturate(dot(halfway, lightDir));

		float NDF = DistributionGGX(normal, halfway, material.Roughness);
		float G   = GeometrySmith(normal, viewDir, lightDir, material.Roughness);
		float F   = FresnelSchlick(hDotV, f0);

		float3 kS = F;
		float3 kD = float3(1.0) - kS;
		kD *= 1.0 - material.Metallic;

		float3 diffuse = kD * material.Albedo / M_PI;
		float3 specular = NDF * G * F / (4.0 * nDotV * nDotL + 0.0001);

		return (diffuse + specular) * nDotL;

		/*
		float diff = max(0.0, dot(lightDir, normal));
		float spec = pow(max(0.0, dot(viewDir, reflectDir)), material.Shininess);

		float3 ambient  = Ambient * material.Diffuse;
		float3 diffuse  = Diffuse * diff * material.Diffuse;
		float3 specular = Specular * spec * material.Specular;

		return ambient + diffuse + specular;
		*/
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