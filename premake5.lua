include "./vendor/premake/premake_customization/solution_items.lua"
include "Dependencies.lua"

workspace "Ume"
	architecture "x86_64"
	startproject "UmeLauncher"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	solution_items
	{
		".editorconfig"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "vendor/premake"
	include "Ume/vendor/Box2D"
	include "Ume/vendor/GLFW"
	include "Ume/vendor/Glad"
	include "Ume/vendor/msdf-atlas-gen"
	include "Ume/vendor/imgui"
	include "Ume/vendor/yaml-cpp"
group ""

group "Core"
	include "Ume"
	include "Ume-ScriptCore"
group ""

group "Tools"
	include "UmeLauncher"
	include "UmeEditor"
group ""

group "Misc"
	include "Sandbox"
group ""
