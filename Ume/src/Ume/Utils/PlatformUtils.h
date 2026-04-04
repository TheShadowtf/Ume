#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace Ume {

    class FileDialogs {
    public:
        // ImGui-based pickers
        static bool OpenFolderPicker(std::string& outPath);
        static bool OpenFilePicker(std::string& outPath, const char* extensionFilter = "*");

        // Native/Legacy fallbacks
        static std::string OpenFile(const char* filter);
        static std::string SaveFile(const char* filter);
        static std::string OpenFolder();
    };

    class Time {
    public:
        static float GetTime();
    };

    class ProcessUtils {
    public:
        // Launch and detach (fire-and-forget).
        static bool LaunchProcess(const std::filesystem::path& executablePath, 
                                 const std::vector<std::string>& args, 
                                 const std::filesystem::path& workingDirectory = "");

        // Launch and wait for completion.  Returns the process exit code,
        // or -1 on launch failure.
        static int LaunchProcessSync(const std::filesystem::path& executablePath,
                                     const std::vector<std::string>& args,
                                     const std::filesystem::path& workingDirectory = "");
        
        static std::filesystem::path GetExecutablePath();
        static std::filesystem::path GetAppDataDirectory();
    };

} // namespace Ume