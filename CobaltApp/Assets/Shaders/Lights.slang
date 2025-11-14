module Lights;

import Material;
import PBRShading;

public struct DirectionalLight
{
	public float3 Direction;

	public float3 Ambient;
	public float3 Diffuse;
	public float3 Specular;

	public float3 CalculateLuminance(in UnpackedMaterial material, in float3 normal, in float3 viewDir)
	{
		float3 lightDir = -Direction;
		float nDotL = saturate(dot(normal, lightDir));

		return BRDF(normal, viewDir, lightDir, material) * nDotL;
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