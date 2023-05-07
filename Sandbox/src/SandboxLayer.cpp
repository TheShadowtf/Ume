#include "SandboxLayer.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui/imgui.h"
#include <chrono>

EditorLayer::EditorLayer() : Layer("UmeEditor"), m_CameraController(1280.0f / 720.0f, true)
{

}

void EditorLayer::OnAttach()
{
	UME_PROFILE_FUNC();

	m_TextureCheck = Ume::Texture2D::Create("assets/textures/Checkerboard.png");

	m_CameraController.SetZoomLevel(5.0f);
}

void EditorLayer::OnDetach()
{
	UME_PROFILE_FUNC();
}

void EditorLayer::OnImGuiRender()
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

void EditorLayer::OnUpdate(Ume::Timestep ts)
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
		Ume::Renderer2D::DrawQuad({ 0.0f, 0.0f, -0.2f }, { 20.0f, 20.0f }, m_TextureCheck, 10.0f);

		Ume::Renderer2D::DrawQuad({ -2.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, m_TextureCheck, glm::radians(glm::vec1(rotation)), 10.0f);
		Ume::Renderer2D::DrawQuad({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, m_TextureCheck, 10.0f);

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
}

void EditorLayer::OnEvent(Ume::Event& e)
{
	m_CameraController.OnEvent(e);
}