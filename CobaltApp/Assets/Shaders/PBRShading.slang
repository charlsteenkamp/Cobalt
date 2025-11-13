module PBRShading;

// Trowbridge-Reitz GGX normal distribution function
//   n - normal
//   h - halfway vector between light dir and normal
public float DistributionGGX(in float3 n, in float3 h, in float roughness)
{
	float a      = roughness * roughness;
	float a2     = a * a;
	float nDotH  = saturate(dot(n, h));
	float nDotH2 = nDotH * nDotH;

	float num    = a2;
	float denom  = nDotH2 * (a2 - 1.0) + 1.0;
	denom        = float.getPi() * denom * denom;

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
public float GeometrySmith(in float3 n, in float3 v, in float3 l, in float roughness)
{
	float nDotV = saturate(dot(n, v));
	float nDotL = saturate(dot(n, l));
	float ggx1  = GeometrySchlickGGX(nDotV, roughness);
	float ggx2  = GeometrySchlickGGX(nDotL, roughness);

	return ggx1 * ggx2;
}

// hDotV - dot product between halfway vector and view vector
// f0 - base reflectivity
public float3 FresnelSchlick(in float hDotV, in float3 f0)
{
	return f0 + (float3(1.0) - f0) * pow(saturate(1.0 - hDotV), 5.0);
}
