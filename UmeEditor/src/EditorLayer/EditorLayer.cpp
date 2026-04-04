#include "EditorLayer.h"
#include "Ume/Scene/SceneSerializer.h"
#include "Ume/Project/ProjectUtils.h"
#include "Ume/Utils/PlatformUtils.h"
#include "Ume/Math/Math.h"
#include "Ume/Core/TaskSystem.h"
#include "Ume/Scripting/ScriptEngine.h"
#include "Ume/Renderer/Font.h"
#include "Ume/Asset/VPakReader.h"
#include "Ume/ImGui/ImGuiTextureHelpers.h"
#include "../StartupSplash.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#include "ImGuizmo.h"

namespace Ume {

	static Ref<Font> s_Font;
	SceneHierarchyPanel s_SHP;

	namespace
	{
		const ImVec4 kGodotBlue = ImVec4(0.239f, 0.549f, 0.945f, 1.0f);
		const ImVec4 kToolbarChip = ImVec4(0.186f, 0.190f, 0.198f, 1.0f);
		const ImVec4 kToolbarChipActive = ImVec4(0.239f, 0.549f, 0.945f, 0.22f);
		const ImVec4 kToolbarHover = ImVec4(0.214f, 0.220f, 0.229f, 1.0f);
		constexpr float kToolbarSpacing = 8.0f;
		constexpr float kViewportToolbarHeight = 38.0f;

		bool DrawStripButton(const char* label, bool active, const ImVec2& size = ImVec2(0.0f, 0.0f))
		{
			ImGui::PushStyleColor(ImGuiCol_Button, active ? kToolbarChipActive : kToolbarChip);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kToolbarHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, kGodotBlue);
			const bool pressed = ImGui::Button(label, size);
			ImGui::PopStyleColor(3);
			return pressed;
		}

		bool HasSavedEditorLayout()
		{
				return ImGui::FindWindowSettings(ImHashStr("Editor View")) != nullptr
					|| ImGui::FindWindowSettings(ImHashStr("Game View")) != nullptr
				|| ImGui::FindWindowSettings(ImHashStr("Scene")) != nullptr
				|| ImGui::FindWindowSettings(ImHashStr("FileSystem")) != nullptr
				|| ImGui::FindWindowSettings(ImHashStr("Inspector")) != nullptr
				|| ImGui::FindWindowSettings(ImHashStr("Debugger")) != nullptr
				|| ImGui::FindWindowSettings(ImHashStr("Settings")) != nullptr
				|| ImGui::FindWindowSettings(ImHashStr("Performance")) != nullptr;
		}

