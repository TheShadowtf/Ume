#include "LauncherLayer.h"

#include "Ume/Project/ProjectUtils.h"
#include "Ume/Utils/PlatformUtils.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <numeric> // For std::iota
#include <array>
#include <fstream>
#include <cmath>
#include <cctype> // Required for alphabetical string sorting

namespace Ume {

    namespace
    {
        constexpr size_t kMaxRecentProjects = 12;

        bool PathsMatch(const std::filesystem::path& left, const std::filesystem::path& right)
        {
            return left.lexically_normal().string() == right.lexically_normal().string();
        }

        ImVec4 GetWindowBgColor()
        {
            return ImVec4(0.12f, 0.12f, 0.13f, 1.0f); 
        }

        ImVec4 GetPanelBgColor()
        {
            return ImVec4(0.15f, 0.15f, 0.16f, 1.0f); 
        }

        ImU32 CardColor(bool selected)
        {
            return selected ? IM_COL32(50, 60, 75, 255) : IM_COL32(35, 35, 38, 255);
        }

        ImU32 CardHoverColor(bool selected)
        {
            return selected ? IM_COL32(60, 75, 95, 255) : IM_COL32(45, 45, 50, 255);
        }

        // Shared static icons
        static Ref<Texture2D> s_IconEdit, s_IconRun, s_IconRename, s_IconDuplicate, s_IconRemove, s_IconRemoveMissing;
        static Ref<Texture2D> s_DefaultLogo;
        
        static Ref<Texture2D> s_IconProjects, s_IconAssetLib, s_IconSettings;
        static Ref<Texture2D> s_IconAdd, s_IconFolder, s_IconSearch;

        static bool s_IconsLoaded = false;

        // Sorting state
        static int s_SortMode = 0;
        static int s_PrevSortMode = 0;

        void LoadIconsIfNeeded()
        {
            if (!s_IconsLoaded)
            {
                s_IconEdit = Texture2D::Create("Resources/icons/edit.png");
                s_IconRun = Texture2D::Create("Resources/icons/run.png");
                s_IconRename = Texture2D::Create("Resources/icons/rename.png");
                s_IconDuplicate = Texture2D::Create("Resources/icons/duplicate.png");
                s_IconRemove = Texture2D::Create("Resources/icons/remove.png");
                s_IconRemoveMissing = Texture2D::Create("Resources/icons/remove-missing.png");
                
                s_DefaultLogo = Texture2D::Create("Resources/Icons/umelogo.png");

                s_IconProjects = Texture2D::Create("Resources/icons/projects.png");
                s_IconAssetLib = Texture2D::Create("Resources/icons/asset_library.png");
                s_IconSettings = Texture2D::Create("Resources/icons/settings.png");
                s_IconAdd = Texture2D::Create("Resources/icons/add.png");
                s_IconFolder = Texture2D::Create("Resources/icons/folder.png");
                s_IconSearch = Texture2D::Create("Resources/icons/search.png");

                s_IconsLoaded = true;
            }
        }
    }

    LauncherLayer::LauncherLayer()
        : Layer("LauncherLayer")
    {
    }

    void LauncherLayer::OnAttach()
    {
        m_SettingsPath = GetSettingsPath();
        LoadSettings();
        RefreshProjectMetadata();
        if (!m_RecentProjects.empty())
            m_SelectedProjectIndex = 0;
    }

    void LauncherLayer::OnDetach()
    {
        SaveSettings();
    }

    void LauncherLayer::OnUpdate(Timestep ts)
    {
        (void)ts;
    }

    void LauncherLayer::OnEvent(Event& e)
    {
        (void)e;
    }

    void LauncherLayer::OnImGuiRender()
    {
        LoadIconsIfNeeded();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, GetWindowBgColor());
        
        ImGui::Begin("Ume Launcher", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings);

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        float windowWidth = ImGui::GetWindowWidth();
        float windowHeight = ImGui::GetWindowHeight();

