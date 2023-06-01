#pragma once

#include "Ume/Renderer/Camera.h"

namespace Ume
{
	class SceneCamera : public Camera
	{
	public:
		enum class ProjectionType
		{
			Perspective = 0,
			Orthographic = 1
		};
	public:
		SceneCamera();
		virtual ~SceneCamera() = default;

		void SetOrtho(float size, float nearClip, float farCip);
		void SetPerspective(float fov, float nearClip, float farCip);

		void SetViewportSize(uint32_t width, uint32_t height);

		float GetPerspectiveSize() const { return m_PerspectiveFOV; }
		void  SetPerspectiveSize(float fov) { m_PerspectiveFOV = fov; RecalculateProjection(); }
		float GetPerspectiveNearCip() const { return m_PerspectiveNear; }
		void  SetPerspectiveNearCip(float nearClip) { m_PerspectiveNear = nearClip; RecalculateProjection(); }
		float GetPerspectiveFarCip() const { return m_PerspectiveFar; }
		void  SetPerspectiveFarCip(float farClip) { m_PerspectiveFar = farClip; RecalculateProjection(); }

		float GetOrthoSize() const { return m_OrthoSize; }
		void  SetOrthoSize(float orthoSize) { m_OrthoSize = orthoSize; RecalculateProjection(); }
		float GetOrthoNearCip() const { return m_OrthoNear; }
		void  SetOrthoNearCip(float nearClip) { m_OrthoNear = nearClip; RecalculateProjection(); }
		float GetOrthoFarCip() const { return m_OrthoFar; }
		void  SetOrthoFarCip(float farClip) { m_OrthoFar = farClip; RecalculateProjection(); }

		ProjectionType GeProjectionType() const { return m_ProjectionType; }
		void SetProjection(ProjectionType type) { m_ProjectionType = type; RecalculateProjection(); }
	private:
		void RecalculateProjection();
	private:
		ProjectionType m_ProjectionType = ProjectionType::Orthographic;

		float m_OrthoSize = 10.0f;
		float m_OrthoNear = -1.0f;
		float m_OrthoFar  = 1.0f;

		float m_PerspectiveFOV = glm::radians(45.0f);
		float m_PerspectiveNear = 0.01f;
		float m_PerspectiveFar = 1000.0f;

		float m_AspectRatio = 1.0f;
	};
}