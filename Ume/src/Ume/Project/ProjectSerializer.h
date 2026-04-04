#pragma once

#include "Project.h"

namespace Ume {

	class ProjectSerializer
	{
	public:
		ProjectSerializer(Ref<Project> project);

		bool Serialize(const std::filesystem::path& filepath);
		bool Deserialize(const std::filesystem::path& filepath);

		inline Ref<Project> GetProject() { return m_Project; }
	private:
		Ref<Project> m_Project;
	};

}
