#pragma once

#include "Ume/Core/Debug/Instrumentor.h"
#include "Application.h"

#ifdef UME_PLATFORM_WINDOWS

extern Ume::Application* Ume::CreateApplication();

int main(int argc, char** argv)
{
	Ume::Log::Init();

	UME_PROFILE_BEGIN_SESSION("Startup", "UmeProfile-Startup.json");
	auto app = Ume::CreateApplication();
	UME_PROFILE_END_SESSION();

	UME_PROFILE_BEGIN_SESSION("Runtime", "UmeProfile-Runtime.json");
	app->Run();
	UME_PROFILE_END_SESSION();

	UME_PROFILE_BEGIN_SESSION("Shutdown", "UmeProfile-Shutdown.json");
	delete app;
	UME_PROFILE_END_SESSION();
}
#endif