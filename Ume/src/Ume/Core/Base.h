#pragma once

#include "Ume/Core/PlatformDetection.h"

#include <memory>

#ifdef UME_DEBUG
	#if defined(UME_PLATFORM_WINDOWS)
		#define UME_DEBUGBREAK() __debugbreak()
	#elif defined(UME_PLATFORM_LINUX)
		#include <signal.h>
		#define UME_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define UME_ENABLE_ASSERTS
#else
	#define UME_DEBUGBREAK()
#endif

#define UME_EXPAND_MACRO(x) x
#define UME_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define UME_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Ume {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}

#include "Ume/Core/Log.h"
#include "Ume/Core/Assert.h"
