project "UmeLauncher"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"%{wks.location}/Ume/vendor/spdlog/include",
		"%{wks.location}/Ume/src",
		"%{wks.location}/Ume/vendor",
		"%{IncludeDir.entt}",
		"%{IncludeDir.filewatch}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGuizmo}"
	}

	links
	{
		"Ume"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "UME_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "UME_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "UME_DIST"
		runtime "Release"
		optimize "on"