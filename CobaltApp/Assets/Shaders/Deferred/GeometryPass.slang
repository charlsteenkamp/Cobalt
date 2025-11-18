module GeometryPass;

import Common.Common;
import Common.Material;

struct CoarseVertex
{
	float3 Normal;
	float2 UV;
	nointerpolation uint MaterialHandle;
};

struct VSOutput
{
	float4       Position:     SV_Position;
	CoarseVertex CoarseVertex: CoarseVertex;
};

[shader("vertex")]
VSOutput VSMain(ConstantBuffer<Scene> scene, uniform StructuredBuffer<Object> objects, uint instanceID: SV_InstanceID, uint vertexID: SV_VertexID)
{
	Object     object = objects[instanceID];
	MeshVertex vertex = object.Vertices[vertexID];

	float4 positionLocal = float4(vertex.Position, 1.0);
	float4 positionWorld = mul(object.Transform, positionLocal);

	VSOutput output;
	output.Position = mul(scene.Camera.ViewProjection, positionWorld);

	output.CoarseVertex.Normal         = mul(float3x3(object.NormalMatrix), vertex.Normal);
	output.CoarseVertex.UV             = float2(vertex.TexCoordU, vertex.TexCoordV);
	output.CoarseVertex.MaterialHandle = object.MaterialHandle;

	return output;
}

struct FSOutput
{
	float4 BaseColor:                  SV_Target0;
	float4 Normal:                     SV_Target1;
	float4 OcclusionRoughnessMetallic: SV_Target2;
	float4 Emissive:                   SV_Target3;
};

[shader("fragment")]
FSOutput FSMain(CoarseVertex coarseVertex: CoarseVertex, uniform StructuredBuffer<PackedMaterial> materials, uniform Sampler2D textures[])
{
	PackedMaterial packedMaterial = materials[coarseVertex.MaterialHandle];

	Material material = packedMaterial.Unpack(
		textures[packedMaterial.BaseColorMapHandle],
		textures[packedMaterial.NormalMapHandle],
		textures[packedMaterial.OcclusionRoughnessMetallicMapHandle],
		textures[packedMaterial.EmissiveMap],
		coarseVertex.UV
    );

	FSOutput output;
	output.BaseColor = material.BaseColor;
	//output.Normal = float4(material.Normal, 1.0);
	output.Normal = float4(coarseVertex.Normal, 1.0);
	output.OcclusionRoughnessMetallic = float4(material.Occlusion, material.Roughness, material.Metallic, 1.0);
	output.Emissive = float4(material.Emissive, 1.0);

	return output;
}

