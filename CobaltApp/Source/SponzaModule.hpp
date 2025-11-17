#pragma once
#include "Module.hpp"
#include "Vulkan/Renderer.hpp"
#include "CameraController.hpp"
#include "Model.hpp"

namespace Cobalt
{

	class SponzaModule : public Module
	{
	public:
		SponzaModule();
		~SponzaModule();

	public:
		virtual void OnInit() override;
		virtual void OnShutdown() override;

		virtual void OnUpdate(float deltaTime) override;
		virtual void OnRender() override;

		virtual void OnUIRender() override;

		virtual void OnMouseMove(float x, float y) override;

	private:
		void RenderPointLight(const char* name, PointLightData& pointLight);
		void RenderUITransform(const char* name, Transform& transform);
		void RenderUIMaterial(const char* name, MaterialHandle material);

	private:
		CameraController mCameraController;
		SceneData mScene;

		std::unique_ptr<Model> mSponzaModel;

		bool mCaptureMouse = true;
		float mDeltaTime = 0.0f;

		int32_t mRenderMeshCount = 0;
	};

}