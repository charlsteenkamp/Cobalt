module BRDF;

public interface IBRDF
{
	float3 Evaluate(float3 v, float3 l, float3 n);
};

public struct CookTorranceBRDF : IBRDF
{
	public __init(in float3 baseColor, in float roughness, in float metallic)
	{
		NormalisedBaseColor = baseColor / float.getPi(); // for diffuse normalisation
		Roughness = roughness;
		Metallic = metallic;
		
		F0 = float3(0.04);
		F0 = lerp(F0, baseColor, Metallic);
	}

	float3 NormalisedBaseColor;
	float Roughness;
	float Metallic;

	float3 F0;

	public float3 Evaluate(float3 v, float3 l, float3 n)
	{
		float3 h = normalize(v + l);

		float nDotL = saturate(dot(n, l));
		float nDotV = saturate(dot(n, v));
		float nDotH = saturate(dot(n, h));
		float hDotV = saturate(dot(h, v));

		float  NDF = DistributionGGX(nDotH, Roughness);
		float  G   = GeometrySmith(nDotV, nDotL, Roughness);
		float3 F   = FresnelSchlick(hDotV, F0);

		float3 kS = F;
		float3 kD = float3(1.0) - kS;
		kD *= 1.0 - Metallic;

		float3 diffuse = kD * NormalisedBaseColor;
		float3 specular = NDF * G * F / (4.0 * nDotV * nDotL + 1e-4);

		return (diffuse + specular) * nDotL;
	}

};

// Trowbridge-Reitz GGX normal distribution function
internal float DistributionGGX(in float nDotH, in float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;

	float num = a2;
	float denom = nDotH * nDotH * (a2 - 1.0) + 1.0;
	denom = denom * denom * float.getPi();

	return num / denom;
}

internal float GeometrySchlickGGX(float nDotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;

	float num   = nDotV;
	float denom = nDotV * (1.0 - k) + k;

	return num / denom;
}

internal float GeometrySmith(in float nDotV, in float nDotL, in float roughness)
{
	float ggx1  = GeometrySchlickGGX(nDotV, roughness);
	float ggx2  = GeometrySchlickGGX(nDotL, roughness);

	return ggx1 * ggx2;
}

internal float3 FresnelSchlick(in float hDotV, in float3 f0)
{
	float f = pow(saturate(1.0 - hDotV), 5.0);
	return f + f0 * (1.0 - f);
}

