#pragma once

#include "Ume/Core/Layers/Layer.h"
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
		virtual void OnEvent(Event& e) override;

		void Begin();
		void End();

		void BlockImGuiEvents(bool block) { m_BlockImGuiEvents = block; }
	private:
		bool m_BlockImGuiEvents = true;
		float m_Time = 0.0f;
	};
}