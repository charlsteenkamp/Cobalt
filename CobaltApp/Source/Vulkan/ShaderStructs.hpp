#pragma once
#include "Mesh.hpp"
#include "Material.hpp"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>

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
		alignas(16) glm::vec3 Intensity;
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
		//PointLightData PointLights[CO_MAX_POINT_LIGHT_COUNT];
		//uint32_t PointLightCount;
	};


	struct ObjectData
	{
		ObjectData(const glm::mat4& transform, const Mesh* mesh)
			: Transform(transform), NormalMatrix(glm::transpose(glm::inverse(transform))),
			  VertexBufferRef(mesh->GetVertexBufferReference()), MaterialHandle(mesh->GetMaterial()->GetMaterialHandle())
		{
		}

		glm::mat4 Transform = glm::mat4(1.0f);
		glm::mat4 NormalMatrix = glm::mat4(1.0f);
		VkDeviceAddress VertexBufferRef = 0;
		MaterialHandle MaterialHandle;
		float __padding[1]{};
	};

	static_assert(sizeof(ObjectData) == 144);
}

