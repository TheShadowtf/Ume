#include "umepch.h"
#include "ProjectUtils.h"

#include "Project.h"
#include "Ume/Utils/PlatformUtils.h"

namespace Ume {

	namespace
	{
		bool DirectoryContainsProjectFile(const std::filesystem::path& directory)
		{
			if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
				return false;

			for (const auto& entry : std::filesystem::directory_iterator(directory))
			{
				if (entry.is_regular_file() && entry.path().extension() == ".uproj")
					return true;
			}

			return false;
		}

		std::filesystem::path FindTemplateDirectory()
		{
			std::vector<std::filesystem::path> templateCandidates;

			std::filesystem::path cursor = ProcessUtils::GetExecutablePath().parent_path();
			for (int depth = 0; depth < 6 && !cursor.empty(); depth++)
			{
				templateCandidates.emplace_back(cursor / "TestProject");
				templateCandidates.emplace_back(cursor / "SandboxProject");
				templateCandidates.emplace_back(cursor / "UmeEditor" / "TestProject");
				templateCandidates.emplace_back(cursor / "UmeEditor" / "SandboxProject");
				cursor = cursor.parent_path();
			}

			if (Project::GetActive())
				templateCandidates.emplace_back(Project::GetProjectDirectory());

			for (const auto& candidate : templateCandidates)
			{
				if (DirectoryContainsProjectFile(candidate))
					return candidate;
			}

			return {};
		}
	}

	ProjectCreationResult ProjectUtils::CreateProjectFromTemplate(const std::filesystem::path& requestedProjectFilepath)
	{
		ProjectCreationResult result;

		std::filesystem::path projectFilepath = requestedProjectFilepath;
		if (projectFilepath.empty())
		{
			result.ErrorMessage = "Project path is empty.";
			return result;
		}

		if (projectFilepath.extension() != ".uproj")
			projectFilepath += ".uproj";

		const std::filesystem::path projectDirectory = projectFilepath.parent_path();
		if (projectDirectory.empty())
		{
			result.ErrorMessage = "Project directory is invalid.";
			return result;
		}

		std::error_code ec;
		std::filesystem::create_directories(projectDirectory, ec);
		if (ec)
		{
			result.ErrorMessage = "Could not create project directory: " + ec.message();
			return result;
		}

		if (std::filesystem::exists(projectFilepath))
		{
			result.ErrorMessage = "Project file already exists.";
			return result;
		}

		if (!std::filesystem::is_empty(projectDirectory, ec))
		{
			result.ErrorMessage = "Project directory must be empty.";
			return result;
		}

		const std::filesystem::path templateDirectory = FindTemplateDirectory();
		if (templateDirectory.empty())
		{
			result.ErrorMessage = "Could not find a project template directory.";
			return result;
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(templateDirectory))
		{
			const std::filesystem::path relative = std::filesystem::relative(entry.path(), templateDirectory, ec);
			if (ec)
			{
				result.ErrorMessage = "Failed to resolve template path: " + ec.message();
				return result;
			}

			const std::filesystem::path destination = projectDirectory / relative;
			if (entry.is_directory())
			{
				std::filesystem::create_directories(destination, ec);
				if (ec)
				{
					result.ErrorMessage = "Failed to create project directory: " + ec.message();
					return result;
				}
			}
			else if (entry.is_regular_file())
			{
				std::filesystem::create_directories(destination.parent_path(), ec);
				if (ec)
				{
					result.ErrorMessage = "Failed to create project parent directory: " + ec.message();
					return result;
				}

				ec.clear();
				std::filesystem::copy_file(entry.path(), destination, std::filesystem::copy_options::overwrite_existing, ec);
				if (ec)
				{
					result.ErrorMessage = "Failed to copy template file: " + ec.message();
					return result;
				}
			}
		}

		std::filesystem::path copiedTemplateProjectFile;
		for (const auto& entry : std::filesystem::recursive_directory_iterator(projectDirectory))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".uproj")
			{
				copiedTemplateProjectFile = entry.path();
				break;
			}
		}

		if (copiedTemplateProjectFile.empty())
		{
			result.ErrorMessage = "Template copy completed but no .uproj file was found.";
			return result;
		}

		ec.clear();
		if (copiedTemplateProjectFile != projectFilepath)
			std::filesystem::rename(copiedTemplateProjectFile, projectFilepath, ec);

		if (ec)
		{
			result.ErrorMessage = "Failed to rename template project file: " + ec.message();
			return result;
		}

		Ref<Project> newProject = Project::Load(projectFilepath);
		if (!newProject)
		{
			result.ErrorMessage = "Failed to load the newly created project.";
			return result;
		}


		       auto& config = newProject->GetConfig();
		       config.Name = projectFilepath.stem().string();
		       if (config.AssetDirectory.empty())
			       config.AssetDirectory = "Assets";
		       if (config.ScriptModulePath.empty())
			       config.ScriptModulePath = "Scripts/Binaries/Sandbox.dll";
		       if (config.RuntimeAssetDirectory.empty())
			       config.RuntimeAssetDirectory = config.AssetDirectory;
		       if (config.RuntimeVPackDirectory.empty())
			       config.RuntimeVPackDirectory = std::filesystem::path("Build") / "Paks";
		       if (config.ExportVPackDirectory.empty())
			       config.ExportVPackDirectory = std::filesystem::path("Export") / "Windows" / "Content" / "Paks";

		       // Set creation date to current time in ISO 8601 format
		       {
			       std::time_t now = std::time(nullptr);
			       std::tm tm;
	#ifdef _WIN32
			       localtime_s(&tm, &now);
	#else
			       localtime_r(&now, &tm);
	#endif
			       char buf[32];
			       std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
			       config.CreationDate = buf;
		       }

		       if (!Project::SaveActive(projectFilepath))
		       {
			       result.ErrorMessage = "Failed to save the newly created project.";
			       return result;
		       }

		result.Success = true;
		result.ProjectFilePath = projectFilepath;
		return result;
	}

}