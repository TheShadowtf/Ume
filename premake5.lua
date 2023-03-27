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

include "Ume/vendor/GLFW"
include "Ume/vendor/glad"
include "Ume/vendor/imgui"

project "Ume"
    location "Ume"
    kind "SharedLib"
    language "C++"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "umepch.h"
    pchsource "Ume/src/umepch.cpp"

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
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
        "%{IncludeDir.glm}"
    }

    links
    {
        "GLFW",
        "glad",
        "ImGui",
        "opengl32.lib"
    }

    postbuildcommands
    {
        ("{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/Sandbox/\"")
    }

    filter "system:windows"
        cppdialect "C++17"
        systemversion "latest"

        defines
        {
            "UME_PLATFORM_WINDOWS",
            "UME_BUILD_DLL",
            "GLFW_INCLUDE_NONE",
            "IMGUI_IMPL_OPENGL_LOADER_CUSTOM"
        }

    filter "configurations:Debug"
        defines "UME_DEBUG"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines "UME_RELEASE"
        runtime "Release"
        optimize "On"

    filter "configurations:Dist"
        defines "UME_DIST"
        buildoptions "/MD"
        optimize "On"

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"
    staticruntime "off"

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
        "%{IncludeDir.glm}"
    }

    links
    {
        "Ume"
    }

    filter "system:windows"
        cppdialect "C++17"
        systemversion "latest"

        defines
        {
            "UME_PLATFORM_WINDOWS"
        }

    filter "configurations:Debug"
        defines "UME_DEBUG"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines "UME_RELEASE"
        runtime "Release"
        optimize "On"

    filter "configurations:Dist"
        defines "UME_DIST"
        buildoptions "/MD"
        optimize "On"