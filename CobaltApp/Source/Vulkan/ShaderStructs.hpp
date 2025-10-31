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

	using TextureHandle = uint32_t;
	using MaterialHandle = uint32_t;

	struct MaterialData
	{
		TextureHandle AlbedoMapHandle;
		TextureHandle MetallicMapHandle;
		TextureHandle RoughnessMapHandle;
	};

	struct ObjectData
	{
		glm::mat4 Transform = glm::mat4(1.0f);
		glm::mat4 NormalMatrix = glm::mat4(1.0f);
		VkDeviceAddress VertexBufferRef = 0;
		MaterialHandle MaterialHandle = -1;
	};

	static_assert(sizeof(ObjectData) == 144);
}

namespace std
{

	template<>
	struct hash<Cobalt::MaterialData>
	{
		size_t operator()(const Cobalt::MaterialData& materialData)
		{
			size_t hash1 = hash<uint32_t>{}(materialData.DiffuseMapHandle);
			size_t hash2 = hash<uint32_t>{}(materialData.SpecularMapHandle);
			size_t hash3 = hash<uint32_t>{}(materialData.Shininess);

			return hash1 ^ (hash2 << 1) ^ (hash3 << 2);
		}
	};

}
