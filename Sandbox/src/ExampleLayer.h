#pragma once

#include "Ume.h"

class ExampleLayer : public Ume::Layer
{
public:
	ExampleLayer();
	virtual ~ExampleLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Ume::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Ume::Event& e) override;
private:
	Ume::ShaderLibrary m_ShaderLibrary;
	Ume::Ref<Ume::Shader> m_Shader;
	Ume::Ref<Ume::VertexArray> m_VertexArray;

	Ume::Ref<Ume::Shader> m_FlatColorShader;
	Ume::Ref<Ume::VertexArray> m_SquareVA;

	Ume::Ref<Ume::Texture2D> m_Texture, m_UmeLogoTexture;

	Ume::OrthographicCameraController m_CameraController;
	glm::vec3 m_SquareColor = { 0.2f, 0.3f, 0.8f };
};

