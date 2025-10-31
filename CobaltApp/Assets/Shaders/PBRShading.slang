module PBRShading;

// Trowbridge-Reitz GGX normal distribution function
//   n - normal
//   h - halfway vector between light dir and normal
public float DistributionGGX(vec3 n, vec3 h, float roughness)
{
	float a      = roughness * roughness;
	float a2     = a * a;
	float nDotH  = saturate(dot(n, h));
	float nDotH2 = nDotH * nDotH;

	float num    = a2;
	float denom  = nDotH2 * (a2 - 1.0) + 1.0;
	denom        = M_PI * denom * denom;

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
public float GeometrySmith(vec3 n, vec3 v, vec3 l, float roughness)
{
	float nDotV = saturate(dot(n, v));
	float nDotL = saturate(dot(n, l));
	float ggx1  = GeometrySchlickGGX(nDotV, roughness);
	float ggx2  = GeometrySchlickGGX(nDotL, roughness);

	return ggx1 * ggx2;
}

// cosTheta - dot product between halfway vector and view vector
// f0 - base reflectivity
public float FresnelSchlick(float cosTheta, vec3 f0)
{
	return f0 + (1.0 - f0) * pow(saturate(1.0 - cosTheta), 5.0);
}
