#include <umepch.h>
#include "OrthoCameraController.h"

#include "Ume/Core/Input.h"
#include "Ume/Core/KeyCodes.h"

namespace Ume
{

	OrthoCameraController::OrthoCameraController(float aspectRatio, bool rotation)
		: m_AspectRatio(aspectRatio), m_Bounds({ -m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel }), m_Camera(m_Bounds.Left, m_Bounds.Right, m_Bounds.Bottom, m_Bounds.Top), m_Rotation(rotation)
	{
	}

	void OrthoCameraController::OnUpdate(Timestep ts)
	{
		UME_PROFILE_FUNC();

		if (Input::IsKeyPressed(A))
		{
			m_CameraPos.x -= cos(glm::radians(m_CameraRot)) * m_CameraTransSpeed * ts;
			m_CameraPos.y -= sin(glm::radians(m_CameraRot)) * m_CameraTransSpeed * ts;
		}
		else if (Input::IsKeyPressed(D))
		{

			m_CameraPos.x += cos(glm::radians(m_CameraRot)) * m_CameraTransSpeed * ts;
			m_CameraPos.y += sin(glm::radians(m_CameraRot)) * m_CameraTransSpeed * ts;
		}

		if (Input::IsKeyPressed(S))
		{

			m_CameraPos.y -= cos(glm::radians(m_CameraRot)) * m_CameraTransSpeed * ts;
			m_CameraPos.x -= -sin(glm::radians(m_CameraRot)) * m_CameraTransSpeed * ts;
		}
		else if (Input::IsKeyPressed(W))
		{
			m_CameraPos.y -= -cos(glm::radians(m_CameraRot)) * m_CameraTransSpeed * ts;
			m_CameraPos.x -= sin(glm::radians(m_CameraRot)) * m_CameraTransSpeed * ts;
		}

		if (m_Rotation)
		{
			if (Input::IsKeyPressed(Q))
				m_CameraRot -= m_CameraRotSpeed * ts;
			if (Input::IsKeyPressed(E))
				m_CameraRot += m_CameraRotSpeed * ts;

			if (m_CameraRot > 180.0f)
				m_CameraRot -= 360.0f;
			else if (m_CameraRot <= -180.0f)
				m_CameraRot += 360.0f;

			m_Camera.SetRotation(m_CameraRot);
		}

		m_Camera.SetPosition(m_CameraPos);

		m_CameraTransSpeed = m_ZoomLevel;
	}

	void OrthoCameraController::OnEvent(Event& e)
	{
		UME_PROFILE_FUNC();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(UME_BIND_EVENT_FN(OrthoCameraController::OnMouseScrolled));
		dispatcher.Dispatch<WindowResizeEvent>(UME_BIND_EVENT_FN(OrthoCameraController::OnWindowResized));
	}

	void OrthoCameraController::CalculateView()
	{
		m_Bounds = { -m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel };
		m_Camera.SetProjection(m_Bounds.Left, m_Bounds.Right, m_Bounds.Bottom, m_Bounds.Top);
	}

	bool OrthoCameraController::OnMouseScrolled(MouseScrolledEvent& e)
	{
		UME_PROFILE_FUNC();

		m_ZoomLevel -= e.GetYOffset() * 0.25f;
		m_ZoomLevel = std::max(m_ZoomLevel, 0.25f);
		CalculateView();

		return false;
	}

	bool OrthoCameraController::OnWindowResized(WindowResizeEvent& e)
	{
		UME_PROFILE_FUNC();

		Resize((float)e.GetWidth(), (float)e.GetHeight());
		return false;
	}

	void OrthoCameraController::Resize(float width, float height)
	{
		m_AspectRatio = width / height;
		CalculateView();
	}

}