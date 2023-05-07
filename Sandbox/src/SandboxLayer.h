#pragma once

#include <Ume.h>

class EditorLayer : public Ume::Layer
{
public:
	EditorLayer();
	virtual ~EditorLayer() = default;
	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnImGuiRender() override;
	void OnUpdate(Ume::Timestep ts) override;
	void OnEvent(Ume::Event& e) override;
private:
	Ume::OrthoCameraController m_CameraController;

	Ume::Ref<Ume::VertexArray>  m_SquareVA;
	Ume::Ref<Ume::Shader>       m_FlatColorShader;
	Ume::Ref<Ume::Texture2D>    m_TextureCheck;

	glm::vec4 m_SquareColor = { 1.0f, 1.0f, 1.0f, 1.0f };
};
