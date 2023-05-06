#include "GameLayer.h"

#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Ume/Math/Random.h"

using namespace Ume;

GameLayer::GameLayer() : Layer("Game")
{
	auto& window = Application::Get().GetWindow();
	CreateCamera(window.GetWidth(), window.GetHeight());

	//Random::Init();
}

void GameLayer::OnAttach()
{
	//ImGuiIO io = ImGui::GetIO();
	//m_Font = io.Fonts->AddFontFromFileTTF("assets/OpenSans-Regular.ttf", 120.0f);
}

void GameLayer::OnDetach()
{

}

void GameLayer::OnImGuiRender()
{

}

void GameLayer::OnUpdate(Ume::Timestep ts)
{
	m_Time += ts;
	if ((int)(m_Time * 10.0f) % 8 > 4)
		m_Blink = !m_Blink;
	m_Camera->SetPosition({ 0.0f, 0.0f, 0.0f });

	Ume::RendererCommand::Clear({ 0.2f, 0.2f, 0.2f, 1});
	Ume::RendererCommand::Clear();

	Ume::Renderer2D::BeginScene(*m_Camera);
	Ume::Renderer2D::EndScene();
}

void GameLayer::OnEvent(Ume::Event& e)
{

}

bool GameLayer::OnMouseButtonPressed(Ume::MouseButtonEvent& e)
{
	m_State = GameState::Play;
	return false;
}

bool GameLayer::OnWindowResize(Ume::WindowResizeEvent& e)
{
	CreateCamera(e.GetWidth(), e.GetHeight());
	return false;
}

void GameLayer::CreateCamera(uint32_t width, uint32_t height)
{
	float aspectRatio = (float)width / (float)height;

	float camWidth = 8.0f;
	float bottom = -camWidth;
	float top = camWidth;
	float left = bottom * aspectRatio;
	float right = top * aspectRatio;
	m_Camera = CreateScope<OrthographicCamera>(left, right, bottom, top);
}
