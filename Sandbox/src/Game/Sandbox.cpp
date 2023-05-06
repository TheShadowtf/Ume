#include <Ume.h>
#include <Ume/Core/EntryPoint.h>
#include "Platform/OpenGL/OpenGLShader.h"
#include <umepch.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui/imgui.h"

//#include "Sandbox2D.h"

class TestLayer : public Ume::Layer
{
public:
	TestLayer() : Layer("Test"), m_CameraController(1280.0f / 720.0f, true)
	{
		m_VertexArray = Ume::VertexArray::Create();

		float vertices[3 * 7] =
		{
			-0.5f, -0.5f, 0.0f, 0.8f, 0.2f, 0.8f, 1.0f,
			 0.5f, -0.5f, 0.0f, 0.2f, 0.3f, 0.8f, 1.0f,
			 0.0f,  0.5f, 0.0f, 0.8f, 0.8f, 0.2f, 1.0f
		};

		Ume::Ref<Ume::VertexBuffer> vertexBuffer;
		vertexBuffer = Ume::VertexBuffer::Create(vertices, sizeof(vertices));

		Ume::BufferLayout layout =
		{
			{ Ume::ShaderDataType::Float3, "a_Position" },
			{ Ume::ShaderDataType::Float4, "a_Color" }
		};

		vertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(vertexBuffer);

		unsigned int indices[3] =
		{
			0, 1, 2
		};

		Ume::Ref<Ume::IndexBuffer> indexBuffer;
		indexBuffer = Ume::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
		m_VertexArray->SetIndexBuffer(indexBuffer);

		m_SquareVertexArray = Ume::VertexArray::Create();

		float squareVertices[5 * 4] =
		{
			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f, 0.0f, 1.0f
		};

		Ume::Ref<Ume::VertexBuffer> squareVB;
		squareVB = Ume::VertexBuffer::Create(squareVertices, sizeof(squareVertices));

		squareVB->SetLayout({ 
			{ Ume::ShaderDataType::Float3, "a_Position" },
			{ Ume::ShaderDataType::Float2, "a_TexCoord" }
		});
		m_SquareVertexArray->AddVertexBuffer(squareVB);

		unsigned int sqaureIndices[6] =
		{
			0, 1, 2, 2, 3, 0
		};

		Ume::Ref<Ume::IndexBuffer> squareIB;
		squareIB = Ume::IndexBuffer::Create(sqaureIndices, sizeof(sqaureIndices) / sizeof(uint32_t));
		m_SquareVertexArray->SetIndexBuffer(squareIB);

		// Shader
		std::string vertexShader = R"(
			#version 330 core

			layout(location = 0) in vec3 a_Position;
			layout(location = 1) in vec4 a_Color;

			uniform mat4 u_ViewProjection;
			uniform mat4 u_Transform;

			out vec3 v_Position;
			out vec4 v_Color;
			
			void main()
			{
				v_Color = a_Color;
				v_Position = a_Position;
				gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
			}
		)";

		std::string fragmentShader = R"(
			#version 330 core
			
			out vec4 outColor;
			in vec3 v_Position;
			in vec4 v_Color;

			void main() 
			{
				outColor = v_Color;
			}
		)";

		m_Shader = Ume::Shader::Create("VertexPosColor", vertexShader, fragmentShader);

		// Flat Shader
		std::string flatColorVertexShader = R"(
			#version 330 core

			layout(location = 0) in vec3 a_Position;

			uniform mat4 u_ViewProjection;
			uniform mat4 u_Transform;

			out vec3 v_Position;
			
			void main()
			{
				v_Position = a_Position;
				gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
			}
		)";

		std::string flatColorFragmentShader = R"(
			#version 330 core
			
			layout(location = 0) out vec4 color;

			uniform vec3 u_Color;

			void main() 
			{
				color = vec4(u_Color, 1.0);
			}
		)";

		m_FlatColorShader = Ume::Shader::Create("FlatColor", flatColorVertexShader, flatColorFragmentShader);

		// Texture Shader


		auto m_TextureShader = m_ShaderLib.Load("assets/shaders/Texture.glsl");

		m_Texture = Ume::Texture2D::Create("assets/textures/Checkerboard.png");
		m_TestLogoTexture = Ume::Texture2D::Create("assets/textures/logo.png");

		std::dynamic_pointer_cast<Ume::OpenGlShader>(m_TextureShader)->Bind();
		std::dynamic_pointer_cast<Ume::OpenGlShader>(m_TextureShader)->UploadUniformInt("u_Texture", 0);
	}

	void OnUpdate(Ume::Timestep ts) override
	{
		m_CameraController.OnUpdate(ts);
		//UME_TRACE("Delta time: {0}s ({1}ms)", ts.GetSeconds(), ts.GetMilliseconds());

		Ume::RendererCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });
		Ume::RendererCommand::Clear();

		Ume::Renderer::BeginScene(m_CameraController.GetCamera());

		glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));

		std::dynamic_pointer_cast<Ume::OpenGlShader>(m_FlatColorShader)->Bind();
		std::dynamic_pointer_cast<Ume::OpenGlShader>(m_FlatColorShader)->UploadUniformFloat3("u_Color", m_SquareColor);

		for (int y = 0; y < 20; y++)
		{
			for (int x = 0; x < 20; x++)
			{
				glm::vec3 pos(x * 0.11f, y * 0.11f, 0.0f);
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * scale;
				Ume::Renderer::Submit(m_SquareVertexArray, m_FlatColorShader, transform);
			}
		}

		auto textureShader = m_ShaderLib.Get("Texture");

		m_Texture->Bind(0);
		Ume::Renderer::Submit(m_SquareVertexArray, textureShader, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));
		m_TestLogoTexture->Bind(0);
		Ume::Renderer::Submit(m_SquareVertexArray, textureShader, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));

		//Ume::Renderer::Submit(m_VertexArray, m_Shader);

		Ume::Renderer::EndScene();
	}

	virtual void OnImGuiRender() override
	{
		//ImGui::Begin("Setting");
		//ImGui::ColorEdit3("Square Color", glm::value_ptr(m_SquareColor));
		//ImGui::End();
	}

	void OnEvent(Ume::Event& e) override
	{
		m_CameraController.OnEvent(e);
	}

private:
	Ume::ShaderLibrary m_ShaderLib;
	Ume::Ref<Ume::Shader>       m_Shader;
	Ume::Ref<Ume::Shader>       m_FlatColorShader;
	Ume::Ref<Ume::VertexArray>  m_VertexArray;
	Ume::Ref<Ume::VertexArray>  m_SquareVertexArray;
	Ume::Ref<Ume::Texture2D>    m_Texture, m_TestLogoTexture;

	Ume::OrthoCameraController m_CameraController;

	glm::vec3 m_SquareColor = { 0.02f, 0.3f, 0.8f };
};

class Sandbox : public Ume::Application
{
public:
	Sandbox()
	{
		//PushLayer(new TestLayer());
		//PushLayer(new Sandbox2D());
;	}

	~Sandbox()
	{

	}
};

//Ume::Application* Ume::CreateApplication()
//{
//	return new Sandbox();
//}