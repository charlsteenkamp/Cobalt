#include "Application.hpp"
#include "SandboxModule.hpp"

int main()
{
	Cobalt::Application app({
		.EnableImGui = true,
		.EnableOptickCapture = false
	});

	app.AddModule<Cobalt::SandboxModule>();

	app.Init();
	app.Run();
	app.Shutdown();
}