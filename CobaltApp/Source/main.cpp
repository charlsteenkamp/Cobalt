#include "Application.hpp"
#include "SandboxModule.hpp"

int main()
{
	Cobalt::Application app({
		.EnableImGui = false,
		.EnableOptickCapture = true
	});

	app.AddModule<Cobalt::SandboxModule>();

	app.Init();
	app.Run();
	app.Shutdown();
}