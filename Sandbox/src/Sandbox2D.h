#pragma once

#include <Ume.h>

#include "ParticleSystem.h"

class Sandbox2D : public Ume::Layer
{
public:
	Sandbox2D();
	virtual ~Sandbox2D() = default;
	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnImGuiRender() override;
	void OnUpdate(Ume::Timestep ts) override;
	void OnEvent(Ume::Event& e) override;
private:
	Ume::OrthoCameraController m_CameraController;

	Ume::Ref<Ume::VertexArray>  m_SquareVA;
	Ume::Ref<Ume::Shader>       m_FlatColorShader;
	Ume::Ref<Ume::Texture2D>    m_Texture;

	glm::vec4 m_SquareColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	ParticleSystem m_ParticleSystem;
	ParticleProps m_Particle;
};