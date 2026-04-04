#include <Ume.h>
#include <Ume/Core/EntryPoint.h>

#include "LauncherLayer.h"

namespace Ume {

	class UmeLauncherApp : public Application
	{
	public:
		UmeLauncherApp(const ApplicationSpecification& spec)
			: Application(spec)
		{
			PushLayer(new LauncherLayer());
		}
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "Ume Launcher";
		spec.Width = 1280;
		spec.Height = 780;
		spec.CommandLineArgs = args;

		if (args.Count > 0 && args[0] != nullptr)
		{
			std::error_code ec;
			std::filesystem::path executablePath = std::filesystem::weakly_canonical(std::filesystem::path(args[0]), ec);
			if (ec)
				executablePath = std::filesystem::absolute(std::filesystem::path(args[0]), ec);

			if (!executablePath.empty())
				spec.WorkingDirectory = executablePath.parent_path().string();
		}

		return new UmeLauncherApp(spec);
	}

}