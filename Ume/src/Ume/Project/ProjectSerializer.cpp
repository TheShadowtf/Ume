#include "umepch.h"
#include "ProjectSerializer.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Ume {

	ProjectSerializer::ProjectSerializer(Ref<Project> project)
		: m_Project(project)
	{
	}

	bool ProjectSerializer::Serialize(const std::filesystem::path& filepath)
	{
		const auto& config = m_Project->GetConfig();

		YAML::Emitter out;
		{
			out << YAML::BeginMap; // Root
			out << YAML::Key << "Project" << YAML::Value;
			{
				out << YAML::BeginMap;// Project
				       out << YAML::Key << "Name" << YAML::Value << config.Name;
				       out << YAML::Key << "ProjectType" << YAML::Value << config.ProjectType;
				       out << YAML::Key << "StartScene" << YAML::Value << config.StartScene.string();
				       out << YAML::Key << "AssetDirectory" << YAML::Value << config.AssetDirectory.string();
				       out << YAML::Key << "ScriptModulePath" << YAML::Value << config.ScriptModulePath.string();
				       if (!config.RuntimeAssetDirectory.empty())
					       out << YAML::Key << "RuntimeAssetDirectory" << YAML::Value << config.RuntimeAssetDirectory.string();
				       if (!config.RuntimeVPackDirectory.empty())
					       out << YAML::Key << "RuntimeVPackDirectory" << YAML::Value << config.RuntimeVPackDirectory.string();
				       if (!config.ExportVPackDirectory.empty())
					       out << YAML::Key << "ExportVPackDirectory" << YAML::Value << config.ExportVPackDirectory.string();
				       if (!config.CreationDate.empty())
					       out << YAML::Key << "CreationDate" << YAML::Value << config.CreationDate;
				out << YAML::EndMap; // Project
			}
			out << YAML::EndMap; // Root
		}

		std::ofstream fout(filepath);
		fout << out.c_str();

		return true;
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		auto& config = m_Project->GetConfig();

		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath.string());
		}
		catch (YAML::ParserException e)
		{
			UME_CORE_ERROR("Failed to load project file '{0}'\n     {1}", filepath, e.what());
			return false;
		}

		auto projectNode = data["Project"];
		if (!projectNode)
			return false;

		       config.Name = projectNode["Name"].as<std::string>();
		       config.ProjectType = projectNode["ProjectType"].as<std::string>();
		       config.StartScene = projectNode["StartScene"].as<std::string>();
		       config.AssetDirectory = projectNode["AssetDirectory"].as<std::string>();
		       config.ScriptModulePath = projectNode["ScriptModulePath"].as<std::string>();

		       // RuntimeAssetDirectory is optional, for portable/released projects
		       if (projectNode["RuntimeAssetDirectory"])
			       config.RuntimeAssetDirectory = projectNode["RuntimeAssetDirectory"].as<std::string>();

		       // Optional VPAK staging directories for runtime and exports.
		       if (projectNode["RuntimeVPackDirectory"])
			       config.RuntimeVPackDirectory = projectNode["RuntimeVPackDirectory"].as<std::string>();
		       if (projectNode["ExportVPackDirectory"])
			       config.ExportVPackDirectory = projectNode["ExportVPackDirectory"].as<std::string>();

		       // CreationDate is optional
		       if (projectNode["CreationDate"])
			       config.CreationDate = projectNode["CreationDate"].as<std::string>();

		       return true;
	}

}
