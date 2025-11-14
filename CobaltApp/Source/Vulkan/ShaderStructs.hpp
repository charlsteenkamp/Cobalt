#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#define CO_MAX_POINT_LIGHT_COUNT 16

namespace Cobalt
{

	struct CameraData
	{
		glm::mat4 ViewProjectionMatrix;
		alignas(16) glm::vec3 CameraTranslation;
	};

	struct DirectionalLightData
	{
		alignas(16) glm::vec3 Direction;

		alignas(16) glm::vec3 Ambient;
		alignas(16) glm::vec3 Diffuse;
		alignas(16) glm::vec3 Specular;
	};

	struct PointLightData
	{
		alignas(16) glm::vec3 Position;

		alignas(16) glm::vec3 Ambient;
		alignas(16) glm::vec3 Diffuse;
		alignas(16) glm::vec3 Specular;

		float Constant;
		float Linear;
		float Quadratic;

		float __padding0;
	};

	struct SceneData
	{
		CameraData Camera;
		DirectionalLightData DirectionalLight;
		PointLightData PointLights[CO_MAX_POINT_LIGHT_COUNT];
		uint32_t PointLightCount;
	};

#define CO_INVALID_MATERIAL_HANDLE -1

	using TextureHandle = uint32_t;
	using MaterialHandle = uint32_t;

#define CO_DEFAULT_TEXTURE_HANDLE 0

	struct MaterialData
	{
		TextureHandle BaseColorMapHandle                  = CO_DEFAULT_TEXTURE_HANDLE; // albedo for non-metallic materials, base color otherwise
		TextureHandle NormalMapHandle                     = CO_DEFAULT_TEXTURE_HANDLE; // defined in tangent space
		TextureHandle OcclusionRoughnessMetallicMapHandle = CO_DEFAULT_TEXTURE_HANDLE; // R - occlusion, G - roughness, B - metallic
		TextureHandle EmissiveMapHandle                   = CO_DEFAULT_TEXTURE_HANDLE; // RGB

		glm::vec4 BaseColorFactor = glm::vec4(1.0f);

		float NormalScale = 1.0f;
		float OcclusionStrength = 1.0f;
		float RoughnessFactor = 1.0f;
		float MetallicFactor = 0.0f;

		glm::vec3 EmissiveFactor = glm::vec3(0.0f);
		float __padding1;
	};

	struct ObjectData
	{
		glm::mat4 Transform = glm::mat4(1.0f);
		glm::mat4 NormalMatrix = glm::mat4(1.0f);
		VkDeviceAddress VertexBufferRef = 0;
		MaterialHandle MaterialHandle;
		float __padding[1]{};
	};

	static_assert(sizeof(ObjectData) == 144);
}

