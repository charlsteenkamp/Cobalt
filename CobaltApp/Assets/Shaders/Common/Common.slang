module Common;

import Lights;

public struct Camera
{
	public float4x4 ViewProjection;
	public float3 Translation;
};

public struct Scene
{
	public Camera Camera;
	public DirectionalLight DirectionalLight;
};

public struct MeshVertex
{
	public float4 Position;
	public float4 Normal;
	public float4 Tangent;
	public float4 UV;
};

public struct Object
{
	public float4x4 Transform;
	public float4x4 NormalMatrix;

	public ConstBufferPointer<MeshVertex> Vertices;

	public uint MaterialHandle;
};
