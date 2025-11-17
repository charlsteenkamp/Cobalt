#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace Cobalt
{

	class CameraController
	{
	public:
		CameraController() = default;
		CameraController(float viewportWidth, float viewportHeight);
		CameraController(float viewportWidth, float viewportHeight, float fov, float nearClip, float farClip);

	public:
		void OnUpdate(float deltaTime);
		void OnResize(float viewportWidth, float viewportHeight);
		void OnMouseMove(float x, float y);

	public:
		const glm::vec3& GetTranslation() const { return mTranslation; }
		const glm::mat4& GetViewProjectionMatrix() const { return mViewProjectionMatrix; }

		float GetAspectRatio() const { return mAspectRatio; }
		float GetFOV() const { return mFOV; }
		float GetNearClip() const { return mNearClip; }
		float GetFarClip() const { return mFarClip; }

	public:
		void SetAspectRatio(float aspectRatio) { mAspectRatio = aspectRatio; RecalculateProjectionMatrix(); }
		void SetFOV(float fov) { mFOV = fov; RecalculateProjectionMatrix(); }
		void SetNearClip(float nearClip) { mNearClip = nearClip; RecalculateProjectionMatrix(); }
		void SetFarClip(float farClip) { mFarClip = farClip; RecalculateProjectionMatrix(); }

	private:
		void RecalculateViewMatrix();
		void RecalculateProjectionMatrix();

	private:
		float mAspectRatio;
		float mFOV = 45.0f;
		float mNearClip = 0.1f;
		float mFarClip = 10000.0f;

		float mViewportWidth, mViewportHeight;

		float mCameraSpeed = 10.0f;
		float mMouseSensitivity = 0.1f;

		glm::vec3 mTranslation = glm::vec3(0.0f, 3.0f, 15.0f);
		glm::vec3 mForwardDir = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 mUpDir = glm::vec3(0.0f, 1.0f, 0.0f);

		glm::mat4 mViewMatrix, mProjectionMatrix;
		glm::mat4 mViewProjectionMatrix;

		float mPitch = 0.0f, mYaw = 0.0f;

		float mLastMouseX = 0.0f;
		float mLastMouseY = 0.0f;
	};

}