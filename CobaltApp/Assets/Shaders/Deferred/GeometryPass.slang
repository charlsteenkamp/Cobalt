module GeometryPass;

import Common.Common;
import Common.Material;

struct CoarseVertex
{
	float3 Normal;
	float3 Tangent;
	float2 UV;
	float3 FragPosition;
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

	float4 positionLocal = float4(vertex.Position.xyz, 1.0);
	float4 positionWorld = mul(object.Transform, positionLocal);

	VSOutput output;
	output.Position = mul(scene.Camera.ViewProjection, positionWorld);

	output.CoarseVertex.Normal         = mul(float3x3(object.NormalMatrix), vertex.Normal.xyz);
	output.CoarseVertex.Tangent        = vertex.Tangent.xyz;
	output.CoarseVertex.FragPosition   = positionWorld.xyz;
	output.CoarseVertex.UV             = vertex.UV.xy;
	output.CoarseVertex.MaterialHandle = object.MaterialHandle;

	return output;
}

struct FSOutput
{
	float4 Position:                   SV_Target0;
	float4 BaseColor:                  SV_Target1;
	float4 Normal:                     SV_Target2;
	float4 OcclusionRoughnessMetallic: SV_Target3;
	float4 Emissive:                   SV_Target4;
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
	output.Position = float4(coarseVertex.FragPosition, 1.0);
	output.BaseColor = material.BaseColor;
	output.Normal = float4(normalize(coarseVertex.Normal), 1.0);
	//output.Normal = float4(coarseVertex.UV, 0.0, 1.0);
	output.OcclusionRoughnessMetallic = float4(material.Occlusion, material.Roughness, material.Metallic, 1.0);
	output.Emissive = float4(material.Emissive, 1.0);

	return output;
}

