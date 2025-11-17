import Common.Common;
import Common.Material;
import Common.BRDF;
import Common.Lights;

struct CoarseVertex
{
	float3 Normal;
	float2 UV;
	float3 FragPosition;
	nointerpolation uint MaterialHandle;
};

struct VSOutput
{
	float4       Position:     SV_Position;
	CoarseVertex CoarseVertex: CoarseVertex;
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
	output.Position = mul(gScene.Camera.ViewProjection, positionWorld);

	output.CoarseVertex.Normal         = mul(float3x3(object.NormalMatrix), vertex.Normal);
	output.CoarseVertex.UV             = float2(vertex.TexCoordU, vertex.TexCoordV);
	output.CoarseVertex.FragPosition   = positionWorld.xyz;
	output.CoarseVertex.MaterialHandle = object.MaterialHandle;

	return output;
}

[shader("fragment")]
float4 FSMain(CoarseVertex coarseVertex: CoarseVertex, uniform StructuredBuffer<PackedMaterial> materials, uniform Sampler2D textures[]) : SV_Target
{
	PackedMaterial packedMaterial = materials[coarseVertex.MaterialHandle];

	Material material = packedMaterial.Unpack(
		textures[packedMaterial.BaseColorMapHandle],
		textures[packedMaterial.NormalMapHandle],
		textures[packedMaterial.OcclusionRoughnessMetallicMapHandle],
		textures[packedMaterial.EmissiveMap],
		coarseVertex.UV
    );

	CookTorranceBRDF brdf = material.PrepareBRDF();

	float3 luminance = 0.0;
	float3 normal   = normalize(coarseVertex.Normal);
	float3 viewDir  = normalize(gScene.Camera.Translation - coarseVertex.FragPosition);

	luminance += gScene.DirectionalLight.Illuminate(brdf, normal, viewDir);

	float3 color = luminance + material.CalculateAmbient();

	color = color / (color + float3(1.0)); // HDR tonemapping
	color = pow(color, float3(1.0 / 2.2)); // Gamma correction
	
	return float4(color, 1.0);
}

