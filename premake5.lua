workspace "Ume"
    architecture "x64"
    startproject "Sandbox"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "Ume/vendor/GLFW/include"
IncludeDir["glad"] = "Ume/vendor/glad/include"
IncludeDir["ImGui"] = "Ume/vendor/imgui"
IncludeDir["glm"] = "Ume/vendor/glm"
IncludeDir["stb_image"] = "Ume/vendor/stb_image"
IncludeDir["entt"] = "Ume/vendor/entt/include"

group "Dependedncies"
    include "Ume/vendor/GLFW"
    include "Ume/vendor/glad"
    include "Ume/vendor/imgui"
group""

project "Ume"
    location "Ume"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "umepch.h"
    pchsource "Ume/src/umepch.cpp"

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/vendor/stb_image/**.cpp",
        "%{prj.name}/vendor/stb_image/**.h",
        "%{prj.name}/vendor/glm/glm/**.hpp",
        "%{prj.name}/vendor/glm/glm/**.inl"
    }

    includedirs
    {
        "%{prj.name}/src",
        "%{prj.name}/vendor/spdlog/include",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.glad}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.stb_image}",
        "%{IncludeDir.entt}"
    }

    links
    {
        "GLFW",
        "glad",
        "ImGui",
        "opengl32.lib"
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "UME_PLATFORM_WINDOWS",
            "UME_BUILD_DLL",
            "GLFW_INCLUDE_NONE",
            "IMGUI_IMPL_OPENGL_LOADER_CUSTOM",
            "_CRT_SECURE_NO_WARNINGS"

        }

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

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"
    }

    includedirs
    {
        "Ume/vendor/spdlog/include",
        "Ume/src",
        "Ume/vendor",
        "%{IncludeDir.glm}"
    }

    links
    {
        "Ume"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "UME_PLATFORM_WINDOWS"
        }

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

project "Ume-Editor"
    location "Ume-Editor"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"
    }

    includedirs
    {
        "Ume/vendor/spdlog/include",
        "Ume/src",
        "Ume/vendor",
        "%{IncludeDir.glm}",
        "%{IncludeDir.entt}"
    }

    links
    {
        "Ume"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "UME_PLATFORM_WINDOWS"
        }

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