		void BuildGodotDockspaceLayout(ImGuiID dockspaceID, ImGuiDockNodeFlags dockspaceFlags, const ImVec2& size)
		{
			ImGui::DockBuilderRemoveNode(dockspaceID);
			ImGui::DockBuilderAddNode(dockspaceID, dockspaceFlags | ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspaceID, size);

			ImGuiID dockMainID = dockspaceID;
			ImGuiID dockLeftID = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Left, 0.19f, nullptr, &dockMainID);
			ImGuiID dockRightID = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Right, 0.20f, nullptr, &dockMainID);
			ImGuiID dockBottomID = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Down, 0.27f, nullptr, &dockMainID);
			ImGuiID dockFileSystemID = ImGui::DockBuilderSplitNode(dockLeftID, ImGuiDir_Down, 0.48f, nullptr, &dockLeftID);

			ImGui::DockBuilderDockWindow("Scene", dockLeftID);
			ImGui::DockBuilderDockWindow("FileSystem", dockFileSystemID);
			ImGui::DockBuilderDockWindow("Inspector", dockRightID);
			ImGui::DockBuilderDockWindow("Debugger", dockBottomID);
			ImGui::DockBuilderDockWindow("Game View", dockMainID);
			ImGui::DockBuilderDockWindow("Editor View", dockMainID);
			ImGui::DockBuilderFinish(dockspaceID);
		}
		
		float SmoothMetric(float previous, float current)
		{
			constexpr float alpha = 0.15f;
			return previous + (current - previous) * alpha;
		}

		struct ScopedTimeCapture
		{
			float& LastValue;
			float& SmoothedValue;
			double Start = 0.0;

			ScopedTimeCapture(float& lastValue, float& smoothedValue)
				: LastValue(lastValue), SmoothedValue(smoothedValue), Start(Time::GetTime())
			{
			}

			~ScopedTimeCapture()
			{
				LastValue = (float)((Time::GetTime() - Start) * 1000.0);
				SmoothedValue = SmoothMetric(SmoothedValue, LastValue);
			}
		};

		const char* GetScriptBuildConfigurationName()
		{
		#if defined(UME_DEBUG)
			return "Debug";
		#elif defined(UME_DIST)
			return "Dist";
		#else
			return "Release";
		#endif
		}

		std::string TrimCompilerOutput(const std::string& output)
		{
			constexpr size_t maxOutputSize = 14000;
			if (output.size() <= maxOutputSize)
				return output;

			return std::string("... (trimmed) ...\n") + output.substr(output.size() - maxOutputSize);
		}

		// Returns only the lines from compiler output that carry real information:
		// error/warning diagnostics, "Build FAILED", and the Error/Warning count summary.
		std::string ExtractCompilerErrors(const std::string& output)
		{
			std::istringstream stream(output);
			std::string line, result;
			while (std::getline(stream, line))
			{
				const bool isError   = line.find(": error ")   != std::string::npos;
				const bool isWarning = line.find(": warning ") != std::string::npos;
				const bool isFailed  = line.find("Build FAILED") != std::string::npos;
				const bool isSummary = line.find("Error(s)")   != std::string::npos
				                    || line.find("Warning(s)") != std::string::npos;
				if (isError || isWarning || isFailed || isSummary)
				{
					if (!result.empty()) result += '\n';
					result += line;
				}
			}
			return result;
		}
	}

	EditorLayer::EditorLayer()
		: Layer("EditorLayer"), m_CameraController(1280.0f / 720.0f), m_SquareColor({ 0.2f, 0.3f, 0.8f, 1.0f })
	{
		s_Font = Font::GetDefault();
	}

	void EditorLayer::LoadEditorPrefs()
	{
		std::ifstream in("editor_prefs.ini");
		if (!in.is_open())
			return;
		std::string line;
		while (std::getline(in, line))
		{
			if (line.rfind("vsync=", 0) == 0)
				m_VSyncEnabled = (line.substr(6) == "1");
		}
	}

	void EditorLayer::SaveEditorPrefs()
	{
		std::ofstream out("editor_prefs.ini");
		if (out.is_open())
			out << "vsync=" << (m_VSyncEnabled ? '1' : '0') << '\n';
	}

	void EditorLayer::OnAttach()
	{
		UME_PROFILE_FUNCTION();
		UME_CORE_INFO("[EditorStartup] OnAttach begin.");
		LoadEditorPrefs();
		Application::Get().GetWindow().SetVSync(m_VSyncEnabled);
		UME_CORE_INFO("[EditorStartup] Window VSync step completed.");

		// Mount editor icon resources from the VPak in the build output folder.
		// Always resolve relative to the executable, not the working directory.
		const auto editorPakDir = ProcessUtils::GetExecutablePath().parent_path() / "Resources" / "Paks";
		UME_CORE_INFO("[EditorStartup] Mounting editor icons from {}", (editorPakDir / "EditorIcons.vpak").string());
		VPakRegistry::Mount("EditorIcons", editorPakDir / "EditorIcons.vpak");
		UME_CORE_INFO("[EditorStartup] Editor icon mount step completed.");

		m_SceneHierarchyPanel.SetScriptBuildCallback([this]() { BeginScriptBuildAndReload(); });
		UME_CORE_INFO("[EditorStartup] Scene hierarchy callback installed.");
		ScriptEngine::SetManagedLogSink([this](const std::string& message, bool isError)
		{
			m_ConsolePanel.AddMessage(message, isError ? glm::vec4{ 1.0f, 0.2f, 0.2f, 1.0f } : glm::vec4{ 0.85f, 0.85f, 0.85f, 1.0f });
		});
		UME_CORE_INFO("[EditorStartup] Managed log sink installed.");

		auto commandLineArgs = Application::Get().GetSpecification().CommandLineArgs;
		for (int index = 1; index < commandLineArgs.Count; index++)
		{
			if (std::strcmp(commandLineArgs[index], "--return-to-launcher") == 0)
				m_ReturnToLauncherOnClose = true;
			else if (commandLineArgs[index][0] != '-' && m_StartupProjectPath.empty())
				m_StartupProjectPath = commandLineArgs[index];
		}

		UME_CORE_INFO("[EditorStartup] Working directory: {}", std::filesystem::current_path().string());
		if (!m_StartupProjectPath.empty())
			UME_CORE_INFO("[EditorStartup] Parsed startup project argument: {}", m_StartupProjectPath.string());
		else
			UME_CORE_INFO("[EditorStartup] No startup project argument provided.");
		UME_CORE_INFO("[EditorStartup] Return to launcher on close: {}", m_ReturnToLauncherOnClose);

	}

	void EditorLayer::OnDetach()
	{
		UME_PROFILE_FUNCTION();
		SaveEditorPrefs();
		ScriptEngine::SetManagedLogSink(nullptr);
		StartupSplash::Stop();
		s_Font = nullptr;
		RelaunchLauncherIfRequested();
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		UME_PROFILE_FUNCTION();
		ScopedTimeCapture updateTimer(m_LastUpdateMs, m_SmoothedUpdateMs);

		if (m_ProjectLoadInProgress)
		{
			AdvanceProjectLoad();
			return;
		}

		if (m_StartupLoadingActive)
			return;

		// Keep UI responsive while scripts compile by avoiding expensive scene update/render work.
		if (m_ScriptBuildInProgress)
			return;


		TryAutoRebuildScripts(ts);

		glm::vec2 sceneViewportSize = m_GameViewportSize;
		if (sceneViewportSize.x <= 0.0f || sceneViewportSize.y <= 0.0f)
			sceneViewportSize = m_EditorViewportSize;

		if (sceneViewportSize.x > 0.0f && sceneViewportSize.y > 0.0f)
			m_ActiveScene->OnViewportResize((uint32_t)sceneViewportSize.x, (uint32_t)sceneViewportSize.y);

		// Resize
		if (FramebufferSpecification spec = m_EditorFramebuffer->GetSpecification();
			m_EditorViewportSize.x > 0.0f && m_EditorViewportSize.y > 0.0f &&
			(spec.Width != m_EditorViewportSize.x || spec.Height != m_EditorViewportSize.y))
		{
			m_EditorFramebuffer->Resize((uint32_t)m_EditorViewportSize.x, (uint32_t)m_EditorViewportSize.y);
			m_CameraController.OnResize(m_EditorViewportSize.x, m_EditorViewportSize.y);
			m_EditorCamera.SetViewportSize(m_EditorViewportSize.x, m_EditorViewportSize.y);
		}

		if (FramebufferSpecification spec = m_GameFramebuffer->GetSpecification();
			m_GameViewportSize.x > 0.0f && m_GameViewportSize.y > 0.0f &&
			(spec.Width != m_GameViewportSize.x || spec.Height != m_GameViewportSize.y))
		{
			m_GameFramebuffer->Resize((uint32_t)m_GameViewportSize.x, (uint32_t)m_GameViewportSize.y);
		}

		// Render
		Renderer2D::ResetStats();
		m_GameViewHasCamera = false;

		if (m_EditorViewportFocused)
			m_CameraController.OnUpdate(ts);
		m_EditorCamera.OnUpdate(ts);

		const bool hasEditorSurface = m_EditorViewportSize.x > 0.0f && m_EditorViewportSize.y > 0.0f;
		const bool hasGameSurface = m_GameViewportSize.x > 0.0f && m_GameViewportSize.y > 0.0f;

		if (m_SceneState == SceneState::Play && hasGameSurface)
		{
			m_GameFramebuffer->Bind();
			m_GameFramebuffer->ClearAttachment(0, { 0.1f, 0.1f, 0.1f, 1.0f });
			m_GameFramebuffer->ClearDepthAttachment();
			m_GameViewHasCamera = (bool)m_ActiveScene->GetPrimaryCameraEntity();
			m_ActiveScene->OnUpdateRuntime(ts);
			m_GameFramebuffer->Unbind();
		}

		if (hasEditorSurface)
		{
			m_EditorFramebuffer->Bind();
			m_EditorFramebuffer->ClearAttachment(0, { 0.1f, 0.1f, 0.1f, 1.0f });
			m_EditorFramebuffer->ClearDepthAttachment();
			m_EditorFramebuffer->ClearAttachment(1, -1);

			switch (m_SceneState)
			{
			case SceneState::Edit:
				m_ActiveScene->OnUpdateEditor(ts, m_EditorCamera);
				break;
			case SceneState::Simulate:
				m_ActiveScene->OnUpdateSimulation(ts, m_EditorCamera);
				break;
			case SceneState::Play:
				if (hasGameSurface)
					m_ActiveScene->RenderScene(m_EditorCamera);
				else
					m_ActiveScene->OnUpdateRuntime(ts);
				break;
			}

			OnOverlayRender();
			m_EditorFramebuffer->Unbind();
		}

		if (hasGameSurface && m_SceneState != SceneState::Play)
		{
			m_GameFramebuffer->Bind();
			m_GameFramebuffer->ClearAttachment(0, { 0.1f, 0.1f, 0.1f, 1.0f });
			m_GameFramebuffer->ClearDepthAttachment();
			m_GameViewHasCamera = m_ActiveScene->RenderPrimaryCamera();
			m_GameFramebuffer->Unbind();
		}

		m_HoveredEntity = {};
		if (hasEditorSurface)
		{
			auto [mx, my] = ImGui::GetMousePos();
			glm::vec2 viewportSize = m_EditorViewportBounds[1] - m_EditorViewportBounds[0];
			const auto& framebufferSpec = m_EditorFramebuffer->GetSpecification();
			mx -= m_EditorViewportBounds[0].x;
			my -= m_EditorViewportBounds[0].y;
			float pixelX = viewportSize.x > 0.0f ? mx * ((float)framebufferSpec.Width / viewportSize.x) : -1.0f;
			float pixelY = viewportSize.y > 0.0f ? my * ((float)framebufferSpec.Height / viewportSize.y) : -1.0f;
			int mouseX = (int)pixelX;
			int mouseY = (int)((float)framebufferSpec.Height - pixelY - 1.0f);

			if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)framebufferSpec.Width && mouseY < (int)framebufferSpec.Height)
			{
				int pixelData = m_EditorFramebuffer->ReadPixel(1, mouseX, mouseY);
				if (pixelData == -1 || !m_ActiveScene->IsEntityHandleValid((entt::entity)pixelData))
					m_HoveredEntity = {};
				else
					m_HoveredEntity = Entity((entt::entity)pixelData, m_ActiveScene.get());
			}
		}
	}

	void EditorLayer::OnImGuiRender()
	{
		UME_PROFILE_FUNCTION();
		ScopedTimeCapture imguiTimer(m_LastImGuiMs, m_SmoothedImGuiMs);
		static bool dockspaceInitialized = false;
		const float uiDelta = std::clamp(ImGui::GetIO().DeltaTime, 0.0f, 0.05f);
		m_StartupPulsePhase += uiDelta * 3.2f;
		m_ScriptBuildPulsePhase += uiDelta * 2.25f;

		if (m_NativeSplashClosed && !m_ProjectLoadInProgress)
		{
			if (m_RequestStartupProjectLoad && !m_StartupProjectPath.empty())
			{
				m_RequestStartupProjectLoad = false;
				UME_CORE_INFO("[EditorStartup] Begin startup project load: {}", m_StartupProjectPath.string());
				BeginProjectLoad(m_StartupProjectPath, true);
			}
			else if (m_RequestStartupProjectPicker)
			{
				m_RequestStartupProjectPicker = false;
				UME_CORE_INFO("[EditorStartup] Startup project picker requested.");
				if (!OpenProject())
				{
					UME_CORE_WARN("[EditorStartup] Startup project picker canceled; closing application.");
					Application::Get().Close();
				}
			}
		}

		if (m_StartupLoadingActive)
		{
			DrawStartupLoadingScreen();

			if (!m_StartupFirstFramePresented)
			{
				m_StartupFirstFramePresented = true;
				m_ProjectLoadStatus = "Starting editor...";
				m_ProjectLoadProgress = 0.02f;
				return;
			}

			if (m_ProjectLoadInProgress)
			{
				if (m_ProjectLoadPhase == ProjectLoadPhase::Complete)
				{
					m_ProjectLoadInProgress = false;
					m_ProjectLoadPhase = ProjectLoadPhase::Idle;
					m_StartupLoadingActive = false;
				}
				else if (m_ProjectLoadPhase == ProjectLoadPhase::Failed)
				{
					m_ProjectLoadInProgress = false;
					m_ProjectLoadPhase = ProjectLoadPhase::Idle;
					if (m_ProjectLoadCloseOnFailure)
						Application::Get().Close();
					else
						m_StartupLoadingActive = false;
				}

				return;
			}

			m_StartupLoadingFrameCount++;
			if (!m_StartupInitializationDone)
			{
				AdvanceStartupInitialization();
				if (m_StartupInitPhase == StartupInitPhase::Complete)
					m_StartupInitializationDone = true;
				return;
			}

			if (m_StartupInitializationDone && m_StartupLoadingFrameCount >= 2)
			{
				StartupSplash::Stop();
				Application::Get().GetWindow().Show();
				m_NativeSplashClosed = true;

				UME_CORE_INFO("[EditorStartup] Startup initialization complete; showing main window.");
				if (!m_StartupProjectPath.empty())
				{
					UME_CORE_INFO("[EditorStartup] Scheduling startup project load: {}", m_StartupProjectPath.string());
					m_RequestStartupProjectLoad = true;
				}
				else
				{
					UME_CORE_INFO("[EditorStartup] No startup project path available; opening project picker.");
					m_RequestStartupProjectPicker = true;
				}

				m_StartupLoadingActive = false;
			}

			return;
		}

		if (m_ProjectLoadInProgress)
		{
			DrawStartupLoadingScreen();

			if (m_ProjectLoadPhase == ProjectLoadPhase::Complete)
			{
				m_ProjectLoadInProgress = false;
				m_ProjectLoadPhase = ProjectLoadPhase::Idle;
			}
			else if (m_ProjectLoadPhase == ProjectLoadPhase::Failed)
			{
				m_ProjectLoadInProgress = false;
				m_ProjectLoadPhase = ProjectLoadPhase::Idle;
			}

			return;
		}

		if (m_ScriptBuildInProgress)
		{
			UI_ScriptBuildOverlay();
			UI_PerformanceHud();
			return;
		}

		static bool dockspaceOpen = true;
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
		ImGui::Begin("##GodotEditorHost", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			const ImGuiID dockspaceID = ImGui::GetID("GodotDockspace");
			if (!dockspaceInitialized)
			{
				dockspaceInitialized = true;
				if (!HasSavedEditorLayout())
					BuildGodotDockspaceLayout(dockspaceID, dockspace_flags, ImGui::GetWindowSize());
			}

			ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Scene"))
			{
				if (ImGui::MenuItem("New Scene", "Ctrl+N"))
					NewScene();

				if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
					SaveScene();

				if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
					SaveSceneAs();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Project"))
			{
				if (ImGui::MenuItem("New Project..."))
					NewProject();

				if (ImGui::MenuItem("Open Project...", "Ctrl+O"))
					OpenProject();

				ImGui::Separator();

				if (ImGui::MenuItem("Exit"))
					Application::Get().Close();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug"))
			{
				if (ImGui::MenuItem("Build Scripts"))
					BeginScriptBuildAndReload();

				if (ImGui::MenuItem("Reload assembly", "Ctrl+R"))
					ScriptEngine::ReloadAssembly();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Editor"))
			{
				ImGui::MenuItem("Performance HUD", nullptr, &m_ShowPerformanceHud);
				ImGui::MenuItem("Scene Dock", nullptr, &m_ShowHierarchyPanel);
				ImGui::MenuItem("FileSystem Dock", nullptr, &m_ShowContentBrowserPanel);
				ImGui::MenuItem("Debugger Dock", nullptr, &m_ShowConsolePanel);
				ImGui::MenuItem("Stats Panel", nullptr, &m_ShowStatsPanel);
				ImGui::MenuItem("Settings Panel", nullptr, &m_ShowSettingsPanel);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				ImGui::TextDisabled("Ume Editor");
				ImGui::Separator();
				ImGui::MenuItem("Godot-style workspace chrome", nullptr, false, false);
				ImGui::EndMenu();
			}

			const char* modes[] = { "2D", "Script", "AssetLib" };
			const float spacing = 6.0f;
			float totalModesWidth = 0.0f;
			for (const char* mode : modes)
				totalModesWidth += ImGui::CalcTextSize(mode).x + 24.0f;
			totalModesWidth += spacing * 2.0f;
			const float centeredX = std::max(ImGui::GetCursorPosX() + 16.0f, (ImGui::GetWindowWidth() - totalModesWidth) * 0.5f);
			ImGui::SetCursorPosX(centeredX);
			for (int index = 0; index < 3; index++)
			{
				if (index > 0)
					ImGui::SameLine(0.0f, spacing);
				DrawStripButton(modes[index], false);
			}

			UI_Toolbar();

			ImGui::EndMenuBar();
		}

		if (m_ShowHierarchyPanel)
			m_SceneHierarchyPanel.OnImGuiRender();
		if (m_ShowContentBrowserPanel && m_ContentBrowserPanel)
			m_ContentBrowserPanel->OnImGuiRender();
		if (m_ShowConsolePanel)
			m_ConsolePanel.OnImGuiRender();
		
		UI_PerformanceHud();

		if (m_ShowStatsPanel)
		{
			ImGui::Begin("Stats", &m_ShowStatsPanel);

			auto stats = Renderer2D::GetStats();
			ImGui::Text("Renderer2D Stats:");
			ImGui::Text("Draw Calls: %d", stats.DrawCalls);
			ImGui::Text("Quads: %d", stats.QuadCount);
			ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
			ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

			ImGui::End();
		}

		if (m_ShowSettingsPanel)
		{
			ImGui::Begin("Settings", &m_ShowSettingsPanel);
			ImGui::Checkbox("Show physics colliders", &m_ShowPhysicsColliders);
			ImGui::Checkbox("Show contact colliders", &m_ShowContactColliders);

			ImGui::Separator();
			ImGui::TextDisabled("Rendering");
			bool vsync = m_VSyncEnabled;
			if (ImGui::Checkbox("VSync", &vsync))
			{
				m_VSyncEnabled = vsync;
				Application::Get().GetWindow().SetVSync(m_VSyncEnabled);
				SaveEditorPrefs();
			}

			ImGui::End();
		}

		DrawEditorViewportPanel();
		DrawGameViewportPanel();
		Application::Get().GetImGuiLayer()->BlockEvents(!(m_EditorViewportHovered || m_GameViewportHovered));
		ImGui::End();

		UI_ScriptBuildOverlay();
	}

	void EditorLayer::AdvanceStartupInitialization()
	{
		switch (m_StartupInitPhase)
		{
		case StartupInitPhase::Idle:
			m_ProjectLoadStatus = "Loading editor resources...";
			m_ProjectLoadProgress = 0.06f;
			m_StartupInitPhase = StartupInitPhase::LoadCheckerboard;
			break;
		case StartupInitPhase::LoadCheckerboard:
			m_ProjectLoadStatus = "Loading textures...";
			m_ProjectLoadProgress = 0.10f;
			m_CheckerboardTexture = Texture2D::Create("assets/textures/Checkerboard.png");
			m_StartupInitPhase = StartupInitPhase::LoadIconPlay;
			break;
		case StartupInitPhase::LoadIconPlay:
			m_ProjectLoadStatus = "Loading toolbar icons...";
			m_ProjectLoadProgress = 0.14f;
			m_IconPlay = Texture2D::Create("Resources/Icons/PlayButton.png");
			m_StartupInitPhase = StartupInitPhase::LoadIconPause;
			break;
		case StartupInitPhase::LoadIconPause:
			m_ProjectLoadStatus = "Loading toolbar icons...";
			m_ProjectLoadProgress = 0.18f;
			m_IconPause = Texture2D::Create("Resources/Icons/PauseButton.png");
			m_StartupInitPhase = StartupInitPhase::LoadIconSimulate;
			break;
		case StartupInitPhase::LoadIconSimulate:
			m_ProjectLoadStatus = "Loading toolbar icons...";
			m_ProjectLoadProgress = 0.22f;
			m_IconSimulate = Texture2D::Create("Resources/Icons/SimulateButton.png");
			m_StartupInitPhase = StartupInitPhase::LoadIconStep;
			break;
		case StartupInitPhase::LoadIconStep:
			m_ProjectLoadStatus = "Loading toolbar icons...";
			m_ProjectLoadProgress = 0.26f;
			m_IconStep = Texture2D::Create("Resources/Icons/StepButton.png");
			m_StartupInitPhase = StartupInitPhase::LoadIconStop;
			break;
		case StartupInitPhase::LoadIconStop:
			m_ProjectLoadStatus = "Loading toolbar icons...";
			m_ProjectLoadProgress = 0.30f;
			m_IconStop = Texture2D::Create("Resources/Icons/StopButton.png");
			m_StartupInitPhase = StartupInitPhase::CreateFramebuffer;
			break;
		case StartupInitPhase::CreateFramebuffer:
		{
			m_ProjectLoadStatus = "Creating render targets...";
			m_ProjectLoadProgress = 0.36f;
			FramebufferSpecification editorFbSpec;
			editorFbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
			editorFbSpec.Width = 1280;
			editorFbSpec.Height = 720;
			m_EditorFramebuffer = Framebuffer::Create(editorFbSpec);

			FramebufferSpecification gameFbSpec;
			gameFbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
			gameFbSpec.Width = 1280;
			gameFbSpec.Height = 720;
			m_GameFramebuffer = Framebuffer::Create(gameFbSpec);
			m_StartupInitPhase = StartupInitPhase::CreateScene;
			break;
		}
		case StartupInitPhase::CreateScene:
			m_ProjectLoadStatus = "Creating editor scene...";
			m_ProjectLoadProgress = 0.42f;
			m_EditorScene = CreateRef<Scene>();
			m_ActiveScene = m_EditorScene;
			m_StartupInitPhase = StartupInitPhase::CreateCamera;
			break;
		case StartupInitPhase::CreateCamera:
			m_ProjectLoadStatus = "Configuring camera...";
			m_ProjectLoadProgress = 0.48f;
			m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);
			Renderer2D::SetLineWidth(1.0f);
			m_StartupInitPhase = StartupInitPhase::Complete;
			break;
		case StartupInitPhase::Complete:
			m_ProjectLoadStatus = "Opening project...";
			m_ProjectLoadProgress = 0.52f;
			break;
		}
	}

	void EditorLayer::DrawStartupLoadingScreen()
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.06f, 0.08f, 1.0f));
		ImGui::Begin("Editor Startup Loading", nullptr,
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoSavedSettings);

		ImGui::PopStyleColor();
		ImGui::PopStyleVar(3);

		const ImVec2 center = ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f, viewport->Pos.y + viewport->Size.y * 0.5f);
		const ImVec2 panelSize = ImVec2(460.0f, 170.0f);
		const ImVec2 panelMin = ImVec2(center.x - panelSize.x * 0.5f, center.y - panelSize.y * 0.5f);
		const ImVec2 panelMax = ImVec2(center.x + panelSize.x * 0.5f, center.y + panelSize.y * 0.5f);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(panelMin, panelMax, IM_COL32(23, 30, 40, 245), 12.0f);
		drawList->AddRect(panelMin, panelMax, IM_COL32(82, 136, 178, 220), 12.0f, 0, 2.0f);

		const char* title = "Loading Ume Editor";
		const char* subtitle = m_ProjectLoadStatus.empty()
			? (m_StartupProjectPath.empty() ? "Preparing project browser..." : "Opening project and restoring editor state...")
			: m_ProjectLoadStatus.c_str();

		drawList->AddText(ImVec2(panelMin.x + 24.0f, panelMin.y + 28.0f), IM_COL32(235, 243, 250, 255), title);
		drawList->AddText(ImVec2(panelMin.x + 24.0f, panelMin.y + 62.0f), IM_COL32(182, 199, 218, 255), subtitle);

		const float barWidth = panelSize.x - 48.0f;
		const float pulse = (std::sin(m_StartupPulsePhase) * 0.5f) + 0.5f;
		const float progress = m_ProjectLoadInProgress ? m_ProjectLoadProgress : (0.25f + 0.55f * pulse);
		const float clampedProgress = std::clamp(progress, 0.05f, 0.98f);
		const float fillWidth = barWidth * clampedProgress;
		const ImVec2 barMin = ImVec2(panelMin.x + 24.0f, panelMax.y - 42.0f);
		const ImVec2 barMax = ImVec2(panelMin.x + 24.0f + barWidth, panelMax.y - 22.0f);
		drawList->AddRectFilled(barMin, barMax, IM_COL32(38, 50, 66, 255), 6.0f);
		drawList->AddRectFilled(barMin, ImVec2(barMin.x + fillWidth, barMax.y), IM_COL32(92, 181, 229, 255), 6.0f);

		if (!m_ProjectLoadFailureReason.empty())
			drawList->AddText(ImVec2(panelMin.x + 24.0f, panelMax.y - 66.0f), IM_COL32(220, 120, 120, 255), m_ProjectLoadFailureReason.c_str());

		ImGui::End();
	}

	void EditorLayer::UI_Toolbar()
	{
		bool toolbarEnabled = (bool)m_ActiveScene && !m_ScriptBuildInProgress;
		const ImVec4 enabledTint = ImVec4(1, 1, 1, 1);
		const ImVec4 disabledTint = ImVec4(1, 1, 1, 0.35f);
		const float iconSize = 22.0f;
		const float imagePadding = 8.0f;
		const float chipSize = iconSize + imagePadding * 2.0f;
		const float chipSpacing = 6.0f;
		const int buttonCount = 4;

		const float transportWidth = (buttonCount * chipSize) + ((buttonCount - 1) * chipSpacing);
		const float totalWidth = transportWidth;
		
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - totalWidth - 12.0f);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(imagePadding, imagePadding));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

		auto drawTransportButton = [&](const char* id, const Ref<Texture2D>& icon, bool active, bool enabled, auto&& onPress)
		{
			ImVec4 tintColor = enabled ? enabledTint : disabledTint;
			ImVec4 buttonColor = enabled
				? (active ? ImVec4(0.239f, 0.549f, 0.945f, 0.26f) : ImVec4(0.180f, 0.188f, 0.208f, 1.0f))
				: ImVec4(0.145f, 0.151f, 0.169f, 1.0f);
			ImVec4 hoverColor = enabled
				? (active ? ImVec4(0.239f, 0.549f, 0.945f, 0.36f) : ImVec4(0.230f, 0.239f, 0.263f, 1.0f))
				: buttonColor;
			ImVec4 activeColor = enabled
				? (active ? ImVec4(0.239f, 0.549f, 0.945f, 0.46f) : ImVec4(0.274f, 0.286f, 0.318f, 1.0f))
				: buttonColor;

			ImGui::PushID(id);
			ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
			if (ImGui::ImageButton(ImGuiTextureHelpers::GetTextureID(icon), ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1), 0, ImVec4(0, 0, 0, 0), tintColor) && toolbarEnabled && enabled)
				onPress();
			ImGui::PopStyleColor(3);
			ImGui::PopID();
		};

		const bool running = m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate;
		const bool paused = running && m_ActiveScene->IsPaused();

		drawTransportButton("Play", m_IconPlay, m_SceneState == SceneState::Play, toolbarEnabled, [&]() {
			if (m_SceneState != SceneState::Play)
				OnScenePlay();
		});

		ImGui::SameLine(0, chipSpacing);
		drawTransportButton("Debug", m_IconSimulate, m_SceneState == SceneState::Simulate, toolbarEnabled, [&]() {
			if (m_SceneState != SceneState::Simulate)
				OnSceneSimulate();
		});

		ImGui::SameLine(0, chipSpacing);
		drawTransportButton("Pause", m_IconPause, paused, running, [&]() {
			m_ActiveScene->SetPaused(!m_ActiveScene->IsPaused());
		});

		ImGui::SameLine(0, chipSpacing);
		drawTransportButton("Stop", m_IconStop, false, running, [&]() {
			OnSceneStop();
		});

		if (paused)
		{
			ImGui::SameLine(0, 10.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.82f, 0.29f, 1.0f));
			ImGui::TextUnformatted("PAUSED");
			ImGui::PopStyleColor();
		}
		
		ImGui::PopStyleVar(3);
	}

	void EditorLayer::DrawGizmoToolbar()
	{
		if (DrawStripButton("Select", m_GizmoType == -1, ImVec2(58.0f, 28.0f)))
			m_GizmoType = -1;
		ImGui::SameLine(0.0f, kToolbarSpacing);
		if (DrawStripButton("Move", m_GizmoType == ImGuizmo::OPERATION::TRANSLATE, ImVec2(58.0f, 28.0f)))
			m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
		ImGui::SameLine(0.0f, kToolbarSpacing);
		if (DrawStripButton("Rotate", m_GizmoType == ImGuizmo::OPERATION::ROTATE, ImVec2(58.0f, 28.0f)))
			m_GizmoType = ImGuizmo::OPERATION::ROTATE;
		ImGui::SameLine(0.0f, kToolbarSpacing);
		if (DrawStripButton("Scale", m_GizmoType == ImGuizmo::OPERATION::SCALE, ImVec2(58.0f, 28.0f)))
			m_GizmoType = ImGuizmo::OPERATION::SCALE;
	}

	void EditorLayer::DrawViewportSurface(Framebuffer* framebuffer, const glm::vec2& size)
	{
		ImGui::Image(ImGuiTextureHelpers::GetFramebufferAttachmentID(framebuffer), ImVec2{ size.x, size.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
	}

	void EditorLayer::DrawEditorViewportPanel()
	{
		if (m_FocusEditorViewNextFrame)
		{
			ImGui::SetNextWindowFocus();
			m_FocusEditorViewNextFrame = false;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Editor View");
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.148f, 0.151f, 0.158f, 1.0f));
		ImGui::BeginChild("##EditorViewToolbar", ImVec2(0.0f, kViewportToolbarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::SetCursorPos(ImVec2(8.0f, 5.0f));
		DrawGizmoToolbar();
		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.643f, 0.663f, 0.706f, 1.0f));
		ImGui::BeginChild("##EditorViewSurface", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		m_EditorViewportFocused = ImGui::IsWindowFocused();
		m_EditorViewportHovered = ImGui::IsWindowHovered();

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		m_EditorViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
		m_EditorViewportBounds[0] = { ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y };
		m_EditorViewportBounds[1] = m_EditorViewportBounds[0] + m_EditorViewportSize;

		if (m_EditorViewportSize.x > 0.0f && m_EditorViewportSize.y > 0.0f)
		{
			DrawViewportSurface(m_EditorFramebuffer.get(), m_EditorViewportSize);
			m_EditorViewportBounds[0] = { ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y };
			m_EditorViewportBounds[1] = { ImGui::GetItemRectMax().x, ImGui::GetItemRectMax().y };
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;
				OpenScene(path);
			}
			ImGui::EndDragDropTarget();
		}

		Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
		if (selectedEntity && m_GizmoType != -1)
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(m_EditorViewportBounds[0].x, m_EditorViewportBounds[0].y, m_EditorViewportBounds[1].x - m_EditorViewportBounds[0].x, m_EditorViewportBounds[1].y - m_EditorViewportBounds[0].y);

			auto& tc = selectedEntity.GetComponent<TransformComponent>();
			glm::mat4 transform = m_ActiveScene->GetWorldTransform(selectedEntity);

			bool snap = Input::IsKeyPressed(Key::LeftControl);
			float snapValue = m_GizmoType == ImGuizmo::OPERATION::ROTATE ? 45.0f : 0.5f;
			float snapValues[3] = { snapValue, snapValue, snapValue };

			glm::mat4 cameraProjection = m_EditorCamera.GetProjection();
			glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();
			ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
				(ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform), nullptr, snap ? snapValues : nullptr);

			if (ImGuizmo::IsUsing())
			{
				glm::mat4 localTransform = transform;
				if (selectedEntity.HasComponent<RelationshipComponent>())
				{
					auto& relationship = selectedEntity.GetComponent<RelationshipComponent>();
					if ((uint64_t)relationship.Parent != 0)
					{
						Entity parentEntity = m_ActiveScene->GetEntityByUUID(relationship.Parent);
						if (parentEntity)
						{
							glm::mat4 parentWorldTransform = m_ActiveScene->GetWorldTransform(parentEntity);
							localTransform = glm::inverse(parentWorldTransform) * transform;
						}
					}
				}

				glm::vec3 translation, rotation, scale;
				if (Math::DecomposeTransform(localTransform, translation, rotation, scale))
				{
					tc.Translation = translation;
					tc.Rotation = rotation;
					tc.Scale = scale;
				}
			}
		}

		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void EditorLayer::DrawGameViewportPanel()
	{
		if (m_FocusGameViewNextFrame)
		{
			ImGui::SetNextWindowFocus();
			m_FocusGameViewNextFrame = false;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Game View");
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.050f, 0.055f, 0.062f, 1.0f));
		ImGui::BeginChild("##GameViewSurface", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		m_GameViewportFocused = ImGui::IsWindowFocused();
		m_GameViewportHovered = ImGui::IsWindowHovered();
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		m_GameViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		if (m_GameViewportSize.x > 0.0f && m_GameViewportSize.y > 0.0f)
			DrawViewportSurface(m_GameFramebuffer.get(), m_GameViewportSize);

		if (!m_GameViewHasCamera)
		{
			const char* message = "No primary camera";
			ImVec2 textSize = ImGui::CalcTextSize(message);
			ImVec2 avail = ImGui::GetContentRegionAvail();
			ImGui::SetCursorPos(ImVec2((avail.x - textSize.x) * 0.5f, (avail.y - textSize.y) * 0.5f));
			ImGui::TextDisabled("%s", message);
		}

		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void EditorLayer::UI_ScriptBuildOverlay()
	{
		if (m_ScriptBuildInProgress)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			const ImVec2 center = viewport->GetCenter();

			ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowBgAlpha(0.95f);

			constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
				| ImGuiWindowFlags_AlwaysAutoResize
				| ImGuiWindowFlags_NoDocking
				| ImGuiWindowFlags_NoSavedSettings
				| ImGuiWindowFlags_NoMove;

			ImGui::Begin("##ScriptBuildProgressOverlay", nullptr, flags);
			ImGui::TextUnformatted("Compiling C# scripts...");
			ImGui::Spacing();

			const float pulse = (std::sinf(m_ScriptBuildPulsePhase) + 1.0f) * 0.5f;
			const float progress = 0.15f + pulse * 0.75f;
			ImGui::ProgressBar(progress, ImVec2(360.0f, 0.0f), "Building");

			ImGui::TextDisabled("Play starts automatically after a successful compile.");
			ImGui::End();
		}

		if (m_OpenScriptBuildFailedPopup)
		{
			// Log to console instead of showing popup
			m_ConsolePanel.AddMessage("=== SCRIPT COMPILATION FAILED ===", {1.0f, 0.2f, 0.2f, 1.0f});
			m_ConsolePanel.AddMessage(m_LastScriptBuildOutput.empty() ? "No compiler output was captured." : m_LastScriptBuildOutput, {1.0f, 0.8f, 0.2f, 1.0f});
			m_ConsolePanel.AddMessage("Play was cancelled. Fix compilation errors and try again.", {1.0f, 0.2f, 0.2f, 1.0f});
			m_OpenScriptBuildFailedPopup = false;
		}
	}

	void EditorLayer::UI_PerformanceHud()
	{
		if (!m_ShowPerformanceHud)
			return;

		ImGui::SetNextWindowBgAlpha(0.85f);
		if (ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			const float fps = ImGui::GetIO().Framerate;
			const float frameMs = m_SmoothedUpdateMs + m_SmoothedImGuiMs;

			ImGui::Text("FPS: %.1f", fps);
			ImGui::Text("Frame (CPU): %.2f ms", frameMs);
			ImGui::Separator();
			ImGui::Text("Update: %.2f ms", m_SmoothedUpdateMs);
			ImGui::Text("ImGui: %.2f ms", m_SmoothedImGuiMs);
			ImGui::Text("Script Build Active: %s", m_ScriptBuildInProgress ? "Yes" : "No");
			ImGui::Text("Script Scan Task: %s", m_ScriptScanTaskActive ? "Running" : "Idle");
		}
		ImGui::End();
	}

	void EditorLayer::OnEvent(Event& e)
	{
		m_CameraController.OnEvent(e);
		if (m_SceneState == SceneState::Edit)
		{
			m_EditorCamera.OnEvent(e);
		}

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(UME_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(UME_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		// Shortcuts
		if (e.IsRepeat())
			return false;

		bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
		bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

		switch (e.GetKeyCode())
		{
		case Key::N:
		{
			if (control)
				NewScene();

			break;
		}
		case Key::O:
		{
			if (control)
				OpenProject();

			break;
		}
		case Key::S:
		{
			if (control)
			{
				if (shift)
					SaveSceneAs();
				else
					SaveScene();
			}

			break;
		}

		// Scene Commands
		case Key::D:
		{
			if (control)
				OnDuplicateEntity();

			break;
		}

		// Gizmos
		}

		switch (e.GetKeyCode())
		{
		case Key::Q:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = -1;
			break;
		}
		case Key::T:
		{
			if (!ImGuizmo::IsUsing())
				m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
			break;
		}
		case Key::R:
		{
			if (control)
			{
				ScriptEngine::ReloadAssembly();
			}
			else
			{
				if (!ImGuizmo::IsUsing())
					m_GizmoType = ImGuizmo::OPERATION::ROTATE;
			}
			break;
		}
		case Key::S:
		{
			
			if (!ImGuizmo::IsUsing())
				m_GizmoType = ImGuizmo::OPERATION::SCALE;
			break;
		}
		case Key::Delete:
		{
			if (Application::Get().GetImGuiLayer()->GetActiveWidgetID() == 0)
			{
				Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
				if (selectedEntity)
				{
					m_SceneHierarchyPanel.SetSelectedEntity({});
					m_ActiveScene->DestroyEntity(selectedEntity);
				}
			}
			break;
		}
		}

		return false;
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (e.GetMouseButton() == Mouse::ButtonLeft)
		{
			if (m_EditorViewportHovered && !Input::IsKeyPressed(Key::LeftAlt))
			{
				const bool interactingWithGizmo = ImGuizmo::IsUsing() || ImGuizmo::IsOver();
				if (!interactingWithGizmo)
				{
					m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity);
					if (m_HoveredEntity)
						m_GizmoType = (m_GizmoType == -1) ? ImGuizmo::OPERATION::TRANSLATE : m_GizmoType;
				}
			}
		}
		return false;
	}

	void EditorLayer::OnOverlayRender()
	{
		Renderer2D::BeginScene(m_EditorCamera);

		if (m_ShowPhysicsColliders)
		{
			// Box Colliders
			{
				auto view = m_ActiveScene->GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();
				for (auto entity : view)
				{
					auto [tc, bc2d] = view.get<TransformComponent, BoxCollider2DComponent>(entity);

					glm::mat4 worldTransform = m_ActiveScene->GetWorldTransform(Entity { entity, m_ActiveScene.get() });
					glm::mat4 transform = worldTransform
						* glm::translate(glm::mat4(1.0f), glm::vec3(bc2d.Offset, 0.001f))
						* glm::scale(glm::mat4(1.0f), glm::vec3(bc2d.Size * 2.0f, 1.0f));

					Renderer2D::DrawRect(transform, glm::vec4(0, 1, 0, 1));
				}
			}

			// Circle Colliders
			{
				auto view = m_ActiveScene->GetAllEntitiesWith<TransformComponent, CircleCollider2DComponent>();
				for (auto entity : view)
				{
					auto [tc, cc2d] = view.get<TransformComponent, CircleCollider2DComponent>(entity);

					glm::mat4 worldTransform = m_ActiveScene->GetWorldTransform(Entity { entity, m_ActiveScene.get() });
					glm::mat4 transform = worldTransform
						* glm::translate(glm::mat4(1.0f), glm::vec3(cc2d.Offset, 0.001f))
						* glm::scale(glm::mat4(1.0f), glm::vec3(cc2d.Radius * 2.0f));

					Renderer2D::DrawCircle(transform, glm::vec4(0, 1, 0, 1), 0.01f);
				}
			}
		}

		if (m_ShowContactColliders)
		{
			// Contact Colliders
			{
				auto view = m_ActiveScene->GetAllEntitiesWith<TransformComponent, ContactListenerComponent>();
				for (auto entity : view)
				{
					auto [tc, bc2d] = view.get<TransformComponent, ContactListenerComponent>(entity);

					glm::mat4 worldTransform = m_ActiveScene->GetWorldTransform(Entity { entity, m_ActiveScene.get() });
					glm::mat4 transform = worldTransform
						* glm::translate(glm::mat4(1.0f), glm::vec3(bc2d.Offset, 0.001f))
						* glm::scale(glm::mat4(1.0f), glm::vec3(bc2d.Size * 2.0f, 1.0f));

					Renderer2D::DrawRect(transform, glm::vec4(0, 1, 0, 1));
				}
			}
		}

		// Draw selected entity outline 
		if (Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity())
		{
			Renderer2D::DrawRect(m_ActiveScene->GetWorldTransform(selectedEntity), glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));
		}

		Renderer2D::EndScene();
	}

	void EditorLayer::NewProject()
	{
		std::string filepath = FileDialogs::SaveFile("Ume Project (*.uproj)\0*.uproj\0");
		if (filepath.empty())
			return;

		ProjectCreationResult result = ProjectUtils::CreateProjectFromTemplate(filepath);
		if (!result.Success)
		{
			UME_CORE_ERROR("[EditorLayer] {}", result.ErrorMessage);
			return;
		}

		OpenProject(result.ProjectFilePath);
	}

	void EditorLayer::OpenProject(const std::filesystem::path& path)
	{
		BeginProjectLoad(path, false);
	}

	bool EditorLayer::OpenProject()
	{
		if (m_ProjectLoadInProgress)
			return false;

		std::string filepath = FileDialogs::OpenFile("Ume Project (*.uproj)\0*.uproj\0");
		if (filepath.empty())
			return false;

		OpenProject(filepath);
		return true;
	}

	void EditorLayer::BeginProjectLoad(const std::filesystem::path& path, bool closeOnFailure)
	{
		UME_CORE_INFO("[EditorStartup] BeginProjectLoad({}, closeOnFailure={})", path.string(), closeOnFailure);
		m_ProjectLoadTargetPath = path;
		m_ProjectLoadStartScenePath.clear();
		m_ProjectLoadCloseOnFailure = closeOnFailure;
		m_ProjectLoadFailureReason.clear();
		m_ProjectLoadInProgress = true;
		m_ProjectLoadPhase = ProjectLoadPhase::StopScene;
		m_ProjectLoadStatus = "Preparing project load...";
		m_ProjectLoadProgress = 0.05f;
	}

	void EditorLayer::AdvanceProjectLoad()
	{
		switch (m_ProjectLoadPhase)
		{
		case ProjectLoadPhase::StopScene:
		{
			m_ProjectLoadStatus = "Stopping running scene...";
			m_ProjectLoadProgress = 0.10f;
			if (m_SceneState != SceneState::Edit)
				OnSceneStop();
			m_ProjectLoadPhase = ProjectLoadPhase::LoadProject;
			break;
		}
		case ProjectLoadPhase::LoadProject:
		{
			m_ProjectLoadStatus = "Reading project file...";
			m_ProjectLoadProgress = 0.30f;
			UME_CORE_INFO("[EditorStartup] Loading project file: {}", m_ProjectLoadTargetPath.string());
			if (!Project::Load(m_ProjectLoadTargetPath))
			{
				UME_CORE_ERROR("[EditorStartup] Project::Load failed for {}", m_ProjectLoadTargetPath.string());
				m_ProjectLoadFailureReason = "Failed to load project file.";
				m_ProjectLoadStatus = "Project load failed";
				m_ProjectLoadPhase = ProjectLoadPhase::Failed;
				break;
			}

			m_ProjectPath = m_ProjectLoadTargetPath;
			UME_CORE_INFO("[EditorStartup] Project::Load succeeded: {}", m_ProjectPath.string());
			const auto& project = Project::GetActive();
			UME_CORE_WARN("=== PROJECT OPENED ===");
			UME_CORE_WARN("Project Name: {}", project->GetConfig().Name);
			UME_CORE_WARN("Project Directory: {}", Project::GetProjectDirectory().string());
			UME_CORE_WARN("Asset Directory: {}", Project::GetAssetDirectory().string());
			UME_CORE_WARN("Script Module Path: {}", project->GetConfig().ScriptModulePath.string());
			UME_CORE_WARN("Resolved Script Assembly: {}", Project::GetScriptModuleFilePath().string());
			UME_CORE_WARN("======================");

			m_ProjectLoadPhase = ProjectLoadPhase::InitScripting;
			break;
		}
		case ProjectLoadPhase::InitScripting:
		{
			m_ProjectLoadStatus = "Initializing scripts...";
			m_ProjectLoadProgress = 0.50f;
			if (!ScriptEngine::IsInitialized())
				ScriptEngine::Init();
			else
				ScriptEngine::ReloadAssembly();

			ResetScriptSourceTracking();
			m_ProjectLoadPhase = ProjectLoadPhase::LoadStartScene;
			break;
		}
		case ProjectLoadPhase::LoadStartScene:
		{
			m_ProjectLoadStatus = "Loading start scene...";
			m_ProjectLoadProgress = 0.72f;
			const auto& startScene = Project::GetActive()->GetConfig().StartScene;
			if (!startScene.empty())
			{
				m_ProjectLoadStartScenePath = Project::GetAssetFileSystemPath(startScene);
				OpenScene(m_ProjectLoadStartScenePath);
			}
			else
			{
				NewScene();
			}

			m_ProjectLoadPhase = ProjectLoadPhase::CreateContentBrowser;
			break;
		}
		case ProjectLoadPhase::CreateContentBrowser:
		{
			m_ProjectLoadStatus = "Preparing content browser...";
			m_ProjectLoadProgress = 0.88f;
			m_ContentBrowserPanel = CreateScope<ContentBrowserPanel>();
			m_ProjectLoadPhase = ProjectLoadPhase::Complete;
			break;
		}
		case ProjectLoadPhase::Complete:
		{
			m_ProjectLoadStatus = "Project ready";
			m_ProjectLoadProgress = 1.0f;
			break;
		}
		case ProjectLoadPhase::Failed:
		case ProjectLoadPhase::Idle:
		default:
			break;
		}
	}

	void EditorLayer::SaveProject()
	{
		if (Project::GetActive() && !m_ProjectPath.empty())
			Project::SaveActive(m_ProjectPath);
	}

	void EditorLayer::NewScene()
	{
		m_ActiveScene = CreateRef<Scene>();
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);

		m_EditorScenePath = std::filesystem::path();
	}

	void EditorLayer::OpenScene()
	{
		std::string filepath = FileDialogs::OpenFile("Ume Scene (*.ume)\0*.ume\0");
		if (!filepath.empty())
			OpenScene(filepath);
	}

	void EditorLayer::OpenScene(const std::filesystem::path& path)
	{
		if (m_SceneState != SceneState::Edit)
			OnSceneStop();

		if (path.extension().string() != ".ume")
		{
			UME_WARN("Could not load {0} - not a scene file", path.filename().string());
			return;
		}

		Ref<Scene> newScene = CreateRef<Scene>();
		SceneSerializer serializer(newScene);
		if (serializer.Deserialize(path.string()))
		{
			m_EditorScene = newScene;
			m_SceneHierarchyPanel.SetContext(m_EditorScene);

			m_ActiveScene = m_EditorScene;
			m_EditorScenePath = path;
			UpdateProjectStartScene(m_EditorScenePath);
		}
	}

	void EditorLayer::SaveScene()
	{
		if (!m_EditorScenePath.empty())
		{
			SerializeScene(m_ActiveScene, m_EditorScenePath);
			UpdateProjectStartScene(m_EditorScenePath);
		}
		else
			SaveSceneAs();
	}

	void EditorLayer::SaveSceneAs()
	{
		std::string filepath = FileDialogs::SaveFile("Ume Scene (*.ume)\0*.ume\0");
		if (!filepath.empty())
		{
			SerializeScene(m_ActiveScene, filepath);
			m_EditorScenePath = filepath;
			UpdateProjectStartScene(m_EditorScenePath);
		}
	}

	void EditorLayer::SerializeScene(Ref<Scene> scene, const std::filesystem::path& path)
	{
		SceneSerializer serializer(scene);
		serializer.Serialize(path.string());
	}

	void EditorLayer::RelaunchLauncherIfRequested()
	{
		if (!m_ReturnToLauncherOnClose)
			return;

		std::filesystem::path launcherPath = ProcessUtils::GetExecutablePath().parent_path() / "UmeLauncher.exe";
		if (!std::filesystem::exists(launcherPath))
			return;

		ProcessUtils::LaunchProcess(launcherPath, {});
	}

	void EditorLayer::UpdateProjectStartScene(const std::filesystem::path& scenePath)
	{
		if (!Project::GetActive() || m_ProjectPath.empty() || scenePath.empty())
			return;

		std::filesystem::path assetDirectory = Project::GetAssetDirectory().lexically_normal();
		std::filesystem::path normalizedScenePath = scenePath.lexically_normal();
		std::filesystem::path relativeScenePath = normalizedScenePath.lexically_relative(assetDirectory);
		if (relativeScenePath.empty())
			return;

		auto relativeIt = relativeScenePath.begin();
		if (relativeIt != relativeScenePath.end() && relativeIt->string() == "..")
			return;

		auto& config = Project::GetActive()->GetConfig();
		if (config.StartScene != relativeScenePath)
		{
			config.StartScene = relativeScenePath;
			SaveProject();
		}
	}

	std::filesystem::path EditorLayer::ResolveScriptProjectFile() const
	{
		if (!Project::GetActive())
			return {};

		const auto& config = Project::GetActive()->GetConfig();
		if (config.ScriptModulePath.empty())
			return {};

		std::filesystem::path scriptAssemblyPath = Project::GetScriptModuleFilePath();
		const std::filesystem::path scriptProjectName = scriptAssemblyPath.stem().string() + ".csproj";

		std::filesystem::path scriptsDirectory = scriptAssemblyPath.parent_path();
		if (scriptsDirectory.filename() == "Binaries")
			scriptsDirectory = scriptsDirectory.parent_path();

		if (scriptsDirectory.empty() || !std::filesystem::exists(scriptsDirectory))
		{
			UME_CORE_ERROR("[EditorLayer] Script directory does not exist or is empty: {}", scriptsDirectory.string());
			return {};
		}

		// First try exact project name match
		const std::filesystem::path expectedProjectFile = scriptsDirectory / scriptProjectName;
		if (std::filesystem::exists(expectedProjectFile))
			return expectedProjectFile;

		// Otherwise find any .csproj in the directory
		for (const auto& entry : std::filesystem::directory_iterator(scriptsDirectory))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".csproj")
				return entry.path();
		}

		UME_CORE_ERROR("[EditorLayer] No script project (.csproj) found in: {}", scriptsDirectory.string());
		return {};
	}

	std::filesystem::path EditorLayer::ResolveScriptSourceDirectory() const
	{
		if (!Project::GetActive())
			return {};

		const auto& config = Project::GetActive()->GetConfig();
		if (config.ScriptModulePath.empty())
			return {};

		std::filesystem::path scriptAssemblyPath = Project::GetScriptModuleFilePath();
		std::filesystem::path scriptsDirectory = scriptAssemblyPath.parent_path();
		if (scriptsDirectory.filename() == "Binaries")
			scriptsDirectory = scriptsDirectory.parent_path();

		if (scriptsDirectory.empty() || !std::filesystem::exists(scriptsDirectory))
			return {};

		const std::filesystem::path sourceDirectory = scriptsDirectory / "Source";
		if (std::filesystem::exists(sourceDirectory) && std::filesystem::is_directory(sourceDirectory))
			return sourceDirectory;

		return scriptsDirectory;
	}

	bool EditorLayer::GetLatestScriptSourceWriteTime(std::filesystem::file_time_type& outWriteTime) const
	{
		const std::filesystem::path sourceDirectory = ResolveScriptSourceDirectory();
		if (sourceDirectory.empty())
			return false;

		std::error_code ec;
		std::filesystem::file_time_type latestWriteTime{};
		bool foundCsFile = false;

		for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDirectory, ec))
		{
			if (ec)
				break;

			if (!entry.is_regular_file())
				continue;

			if (entry.path().extension() != ".cs")
				continue;

			const auto writeTime = entry.last_write_time(ec);
			if (ec)
				continue;

			if (!foundCsFile || writeTime > latestWriteTime)
			{
				latestWriteTime = writeTime;
				foundCsFile = true;
			}
		}

		if (!foundCsFile)
			return false;

		outWriteTime = latestWriteTime;
		return true;
	}

	void EditorLayer::ResetScriptSourceTracking()
	{
		m_HasKnownScriptSourceWriteTime = GetLatestScriptSourceWriteTime(m_KnownScriptSourceWriteTime);
		m_ScriptSourceScanCooldown = 0.0f;
	}

	void EditorLayer::TryAutoRebuildScripts(Timestep ts)
	{
		if (m_SceneState != SceneState::Edit)
			return;

		if (m_ScriptBuildInProgress)
			return;

		if (m_ScriptScanTaskActive)
		{
			if (m_ScriptScanFuture.valid() && m_ScriptScanFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
			{
				ScriptScanResult result = m_ScriptScanFuture.get();
				m_ScriptScanTaskActive = false;

				if (!result.Found)
				{
					m_HasKnownScriptSourceWriteTime = false;
					return;
				}

				if (!m_HasKnownScriptSourceWriteTime)
				{
					m_KnownScriptSourceWriteTime = result.LatestWriteTime;
					m_HasKnownScriptSourceWriteTime = true;
					return;
				}

				if (result.LatestWriteTime > m_KnownScriptSourceWriteTime)
				{
					m_KnownScriptSourceWriteTime = result.LatestWriteTime;
					BeginScriptBuildAndReload();
				}
			}
		}

		m_ScriptSourceScanCooldown -= ts;
		if (m_ScriptSourceScanCooldown > 0.0f || m_ScriptScanTaskActive)
			return;

		m_ScriptSourceScanCooldown = 1.0f;

		const std::filesystem::path sourceDirectory = ResolveScriptSourceDirectory();
		if (sourceDirectory.empty())
		{
			m_HasKnownScriptSourceWriteTime = false;
			return;
		}

		m_ScriptScanTaskActive = true;
		m_ScriptScanFuture = TaskSystem::Get().Submit([sourceDirectory]() -> ScriptScanResult
		{
			ScriptScanResult result;
			std::error_code ec;
			std::filesystem::file_time_type latestWriteTime{};
			bool foundCsFile = false;

			for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDirectory, ec))
			{
				if (ec)
					break;

				if (!entry.is_regular_file())
					continue;

				if (entry.path().extension() != ".cs")
					continue;

				const auto writeTime = entry.last_write_time(ec);
				if (ec)
					continue;

				if (!foundCsFile || writeTime > latestWriteTime)
				{
					latestWriteTime = writeTime;
					foundCsFile = true;
				}
			}

			result.Found = foundCsFile;
			if (foundCsFile)
				result.LatestWriteTime = latestWriteTime;

			return result;
		});
	}

	EditorLayer::ScriptBuildResult EditorLayer::BuildScriptsForActiveProject() const
	{
		ScriptBuildResult result;

		const std::filesystem::path scriptProjectFile = ResolveScriptProjectFile();
		if (scriptProjectFile.empty())
		{
			result.Output = "Could not locate a script project (*.csproj) in the configured script directory.";
			return result;
		}

		auto runCommand = [](const std::string& command, int& exitCode) -> std::string
		{
			std::string output;
			FILE* pipe = _popen(command.c_str(), "r");
			if (!pipe)
			{
				exitCode = -1;
				return "Failed to start compiler process.";
			}

			char buffer[512];
			while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
				output += buffer;

			exitCode = _pclose(pipe);
			return output;
		};

		const std::string configuration = GetScriptBuildConfigurationName();

		std::filesystem::path scriptCoreAssemblyPath;

		auto refreshScriptCoreAssembly = [&](std::string& output) -> bool
		{
			static bool s_HasScriptCoreInputTimestamp = false;
			static std::filesystem::file_time_type s_LastScriptCoreInputTimestamp{};

			std::error_code ec;
			std::filesystem::path searchRoot = std::filesystem::current_path(ec);
			if (ec)
				return true;

			std::filesystem::path scriptCoreProjectFile;
			for (std::filesystem::path probe = searchRoot; !probe.empty(); probe = probe.parent_path())
			{
				const std::filesystem::path candidate = probe / "Ume-ScriptCore" / "Ume-ScriptCore.csproj";
				if (std::filesystem::exists(candidate))
				{
					scriptCoreProjectFile = candidate;
					break;
				}

				if (probe == probe.root_path())
					break;
			}

			// In binary-only distributions there may be no ScriptCore project, so skip this step.
			if (scriptCoreProjectFile.empty())
				return true;

			auto computeScriptCoreInputTimestamp = [&](const std::filesystem::path& scriptCoreRoot, std::filesystem::file_time_type& outTimestamp) -> bool
			{
				std::error_code timestampError;
				bool hasTimestamp = false;
				std::filesystem::file_time_type latestTimestamp{};

				auto considerPath = [&](const std::filesystem::path& filePath)
				{
					const auto writeTime = std::filesystem::last_write_time(filePath, timestampError);
					if (timestampError)
					{
						timestampError.clear();
						return;
					}

					if (!hasTimestamp || writeTime > latestTimestamp)
					{
						latestTimestamp = writeTime;
						hasTimestamp = true;
					}
				};

				considerPath(scriptCoreProjectFile);

				for (const auto& entry : std::filesystem::recursive_directory_iterator(scriptCoreRoot, timestampError))
				{
					if (timestampError)
						break;

					if (!entry.is_regular_file())
						continue;

					const std::filesystem::path extension = entry.path().extension();
					if (extension != ".cs" && extension != ".csproj" && extension != ".props" && extension != ".targets")
						continue;

					considerPath(entry.path());
				}

				if (!hasTimestamp)
					return false;

				outTimestamp = latestTimestamp;
				return true;
			};

			std::filesystem::file_time_type scriptCoreInputTimestamp{};
			const std::filesystem::path scriptCoreRoot = scriptCoreProjectFile.parent_path();
			const bool hasScriptCoreInputTimestamp = computeScriptCoreInputTimestamp(scriptCoreRoot, scriptCoreInputTimestamp);

			const std::filesystem::path preferredAssembly = scriptCoreProjectFile.parent_path().parent_path() / "UmeEditor" / "Resources" / "Scripts" / "Ume-ScriptCore.dll";
			const std::filesystem::path runtimeAssembly = std::filesystem::current_path() / "Resources" / "Scripts" / "Ume-ScriptCore.dll";

			if (hasScriptCoreInputTimestamp && s_HasScriptCoreInputTimestamp && scriptCoreInputTimestamp <= s_LastScriptCoreInputTimestamp)
			{
				if (std::filesystem::exists(preferredAssembly))
				{
					scriptCoreAssemblyPath = preferredAssembly;
					return true;
				}

				if (std::filesystem::exists(runtimeAssembly))
				{
					scriptCoreAssemblyPath = runtimeAssembly;
					return true;
				}
			}

			int exitCode = -1;
			const std::string command = "dotnet build \"" + scriptCoreProjectFile.string() + "\" -c "
				+ configuration + " -p:Platform=x64 -nologo -v minimal 2>&1";
			std::string buildOutput = runCommand(command, exitCode);

			if (exitCode == 0)
			{
				if (std::filesystem::exists(preferredAssembly))
				{
					scriptCoreAssemblyPath = preferredAssembly;

					std::error_code copyError;
					std::filesystem::create_directories(runtimeAssembly.parent_path(), copyError);
					if (copyError)
					{
						output = "Failed to prepare runtime ScriptCore directory: " + runtimeAssembly.parent_path().string() + " (" + copyError.message() + ")";
						return false;
					}

					copyError.clear();
					std::filesystem::copy_file(preferredAssembly, runtimeAssembly, std::filesystem::copy_options::overwrite_existing, copyError);
					if (copyError)
					{
						UME_CORE_WARN("[EditorLayer] Failed to sync Ume-ScriptCore.dll to runtime path '{}': {}", runtimeAssembly.string(), copyError.message());
					}

					const std::filesystem::path preferredPdb = preferredAssembly.parent_path() / "Ume-ScriptCore.pdb";
					if (std::filesystem::exists(preferredPdb))
					{
						const std::filesystem::path runtimePdb = runtimeAssembly.parent_path() / "Ume-ScriptCore.pdb";
						copyError.clear();
						std::filesystem::copy_file(preferredPdb, runtimePdb, std::filesystem::copy_options::overwrite_existing, copyError);
					}
				}
				else if (std::filesystem::exists(runtimeAssembly))
				{
					scriptCoreAssemblyPath = runtimeAssembly;
				}
				else
					scriptCoreAssemblyPath = preferredAssembly;

				if (hasScriptCoreInputTimestamp)
				{
					s_LastScriptCoreInputTimestamp = scriptCoreInputTimestamp;
					s_HasScriptCoreInputTimestamp = true;
				}

				return true;
			}

			output = "Failed to build Ume-ScriptCore required for script compilation: " + scriptCoreProjectFile.string() + "\n\n" + buildOutput;
			return false;
		};

		auto ensureScriptCoreReference = [&](const std::filesystem::path& projectFile) -> std::string
		{
			std::ifstream projectStream(projectFile);
			if (!projectStream.is_open())
				return "Failed to open script project file: " + projectFile.string();

			std::string projectText;
			std::string line;
			while (std::getline(projectStream, line))
			{
				projectText += line;
				projectText.push_back('\n');
			}

			constexpr const char* projectReferenceToken = "<ProjectReference Include=\"";
			size_t searchPos = 0;
			bool hasCoreProjectReference = false;
			while (true)
			{
				searchPos = projectText.find(projectReferenceToken, searchPos);
				if (searchPos == std::string::npos)
					break;

				size_t pathStart = searchPos + std::strlen(projectReferenceToken);
				size_t pathEnd = projectText.find('"', pathStart);
				if (pathEnd == std::string::npos)
					break;

				const std::string includePath = projectText.substr(pathStart, pathEnd - pathStart);
				if (includePath.find("Ume-ScriptCore.csproj") != std::string::npos)
					hasCoreProjectReference = true;

				searchPos = pathEnd + 1;
			}

			if (!hasCoreProjectReference)
			{
				const std::filesystem::path propsFile = projectFile.parent_path() / "Directory.Build.props";
				if (std::filesystem::exists(propsFile))
				{
					std::ifstream propsStream(propsFile);
					if (propsStream.is_open())
					{
						std::string propsText;
						std::string propsLine;
						while (std::getline(propsStream, propsLine))
						{
							propsText += propsLine;
							propsText.push_back('\n');
						}

						if (propsText.find("Auto-generated by UmeEditor to allow portable script builds") != std::string::npos)
						{
							std::error_code removeError;
							std::filesystem::remove(propsFile, removeError);
							if (removeError)
								UME_CORE_WARN("[EditorLayer] Failed to remove stale fallback props file '{}': {}", propsFile.string(), removeError.message());
						}
					}
				}

				return {};
			}

			const std::filesystem::path localScriptCoreDll = projectFile.parent_path() / "Binaries" / "Ume-ScriptCore.dll";

			std::filesystem::path sourceScriptCoreDll = scriptCoreAssemblyPath;
			if (sourceScriptCoreDll.empty())
			{
				const std::filesystem::path fallbackRuntimeDll = std::filesystem::current_path() / "Resources" / "Scripts" / "Ume-ScriptCore.dll";
				if (std::filesystem::exists(fallbackRuntimeDll))
					sourceScriptCoreDll = fallbackRuntimeDll;
			}

			if (sourceScriptCoreDll.empty() || !std::filesystem::exists(sourceScriptCoreDll))
				return "Script project references Ume-ScriptCore.csproj, but that path is missing and no fallback Ume-ScriptCore.dll was found.";

			std::error_code ec;
			std::filesystem::create_directories(localScriptCoreDll.parent_path(), ec);
			if (ec)
				return "Failed to prepare script fallback binary directory: " + localScriptCoreDll.parent_path().string() + " (" + ec.message() + ")";

			if (std::filesystem::exists(sourceScriptCoreDll))
			{
				std::error_code copyError;
				std::filesystem::copy_file(sourceScriptCoreDll, localScriptCoreDll, std::filesystem::copy_options::overwrite_existing, copyError);
				if (copyError)
					return "Failed to sync Ume-ScriptCore.dll into script project: " + localScriptCoreDll.string() + " (" + copyError.message() + ")";

				const std::filesystem::path editorScriptCorePdb = sourceScriptCoreDll.parent_path() / "Ume-ScriptCore.pdb";
				const std::filesystem::path localScriptCorePdb = localScriptCoreDll.parent_path() / "Ume-ScriptCore.pdb";
				if (std::filesystem::exists(editorScriptCorePdb))
				{
					copyError.clear();
					std::filesystem::copy_file(editorScriptCorePdb, localScriptCorePdb, std::filesystem::copy_options::overwrite_existing, copyError);
				}
			}

			// For old-style (non-SDK) projects, Directory.Build.props is imported BEFORE the project's
			// own <ItemGroup> declarations, so <ProjectReference Remove> there has no effect on items
			// declared later. Directory.Build.targets is imported AFTER all items, so Remove works there.
			const bool isSdkStyle = projectText.find("<Project Sdk=") != std::string::npos ||
			                        projectText.find("Sdk=\"Microsoft.NET.Sdk") != std::string::npos;

			if (isSdkStyle)
			{
				const std::filesystem::path propsFile = projectFile.parent_path() / "Directory.Build.props";
				std::ofstream propsStream(propsFile, std::ios::trunc);
				if (!propsStream.is_open())
					return "Failed to write script build fallback props file: " + propsFile.string();

				propsStream
					<< "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
					<< "<Project>\n"
					<< "  <ItemGroup>\n"
					<< "    <!-- Auto-generated by UmeEditor to avoid rebuilding ScriptCore during game script compile -->\n"
					<< "    <ProjectReference Remove=\"**\\Ume-ScriptCore.csproj\" />\n"
					<< "    <Reference Include=\"Ume-ScriptCore\">\n"
					<< "      <HintPath>$(MSBuildThisFileDirectory)Binaries\\Ume-ScriptCore.dll</HintPath>\n"
					<< "      <Private>false</Private>\n"
					<< "    </Reference>\n"
					<< "  </ItemGroup>\n"
					<< "</Project>\n";
			}
			else
			{
				// Old-style project: write reference fix into Directory.Build.targets so the Remove
				// runs after the project's own items are declared. Also include the Compile wildcard
				// for Source/*.cs if that directory exists (same as ensureScriptSourceIncludes does).
				const std::filesystem::path targetsFile = projectFile.parent_path() / "Directory.Build.targets";
				std::ofstream targetsStream(targetsFile, std::ios::trunc);
				if (!targetsStream.is_open())
					return "Failed to write script build fallback targets file: " + targetsFile.string();

				const bool hasSourceDir = std::filesystem::exists(projectFile.parent_path() / "Source") &&
				                          std::filesystem::is_directory(projectFile.parent_path() / "Source");

				targetsStream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
				              << "<Project>\n";

				if (hasSourceDir)
				{
					targetsStream
						<< "  <ItemGroup>\n"
						<< "    <!-- Auto-generated by UmeEditor to keep old-style script projects in sync with Source/*.cs files -->\n"
						<< "    <Compile Include=\"**\\*.cs\" Exclude=\"@(Compile);Binaries\\**\\*;Intermediates\\**\\*;bin\\**\\*;obj\\**\\*\" />\n"
						<< "  </ItemGroup>\n";
				}

				targetsStream
					<< "  <ItemGroup>\n"
					<< "    <!-- Auto-generated by UmeEditor to avoid rebuilding ScriptCore during game script compile -->\n"
					<< "    <ProjectReference Remove=\"**\\Ume-ScriptCore.csproj\" />\n"
					<< "    <Reference Include=\"Ume-ScriptCore\">\n"
					<< "      <HintPath>$(MSBuildThisFileDirectory)Binaries\\Ume-ScriptCore.dll</HintPath>\n"
					<< "      <Private>false</Private>\n"
					<< "    </Reference>\n"
					<< "  </ItemGroup>\n"
					<< "</Project>\n";

				// Clear any stale props file that has an ineffective Remove for old-style projects.
				const std::filesystem::path propsFile = projectFile.parent_path() / "Directory.Build.props";
				if (std::filesystem::exists(propsFile))
				{
					std::ifstream propsIn(propsFile);
					std::string propsContent;
					if (propsIn.is_open())
					{
						std::string line;
						while (std::getline(propsIn, line)) { propsContent += line; propsContent += '\n'; }
					}
					if (propsContent.find("Auto-generated by UmeEditor to avoid rebuilding ScriptCore") != std::string::npos)
					{
						std::error_code removeErr;
						std::filesystem::remove(propsFile, removeErr);
					}
				}
			}

			return {};
		};

		auto ensureScriptSourceIncludes = [&](const std::filesystem::path& projectFile) -> std::string
		{
			std::ifstream projectStream(projectFile);
			if (!projectStream.is_open())
				return "Failed to open script project file: " + projectFile.string();

			std::string projectText;
			std::string line;
			while (std::getline(projectStream, line))
			{
				projectText += line;
				projectText.push_back('\n');
			}

			if (projectText.find("<Project Sdk=") != std::string::npos || projectText.find("Sdk=\"Microsoft.NET.Sdk") != std::string::npos)
				return {};

			const std::filesystem::path sourceDirectory = projectFile.parent_path() / "Source";
			if (!std::filesystem::exists(sourceDirectory) || !std::filesystem::is_directory(sourceDirectory))
				return {};

			const std::filesystem::path targetsFile = projectFile.parent_path() / "Directory.Build.targets";
			std::ofstream targetsStream(targetsFile, std::ios::trunc);
			if (!targetsStream.is_open())
				return "Failed to write script source include targets file: " + targetsFile.string();

			targetsStream
				<< "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
				<< "<Project>\n"
				<< "  <ItemGroup>\n"
				<< "    <!-- Auto-generated by UmeEditor to keep old-style script projects in sync with Source/*.cs files -->\n"
				<< "    <Compile Include=\"**\\*.cs\" Exclude=\"@(Compile);Binaries\\**\\*;Intermediates\\**\\*;bin\\**\\*;obj\\**\\*\" />\n"
				<< "  </ItemGroup>\n"
				<< "</Project>\n";

			return {};
		};

		auto syncBuiltAssemblyToActiveProject = [&](const std::filesystem::path& builtProjectFile) -> std::string
		{
			if (!Project::GetActive())
				return {};

			const auto& config = Project::GetActive()->GetConfig();
			if (config.ScriptModulePath.empty())
				return {};

			const std::filesystem::path configuredAssemblyPath = Project::GetScriptModuleFilePath();
			if (configuredAssemblyPath.empty())
				return {};

			std::filesystem::path builtAssemblyPath = builtProjectFile.parent_path() / "Binaries" / configuredAssemblyPath.filename();
			if (!std::filesystem::exists(builtAssemblyPath))
				builtAssemblyPath = builtProjectFile.parent_path() / "Binaries" / (builtProjectFile.stem().string() + ".dll");

			if (!std::filesystem::exists(builtAssemblyPath))
				return "Script build succeeded, but built assembly was not found at: " + builtAssemblyPath.string();

			std::error_code ec;
			std::filesystem::create_directories(configuredAssemblyPath.parent_path(), ec);
			if (ec)
				return "Failed to prepare script output directory: " + configuredAssemblyPath.parent_path().string() + " (" + ec.message() + ")";

			auto normalizePath = [](const std::filesystem::path& path)
			{
				std::error_code normalizeError;
				const std::filesystem::path absolutePath = std::filesystem::absolute(path, normalizeError);
				if (normalizeError)
					return path.lexically_normal();

				const std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(absolutePath, normalizeError);
				if (normalizeError)
					return absolutePath.lexically_normal();

				return canonicalPath.lexically_normal();
			};

			if (normalizePath(builtAssemblyPath) != normalizePath(configuredAssemblyPath))
			{
				std::filesystem::copy_file(builtAssemblyPath, configuredAssemblyPath, std::filesystem::copy_options::overwrite_existing, ec);
				if (ec)
					return "Failed to sync built script assembly to active project path: " + configuredAssemblyPath.string() + " (" + ec.message() + ")";

				const std::filesystem::path builtPdbPath = builtAssemblyPath.parent_path() / (builtAssemblyPath.stem().string() + ".pdb");
				if (std::filesystem::exists(builtPdbPath))
				{
					const std::filesystem::path configuredPdbPath = configuredAssemblyPath.parent_path() / (configuredAssemblyPath.stem().string() + ".pdb");
					ec.clear();
					std::filesystem::copy_file(builtPdbPath, configuredPdbPath, std::filesystem::copy_options::overwrite_existing, ec);
				}
			}

			return {};
		};

		// Build to a temp output dir so the final Sandbox.dll is not locked during compile.
		// After a successful build, the caller unloads the old assembly and copies temp -> final.
		const std::filesystem::path tempBuildDir = scriptProjectFile.parent_path() / "Intermediates" / "TempBuild";
		std::string tempBuildDirArg = tempBuildDir.string();
		if (!tempBuildDirArg.empty() && (tempBuildDirArg.back() == '\\' || tempBuildDirArg.back() == '/'))
			tempBuildDirArg.pop_back();

		std::filesystem::path builtTempAssemblyPath;

		auto buildScriptProject = [&](const std::filesystem::path& projectFile, std::string& output) -> bool
		{
			const std::string scriptProject = projectFile.string();

			if (!refreshScriptCoreAssembly(output))
				return false;

			const std::string sourceIncludeError = ensureScriptSourceIncludes(projectFile);
			if (!sourceIncludeError.empty())
			{
				output = sourceIncludeError;
				return false;
			}

			const std::string referenceError = ensureScriptCoreReference(projectFile);
			if (!referenceError.empty())
			{
				output = referenceError;
				return false;
			}

			int msbuildExitCode = -1;
			const std::string msbuildCommand = "msbuild \"" + scriptProject + "\" /m:2 /nologo /verbosity:minimal /p:Configuration="
				+ configuration + " /p:Platform=x64 /p:BuildProjectReferences=false /p:OutputPath=\"" + tempBuildDirArg + "\" 2>&1";
			std::string msbuildOutput = runCommand(msbuildCommand, msbuildExitCode);

			if (msbuildExitCode == 0)
			{
				if (msbuildOutput.find("CS2008: No source files specified.") != std::string::npos)
				{
					output = msbuildOutput + "\n\nScript build produced an empty assembly because no .cs source files were compiled.";
					return false;
				}

				output = "Built script project: " + scriptProject + "\n" + msbuildOutput;
				return true;
			}

			int dotnetExitCode = -1;
			const std::string dotnetCommand = "dotnet build \"" + scriptProject + "\" -m:2 -c " + configuration
				+ " -p:Platform=x64 -p:BuildProjectReferences=false -p:OutputPath=\"" + tempBuildDirArg + "\" -nologo -v minimal 2>&1";
			std::string dotnetOutput = runCommand(dotnetCommand, dotnetExitCode);

			if (dotnetExitCode == 0)
			{
				if (dotnetOutput.find("CS2008: No source files specified.") != std::string::npos)
				{
					output = dotnetOutput + "\n\nScript build produced an empty assembly because no .cs source files were compiled.";
					return false;
				}

				output = "Built script project: " + scriptProject + "\n" + dotnetOutput;
				return true;
			}

			// Show only the meaningful diagnostic lines so the console isn't flooded with
			// restore/timing noise. Fall back to the raw output if nothing was extracted.
			std::string filtered = ExtractCompilerErrors(dotnetOutput);
			output = filtered.empty()
				? ("msbuild output:\n" + msbuildOutput + "\ndotnet output:\n" + dotnetOutput)
				: filtered;
			return false;
		};

		std::string primaryBuildOutput;
		if (buildScriptProject(scriptProjectFile, primaryBuildOutput))
		{
			// Determine which DLL was built in the temp directory
			const std::filesystem::path configuredFinalPath = Project::GetScriptModuleFilePath();
			std::filesystem::path tempDll = tempBuildDir / configuredFinalPath.filename();
			if (!std::filesystem::exists(tempDll))
				tempDll = tempBuildDir / (scriptProjectFile.stem().string() + ".dll");

			result.Success = true;
			result.Output = TrimCompilerOutput(primaryBuildOutput);
			result.TempAssemblyPath = std::filesystem::exists(tempDll) ? tempDll : std::filesystem::path{};
			result.FinalAssemblyPath = configuredFinalPath;
			return result;
		}
		result.Success = false;
		result.Output = TrimCompilerOutput("Failed script project: " + scriptProjectFile.string() + "\n\n" + primaryBuildOutput);
		return result;
	}

	void EditorLayer::BeginScriptBuild(bool startRuntimeOnSuccess)
	{
		if (m_ScriptBuildInProgress)
			return;

		m_ScriptBuildInProgress = true;
		m_LastScriptBuildOutput.clear();

		std::thread([this, startRuntimeOnSuccess]()
		{
			ScriptBuildResult buildResult = BuildScriptsForActiveProject();

			Application::Get().SubmitToMainThread([this, buildResult = std::move(buildResult), startRuntimeOnSuccess]() mutable
			{
				m_ScriptBuildInProgress = false;

				if (buildResult.Success)
				{
					// Unload first so the final assembly file is no longer locked by Mono/FileWatch,
					// then copy the temp build into place before reloading.
					ScriptEngine::UnloadAppAssembly();

					if (!buildResult.TempAssemblyPath.empty() && !buildResult.FinalAssemblyPath.empty())
					{
						std::error_code ec;
						std::filesystem::create_directories(buildResult.FinalAssemblyPath.parent_path(), ec);
						if (!ec)
						{
							std::filesystem::copy_file(buildResult.TempAssemblyPath, buildResult.FinalAssemblyPath,
								std::filesystem::copy_options::overwrite_existing, ec);
							if (ec)
								UME_CORE_WARN("[EditorLayer] Failed to deploy built script assembly: {}", ec.message());
							else
							{
								// Also copy .pdb if present
								auto tempPdb = buildResult.TempAssemblyPath;
								tempPdb.replace_extension(".pdb");
								if (std::filesystem::exists(tempPdb))
								{
									auto finalPdb = buildResult.FinalAssemblyPath;
									finalPdb.replace_extension(".pdb");
									ec.clear();
									std::filesystem::copy_file(tempPdb, finalPdb,
										std::filesystem::copy_options::overwrite_existing, ec);
								}
							}
						}
					}

					ScriptEngine::ReloadAssembly();
					if (startRuntimeOnSuccess)
						StartRuntimePlay();
					return;
				}

				m_LastScriptBuildOutput = std::move(buildResult.Output);
				m_OpenScriptBuildFailedPopup = true;
			});
		}).detach();
	}

	void EditorLayer::BeginScriptBuildAndReload()
	{
		BeginScriptBuild(false);
	}

	void EditorLayer::BeginScriptBuildAndPlay()
	{
		BeginScriptBuild(true);
	}

	void EditorLayer::StartRuntimePlay()
	{
		m_SceneState = SceneState::Play;

		m_ActiveScene = Scene::Copy(m_EditorScene);
		m_ActiveScene->OnRuntimeStart();

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
		m_FocusGameViewNextFrame = true;
	}

	void EditorLayer::OnScenePlay()
	{
		if (m_SceneState == SceneState::Simulate)
			OnSceneStop();

		// Pack project textures into a VPak so the runtime can load them efficiently.
		if (Project::GetActive() && !m_ProjectPath.empty())
		{
			const std::filesystem::path toolPath =
				ProcessUtils::GetExecutablePath().parent_path() / "UmeVPackTool.exe";

			if (std::filesystem::exists(toolPath))
			{
				const std::filesystem::path outputDir = Project::GetRuntimeVPackDirectory();
				const int exitCode = ProcessUtils::LaunchProcessSync(toolPath,
					{ "--project", m_ProjectPath.string(),
					  "--output",  outputDir.string() });

				if (exitCode == 0)
				{
					const std::string projectName = Project::GetActive()->GetConfig().Name;
					VPakRegistry::Mount(projectName, outputDir / "Textures.vpak");
				}
				else
				{
					UME_CORE_WARN("VPak packing exited with code {} — project textures may not be up-to-date.", exitCode);
				}
			}
			else
			{
				UME_CORE_WARN("UmeVPackTool not found at '{}' — skipping texture pack.", toolPath.string());
			}
		}

		BeginScriptBuildAndPlay();
	}

	void EditorLayer::OnSceneSimulate()
	{
		if (m_SceneState == SceneState::Play)
			OnSceneStop();

		m_SceneState = SceneState::Simulate;

		m_ActiveScene = Scene::Copy(m_EditorScene);
		m_ActiveScene->OnSimulationStart();

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
		m_FocusEditorViewNextFrame = true;
	}

	void EditorLayer::OnSceneStop()
	{
		UME_CORE_ASSERT(m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate);

		if (m_SceneState == SceneState::Play)
			m_ActiveScene->OnRuntimeStop();
		else if (m_SceneState == SceneState::Simulate)
			m_ActiveScene->OnSimulationStop();

		m_SceneState = SceneState::Edit;

		m_ActiveScene = m_EditorScene;

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
		m_FocusEditorViewNextFrame = true;
	}

	void EditorLayer::OnScenePause()
	{
		if (m_SceneState == SceneState::Edit)
			return;

		m_ActiveScene->SetPaused(true);
	}

	void EditorLayer::OnDuplicateEntity()
	{
		if (m_SceneState != SceneState::Edit)
			return;

		Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
		if (selectedEntity)
		{
			Entity newEntity = m_EditorScene->DuplicateEntity(selectedEntity);
			m_SceneHierarchyPanel.SetSelectedEntity(newEntity);
		}
	}

}





