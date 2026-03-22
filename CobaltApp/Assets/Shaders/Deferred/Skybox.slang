import Common.Common;

struct Uniforms
{
	float4x4 ViewProjection;
	ConstBufferPointer<MeshVertex> Vertices;
};

struct VSOutput
{
	float4 Position: SV_Position;
	float3 UVW: VSOutput;
};

[shader("vertex")]
VSOutput VSMain(uniform ConstantBuffer<Uniforms> uniforms, uint vertexID: SV_VertexID)
{
	MeshVertex vertex = uniforms.Vertices[vertexID];

	VSOutput output;
	output.Position = mul(uniforms.ViewProjection, vertex.Position);
	output.UVW = vertex.Position.xyz;
	
	return output;
}

[shader("fragment")]
float4 FSMain(float3 uvw: VSOutput, uniform SamplerCube skybox)
{
	return skybox.Sample(uvw);	
}