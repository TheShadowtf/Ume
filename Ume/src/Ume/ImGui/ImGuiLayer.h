#pragma once

#include "Ume/Core/Layer.h"

#include "Ume/Events/ApplicationEvent.h"
#include "Ume/Events/KeyEvent.h"
#include "Ume/Events/MouseEvent.h"

#include <string>

namespace Ume {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		void Begin();
		void End();

		void BlockEvents(bool block) { m_BlockEvents = block; }
		
		void SetDarkThemeColors();
		void SetLightThemeColors();
		void SetHazelDarkThemeColors();

		uint32_t GetActiveWidgetID() const;

	private:
		void LoadCustomFonts();
		void UploadFontsTexture();
	private:
		bool m_BlockEvents = true;
		bool m_PendingCustomFontLoad = true;
		int m_FrameCounter = 0;
		std::string m_IniFilePath;
	};

}
