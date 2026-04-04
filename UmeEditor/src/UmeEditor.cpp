#include <Ume.h>
#include <Ume/Core/EntryPoint.h>

#include "EditorLayer/EditorLayer.h"
#include "StartupSplash.h"
#include <iostream>
#include <filesystem>

namespace Ume {

	class UmeEditor : public Application
	{
	public:
		UmeEditor(const ApplicationSpecification& spec)
			: Application(spec)
		{
			UME_CORE_INFO("[EditorStartup] Constructing EditorLayer.");
			EditorLayer* editorLayer = new EditorLayer();
			UME_CORE_INFO("[EditorStartup] EditorLayer constructed; pushing layer.");
			PushLayer(editorLayer);
			UME_CORE_INFO("[EditorStartup] EditorLayer push completed.");
		}

		~UmeEditor() override
		{
			StartupSplash::Stop();
		}
	};

	int a[];
	int b[];

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "UmeEditor";
		spec.StartHidden = true;
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

		StartupSplash::Start();

		return new UmeEditor(spec);
	}

}
