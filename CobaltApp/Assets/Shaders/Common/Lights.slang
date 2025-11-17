module Lights;

import Material;
import BRDF;

public interface ILightEnv
{
	float3 Illuminate<BRDF: IBRDF>(in BRDF brdf, in float3 normal, in float3 viewDir);
};

public struct DirectionalLight : ILightEnv
{
	float3 Direction;
	float3 Intensity;

	public float3 Illuminate<BRDF: IBRDF>(in BRDF brdf, in float3 normal, in float3 viewDir)
	{
		return Intensity * brdf.Evaluate(viewDir, -Direction, normal);		
	}

	/*public float3 CalculateLuminance(in UnpackedMaterial material, in float3 normal, in float3 viewDir)
	{
		float3 lightDir = -Direction;
		float nDotL = saturate(dot(normal, lightDir));

		return BRDF(normal, viewDir, lightDir, material) * nDotL;
	}*/
};

/*public struct PointLight
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
};*/