#pragma once

#include "Core.h"
#include "Window.h"
#include "LayerStack.h"
#include "Events/Event.h"
#include "ImGui/ImGuiLayer.h"
#include "Ume/Core/Timestep.h"
#include "Ume/Renderer/Shader.h"
#include "Ume/Renderer/Buffer.h"
#include "Events/ApplicationEvent.h"
#include "Ume/Renderer/VertexArray.h"
#include "Ume/Renderer/OrthographicCamera.h"

namespace Ume
{
	class UME_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void pushOverlay(Layer* layer);

		inline static Application& Get() { return *s_Instance; }

		inline Window& GetWindow() { return *m_Window;  }
	private:
		bool OnWindowClosed(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
	private:
		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		bool m_Minimized = false;
		LayerStack m_LayerStack;
		float m_LastFrameTime = 0.0f;
		static Application* s_Instance;
	};

	Application* CreateApplication();
}