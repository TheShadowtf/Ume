#pragma once

#include "Ume/Layer.h"
#include "Ume/Events/ApplicationEvent.h"
#include "Ume/Events/KeyEvent.h"
#include "Ume/Events/MouseEvent.h"

namespace Ume
{
	class UME_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;

		void Begin();
		void End();
	private:
		float m_Time = 0.0f;
	};
}