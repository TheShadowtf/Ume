#pragma once

#include <string>
#include <filesystem>

#include "Ume/Core/Base.h"
#include "Ume/Renderer/EditorCamera.h"

namespace Ume {

	       struct ProjectConfig
	       {
		       std::string Name = "Untitled";
		       std::string ProjectType = "3D";

		       std::filesystem::path StartScene;

		       std::filesystem::path AssetDirectory;
		       std::filesystem::path ScriptModulePath;

		       // Optional: directory where runtime assets are located (e.g., for released projects).
		       // If empty, assets are resolved relative to the project directory.
		       // Allows projects to be portable across different installations.
		       std::filesystem::path RuntimeAssetDirectory;

		       // Directory where compiled VPAK bundles are copied for local runtime/editor use.
		       std::filesystem::path RuntimeVPackDirectory;

		       // Directory where compiled VPAK bundles are copied for exported game builds.
		       std::filesystem::path ExportVPackDirectory;

		       // Project creation date/time in ISO 8601 format (e.g., "2026-03-30T14:23:00")
		       std::string CreationDate;
	       };

	class Project
	{
	public:
		static const std::filesystem::path& GetProjectDirectory()
		{
			UME_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->m_ProjectDirectory;
		}

		static std::filesystem::path GetAssetDirectory()
		{
			UME_CORE_ASSERT(s_ActiveProject);
			// If RuntimeAssetDirectory is set (for portable projects), use that instead
			if (!s_ActiveProject->m_Config.RuntimeAssetDirectory.empty())
				return GetProjectDirectory() / s_ActiveProject->m_Config.RuntimeAssetDirectory;
			return GetProjectDirectory() / s_ActiveProject->m_Config.AssetDirectory;
		}

		static std::filesystem::path GetScriptRootDirectory()
		{
			UME_CORE_ASSERT(s_ActiveProject);

			const std::filesystem::path projectScripts = GetProjectDirectory() / "Scripts";
			if (std::filesystem::exists(projectScripts) && std::filesystem::is_directory(projectScripts))
				return projectScripts;

			const std::filesystem::path assetScripts = GetAssetDirectory() / "Scripts";
			if (std::filesystem::exists(assetScripts) && std::filesystem::is_directory(assetScripts))
				return assetScripts;

			// Prefer project-local Scripts folder for new/portable projects.
			return projectScripts;
		}

		static std::filesystem::path GetScriptModuleFilePath()
		{
			UME_CORE_ASSERT(s_ActiveProject);

			const std::filesystem::path configuredPath = s_ActiveProject->m_Config.ScriptModulePath;
			if (configuredPath.empty())
				return {};

			if (configuredPath.is_absolute())
				return configuredPath;

			std::filesystem::path relativePath = configuredPath;
			auto it = relativePath.begin();
			if (it != relativePath.end())
			{
				const std::string firstPart = it->string();
				if (firstPart == "Scripts" || firstPart == "scripts")
				{
					std::filesystem::path trimmed;
					++it;
					for (; it != relativePath.end(); ++it)
						trimmed /= *it;
					relativePath = trimmed;
				}
			}

			if (relativePath.empty())
				relativePath = std::filesystem::path("Binaries") / "Sandbox.dll";

			return GetScriptRootDirectory() / relativePath;
		}

		// TODO(Yan): move to asset manager when we have one
		static std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path& path)
		{
			UME_CORE_ASSERT(s_ActiveProject);
			return GetAssetDirectory() / path;
		}

		static std::filesystem::path GetRuntimeVPackDirectory()
		{
			UME_CORE_ASSERT(s_ActiveProject);
			if (s_ActiveProject->m_Config.RuntimeVPackDirectory.empty())
				return GetProjectDirectory() / "Build" / "Paks";
			return GetProjectDirectory() / s_ActiveProject->m_Config.RuntimeVPackDirectory;
		}

		static std::filesystem::path GetExportVPackDirectory()
		{
			UME_CORE_ASSERT(s_ActiveProject);
			if (s_ActiveProject->m_Config.ExportVPackDirectory.empty())
				return GetProjectDirectory() / "Export" / "Windows" / "Content" / "Paks";
			return GetProjectDirectory() / s_ActiveProject->m_Config.ExportVPackDirectory;
		}

		ProjectConfig& GetConfig() { return m_Config; }
		void SetConfig(ProjectConfig& config) { m_Config = config; }

		static Ref<Project> GetActive() { return s_ActiveProject; }
		static void SetActive(Ref<Project> active) { s_ActiveProject = active; }

		static Ref<Project> New();
		static Ref<Project> Load(const std::filesystem::path& path);
		static bool SaveActive(const std::filesystem::path& path);
	private:
		ProjectConfig m_Config;
		std::filesystem::path m_ProjectDirectory;

		inline static Ref<Project> s_ActiveProject;
	};

}
