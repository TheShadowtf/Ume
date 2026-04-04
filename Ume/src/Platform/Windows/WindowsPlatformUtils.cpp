#include "umepch.h"
#include "Ume/Utils/PlatformUtils.h"
#include "Ume/Core/Application.h"
#include <commdlg.h>
#include <shlobj.h>
#include <windows.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vector>

namespace Ume {

std::string FileDialogs::OpenFolder()
{
#ifdef _WIN32
	BROWSEINFOA bi = { 0 };
	CHAR szDisplayName[MAX_PATH] = { 0 };
	bi.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
	bi.pszDisplayName = szDisplayName;
	bi.lpszTitle = "Select a folder";
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
	if (pidl != nullptr)
	{
		CHAR szPath[MAX_PATH];
		if (SHGetPathFromIDListA(pidl, szPath))
		{
			CoTaskMemFree(pidl);
			return szPath;
		}
		CoTaskMemFree(pidl);
	}
	return std::string();
#else
	return std::string();
#endif
}

} // namespace Ume
#include "umepch.h"
#include "Ume/Utils/PlatformUtils.h"
#include "Ume/Core/Application.h"

#include <commdlg.h>
#include <shlobj.h>
#include <windows.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <vector>

namespace Ume {

	namespace
	{
		std::string QuoteCommandLineArgument(const std::string& argument)
		{
			if (argument.empty())
				return "\"\"";

			const bool needsQuotes = argument.find_first_of(" \t\"") != std::string::npos;
			if (!needsQuotes)
				return argument;

			std::string quoted = "\"";
			for (char ch : argument)
			{
				if (ch == '\"')
					quoted += '\\';
				quoted += ch;
			}
			quoted += '"';
			return quoted;
		}
	}

	float Time::GetTime()
	{
		return glfwGetTime();
	}


	std::string FileDialogs::OpenFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		CHAR currentDir[256] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		if (GetCurrentDirectoryA(256, currentDir))
			ofn.lpstrInitialDir = currentDir;
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return std::string();

	}

	std::string FileDialogs::SaveFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		CHAR currentDir[256] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		if (GetCurrentDirectoryA(256, currentDir))
			ofn.lpstrInitialDir = currentDir;
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		// Sets the default extension by extracting it from the filter
		ofn.lpstrDefExt = strchr(filter, '\0') + 1;

		if (GetSaveFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;
		
		return std::string();
	}

	std::filesystem::path ProcessUtils::GetExecutablePath()
	{
		std::vector<char> buffer(MAX_PATH);
		DWORD length = 0;

		while (true)
		{
			length = GetModuleFileNameA(nullptr, buffer.data(), (DWORD)buffer.size());
			if (length == 0)
				return {};

			if (length < buffer.size() - 1)
				break;

			buffer.resize(buffer.size() * 2);
		}

		return std::filesystem::path(std::string(buffer.data(), length));
	}

	std::filesystem::path ProcessUtils::GetAppDataDirectory()
	{
		char buffer[MAX_PATH] = {};
		DWORD length = GetEnvironmentVariableA("APPDATA", buffer, MAX_PATH);
		if (length == 0 || length >= MAX_PATH)
			return {};

		return std::filesystem::path(buffer);
	}

	bool ProcessUtils::LaunchProcess(const std::filesystem::path& executablePath, const std::vector<std::string>& arguments, const std::filesystem::path& workingDirectory)
	{
		if (executablePath.empty())
			return false;

		std::string commandLine = QuoteCommandLineArgument(executablePath.string());
		for (const auto& argument : arguments)
		{
			commandLine += ' ';
			commandLine += QuoteCommandLineArgument(argument);
		}

		STARTUPINFOA startupInfo = {};
		startupInfo.cb = sizeof(STARTUPINFOA);

		PROCESS_INFORMATION processInfo = {};
		std::vector<char> mutableCommandLine(commandLine.begin(), commandLine.end());
		mutableCommandLine.push_back('\0');

		const std::string workingDirectoryString = workingDirectory.empty() ? executablePath.parent_path().string() : workingDirectory.string();
		BOOL success = CreateProcessA(
			nullptr,
			mutableCommandLine.data(),
			nullptr,
			nullptr,
			FALSE,
			0,
			nullptr,
			workingDirectoryString.empty() ? nullptr : workingDirectoryString.c_str(),
			&startupInfo,
			&processInfo);

		if (!success)
			return false;

		CloseHandle(processInfo.hThread);
		CloseHandle(processInfo.hProcess);
		return true;
	}

	int ProcessUtils::LaunchProcessSync(const std::filesystem::path& executablePath, const std::vector<std::string>& arguments, const std::filesystem::path& workingDirectory)
	{
		if (executablePath.empty())
			return -1;

		std::string commandLine = QuoteCommandLineArgument(executablePath.string());
		for (const auto& argument : arguments)
		{
			commandLine += ' ';
			commandLine += QuoteCommandLineArgument(argument);
		}

		STARTUPINFOA startupInfo = {};
		startupInfo.cb = sizeof(STARTUPINFOA);

		PROCESS_INFORMATION processInfo = {};
		std::vector<char> mutableCommandLine(commandLine.begin(), commandLine.end());
		mutableCommandLine.push_back('\0');

		const std::string workingDirectoryString = workingDirectory.empty() ? executablePath.parent_path().string() : workingDirectory.string();
		BOOL success = CreateProcessA(
			nullptr,
			mutableCommandLine.data(),
			nullptr,
			nullptr,
			FALSE,
			0,
			nullptr,
			workingDirectoryString.empty() ? nullptr : workingDirectoryString.c_str(),
			&startupInfo,
			&processInfo);

		if (!success)
			return -1;

		CloseHandle(processInfo.hThread);
		WaitForSingleObject(processInfo.hProcess, INFINITE);

		DWORD exitCode = 0;
		GetExitCodeProcess(processInfo.hProcess, &exitCode);
		CloseHandle(processInfo.hProcess);
		return static_cast<int>(exitCode);
	}

}
