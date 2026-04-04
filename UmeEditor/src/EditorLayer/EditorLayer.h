#pragma once

#include "Ume.h"
#include "../Panels/SceneHierarchyPanel.h"
#include "../Panels/ContentBrowserPanel.h"
#include "../Panels/ConsolePanel.h"

#include "Ume/Renderer/EditorCamera.h"

#include <future>

namespace Ume {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Event& e) override;

		bool m_ShowContactColliders = false;
	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void OnOverlayRender();

		void NewProject();
		bool OpenProject();
		void OpenProject(const std::filesystem::path& path);
		void SaveProject();

		void NewScene();
		void OpenScene();
		void OpenScene(const std::filesystem::path& path);
		void SaveScene();
		void SaveSceneAs();

		void SerializeScene(Ref<Scene> scene, const std::filesystem::path& path);
		void UpdateProjectStartScene(const std::filesystem::path& scenePath);
		void RelaunchLauncherIfRequested();
		void DrawStartupLoadingScreen();
		void AdvanceStartupInitialization();
		void BeginProjectLoad(const std::filesystem::path& path, bool closeOnFailure);
		void AdvanceProjectLoad();

		struct ScriptBuildResult
		{
			bool Success = false;
			std::string Output;
			std::filesystem::path TempAssemblyPath;
			std::filesystem::path FinalAssemblyPath;
		};

		std::filesystem::path ResolveScriptProjectFile() const;
		std::filesystem::path ResolveScriptSourceDirectory() const;
		bool GetLatestScriptSourceWriteTime(std::filesystem::file_time_type& outWriteTime) const;
		void ResetScriptSourceTracking();
		void TryAutoRebuildScripts(Timestep ts);
		ScriptBuildResult BuildScriptsForActiveProject() const;
		void BeginScriptBuild(bool startRuntimeOnSuccess);
		void BeginScriptBuildAndReload();
		void BeginScriptBuildAndPlay();
		void StartRuntimePlay();

		void OnScenePlay();
		void OnSceneSimulate();
		void OnSceneStop();
		void OnScenePause();

		void OnDuplicateEntity();
		void RenderEditorView();
		void RenderGameView();
		void DrawEditorViewportPanel();
		void DrawGameViewportPanel();
		void DrawViewportSurface(Framebuffer* framebuffer, const glm::vec2& size);
		void DrawViewportToolbar();
		void DrawGizmoToolbar();

		// UI Panels
		void UI_Toolbar();
		void UI_ScriptBuildOverlay();
		void UI_PerformanceHud();
	private:
		Ume::OrthographicCameraController m_CameraController;

		// Temp
		Ref<VertexArray> m_SquareVA;
		Ref<Shader> m_FlatColorShader;
		Ref<Framebuffer> m_EditorFramebuffer;
		Ref<Framebuffer> m_GameFramebuffer;

		Ref<Scene> m_ActiveScene;
		Ref<Scene> m_EditorScene;
		std::filesystem::path m_ProjectPath;
		std::filesystem::path m_EditorScenePath;
		Entity m_SquareEntity;
		Entity m_CameraEntity;
		Entity m_SecondCamera;

		Entity m_HoveredEntity;

		bool m_PrimaryCamera = true;

		EditorCamera m_EditorCamera;

		Ref<Texture2D> m_CheckerboardTexture;

		bool m_EditorViewportFocused = false, m_EditorViewportHovered = false;
		bool m_GameViewportFocused = false, m_GameViewportHovered = false;
		bool m_GameViewHasCamera = false;
		bool m_FocusGameViewNextFrame = false;
		bool m_FocusEditorViewNextFrame = false;
		glm::vec2 m_EditorViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_GameViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_EditorViewportBounds[2];

		glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.8f, 1.0f };

		int m_GizmoType = -1;
		bool m_ReturnToLauncherOnClose = false;

		bool m_ShowPhysicsColliders = false;
		bool m_StartupLoadingActive = true;
		bool m_StartupInitializationDone = false;
		int m_StartupLoadingFrameCount = 0;
		std::filesystem::path m_StartupProjectPath;

		enum class SceneState
		{
			Edit = 0, Play = 1, Simulate = 2
		};

		enum class ProjectLoadPhase
		{
			Idle = 0,
			StopScene,
			LoadProject,
			InitScripting,
			LoadStartScene,
			CreateContentBrowser,
			Complete,
			Failed
		};

		enum class StartupInitPhase
		{
			Idle = 0,
			LoadCheckerboard,
			LoadIconPlay,
			LoadIconPause,
			LoadIconSimulate,
			LoadIconStep,
			LoadIconStop,
			CreateFramebuffer,
			CreateScene,
			CreateCamera,
			Complete
		};

		SceneState m_SceneState = SceneState::Edit;
		StartupInitPhase m_StartupInitPhase = StartupInitPhase::Idle;
		bool m_StartupFirstFramePresented = false;
		bool m_NativeSplashClosed = false;
		int m_ReadyFrameCount = 0;
		bool m_RequestStartupProjectLoad = false;
		bool m_RequestStartupProjectPicker = false;
		ProjectLoadPhase m_ProjectLoadPhase = ProjectLoadPhase::Idle;
		bool m_ProjectLoadInProgress = false;
		bool m_ProjectLoadCloseOnFailure = false;
		std::filesystem::path m_ProjectLoadTargetPath;
		std::filesystem::path m_ProjectLoadStartScenePath;
		std::string m_ProjectLoadStatus = "Initializing...";
		std::string m_ProjectLoadFailureReason;
		float m_ProjectLoadProgress = 0.0f;

		// Editor preferences (persisted to editor_prefs.ini)
		bool m_VSyncEnabled = true;

		void LoadEditorPrefs();
		void SaveEditorPrefs();

		// Panel visibility toggles
		bool m_ShowStatsPanel = true;
		bool m_ShowSettingsPanel = true;
		bool m_ShowContentBrowserPanel = true;
		bool m_ShowConsolePanel = true;
		bool m_ShowHierarchyPanel = true;

		// Panels
		SceneHierarchyPanel m_SceneHierarchyPanel;
		Scope<ContentBrowserPanel> m_ContentBrowserPanel;
		ConsolePanel m_ConsolePanel;

		// Editor resources
		Ref<Texture2D> m_IconPlay, m_IconPause, m_IconStep, m_IconSimulate, m_IconStop;

		bool m_ScriptBuildInProgress = false;
		bool m_OpenScriptBuildFailedPopup = false;
		std::string m_LastScriptBuildOutput;
		float m_ScriptSourceScanCooldown = 0.0f;
		bool m_HasKnownScriptSourceWriteTime = false;
		std::filesystem::file_time_type m_KnownScriptSourceWriteTime{};

		struct ScriptScanResult
		{
			bool Found = false;
			std::filesystem::file_time_type LatestWriteTime{};
		};

		bool m_ScriptScanTaskActive = false;
		std::future<ScriptScanResult> m_ScriptScanFuture;

		bool m_ShowPerformanceHud = true;
		float m_LastUpdateMs = 0.0f;
		float m_LastImGuiMs = 0.0f;
		float m_SmoothedUpdateMs = 0.0f;
		float m_SmoothedImGuiMs = 0.0f;
		float m_StartupPulsePhase = 0.0f;
		float m_ScriptBuildPulsePhase = 0.0f;
	};

}
