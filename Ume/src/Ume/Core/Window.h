#pragma once

#include "Ume/Core/Base.h"
#include "Ume/Events/Event.h"

#include <sstream>

namespace Ume {

	struct WindowProps
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;
		bool Visible;

		WindowProps(const std::string& title = "Ume Engine", uint32_t width = 1600, uint32_t height = 900, bool visible = true)
			: Title(title), Width(width), Height(height), Visible(visible)
		{
		}
	};

	// Interface representing a desktop system based Window
	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() = default;

		virtual void OnUpdate() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;
		virtual void Show() = 0;
		virtual void Hide() = 0;

		virtual void* GetNativeWindow() const = 0;

		static Scope<Window> Create(const WindowProps& props = WindowProps());
	};

}
