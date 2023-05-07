#pragma once

#include <Ume.h>

namespace Ume
{
	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer() = default;
		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;
		void OnUpdate(Timestep ts) override;
		void OnEvent(Event& e) override;
	private:
		OrthoCameraController m_CameraController;

		Ref<VertexArray>  m_SquareVA;
		Ref<Shader>       m_FlatColorShader;
		Ref<Texture2D>    m_TextureCheck;
		Ref<Framebuffer>  m_FrameBuffer;

		Ref<Scene> m_ActiveScene;
		entt::entity m_SqareEntity;

		bool m_ViewportFocused = false, m_ViewportHovered = false;

		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };

		glm::vec4 m_SquareColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	};
}
