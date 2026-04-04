#include "Ume/Utils/PlatformUtils.h"
#include <imgui.h>
#include <imgui_internal.h>

#ifdef _WIN32
    #include <windows.h>
    #include <shellapi.h>
    #include <shlobj.h>
#endif

#include <filesystem>
#include <string>
#include <algorithm>

namespace Ume {

    // --- Internal State for the Picker ---
    static std::filesystem::path s_CurrentPath = std::filesystem::current_path();
    static std::string s_SelectedEntry = "";

    // 1. MATCHING HEADER: OpenFolderPicker
    bool FileDialogs::OpenFolderPicker(std::string& outPath) {
        if (!ImGui::IsPopupOpen("Open a Directory"))
            ImGui::OpenPopup("Open a Directory");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.13f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);

        bool folderSelected = false;

        if (ImGui::BeginPopupModal("Open a Directory", NULL, ImGuiWindowFlags_NoScrollbar)) {
            
            // Breadcrumbs / Path
            ImGui::BeginChild("NavBar", ImVec2(0, 40), true);
            if (ImGui::Button(" < ") && s_CurrentPath.has_parent_path()) { 
                s_CurrentPath = s_CurrentPath.parent_path(); 
                s_SelectedEntry = ""; 
            }
            ImGui::SameLine();
            ImGui::Text("Path: %s", s_CurrentPath.string().c_str());
            ImGui::EndChild();

            // Grid View
            ImGui::BeginChild("BrowserView", ImVec2(0, -50), true);
            float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
            float item_width = 100.0f;
            
            int id = 0;
            try {
                for (auto const& entry : std::filesystem::directory_iterator(s_CurrentPath)) {
                    ImGui::PushID(id++);
                    bool isDir = entry.is_directory();
                    std::string filename = entry.path().filename().string();

                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    bool selected = (s_SelectedEntry == filename);
                    
                    if (ImGui::InvisibleButton(filename.c_str(), ImVec2(item_width, 110.0f))) {
                        s_SelectedEntry = filename;
                        if (isDir && ImGui::IsMouseDoubleClicked(0)) {
                            s_CurrentPath /= filename;
                            s_SelectedEntry = "";
                        }
                    }

                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    ImU32 bgColor = selected ? IM_COL32(60, 75, 95, 255) : (ImGui::IsItemHovered() ? IM_COL32(45, 45, 50, 255) : IM_COL32(35, 35, 38, 255));
                    dl->AddRectFilled(pos, {pos.x + item_width, pos.y + 110}, bgColor, 6.0f);
                    
                    // Folder Icon Color
                    ImU32 iconCol = isDir ? IM_COL32(100, 150, 240, 255) : IM_COL32(180, 180, 180, 255);
                    dl->AddRectFilled({pos.x + 25, pos.y + 15}, {pos.x + 75, pos.y + 55}, iconCol, 4.0f);

                    ImGui::SetCursorScreenPos({pos.x + 5, pos.y + 65});
                    ImGui::PushTextWrapPos(pos.x + item_width - 5);
                    ImGui::TextWrapped("%s", filename.c_str());
                    ImGui::PopTextWrapPos();

                    float next_x2 = ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + item_width;
                    if (next_x2 < window_visible_x2) ImGui::SameLine();
                    ImGui::PopID();
                }
            } catch (...) {}
            ImGui::EndChild();

            if (ImGui::Button("Select Current Folder", ImVec2(200, 30))) {
                outPath = s_CurrentPath.string();
                folderSelected = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 30))) ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        return folderSelected;
    }

    // 2. MATCHING HEADER: OpenFilePicker
    bool FileDialogs::OpenFilePicker(std::string& outPath, const char* extensionFilter) {
        // For now, we use the same logic as FolderPicker to resolve the Linker error.
        // In the future, you can filter the file list by extensionFilter.
        return OpenFolderPicker(outPath); 
    }

} // namespace Ume