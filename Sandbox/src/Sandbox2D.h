#pragma once

#include "Ume.h"

class Sandbox2D : public Ume::Layer
{
public:
	Sandbox2D();
	virtual ~Sandbox2D() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Ume::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Ume::Event& e) override;
private:
	Ume::OrthographicCameraController m_CameraController;
	
	// Temp
	Ume::Ref<Ume::VertexArray> m_SquareVA;
	Ume::Ref<Ume::Shader> m_FlatColorShader;

	Ume::Ref<Ume::Texture2D> m_CheckerboardTexture;

	glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.8f, 1.0f };
};