module PBRShading;

import Material;

// Trowbridge-Reitz GGX normal distribution function
//   n - normal
//   h - halfway vector between light dir and normal
public float DistributionGGX(in float nDotH, in float roughness)
{
	/*float a = nDotH * roughness;
	float k = roughness / (1.0 - nDotH * nDotH + a * a);
	return k * k * (1.0 / float.getPi());*/

	float a = roughness * roughness;
	float a2 = a * a;

	float num = a2;
	float denom = nDotH * nDotH * (a2 - 1.0) + 1.0;
	denom = denom * denom * float.getPi();

	return num / denom;
}

// k - remapping of surface roughness
public float GeometrySchlickGGX(float nDotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;

	float num   = nDotV;
	float denom = nDotV * (1.0 - k) + k;

	return num / denom;
}

// Geometry function - Schlick-GGX w/ Smith's method
// n - normal
// v - view dir
// l - light dir
public float GeometrySmith(in float nDotV, in float nDotL, in float roughness)
{
	/*float a2 = roughness;
	float GGXV = nDotL * sqrt(nDotV * nDotV * (1.0 - a2) + a2);
	float GGXL = nDotV * sqrt(nDotL * nDotL * (1.0 - a2) + a2);
	return 0.5 / (GGXV + GGXL + 0.0001);*/

	float ggx1  = GeometrySchlickGGX(nDotV, roughness);
	float ggx2  = GeometrySchlickGGX(nDotL, roughness);

	return ggx1 * ggx2;
}

// hDotV - dot product between halfway vector and view vector
// f0 - base reflectivity
public float3 FresnelSchlick(in float hDotV, in float3 f0)
{
	//return f0 + (float3(1.0) - f0) * pow(saturate(1.0 - hDotV), 5.0);
	float f = pow(saturate(1.0 - hDotV), 5.0);
	return f + f0 * (1.0 - f);
}

public float3 BRDF(in float3 n, in float3 v, in float3 l, in UnpackedMaterial material)
{
	float3 h = normalize(v + l);

	float nDotL = saturate(dot(n, l));
	float nDotV = saturate(dot(n, v));
	float nDotH = saturate(dot(n, h));
	float hDotV = saturate(dot(h, v));

	float3 f0 = float3(0.04);
	f0 = lerp(f0, material.BaseColor.xyz, material.Metallic);

	float  NDF = DistributionGGX(nDotH, material.Roughness);
	float  G   = GeometrySmith(nDotV, nDotL, material.Roughness);
	float3 F   = FresnelSchlick(hDotV, f0);

	float3 kS = F;
	float3 kD = float3(1.0) - kS;
	kD *= 1.0 - material.Metallic;

	float3 diffuse = kD * material.BaseColor.xyz / float.getPi();
	float3 specular = NDF * G * F / (4.0 * nDotV * nDotL + 0.0001);

	return diffuse + specular;
}
