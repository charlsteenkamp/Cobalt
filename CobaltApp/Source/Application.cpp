#include "copch.hpp"
#include "Application.hpp"
#include "AssetManager.hpp"
#include "Vulkan/Renderer.hpp"
#include "Vulkan/ImGuiBackend.hpp"
#include "Vulkan/ShaderCompiler.hpp"

namespace Cobalt
{

	Application::Application(ApplicationInfo&& info)
		: mInfo(std::move(info))
	{
		CO_PROFILE_FN();

		if (sInstance)
			return;

		sInstance = this;

		if (mInfo.EnableOptickCapture)
		{
			CO_PROFILE_START_CAPTURE();
		}

		mWindow = std::make_unique<Window>();
		mWindow->OnWindowClose([this]() { this->OnWindowClose(); });
		mWindow->OnWindowResize([this](uint32_t width, uint32_t height) { this->OnWindowResize(width, height); });
		mWindow->OnMouseMove([this](float x, float y) { this->OnMouseMove(x, y); });

		mGraphicsContext = std::make_unique<GraphicsContext>(*mWindow);
	}

	Application::~Application()
	{
		CO_PROFILE_FN();
	}

	void Application::Init()
	{
		CO_PROFILE_FN();

		mWindow->Create();
		mGraphicsContext->Init();

		ShaderCompiler::Init();
		Renderer::Init();
		AssetManager::Init();

		if (mInfo.EnableImGui)
			ImGuiBackend::Init();

		for (Module* module : mModules)
			module->OnInit();
	}

	void Application::Run()
	{
		CO_PROFILE_FN();

		float lastFrameTime = 0.0f;

		while (mRunning)
		{
			CO_PROFILE_FRAME("Main Loop");

			float currentTime = glfwGetTime();
			float deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			{
				CO_PROFILE_CATEGORY("Input", Optick::Category::Input);
				mWindow->Update();
			}

			{
				CO_PROFILE_CATEGORY("Update", Optick::Category::GameLogic);

				for (Module* module : mModules)
					module->OnUpdate(deltaTime);
			}

			{
				CO_PROFILE_CATEGORY("Swapchain Recreation", Optick::Category::Wait);

				if (mGraphicsContext->ShouldRecreateSwapchain())
				{
					mGraphicsContext->OnResize();
					Renderer::OnResize();

					if (mInfo.EnableImGui)
						ImGuiBackend::OnResize();
				}
			}

			{
				CO_PROFILE_CATEGORY("ImGui", Optick::Category::UI);

				if (mInfo.EnableImGui)
				{
					ImGuiBackend::BeginFrame();

					for (Module* module : mModules)
						module->OnUIRender();

					ImGuiBackend::EndFrame();
				}
			}

			{
				CO_PROFILE_CATEGORY("Rendering", Optick::Category::Rendering);

				mGraphicsContext->RenderFrame(mModules);
				mGraphicsContext->PresentFrame();
			}
		}

		if (mInfo.EnableOptickCapture)
		{
			CO_PROFILE_STOP_CAPTURE();
			CO_PROFILE_SAVE_CAPTURE("capture_2025-11-13.opt");
		}
	}

	void Application::Shutdown()
	{
		CO_PROFILE_FN();

		for (Module* module : mModules)
		{
			module->OnShutdown();
			delete module;
			module = nullptr;
		}

		mModules.clear();

		vkDeviceWaitIdle(GraphicsContext::Get().GetDevice());

		if (mInfo.EnableImGui)
			ImGuiBackend::Shutdown();

		AssetManager::Shutdown();
		Renderer::Shutdown();
		ShaderCompiler::Shutdown();

		mGraphicsContext->Shutdown();
		mWindow->Close();
	}

	void Application::OnWindowClose()
	{
		CO_PROFILE_FN();

		mRunning = false;
	}

	void Application::OnWindowResize(uint32_t width, uint32_t height)
	{
		CO_PROFILE_FN();
	}

	void Application::OnMouseMove(float x, float y)
	{
		CO_PROFILE_FN();

		for (Module* module : mModules)
			module->OnMouseMove(x, y);
	}

}