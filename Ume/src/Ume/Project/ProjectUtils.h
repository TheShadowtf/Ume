#pragma once

#include <filesystem>
#include <string>

namespace Ume {

	struct ProjectCreationResult
	{
		bool Success = false;
		std::filesystem::path ProjectFilePath;
		std::string ErrorMessage;
	};

	class ProjectUtils
	{
	public:
		static ProjectCreationResult CreateProjectFromTemplate(const std::filesystem::path& projectFilepath);
	};

}