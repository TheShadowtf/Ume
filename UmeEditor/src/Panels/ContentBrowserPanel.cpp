#include "umepch.h"
#include "ContentBrowserPanel.h"

#include "Ume/ImGui/ImGuiTextureHelpers.h"
#include "Ume/Project/Project.h"

#include <imgui/imgui.h>

namespace Ume {

	namespace
	{
		bool PathMatchesFilter(const std::filesystem::path& path, const char* filter)
		{
			return filter[0] == '\0' || path.filename().string().find(filter) != std::string::npos;
		}
	}

	ContentBrowserPanel::ContentBrowserPanel()
		: m_BaseDirectory(Project::GetAssetDirectory()), m_CurrentDirectory(m_BaseDirectory)
	{
		m_DirectoryIcon = Texture2D::Create("Resources/Icons/ContentBrowser/DirectoryIcon.png");
		m_FileIcon = Texture2D::Create("Resources/Icons/ContentBrowser/FileIcon.png");
		RefreshDirectoryCache();
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		static char fileFilter[128] = "";
		char directoryBuffer[512] = {};
		bool navigateToParent = false;
		std::filesystem::path nextDirectory;
		ImGui::Begin("FileSystem");

		if (m_CurrentDirectory != m_CachedDirectory || DirectoryCacheNeedsRefresh())
			RefreshDirectoryCache();

		if (ImGui::BeginTabBar("##FileSystemTabs", ImGuiTabBarFlags_FittingPolicyScroll))
		{
			if (ImGui::BeginTabItem("FileSystem"))
			{
				if (m_CurrentDirectory != std::filesystem::path(m_BaseDirectory))
				{
					if (ImGui::Button("<-", ImVec2(28.0f, 0.0f)))
						navigateToParent = true;
					ImGui::SameLine();
				}

				std::string relativeDirectory = m_CurrentDirectory == m_BaseDirectory
					? "res://"
					: "res://" + std::filesystem::relative(m_CurrentDirectory, m_BaseDirectory).generic_string();
				strncpy_s(directoryBuffer, sizeof(directoryBuffer), relativeDirectory.c_str(), _TRUNCATE);
				ImGui::SetNextItemWidth(-1.0f);
				ImGui::InputText("##CurrentDirectory", directoryBuffer, sizeof(directoryBuffer), ImGuiInputTextFlags_ReadOnly);

				ImGui::SetNextItemWidth(-1.0f);
				ImGui::InputTextWithHint("##FileSystemFilter", "Filter Files", fileFilter, sizeof(fileFilter));
				ImGui::Separator();

				ImGui::BeginChild("##FileSystemEntries", ImVec2(0.0f, 0.0f), false);
				for (const auto& entry : m_CachedEntries)
				{
					const auto& path = entry.Path;
					if (!PathMatchesFilter(path, fileFilter))
						continue;

					const std::string filename = path.filename().string();
					ImGui::PushID(filename.c_str());
					Ref<Texture2D> icon = entry.IsDirectory ? m_DirectoryIcon : m_FileIcon;
					ImGui::Image(ImGuiTextureHelpers::GetTextureID(icon), ImVec2(16.0f, 16.0f), ImVec2(0, 1), ImVec2(1, 0));
					ImGui::SameLine(0.0f, 8.0f);
					if (ImGui::Selectable(filename.c_str(), false, 0, ImVec2(0.0f, 20.0f)))
					{
						if (entry.IsDirectory)
							nextDirectory = path;
					}

					if (ImGui::BeginDragDropSource())
					{
						std::filesystem::path relativePath(path);
						const wchar_t* itemPath = relativePath.c_str();
						ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
						ImGui::TextUnformatted(filename.c_str());
						ImGui::EndDragDropSource();
					}

					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && entry.IsDirectory)
						nextDirectory = path;

					ImGui::PopID();
				}
				ImGui::EndChild();

				if (navigateToParent)
				{
					m_CurrentDirectory = m_CurrentDirectory.parent_path();
					RefreshDirectoryCache();
				}
				else if (!nextDirectory.empty() && nextDirectory != m_CurrentDirectory)
				{
					m_CurrentDirectory = nextDirectory;
					RefreshDirectoryCache();
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("History"))
			{
				ImGui::TextDisabled("Recent filesystem history is not tracked yet.");
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
	}

	void ContentBrowserPanel::RefreshDirectoryCache()
	{
		m_CachedDirectory = m_CurrentDirectory;
		m_CachedEntries.clear();

		std::error_code ec;
		if (!std::filesystem::exists(m_CurrentDirectory, ec))
			return;

		for (const auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory, ec))
		{
			if (ec)
				break;

			CachedEntry entry;
			entry.Path = directoryEntry.path();
			entry.IsDirectory = directoryEntry.is_directory();
			m_CachedEntries.emplace_back(std::move(entry));
		}

		std::sort(m_CachedEntries.begin(), m_CachedEntries.end(), [](const CachedEntry& left, const CachedEntry& right)
		{
			if (left.IsDirectory != right.IsDirectory)
				return left.IsDirectory > right.IsDirectory;
			return left.Path.filename().string() < right.Path.filename().string();
		});

		m_HasCachedDirectoryWriteTime = false;
		const auto directoryWriteTime = std::filesystem::last_write_time(m_CurrentDirectory, ec);
		if (!ec)
		{
			m_CachedDirectoryWriteTime = directoryWriteTime;
			m_HasCachedDirectoryWriteTime = true;
		}
	}

	bool ContentBrowserPanel::DirectoryCacheNeedsRefresh() const
	{
		std::error_code ec;
		if (!std::filesystem::exists(m_CurrentDirectory, ec))
			return false;

		const auto directoryWriteTime = std::filesystem::last_write_time(m_CurrentDirectory, ec);
		if (ec)
			return false;

		if (!m_HasCachedDirectoryWriteTime)
			return true;

		return directoryWriteTime != m_CachedDirectoryWriteTime;
	}

}
