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

		void Begin();
		void End();
	private:
		float m_Time = 0.0f;
	};
}