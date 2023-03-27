#include <umepch.h>
#include <glad/glad.h>

#include "Application.h"

#include "Input.h"

namespace Ume
{
#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;
	Application::Application()
	{
		UME_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

		m_ImGuiLayer = new ImGuiLayer();
		pushOverlay(m_ImGuiLayer);

		glGenVertexArrays(1, &m_VertexArray);
		glBindVertexArray(m_VertexArray);

		glGenBuffers(1, &m_VertexArray);
		glBindBuffer(GL_ARRAY_BUFFER, m_VertexArray);

		float vertices[3 * 3] =
		{
			-0.5f, -0.5f, 0.0f,
			 0.5f, -0.5f, 0.0f,
			 0.0f,  0.5f, 0.0f
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

		glGenBuffers(1, &m_IndexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);

		unsigned int indices[3] =
		{
			0, 1, 2
		};

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		std::string vertexShader = R"(
			#version 330 core

			layout(location = 0) in vec3 a_Position;

			out vec3 v_Position;
			
			void main()
			{
				v_Position = a_Position;
				gl_Position = vec4(a_Position, 1.0);
			}
		)";

		std::string fragmentShader = R"(
			#version 330 core

			layout(location = 0) out vec4 a_Color;
			in vec3 v_Position;

			uniform float time;

			uint hash( uint x ) {
				x += ( x << 10u );
				x ^= ( x >>  6u );
				x += ( x <<  3u );
				x ^= ( x >> 11u );
				x += ( x << 15u );
				return x;
			}

			uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
			uint hash( uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
			uint hash( uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }

			float floatConstruct( uint m ) {
				const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
				const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

				m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
				m |= ieeeOne;                          // Add fractional part to 1.0

				float  f = uintBitsToFloat( m );       // Range [1:2]
				return f - 1.0;                        // Range [0:1]
			}

			float random( float x ) { return floatConstruct(hash(floatBitsToUint(x))); }
			float random( vec2  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
			float random( vec3  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
			float random( vec4  v ) { return floatConstruct(hash(floatBitsToUint(v))); }

			void main()
			{
				vec3  inputs = vec3( gl_FragCoord.xy, time ); // Spatial and temporal inputs
				float rand   = random( inputs );              // Random per-pixel value
				vec3  luma   = vec3( rand );                  // Expand to RGB

				a_Color = vec4( luma, 1.0 );
			}
		)";

		m_Shader.reset(new Shader(vertexShader, fragmentShader));
	}

	Application::~Application()
	{

	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::pushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClosed));
		//UME_CORE_TRACE("{0}", e);

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);
			if (e.m_Handled)
				break;
		}
	}
	
	void Application::Run()
	{
		while (m_Running)
		{
			glClearColor(0.1f, 0.1f, 0.1f, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			m_Shader->Bind();
			glBindVertexArray(m_VertexArray);
			glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);

			for (Layer* layer : m_LayerStack)
				layer->OnUpdate();
			m_ImGuiLayer->Begin();
			for (Layer* layer : m_LayerStack)
				layer->OnImGuiRender();
			m_ImGuiLayer->End();
			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClosed(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}
}