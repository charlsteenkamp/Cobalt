#pragma once
#include "Module.hpp"
#include "Vulkan/Renderer.hpp"
#include "Camera.hpp"
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

	private:
		CameraController mCameraController;
		SceneData mScene;

		Mesh* mCubeMesh = nullptr;
		Mesh* mSphereMesh = nullptr;

		Material* mPBRMaterial = nullptr;

		float mLastDeltaTime = 0.0f;
		float mLastFPS = 0.0f;
		
		glm::vec4 mSphereBaseColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		float mSphereRoughnessFactor = 1.0f;
		float mSphereMetallicFactor  = 0.0f;
		bool mSphereMaterialChanged = false;

		uint32_t mSphereGridSize = 10;


	};

}
