#pragma once

#include "Ume/Renderer/OrthographicCamera.h"
#include "Ume/Core/Timestep.h"
#include "Ume/Events/Event.h"
#include "Ume/Events/ApplicationEvent.h"
#include "Ume/Events/MouseEvent.h"

namespace Ume
{
	struct OrthographicCameraBounds
	{
		float Left, Right;
		float Bottom, Top;

		float GetWidth() { return Right - Left; }
		float GetHeight() { return Top - Bottom; }
	};

	class OrthoCameraController
	{
	public:
		OrthoCameraController(float aspectRatio, bool rotation = false);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		OrthographicCamera& GetCamera() { return m_Camera; }
		const OrthographicCamera& GetCamera() const { return m_Camera; }

		void SetZoomLevel(float level) { m_ZoomLevel = level; }
		float GetZoomLevel() const { return m_ZoomLevel; }

		const OrthographicCameraBounds& GetBounds() const { return m_Bounds; }
	private:
		bool OnMouseScrolled(MouseScrolledEvent& e);
		bool OnWindowResized(WindowResizeEvent& e);
	private:
		float m_AspectRatio;
		float m_ZoomLevel = 1.0f;
		OrthographicCameraBounds m_Bounds;
		OrthographicCamera m_Camera;

		bool m_Rotation;
		glm::vec3 m_CameraPos = { 0.0f, 0.0f, 0.0f };
		float m_CameraRot = 0.0f;
		float m_CameraTransSpeed = 5.0f, m_CameraRotSpeed = 180.0f;
	};
}