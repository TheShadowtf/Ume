#include "Sandbox2D.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui/imgui.h"
#include <chrono>

Sandbox2D::Sandbox2D() : Layer("Sandbox2D"), m_CameraController(1280.0f / 720.0f, true)
{

}

void Sandbox2D::OnAttach()
{
	UME_PROFILE_FUNC();

	m_Texture = Ume::Texture2D::Create("assets/textures/Checkerboard.png");

	m_Particle.ColorBegin = { 215 / 255.0f, 53 / 255.0f, 2 / 255.0f, 1.0f };
	m_Particle.ColorEnd = { 250 / 255.0f, 192 / 117, 0 / 255.0f, 1.0f };
	m_Particle.SizeBegin = 0.5f, m_Particle.SizeVariation = 0.3f, m_Particle.SizeEnd = 0.0f;
	m_Particle.LifeTime = 5.0f;
	m_Particle.Velocity = { 0.0f, 0.0f };
	m_Particle.VelocityVariation = { 3.0f, 1.0f };
	m_Particle.Position = { 0.0f, 0.0f, 0.1f };
}

void Sandbox2D::OnDetach()
{
	UME_PROFILE_FUNC();
}

void Sandbox2D::OnImGuiRender()
{
	ImGui::Begin("Setting");
	
	auto stats = Ume::Renderer2D::GetStats();
	ImGui::Text("Renderer2D Stats:");
	ImGui::Text("DrawCalls: %d", stats.drawCalls);
	ImGui::Text("Quads: %d", stats.quadCount);
	ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
	ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

	ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));
	
	ImGui::End();
}

void Sandbox2D::OnUpdate(Ume::Timestep ts)
{
	UME_PROFILE_FUNC("Sandbox2D::OnUpdate");

	m_CameraController.OnUpdate(ts);

	static float rotation = 0.0f;
	rotation += ts * 50.0f;

	Ume::Renderer2D::ResetStats();
	{
		UME_PROFILE_SCOPE("Renderer Prep");
		Ume::RendererCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });
		Ume::RendererCommand::Clear();
	}

	{
		UME_PROFILE_FUNC("Renderer Draw");
		Ume::Renderer2D::BeginScene(m_CameraController.GetCamera());

		Ume::Renderer2D::DrawQuad({ 0.5f, -0.5f, 0.0f }, { 0.5f, 0.5f }, { 0.2f, 0.3f, 0.8f, 1.0f });
		Ume::Renderer2D::DrawQuad({ -1.0f, 0.0f, 0.0f }, { 0.5f, 0.5f }, { 0.8f, 0.2f, 0.3f, 1.0f });
		Ume::Renderer2D::DrawQuad({ -1.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, m_SquareColor, 1.0f);
		Ume::Renderer2D::DrawQuad({ 0.0f, 0.0f, -0.2f }, { 20.0f, 20.0f }, m_Texture, 10.0f);

		Ume::Renderer2D::DrawQuad({ -2.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, m_Texture, glm::radians(glm::vec1(rotation)), 10.0f);
		Ume::Renderer2D::DrawQuad({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, m_Texture, 10.0f);

		Ume::Renderer2D::EndScene();


		Ume::Renderer2D::BeginScene(m_CameraController.GetCamera());

		for (float y = -5.0f; y < 5.0f; y += 0.5f)
		{
			for (float x = -5.0f; x < 5.0f; x += 0.5f)
			{
				glm::vec4 color = { (x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 0.7f };
				Ume::Renderer2D::DrawQuad({ x, y }, { 0.45f, 0.45f }, color);
			}
		}

		Ume::Renderer2D::EndScene();
	}

	if (Ume::Input::IsMouseButtonPressed(Ume::MouseButtonLeft))
	{
		auto [x, y] = Ume::Input::GetMousePos();
		auto width = Ume::Application::Get().GetWindow().GetWidth();
		auto height = Ume::Application::Get().GetWindow().GetHeight();

		auto bounds = m_CameraController.GetBounds();
		auto pos = m_CameraController.GetCamera().GetPosition();
		x = (x / width) * bounds.GetWidth() - bounds.GetWidth() * 0.5f;
		y = bounds.GetHeight() * 0.5f - (y / height) * bounds.GetHeight();
		m_Particle.Position = { x + pos.x, y + pos.y, m_Particle.Position.z };
		for (int i = 0; i < 50; i++)
			m_ParticleSystem.Emit(m_Particle);
	}

	m_ParticleSystem.OnUpdate(ts);
	m_ParticleSystem.OnRender(m_CameraController.GetCamera());

}

void Sandbox2D::OnEvent(Ume::Event& e)
{
	m_CameraController.OnEvent(e);
}

class Sandbox : public Ume::Application
{
public:
	Sandbox()
	{
		//PushLayer(new TestLayer());
		PushLayer(new Sandbox2D());
	}

	~Sandbox()
	{

	}
};

Ume::Application* Ume::CreateApplication()
{
	return new Sandbox();
}