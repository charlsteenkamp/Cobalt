import Material;
import Lights;

struct Camera
{
	float4x4 ViewProjection;
	float3 Translation;
};

#define MAX_POINT_LIGHT_COUNT 16

struct Scene
{
	Camera Camera;

	DirectionalLight DirectionalLight;

	PointLight PointLights[MAX_POINT_LIGHT_COUNT];
	uint PointLightCount;
};

struct MeshVertex
{
	float3 Position;
	float  TexCoordU;

	float3 Normal;

	float  TexCoordV;

	uint   MaterialHandle;
};

struct Object
{
	float4x4 Transform;
	float4x4 NormalMatrix;

	ConstBufferPointer<MeshVertex> Vertices;
};

struct VSOutput
{
	float4 Position: SV_POSITION;
	
	float3 Normal;
	float2 UV;
	float3 FragPosition;
	nointerpolation uint MaterialHandle;
};

ConstantBuffer<Scene> gScene;

[shader("vertex")]
VSOutput VSMain(uniform StructuredBuffer<Object> objects, uint instanceID: SV_InstanceID, uint vertexID: SV_VertexID)
{
	Object object = objects[instanceID];
	MeshVertex vertex = object.Vertices[vertexID];

	float4 positionLocal = float4(vertex.Position, 1.0);
	float4 positionWorld = mul(object.Transform, positionLocal);

	VSOutput output;
	output.Position       = mul(gScene.Camera.ViewProjection, positionWorld);
	output.Normal         = mul(float3x3(object.NormalMatrix), vertex.Normal);
	output.UV             = float2(vertex.TexCoordU, vertex.TexCoordV);
	output.FragPosition   = positionWorld.xyz;
	output.MaterialHandle = vertex.MaterialHandle;

	return output;
}

[shader("fragment")]
float4 FSMain(VSOutput input, uniform StructuredBuffer<Material> materials, uniform Sampler2D textures[]) : SV_Target0
{
	Material material = materials[input.MaterialHandle];

	UnpackedMaterial unpackedMaterial = material.Unpack(
		textures[material.BaseColorMapHandle],
		textures[material.NormalMapHandle],
		textures[material.OcclusionRoughnessMetallicMapHandle],
		textures[material.EmissiveMap],
		input.UV
    );

	float3 luminance = 0.0;
	float3 normal   = normalize(input.Normal);
	float3 viewDir  = normalize(gScene.Camera.Translation - input.FragPosition);

	luminance += gScene.DirectionalLight.CalculateLuminance(unpackedMaterial, normal, viewDir);

	float3 ambient = CalculateAmbient(unpackedMaterial);
	float3 color = GammaCorrection(luminance + ambient);
	
	return float4(color, 1.0);
	//return float4(textures[0].Sample(input.UV).xyz, 1.0);
	//return float4(float(input.MaterialHandle), 1.0, 0.0, 1.0);
	//return float4(input.UV, 1.0, 1.0);
	//return float4(float(input.MaterialHandle), 1.0 - float(input.MaterialHandle), 0.0, 1.0);
}

