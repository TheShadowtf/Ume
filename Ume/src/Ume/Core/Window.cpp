#include "umepch.h"
#include "Ume/Core/Window.h"

#ifdef UME_PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsWindow.h"
#endif

namespace Ume
{
	Scope<Window> Window::Create(const WindowProps& props)
	{
	#ifdef UME_PLATFORM_WINDOWS
		return CreateScope<WindowsWindow>(props);
	#else
		UME_CORE_ASSERT(false, "Unknown platform!");
		return nullptr;
	#endif
	}

}