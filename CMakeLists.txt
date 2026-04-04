cmake_minimum_required(VERSION 3.20)

project(Ume LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This CMake setup currently supports Windows only, matching the current project state.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(UME_BUILD_EDITOR "Build the UmeEditor target" ON)
option(UME_BUILD_LAUNCHER "Build the UmeLauncher target" ON)
option(UME_BUILD_SANDBOX "Build the Sandbox target" ON)
option(UME_BUILD_VPAK_TOOL "Build the UmeVPackTool target" ON)

set(UME_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(UME_ENGINE_DIR ${UME_ROOT}/Ume)
set(UME_EDITOR_DIR ${UME_ROOT}/UmeEditor)
set(UME_LAUNCHER_DIR ${UME_ROOT}/UmeLauncher)
set(UME_SANDBOX_DIR ${UME_ROOT}/Sandbox)
set(UME_TOOLS_DIR ${UME_ROOT}/tools)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Keep output layout predictable for scripts and manual runs.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# -----------------------------
# Dependencies
# -----------------------------

# Box2D
file(GLOB_RECURSE BOX2D_SOURCES CONFIGURE_DEPENDS
    ${UME_ENGINE_DIR}/vendor/Box2D/src/*.cpp
)
add_library(Box2D STATIC ${BOX2D_SOURCES})
target_include_directories(Box2D PUBLIC
    ${UME_ENGINE_DIR}/vendor/Box2D/include
    ${UME_ENGINE_DIR}/vendor/Box2D/src
)
target_compile_features(Box2D PUBLIC cxx_std_11)

# GLFW
set(GLFW_SOURCES
    ${UME_ENGINE_DIR}/vendor/GLFW/src/context.c
    ${UME_ENGINE_DIR}/vendor/GLFW/src/init.c
    ${UME_ENGINE_DIR}/vendor/GLFW/src/input.c
    ${UME_ENGINE_DIR}/vendor/GLFW/src/monitor.c
    ${UME_ENGINE_DIR}/vendor/GLFW/src/vulkan.c
    ${UME_ENGINE_DIR}/vendor/GLFW/src/window.c
    ${UME_ENGINE_DIR}/vendor/GLFW/src/win32_init.c
    ${UME_ENGINE_DIR}/vendor/GLFW/src/win32_joystick.c
    ${UME_ENGINE_DIR}/vendor/GLFW/src/win32_monitor.c
    ${UME_ENGINE_DIR}/vendor/GLFW/src/win32_time.c
    ${UME_ENGINE_DIR}/vendor/GLFW/src/win32_thread.c
    ${UME_ENGINE_DIR}/vendor/GLFW/src/win32_window.c
    ${UME_ENGINE_DIR}/vendor/GLFW/src/wgl_context.c
    ${UME_ENGINE_DIR}/vendor/GLFW/src/egl_context.c
    ${UME_ENGINE_DIR}/vendor/GLFW/src/osmesa_context.c
)
add_library(GLFW STATIC ${GLFW_SOURCES})
target_include_directories(GLFW PUBLIC ${UME_ENGINE_DIR}/vendor/GLFW/include)
target_compile_definitions(GLFW PRIVATE _GLFW_WIN32 _CRT_SECURE_NO_WARNINGS)

# Glad
add_library(Glad STATIC ${UME_ENGINE_DIR}/vendor/Glad/src/glad.c)
target_include_directories(Glad PUBLIC ${UME_ENGINE_DIR}/vendor/Glad/include)

# ImGui
set(IMGUI_SOURCES
    ${UME_ENGINE_DIR}/vendor/imgui/imgui.cpp
    ${UME_ENGINE_DIR}/vendor/imgui/imgui_draw.cpp
    ${UME_ENGINE_DIR}/vendor/imgui/imgui_widgets.cpp
    ${UME_ENGINE_DIR}/vendor/imgui/imgui_demo.cpp
)
add_library(ImGui STATIC ${IMGUI_SOURCES})
target_include_directories(ImGui PUBLIC ${UME_ENGINE_DIR}/vendor/imgui)

# freetype bundled with msdfgen
set(FREETYPE_DIR ${UME_ENGINE_DIR}/vendor/msdf-atlas-gen/msdfgen/freetype)
set(FREETYPE_SOURCES
    ${FREETYPE_DIR}/src/autofit/autofit.c
    ${FREETYPE_DIR}/src/base/ftbase.c
    ${FREETYPE_DIR}/src/base/ftbbox.c
    ${FREETYPE_DIR}/src/base/ftbdf.c
    ${FREETYPE_DIR}/src/base/ftbitmap.c
    ${FREETYPE_DIR}/src/base/ftcid.c
    ${FREETYPE_DIR}/src/base/ftdebug.c
    ${FREETYPE_DIR}/src/base/ftfstype.c
    ${FREETYPE_DIR}/src/base/ftgasp.c
    ${FREETYPE_DIR}/src/base/ftglyph.c
    ${FREETYPE_DIR}/src/base/ftgxval.c
    ${FREETYPE_DIR}/src/base/ftinit.c
    ${FREETYPE_DIR}/src/base/ftmm.c
    ${FREETYPE_DIR}/src/base/ftotval.c
    ${FREETYPE_DIR}/src/base/ftpatent.c
    ${FREETYPE_DIR}/src/base/ftpfr.c
    ${FREETYPE_DIR}/src/base/ftstroke.c
    ${FREETYPE_DIR}/src/base/ftsynth.c
    ${FREETYPE_DIR}/src/base/ftsystem.c
    ${FREETYPE_DIR}/src/base/fttype1.c
    ${FREETYPE_DIR}/src/base/ftwinfnt.c
    ${FREETYPE_DIR}/src/bdf/bdf.c
    ${FREETYPE_DIR}/src/bzip2/ftbzip2.c
    ${FREETYPE_DIR}/src/cache/ftcache.c
    ${FREETYPE_DIR}/src/cff/cff.c
    ${FREETYPE_DIR}/src/cid/type1cid.c
    ${FREETYPE_DIR}/src/gzip/ftgzip.c
    ${FREETYPE_DIR}/src/lzw/ftlzw.c
    ${FREETYPE_DIR}/src/pcf/pcf.c
    ${FREETYPE_DIR}/src/pfr/pfr.c
    ${FREETYPE_DIR}/src/psaux/psaux.c
    ${FREETYPE_DIR}/src/pshinter/pshinter.c
    ${FREETYPE_DIR}/src/psnames/psnames.c
    ${FREETYPE_DIR}/src/raster/raster.c
    ${FREETYPE_DIR}/src/sdf/sdf.c
    ${FREETYPE_DIR}/src/sfnt/sfnt.c
    ${FREETYPE_DIR}/src/smooth/smooth.c
    ${FREETYPE_DIR}/src/truetype/truetype.c
    ${FREETYPE_DIR}/src/type1/type1.c
    ${FREETYPE_DIR}/src/type42/type42.c
    ${FREETYPE_DIR}/src/winfonts/winfnt.c
)
add_library(freetype STATIC ${FREETYPE_SOURCES})
target_include_directories(freetype PUBLIC ${FREETYPE_DIR}/include)
target_compile_definitions(freetype PRIVATE FT2_BUILD_LIBRARY _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)

# msdfgen
file(GLOB_RECURSE MSDFGEN_CORE_SOURCES CONFIGURE_DEPENDS
    ${UME_ENGINE_DIR}/vendor/msdf-atlas-gen/msdfgen/core/*.cpp
    ${UME_ENGINE_DIR}/vendor/msdf-atlas-gen/msdfgen/ext/*.cpp
    ${UME_ENGINE_DIR}/vendor/msdf-atlas-gen/msdfgen/lib/*.cpp
)
add_library(msdfgen STATIC ${MSDFGEN_CORE_SOURCES})
target_include_directories(msdfgen PUBLIC
    ${UME_ENGINE_DIR}/vendor/msdf-atlas-gen/msdfgen/include
    ${FREETYPE_DIR}/include
)
target_compile_definitions(msdfgen PUBLIC MSDFGEN_USE_CPP11)
target_link_libraries(msdfgen PUBLIC freetype)

# msdf-atlas-gen
file(GLOB_RECURSE MSDF_ATLAS_GEN_SOURCES CONFIGURE_DEPENDS
    ${UME_ENGINE_DIR}/vendor/msdf-atlas-gen/msdf-atlas-gen/*.cpp
)
add_library(msdf-atlas-gen STATIC ${MSDF_ATLAS_GEN_SOURCES})
target_include_directories(msdf-atlas-gen PUBLIC
    ${UME_ENGINE_DIR}/vendor/msdf-atlas-gen/msdf-atlas-gen
    ${UME_ENGINE_DIR}/vendor/msdf-atlas-gen/msdfgen
    ${UME_ENGINE_DIR}/vendor/msdf-atlas-gen/msdfgen/include
)
target_compile_definitions(msdf-atlas-gen PRIVATE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(msdf-atlas-gen PUBLIC msdfgen)

# yaml-cpp
file(GLOB_RECURSE YAML_CPP_SOURCES CONFIGURE_DEPENDS
    ${UME_ENGINE_DIR}/vendor/yaml-cpp/src/*.cpp
)
add_library(yaml-cpp STATIC ${YAML_CPP_SOURCES})
target_include_directories(yaml-cpp PUBLIC ${UME_ENGINE_DIR}/vendor/yaml-cpp/include)

# -----------------------------
# Ume engine
# -----------------------------

set(UME_ENGINE_SOURCES
    ${UME_ENGINE_DIR}/src/Platform/OpenGL/OpenGLBuffer.cpp
    ${UME_ENGINE_DIR}/src/Platform/OpenGL/OpenGLContext.cpp
    ${UME_ENGINE_DIR}/src/Platform/OpenGL/OpenGLFramebuffer.cpp
    ${UME_ENGINE_DIR}/src/Platform/OpenGL/OpenGLRendererAPI.cpp
    ${UME_ENGINE_DIR}/src/Platform/OpenGL/OpenGLShader.cpp
    ${UME_ENGINE_DIR}/src/Platform/OpenGL/OpenGLTexture.cpp
    ${UME_ENGINE_DIR}/src/Platform/OpenGL/OpenGLUniformBuffer.cpp
    ${UME_ENGINE_DIR}/src/Platform/OpenGL/OpenGLVertexArray.cpp
    ${UME_ENGINE_DIR}/src/Platform/Vulkan/VulkanBuffer.cpp
    ${UME_ENGINE_DIR}/src/Platform/Vulkan/VulkanContext.cpp
    ${UME_ENGINE_DIR}/src/Platform/Vulkan/VulkanFramebuffer.cpp
    ${UME_ENGINE_DIR}/src/Platform/Vulkan/VulkanRendererAPI.cpp
    ${UME_ENGINE_DIR}/src/Platform/Vulkan/VulkanShader.cpp
    ${UME_ENGINE_DIR}/src/Platform/Vulkan/VulkanTexture.cpp
    ${UME_ENGINE_DIR}/src/Platform/Vulkan/VulkanUniformBuffer.cpp
    ${UME_ENGINE_DIR}/src/Platform/Vulkan/VulkanVertexArray.cpp
    ${UME_ENGINE_DIR}/src/Platform/Windows/WindowsInput.cpp
    ${UME_ENGINE_DIR}/src/Platform/Windows/WindowsPlatformUtils.cpp
    ${UME_ENGINE_DIR}/src/Platform/Windows/WindowsWindow.cpp
    ${UME_ENGINE_DIR}/src/Ume/Core/Application.cpp
    ${UME_ENGINE_DIR}/src/Ume/Core/FileSystem.cpp
    ${UME_ENGINE_DIR}/src/Ume/Core/Layer.cpp
    ${UME_ENGINE_DIR}/src/Ume/Core/LayerStack.cpp
    ${UME_ENGINE_DIR}/src/Ume/Core/Log.cpp
    ${UME_ENGINE_DIR}/src/Ume/Core/TaskSystem.cpp
    ${UME_ENGINE_DIR}/src/Ume/Core/UUID.cpp
    ${UME_ENGINE_DIR}/src/Ume/Core/Window.cpp
    ${UME_ENGINE_DIR}/src/Ume/ImGui/ImGuiBuild.cpp
    ${UME_ENGINE_DIR}/src/Ume/ImGui/ImGuiLayer.cpp
    ${UME_ENGINE_DIR}/src/Ume/Math/Math.cpp
    ${UME_ENGINE_DIR}/src/Ume/Project/Project.cpp
    ${UME_ENGINE_DIR}/src/Ume/Project/ProjectUtils.cpp
    ${UME_ENGINE_DIR}/src/Ume/Project/ProjectSerializer.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/Buffer.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/EditorCamera.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/Font.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/Framebuffer.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/GraphicsContext.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/OrthographicCamera.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/OrthographicCameraController.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/RenderCommand.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/Renderer.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/Renderer2D.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/RendererAPI.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/Shader.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/Texture.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/UniformBuffer.cpp
    ${UME_ENGINE_DIR}/src/Ume/Renderer/VertexArray.cpp
    ${UME_ENGINE_DIR}/src/Ume/Scene/Entity.cpp
    ${UME_ENGINE_DIR}/src/Ume/Scene/Scene.cpp
    ${UME_ENGINE_DIR}/src/Ume/Scene/SceneCamera.cpp
    ${UME_ENGINE_DIR}/src/Ume/Scene/SceneSerializer.cpp
    ${UME_ENGINE_DIR}/src/Ume/Scripting/ScriptEngine.cpp
    ${UME_ENGINE_DIR}/src/Ume/Scripting/ScriptGlue.cpp
    ${UME_ENGINE_DIR}/src/Ume/Asset/VPakReader.cpp
    ${UME_ENGINE_DIR}/src/umepch.cpp
    ${UME_ENGINE_DIR}/vendor/ImGuizmo/ImGuizmo.cpp
    ${UME_ENGINE_DIR}/vendor/stb_image/stb_image.cpp
    ${UME_ENGINE_DIR}/src/Ume/Utils/PlatformUtils.cpp
)

add_library(Ume STATIC ${UME_ENGINE_SOURCES})

# Match Premake behavior: these files are compiled without PCH.
set_source_files_properties(
    ${UME_ENGINE_DIR}/vendor/ImGuizmo/ImGuizmo.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

# Enable PCH for the engine target.
target_precompile_headers(Ume PRIVATE ${UME_ENGINE_DIR}/src/umepch.h)

target_compile_definitions(Ume PRIVATE
    _CRT_SECURE_NO_WARNINGS
    GLFW_INCLUDE_NONE
    $<$<CONFIG:Debug>:UME_DEBUG>
    $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:UME_RELEASE>
)

target_include_directories(Ume PUBLIC
    ${UME_ENGINE_DIR}/src
    ${UME_ENGINE_DIR}/vendor/spdlog/include
    ${UME_ENGINE_DIR}/vendor/Box2D/include
    ${UME_ENGINE_DIR}/vendor/filewatch
    ${UME_ENGINE_DIR}/vendor/GLFW/include
    ${UME_ENGINE_DIR}/vendor/Glad/include
    ${UME_ENGINE_DIR}/vendor/imgui
    ${UME_ENGINE_DIR}/vendor/glm
    ${UME_ENGINE_DIR}/vendor/msdf-atlas-gen/msdfgen
    ${UME_ENGINE_DIR}/vendor/msdf-atlas-gen/msdf-atlas-gen
    ${UME_ENGINE_DIR}/vendor/stb_image
    ${UME_ENGINE_DIR}/vendor/entt/include
    ${UME_ENGINE_DIR}/vendor/mono/include
    ${UME_ENGINE_DIR}/vendor/yaml-cpp/include
    ${UME_ENGINE_DIR}/vendor/ImGuizmo
    $ENV{VULKAN_SDK}/Include
)

# Choose the mono static lib by configuration.
set(MONO_LIB_DEBUG ${UME_ENGINE_DIR}/vendor/mono/lib/Debug/libmono-static-sgen.lib)
set(MONO_LIB_RELEASE ${UME_ENGINE_DIR}/vendor/mono/lib/Release/libmono-static-sgen.lib)

if(NOT EXISTS ${MONO_LIB_DEBUG})
    message(FATAL_ERROR "Missing mono static library: ${MONO_LIB_DEBUG}")
endif()
if(NOT EXISTS ${MONO_LIB_RELEASE})
    message(FATAL_ERROR "Missing mono static library: ${MONO_LIB_RELEASE}")
endif()

target_link_libraries(Ume PUBLIC
    Box2D
    GLFW
    Glad
    ImGui
    msdf-atlas-gen
    yaml-cpp
    opengl32
    ws2_32
    winmm
    version
    bcrypt
    $ENV{VULKAN_SDK}/Lib/vulkan-1.lib
    $<$<CONFIG:Debug>:$ENV{VULKAN_SDK}/Lib/shaderc_combinedd.lib>
    $<$<NOT:$<CONFIG:Debug>>:$ENV{VULKAN_SDK}/Lib/shaderc_combined.lib>
    $<$<CONFIG:Debug>:$ENV{VULKAN_SDK}/Lib/spirv-cross-cored.lib>
    $<$<NOT:$<CONFIG:Debug>>:$ENV{VULKAN_SDK}/Lib/spirv-cross-core.lib>
    $<$<CONFIG:Debug>:${MONO_LIB_DEBUG}>
    $<$<NOT:$<CONFIG:Debug>>:${MONO_LIB_RELEASE}>
)

# -----------------------------
# UmeLauncher
# -----------------------------

if(UME_BUILD_LAUNCHER)
    # Generate the .rc with an absolute icon path so rc.exe finds it from any working dir.
    set(UME_LAUNCHER_ICON "${UME_LAUNCHER_DIR}/Resources/umelogo.ico")
    configure_file(
        "${UME_LAUNCHER_DIR}/Resources/UmeLauncher.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/UmeLauncher.rc"
        @ONLY
    )

    set(UME_LAUNCHER_SOURCES
        ${UME_LAUNCHER_DIR}/src/LauncherLayer.cpp
        ${UME_LAUNCHER_DIR}/src/UmeLauncher.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/UmeLauncher.rc
    )

    add_executable(UmeLauncher ${UME_LAUNCHER_SOURCES})
    target_compile_definitions(UmeLauncher PRIVATE
        $<$<CONFIG:Debug>:UME_DEBUG>
        $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:UME_RELEASE>
    )
    target_link_options(UmeLauncher PRIVATE
        $<$<CONFIG:Debug>:/MAP:$<TARGET_FILE_DIR:UmeLauncher>/UmeLauncher.map>
    )
    target_include_directories(UmeLauncher PRIVATE
        ${UME_ENGINE_DIR}/vendor/spdlog/include
        ${UME_ENGINE_DIR}/src
        ${UME_ENGINE_DIR}/vendor
        ${UME_ENGINE_DIR}/vendor/entt/include
        ${UME_ENGINE_DIR}/vendor/filewatch
        ${UME_ENGINE_DIR}/vendor/glm
        ${UME_ENGINE_DIR}/vendor/ImGuizmo
    )
    target_link_libraries(UmeLauncher PRIVATE Ume)

    add_custom_command(TARGET UmeLauncher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${UME_EDITOR_DIR}/assets"
            "$<TARGET_FILE_DIR:UmeLauncher>/assets"
        COMMENT "Syncing runtime shader assets for UmeLauncher"
        VERBATIM
    )

    if(UME_BUILD_VPAK_TOOL)
        add_dependencies(UmeLauncher UmeVPackTool)
        add_custom_command(TARGET UmeLauncher POST_BUILD
            COMMAND $<TARGET_FILE:UmeVPackTool>
                --source-dir "${UME_LAUNCHER_DIR}/Resources"
                --pack-name "LauncherIcons"
                --output "$<TARGET_FILE_DIR:UmeLauncher>/Resources/Paks"
            COMMENT "Packing UmeLauncher icon resources into LauncherIcons.vpak"
            VERBATIM
        )
    endif()
endif()

# -----------------------------
# UmeEditor
# -----------------------------

if(UME_BUILD_EDITOR)
    # Generate the .rc with an absolute icon path so rc.exe finds it from any working dir.
    set(UME_EDITOR_ICON "${UME_EDITOR_DIR}/Resources/Icons/umelogo.ico")
    configure_file(
        "${UME_EDITOR_DIR}/Resources/UmeEditor.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/UmeEditor.rc"
        @ONLY
    )

    set(UME_EDITOR_SOURCES
        ${UME_EDITOR_DIR}/src/EditorLayer/EditorLayer.cpp
        ${UME_EDITOR_DIR}/src/Panels/ContentBrowserPanel.cpp
        ${UME_EDITOR_DIR}/src/Panels/SceneHierarchyPanel.cpp
        ${UME_EDITOR_DIR}/src/Panels/ConsolePanel.cpp
        ${UME_EDITOR_DIR}/src/StartupSplash.cpp
        ${UME_EDITOR_DIR}/src/UmeEditor.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/UmeEditor.rc
    )

    add_executable(UmeEditor ${UME_EDITOR_SOURCES})
    target_compile_definitions(UmeEditor PRIVATE
        $<$<CONFIG:Debug>:UME_DEBUG>
        $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:UME_RELEASE>
    )
    target_include_directories(UmeEditor PRIVATE
        ${UME_ENGINE_DIR}/vendor/spdlog/include
        ${UME_ENGINE_DIR}/src
        ${UME_ENGINE_DIR}/vendor
        ${UME_ENGINE_DIR}/vendor/entt/include
        ${UME_ENGINE_DIR}/vendor/filewatch
        ${UME_ENGINE_DIR}/vendor/glm
        ${UME_ENGINE_DIR}/vendor/ImGuizmo
    )
    target_link_libraries(UmeEditor PRIVATE Ume)

    add_custom_command(TARGET UmeEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${UME_EDITOR_DIR}/assets"
            "$<TARGET_FILE_DIR:UmeEditor>/assets"
        COMMENT "Syncing runtime shader assets for UmeEditor"
        VERBATIM
    )

    add_custom_command(TARGET UmeEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:UmeEditor>/assets/textures"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${UME_SANDBOX_DIR}/assets/textures/Checkerboard.png"
            "$<TARGET_FILE_DIR:UmeEditor>/assets/textures/Checkerboard.png"
        COMMENT "Syncing editor fallback textures for UmeEditor"
        VERBATIM
    )

    if(UME_BUILD_VPAK_TOOL)
        add_dependencies(UmeEditor UmeVPackTool)
        add_custom_command(TARGET UmeEditor POST_BUILD
            COMMAND $<TARGET_FILE:UmeVPackTool>
                --source-dir "${UME_EDITOR_DIR}/Resources/Icons"
                --pack-name "EditorIcons"
                --output "$<TARGET_FILE_DIR:UmeEditor>/Resources/Paks"
            COMMENT "Packing UmeEditor icon resources into EditorIcons.vpak"
            VERBATIM
        )
    endif()
endif()

# -----------------------------
# Sandbox
# -----------------------------

if(UME_BUILD_SANDBOX)
    set(UME_SANDBOX_SOURCES
        ${UME_SANDBOX_DIR}/src/ExampleLayer.cpp
        ${UME_SANDBOX_DIR}/src/Sandbox2D.cpp
        ${UME_SANDBOX_DIR}/src/SandboxApp.cpp
    )

    add_executable(Sandbox ${UME_SANDBOX_SOURCES})
    target_compile_definitions(Sandbox PRIVATE
        $<$<CONFIG:Debug>:UME_DEBUG>
        $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:UME_RELEASE>
    )
    target_include_directories(Sandbox PRIVATE
        ${UME_ENGINE_DIR}/vendor/spdlog/include
        ${UME_ENGINE_DIR}/src
        ${UME_ENGINE_DIR}/vendor
        ${UME_ENGINE_DIR}/vendor/glm
        ${UME_ENGINE_DIR}/vendor/entt/include
    )
    target_link_libraries(Sandbox PRIVATE Ume)

    add_custom_command(TARGET Sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${UME_SANDBOX_DIR}/assets"
            "$<TARGET_FILE_DIR:Sandbox>/assets"
        COMMENT "Syncing runtime assets for Sandbox"
        VERBATIM
    )
endif()

# -----------------------------
# UmeVPackTool
# -----------------------------

if(UME_BUILD_VPAK_TOOL)
    set(UME_VPAK_TOOL_SOURCES
        ${UME_TOOLS_DIR}/UmeVPack/src/UmeVPackTool.cpp
    )

    add_executable(UmeVPackTool ${UME_VPAK_TOOL_SOURCES})
    target_compile_definitions(UmeVPackTool PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
    target_include_directories(UmeVPackTool PRIVATE
        ${UME_ENGINE_DIR}/vendor/stb_image
        ${UME_ENGINE_DIR}/vendor/yaml-cpp/include
    )
    target_link_libraries(UmeVPackTool PRIVATE yaml-cpp)
    set_target_properties(UmeVPackTool PROPERTIES FOLDER "Tools")
endif()
