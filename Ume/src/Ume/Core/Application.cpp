#include "umepch.h"
#include "Ume/Core/Application.h"

#include "Ume/Core/Log.h"

#include "Ume/Renderer/Renderer.h"
#include "Ume/Renderer/Font.h"
#include "Ume/Scripting/ScriptEngine.h"
#include "Platform/Vulkan/VulkanContext.h"

#include "Ume/Core/Input.h"
#include "Ume/Utils/PlatformUtils.h"

#include <algorithm>

namespace Ume {

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification)
	{
		UME_PROFILE_FUNCTION();

		UME_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		if (!m_Specification.WorkingDirectory.empty())
			std::filesystem::current_path(m_Specification.WorkingDirectory);

		m_Window = Window::Create(WindowProps(m_Specification.Name, m_Specification.Width, m_Specification.Height, !m_Specification.StartHidden));
		m_Window->SetEventCallback(UME_BIND_EVENT_FN(Application::OnEvent));

		Renderer::Init();

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}

	Application::~Application()
	{
		UME_PROFILE_FUNCTION();

		if (Renderer::GetAPI() == RendererAPI::API::Vulkan)
		{
			if (auto* context = VulkanContext::Get())
				vkDeviceWaitIdle(context->GetDevice());
		}

		m_LayerStack.Clear();
		m_ImGuiLayer = nullptr;
		Font::ReleaseDefault();

		ScriptEngine::Shutdown();
		Renderer::Shutdown();
	}

	void Application::PushLayer(Layer* layer)
	{
		UME_PROFILE_FUNCTION();
		UME_CORE_INFO("[Application] PushLayer begin.");

		m_LayerStack.PushLayer(layer);
		UME_CORE_INFO("[Application] PushLayer stored layer.");
		layer->OnAttach();
		UME_CORE_INFO("[Application] PushLayer OnAttach finished.");
	}

	void Application::PushOverlay(Layer* layer)
	{
		UME_PROFILE_FUNCTION();

		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::Close()
	{
		UME_CORE_WARN("[Application] Close() called.");
		m_Running = false;
	}
	void Application::SubmitToMainThread(const std::function<void()>& function)
	{
		std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

		m_MainThreadQueue.emplace_back(function);
	}

	void Application::OnEvent(Event& e)
	{
		UME_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(UME_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(UME_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (e.Handled) 
				break;
			(*it)->OnEvent(e);
		}
	}

	void Application::Run()
	{
		UME_PROFILE_FUNCTION();

		while (m_Running)
		{
			UME_PROFILE_SCOPE("RunLoop");

			float time = Time::GetTime();
			float frameDelta = time - m_LastFrameTime;
			if (m_LastFrameTime <= 0.0f || frameDelta < 0.0f)
				frameDelta = 1.0f / 60.0f;

			// Prevent large stalls from propagating into simulation/camera jitter.
			frameDelta = std::min(frameDelta, 0.05f);
			Timestep timestep = frameDelta;
			m_LastFrameTime = time;

			ExecuteMainThreadQueue();

			if (!m_Minimized)
			{
				{
					UME_PROFILE_SCOPE("LayerStack OnUpdate");

					for (Layer* layer : m_LayerStack)
						layer->OnUpdate(timestep);
				}
			}

			m_ImGuiLayer->Begin();
			{
				UME_PROFILE_SCOPE("LayerStack OnImGuiRender");

				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender();
			}
			m_ImGuiLayer->End();

			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		UME_CORE_WARN("[Application] WindowCloseEvent received.");
		m_Running = false;
		return true;
	}
	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		UME_PROFILE_FUNCTION();

		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

		return false;
	}

	void Application::ExecuteMainThreadQueue()
	{
		std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

		for (auto& func : m_MainThreadQueue)
			func();

		m_MainThreadQueue.clear();
	}

}
