#include <umepch.h>
#include "OrthoCameraController.h"

#include "Ume/Input.h"
#include "Ume/KeyCodes.h"

namespace Ume
{

	OrthoCameraController::OrthoCameraController(float aspectRatio, bool rotation) : m_AspectRatio(aspectRatio), m_Camera(-m_AspectRatio * m_ZoomLevel, m_AspectRatio* m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel), m_Rotation(rotation)
	{

	}

	void OrthoCameraController::OnUpdate(Timestep ts)
	{
		if (Input::IsKeyPressed(A))
			m_CameraPos.x -= m_CameraTransSpeed * ts;
		else if (Input::IsKeyPressed(D))
			m_CameraPos.x += m_CameraTransSpeed * ts;

		if (Input::IsKeyPressed(S))
			m_CameraPos.y -= m_CameraTransSpeed * ts;
		else if (Input::IsKeyPressed(W))
			m_CameraPos.y += m_CameraTransSpeed * ts;

		if (m_Rotation)
		{
			if (Input::IsKeyPressed(Q))
				m_CameraRot -= m_CameraRotSpeed * ts;
			if (Input::IsKeyPressed(E))
				m_CameraRot += m_CameraRotSpeed * ts;

			m_Camera.SetRotation(m_CameraRot);
		}

		m_Camera.SetPosition(m_CameraPos);

		m_CameraTransSpeed = m_ZoomLevel;
	}

	void OrthoCameraController::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(UME_BIND_EVENT_FN(OrthoCameraController::OnMouseScrolled));
		dispatcher.Dispatch<WindowResizeEvent>(UME_BIND_EVENT_FN(OrthoCameraController::OnWindowResized));
	}

	bool OrthoCameraController::OnMouseScrolled(MouseScrolledEvent& e)
	{
		m_ZoomLevel -= e.GetYOffset() * 0.25f;
		m_ZoomLevel = std::max(m_ZoomLevel, 0.25f);
		m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
		return false;
	}

	bool OrthoCameraController::OnWindowResized(WindowResizeEvent& e)
	{
		m_AspectRatio = (float)e.GetWidth() / (float)e.GetHeight();
		m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
		return false;
	}

}