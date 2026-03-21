#pragma once
#include "Module.hpp"
#include "Vulkan/Renderer.hpp"
#include "CameraController.hpp"
#include "Model.hpp"

namespace Cobalt
{

	class SandboxModule : public Module
	{
	public:
		SandboxModule();
		~SandboxModule();

	public:
		void OnInit() override;
		void OnShutdown() override;
		void OnUpdate(float deltaTime) override;
		void OnRender() override;
		void OnUIRender() override;
		void OnMouseMove(float x, float y) override;
		void OnResize(uint32_t width, uint32_t height) override;

	private:
		CameraController mCameraController;
		GPUScene mScene;

		Mesh* mCubeMesh = nullptr;
		Mesh* mSphereMesh = nullptr;

		constexpr static uint32_t sSphereGridSize = 10;

		Material* mSphereMaterials[sSphereGridSize][sSphereGridSize];

		float mLastDeltaTime = 0.0f;
		float mLastFPS = 0.0f;
		
		glm::vec4 mSphereBaseColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		float mSphereRoughnessFactor = 1.0f;
		float mSphereMetallicFactor  = 0.0f;
		bool mSphereMaterialChanged = false;
	};

}