        // ---------------------------------------------------------
        // 1. TOP NAVIGATION HEADER
        // ---------------------------------------------------------
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.09f, 1.0f)); 
        ImGui::BeginChild("TopHeader", ImVec2(windowWidth, 44.0f), false);
        
        ImGui::SetCursorPos(ImVec2(16.0f, 8.0f));
        
        if (s_DefaultLogo)
        {
            ImGui::Image((ImTextureID)(uint64_t)s_DefaultLogo->GetRendererID(), ImVec2(28.0f, 28.0f), ImVec2(0, 1), ImVec2(1, 0));
            ImGui::SameLine(0, 10.0f);
            ImGui::SetCursorPosY(13.0f); 
        }
        else
        {
            ImGui::SetCursorPosY(13.0f);
            ImGui::TextColored(ImVec4(0.40f, 0.75f, 0.95f, 1.0f), "[Logo]"); 
            ImGui::SameLine(0, 10.0f);
        }

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); 
        ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.95f, 1.0f), "UME");
        ImGui::PopFont();

        auto DrawTab = [](Ref<Texture2D> icon, const char* label, ImVec4 color) {
            float startY = ImGui::GetCursorPosY();
            if (icon) {
                ImGui::Image((ImTextureID)(uint64_t)icon->GetRendererID(), ImVec2(16.0f, 16.0f), ImVec2(0, 1), ImVec2(1, 0));
                ImGui::SameLine(0, 6.0f);
            }
            ImGui::SetCursorPosY(startY + 1.0f); 
            ImGui::TextColored(color, label);
        };

        float centerOffset = (windowWidth / 2.0f) - 120.0f;
        ImGui::SameLine(centerOffset);
        ImGui::SetCursorPosY(13.0f);
        DrawTab(s_IconProjects, "Projects", ImVec4(0.40f, 0.75f, 0.95f, 1.0f)); 
        
        ImGui::SameLine(centerOffset + 110.0f);
        ImGui::SetCursorPosY(13.0f);
        DrawTab(s_IconAssetLib, "Asset Library", ImVec4(0.60f, 0.60f, 0.60f, 1.0f)); 

        float settingsOffset = windowWidth - 100.0f;
        ImGui::SameLine(settingsOffset);
        ImGui::SetCursorPosY(13.0f);
        DrawTab(s_IconSettings, "Settings", ImVec4(0.60f, 0.60f, 0.60f, 1.0f));

        ImGui::EndChild();
        ImGui::PopStyleColor();

        // ---------------------------------------------------------
        // 2. TOOLBAR (Controls & Search)
        // ---------------------------------------------------------
        ImGui::PushStyleColor(ImGuiCol_ChildBg, GetPanelBgColor());
        ImGui::BeginChild("Toolbar", ImVec2(windowWidth, 50.0f), false);
        
        float toolbarBtnY = (50.0f - 30.0f) * 0.5f; 
        ImGui::SetCursorPos(ImVec2(10.0f, toolbarBtnY));

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f); 

        auto ToolButton = [](Ref<Texture2D> icon, const char* label, float width) -> bool {
            bool pressed = false;
            ImVec2 cursor = ImGui::GetCursorScreenPos();
            ImVec2 size = ImVec2(width, 30.0f);
            
            pressed = ImGui::InvisibleButton(label, size);
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            
            ImU32 col = IM_COL32(50, 50, 55, 255);
            if (ImGui::IsItemHovered()) col = IM_COL32(70, 70, 75, 255);
            if (ImGui::IsItemActive()) col = IM_COL32(90, 90, 95, 255);
            
            drawList->AddRectFilled(cursor, ImVec2(cursor.x + size.x, cursor.y + size.y), col, 4.0f);
            drawList->AddRect(cursor, ImVec2(cursor.x + size.x, cursor.y + size.y), IM_COL32(30, 30, 30, 255), 4.0f);

            float paddingLeft = 10.0f;
            if (icon) {
                float iconSize = 14.0f;
                float iconY = cursor.y + (size.y - iconSize) * 0.5f;
                drawList->AddImage((ImTextureID)(uint64_t)icon->GetRendererID(), ImVec2(cursor.x + paddingLeft, iconY), ImVec2(cursor.x + paddingLeft + iconSize, iconY + iconSize), ImVec2(0, 1), ImVec2(1, 0));
                paddingLeft += iconSize + 6.0f;
            }

            ImVec2 textSize = ImGui::CalcTextSize(label);
            float textY = cursor.y + (size.y - textSize.y) * 0.5f - 1.0f; 
            drawList->AddText(ImVec2(cursor.x + paddingLeft, std::floor(textY)), IM_COL32(220, 220, 220, 255), label);
            
            return pressed;
        };

        if (ToolButton(s_IconAdd, "Create", 80.0f)) CreateProject(); 
        ImGui::SameLine();
        if (ToolButton(s_IconFolder, "Import", 85.0f)) OpenExistingProject();
        ImGui::SameLine();
        if (ToolButton(s_IconSearch, "Scan", 75.0f)) RefreshProjectMetadata();
        
        ImGui::SameLine(0, 15.0f); 
        float filterStartX = ImGui::GetCursorPosX();

        float sortWidth = 160.0f;
        float sortLabelWidth = ImGui::CalcTextSize("Sort:").x;
        float sortPos = windowWidth - sortWidth - sortLabelWidth - 26.0f;
        
        float filterWidth = sortPos - filterStartX - 20.0f; 
        if (filterWidth < 100.0f) filterWidth = 100.0f;

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.09f, 1.0f)); 
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.10f, 0.10f, 0.11f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.12f, 0.13f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f)); 
        
        float inputHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
        float inputY = (50.0f - inputHeight) * 0.5f; 
        
        ImGui::SetCursorPosY(inputY);
        ImGui::SetNextItemWidth(filterWidth);
        static char filterBuffer[128] = "";
        ImGui::InputTextWithHint("##ProjectFilter", "Filter Projects", filterBuffer, sizeof(filterBuffer));
        
        ImGui::PopStyleVar(); 
        ImGui::PopStyleColor(3); 

        ImGui::SameLine(sortPos);
        
        float comboHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
        float sortY = (50.0f - comboHeight) * 0.5f;
        
        ImGui::SetCursorPosY(sortY);
        ImGui::AlignTextToFramePadding(); 
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Sort:");
        
        ImGui::SameLine();
        ImGui::SetCursorPosY(sortY);
        ImGui::SetNextItemWidth(sortWidth);
        const char* sortItems[] = { "Last Edited", "Name", "Path" };
        
        bool sortChanged = ImGui::Combo("##ProjectSort", &s_SortMode, sortItems, 3);

        ImGui::PopStyleVar(); 
        ImGui::EndChild();
        ImGui::PopStyleColor();

        if (!m_StatusMessage.empty())
        {
            ImGui::SetCursorPosX(16.0f);
            ImGui::TextColored(m_StatusIsError ? ImVec4(0.92f, 0.36f, 0.34f, 1.0f) : ImVec4(0.48f, 0.84f, 0.55f, 1.0f), "%s", m_StatusMessage.c_str());
        }

        // ---------------------------------------------------------
        // ACTIVE SORTING EXECUTION
        // Executed right here so the list updates instantly
        // ---------------------------------------------------------
        // Only sort when the sort mode changes
        if (sortChanged || s_SortMode != s_PrevSortMode) {
            std::filesystem::path selectedPath;
            if (m_SelectedProjectIndex >= 0 && m_SelectedProjectIndex < (int)m_RecentProjects.size())
                selectedPath = m_RecentProjects[m_SelectedProjectIndex].Path;

            std::sort(m_RecentProjects.begin(), m_RecentProjects.end(), [](const RecentProjectEntry& a, const RecentProjectEntry& b) {
                auto toLower = [](const std::string& str) {
                    std::string lowerStr = str;
                    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c){ return std::tolower(c); });
                    return lowerStr;
                };
                if (s_SortMode == 0) // Last Edited
                {
                    if (a.CreationDate.empty() && b.CreationDate.empty()) return toLower(a.Name) < toLower(b.Name);
                    if (a.CreationDate.empty()) return false;
                    if (b.CreationDate.empty()) return true;
                    return a.CreationDate > b.CreationDate;
                } 
                else if (s_SortMode == 1) // Name
                {
                    return toLower(a.Name) < toLower(b.Name);
                } 
                else // Path
                {
                    return toLower(a.Path.string()) < toLower(b.Path.string());
                }
            });

            // Reselect the highlighted item after it moved
            if (!selectedPath.empty()) {
                for (int i = 0; i < (int)m_RecentProjects.size(); i++) {
                    if (m_RecentProjects[i].Path == selectedPath) {
                        m_SelectedProjectIndex = i;
                        break;
                    }
                }
            }
            s_PrevSortMode = s_SortMode;
        }


        // ---------------------------------------------------------
        // 3. MAIN SPLIT CONTENT (Left: Projects, Right: Actions)
        // ---------------------------------------------------------
        float actionsPanelWidth = 195.0f; 
        float contentPadding = 10.0f;
        float contentHeight = windowHeight - ImGui::GetCursorPosY() - contentPadding;
        float projectsPanelWidth = windowWidth - actionsPanelWidth - (contentPadding * 2.0f);

        ImGui::SetCursorPosX(contentPadding);
        
        // --- LEFT PANEL: Projects List ---
        ImGui::PushStyleColor(ImGuiCol_ChildBg, GetWindowBgColor()); 
        ImGui::BeginChild("RecentProjects", ImVec2(projectsPanelWidth, contentHeight), false);

        // Optimized filter: lowercase buffer, filter only once
        std::string filterStr = filterBuffer;
        std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), [](unsigned char c){ return std::tolower(c); });

        // Precompute filtered indices for performance
        std::vector<size_t> filteredIndices;
        if (!filterStr.empty()) {
            for (size_t i = 0; i < m_RecentProjects.size(); ++i) {
                const auto& proj = m_RecentProjects[i];
                std::string name = proj.Name;
                std::string path = proj.Path.string();
                std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c){ return std::tolower(c); });
                std::transform(path.begin(), path.end(), path.begin(), [](unsigned char c){ return std::tolower(c); });
                if (name.find(filterStr) != std::string::npos || path.find(filterStr) != std::string::npos)
                    filteredIndices.push_back(i);
            }
        } else {
            filteredIndices.resize(m_RecentProjects.size());
            std::iota(filteredIndices.begin(), filteredIndices.end(), 0);
        }

        if (filteredIndices.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.0f), "  No recent projects found.");
        } else {
            for (size_t idx = 0; idx < filteredIndices.size(); ++idx) {
                size_t index = filteredIndices[idx];
                if (DrawProjectCard((int)index, m_RecentProjects[index]))
                    LaunchProject(m_RecentProjects[index].Path);
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::SameLine();

        // --- RIGHT PANEL: Action Sidebar ---
        ImGui::PushStyleColor(ImGuiCol_ChildBg, GetPanelBgColor());
        ImGui::BeginChild("ProjectActions", ImVec2(actionsPanelWidth, contentHeight), false);
        
        const bool hasSelection = m_SelectedProjectIndex >= 0 && m_SelectedProjectIndex < (int)m_RecentProjects.size();
        
        if (hasSelection)
        {
            bool exists = std::filesystem::exists(m_RecentProjects[m_SelectedProjectIndex].Path);
            bool canEdit = exists;
            bool canRun = exists;
            bool canRename = exists;
            bool canDuplicate = exists;
            bool canRemove = true;
            bool canRemoveMissing = !exists;

            auto IconButton = [](Ref<Texture2D> icon, const char* label, bool enabled) -> bool {
                if (!enabled) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.4f);
                
                ImVec2 avail = ImGui::GetContentRegionAvail();
                float paddingX = 10.0f; 
                float width = avail.x - (paddingX * 2.0f);
                ImVec2 buttonSize = ImVec2(width, 36.0f); 
                
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + paddingX);
                
                ImVec2 cursor = ImGui::GetCursorScreenPos();
                bool pressed = ImGui::InvisibleButton(label, buttonSize);
                
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                
                ImU32 col = IM_COL32(65, 68, 72, 255); 
                if (ImGui::IsItemHovered() && enabled) col = IM_COL32(85, 88, 92, 255);
                if (ImGui::IsItemActive() && enabled)  col = IM_COL32(45, 48, 52, 255);
                
                float rounding = 6.0f; 
                
                draw_list->AddRectFilled(cursor, ImVec2(cursor.x + buttonSize.x, cursor.y + buttonSize.y), col, rounding);
                draw_list->AddRect(cursor, ImVec2(cursor.x + buttonSize.x, cursor.y + buttonSize.y), IM_COL32(30, 30, 35, 120), rounding, 0, 1.0f);

                float paddingLeft = 14.0f;
                float iconSize = 16.0f;
                
                if (icon) {
                    float iconY = cursor.y + (buttonSize.y - iconSize) * 0.5f;
                    ImVec2 pMin = ImVec2(std::floor(cursor.x + paddingLeft), std::floor(iconY));
                    ImVec2 pMax = ImVec2(pMin.x + iconSize, pMin.y + iconSize);
                    
                    draw_list->AddImage(
                        (ImTextureID)(uint64_t)icon->GetRendererID(), 
                        pMin, pMax,
                        ImVec2(0, 1), ImVec2(1, 0)
                    );
                }
                
                ImVec2 textSize = ImGui::CalcTextSize(label);
                float textX = cursor.x + (buttonSize.x - textSize.x) * 0.5f;
                
                float minTextX = cursor.x + paddingLeft + iconSize + 10.0f;
                if (textX < minTextX) textX = minTextX;

                float textY = cursor.y + (buttonSize.y - textSize.y) * 0.5f - 1.0f;

                draw_list->AddText(ImVec2(std::floor(textX), std::floor(textY)), IM_COL32(220, 225, 230, 255), label);
                
                if (!enabled) ImGui::PopStyleVar();
                return pressed && enabled;
            };

            ImGui::Dummy(ImVec2(0.0f, 8.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8.0f)); 

            if (IconButton(s_IconEdit, "Edit", canEdit)) { /* Edit logic */ }
            if (IconButton(s_IconRun, "Run", canRun)) { /* Run logic */ }
            if (IconButton(s_IconRename, "Rename", canRename)) { /* Rename logic */ }
            if (IconButton(s_IconDuplicate, "Duplicate", canDuplicate)) { /* Duplicate logic */ }
            if (IconButton(s_IconRemove, "Remove", canRemove)) { RequestDeleteSelectedProject(false); }
            if (IconButton(s_IconRemoveMissing, "Remove Missing", canRemoveMissing)) { RequestDeleteSelectedProject(false); }

            ImGui::PopStyleVar();
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Modal logic
        if (!m_PendingProjectPath.empty())
            ImGui::OpenPopup("Confirm Project Removal");

        bool popupOpen = true;
        if (ImGui::BeginPopupModal("Confirm Project Removal", &popupOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (m_DeleteProjectFromDisk)
            {
                ImGui::TextWrapped("Delete this project directory from disk?");
                ImGui::Spacing();
                ImGui::TextWrapped("%s", m_PendingProjectPath.parent_path().string().c_str());
            }
            else
            {
                ImGui::TextWrapped("Remove this project from the recent list?");
                ImGui::Spacing();
                ImGui::TextWrapped("%s", m_PendingProjectPath.string().c_str());
            }

            ImGui::Spacing();
            if (ImGui::Button("Confirm", ImVec2(140.0f, 0.0f)))
            {
                ExecutePendingDelete();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(140.0f, 0.0f)))
            {
                m_PendingProjectPath.clear();
                m_DeleteProjectFromDisk = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void LauncherLayer::LoadSettings()
    {
        m_RecentProjects.clear();
        if (m_SettingsPath.empty() || !std::filesystem::exists(m_SettingsPath))
            return;

        YAML::Node data;
        try
        {
            data = YAML::LoadFile(m_SettingsPath.string());
        }
        catch (const YAML::Exception&)
        {
            SetStatusMessage("Launcher settings could not be read. Starting with an empty recent list.", true);
            return;
        }

        YAML::Node launcherNode = data["Launcher"];
        if (!launcherNode)
            return;

        YAML::Node recentNode = launcherNode["RecentProjects"];
        if (!recentNode || !recentNode.IsSequence())
            return;

        for (const auto& entry : recentNode)
        {
            if (!entry["Path"])
                continue;

            RecentProjectEntry project;
            project.Path = NormalizeProjectPath(entry["Path"].as<std::string>());
            project.Name = ReadProjectName(project.Path);
            try {
                YAML::Node projData = YAML::LoadFile(project.Path.string());
                if (projData["Project"] && projData["Project"]["CreationDate"])
                    project.CreationDate = projData["Project"]["CreationDate"].as<std::string>();
            } catch (...) {}
            m_RecentProjects.push_back(project);
        }
        
        // No longer needed: always sorted in render
    }

    void LauncherLayer::SaveSettings() const
    {
        if (m_SettingsPath.empty())
            return;

        std::error_code ec;
        std::filesystem::create_directories(m_SettingsPath.parent_path(), ec);
        if (ec)
            return;

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Launcher" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "RecentProjects" << YAML::Value << YAML::BeginSeq;
        for (const auto& project : m_RecentProjects)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "Path" << YAML::Value << project.Path.string();
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
        out << YAML::EndMap;

        std::ofstream output(m_SettingsPath);
        output << out.c_str();
    }

    void LauncherLayer::RefreshProjectMetadata()
    {
        for (auto& project : m_RecentProjects)
        {
            project.Name = ReadProjectName(project.Path);
            try {
                YAML::Node projData = YAML::LoadFile(project.Path.string());
                if (projData["Project"] && projData["Project"]["CreationDate"])
                    project.CreationDate = projData["Project"]["CreationDate"].as<std::string>();
            } catch (...) {}
            LoadPreviewForProject(project);
        }

        if (m_SelectedProjectIndex >= (int)m_RecentProjects.size())
            m_SelectedProjectIndex = m_RecentProjects.empty() ? -1 : (int)m_RecentProjects.size() - 1;
            
        // No longer needed: always sorted in render
    }

    void LauncherLayer::AddRecentProject(const std::filesystem::path& projectPath)
    {
        std::filesystem::path normalizedPath = NormalizeProjectPath(projectPath);
        m_RecentProjects.erase(std::remove_if(m_RecentProjects.begin(), m_RecentProjects.end(), [&](const RecentProjectEntry& entry)
        {
            return PathsMatch(entry.Path, normalizedPath);
        }), m_RecentProjects.end());

        RecentProjectEntry entry;
        entry.Path = normalizedPath;
        entry.Name = ReadProjectName(normalizedPath);
        LoadPreviewForProject(entry);
        m_RecentProjects.insert(m_RecentProjects.begin(), entry);

        if (m_RecentProjects.size() > kMaxRecentProjects)
            m_RecentProjects.resize(kMaxRecentProjects);

        m_SelectedProjectIndex = 0; 
        // No longer needed: always sorted in render
        SaveSettings();
    }

    void LauncherLayer::RemoveRecentProject(const std::filesystem::path& projectPath)
    {
        std::filesystem::path normalizedPath = NormalizeProjectPath(projectPath);
        m_RecentProjects.erase(std::remove_if(m_RecentProjects.begin(), m_RecentProjects.end(), [&](const RecentProjectEntry& entry)
        {
            return PathsMatch(entry.Path, normalizedPath);
        }), m_RecentProjects.end());

        if (m_SelectedProjectIndex >= (int)m_RecentProjects.size())
            m_SelectedProjectIndex = m_RecentProjects.empty() ? -1 : (int)m_RecentProjects.size() - 1;

        // No longer needed: always sorted in render
        SaveSettings();
    }

    void LauncherLayer::LaunchProject(const std::filesystem::path& projectPath)
    {
        std::filesystem::path normalizedPath = NormalizeProjectPath(projectPath);
        if (!std::filesystem::exists(normalizedPath))
        {
            SetStatusMessage("The selected project file does not exist anymore.", true);
            return;
        }

        std::filesystem::path editorExecutable = GetEditorExecutablePath();
        if (editorExecutable.empty() || !std::filesystem::exists(editorExecutable))
        {
            SetStatusMessage("UmeEditor.exe was not found next to the launcher executable.", true);
            return;
        }

        AddRecentProject(normalizedPath);
        if (!ProcessUtils::LaunchProcess(editorExecutable, { normalizedPath.string(), "--return-to-launcher" }))
        {
            SetStatusMessage("Failed to launch the editor process.", true);
            return;
        }

        Application::Get().Close();
    }

    void LauncherLayer::OpenExistingProject()
    {
        std::string filepath = FileDialogs::OpenFile("Ume Project (*.uproj)\0*.uproj\0");
        if (filepath.empty())
            return;

        LaunchProject(filepath);
    }

    void LauncherLayer::CreateProject()
    {
        std::string filepath = FileDialogs::SaveFile("Ume Project (*.uproj)\0*.uproj\0");
        if (filepath.empty())
            return;

        ProjectCreationResult result = ProjectUtils::CreateProjectFromTemplate(filepath);
        if (!result.Success)
        {
            SetStatusMessage(result.ErrorMessage, true);
            return;
        }

        SetStatusMessage("Project created successfully.", false);
        LaunchProject(result.ProjectFilePath);
    }

    void LauncherLayer::RequestDeleteSelectedProject(bool deleteFromDisk)
    {
        if (!IsSelectedProjectValid())
            return;

        m_PendingProjectPath = m_RecentProjects[m_SelectedProjectIndex].Path;
        m_DeleteProjectFromDisk = deleteFromDisk;
    }

    void LauncherLayer::ExecutePendingDelete()
    {
        if (m_PendingProjectPath.empty())
            return;

        if (m_DeleteProjectFromDisk)
        {
            std::error_code ec;
            std::filesystem::remove_all(m_PendingProjectPath.parent_path(), ec);
            if (ec)
            {
                SetStatusMessage("Failed to delete project directory from disk.", true);
                m_PendingProjectPath.clear();
                m_DeleteProjectFromDisk = false;
                return;
            }
            SetStatusMessage("Project deleted from disk.", false);
        }
        else
        {
            SetStatusMessage("Project removed from recent list.", false);
        }

        RemoveRecentProject(m_PendingProjectPath);
        m_PendingProjectPath.clear();
        m_DeleteProjectFromDisk = false;
    }

    void LauncherLayer::LoadPreviewForProject(RecentProjectEntry& project)
    {
        const std::filesystem::path previewPath = FindProjectPreviewImage(project.Path);
        if (previewPath.empty())
        {
            project.PreviewPath.clear();
            project.PreviewTexture = nullptr;
            return;
        }

        if (project.PreviewTexture && PathsMatch(project.PreviewPath, previewPath))
            return;

        project.PreviewPath = previewPath;
        project.PreviewTexture = Texture2D::Create(previewPath.string());
    }

    std::filesystem::path LauncherLayer::FindProjectPreviewImage(const std::filesystem::path& projectPath) const
    {
        if (projectPath.empty())
            return {};

        const std::filesystem::path projectDirectory = projectPath.parent_path();
        const std::array<std::filesystem::path, 8> candidates = {
            projectDirectory / "preview.png",
            projectDirectory / "preview.jpg",
            projectDirectory / "screenshot.png",
            projectDirectory / "screenshot.jpg",
            projectDirectory / "thumbnail.png",
            projectDirectory / "thumbnail.jpg",
            projectDirectory / "Assets" / "preview.png",
            projectDirectory / "Assets" / "screenshot.png"
        };

        for (const auto& candidate : candidates)
        {
            if (std::filesystem::exists(candidate))
                return candidate;
        }

        return {};
    }

    bool LauncherLayer::DrawProjectCard(int index, RecentProjectEntry& project)
    {
        const bool selected = index == m_SelectedProjectIndex;
        const bool exists = std::filesystem::exists(project.Path);

        ImGui::PushID(index);
        
        const ImVec2 cardSize = ImVec2(ImGui::GetContentRegionAvail().x, 66.0f); 
        const ImVec2 cardPos = ImGui::GetCursorScreenPos();
        
        ImGui::InvisibleButton("ProjectCard", cardSize);
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        const bool doubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && hovered;

        if (clicked) SelectProject(index);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 cardMin = cardPos;
        const ImVec2 cardMax = ImVec2(cardPos.x + cardSize.x, cardPos.y + cardSize.y);
        
        drawList->AddRectFilled(cardMin, cardMax, hovered ? CardHoverColor(selected) : CardColor(selected), 6.0f);
        
        if (selected)
        {
            drawList->AddRect(cardMin, cardMax, IM_COL32(80, 100, 120, 255), 6.0f, 0, 1.5f);
        }

        const ImVec2 circlePos = ImVec2(cardMin.x + 20.0f, cardMin.y + cardSize.y * 0.5f);
        drawList->AddCircle(circlePos, 6.0f, IM_COL32(100, 100, 100, 255), 0, 1.5f);

        const ImVec2 imageMin = ImVec2(cardMin.x + 40.0f, cardMin.y + 10.0f);
        const ImVec2 imageMax = ImVec2(cardMin.x + 86.0f, cardMax.y - 10.0f);
        
        drawList->AddRectFilled(imageMin, imageMax, IM_COL32(30, 35, 45, 255), 8.0f); 

        if (project.PreviewTexture)
        {
            drawList->AddImage((ImTextureID)(uint64_t)project.PreviewTexture->GetRendererID(), imageMin, imageMax, ImVec2(0, 1), ImVec2(1, 0));
        }
        else if (s_DefaultLogo)
        {
            ImVec2 logoMin = ImVec2(std::floor(imageMin.x + 8.0f), std::floor(imageMin.y + 8.0f));
            ImVec2 logoMax = ImVec2(std::floor(imageMax.x - 8.0f), std::floor(imageMax.y - 8.0f));
            drawList->AddImage((ImTextureID)(uint64_t)s_DefaultLogo->GetRendererID(), logoMin, logoMax, ImVec2(0, 1), ImVec2(1, 0));
        }

        const ImVec2 textBasePos = ImVec2(imageMax.x + 16.0f, cardMin.y + 12.0f);
        
        drawList->AddText(textBasePos, IM_COL32(235, 240, 245, 255), project.Name.c_str());
        
        ImVec2 pathPos = ImVec2(textBasePos.x, textBasePos.y + 24.0f);
        if (s_IconFolder) {
            float iconSize = 12.0f;
            drawList->AddImage((ImTextureID)(uint64_t)s_IconFolder->GetRendererID(), 
                ImVec2(pathPos.x, pathPos.y + 2.0f), 
                ImVec2(pathPos.x + iconSize, pathPos.y + 2.0f + iconSize), 
                ImVec2(0, 1), ImVec2(1, 0));
            pathPos.x += iconSize + 6.0f;
        }
        drawList->AddText(pathPos, IM_COL32(110, 115, 120, 255), project.Path.string().c_str());

        std::string dateStr = project.CreationDate.empty() ? "2026-03-24 20:46:05" : project.CreationDate;
        std::string verStr = "4.6"; 

        ImVec2 dateSize = ImGui::CalcTextSize(dateStr.c_str());
        ImVec2 verSize = ImGui::CalcTextSize(verStr.c_str());

        float rightPadding = 24.0f;
        float dateX = cardMax.x - dateSize.x - rightPadding;
        float verX = dateX - verSize.x - 24.0f;

        float alignY = cardMin.y + (cardSize.y - dateSize.y) * 0.5f;

        drawList->AddText(ImVec2(verX, alignY), IM_COL32(170, 175, 180, 255), verStr.c_str());
        drawList->AddText(ImVec2(dateX, alignY), IM_COL32(140, 145, 150, 255), dateStr.c_str());

        if (!exists)
        {
            drawList->AddText(ImVec2(textBasePos.x + 200.0f, textBasePos.y), IM_COL32(220, 100, 100, 255), "[Missing from disk]");
        }

        ImGui::Spacing();
        ImGui::PopID();
        return doubleClicked && exists;
    }

    std::filesystem::path LauncherLayer::NormalizeProjectPath(const std::filesystem::path& path) const
    {
        std::error_code ec;
        if (std::filesystem::exists(path))
            return std::filesystem::weakly_canonical(path, ec);

        std::filesystem::path absolute = std::filesystem::absolute(path, ec);
        return ec ? path.lexically_normal() : absolute.lexically_normal();
    }

    std::string LauncherLayer::ReadProjectName(const std::filesystem::path& projectPath) const
    {
        if (!std::filesystem::exists(projectPath))
            return projectPath.stem().string();

        try
        {
            YAML::Node data = YAML::LoadFile(projectPath.string());
            if (data["Project"] && data["Project"]["Name"])
                return data["Project"]["Name"].as<std::string>();
        }
        catch (const YAML::Exception&)
        {
        }

        return projectPath.stem().string();
    }

    std::filesystem::path LauncherLayer::GetSettingsPath() const
    {
        std::filesystem::path appDataDirectory = ProcessUtils::GetAppDataDirectory();
        if (appDataDirectory.empty())
            return {};

        return appDataDirectory / "Ume" / "LauncherSettings.yaml";
    }

    std::filesystem::path LauncherLayer::GetEditorExecutablePath() const
    {
        return ProcessUtils::GetExecutablePath().parent_path() / "UmeEditor.exe";
    }

    void LauncherLayer::SelectProject(int index)
    {
        if (index < 0 || index >= (int)m_RecentProjects.size())
            return;

        m_SelectedProjectIndex = index;
    }

    bool LauncherLayer::IsSelectedProjectValid() const
    {
        return m_SelectedProjectIndex >= 0 && m_SelectedProjectIndex < (int)m_RecentProjects.size();
    }

    void LauncherLayer::SetStatusMessage(const std::string& message, bool isError)
    {
        m_StatusMessage = message;
        m_StatusIsError = isError;
    }

}