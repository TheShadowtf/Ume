#pragma once

#include "Ume.h"

namespace Ume {

	class LauncherLayer : public Layer
	{
	public:
		LauncherLayer();
		virtual ~LauncherLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& e) override;
	private:
		       struct RecentProjectEntry
		       {
			       std::filesystem::path Path;
			       std::string Name;
			       std::string LastEdited;
			       std::filesystem::path PreviewPath;
			       Ref<Texture2D> PreviewTexture;
		       };

		void LoadSettings();
		void SaveSettings() const;
		void RefreshProjectMetadata();
		void AddRecentProject(const std::filesystem::path& projectPath);
		void RemoveRecentProject(const std::filesystem::path& projectPath);
		void LaunchProject(const std::filesystem::path& projectPath);
		void OpenExistingProject();
		void CreateProject();
		void RequestDeleteSelectedProject(bool deleteFromDisk);
		void ExecutePendingDelete();
		void LoadPreviewForProject(RecentProjectEntry& project);
		std::filesystem::path FindProjectPreviewImage(const std::filesystem::path& projectPath) const;
		bool DrawProjectCard(int index, RecentProjectEntry& project);

		std::filesystem::path NormalizeProjectPath(const std::filesystem::path& path) const;
		std::string ReadProjectName(const std::filesystem::path& projectPath) const;
		std::string ReadProjectLastEdited(const std::filesystem::path& projectPath) const;
		bool UpdateProjectLastEdited(const std::filesystem::path& projectPath, std::string* writtenTimestamp = nullptr) const;
		std::filesystem::path GetSettingsPath() const;
		std::filesystem::path GetEditorExecutablePath() const;
		void SelectProject(int index);
		bool IsSelectedProjectValid() const;
		void SetStatusMessage(const std::string& message, bool isError);
	private:
		std::vector<RecentProjectEntry> m_RecentProjects;
		int m_SelectedProjectIndex = -1;
		std::filesystem::path m_SettingsPath;
		std::filesystem::path m_PendingProjectPath;
		bool m_DeleteProjectFromDisk = false;
		std::string m_StatusMessage;
		bool m_StatusIsError = false;
	};

}