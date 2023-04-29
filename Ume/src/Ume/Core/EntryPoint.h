#pragma once

#include "Ume/Core/Log/Log.h"
#include "Application.h"

#ifdef UME_PLATFORM_WINDOWS

extern Ume::Application* Ume::CreateApplication();

int main(int argc, char** argv)
{
	Ume::Log::Init();
	UME_CORE_WARN("Initialized Log!");
	int a = 5;
	UME_INFO("Hello! Var={0}", a);

	auto app = Ume::CreateApplication();
	app->Run();
	delete app;
}
#endif