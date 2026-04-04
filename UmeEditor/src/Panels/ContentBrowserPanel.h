#pragma once

#include "Ume/Renderer/Texture.h"

#include <filesystem>
#include <vector>

namespace Ume {

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender();
	private:
		struct CachedEntry
		{
			std::filesystem::path Path;
			bool IsDirectory = false;
		};

		void RefreshDirectoryCache();
		bool DirectoryCacheNeedsRefresh() const;

		std::filesystem::path m_BaseDirectory;
		std::filesystem::path m_CurrentDirectory;
		std::filesystem::path m_CachedDirectory;
		std::vector<CachedEntry> m_CachedEntries;
		std::filesystem::file_time_type m_CachedDirectoryWriteTime{};
		bool m_HasCachedDirectoryWriteTime = false;
		
		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_FileIcon;
	};

}
