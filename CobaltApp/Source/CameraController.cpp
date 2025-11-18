#include "copch.hpp"
#include "CameraController.hpp"
#include "Application.hpp"

#include <algorithm>

namespace Cobalt
{

	CameraController::CameraController(float viewportWidth, float viewportHeight)
		: mViewportWidth(viewportWidth), mViewportHeight(viewportHeight), mAspectRatio(viewportWidth / viewportHeight)
	{
		CO_PROFILE_FN();

		RecalculateViewMatrix();
		RecalculateProjectionMatrix();
	}

	CameraController::CameraController(float viewportWidth, float viewportHeight, float fov, float nearClip, float farClip)
		: mViewportWidth(viewportWidth), mViewportHeight(viewportHeight), mAspectRatio(viewportWidth / viewportHeight),
		  mFOV(fov), mNearClip(nearClip), mFarClip(farClip)
	{
		CO_PROFILE_FN();

		RecalculateViewMatrix();
		RecalculateProjectionMatrix();
	}

	void CameraController::OnUpdate(float deltaTime)
	{
		CO_PROFILE_FN();

		GLFWwindow* window = Application::Get()->GetWindow().GetWindow();

		glm::vec3 velocityDir = glm::vec3(0.0f);

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			velocityDir = mForwardDir;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			velocityDir = -mForwardDir;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			velocityDir = glm::normalize(glm::cross(mForwardDir, mUpDir));
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			velocityDir = -glm::normalize(glm::cross(mForwardDir, mUpDir));
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			velocityDir = mUpDir;
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			velocityDir = -mUpDir;

		if (velocityDir.x != 0.0f || velocityDir.y != 0.0f || velocityDir.z != 0.0f)
		{
			mTranslation += mCameraSpeed * deltaTime * glm::normalize(velocityDir);

			RecalculateViewMatrix();
		}
	}

	void CameraController::OnResize(float viewportWidth, float viewportHeight)
	{
		CO_PROFILE_FN();

		mViewportWidth = viewportWidth;
		mViewportHeight = viewportHeight;
		mAspectRatio = mViewportWidth / mViewportHeight;

		RecalculateProjectionMatrix();
	}

	void CameraController::OnMouseMove(float x, float y)
	{
		CO_PROFILE_FN();

		static bool firstTime = true;

		if (firstTime)
		{
			mLastMouseX = x;
			mLastMouseY = y;
			firstTime = false;
		}

		mYaw   += mMouseSensitivity * (x - mLastMouseX);
		mPitch -= mMouseSensitivity * (y - mLastMouseY);

		mPitch = std::clamp(mPitch, -89.0f, 89.0f);

		mLastMouseX = x;
		mLastMouseY = y;

		mForwardDir.x = glm::cos(glm::radians(mYaw)) * glm::cos(glm::radians(mPitch));
		mForwardDir.y = glm::sin(glm::radians(mPitch));
		mForwardDir.z = glm::sin(glm::radians(mYaw)) * glm::cos(glm::radians(mPitch));

		RecalculateViewMatrix();
	}

	void CameraController::RecalculateViewMatrix()
	{
		CO_PROFILE_FN();

		mViewMatrix = glm::lookAt(mTranslation, mTranslation + mForwardDir, mUpDir);
		mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
	}

	void CameraController::RecalculateProjectionMatrix()
	{
		CO_PROFILE_FN();

		mProjectionMatrix = glm::perspective(mFOV, mAspectRatio, mNearClip, mFarClip);
		mProjectionMatrix[1][1] *= -1.0f; // Flip y

		mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
	}

}